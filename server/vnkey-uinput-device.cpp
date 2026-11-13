/*
 * SPDX-FileCopyrightText: 2026 Nguyễn Hoàng Kỳ  <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "vnkey-uinput-device.h"

#include <string>

LibevdevDevice buildVNKeyUinputDevice() {
    LibevdevDevice device{libevdev_new()};
    if (device == nullptr) {
        return device;
    }
    libevdev_set_name(device.get(), std::string(kVNKeyUinputName).c_str());
    libevdev_set_id_bustype(device.get(), BUS_USB);
    libevdev_set_id_vendor(device.get(), kVNKeyUinputVendor);
    libevdev_set_id_product(device.get(), kVNKeyUinputProduct);
    libevdev_enable_event_type(device.get(), EV_KEY);
    for (const unsigned int code : kVNKeyUinputKeys) {
        libevdev_enable_event_code(device.get(), EV_KEY, code, nullptr);
    }
    return device;
}
