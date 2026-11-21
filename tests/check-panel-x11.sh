#!/usr/bin/env bash
# Uses its own Xvfb display and session bus, including a simulated unlocked screen.
set -eu
app=$(realpath "${1:?pass the lumenshot binary}")
tmp=$(mktemp -d)
Xvfb -displayfd 3 -screen 0 "${TEST_SCREEN:-1100x760}x24" -nolisten tcp 3>"$tmp/display" &
server=$!
trap 'kill "$server" 2>/dev/null || true; rm -rf "$tmp"' EXIT
for ((i=0; i<50; i++)); do
    test ! -s "$tmp/display" || break
    sleep .1
done
test -s "$tmp/display"
export DISPLAY=:$(cat "$tmp/display") XDG_SESSION_TYPE=x11
export XDG_CONFIG_HOME="$tmp/config"
mapfile -t bus < <(dbus-daemon --session --fork --print-address=1 --print-pid=1)
export DBUS_SESSION_BUS_ADDRESS="${bus[0]}"
/usr/bin/python3 - "$tmp/locked" <<'PY' &
import os, sys
from dbus.mainloop.glib import DBusGMainLoop
from dbus import SessionBus
from dbus.service import BusName, Object, method
from gi.repository import GLib
DBusGMainLoop(set_as_default=True)
class Screen(Object):
    @method("org.gnome.ScreenSaver", out_signature="b")
    def GetActive(self):
        return os.path.exists(sys.argv[1])
screen = Screen(BusName("org.gnome.ScreenSaver", SessionBus()), "/org/gnome/ScreenSaver")
GLib.MainLoop().run()
PY
screen_service=$!
trap 'kill "$screen_service" "${bus[1]}" "$server" 2>/dev/null || true; rm -rf "$tmp"' EXIT
gdbus wait --session --timeout 5 org.gnome.ScreenSaver
openbox --sm-disable >"$tmp/wm.log" 2>&1 &
sleep .5
xmessage -name ShotTarget -geometry 240x120+55+80 'Window capture test' 2>/dev/null &
"$app" --background &
pid=$!
sleep .5
if xdotool search --onlyvisible --name '截图工作台' >/dev/null; then exit 1; fi
if [[ ${PERF_CHECK:-0} == 1 ]]; then
    /usr/bin/python3 "$(dirname "$0")/measure-x11.py" "$pid"
    kill "$pid"
    exit
