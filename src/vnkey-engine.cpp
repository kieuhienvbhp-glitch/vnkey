/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2025 Võ Ngô Hoàng Thành <thanhpy2009@gmail.com>
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 * SPDX-FileCopyrightText: 2026 kimxuanhong <kimxuanhong@outlook.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#include "vnkey-engine.h"
#include "common-fd-guard.h"
#include "fcitx-utils/keysym.h"
#include "vnkey-config.h"
#include "vnkey-log.h"
#include "vnkey-mode-menu.h"
#include "vnkey-state.h"
#include "vnkey-candidates.h"
#include "vnkey-monitor.h"
#include "vnkey-notify.h"
#include "vnkey-utils.h"
#include "vnkey-x11.h"
#include "vnkey-plasma-theme.h"
#include "vnkey-wlr-virtual-keyboard.h"
#include "vnkey-x11-keyboard.h"
#include "window_focus_tracker.h"

#include "dictionary.h"
#include "emoji.h"
#include "macro_table.h"
#include "utf8.h"

#include <optional>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include <fcitx-config/iniparser.h>
#include <fcitx/menu.h>
#include <fcitx/userinterfacemanager.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/utf8.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_set>

#include <fcntl.h>
#include <sstream>

namespace fcitx {
    constexpr const char* CharsetActionPrefix = "vnkey-charset-";
    constexpr const char* CustomKeymapFile    = "conf/vnkey-custom-keymap.conf";
    constexpr const char* MacroTableFile      = "conf/vnkey-macro-table.conf";

    EmojiLoader&          VNKeyEngine::emojiLoader() {
        if (!emojiLoader_) {
            emojiLoader_ = std::make_unique<EmojiLoader>(&instance_.addonManager());
        }
        return *emojiLoader_;
    }

    bool VNKeyEngine::isDarkMode() {
        // Each probe spawns subprocesses, and subModeIconImpl calls this on
        // every tray update while IconTheme is Auto.  Cache the result briefly
        // so the cost is paid at most once per few seconds.
        static int64_t lastCheckMs = 0;
        static bool    cachedValue = false;
        const int64_t  now         = nowMs();
        if (now - lastCheckMs < 5000) {
            return cachedValue;
        }
        lastCheckMs = now;
        cachedValue = false;

        // KDE Plasma: the tray sits on the panel, painted by the Plasma Style,
        // while the portal below reports the application colour scheme.  The
        // two differ in the default Fedora/Kubuntu look (#374).
        if (isKdePlasmaSession(getEnv("XDG_CURRENT_DESKTOP"))) {
            if (const auto dark = isPlasmaPanelDark(plasmaThemeSearchPathsFromEnv())) {
                cachedValue = *dark;
                return cachedValue;
            }
        }

        // GTK_THEME is honored by lightweight DEs that lack the settings
        // portal; covers XFCE, openbox, etc. with a dark theme.
        if (std::string theme = getEnv("GTK_THEME"); !theme.empty()) {
            std::ranges::transform(theme, theme.begin(), ::tolower);
            if (theme.find("dark") != std::string::npos) {
                cachedValue = true;
                return cachedValue;
            }
        }

        const auto openPipe = [](const char* command) { return std::unique_ptr<FILE, int (*)(FILE*)>(popen(command, "r"), pclose); };

        auto pipe = openPipe("dbus-send --session --dest=org.freedesktop.portal.Desktop --print-reply /org/freedesktop/portal/desktop org.freedesktop.portal.Settings.ReadOne "
                             "string:'org.freedesktop.appearance' string:'color-scheme' 2>/dev/null");
        if (pipe) {
            std::array<char, 256> buffer{};
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                uint32_t value = 0;
                // The sscanf return value is checked and the value is only compared against 1.
                if (sscanf(buffer.data(), "%*[^v]variant uint32 %u", &value) == 1) { //NOLINT(bugprone-unchecked-string-to-number-conversion)
                    cachedValue = value == 1;
                    return cachedValue;
                }
            }
        }

