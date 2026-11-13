/*
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file vnkey-mode.h
 * @brief Typing modes and per-app rule settings shared by the config, the engine and the state.
 */

#ifndef _FCITX5_VNKEY_MODE_H_
#define _FCITX5_VNKEY_MODE_H_

#include <cstdint>

namespace fcitx {

    /**
     * @brief Operating modes for the VNKey input method.
     */
    enum class VNKeyMode : std::uint8_t {
        Off,
        Smooth,
        SuperSmooth,
        SurroundingText,
        Preedit,
        Emoji,
        Select,
        Adaptive, ///< Hiển thị là "Tự động": tự chọn cách xoá theo ứng dụng, xem VNKeyEngine::resolveMode
    };

    /**
     * @brief Resolved per-app rule: typing mode plus optional delay overrides (0 = use global default).
     */
    struct VNKeyAppRuleSetting {
        VNKeyMode mode              = VNKeyMode::Smooth; ///< Typing mode for the app
        int       commitInterval    = 0;                 ///< ms cooldown between commits (KbMsg.preDelay); 0 = default
        int       backspaceInterval = 0;                 ///< ms between injected keys (KbMsg.interval); 0 = default
        int       postDelay         = 0;                 ///< ms before the final trigger key (KbMsg.postDelay); 0 = default
    };

    /**
     * @brief Converts VNKeyMode to int and vice versa.
     */
    int       modeToInt(VNKeyMode mode);
    VNKeyMode intToMode(int mode);

} // namespace fcitx

#endif // _FCITX5_VNKEY_MODE_H_
