cmake_minimum_required(VERSION 3.16)

project(fcitx5-vnkey VERSION 1.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_TESTING "Build VNKey tests" OFF)

# The addon entry point is intentionally excluded: headless tests link the
# production implementation through the vnkey_test_core target, never the
# module build itself.
set(fcitx_vnkey_core_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-kb-sender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-dbus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-x11.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-fcitx-text.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-mode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-mode-menu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-candidates.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-monitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window_focus_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-plasma-theme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-key-buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-notify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/emoji.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-wlr-virtual-keyboard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-delayed-task-runner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vnkey-x11-keyboard.cpp
)

set(REQUIRED_FCITX_VERSION 5.0.14)
find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core ${REQUIRED_FCITX_VERSION} REQUIRED)
find_package(Fcitx5ModuleEmoji REQUIRED)
find_package(Pthread REQUIRED)
find_package(Gettext REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(LIBINPUT REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
pkg_check_modules(XCB_XTEST REQUIRED IMPORTED_TARGET xcb-xtest)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
set(VNKEY_VK_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/virtual-keyboard-unstable-v1.xml")

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-vnkey\") 

if (Fcitx5Utils_VERSION VERSION_GREATER_EQUAL "5.1.13")
    add_definitions(-DVNKEY_USE_MODERN_FCITX_API)
    message(STATUS "Fcitx5 version ${Fcitx5Utils_VERSION} detected: Using modern API")
else()
    message(STATUS "Fcitx5 version ${Fcitx5Utils_VERSION} detected: Using legacy API")
endif()
fcitx5_add_i18n_definition()

# Absolute paths that every packaging has to override.
set(VNKEY_FCITX5_EXECUTABLE "/usr/bin/fcitx5" CACHE STRING
    "fcitx5 executable allowed to connect to the VNKey uinput/mouse sockets")
set(VNKEY_SERVER_EXECUTABLE "/usr/bin/fcitx5-vnkey-server" CACHE STRING
    "fcitx5-vnkey-server executable allowed to connect to the VNKey mouse socket")
set(VNKEY_ALT_EXECUTABLE_PREFIX "" CACHE STRING
    "Additional prefix whose <prefix>*bin/<name> executables are accepted too (NixOS: /nix/store/)")
set(VNKEY_SETFACL_EXECUTABLE "/usr/bin/setfacl" CACHE FILEPATH
    "setfacl used by the udev rule and the service units")

add_subdirectory(common)
add_subdirectory(po)
add_subdirectory(vnkey)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(server)
add_subdirectory(misc)
add_subdirectory(settings-gui)

if (BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml.in.in"
    "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml.in"
    @ONLY
)

fcitx5_translate_desktop_file(
    "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml.in"
    org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml
    XML
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

install(FILES 
        LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