        pipe = openPipe("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null");
        if (pipe) {
            std::array<char, 256> buffer{};
            if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                cachedValue = strstr(buffer.data(), "prefer-dark") != nullptr;
                return cachedValue;
            }
        }

        // GTK settings file — covers DEs where the dark preference is stored
        // there instead of being exposed via portal/gsettings.
        if (std::string home = getEnv("HOME"); !home.empty()) {
            std::ifstream settingsFile(home + "/.config/gtk-3.0/settings.ini");
            if (settingsFile.is_open()) {
                std::string line;
                while (std::getline(settingsFile, line)) {
                    if (line.find("gtk-application-prefer-dark-theme=1") != std::string::npos) {
                        cachedValue = true;
                        return cachedValue;
                    }
                }
            }
            settingsFile.close();
        }

        return cachedValue;
    }

    std::shared_ptr<const vnkey::MacroTable> buildMacroTable(const vnkeyMacroTable& macroTable) {
        auto        table  = std::make_shared<vnkey::MacroTable>();
        const auto& macros = *macroTable.macros;
        for (const auto& keymap : macros) {
            table->set(*keymap.key, *keymap.value);
        }
        return table;
    }

    const std::shared_ptr<const vnkey::MacroTable>& VNKeyEngine::macroTable() const {
        return macroTable_;
    }

    const std::shared_ptr<const vnkey::Dictionary>& VNKeyEngine::dictionary() const {
        return dictionary_;
    }

    bool VNKeyEngine::isLikeAddressBarChromium() const {
        return windowFocusTracker_->isLikeAddressBarChromium();
    }

    VNKeyEngine::VNKeyEngine(Instance* instance) :
        instance_(*instance), kbSender_(buildSocketPath("kbSocket")), factory_([this](InputContext& ic) { return new VNKeyState(this, &ic); }) { //NOLINT
        windowFocusTracker_ = std::make_unique<WindowFocusTracker>([this] { appRuleDispatcher_.schedule([this] { refreshFocusedAppRule(); }); });
        std::string desktop = getEnv("XDG_CURRENT_DESKTOP");
        isGnome_            = (!desktop.empty()) && desktop.find("GNOME") != std::string::npos;
        if (*config_.preferVirtualKeyboard) {
            VNKeyWlrVirtualKeyboard::instance().available();
        }
        // emptyCustomKeymap_.customKeymap is implicitly initialized to empty by fcitx::Option default value macro.

        auto& uiManager = instance_.userInterfaceManager();

        charsetAction_ = std::make_unique<SimpleAction>();
        charsetAction_->setShortText(_("Charset"));
        uiManager.registerAction("vnkey-charset", charsetAction_.get());
        charsetMenu_ = std::make_unique<Menu>();
        charsetAction_->setMenu(charsetMenu_.get());

        for (std::size_t i = 0; i < vnkey::kCharsetCount; ++i) {
            const auto        charset = static_cast<vnkey::Charset>(i);
            const std::string name    = CharsetToString(charset);
            charsetSubAction_.emplace_back(std::make_unique<SimpleAction>());
            auto* action = charsetSubAction_.back().get();
            action->setShortText(stringutils::concat(_("Charset"), ": ", name)); // menu phẳng: ghi rõ "Bảng mã: ..."
            action->setCheckable(true);
            uiManager.registerAction(stringutils::concat(CharsetActionPrefix, name), action);
            connections_.emplace_back(action->connect<SimpleAction::Activated>([this, charset](InputContext* ic) {
                if (config_.outputCharset.value() == charset)
                    return;
                config_.outputCharset.setValue(charset);
                saveConfig();
                refreshEngine();
                updateCharsetAction(ic);
                if (ic)
                    ic->updateUserInterface(UserInterfaceComponent::StatusArea);
            }));
            charsetMenu_->addAction(action);
        }

        initToggleAction(spellCheckAction_, config_.spellCheck, "vnkey-spellcheck", "tools-check-spelling", _("Spell Check"), _("Spell Check"), uiManager);
        initToggleAction(macroAction_, config_.enableMacro, "vnkey-macro", "document-edit", _("Macro"), _("Macro"), uiManager);
        initToggleAction(capitalizeMacroAction_, config_.capitalizeMacro, "vnkey-capitalizemacro", "format-text-uppercase", _("Capitalize Macro"), _("Capitalize Macro"),
                         uiManager);
        initToggleAction(autoNonVnRestoreAction_, config_.autoNonVnRestore, "vnkey-autonvnrestore", "edit-undo", _("Auto Restore Invalid Words"), _("Auto Non-VN Restore"),
                         uiManager);
        initToggleAction(enableDictionaryAction_, config_.enableDictionary, "vnkey-dictionary", "accessories-dictionary", _("Custom Dictionary"), _("Custom Dictionary"),
                         uiManager);

        settingsAction_ = std::make_unique<SimpleAction>();
        settingsAction_->setShortText(_("Settings"));
        connections_.emplace_back(settingsAction_->connect<SimpleAction::Activated>([](InputContext*) { startProcess({FCITX5_VNKEY_SETTINGS_PATH}); }));
        uiManager.registerAction("vnkey-settings", settingsAction_.get());

        // ── Menu khay hệ thống PHẲNG (không menu con) ──────────────────────────
        // Khay của Deepin đặt menu con đè lên menu chính khi icon nằm sát mép màn hình,
        // nên mọi mục nằm cùng một cấp, chia nhóm bằng đường kẻ.
        for (std::size_t i = 0; i < vnkey::kInputMethodCount; ++i) {
            const auto id = static_cast<vnkey::InputMethodId>(i);
            inputMethodSubAction_.emplace_back(std::make_unique<SimpleAction>());
            auto* action = inputMethodSubAction_.back().get();
            action->setShortText(id == vnkey::InputMethodId::Custom ? std::string(_("Custom Keymap")) : std::string(InputMethodIdToString(id)));
            action->setCheckable(true);
            uiManager.registerAction(stringutils::concat("vnkey-im-", i), action);
            connections_.emplace_back(action->connect<SimpleAction::Activated>([this, id](InputContext* ic) {
                if (config_.inputMethod.value() != id) {
                    config_.inputMethod.setValue(id);
                    saveConfig();
                    refreshEngine();
                }
                updateTrayMenus(ic);
            }));
        }
        for (int i = 0; i <= static_cast<int>(VNKeyMode::Adaptive); ++i) {
            const auto mode = static_cast<VNKeyMode>(i);
            modeSubAction_.emplace_back(std::make_unique<SimpleAction>());
            auto* action = modeSubAction_.back().get();
            action->setShortText(_(VNKeyModeToString(mode)));
            action->setCheckable(true);
            uiManager.registerAction(stringutils::concat("vnkey-mode-", i), action);
            connections_.emplace_back(action->connect<SimpleAction::Activated>([this, mode](InputContext* ic) {
                if (config_.mode.value() != mode) {
                    config_.mode.setValue(mode);
                    saveConfig();
                }
                setMode(mode, ic);
                updateTrayMenus(ic);
            }));
        }
        for (int i = 0; i < 4; ++i) {
            separatorActions_.emplace_back(std::make_unique<SimpleAction>());
            separatorActions_.back()->setSeparator(true);
            uiManager.registerAction(stringutils::concat("vnkey-separator-", i), separatorActions_.back().get());
        }

        std::string configDir = vnkeyConfigDir();

        if (!std::filesystem::exists(configDir)) {
            std::filesystem::create_directories(configDir);
        }
        reloadConfig();
        setMode(config_.mode.value(), nullptr);
        checkServerForMode(config_.mode.value());
        instance_.inputContextManager().registerProperty("VNKeyState", &factory_);
        appRulesPath_ = configDir + "/vnkey-app-rules.conf";
        loadAppRules();
        updateTrayMenus(nullptr);
        appRuleDispatcher_.attach(&instance_.eventLoop());
        windowFocusTracker_->start();
    }

    void VNKeyEngine::initToggleAction(std::unique_ptr<SimpleAction>& action, Option<bool>& option, const std::string& actionId, const std::string& iconName,
                                       const std::string& textLong, const std::string& textOnOff, UserInterfaceManager& uiManager) {
        action = std::make_unique<SimpleAction>();
        action->setShortText(textLong);
        (void)iconName; // menu khay không dùng icon minh hoạ
        action->setCheckable(true);
        auto* const actionPtr = action.get();
        auto* const optionPtr = &option;
        connections_.emplace_back(action->connect<SimpleAction::Activated>([this, actionPtr, optionPtr, textOnOff](InputContext* ic) {
            optionPtr->setValue(!optionPtr->value());
            saveConfig();
            refreshOption();
            updateAction(ic, *actionPtr, *optionPtr, textOnOff);
        }));
        uiManager.registerAction(actionId, action.get());
    }

    void VNKeyEngine::updateAction(InputContext* ic, SimpleAction& action, Option<bool>& option, const std::string& textOnOff) {
        // Dấu tích chuẩn của menu thay cho tiền tố "✔/✖" trong chữ.
        action.setShortText(textOnOff);
        action.setChecked(option.value());
        if (ic != nullptr) {
            action.update(ic);
        }
    }

    VNKeyEngine::~VNKeyEngine() {
        windowFocusTracker_->stop();
        VNKeyWlrVirtualKeyboard::instance().cancelPendingTasks();
        VNKeyX11Keyboard::instance().cancelPendingTasks();
        appRuleDispatcher_.detach();
        g_stopFlagMonitor.store(true, std::memory_order_release);
        teardownMouseWatcher();
        VNKEY_INFO("Engine destroyed.");
    }

    void VNKeyEngine::scheduleOnMainThread(std::function<void()> task) {
        appRuleDispatcher_.schedule(std::move(task));
    }

    VNKeyState* VNKeyEngine::stateFor(InputContext* ic) {
        return ic != nullptr ? ic->propertyFor(&factory_) : nullptr;
    }

    const vnkeyCustomKeymap& VNKeyEngine::customKeymap() const {
        if (config_.enableCustomKeymap.value()) {
            return customKeymap_;
        }
        return emptyCustomKeymap_;
    }

    void VNKeyEngine::reloadConfig() {
        readAsIni(config_, "conf/vnkey.conf");
        readAsIni(customKeymap_, CustomKeymapFile);
        readAsIni(macroTables_, MacroTableFile);
        macroTable_ = buildMacroTable(macroTables_);
        loadDictionary();
        loadAppRules();
        populateConfig();
    }

    void VNKeyEngine::loadDictionary() {
        auto dictionary = std::make_shared<vnkey::Dictionary>();
        // Go read the descriptor handed to it; the port reads the same descriptor into the text
        // Dictionary::loadFromText expects.
        auto loadFromFd = [&dictionary](int rawFd) {
            FdGuard                fd(rawFd);
            std::string            contents;
            std::array<char, 4096> buffer{};
            for (ssize_t count = ::read(fd.get(), buffer.data(), buffer.size()); count > 0; count = ::read(fd.get(), buffer.data(), buffer.size())) {
                contents.append(buffer.data(), static_cast<std::size_t>(count));
            }
            dictionary->loadFromText(contents);
        };
        if (config_.enableDictionary.value()) {
#if VNKEY_USE_MODERN_FCITX_API
            auto fd = StandardPaths::global().open(StandardPathsType::PkgData, "vnkey/vietnamese.cm.dict");
#else
            auto fd = StandardPath::global().open(StandardPath::Type::PkgData, "vnkey/vietnamese.cm.dict", O_RDONLY);
#endif
            if (fd.isValid()) {
                loadFromFd(fd.release());
            }
        } else {
#if VNKEY_USE_MODERN_FCITX_API
            auto paths = StandardPaths::global().locateAll(StandardPathsType::PkgData, "vnkey/vietnamese.cm.dict");
#else
            auto paths = StandardPath::global().locateAll(StandardPath::Type::PkgData, "vnkey/vietnamese.cm.dict");
#endif
            for (const auto& p : paths) {
#if VNKEY_USE_MODERN_FCITX_API
                if (!p.string().starts_with("/home/")) {
                    auto fd = fcitx::UnixFD(::open(p.c_str(), O_RDONLY));
                    if (fd.isValid()) {
                        loadFromFd(fd.release());
#else
                if (!p.starts_with("home/")) {
                    int fd = ::open(p.c_str(), O_RDONLY);
                    if (fd != -1) {
                        loadFromFd(fd);
#endif
                        break;
                    }
                }
            }
        }
        dictionary_ = std::move(dictionary);
    }

    const Configuration* VNKeyEngine::getSubConfig(const std::string& path) const {
        if (path == "custom_keymap")
            return &customKeymap_;
        if (path == "vnkey-macro") {
            return &macroTables_;
        }
        if (path == "app_rules") {
            return &appRulesTables_;
        }
        return nullptr;
    }

    void VNKeyEngine::setConfig(const RawConfig& config) {
        config_.load(config, true);
        saveConfig();
        syncMouseWatcher();
        loadDictionary();
        populateConfig();
    }

    void VNKeyEngine::populateConfig() {
        refreshEngine();
        refreshOption();
        updateCharsetAction(nullptr);
        updateAction(nullptr, *spellCheckAction_, config_.spellCheck, _("Spell Check"));
        updateAction(nullptr, *macroAction_, config_.enableMacro, _("Macro"));
        updateAction(nullptr, *capitalizeMacroAction_, config_.capitalizeMacro, _("Capitalize Macro"));
        updateAction(nullptr, *autoNonVnRestoreAction_, config_.autoNonVnRestore, _("Auto Non-VN Restore"));
        updateAction(nullptr, *enableDictionaryAction_, config_.enableDictionary, _("Custom Dictionary"));
    }

    void VNKeyEngine::setSubConfig(const std::string& path, const RawConfig& config) {
        if (path == "custom_keymap") {
            customKeymap_.load(config, true);
            safeSaveAsIni(customKeymap_, CustomKeymapFile);
            refreshEngine();
        } else if (path == "vnkey-macro") {
            macroTables_.load(config, true);
            safeSaveAsIni(macroTables_, MacroTableFile);
            macroTable_ = buildMacroTable(macroTables_);
            refreshEngine();
        } else if (path == "app_rules") {
            appRulesTables_.load(config, true);
            {
                std::scoped_lock lock(appRulesMutex_);
                for (auto it = appRules_.begin(); it != appRules_.end();) {
                    if (!it->first.starts_with("ctx_")) {
                        it = appRules_.erase(it);
                    } else {
                        ++it;
                    }
                }
                for (const auto& rule : *appRulesTables_.rules) {
                    appRules_[*rule.app] = {
                        .mode              = intToMode(*rule.mode),
                        .commitInterval    = *rule.commitInterval,
                        .backspaceInterval = *rule.backspaceInterval,
                        .postDelay         = *rule.postDelay,
                    };
                }
            }
            saveAppRules();
            refreshEngine();
        }
    }

    std::string VNKeyEngine::subMode(const InputMethodEntry& /*entry*/, InputContext& /*inputContext*/) {
        return InputMethodIdToString(*config_.inputMethod);
    }

    void VNKeyEngine::activate(const InputMethodEntry& /*entry*/, InputContextEvent& event) {
        auto*      ic        = event.inputContext();
        const bool surrvalid = ic->surroundingText().isValid();
        const bool isDbus    = getFrontendName(ic) == "dbus";
        syncMouseWatcher();

        auto& statusArea = event.inputContext()->statusArea();
        if (ic->capabilityFlags().test(CapabilityFlag::Preedit))
            instance_.inputContextManager().setPreeditEnabledByDefault(true);

        auto*       state   = ic->propertyFor(&factory_);
        std::string appName = programNameFor(ic);
        activeContext_      = ic->watch();
        activeProgram_      = appName;
        VNKEY_INFO("App name: " + appName);
        VNKEY_INFO("Frontend: " + getFrontendName(ic));

        instance_.inputContextManager().foreach ([this, ic](InputContext* other) {
            if (other != ic) {
                if (auto* otherState = other->propertyFor(&factory_)) {
                    otherState->abortDeferredSelectIfPending();
                }
            }
            return true;
        });

        const VNKeyMode ruleMode   = getAppRule(appName);
        const VNKeyMode targetMode = resolveMode(ruleMode, appName);
        VNKEY_INFO("Target mode: " + VNKeyModeI18NAnnotation::toString(targetMode) +
                   (ruleMode == VNKeyMode::Adaptive ? " (adaptive)" : ""));

        updateCharsetAction(event.inputContext());

        state->isLibreoffice_ = (appName == "soffice");
        state->isChromium_    = isChromiumProgram(appName);
        state->finishOverdueReplacement();
        // A burst aborted while the context was away stashed its replacement text: commit it before setMode()
        // clears the buffers it lives in.
        state->reconcileAbortedUinputReplacement();

        const bool uinputMode         = isUinputMode(targetMode);
        const bool focusBounce        = uinputMode && state->lastDeactivateTime_ > 0 && nowMs() - state->lastDeactivateTime_ < 100;
        const bool resumeReplacement  = focusBounce && state->deletionInterruptedAt_ > 0 && g_isDeleting.load(std::memory_order_acquire);
        state->deletionInterruptedAt_ = 0;

        if (!resumeReplacement) {
            state->cancelPendingUinput();
            g_isDeleting.store(false, std::memory_order_release);
        } else {
            state->cancelSelectAbort();
            state->rearmReplacementDeadline();
        }

        if (focusBounce) {
            state->cancelDeferredFocusOut();
            setRealMode(targetMode);
            ic->updateUserInterface(UserInterfaceComponent::StatusArea);
            VNKEY_INFO("Focus bounce: keep word buffers");
        } else {
            state->flushDeferredFocusOut();
            setMode(targetMode, event.inputContext());
        }
        if (event.type() == EventType::InputContextFocusIn && isDbus && !surrvalid) {
            VNKEY_INFO("Skip clearAllBuffers");
        } else if (surrvalid && !state->oldPreBuffer_.empty() && (nowMs() - state->lastDeactivateTime_) >= 100) {
            state->clearAllBuffers();
        }
        if (!resumeReplacement) {
            g_isDeleting.store(false, std::memory_order_release);
        }
        if (targetMode == VNKeyMode::Emoji) {
            state->updateEmojiPreedit();
        } else {
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            const VNKeyMode activeMode = currentMode();
            if (activeMode == VNKeyMode::Preedit || activeMode == VNKeyMode::SurroundingText)
                ic->updatePreedit();
        }
        for (auto* action : trayActions()) {
            statusArea.addAction(StatusGroup::InputMethod, action);
        }
    }

    bool VNKeyEngine::handleAppModeMenu(KeyEvent& keyEvent) {
        auto* ic    = keyEvent.inputContext();
        auto* state = ic->propertyFor(&factory_);

        if (isSelectingAppMode_) {
            if (keyEvent.isRelease())
                return true;

            auto   baseList = ic->inputPanel().candidateList();
            auto   menuList = std::dynamic_pointer_cast<CommonCandidateList>(baseList);
            KeySym keySym   = keyEvent.key().sym();

            auto   moveCursor = [&](int delta) {
                if (!menuList || menuList->empty()) {
                    return false;
                }

                int totalSize = menuList->totalSize();
                if (totalSize <= 1) {
                    return false;
                }

                int cursorIndex = menuList->globalCursorIndex();
                if (cursorIndex < 0 || cursorIndex >= totalSize) {
                    cursorIndex = 0;
                }

                int nextIndex = cursorIndex + delta;
                if (nextIndex < 0) {
                    nextIndex = totalSize - 1;
                } else if (nextIndex >= totalSize) {
                    nextIndex = 0;
                }

                menuList->setGlobalCursorIndex(nextIndex);
                ic->updateUserInterface(UserInterfaceComponent::InputPanel);
                return true;
            };

            keyEvent.filterAndAccept();

            std::optional<VNKeyMode> selectedMode  = std::nullopt;
            bool                     selectionMade = false;

            switch (keySym) {
                case FcitxKey_Tab:
                case FcitxKey_Down: {
                    if (moveCursor(1)) {
                        return true;
                    }
                    break;
                }
                case FcitxKey_ISO_Left_Tab:
                case FcitxKey_Up: {
                    if (moveCursor(-1)) {
                        return true;
                    }
                    break;
                }
                case FcitxKey_space:
                case FcitxKey_Return: {
                    if (menuList && !menuList->empty()) {
                        int selectedIndex = menuList->globalCursorIndex();
                        if (selectedIndex < 0 || selectedIndex >= menuList->totalSize()) {
                            selectedIndex = 0;
                        }
                        menuList->candidateFromAll(selectedIndex).select(ic);
                        return true;
                    }
                    break;
                }
                case FcitxKey_Escape: {
                    selectionMade = true;
                    break;
                }
                default: {
                    auto it = modeMenuMapping_.find(keySym);
                    if (it != modeMenuMapping_.end()) {
                        selectedMode = it->second;
                    }

                    if (selectedMode == std::nullopt) {
                        const auto& kl = *config_.modeMenuKey;
                        if (kl.size() == 1 && !kl[0].hasModifier()) {
                            std::string charStr = Key::keySymToUTF8(kl[0].sym());
                            if (!charStr.empty() && keySym == typeKeyForModeMenuHotkey(kl[0].sym(), config_)) {
                                isSelectingAppMode_ = false;
                                ic->inputPanel().reset();
                                ic->updateUserInterface(UserInterfaceComponent::InputPanel);
                                state->commitBuffer();
                                state->reset();
                                ic->commitString(charStr);
                                return true;
                            }
                        }
                    }
                    break;
                }
            }

            if (selectedMode != std::nullopt) {
                VNKEY_INFO("Selected mode: " + VNKeyModeI18NAnnotation::toString(selectedMode.value()));
                if (selectedMode != VNKeyMode::Emoji) {
                    if (keySym == Key(*config_.shortcutDefault).sym()) { // Default Typing key
                        clearAppRule(currentConfigureApp_);
                    } else {
                        setAppRule(currentConfigureApp_, selectedMode.value());
                        if (!currentConfigureApp_.starts_with("ctx_")) {
                            saveAppRules();
                        }
                    }
                }
                selectionMade = true;
            }

            if (selectionMade) {
                isSelectingAppMode_ = false;
                ic->inputPanel().reset();
                ic->updateUserInterface(UserInterfaceComponent::InputPanel);

                if (selectedMode != std::nullopt) {
                    state->commitBuffer();
                    state->reset();
                    setMode(selectedMode.value(), ic);
                    if (selectedMode == VNKeyMode::Emoji) {
                        state->updateEmojiPreedit();
                    } else {
                        showCycleModeNotification(selectedMode.value(), ic);
                    }
                }
            }
            return true;
        }
        return false;
    }

    void VNKeyEngine::keyEvent(const InputMethodEntry& /*entry*/, KeyEvent& keyEvent) {
        auto* ic = keyEvent.inputContext();
        if (ic == nullptr) {
            return;
        }

        auto* state = ic->propertyFor(&factory_);

        if (ic->capabilityFlags().testAny(CapabilityFlag::Password)) {
            if (state != nullptr) {
                state->reset();
            }
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            return;
        }

        if (isSelectingAppMode_ && g_mouseClicked.load(std::memory_order_acquire)) {
            closeAppModeMenu();
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            state->commitBuffer();
            state->reset();
        }

        if (handleAppModeMenu(keyEvent)) {
            return;
        }

        if (handleCycleModeHotkey(ic, keyEvent)) {
            return;
        }

        if (handleModeMenuHotkey(ic, keyEvent)) {
            return;
        }
        state->keyEvent(keyEvent);
        syncRealtextLen(ic);
    }

    bool VNKeyEngine::handleModeMenuHotkey(InputContext* ic, KeyEvent& keyEvent) {
        auto* state = ic->propertyFor(&factory_);
        if (!keyEvent.isRelease() && !config_.modeMenuKey->empty() && keyEvent.key().checkKeyList(*config_.modeMenuKey)) {
            VNKEY_INFO("Mode menu key pressed");
            if (state != nullptr) {
                state->commitBuffer();
                state->reset();
            }
            currentConfigureApp_ = programNameFor(ic);
            g_mouseClicked.store(false, std::memory_order_release);
            std::string appName = programNameFor(ic);
            setMode(getAppRule(appName), ic);
            showAppModeMenu(ic);
            keyEvent.filterAndAccept();
            return true;
        }
        return false;
    }

    void VNKeyEngine::syncRealtextLen(InputContext* ic) {
        const auto&  s       = ic->surroundingText();
        const auto&  text    = s.text();
        const size_t textLen = vnkey::utf8::length(text);
        unsigned int cursor  = s.cursor();
        if (textLen == static_cast<size_t>(cursor))
            g_realTextLen.store(static_cast<unsigned int>(textLen), std::memory_order_release);
    }

    bool VNKeyEngine::handleCycleModeHotkey(InputContext* ic, KeyEvent& keyEvent) {
        if (!keyEvent.isRelease() && !config_.cycleModeKey->empty() && keyEvent.key().checkKeyList(*config_.cycleModeKey)) {
            VNKEY_INFO("Cycle mode key pressed");
            std::string            appName = programNameFor(ic);
            const VNKeyMode        appMode = getAppRule(appName);

            std::vector<VNKeyMode> enabledModes = enabledModesInOrder();

            if (!enabledModes.empty()) {
                size_t currentIdx = 0;
                bool   found      = false;
                for (size_t i = 0; i < enabledModes.size(); ++i) {
                    if (enabledModes[i] == appMode) {
                        currentIdx = i;
                        found      = true;
                        break;
                    }
                }

                VNKeyMode nextMode = found ? enabledModes[(currentIdx + 1) % enabledModes.size()] : enabledModes[0];
                setMode(nextMode, ic);
                setAppRule(appName, nextMode);
                showCycleModeNotification(nextMode, ic);
            }

            keyEvent.filterAndAccept();
            return true;
        }
        return false;
    }

    std::vector<VNKeyMode> VNKeyEngine::enabledModesInOrder() const {
        std::vector<VNKeyMode> enabledModes;
        for (const auto& entry : modeOrderEntries(config_)) {
            if (!entry.visible) {
                continue;
            }

            bool duplicate = false;
            for (auto m : enabledModes) {
                if (m == entry.mode) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                enabledModes.push_back(entry.mode);
            }
        }
        return enabledModes;
    }

    void VNKeyEngine::reset(const InputMethodEntry& /*entry*/, InputContextEvent& event) {
        if (g_isDeleting.load(std::memory_order_acquire)) {
            VNKEY_INFO("Ignore reset while is_deleting");
            return;
        }
        VNKEY_INFO("Reset engine");
        auto* state = event.inputContext()->propertyFor(&factory_);
        if (!state->isEmptyHistory() && event.type() != EventType::InputContextFocusOut) {
            return;
        }

        if (event.type() == EventType::InputContextFocusOut || event.type() == EventType::InputContextReset) {
            state->reset(event.type() == EventType::InputContextFocusOut);
        }
    }

    void VNKeyEngine::deactivate(const InputMethodEntry& /*entry*/, InputContextEvent& event) {
        auto* ic = event.inputContext();
        if (activeContext_.get() == ic) {
            activeContext_ = {};
            activeProgram_.clear();
        }
        auto*      state      = ic->propertyFor(&factory_);
        const bool uinputMode = isUinputMode(currentMode());

        if (uinputMode && g_isDeleting.load(std::memory_order_acquire) && state->expectedBackspaces_ > 0) {
            state->lastDeactivateTime_    = nowMs();
            state->deletionInterruptedAt_ = state->lastDeactivateTime_;
            if (state->realSelect_) {
                state->deferSelectAbort();
            }
            VNKEY_INFO("Replacement interrupted by focus out");
            return;
        }

        const bool surrvalid       = ic->surroundingText().isValid();
        const bool isDbus          = getFrontendName(ic) == "dbus";
        state->lastDeactivateTime_ = nowMs();
        if (event.type() == EventType::InputContextFocusOut) {
            state->forgetShiftState();
        }
        if (currentMode() == VNKeyMode::Preedit && event.type() != EventType::InputContextFocusOut) {
            state->commitBuffer();
        } else {
            const bool preedit = currentMode() == VNKeyMode::Preedit;
            if (!preedit && event.type() == EventType::InputContextFocusOut && isDbus && !surrvalid) {
                state->lastDeactivateTime_ = nowMs();
                VNKEY_INFO("Skip clearAllBuffers");
            } else if (preedit || ((!uinputMode || event.type() != EventType::InputContextFocusOut) && surrvalid && !state->oldPreBuffer_.empty())) {
                state->clearAllBuffers();
            }

            g_isDeleting.store(false, std::memory_order_release);
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            if (currentMode() == VNKeyMode::Preedit || currentMode() == VNKeyMode::Emoji || currentMode() == VNKeyMode::SurroundingText)
                ic->updatePreedit();
        }
    }

    void VNKeyEngine::refreshEngine() {
        if (!factory_.registered())
            return;
        instance_.inputContextManager().foreach ([this](InputContext* ic) {
            auto* state = ic->propertyFor(&factory_);
            state->setEngine();
            if (ic->hasFocus()) {
                // Re-resolve the focused window's rule; setEngine() must not
                // reset it to the global mode.
                setMode(getAppRule(programNameFor(ic)), ic);
                state->reset();
            }
            return true;
        });
    }

    void VNKeyEngine::refreshOption() {
        if (!factory_.registered())
            return;
        instance_.inputContextManager().foreach ([this](InputContext* ic) {
            auto* state = ic->propertyFor(&factory_);
            state->setOption();
            if (ic->hasFocus())
                state->reset();
            return true;
        });
    }

    void VNKeyEngine::updateCharsetAction(InputContext* ic) {
        auto name = stringutils::concat(CharsetActionPrefix, CharsetToString(*config_.outputCharset));
        for (const auto& action : charsetSubAction_) {
            action->setChecked(action->name() == name);
            if (ic != nullptr)
                action->update(ic);
        }
        updateTrayMenus(ic);
    }

    void VNKeyEngine::updateTrayMenus(InputContext* ic) {
        for (std::size_t i = 0; i < inputMethodSubAction_.size(); ++i) {
            auto* action = inputMethodSubAction_[i].get();
            action->setChecked(static_cast<std::size_t>(config_.inputMethod.value()) == i);
            if (ic != nullptr)
                action->update(ic);
        }
        for (std::size_t i = 0; i < modeSubAction_.size(); ++i) {
            auto* action = modeSubAction_[i].get();
            action->setChecked(static_cast<std::size_t>(config_.mode.value()) == i);
            if (ic != nullptr)
                action->update(ic);
        }
    }

    std::vector<Action*> VNKeyEngine::trayActions() const {
        std::vector<Action*> actions;
        // Kiểu gõ: các kiểu phổ biến + kiểu đang dùng (nếu khác)
        const auto currentIm = config_.inputMethod.value();
        for (const auto id : {vnkey::InputMethodId::Telex, vnkey::InputMethodId::Vni, vnkey::InputMethodId::TelexVni, vnkey::InputMethodId::Viqr}) {
            actions.push_back(inputMethodSubAction_[static_cast<std::size_t>(id)].get());
        }
        if (currentIm != vnkey::InputMethodId::Telex && currentIm != vnkey::InputMethodId::Vni && currentIm != vnkey::InputMethodId::TelexVni &&
            currentIm != vnkey::InputMethodId::Viqr) {
            actions.push_back(inputMethodSubAction_[static_cast<std::size_t>(currentIm)].get());
        }
        actions.push_back(separatorActions_[0].get());
        // Chế độ: chỉ các chế độ đang bật, theo thứ tự người dùng sắp
        for (const VNKeyMode mode : enabledModesInOrder()) {
            actions.push_back(modeSubAction_[static_cast<std::size_t>(mode)].get());
        }
        actions.push_back(separatorActions_[1].get());
        // Tuỳ chọn gõ
        for (auto* toggle : {spellCheckAction_.get(), autoNonVnRestoreAction_.get(), macroAction_.get(), capitalizeMacroAction_.get(), enableDictionaryAction_.get()}) {
            actions.push_back(toggle);
        }
        actions.push_back(separatorActions_[2].get());
        // Bảng mã: các bảng phổ biến + bảng đang dùng (nếu khác)
        const auto currentCharset = config_.outputCharset.value();
        for (const auto cs : {vnkey::Charset::Unicode, vnkey::Charset::Tcvn3Abc, vnkey::Charset::VniWindows}) {
            actions.push_back(charsetSubAction_[static_cast<std::size_t>(cs)].get());
        }
        if (currentCharset != vnkey::Charset::Unicode && currentCharset != vnkey::Charset::Tcvn3Abc && currentCharset != vnkey::Charset::VniWindows) {
            actions.push_back(charsetSubAction_[static_cast<std::size_t>(currentCharset)].get());
        }
        actions.push_back(separatorActions_[3].get());
        actions.push_back(settingsAction_.get());
        return actions;
    }

    void VNKeyEngine::loadAppRules() {
        {
            std::scoped_lock                                     lock(appRulesMutex_);
            std::unordered_map<std::string, VNKeyAppRuleSetting> ctxRules;
            for (const auto& [app, setting] : appRules_) {
                if (app.starts_with("ctx_")) {
                    ctxRules[app] = setting;
                }
            }
            appRules_ = std::move(ctxRules);
        }
        auto loadFromFile = [this](const std::string& path) {
            if (path.empty()) {
                VNKEY_WARN("App rules path is empty, skipping load");
                return;
            }
            std::ifstream file(path);
            if (!file.is_open())
                return;

            std::unordered_map<std::string, VNKeyAppRuleSetting> tempRules;
            std::string                                          line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#')
                    continue;
                auto delimiterPos = line.find('=');
                if (delimiterPos != std::string::npos) {
                    std::string              app   = line.substr(0, delimiterPos);
                    std::string              rest  = line.substr(delimiterPos + 1);
                    std::vector<std::string> parts = stringutils::split(rest, ",");
                    if (parts.empty()) {
                        VNKEY_WARN("Invalid mode value for app: " + app);
                        continue;
                    }
                    auto field = [&parts](size_t index) -> int { return (parts.size() > index && !parts[index].empty()) ? std::stoi(parts[index]) : 0; };
                    try {
                        int mode       = std::stoi(parts[0]);
                        int commit     = field(1);
                        int backspace  = field(2);
                        int post       = field(3);
                        tempRules[app] = {
                            .mode              = intToMode(mode),
                            .commitInterval    = commit > 0 ? commit : 0,
                            .backspaceInterval = backspace > 0 ? backspace : 0,
                            .postDelay         = post > 0 ? post : 0,
                        };
                    } catch (const std::exception&) { VNKEY_WARN("Invalid mode value for app: " + app); }
                }
            }
            file.close();

            std::scoped_lock lock(appRulesMutex_);
            for (const auto& [app, setting] : tempRules) {
                appRules_[app] = setting;
            }
        };
        loadFromFile(appRulesPath_);

        std::scoped_lock          lock(appRulesMutex_);
        std::vector<vnkeyAppRule> rules;
        for (const auto& pair : appRules_) {
            if (pair.first.starts_with("ctx_"))
                continue;
            vnkeyAppRule rule;
            rule.app.setValue(pair.first);
            rule.mode.setValue(modeToInt(pair.second.mode));
            rule.commitInterval.setValue(pair.second.commitInterval);
            rule.backspaceInterval.setValue(pair.second.backspaceInterval);
            rule.postDelay.setValue(pair.second.postDelay);
            rules.push_back(std::move(rule));
        }
        appRulesTables_.rules.setValue(std::move(rules));
    }

    void VNKeyEngine::saveAppRules() const {
        // Method is const but locks mutable appRulesMutex_ to safely read appRules_ state
        std::ofstream file(appRulesPath_, std::ios::trunc);
        if (!file.is_open())
            return;

        file << "# VNKey Per-App Configuration\n";
        file << "# 0 = Off, 1 = Uinput (Smooth), 2 = Uinput (Smooth, legacy), 3 = Uinput (Super Smooth), 4 = Surrounding Text, 5 = Preedit,\n";
        file << "# 6 = Emoji Picker, 7 = Uinput (Select), 8 = Uinput (Adaptive)\n";
        file << "# app=mode[,commit_interval_ms[,backspace_interval_ms[,post_delay_ms]]]   0/omitted = use defaults\n";
        std::scoped_lock lock(appRulesMutex_);
        for (const auto& pair : appRules_) {
            bool currentIsCtx = pair.first.starts_with("ctx_");
            if (!currentIsCtx) {
                const int   commit    = pair.second.commitInterval;
                const int   backspace = pair.second.backspaceInterval;
                const int   post      = pair.second.postDelay;
                std::string value     = std::to_string(modeToInt(pair.second.mode));
                if (commit > 0 || backspace > 0 || post > 0)
                    value += "," + std::to_string(commit > 0 ? commit : 0);
                if (backspace > 0 || post > 0)
                    value += "," + std::to_string(backspace > 0 ? backspace : 0);
                if (post > 0)
                    value += "," + std::to_string(post);
                file << pair.first << "=" << value << "\n";
            }
        }
        file.close();
    }

    VNKeyAppRuleSetting VNKeyEngine::getAppRuleSetting(const std::string& appName) const {
        std::scoped_lock lock(appRulesMutex_);
        auto             it = appRules_.find(appName);
        if (it != appRules_.end()) {
            return it->second;
        }
        VNKeyAppRuleSetting setting{.mode = config_.mode.value(), .commitInterval = 0, .backspaceInterval = 0, .postDelay = 0};
        if (setting.mode == VNKeyMode::Adaptive && isChromiumProgram(appName)) {
            // Đa năng + trình duyệt Chromium: Chrome đôi khi nhả/lấy lại focus giữa đợt Backspace ảo,
            // phím đến quá sát (5 ms) có thể bị rơi → giãn nhẹ khoảng chờ.
            setting.commitInterval    = 20;
            setting.backspaceInterval = 10;
            setting.postDelay         = 15;
        }
        return setting;
    }

    VNKeyMode VNKeyEngine::resolveMode(VNKeyMode mode, std::string_view program) const {
        if (mode != VNKeyMode::Adaptive) {
            return mode;
        }
        // Xoá bằng Backspace ảo (Siêu mượt) ở mọi nơi. KHÔNG dùng Chọn (Shift+←) cho Chrome: Shift ảo dùng chung
        // trạng thái với bàn phím thật nên phím gõ ngay sau bị dính Shift (Shift+Space mở Tìm kiếm Deepin,
        // Shift có thể bật/tắt bộ gõ) — đo thực tế: Chọn 5/10, Siêu mượt 10/10 trong ô nhập Chrome.
        // Chrome chỉ khác ở khoảng chờ (xem getAppRuleSetting).
        (void)program;
        return VNKeyMode::SuperSmooth;
    }

    VNKeyMode VNKeyEngine::getAppRule(const std::string& appName) const {
        return getAppRuleSetting(appName).mode;
    }

    void VNKeyEngine::setAppRule(const std::string& appName, VNKeyMode mode) {
        auto rules = *appRulesTables_.rules;

        bool found = false;
        for (auto& rule : rules) {
            if (*rule.app == appName) {
                rule.mode.setValue(modeToInt(mode));
                found = true;
                break;
            }
        }

        if (!found) {
            vnkeyAppRule newRule;
            newRule.app.setValue(appName);
            newRule.mode.setValue(modeToInt(mode));
            rules.push_back(std::move(newRule));
        }

        {
            std::scoped_lock lock(appRulesMutex_);
            appRules_[appName].mode = mode;
        }
        appRulesTables_.rules.setValue(std::move(rules));
    }

    void VNKeyEngine::closeAppModeMenu() {
        isSelectingAppMode_ = false;
        g_mouseClicked.store(false, std::memory_order_release);
    }

    void VNKeyEngine::showAppModeMenu(InputContext* ic) {
        isSelectingAppMode_ = true;

        auto candidateList = std::make_unique<CommonCandidateList>();

        candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
        candidateList->setPageSize(10);

        auto getLabel = [&](const VNKeyMode& modeName, const std::string& modeLabel) {
            if (modeName == currentMode()) {
                return Text(">> " + modeLabel);
            }
            return Text("   " + modeLabel);
        };

        auto cleanup = [this](InputContext* ic) {
            isSelectingAppMode_ = false;
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            auto* state = ic->propertyFor(&factory_);
            state->commitBuffer();
            state->reset();
        };

        auto applyMode = [this, cleanup](VNKeyMode mode, bool isDefault = false) {
            return [this, mode, cleanup, isDefault](InputContext* ic) {
                if (mode != VNKeyMode::Emoji) {
                    if (isDefault) {
                        clearAppRule(currentConfigureApp_);
                    } else {
                        setAppRule(currentConfigureApp_, mode);
                        if (!currentConfigureApp_.starts_with("ctx_")) {
                            saveAppRules();
                        }
                    }
                }

                cleanup(ic);
                setMode(mode, ic);
                if (mode == VNKeyMode::Emoji) {
                    auto* state = ic->propertyFor(&factory_);
                    state->updateEmojiPreedit();
                } else {
                    showCycleModeNotification(mode, ic);
                }
            };
        };

        const std::vector<ModeOrderEntry> allModes = modeOrderEntries(config_);

        int                               activeSelectionIdx  = -1;
        int                               currentCandidateIdx = 0;
        std::unordered_set<KeySym>        usedModeKeys;

        modeMenuMapping_.clear();
        const VNKeyMode defaultMode = config_.mode.value();

        for (const auto& entry : allModes) {
            if (entry.visible) {
                const bool hasShortcut = entry.key != FcitxKey_None && entry.key != FcitxKey_VoidSymbol;
                if (hasShortcut && usedModeKeys.insert(entry.key).second) {
                    modeMenuMapping_[entry.key] = entry.mode;
                }

                const bool        isDefaultItem = entry.isDefault;
                const std::string keyUtf8       = Key::keySymToUTF8(entry.key);
                std::string       keyLabel      = keyUtf8.empty() ? "" : "[" + keyUtf8 + "] ";
                candidateList->append(std::make_unique<AppModeCandidateWord>(getLabel(entry.mode, keyLabel + modeLabel(entry)), applyMode(entry.mode, isDefaultItem)));

                const bool selectedByMode    = entry.mode == currentMode() && !isDefaultItem;
                const bool selectedByDefault = isDefaultItem && getAppRule(currentConfigureApp_) == defaultMode && !appRules_.contains(currentConfigureApp_);
                if (selectedByMode || selectedByDefault) {
                    activeSelectionIdx = currentCandidateIdx;
                }

                currentCandidateIdx++;
            }
        }

        {
            const auto& kl = *config_.modeMenuKey;
            if (kl.size() == 1 && !kl[0].hasModifier()) {
                std::string charStr = Key::keySymToUTF8(kl[0].sym());
                if (!charStr.empty()) {
                    KeySym      typeKeySym   = typeKeyForModeMenuHotkey(kl[0].sym(), config_);
                    std::string typeKeyLabel = Key::keySymToUTF8(typeKeySym);
                    std::string label        = "[" + typeKeyLabel + "] " + _("Type") + " " + charStr;
                    candidateList->append(std::make_unique<AppModeCandidateWord>(Text(label), [cleanup, charStr](InputContext* ic) {
                        cleanup(ic);
                        ic->commitString(charStr);
                    }));
                }
            }
        }

        if (activeSelectionIdx != -1) {
            candidateList->setGlobalCursorIndex(activeSelectionIdx);
        } else if (candidateList->totalSize() > 0) {
            candidateList->setGlobalCursorIndex(0);
        }

        ic->inputPanel().reset();
        ic->inputPanel().setCandidateList(std::move(candidateList));
        ic->inputPanel().setAuxDown(Text(_("App: ") + currentConfigureApp_));
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void VNKeyEngine::showCycleModeNotification(VNKeyMode mode, InputContext* ic) {
        auto candidateList = std::make_unique<CommonCandidateList>();
        candidateList->setLayoutHint(CandidateLayoutHint::Vertical);
        candidateList->setPageSize(1);

        auto cleanup = [](InputContext* ic) {
            ic->inputPanel().reset();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
        };

        const std::string modeLabel = VNKeyModeI18NAnnotation::toString(mode);

        auto              setCurrentMode = [cleanup](VNKeyMode) { return [cleanup](InputContext* ic) { cleanup(ic); }; };

        candidateList->append(std::make_unique<AppModeCandidateWord>(Text("✓ " + modeLabel), setCurrentMode(mode)));

        candidateList->setGlobalCursorIndex(0);

        ic->inputPanel().reset();
        ic->inputPanel().setCandidateList(std::move(candidateList));
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);

        cycleModeNotificationTimer_.reset();

        auto& eventLoop   = instance_.eventLoop();
        auto  nowTime     = ::fcitx::now(CLOCK_MONOTONIC);
        auto  timeoutTime = nowTime + kCycleModeNotificationTimeoutUsec;

        cycleModeNotificationTimer_ = eventLoop.addTimeEvent(CLOCK_MONOTONIC, timeoutTime, 0, [icRef = ic->watch()](EventSourceTime*, uint64_t) {
            if (auto* ic = icRef.get(); ic && ic->hasFocus()) {
                ic->inputPanel().reset();
                ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            }
            return false;
        });
    }

    void VNKeyEngine::checkServerForMode(VNKeyMode mode) {
        if (isUinputMode(mode) && needsUinputServer() && !kbSender_.ensureConnected()) {
            notifyMissingUinputServer();
        }
    }

    void VNKeyEngine::setMode(VNKeyMode mode, InputContext* ic) {
        mode               = resolveMode(mode, ic != nullptr ? programNameFor(ic) : activeProgram_);
        const bool changed = currentMode() != mode;
        setRealMode(mode);
        if (ic != nullptr) {
            if (auto* state = ic->propertyFor(&factory_)) {
                state->appRuleSetting_ = getAppRuleSetting(programNameFor(ic));
                state->clearAllBuffers();
            }
            ic->updateUserInterface(UserInterfaceComponent::StatusArea);
        }
        if (changed) {
            checkServerForMode(mode);
        }
    }

    std::string VNKeyEngine::subModeIconImpl(const InputMethodEntry& /*entry*/, InputContext& /*inputContext*/) {
        std::string baseIconName;
        switch (currentMode()) {
            case VNKeyMode::Off: baseIconName = "fcitx-vnkey-off"; break;
            case VNKeyMode::Emoji: baseIconName = "fcitx-vnkey-emoji"; break;
            default: baseIconName = "fcitx-vnkey"; break;
        }

        std::string iconName;
        if (*config_.useVNKeyIcons) {
            iconName = baseIconName;
        } else {
            const auto& iconTheme = config_.iconTheme.value();
            if (iconTheme == IconTheme::Light) {
                iconName = baseIconName + "-default-black";
            } else if (iconTheme == IconTheme::Dark) {
                iconName = baseIconName + "-default";
            } else if (isGnome_) {
                // GNOME's panel can be dark while applications prefer light.
                // Symbolic icons inherit the panel foreground, including when
                // its Shell theme changes, without guessing from app settings.
                iconName = baseIconName + "-default-symbolic";
            } else {
                iconName = baseIconName + (isDarkMode() ? "-default" : "-default-black");
            }
        }
        return iconName;
    }

    std::string VNKeyEngine::subModeLabelImpl(const InputMethodEntry& /*entry*/, InputContext& /*inputContext*/) {
        switch (currentMode()) {
            // Nhãn hiện trên thanh tác vụ (Deepin hiển thị nhãn thay vì icon): theo logo VKey.
            case VNKeyMode::Off: return "E";
            case VNKeyMode::Emoji: return "😄";
            default: return "V";
        }
    }

    std::string VNKeyEngine::programNameFor(InputContext* ic) {
        if (ic == nullptr) {
            return "unknown-app";
        }
        std::string programName = resolveProgramName(ic->program(), {});
        if (programName.empty()) {
            if (getEnv("WAYLAND_DISPLAY").empty()) {
                programName = activeWindowProgram();
            } else if (const auto focused = windowFocusTracker_->focusedProgram()) {
                programName = *focused;
            }
        }
        if (programName.empty()) {
            std::ostringstream oss;
            oss << "ctx_" << static_cast<const void*>(ic);
            return oss.str();
        }
        return programName;
    }

    void VNKeyEngine::refreshFocusedAppRule() {
        auto* ic = activeContext_.get();
        if (ic == nullptr || !ic->hasFocus() || isSelectingAppMode_ || !resolveProgramName(ic->program(), {}).empty()) {
            return;
        }
        const auto program = programNameFor(ic);
        if (program == activeProgram_) {
            return;
        }
        activeProgram_ = program;
        auto* state    = ic->propertyFor(&factory_);
        // A shared GNOME input context may now belong to a different app: never commit its old
        // composition into the newly focused field.
        state->discardForAppChange();
        state->isLibreoffice_ = program == "soffice";
        state->isChromium_    = isChromiumProgram(program);
        ic->inputPanel().reset();
        setMode(getAppRule(program), ic);
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void VNKeyEngine::clearAppRule(const std::string& appName) {
        {
            std::scoped_lock lock(appRulesMutex_);
            appRules_.erase(appName);
        }
        auto rules = *appRulesTables_.rules;
        rules.erase(std::ranges::remove_if(rules, [&appName](const auto& rule) { return *rule.app == appName; }).begin(), rules.end());
        appRulesTables_.rules.setValue(std::move(rules));
        if (!appName.starts_with("ctx_")) {
            saveAppRules();
        }
    }

    bool VNKeyEngine::needsUinputServer() const {
        const bool preferInjection = *config_.preferVirtualKeyboard;
        if (preferInjection && VNKeyWlrVirtualKeyboard::instance().available()) {
            return false;
        }
        if (preferInjection && getEnv("WAYLAND_DISPLAY").empty() && VNKeyX11Keyboard::instance().available()) {
            return false;
        }
        return true;
    }

    void VNKeyEngine::syncMouseWatcher() {
        if (needsUinputServer()) {
            setupMouseWatcher();
        } else {
            teardownMouseWatcher();
        }
    }

    void VNKeyEngine::teardownMouseWatcher() {
        if (!mouseEventSource_ && g_mouseSocketFd.load(std::memory_order_acquire) < 0) {
            return;
        }
        mouseEventSource_.reset();
        FdGuard fd(g_mouseSocketFd.exchange(-1, std::memory_order_acq_rel));
        if (fd.isValid()) {
            shutdown(fd.get(), SHUT_RDWR);
        }
        g_mouseClicked.store(false, std::memory_order_release);
        VNKEY_INFO("Mouse watcher disabled, the uinput server is not used");
    }

    void VNKeyEngine::setupMouseWatcher() {
        if (mouseEventSource_) {
            return;
        }

        const std::string  mouseSocketPath = buildSocketPath("mouseSocket");
        struct sockaddr_un addr{};
        socklen_t          len = 0;
        FdGuard            sock(makeAbstractUnixSocket(mouseSocketPath, addr, len));
        if (!sock.isValid()) {
            VNKEY_ERROR("Failed to create mouse socket: " + std::string(strerror(errno)));
            return;
        }

        if (connect(sock.get(), reinterpret_cast<struct sockaddr*>(&addr), len) < 0) {
            VNKEY_ERROR("Failed to connect to mouse socket: " + std::string(strerror(errno)));
            return;
        }

        std::string peerExePath;
        if (!authenticateMouseSocketPeer(sock.get(), peerExePath)) {
            VNKEY_WARN("Unauthorized connection attempt from: " + peerExePath);
            return;
        }

        const int sockFd = sock.get();
        g_mouseSocketFd.store(sock.release(), std::memory_order_release);
        VNKEY_INFO("Mouse socket connected and registered to Fcitx EventLoop.");

        mouseEventSource_ = instance_.eventLoop().addIOEvent(sockFd, IOEventFlag::In, [this, sockFd](EventSourceIO*, int, IOEventFlags) {
            std::array<char, 16> buf{};
            ssize_t              n = recv(sockFd, buf.data(), buf.size(), 0);
            if (n <= 0) {
                VNKEY_ERROR("Mouse socket recv error or disconnected: " + std::string(strerror(errno)));
                mouseEventSource_.reset();
                g_mouseSocketFd.store(-1, std::memory_order_release);
                FdGuard closed(sockFd);
                return true;
            }

            if (n >= 1 && buf[0] == 'C') {
                VNKEY_DEBUG("Mouse click detected from server. Resetting.....");
                g_mouseClicked.store(true, std::memory_order_release);
                instance_.inputContextManager().foreachFocused([this](InputContext* ic) {
                    if (ic) {
                        if (auto* state = ic->propertyFor(&factory_)) {
                            switch (mouseClickAction(currentMode())) {
                                case MouseClickAction::ClearBuffers: state->clearAllBuffers(); break;
                                case MouseClickAction::DeferToClient: state->armMouseClickWatchdog(); break;
                            }
                        }
                    }
                    return true;
                });
            }
            return true;
        });
    }
} // namespace fcitx
