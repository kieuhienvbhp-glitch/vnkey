set(VNKEY_UINPUT_PROXY_USER "uinput_proxy" CACHE STRING
    "System user granted rw ACL on /dev/uinput (udev rules, sysusers.d, service scripts)")

option(INSTALL_OPENRC "Install OpenRC script" OFF)
option(INSTALL_RUNIT "Install Runit script" OFF)

configure_file(99-vnkey.rules.in ${CMAKE_CURRENT_BINARY_DIR}/99-vnkey.rules @ONLY)
configure_file(fcitx5-vnkey-server@.service.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey-server@.service @ONLY)
configure_file(user-vnkey.conf.in ${CMAKE_CURRENT_BINARY_DIR}/user-vnkey.conf @ONLY)

if(INSTALL_OPENRC)
    configure_file(fcitx5-vnkey.openrc.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey.openrc @ONLY)
endif(INSTALL_OPENRC)

if(INSTALL_RUNIT)
    configure_file(fcitx5-vnkey.runit.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey.runit @ONLY)
endif(INSTALL_RUNIT)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey-server@.service"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/99-vnkey.rules"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d)

install(FILES module-fcitx5-vnkey.conf
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/modules-load.d
    RENAME fcitx5-vnkey.conf)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/user-vnkey.conf"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/sysusers.d
    RENAME vnkey.conf)

if(INSTALL_OPENRC)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey.openrc"
        DESTINATION /etc/init.d
        RENAME fcitx5-vnkey)
endif(INSTALL_OPENRC)

if(INSTALL_RUNIT)
    set(RUNIT_SV_DIR "/etc/sv" CACHE PATH
        "Path to runit service definitions directory (e.g. /etc/sv on Void, /etc/runit/sv on Artix)")
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-vnkey.runit"
        DESTINATION "${RUNIT_SV_DIR}/fcitx5-vnkey"
        RENAME run)
endif(INSTALL_RUNIT)
