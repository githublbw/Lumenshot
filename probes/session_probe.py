#!/usr/bin/python3
"""GNOME 42 capability probe. Run with the distribution's Python."""
import argparse
import os
import sys
import uuid

import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gdk, GdkPixbuf, Gio, GLib, Gtk

PORTAL = "org.freedesktop.portal.Desktop"
DESKTOP = "/org/freedesktop/portal/desktop"
REQUEST = "org.freedesktop.portal.Request"


def require_unlocked(bus):
    active, = bus.call_sync(
        "org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", "org.gnome.ScreenSaver",
        "GetActive", None, GLib.VariantType.new("(b)"), Gio.DBusCallFlags.NONE,
        3000, None).unpack()
    if active:
        raise RuntimeError("桌面已锁定，本次测试结束")


def preview(image):
    window = Gtk.Window(title="Lumenshot · 置顶验证")
    scale = min(480 / image.get_width(), 300 / image.get_height(), 1)
    thumbnail = image.scale_simple(max(1, round(image.get_width() * scale)),
                                   max(1, round(image.get_height() * scale)),
                                   GdkPixbuf.InterpType.BILINEAR)
    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
    box.pack_start(Gtk.Image.new_from_pixbuf(thumbnail), True, True, 0)
    box.pack_start(Gtk.Label(label="切换到其他窗口检查置顶效果。需要时右击标题栏，选择“总在最前”。",
                             wrap=True), False, False, 0)
    window.add(box)
    window.connect("destroy", lambda *_: Gtk.main_quit())
    window.set_keep_above(True)
    window.show_all()
    Gtk.main()


def portal_capture(bus):
    token = "lumenshot" + uuid.uuid4().hex
    sender = bus.get_unique_name()[1:].replace(".", "_")
    path = f"{DESKTOP}/request/{sender}/{token}"
    loop = GLib.MainLoop()
    result = []

    def response(conn, name, obj, interface, signal, parameters):
        result.append(parameters.unpack())
        loop.quit()

    subscription = bus.signal_subscribe(PORTAL, REQUEST, "Response", path, None,
                                        Gio.DBusSignalFlags.NONE, response)
    try:
        options = {"handle_token": GLib.Variant("s", token),
                   "interactive": GLib.Variant("b", True)}
        handle, = bus.call_sync(
            PORTAL, DESKTOP, "org.freedesktop.portal.Screenshot", "Screenshot",
            GLib.Variant("(sa{sv})", ("", options)), GLib.VariantType.new("(o)"),
            Gio.DBusCallFlags.NONE, 15000, None).unpack()
        if handle != path:
            raise RuntimeError("门户返回了意外的请求路径")
        if not result:
            loop.run()
    finally:
        bus.signal_unsubscribe(subscription)
    status, fields = result[0]
    if status == 1:
        return None
    if status != 0:
        raise RuntimeError("系统截图失败")
    require_unlocked(bus)
    location = Gio.File.new_for_uri(fields.get("uri", ""))
    if not location.is_native():
        raise RuntimeError("门户未返回本机图像")
    stream = location.read(None)
    try:
        return GdkPixbuf.Pixbuf.new_from_stream(stream, None)
    finally:
        stream.close(None)


def desktop_image():
    root = Gdk.get_default_root_window()
    image = Gdk.pixbuf_get_from_window(root, 0, 0, root.get_width(), root.get_height())
    if image is None:
        raise RuntimeError("未能读取桌面图像")
    return image


def wait_for_shortcut():
    from itertools import combinations
    from Xlib import X, XK, display, error

    connection = display.Display()
    try:
        root = connection.screen().root
        key = connection.keysym_to_keycode(XK.string_to_keysym("a"))
        num = connection.keysym_to_keycode(XK.string_to_keysym("Num_Lock"))
        locks = {X.LockMask}
        locks.update(1 << i for i, keys in enumerate(connection.get_modifier_mapping())
                     if num and num in keys)
        errors = []
        def on_error(problem, request):
            errors.append(problem)
            return True
        for count in range(len(locks) + 1):
            for subset in combinations(locks, count):
                root.grab_key(key, X.ControlMask | X.Mod1Mask | sum(subset), False,
                              X.GrabModeAsync, X.GrabModeAsync, onerror=on_error)
        connection.sync()
        if errors:
            if isinstance(errors[0], error.BadAccess):
                raise RuntimeError("Ctrl+Alt+A 已被其他程序占用")
            raise RuntimeError("注册快捷键失败")
        print("已注册 Ctrl+Alt+A，等待一次触发", flush=True)
        while True:
            event = connection.next_event()
            if event.type == X.KeyPress and event.detail == key:
                return
    finally:
        connection.close()


def main():
    parser = argparse.ArgumentParser(description="Lumenshot 阶段 0 桌面验证")
    parser.add_argument("--listen", action="store_true", help="X11：等待一次全局快捷键")
    parser.add_argument("--pin", action="store_true", help="打开带系统标题栏的置顶测试窗口")
    args = parser.parse_args()
    if not Gtk.init_check()[0]:
        raise RuntimeError("请在已登录的图形桌面运行")
    session = os.environ.get("XDG_SESSION_TYPE")
    backend = Gdk.Display.get_default().__gtype__.name
    bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    require_unlocked(bus)
    if session == "wayland" and backend == "GdkWaylandDisplay":
        if args.listen:
            raise RuntimeError("请通过 GNOME 自定义快捷键运行本程序，省略 --listen")
        image = portal_capture(bus)
    elif session == "x11" and backend == "GdkX11Display":
        if args.listen:
            wait_for_shortcut()
        require_unlocked(bus)
        image = desktop_image()
    else:
        raise RuntimeError("请使用与登录会话一致的原生 X11 或 Wayland 后端")
    if image is None:
        print("系统截图已取消")
        return
    print(f"已读取图像 {image.get_width()} × {image.get_height()}", flush=True)
    if args.pin:
        preview(image)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, GLib.Error, ImportError) as exc:
        message = "桌面服务或图像读取失败" if isinstance(exc, GLib.Error) else str(exc)
        sys.exit(message)
