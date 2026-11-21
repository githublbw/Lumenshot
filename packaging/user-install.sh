#!/usr/bin/env bash
# User installation for an Ubuntu build linked to the distribution's Qt 6.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
prefix="$HOME/.local/opt/lumenshot"
command_path="$HOME/.local/bin/lumenshot"
desktop="${XDG_DATA_HOME:-$HOME/.local/share}/applications/lumenshot.desktop"
startup="${XDG_CONFIG_HOME:-$HOME/.config}/autostart/lumenshot.desktop"

case ${1:-} in
install)
    binary=$(realpath "${2:?请指定构建好的 lumenshot 程序}")
    test -x "$binary"
    if [[ -e $command_path || -L $command_path ]]; then
        [[ $(readlink "$command_path") == "$prefix/start" ]] || { echo "已有其他同名命令，请检查 $command_path" >&2; exit 1; }
    fi
    mkdir -p "$prefix/bin" "$HOME/.local/bin" "$(dirname "$desktop")"
    # Rename rather than truncate a binary which may still be running.
    staged=$(mktemp "$prefix/bin/.lumenshot.XXXXXX")
    trap 'rm -f "$staged"' EXIT
    install -m755 "$binary" "$staged"
    install -Dm644 "$root/assets/lumenshot-icon.png" "$prefix/share/icons/hicolor/256x256/apps/lumenshot.png"
    install -Dm644 "$root/assets/LUCIDE-LICENSE" "$prefix/share/licenses/lumenshot/LUCIDE-LICENSE"
    install -m755 "$root/packaging/start" "$prefix/start"
    mv -f "$staged" "$prefix/bin/lumenshot"
    ln -sfn "$prefix/start" "$command_path"
    # OCR 可选组件：模型与推理库存在才安装。
    if compgen -G "$root/assets/ocr/*.onnx" >/dev/null; then
        mkdir -p "$prefix/share/ocr"
        install -m644 "$root"/assets/ocr/*.onnx "$prefix/share/ocr/"
    fi
    if compgen -G "$root/third_party/onnxruntime/lib/libonnxruntime.so*" >/dev/null; then
        mkdir -p "$prefix/lib"
        install -m755 "$root"/third_party/onnxruntime/lib/libonnxruntime.so* "$prefix/lib/"
    fi
    # Quote the absolute launcher even when HOME contains spaces or field codes.
    python3 - "$root/packaging/lumenshot.desktop" "$desktop" "$prefix" <<'PY'
import os, sys, tempfile
from pathlib import Path
template, destination, prefix = map(Path, sys.argv[1:])
launcher = str(prefix / "start").replace("%", "%%")
for char in ('\\', '"', '$', '`'):
    launcher = launcher.replace(char, '\\' + char)
launcher = launcher.replace('\\', '\\\\')
data = template.read_text().replace("Exec=lumenshot", f'Exec="{launcher}"')
data = data.replace("Icon=lumenshot", "Icon=" + str(prefix / "share/icons/hicolor/256x256/apps/lumenshot.png"))
with tempfile.NamedTemporaryFile(mode="w", dir=destination.parent, delete=False) as f:
    f.write(data)
os.replace(f.name, destination)
PY
    ;;
uninstall)
    rm -f "$startup" "$desktop"
    if [[ $(readlink "$command_path" || :) == "$prefix/start" ]]; then rm -f "$command_path"; fi
    rm -f "$prefix/bin/lumenshot" "$prefix/start" "$prefix/share/applications/lumenshot.desktop" \
        "$prefix/share/icons/hicolor/256x256/apps/lumenshot.png" "$prefix/share/licenses/lumenshot/LUCIDE-LICENSE"
    # Earlier development installs included a private Qt runtime. Keep unrelated files.
    for lib in Qt6Concurrent Qt6Core Qt6DBus Qt6Gui Qt6Widgets Qt6XcbQpa; do rm -f "$prefix/lib/lib$lib.so.6"; done
    for lib in GLX.so.0 OpenGL.so.0 b2.so.1 double-conversion.so.3 md4c.so.0 pcre2-16.so.0 xcb-xinput.so.0; do rm -f "$prefix/lib/lib$lib"; done
    rm -f "$prefix/plugins/platforms/libqxcb.so" "$prefix/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so"
    rm -rf "$prefix/share/ocr"
    rm -f "$prefix"/lib/libonnxruntime.so*
    for dir in bin lib plugins/platforms plugins/platforminputcontexts plugins share/applications \
        share/icons/hicolor/256x256/apps share/icons/hicolor/256x256 share/icons/hicolor share/icons \
        share/licenses/lumenshot share/licenses share; do rmdir "$prefix/$dir" 2>/dev/null || :; done
    rmdir "$prefix" 2>/dev/null || :
    ;;
*) echo "用法：$0 install BUILD/lumenshot | uninstall" >&2; exit 2 ;;
esac
