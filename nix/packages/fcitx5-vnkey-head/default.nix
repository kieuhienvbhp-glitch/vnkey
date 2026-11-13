{
  callPackage,
  inputs,
}:
(callPackage ../fcitx5-vnkey { }).overrideAttrs (old: {
  pname = "fcitx5-vnkey-head";
  version = "0-unstable";
  src = inputs.self;
})
