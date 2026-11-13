/*
 * SPDX-FileCopyrightText: 2026 Nguyen Hoàng Ky <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "vnkey-notify.h"
#include "vnkey-log.h"
#include "vnkey-utils.h"

#include <fcitx-utils/i18n.h>
#include <libnotify/notify.h>

#include <memory>
#include <string>

namespace {
    constexpr int64_t kNotifyCooldownMs = 60000;

    constexpr int     kNotifyTimeoutMs = 20000;

    int64_t           lastNotifyMs = 0;

    struct GErrorDeleter {
        void operator()(GError* const error) const noexcept {
            if (error != nullptr) {
                g_error_free(error);
            }
        }
    };

    struct NotificationDeleter {
        void operator()(NotifyNotification* const notification) const noexcept {
            if (notification != nullptr) {
                g_object_unref(notification);
            }
        }
    };

    using GErrorPtr       = std::unique_ptr<GError, GErrorDeleter>;
    using NotificationPtr = std::unique_ptr<NotifyNotification, NotificationDeleter>;

    class NotifyLibrary {
      public:
        NotifyLibrary() : initialised_(notify_init("fcitx5-vnkey") != FALSE) {}

        ~NotifyLibrary() {
            if (initialised_) {
                notify_uninit();
            }
        }

        NotifyLibrary(const NotifyLibrary&)            = delete;
        NotifyLibrary& operator=(const NotifyLibrary&) = delete;
        NotifyLibrary(NotifyLibrary&&)                 = delete;
        NotifyLibrary&     operator=(NotifyLibrary&&)  = delete;

        [[nodiscard]] bool initialised() const {
            return initialised_;
        }

      private:
        bool initialised_ = false;
    };

    bool notifyLibraryReady() {
        static const NotifyLibrary library;
        return library.initialised();
    }
} // namespace

void notifyMissingUinputServer() {
    const int64_t now = nowMs();
    if (lastNotifyMs != 0 && now - lastNotifyMs < kNotifyCooldownMs) {
        return;
    }
    lastNotifyMs = now;
    if (!notifyLibraryReady()) {
        VNKEY_WARN("Cannot initialise libnotify");
        return;
    }
    constexpr std::string_view kEnableServerHint = "sudo systemctl enable --now fcitx5-vnkey-server@$(whoami).service";
    std::string                body          = _("Uinput modes need the VNKey uinput server. Enable the server service for your distribution and init system.");
    body += '\n';
    body += kEnableServerHint;

    NotificationPtr notification{notify_notification_new(_("VNKey: the uinput server is not running"), body.c_str(), "fcitx5-vnkey")};
    if (notification == nullptr) {
        VNKEY_WARN("Cannot create the uinput server notification");
        return;
    }
    notify_notification_set_timeout(notification.get(), kNotifyTimeoutMs);
    notify_notification_set_urgency(notification.get(), NOTIFY_URGENCY_NORMAL);
    GError*         error = nullptr;
    const bool      shown = notify_notification_show(notification.get(), &error) != FALSE;
    const GErrorPtr failure{error};
    if (!shown) {
        VNKEY_WARN(std::string("Cannot show the uinput server notification: ") + (failure != nullptr ? failure->message : "unknown error"));
    }
}
