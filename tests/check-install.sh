#!/usr/bin/env bash
set -euo pipefail
old=$(realpath "${1:?previous binary}")
new=$(realpath "${2:?new binary}")
root=$(cd "$(dirname "$0")/.." && pwd)
workspace=$(mktemp -d)
trap 'rm -rf "$workspace"' EXIT
export HOME="$workspace/test user" XDG_CONFIG_HOME="$workspace/config" XDG_DATA_HOME="$workspace/data"
export QT_QPA_PLATFORM=offscreen
prefix="$HOME/.local/opt/lumenshot"
cmd="$HOME/.local/bin/lumenshot"
desktop="$XDG_DATA_HOME/applications/lumenshot.desktop"
startup="$XDG_CONFIG_HOME/autostart/lumenshot.desktop"
mkdir -p "$HOME/Pictures" "$(dirname "$startup")" "$XDG_CONFIG_HOME/Lumenshot"
cp "$root/assets/focus.png" "$HOME/Pictures/test.png"
touch "$XDG_CONFIG_HOME/Lumenshot/desktop.ini"
install_user() { bash "$root/packaging/user-install.sh" "$@"; }
install_user install "$old"
cmp "$old" "$prefix/bin/lumenshot"
"$cmd" --help >/dev/null
cp "$desktop" "$startup"
install_user install "$new"
cmp "$new" "$prefix/bin/lumenshot"
desktop-file-validate "$desktop"
"$cmd" --help >/dev/null
test -f "$startup"
install_user uninstall
test ! -e "$prefix"
test ! -L "$cmd"
test ! -e "$startup"
test ! -e "$desktop"
cmp "$root/assets/focus.png" "$HOME/Pictures/test.png"
test -f "$XDG_CONFIG_HOME/Lumenshot/desktop.ini"
echo "Install, upgrade, uninstall, screenshot and preference retention passed"
