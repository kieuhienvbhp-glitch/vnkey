#!/bin/bash

xgettext \
--language=C++ \
--from-code=UTF-8 \
--keyword=_ \
--keyword=N_ \
-o /tmp/vnkey-cpp.pot \
$(find . \( -name "*.cpp" -o -name "*.h" \))

xgettext \
--language=appdata \
--from-code=UTF-8 \
-o /tmp/vnkey-xml.pot \
org.fcitx.Fcitx5.Addon.VNKey.metainfo.xml.in.in

xgettext \
--language=Python \
--from-code=UTF-8 \
--keyword=_ \
--keyword=N_ \
-o /tmp/vnkey-python.pot \
$(find . -name "*.py")

xgettext \
--language=Desktop \
--from-code=UTF-8 \
--keyword=Name \
--keyword=Comment \
-o /tmp/vnkey-desktop.pot \
settings-gui/org.fcitx.Fcitx5.Addon.VNKey.Settings.desktop.in

{
    echo 'msgid ""'
    echo 'msgstr ""'
    echo '"Content-Type: text/plain; charset=UTF-8\n"'
    echo ""
    grep -hE "^Name=" \
    src/vnkey.conf.in \
    src/vnkey-addon.conf.in.in \
    | sed 's/^Name=\(.*\)/msgid "\1"\nmsgstr ""\n/'
} > /tmp/vnkey-conf.pot

msgcat \
--use-first \
--sort-output \
/tmp/vnkey-cpp.pot \
/tmp/vnkey-xml.pot \
/tmp/vnkey-conf.pot \
/tmp/vnkey-python.pot \
/tmp/vnkey-desktop.pot \
-o po/fcitx5-vnkey.pot

rm -f /tmp/vnkey-*.pot