/*
 * SPDX-FileCopyrightText: 2025 Võ Ngô Hoàng Thành <thanhpy2009@gmail.com>
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file vnkey-log.h
 * @brief Log category and message macros of the addon.
 *
 * Macro-only header: only translation units that log include it, so the log
 * header chain stays out of every other consumer of vnkey-utils.h.
 */

#ifndef _FCITX5_VNKEY_LOG_H_
#define _FCITX5_VNKEY_LOG_H_

#include <fcitx-utils/log.h>

// The log category lives inside namespace fcitx: a global "vnkey" function would collide with the
// vnkey:: engine namespace, which the addon includes since it drives the C++ engine directly.
namespace fcitx {
    FCITX_DECLARE_LOG_CATEGORY(vnkeyLog);
}

#define VNKEY_DEBUG(msg) FCITX_LOGC(fcitx::vnkeyLog, Debug) << "[DEBUG] " << msg
#define VNKEY_INFO(msg)  FCITX_LOGC(fcitx::vnkeyLog, Info) << "[INFO] " << msg
#define VNKEY_WARN(msg)  FCITX_LOGC(fcitx::vnkeyLog, Warn) << "[WARN] " << msg
#define VNKEY_ERROR(msg) FCITX_LOGC(fcitx::vnkeyLog, Error) << "[ERROR] " << msg

#endif // _FCITX5_VNKEY_LOG_H_
