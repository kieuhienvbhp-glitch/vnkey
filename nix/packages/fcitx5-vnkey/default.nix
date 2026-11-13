{
  lib,
  stdenv,
  acl,
  cmake,
  dbus,
  fcitx5,
  gettext,
  hicolor-icon-theme,
  kdePackages,
  libevdev,
  libinput,
  libnotify,
  libxcb,
  pkg-config,
  python3,
  qt6,
  udev,
}:

let
  pythonEnv = python3.withPackages (
    ps: with ps; [
      dbus-python
      pyqt6
      qtpy
    ]
  );
in
stdenv.mkDerivation (finalAttrs: {
  pname = "fcitx5-vnkey";
  version = "1.0.0";

  src = lib.cleanSource ../../..;

  nativeBuildInputs = [
    cmake
    gettext
    hicolor-icon-theme
    kdePackages.extra-cmake-modules
    pkg-config
    qt6.wrapQtAppsHook
  ];

  # Provides dbus-run-session for the private-bus regression when checks run.
  nativeCheckInputs = [ dbus ];

  buildInputs = [
    acl
    dbus
    fcitx5
    kdePackages.extra-cmake-modules
    libevdev
    libinput
    libnotify
    libxcb
    pythonEnv
    qt6.qtbase
    qt6.qtsvg
    udev
  ];

  strictDeps = true;

  __structuredAttrs = true;

  dontWrapQtApps = true;

 doCheck = true;

  cmakeFlags = [
    "-DVNKEY_ALT_EXECUTABLE_PREFIX=/nix/store/"
    "-DVNKEY_SETFACL_EXECUTABLE=${acl}/bin/setfacl"
    (lib.cmakeBool "BUILD_TESTING" finalAttrs.finalPackage.doCheck)
  ];

  postFixup = ''
    patchShebangs $out/share/fcitx5-vnkey/settings-gui
    wrapQtApp $out/bin/fcitx5-vnkey-settings \
      --prefix XDG_DATA_DIRS : "${hicolor-icon-theme}/share"
  '';

  meta = {
    description = "Vietnamese input method engine for Fcitx5";
    homepage = "https://github.com/kieuhienvbhp-glitch/vnkey";
    license = with lib.licenses; [
      gpl3Plus
    ];
    maintainers = with lib.maintainers; [
      justanoobcoder
    ];
    platforms = lib.platforms.linux;
  };
})
