/*
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file vnkey-mode-menu.cpp
 * @brief Mode Order resolution and the mode menu shortcuts.
 */

#include "vnkey-mode-menu.h"

#include <algorithm>
#include <array>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/stringutils.h>

namespace fcitx {

    KeySym shortcutKey(const std::string& shortcut) {
        return Key(shortcut).sym();
    }

    bool modeVisible(const vnkeyConfig& config, VNKeyMode mode) {
        switch (mode) {
            case VNKeyMode::Adaptive: return *config.showModeAdaptive;
            case VNKeyMode::Smooth: return *config.showModeSmooth;
            case VNKeyMode::SuperSmooth: return *config.showModeSuperSmooth;
            case VNKeyMode::Select: return *config.showModeSelect;
            case VNKeyMode::SurroundingText: return *config.showModeSurroundingText;
            case VNKeyMode::Preedit: return *config.showModePreedit;
            case VNKeyMode::Emoji: return *config.showModeEmoji;
            case VNKeyMode::Off: return *config.showModeOff;
        }
        return false;
    }

    KeySym modeShortcut(const vnkeyConfig& config, VNKeyMode mode) {
        switch (mode) {
            case VNKeyMode::Adaptive: return shortcutKey(*config.shortcutAdaptive);
            case VNKeyMode::Smooth: return shortcutKey(*config.shortcutSmooth);
            case VNKeyMode::SuperSmooth: return shortcutKey(*config.shortcutSuperSmooth);
            case VNKeyMode::Select: return shortcutKey(*config.shortcutSelect);
            case VNKeyMode::SurroundingText: return shortcutKey(*config.shortcutSurroundingText);
            case VNKeyMode::Preedit: return shortcutKey(*config.shortcutPreedit);
            case VNKeyMode::Emoji: return shortcutKey(*config.shortcutEmoji);
            case VNKeyMode::Off: return shortcutKey(*config.shortcutOff);
        }
        return FcitxKey_None;
    }

    std::string modeLabel(const ModeOrderEntry& entry) {
        return entry.isDefault ? _("Default Typing") : VNKeyModeI18NAnnotation::toString(entry.mode);
    }

    std::vector<ModeOrderEntry> modeOrderEntries(const vnkeyConfig& config) {
        const auto order = stringutils::split(*config.modeOrder, ",");

        auto       resolve = [&config](const ModeNameEntry& nameEntry) {
            if (nameEntry.mode.has_value()) {
                return ModeOrderEntry{.mode = *nameEntry.mode, .key = modeShortcut(config, *nameEntry.mode), .visible = modeVisible(config, *nameEntry.mode), .isDefault = false};
            }
            return ModeOrderEntry{.mode = config.mode.value(), .key = shortcutKey(*config.shortcutDefault), .visible = *config.showModeDefault, .isDefault = true};
        };

        std::vector<ModeOrderEntry> entries;
        for (const auto& name : order) {
            const auto* const it = std::ranges::find_if(kModeNames, [&name](const ModeNameEntry& entry) { return entry.name == name; });
            if (it != kModeNames.end()) {
                entries.push_back(resolve(*it));
            }
        }
        for (const auto& nameEntry : kModeNames) {
            const bool listed = std::ranges::any_of(order, [&nameEntry](const std::string& name) { return nameEntry.name == name; });
            if (!listed) {
                entries.push_back(resolve(nameEntry));
            }
        }
        return entries;
    }

    bool isAppModeMenuReservedKey(KeySym sym, const vnkeyConfig& config) {
        std::array<KeySym, kModeNames.size()> modeShortcuts{};
        std::ranges::transform(kModeNames, modeShortcuts.begin(), [&config](const ModeNameEntry& nameEntry) {
            if (nameEntry.mode.has_value()) {
                return modeShortcut(config, *nameEntry.mode);
            }
            return shortcutKey(*config.shortcutDefault);
        });
        if (std::ranges::find(modeShortcuts, sym) != modeShortcuts.end()) {
            return true;
        }

        static const auto reservedMenuKeys = std::to_array<KeySym>({
            FcitxKey_Escape,
            FcitxKey_Tab,
            FcitxKey_ISO_Left_Tab,
            FcitxKey_Return,
            FcitxKey_space,
            FcitxKey_Up,
            FcitxKey_Down,
        });
        return std::ranges::find(reservedMenuKeys, sym) != reservedMenuKeys.end();
    }

    KeySym typeKeyForModeMenuHotkey(KeySym hotkeySym, const vnkeyConfig& config) {
        return isAppModeMenuReservedKey(hotkeySym, config) ? FcitxKey_f : hotkeySym;
    }

} // namespace fcitx
