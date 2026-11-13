# Wraps the core objects compiled once in src/vnkey_core_objects for the headless tests.
add_library(vnkey_test_core SHARED
    $<TARGET_OBJECTS:vnkey_core_objects>
)
target_include_directories(vnkey_test_core PUBLIC
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(vnkey_test_core PUBLIC
    vnkey_common_executable_path
    Fcitx5::Core
    Fcitx5::Config
    Fcitx5::Module::Emoji
    VNKey::Engine
    PkgConfig::LIBNOTIFY
    PkgConfig::DBUS
    PkgConfig::XCB
    PkgConfig::XCB_XTEST
    PkgConfig::WAYLAND_CLIENT
    Pthread::Pthread
)

# The headless tests compile without PIC while vnkey_core_objects builds with
# -fPIC for the module, and clang rejects a reused PCH whose PIC setting
# differs from the current file. Host the tests' PCH in an object library
# whose flags match plain executables instead.
add_library(vnkey_test_pch OBJECT EXCLUDE_FROM_ALL vnkey-test-pch.cpp)
target_include_directories(vnkey_test_pch PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(vnkey_test_pch PRIVATE
    vnkey_test_core
    Fcitx5::Core
    Fcitx5::Config
    Fcitx5::Module::Emoji
    Pthread::Pthread
)
target_compile_definitions(vnkey_test_pch PRIVATE
    FCITX5_VNKEY_SETTINGS_PATH="${CMAKE_INSTALL_FULL_BINDIR}/fcitx5-vnkey-settings"
    VNKEY_SERVER_EXECUTABLE="${VNKEY_SERVER_EXECUTABLE}"
    VNKEY_ALT_EXECUTABLE_PREFIX="${VNKEY_ALT_EXECUTABLE_PREFIX}"
)
target_precompile_headers(vnkey_test_pch PRIVATE ${VNKEY_PCH_HEADERS})

# Helper to compile a standalone headless integration test linked with the
# VNKey engine core.
#
# Usage:
#   add_vnkey_headless_test(TARGET <target_name> SOURCES <source_files...> LABELS "<labels>" [INCLUDE_DIRECTORIES <dirs...>] [LINK_LIBRARIES <libs...>])
# Example:
#   add_vnkey_headless_test(TARGET my_test SOURCES my-test.cpp LABELS "integration;myfeature")
function(add_vnkey_headless_test)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES LABELS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "add_vnkey_headless_test: Unrecognized arguments: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "add_vnkey_headless_test: Missing TARGET!")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_vnkey_headless_test: Target '${ARG_TARGET}' is missing SOURCES!")
    endif()

    add_executable(${ARG_TARGET} ${ARG_SOURCES})
    # vnkey_test_pch mirrors the executables' flags, so its prebuilt PCH is
    # reused, not rejected.
    target_compile_definitions(${ARG_TARGET} PRIVATE
        FCITX5_VNKEY_SETTINGS_PATH="${CMAKE_INSTALL_FULL_BINDIR}/fcitx5-vnkey-settings"
        VNKEY_SERVER_EXECUTABLE="${VNKEY_SERVER_EXECUTABLE}"
        VNKEY_ALT_EXECUTABLE_PREFIX="${VNKEY_ALT_EXECUTABLE_PREFIX}"
    )
    target_precompile_headers(${ARG_TARGET} REUSE_FROM vnkey_test_pch)
    target_link_libraries(${ARG_TARGET} PRIVATE
        vnkey_test_core
        Fcitx5::Core
        Fcitx5::Config
        Fcitx5::Module::Emoji
        Pthread::Pthread
        ${ARG_LINK_LIBRARIES}
    )
    if(ARG_INCLUDE_DIRECTORIES)
        target_include_directories(${ARG_TARGET} PRIVATE
            ${ARG_INCLUDE_DIRECTORIES}
        )
    endif()
    add_test(NAME ${ARG_TARGET} COMMAND ${ARG_TARGET})
    set_tests_properties(${ARG_TARGET} PROPERTIES LABELS "${ARG_LABELS}" TIMEOUT 20)
endfunction()

# One binary runs every engine unit test group via "--run <fn>"; ctest keeps
# one entry per group (own process), so labels, timeouts and isolation hold.
#
# Usage:
#   add_vnkey_unit_test(<ctest_name> <run_function> <source_file> "<labels>")
function(add_vnkey_unit_test name fn source labels)
    target_sources(vnkey_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${source})
    add_test(NAME ${name} COMMAND vnkey_unit_tests --run ${fn})
    set_tests_properties(${name} PROPERTIES LABELS "${labels}" TIMEOUT 60)
endfunction()

add_executable(vnkey_unit_tests vnkey-unit-test-main.cpp)
target_link_libraries(vnkey_unit_tests PRIVATE VNKey::Engine)
target_compile_definitions(vnkey_unit_tests PRIVATE VNKEY_TEST_GOLDEN_DIR="${PROJECT_SOURCE_DIR}/test/golden")

set_target_properties(vnkey_unit_tests PROPERTIES UNITY_BUILD OFF)

add_vnkey_unit_test(vnkey_utf8 utf8 vnkey-utf8-test.cpp "unit;vnkey")
add_vnkey_unit_test(vnkey_data data vnkey-data-test.cpp "unit;vnkey")
add_vnkey_unit_test(vnkey_utils utils vnkey-utils-test.cpp "unit;vnkey")
add_vnkey_unit_test(vnkey_encoder encoder vnkey-encoder-test.cpp "unit;vnkey;charset")
add_vnkey_unit_test(vnkey_rules_parser rules_parser vnkey-rules-parser-test.cpp "unit;vnkey;rules")
add_vnkey_unit_test(vnkey_flattener flattener vnkey-flattener-test.cpp "unit;vnkey;flattener")
add_vnkey_unit_test(vnkey_spelling spelling vnkey-spelling-test.cpp "unit;vnkey;spelling")
add_vnkey_unit_test(vnkey_engine_utils engine_utils vnkey-engine-utils-test.cpp "unit;vnkey;engine-utils")
add_vnkey_unit_test(vnkey_engine_smoke engine_smoke vnkey-engine-smoke.cpp "unit;vnkey;engine")
add_vnkey_unit_test(vnkey_engine_test engine vnkey-engine-test.cpp "unit;vnkey;engine")
add_vnkey_unit_test(vnkey_rebuild rebuild vnkey-rebuild-test.cpp "unit;vnkey;rebuild")
add_vnkey_unit_test(vnkey_macro_dictionary macro_dictionary vnkey-macro-dictionary-test.cpp "unit;vnkey;macro")
add_vnkey_unit_test(vnkey_key_processor key_processor vnkey-key-processor-test.cpp "unit;vnkey;key-processor")
add_vnkey_unit_test(vnkey_golden_smoke golden_smoke vnkey-golden-smoke.cpp "unit;vnkey;golden")
add_vnkey_unit_test(numeric_macro_case_preservation numeric_macro_case numeric-macro-case-preservation.cpp "unit;macro")

add_vnkey_headless_test(TARGET preedit_lifecycle SOURCES preedit-lifecycle.cpp LABELS "integration;preedit")
add_vnkey_headless_test(TARGET preedit_smoke_client SOURCES preedit-smoke-client.cpp LABELS "integration;preedit")
add_vnkey_headless_test(TARGET preedit_smoke_server SOURCES preedit-smoke-server.cpp LABELS "integration;preedit")
add_vnkey_headless_test(TARGET app_rule_reset SOURCES app-rule-reset.cpp LABELS "integration;apprule")
add_vnkey_headless_test(TARGET app_rule_delays SOURCES app-rule-delays.cpp LABELS "integration;apprule")
add_vnkey_headless_test(TARGET app_rule_delay_wire SOURCES app-rule-delay-wire.cpp LABELS "integration;apprule")
add_vnkey_headless_test(TARGET smooth_buffered_key_replay SOURCES smooth-buffered-key-replay.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET focus_bounce_word_survival SOURCES focus-bounce-word-survival.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET focus_out_mid_replacement SOURCES focus-out-mid-replacement.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET uinput_auto_restore SOURCES uinput-auto-restore.cpp LABELS "integration;uinput")
add_vnkey_headless_test(TARGET anchor_key_protocol SOURCES anchor-key-protocol.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET uinput_server_probe SOURCES uinput-server-probe.cpp LABELS "unit;server")
add_vnkey_headless_test(TARGET preedit_backspace SOURCES preedit-backspace.cpp LABELS "integration;preedit")
add_vnkey_headless_test(TARGET select_mode_replacement SOURCES select-mode-replacement.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET select_modifier_replay SOURCES select-modifier-replay.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET chromium_address_bar SOURCES chromium-address-bar.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET select_anchor_completion SOURCES select-anchor-completion.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET select_cancel SOURCES select-cancel.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET select_cancel_reconcile SOURCES select-cancel-reconcile.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET select_word_break SOURCES select-word-break.cpp LABELS "integration;select")
add_vnkey_headless_test(TARGET replacement_delays SOURCES replacement-delays.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET merged_replacement_commit SOURCES merged-replacement-commit.cpp LABELS "integration;smooth")
add_vnkey_headless_test(TARGET dict_reload SOURCES dict-reload.cpp LABELS "integration;dictionary")
add_vnkey_headless_test(TARGET config_enum_names SOURCES config-enum-names.cpp LABELS "unit;config")
add_vnkey_headless_test(
    TARGET uinput_device
    SOURCES
        uinput-device.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-uinput-device.cpp
    LABELS "unit;server"
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/server
    LINK_LIBRARIES PkgConfig::LIBEVDEV
)
add_vnkey_headless_test(
    TARGET select_shift_plan
    SOURCES
        select-shift-plan.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-physical-shift.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-logger.cpp
    LABELS "unit;server"
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/server
    LINK_LIBRARIES PkgConfig::LIBEVDEV PkgConfig::LIBINPUT
)
add_vnkey_headless_test(
    TARGET server_socket_init
    SOURCES
        server-socket-init.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-server-socket.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-logger.cpp
        ${PROJECT_SOURCE_DIR}/server/vnkey-abstract-socket.cpp
    LABELS "unit;server"
    INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/server ${PROJECT_SOURCE_DIR}/common
)
add_vnkey_headless_test(TARGET status_icon SOURCES status-icon.cpp LABELS "integration;icons")
add_test(NAME status_icon_gnome COMMAND status_icon gnome)
set_tests_properties(status_icon_gnome PROPERTIES LABELS "integration;icons" TIMEOUT 20)
add_vnkey_headless_test(TARGET key_buffer SOURCES key-buffer.cpp LABELS "unit;buffer")
add_vnkey_headless_test(TARGET auto_capitalize_reset SOURCES auto-capitalize-reset.cpp LABELS "integration;capitalize")
add_vnkey_headless_test(TARGET auto_capitalize_shortcut SOURCES auto-capitalize-shortcut.cpp LABELS "integration;capitalize")
add_vnkey_headless_test(TARGET dbus_deferred_commit SOURCES dbus-deferred-commit.cpp LABELS "integration;dbus")
add_vnkey_headless_test(TARGET modifier_key_bypasses_surrounding_text SOURCES modifier-key-bypasses-surrounding-text.cpp LABELS "integration;surrounding")
add_vnkey_headless_test(TARGET surrounding_text_typing SOURCES surrounding-text-typing.cpp LABELS "integration;surrounding")
add_vnkey_headless_test(TARGET runtime_safety SOURCES runtime-safety.cpp LABELS "integration;raii")
add_vnkey_headless_test(TARGET emoji_history_page_navigation SOURCES emoji-history-page-navigation.cpp LABELS "integration;emoji")
add_vnkey_headless_test(TARGET emoji_search_history SOURCES emoji-search-history.cpp LABELS "unit;emoji")
add_vnkey_headless_test(TARGET emoji_history_unit SOURCES emoji-history-unit.cpp LABELS "unit;emoji")
add_vnkey_headless_test(TARGET kb_sender SOURCES kb-sender.cpp LABELS "unit;socket")
add_vnkey_headless_test(TARGET program_name SOURCES program-name.cpp LABELS "unit;apprule")
add_vnkey_headless_test(TARGET mouse_click_plan SOURCES mouse-click-plan.cpp LABELS "unit;preedit")
add_vnkey_headless_test(TARGET x11_keyboard_inject SOURCES x11-keyboard-inject.cpp LABELS "integration;x11" LINK_LIBRARIES PkgConfig::XCB_XTEST PkgConfig::XCB)
# The real check needs an X server with Xtest: run it again under a throwaway Xvfb.
find_program(XVFB_RUN xvfb-run)
if(XVFB_RUN)
    add_test(NAME x11_keyboard_inject_xvfb COMMAND ${XVFB_RUN} -a $<TARGET_FILE:x11_keyboard_inject>)
    set_tests_properties(x11_keyboard_inject_xvfb PROPERTIES LABELS "integration;x11" TIMEOUT 60 ENVIRONMENT "VNKEY_X11_KEYBOARD_TEST=1")
endif()
add_vnkey_headless_test(TARGET window_focus_tracker SOURCES window-focus-tracker.cpp LABELS "integration;apprule;dbus")
target_compile_definitions(window_focus_tracker PRIVATE VNKEY_TEST_DBUS_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/dbus-session.conf")
set_tests_properties(kb_sender PROPERTIES
    RESOURCE_LOCK vnkey_kb_socket
    ENVIRONMENT "VNKEY_SOCKET_NAMESPACE=ctest-${VNKEY_CTEST_SOCKET_NAMESPACE}")

string(MD5 VNKEY_CTEST_SOCKET_NAMESPACE "${PROJECT_BINARY_DIR}")
set_tests_properties(smooth_buffered_key_replay focus_bounce_word_survival focus_out_mid_replacement uinput_auto_restore anchor_key_protocol uinput_server_probe select_mode_replacement select_modifier_replay chromium_address_bar select_anchor_completion select_cancel select_cancel_reconcile select_word_break replacement_delays app_rule_delay_wire dbus_deferred_commit merged_replacement_commit runtime_safety
    PROPERTIES RESOURCE_LOCK vnkey_kb_socket
    ENVIRONMENT "VNKEY_SOCKET_NAMESPACE=ctest-${VNKEY_CTEST_SOCKET_NAMESPACE}")

add_executable(executable_path_match
    executable-path-match.cpp
)
target_link_libraries(executable_path_match PRIVATE vnkey_common_executable_path)
add_test(NAME executable_path_match COMMAND executable_path_match)
set_tests_properties(executable_path_match PROPERTIES
    LABELS "unit;packaging"
    TIMEOUT 20
)

# Headless Python unit tests for the settings-gui, one CTest entry per module.
add_subdirectory(settings-gui)
