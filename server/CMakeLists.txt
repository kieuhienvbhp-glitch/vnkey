find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBINPUT REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(LIBUDEV  REQUIRED IMPORTED_TARGET libudev)

add_executable(fcitx5-vnkey-server vnkey-server.cpp vnkey-server-socket.cpp vnkey-logger.cpp vnkey-abstract-socket.cpp vnkey-uinput-device.cpp vnkey-physical-shift.cpp)

target_link_libraries(fcitx5-vnkey-server
    vnkey_common_executable_path
    PkgConfig::LIBINPUT
    PkgConfig::LIBUDEV
    PkgConfig::LIBEVDEV
)

target_compile_definitions(fcitx5-vnkey-server PRIVATE
    VNKEY_FCITX5_EXECUTABLE="${VNKEY_FCITX5_EXECUTABLE}"
    VNKEY_ALT_EXECUTABLE_PREFIX="${VNKEY_ALT_EXECUTABLE_PREFIX}"
)

install(TARGETS fcitx5-vnkey-server DESTINATION ${CMAKE_INSTALL_BINDIR})
