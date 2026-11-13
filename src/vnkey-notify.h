/*
 * SPDX-FileCopyrightText: 2026 Nguyen Hoàng Ky <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file vnkey-notify.h
 * @brief Desktop notifications sent by the VNKey addon.
 */

#ifndef _VNKEY_NOTIFY_H_
#define _VNKEY_NOTIFY_H_

/**
 * @brief Tells the user that uinput modes cannot work without the VNKey uinput server.
 */
void notifyMissingUinputServer();

#endif // _VNKEY_NOTIFY_H_
