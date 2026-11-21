#!/usr/bin/env bash
# 自包含 .deb 打包：/opt/lumenshot 私有 Qt 运行库 + 主程序 + OCR 模型 + 系统入口。
# 用法：bash packaging/build-deb.sh [二进制路径]
#   VERSION=2026.11.21 bash packaging/build-deb.sh   自定义版本号
#   RUNTIME_DIR=/path/to/runtime                      指定私有运行库来源
#   （默认取 ~/.local/opt/lumenshot，需先执行过 packaging/user-install.sh install）
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
binary="${1:-$HOME/.cache/lumenshot-build/build/lumenshot}"
version="${VERSION:-$(date +%Y.%m.%d)}"
runtime="${RUNTIME_DIR:-$HOME/.local/opt/lumenshot}"

test -x "$binary" || { echo "缺少主程序：$binary" >&2; exit 1; }
[[ -d "$runtime/lib" && -d "$runtime/plugins" ]] || {
    echo "缺少私有 Qt 运行库（$runtime）。" >&2
    echo "请先执行 bash packaging/user-install.sh install，再用 RUNTIME_DIR=路径 指定。" >&2
    exit 1
}

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT

# /opt/lumenshot：应用 + 私有运行库 + OCR 模型
opt="$stage/opt/lumenshot"
install -Dm755 "$binary" "$opt/bin/lumenshot"
install -m755 "$root/packaging/start" "$opt/start"
mkdir -p "$opt/lib" "$opt/plugins" "$opt/share/ocr" "$opt/share/licenses/lumenshot"
cp -a "$runtime/lib/." "$opt/lib/"
cp -a "$runtime/plugins/." "$opt/plugins/"
# 图片格式插件（jpeg/ico/gif，粘贴与拖拽需要）
img="$HOME/.cache/lsbundle/root/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats"
[[ -d "$img" ]] && cp -a "$img" "$opt/plugins/" || true
if compgen -G "$root/assets/ocr/*.onnx" >/dev/null; then
    install -m644 "$root"/assets/ocr/*.onnx "$opt/share/ocr/"
fi
install -m644 "$root/assets/LUCIDE-LICENSE" "$opt/share/licenses/lumenshot/LUCIDE-LICENSE"

# 系统入口：命令行、菜单、图标
mkdir -p "$stage/usr/bin"
ln -s /opt/lumenshot/start "$stage/usr/bin/lumenshot"
install -Dm644 "$root/packaging/lumenshot.desktop" "$stage/usr/share/applications/lumenshot.desktop"
install -Dm644 "$root/assets/lumenshot-icon.png" "$stage/usr/share/icons/hicolor/256x256/apps/lumenshot.png"

# DEBIAN 控制信息与维护脚本
mkdir -p "$stage/DEBIAN"
installed_size=$(du -sk "$stage" | cut -f1)
cat > "$stage/DEBIAN/control" <<CTL
Package: lumenshot
Version: $version
Section: utils
Priority: optional
Architecture: amd64
Maintainer: Lumenshot <lumenshot@localhost>
Depends: libx11-6, libxcb1, libicu70, libfontconfig1, libfreetype6, libglib2.0-0, libpng16-16, libbrotli1
Installed-Size: $installed_size
Description: 离线截图工作台（区域/窗口/整屏、标注、OCR、贴图）
 基于 Qt6 的 X11 截图工具：全局热键 F1/F2/F3、原地浮动卡片标注、
 画笔与马赛克笔刷、对话气泡、离线 OCR 文字识别、置顶贴图。
 .
 自带私有 Qt 运行库与 OCR 模型，识别与保存均在本机完成，不联网。
 仅支持 X11 会话，Wayland 未验证。
CTL
cat > "$stage/DEBIAN/postinst" <<'POST'
#!/bin/sh
set -e
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications || true
fi
# 从旧的用户级安装迁移：移除会覆盖系统入口的旧菜单项，登录自启改指新安装。
if [ -n "${SUDO_USER:-}" ]; then
    user_home=$(getent passwd "$SUDO_USER" | cut -d: -f6)
    if [ -n "$user_home" ]; then
        old_menu="$user_home/.local/share/applications/lumenshot.desktop"
        if [ -f "$old_menu" ]; then
            rm -f "$old_menu"
            echo "已移除旧的用户级菜单入口。"
        fi
        autostart="$user_home/.config/autostart/lumenshot.desktop"
        if [ -f "$autostart" ] && grep -q "$user_home/.local/opt/lumenshot/start" "$autostart"; then
            sed -i "s|Exec=.*|Exec=/usr/bin/lumenshot --background|" "$autostart"
            echo "已将登录自启切换到新安装（/usr/bin/lumenshot）。"
        fi
    fi
fi
exit 0
POST
chmod 755 "$stage/DEBIAN/postinst"

dpkg-deb --build --root-owner-group "$stage" "$root/packaging/lumenshot_${version}_amd64.deb"
echo "输出：$root/packaging/lumenshot_${version}_amd64.deb"
