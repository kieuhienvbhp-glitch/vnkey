/*
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "vnkey-mode.h"

namespace fcitx {

    int modeToInt(VNKeyMode mode) {
        switch (mode) {
            case VNKeyMode::Off: return 0;
            case VNKeyMode::Smooth: return 1;
            case VNKeyMode::SuperSmooth: return 3;
            case VNKeyMode::SurroundingText: return 4;
            case VNKeyMode::Preedit: return 5;
            case VNKeyMode::Emoji: return 6;
            case VNKeyMode::Select: return 7;
            case VNKeyMode::Adaptive: return 8;
            default: return 0;
        }
    }

    VNKeyMode intToMode(int mode) {
        switch (mode) {
            case 0: return VNKeyMode::Off;
            case 1:
            case 2: return VNKeyMode::Smooth;
            case 3: return VNKeyMode::SuperSmooth;
            case 4: return VNKeyMode::SurroundingText;
            case 5: return VNKeyMode::Preedit;
            case 6: return VNKeyMode::Emoji;
            case 7: return VNKeyMode::Select;
            case 8: return VNKeyMode::Adaptive;
            default: return VNKeyMode::Off;
        }
    }

} // namespace fcitx