fi
DISPLAY="$DISPLAY.0" timeout 4 "$app" --background
"$(dirname "$app")/check_hotkey"
# Share the user's bus across two display servers, as on separate graphical logins.
(
    Xvfb -displayfd 3 -screen 0 800x600x24 -nolisten tcp 3>"$tmp/other" &
    other=$!
    trap 'kill "$other" 2>/dev/null || true' EXIT
    for ((i=0; i<50; i++)); do test ! -s "$tmp/other" || break; sleep .1; done
    export DISPLAY=:$(cat "$tmp/other")
    "$app" --background &
    other_pid=$!
    sleep .4
    timeout 4 "$app" --capture screen
    timeout 4 xdotool search --sync --onlyvisible --name '截图工作台' >/dev/null
    kill "$other_pid"
    wait "$other_pid" || true
)
if xdotool search --onlyvisible --name '截图工作台' >/dev/null; then exit 1; fi
touch "$tmp/locked"
"$app" --capture screen
sleep .6
if xdotool search --onlyvisible --name '截图工作台' >/dev/null; then exit 1; fi
rm "$tmp/locked"
xdotool key ctrl+alt+a
sleep .7
overlay=$(timeout 3 xdotool search --onlyvisible --class lumenshot | head -1)
test -n "$overlay"
xdotool key Escape
sleep .2
if xdotool search --onlyvisible --class lumenshot >/dev/null; then exit 1; fi
"$app" --settings
settings=$(timeout 4 xdotool search --sync --onlyvisible --name 'Lumenshot · 设置' | head -1)
autostart="$XDG_CONFIG_HOME/autostart/lumenshot.desktop"
# Checkbox follows the combo box, directory button and status row.
xdotool mousemove --window "$settings" 146 119 click 1
test -f "$autostart"
desktop-file-validate "$autostart"
rg -q -- '--background' "$autostart"
xdotool mousemove --window "$settings" 146 119 click 1
test ! -e "$autostart"
xdotool key Escape
"$app"
panel=$(timeout 5 xdotool search --sync --onlyvisible --name '截图工作台' | head -1)
xdotool windowmove "$panel" 410 120
"$app"
test "$(xdotool search --onlyvisible --name '截图工作台' | wc -l)" -eq 1
click_panel() { xdotool mousemove --window "$panel" "$1" "$2" click 1; }
pick() { click_panel 305 408; sleep .7; xdotool mousemove "$1" "$2" click 1; sleep .3; }
pick 1060 30 # cancel
pick 130 135
click_panel 447 338
timeout 3 xclip -selection clipboard -target image/png -out > "$tmp/window.png"
/usr/bin/python3 - "$tmp/window.png" <<'PY'
import sys
from gi.repository import GdkPixbuf
image = GdkPixbuf.Pixbuf.new_from_file(sys.argv[1])
assert 240 <= image.get_width() < 270 and 120 <= image.get_height() < 170
PY
click_panel 350 338
pin=$(timeout 3 xdotool search --sync --onlyvisible --name '置顶预览' | head -1)
xprop -id "$pin" _NET_WM_STATE | rg -q '_NET_WM_STATE_ABOVE'
xmessage -name Obstruction -geometry 100x65+180+140 'Cover' 2>/dev/null &
sleep .4
pick 130 135
click_panel 447 338
timeout 3 xclip -selection clipboard -target image/png -out > "$tmp/after.png"
cmp "$tmp/window.png" "$tmp/after.png"
xdotool windowminimize "$pin"
xdotool windowactivate --sync "$panel"
copy_image() {
    click_panel 447 338
    timeout 3 xclip -selection clipboard -target image/png -out > "$1"
}
click_panel 43 82 # rectangle
xdotool mousemove --window "$panel" 240 170 mousedown 1 mousemove --window "$panel" 310 215 mouseup 1
copy_image "$tmp/masked.png"
if cmp -s "$tmp/window.png" "$tmp/masked.png"; then exit 1; fi
click_panel 521 82 # undo
copy_image "$tmp/undo.png"
cmp "$tmp/window.png" "$tmp/undo.png"
click_panel 566 82 # redo
copy_image "$tmp/redo.png"
cmp "$tmp/masked.png" "$tmp/redo.png"
click_panel 90 82 # ellipse
xdotool mousemove --window "$panel" 320 175 mousedown 1 mousemove --window "$panel" 365 213 mouseup 1
copy_image "$tmp/ellipse.png"
if cmp -s "$tmp/masked.png" "$tmp/ellipse.png"; then exit 1; fi
click_panel 548 338 # save
timeout 4 xdotool search --onlyvisible --sync --name '保存截图' >/dev/null
xdotool key ctrl+a
xdotool type "$tmp/export.png"
xdotool key Return
for ((i=0; i<50; i++)); do
    test ! -s "$tmp/export.png" || break
    sleep .1
done
/usr/bin/python3 - "$tmp/ellipse.png" "$tmp/export.png" <<'PY'
import sys, gi
gi.require_version("GdkPixbuf", "2.0")
from gi.repository import GdkPixbuf
a, b = [GdkPixbuf.Pixbuf.new_from_file(path) for path in sys.argv[1:]]
assert (a.get_width(), a.get_height()) == (b.get_width(), b.get_height())
assert a.get_pixels() == b.get_pixels()
PY
click_panel 125 408
sleep .7
xdotool mousemove 55 80 mousedown 1 mousemove 215 180 mouseup 1
xdotool mousemove 215 130 mousedown 1 mousemove 255 130 mouseup 1
read -r width height < <(xdotool getdisplaygeometry)
xdotool mousemove "$((width-125))" 30 click 1
sleep .3
copy_image "$tmp/resized.png"
file "$tmp/resized.png" | rg '200 x 100'
click_panel 185 82 # text
click_panel 265 175
timeout 3 xdotool search --onlyvisible --sync --name '文字标注' >/dev/null
xdotool key Escape
copy_image "$tmp/cancel-text.png"
cmp "$tmp/resized.png" "$tmp/cancel-text.png"
click_panel 250 160
dialog=$(timeout 3 xdotool search --onlyvisible --sync --name '文字标注' | head -1)
printf '中文标注' | xclip -selection clipboard -in
xdotool key ctrl+v
eval "$(xdotool getwindowgeometry --shell "$dialog")"
xdotool mousemove --window "$dialog" "$((WIDTH-49))" "$((HEIGHT-24))" click 1
sleep .2
copy_image "$tmp/text.png"
if cmp -s "$tmp/resized.png" "$tmp/text.png"; then exit 1; fi
click_panel 140 82 # arrow
xdotool mousemove --window "$panel" 320 175 mousedown 1 mousemove --window "$panel" 253 175 mouseup 1
copy_image "$tmp/arrow.png"
if cmp -s "$tmp/resized.png" "$tmp/arrow.png"; then exit 1; fi
click_panel 350 338
pin=$(timeout 3 xdotool search --sync --onlyvisible --name '置顶预览' | head -1)
xdotool windowmove "$pin" 5 35 windowsize "$pin" 400 240
sleep .2
click_panel 521 82
copy_image "$tmp/panel-after-undo.png"
cmp "$tmp/text.png" "$tmp/panel-after-undo.png"
click_pin() { xdotool mousemove --window "$pin" "$1" 28 click 1; }
click_pin 270
timeout 3 xclip -selection clipboard -target image/png -out > "$tmp/pin-copy.png"
cmp "$tmp/arrow.png" "$tmp/pin-copy.png"
click_pin 304
timeout 4 xdotool search --onlyvisible --sync --name '保存截图' >/dev/null
xdotool key ctrl+a
xdotool type "$tmp/pin-export.png"
xdotool key Return
for ((i=0; i<50; i++)); do
    test ! -s "$tmp/pin-export.png" || break
    sleep .1
