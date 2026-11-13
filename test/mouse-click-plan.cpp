// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mouse-click-plan.cpp
 * @brief Tests for the mouse-click handling rule used by the Preedit mode.
 */

#include "vnkey-mode.h"
#include "vnkey-utils.h"

#include <cstdio>

namespace {
    int  failures = 0;

    void check(const char* name, bool condition) {
        if (!condition) {
            std::printf("FAIL %s\n", name);
            ++failures;
        }
    }
} // namespace

int main() {
    check("preedit defers the click to the client", mouseClickAction(fcitx::VNKeyMode::Preedit) == MouseClickAction::DeferToClient);
    check("smooth clears the buffers", mouseClickAction(fcitx::VNKeyMode::Smooth) == MouseClickAction::ClearBuffers);
    check("super smooth clears the buffers", mouseClickAction(fcitx::VNKeyMode::SuperSmooth) == MouseClickAction::ClearBuffers);
    check("select clears the buffers", mouseClickAction(fcitx::VNKeyMode::Select) == MouseClickAction::ClearBuffers);
    check("off clears the buffers", mouseClickAction(fcitx::VNKeyMode::Off) == MouseClickAction::ClearBuffers);

    if (failures != 0) {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("vnkey mouse click plan: all checks passed\n");
    return 0;
}
