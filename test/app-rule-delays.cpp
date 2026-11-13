// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file app-rule-delays.cpp
 * @brief Headless test: per-app rules parse the mode plus the three delay
 *        overrides (commit/backspace/post) from vnkey-app-rules.conf, treat
 *        omitted fields as "use defaults", keep the legacy mode id 2 working,
 *        and skip malformed lines without breaking the load.
 */

#include "vnkey-engine.h"
#include "test-input-context.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

    void reportFailure(const std::string& step, const std::string& expected, const std::string& actual, const std::string& meaning) {
        std::cerr << "Step: " << step << '\n';
        std::cerr << "Expected: " << expected << '\n';
        std::cerr << "Actual: " << actual << '\n';
        std::cerr << "Meaning: " << meaning << '\n';
    }

    std::string settingText(const fcitx::VNKeyAppRuleSetting& setting) {
        return "mode=" + std::to_string(static_cast<int>(setting.mode)) + " commit=" + std::to_string(setting.commitInterval) +
            " backspace=" + std::to_string(setting.backspaceInterval) + " post=" + std::to_string(setting.postDelay);
    }

    bool expectSetting(const fcitx::VNKeyEngine& engine, const std::string& app, const fcitx::VNKeyAppRuleSetting& expected, const std::string& meaning) {
        const auto actual = engine.getAppRuleSetting(app);
        if (actual.mode != expected.mode || actual.commitInterval != expected.commitInterval || actual.backspaceInterval != expected.backspaceInterval ||
            actual.postDelay != expected.postDelay) {
            reportFailure("app rule for " + app, settingText(expected), settingText(actual), meaning);
            return false;
        }
        return true;
    }

} // namespace

int main() {
    const char* testName = "fcitx5-vnkey-app-rule-delays";
    configureTestPaths(testName);

    const auto rulesFile = std::filesystem::temp_directory_path() / testName / "config/fcitx5/conf/vnkey-app-rules.conf";
    {
        std::filesystem::create_directories(rulesFile.parent_path());
        std::ofstream file(rulesFile);
        file << "# comment\n";
        file << "test=1,40,20,25\n";
        file << "legacy=2\n";
        file << "partial=3,15\n";
        file << "bare=5\n";
        file << "malformed=abc\n";
        file << "google-chrome-stable=8\n";
        file << "kate=8\n";
        file << "chromium=8,30\n";
    }

    TestInstance       testInstance;
    fcitx::VNKeyEngine engine(&testInstance.instance);

    fcitx::RawConfig   config;
    config.setValueByPath("Mode", "Preedit");
    config.setValueByPath("InputMethod", "Telex");
    engine.setConfig(config);

    bool ok = true;
    ok &= expectSetting(engine, "test", {.mode = fcitx::VNKeyMode::Smooth, .commitInterval = 40, .backspaceInterval = 20, .postDelay = 25},
                        "app=mode,commit,backspace,post must be parsed field by field");
    ok &= expectSetting(engine, "legacy", {.mode = fcitx::VNKeyMode::Smooth}, "legacy mode id 2 must resolve to Smooth");
    ok &= expectSetting(engine, "partial", {.mode = fcitx::VNKeyMode::SuperSmooth, .commitInterval = 15}, "omitted trailing delay fields must stay zero");

    // Chế độ Đa năng (Adaptive): luật ứng dụng giữ nguyên Adaptive, resolveMode quy ra chế độ thật.
    ok &= expectSetting(engine, "google-chrome-stable", {.mode = fcitx::VNKeyMode::Adaptive}, "an Adaptive app rule keeps its own (zero) delays");
    auto checkResolve = [&](const std::string& app, fcitx::VNKeyMode expected, const std::string& meaning) {
        const auto actual = engine.resolveMode(engine.getAppRuleSetting(app).mode, app);
        if (actual != expected) {
            reportFailure("resolveMode for " + app, std::to_string(static_cast<int>(expected)), std::to_string(static_cast<int>(actual)), meaning);
            ok = false;
        }
    };
    checkResolve("google-chrome-stable", fcitx::VNKeyMode::SuperSmooth, "Adaptive must use backspaces in Chromium (no injected Shift)");
    checkResolve("chromium", fcitx::VNKeyMode::SuperSmooth, "Adaptive must use backspaces in Chromium");
    checkResolve("kate", fcitx::VNKeyMode::SuperSmooth, "Adaptive must use fast backspaces outside Chromium");
    checkResolve("test", fcitx::VNKeyMode::Smooth, "a non-Adaptive rule must not be changed by resolveMode");
    ok &= expectSetting(engine, "chromium", {.mode = fcitx::VNKeyMode::Adaptive, .commitInterval = 30}, "explicit delays on an Adaptive rule are kept");
    ok &= expectSetting(engine, "bare", {.mode = fcitx::VNKeyMode::Preedit}, "a mode-only rule must keep the parsed mode");
    ok &= expectSetting(engine, "malformed", {.mode = fcitx::VNKeyMode::Preedit}, "a malformed line must be skipped");
    ok &= expectSetting(engine, "khong-co-rule", {.mode = fcitx::VNKeyMode::Preedit}, "an app without a rule must inherit the global mode");
    return ok ? 0 : 1;
}
