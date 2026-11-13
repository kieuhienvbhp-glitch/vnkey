/*
 * SPDX-FileCopyrightText: 2022-2022 CSSlayer <wengxt@gmail.com>
 * SPDX-FileCopyrightText: 2025 Võ Ngô Hoàng Thành <thanhpy2009@gmail.com>
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
/**
 * @file vnkey.cpp
 * @brief Addon entry point of the VNKey input method.
 */
#include "vnkey-engine.h"

namespace fcitx {

    /**
     * @brief Factory class for creating VNKeyEngine instances.
     */
    class VNKeyFactory : public AddonFactory {
      public:
        /**
         * @brief Creates a new VNKeyEngine instance.
         * @param manager Pointer to addon manager.
         * @return New engine instance.
         */
        AddonInstance* create(AddonManager* manager) override {
            registerDomain("fcitx5-vnkey", FCITX_INSTALL_LOCALEDIR);
            return new VNKeyEngine(manager->instance()); // NOLINT
        }
    };

} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::VNKeyFactory)
