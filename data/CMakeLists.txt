install(DIRECTORY icons/ DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/"
    PATTERN .* EXCLUDE
    PATTERN *~ EXCLUDE)

# ── Status/tray icon coverage: hicolor status/ ────────────────────────────
# The engine returns plain icon names (see vnkey-engine.cpp subModeIconImpl);
# the SNI host resolves them through standard XDG icon theme lookup. Install
# copies of every variant into hicolor status dirs in addition to the
# breeze/breeze-dark theme dirs installed above.
set(VNKEY_STATUS_ICONS
    fcitx-vnkey fcitx-vnkey-off fcitx-vnkey-emoji
    fcitx-vnkey-default fcitx-vnkey-default-black
    fcitx-vnkey-off-default fcitx-vnkey-off-default-black
    fcitx-vnkey-emoji-default fcitx-vnkey-emoji-default-black
    fcitx-vnkey-default-symbolic
    fcitx-vnkey-off-default-symbolic
    fcitx-vnkey-emoji-default-symbolic)

foreach(ICON ${VNKEY_STATUS_ICONS})
    # status/ — system tray (SVG, renderable by most compositors and SNI hosts)
    install(FILES icons/hicolor/scalable/apps/${ICON}.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/status"
        RENAME "${ICON}.svg")
endforeach()

install(FILES dictionaries/vietnamese.cm.dict DESTINATION "${FCITX_INSTALL_PKGDATADIR}/vnkey")
