# Core sources are compiled once here and consumed by both the module and
# vnkey_test_core; VNKEY_EXPORT keeps the test-facing API linkable.
add_library(vnkey_core_objects OBJECT ${fcitx_vnkey_core_sources})
set_target_properties(vnkey_core_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(vnkey_core_objects PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/common
)
target_compile_definitions(vnkey_core_objects PRIVATE
    FCITX5_VNKEY_SETTINGS_PATH="${CMAKE_INSTALL_FULL_BINDIR}/fcitx5-vnkey-settings"
    VNKEY_SERVER_EXECUTABLE="${VNKEY_SERVER_EXECUTABLE}"
    VNKEY_ALT_EXECUTABLE_PREFIX="${VNKEY_ALT_EXECUTABLE_PREFIX}"
)
target_link_libraries(vnkey_core_objects PRIVATE
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

set(VNKEY_VK_HEADER "${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1-client-protocol.h")
set(VNKEY_VK_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/virtual-keyboard-unstable-v1-protocol.c")
add_custom_command(
    OUTPUT "${VNKEY_VK_HEADER}"
    COMMAND "${WAYLAND_SCANNER}" client-header "${VNKEY_VK_XML}" "${VNKEY_VK_HEADER}"
    DEPENDS "${VNKEY_VK_XML}"
    VERBATIM
)
add_custom_command(
    OUTPUT "${VNKEY_VK_SOURCE}"
    COMMAND "${WAYLAND_SCANNER}" private-code "${VNKEY_VK_XML}" "${VNKEY_VK_SOURCE}"
    DEPENDS "${VNKEY_VK_XML}"
    VERBATIM
)
set_source_files_properties("${VNKEY_VK_SOURCE}" PROPERTIES
    LANGUAGE C
    SKIP_PRECOMPILE_HEADERS ON
)
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/vnkey-wlr-virtual-keyboard.cpp"
    PROPERTIES OBJECT_DEPENDS "${VNKEY_VK_HEADER}"
)
target_sources(vnkey_core_objects PRIVATE
    "${VNKEY_VK_HEADER}"
    "${VNKEY_VK_SOURCE}"
)

# Parse the shared fcitx5 headers once instead of once per TU.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    set(VNKEY_PCH_HEADERS
        <fcitx/instance.h>
        <fcitx/addonmanager.h>
        <fcitx/addonfactory.h>
        <fcitx/inputmethodengine.h>
        <fcitx/action.h>
        <fcitx/inputcontext.h>
        <fcitx/candidatelist.h>
        <fcitx/inputpanel.h>
        <fcitx/text.h>
        <fcitx/menu.h>
        <fcitx/userinterface.h>
        <fcitx/userinterfacemanager.h>
        <fcitx-config/iniparser.h>
        <fcitx-utils/log.h>
        <fcitx-utils/event.h>
        <fcitx-utils/keysym.h>
        <fcitx-utils/utf8.h>
    )
    # genex needed because target has a .c source.
    target_precompile_headers(vnkey_core_objects PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/instance.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/addonmanager.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/addonfactory.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/inputmethodengine.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/action.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/inputcontext.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/candidatelist.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/inputpanel.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/text.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/menu.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/userinterface.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx/userinterfacemanager.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx-config/iniparser.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx-utils/log.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx-utils/event.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx-utils/keysym.h$<ANGLE-R>>"
        "$<$<COMPILE_LANGUAGE:CXX>:<fcitx-utils/utf8.h$<ANGLE-R>>"
    )
    # test/ reuses the same list for its own PCH host (sibling scope).
    set(VNKEY_PCH_HEADERS "${VNKEY_PCH_HEADERS}" PARENT_SCOPE)
endif()

add_library(vnkey MODULE vnkey.cpp
    $<TARGET_OBJECTS:vnkey_core_objects>
)
set_target_properties(vnkey PROPERTIES OUTPUT_NAME "vnkey")
set_target_properties(vnkey PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(vnkey
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

target_include_directories(vnkey PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
# Same flags as vnkey_core_objects so its prebuilt PCH is reused, not rejected.
target_compile_definitions(vnkey PRIVATE
    FCITX5_VNKEY_SETTINGS_PATH="${CMAKE_INSTALL_FULL_BINDIR}/fcitx5-vnkey-settings"
    VNKEY_SERVER_EXECUTABLE="${VNKEY_SERVER_EXECUTABLE}"
    VNKEY_ALT_EXECUTABLE_PREFIX="${VNKEY_ALT_EXECUTABLE_PREFIX}"
)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16)
    target_precompile_headers(vnkey REUSE_FROM vnkey_core_objects)
endif()
install(TARGETS vnkey DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")

fcitx5_translate_desktop_file(vnkey.conf.in vnkey.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vnkey.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
configure_file(vnkey-addon.conf.in.in vnkey-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/vnkey-addon.conf.in" vnkey-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vnkey-addon.conf" RENAME vnkey.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
