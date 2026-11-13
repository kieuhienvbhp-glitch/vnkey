/*
 * SPDX-FileCopyrightText: 2026 Nguyen Hoang Ky <nhktmdzhg@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * @file vnkey-x11-keyboard.h
 * @brief X11 keyboard backend built on the Xtest extension.
 */

#ifndef _VNKEY_X11_KEYBOARD_H_
#define _VNKEY_X11_KEYBOARD_H_

#include "vnkey-delayed-task-runner.h"
#include "vnkey-x11.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace fcitx {

    /**
     * @brief Injects keys into the X server through Xtest.
     *
     * Behaves like the wlroots virtual keyboard: BackSpace, Shift+Left, Delete and text are sent as real key events,
     * so the focused client and the input method both see them in order. Text uses the keysyms of spare keycodes,
     * which are remapped for the burst and always restored afterwards. All X calls run on the shared runner thread,
     * where the mandatory per-key interval is spent, so the fcitx main thread never sleeps.
     */
    class VNKEY_EXPORT VNKeyX11Keyboard {
      public:
        static VNKeyX11Keyboard& instance();

        /**
         * @brief Probes the X server once for the Xtest extension.
         * @return True when keys can be injected.
         */
        bool available();

        /**
         * @brief Reports whether either Shift key is currently held on the X keyboard.
         * @return True when Shift is down, or when the state cannot be read (fail safe).
         */
        [[nodiscard]] static bool shiftHeld();

        /**
         * @brief Queues a replacement burst.
         * @param backspaces Number of BackSpace (or Shift+Left when @p select) events.
         * @param utf8 Text to type after the deletions; may be empty.
         * @param intervalMs Delay between two injected keys, at least 1 ms.
         * @param select True to select with Shift+Left and Delete instead of backspacing.
         * @return True when the burst was queued.
         */
        bool emit(int backspaces, std::string_view utf8, int intervalMs, bool select);

        /**
         * @brief Hands a delayed task to the shared runner.
         * @param delayMs Delay in milliseconds.
         * @param task Task to run on the worker thread.
         */
        void runAfter(int delayMs, std::function<void()> task) {
            runner_.runAfter(delayMs, std::move(task));
        }

        /**
         * @brief Drops queued bursts and joins the worker.
         */
        void cancelPendingTasks();

        VNKeyX11Keyboard(const VNKeyX11Keyboard&)            = delete;
        VNKeyX11Keyboard& operator=(const VNKeyX11Keyboard&) = delete;
        VNKeyX11Keyboard(VNKeyX11Keyboard&&)                 = delete;
        VNKeyX11Keyboard& operator=(VNKeyX11Keyboard&&)      = delete;

      private:
        VNKeyX11Keyboard() = default;
        ~VNKeyX11Keyboard();

        xcb_connection_t*      ensureConnection();
        void                   runBurst(int backspaces, const std::vector<uint32_t>& cps, int intervalMs, bool select);

        std::mutex             connectionMutex_;   ///< Guards the worker connection
        XcbConnectionPtr       connection_;        ///< X server connection owned by the runner thread
        std::atomic<bool>      cancelled_{false};  ///< Set by cancelPendingTasks() to cut a burst short
        bool                   probed_    = false; ///< The probe already ran
        bool                   supported_ = false; ///< The probe found Xtest
        VNKeyDelayedTaskRunner runner_;            ///< Paces the injected keys off the main thread
    };

} // namespace fcitx

#endif