done
/usr/bin/python3 - "$tmp/arrow.png" "$tmp/pin-export.png" <<'PY'
import sys, gi
gi.require_version("GdkPixbuf", "2.0")
from gi.repository import GdkPixbuf
a, b = [GdkPixbuf.Pixbuf.new_from_file(path) for path in sys.argv[1:]]
assert (a.get_width(), a.get_height(), a.get_pixels()) == (b.get_width(), b.get_height(), b.get_pixels())
PY
click_pin 338
pin=$(timeout 3 xdotool search --sync --onlyvisible --name '图片预览' | head -1)
if xprop -id "$pin" _NET_WM_STATE | rg -q '_NET_WM_STATE_ABOVE'; then exit 1; fi
click_pin 338
pin=$(timeout 3 xdotool search --sync --onlyvisible --name '置顶预览' | head -1)
xprop -id "$pin" _NET_WM_STATE | rg -q '_NET_WM_STATE_ABOVE'
click_pin 372
sleep .2
if xdotool search --onlyvisible --name '置顶预览' >/dev/null; then exit 1; fi
if [[ ${TEST_SCREEN:-} == 3840x2160 ]]; then
    click_panel 470 408
    sleep .8
    for tool in 225 269; do
        click_panel "$tool" 82
        xdotool mousemove --window "$panel" 140 135 mousedown 1 mousemove --window "$panel" 465 285 mouseup 1
        xdotool windowmove "$panel" 415 125
        sleep .4
        copy_image "$tmp/large-$tool.png"
        file "$tmp/large-$tool.png" | rg '3840 x 2160'
    done
    click_panel 548 338
    timeout 4 xdotool search --onlyvisible --sync --name '保存截图' >/dev/null
    xdotool key ctrl+a
    xdotool type "$tmp/export-4k.png"
    xdotool key Return
    for ((i=0; i<100; i++)); do
        test ! -s "$tmp/export-4k.png" || break
        sleep .05
    done
    /usr/bin/python3 - "$tmp/large-269.png" "$tmp/export-4k.png" <<'PY'
import sys, gi
gi.require_version("GdkPixbuf", "2.0")
from gi.repository import GdkPixbuf
a,b = [GdkPixbuf.Pixbuf.new_from_file(path) for path in sys.argv[1:]]
assert a.get_pixels() == b.get_pixels()
PY
fi
if command -v trayer >/dev/null; then
    trayer --edge top --align left --widthtype request --height 32 >"$tmp/tray.log" 2>&1 &
    sleep .8
    xdotool windowactivate --sync "$panel" key alt+F4
    sleep .3
    if xdotool search --onlyvisible --name '截图工作台' >/dev/null; then exit 1; fi
    xdotool mousemove 14 14 click 1
    timeout 4 xdotool search --sync --onlyvisible --name '截图工作台' >/dev/null
    echo "Tray click restored the hidden panel"
fi
kill "$pid"
echo 'X11 panel checks passed'
