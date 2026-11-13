/*
 * SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file common-export.h
 * @brief Dependency-free exported-API marker (single VNKEY_EXPORT macro).
 */

#ifndef _COMMON_EXPORT_H_
#define _COMMON_EXPORT_H_

// Exported API marker: the project builds with hidden visibility, but the
// headless tests link shared symbols from vnkey_test_core. Keep this header
// dependency-free, like fcitx5's fcitx-utils/macros.h.
#if defined(__GNUC__) || defined(__clang__)
#define VNKEY_EXPORT __attribute__((visibility("default")))
#else
#define VNKEY_EXPORT
#endif

#endif // _COMMON_EXPORT_H_
