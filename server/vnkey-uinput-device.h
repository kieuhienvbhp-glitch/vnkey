/*
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/**
 * @file vnkey-uinput-device.h
 * @brief Description of the virtual keyboard fcitx5-vnkey-server injects keys through.
 */

#ifndef _VNKEY_UINPUT_DEVICE_H_
#define _VNKEY_UINPUT_DEVICE_H_

#include <libevdev/libevdev-uinput.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

/**
 * @brief Name of the virtual keyboard, identical to the raw setup it replaces.
 */
inline constexpr std::string_view kVNKeyUinputName = "VNKey-Uinput-Server";

/**
 * @brief Vendor id of the virtual keyboard.
 */
inline constexpr uint16_t kVNKeyUinputVendor = 0x1234;

/**
 * @brief Product id of the virtual keyboard.
 */
inline constexpr uint16_t kVNKeyUinputProduct = 0x5678;

/**
 * @brief Keys the virtual keyboard can emit, in the order they are enabled.
 */
inline constexpr auto kVNKeyUinputKeys = std::to_array<unsigned int>({KEY_BACKSPACE, KEY_LEFT, KEY_LEFTSHIFT, KEY_DELETE, KEY_PAUSE});

/**
 * @brief Frees a libevdev device.
 */
struct LibevdevDeleter {
    /**
     * @brief Releases the device handle.
     * @param device Device to free, may be null.
     */
    void operator()(libevdev* device) const noexcept {
        libevdev_free(device);
    }
};

/**
 * @brief Destroys a uinput device, closing /dev/uinput when it was opened by libevdev.
 */
struct LibevdevUinputDeleter {
    /**
     * @brief Releases the uinput device handle.
     * @param device Device to destroy, may be null.
     */
    void operator()(libevdev_uinput* device) const noexcept {
        libevdev_uinput_destroy(device); // NOLINT(cppcoreguidelines-owning-memory)
    }
};

/**
 * @brief Owning handle of a libevdev device.
 */
using LibevdevDevice = std::unique_ptr<libevdev, LibevdevDeleter>;

/**
 * @brief Owning handle of a uinput device.
 */
using LibevdevUinput = std::unique_ptr<libevdev_uinput, LibevdevUinputDeleter>;

/**
 * @brief Builds the in-memory description of the virtual keyboard, like the old UI_SET_* setup.
 *
 * @return Description, or a null handle when libevdev cannot allocate.
 */
LibevdevDevice buildVNKeyUinputDevice();

#endif // _VNKEY_UINPUT_DEVICE_H_
