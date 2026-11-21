#!/usr/bin/python3
"""Exercise the probe against the current, unlocked GNOME X11 desktop."""
import os
import select
import subprocess
import sys
import time
from pathlib import Path

from Xlib import X, XK, display
from Xlib.ext import xtest


def wait_until(check):
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        result = check()
        if result:
            return result
        time.sleep(0.05)
    raise RuntimeError("窗口管理器未在五秒内进入预期状态")


def check():
    if os.environ.get("XDG_SESSION_TYPE") != "x11":
        raise RuntimeError("本测试需要 X11 登录会话")
    command = [sys.executable, str(Path(__file__).with_name("session_probe.py")), "--listen"]
    connection = display.Display()
    root = connection.screen().root

    def values(window, name):
        prop = window.get_full_property(connection.intern_atom(name), X.AnyPropertyType)
        return list(prop.value) if prop else []

    with subprocess.Popen(command + ["--pin"], stdout=subprocess.PIPE, text=True) as probe:
        try:
            if not select.select([probe.stdout], [], [], 5)[0] or "已注册" not in probe.stdout.readline():
                raise RuntimeError("快捷键未就绪")
            duplicate = subprocess.run(command, capture_output=True, text=True, timeout=3)
            if duplicate.returncode == 0 or "已被其他程序占用" not in duplicate.stderr:
                raise RuntimeError("快捷键冲突测试失败")
            print("快捷键冲突检查通过")
            keys = [connection.keysym_to_keycode(XK.string_to_keysym(k))
                    for k in ("Control_L", "Alt_L", "a")]
            for key in keys:
                xtest.fake_input(connection, X.KeyPress, key)
            for key in reversed(keys):
                xtest.fake_input(connection, X.KeyRelease, key)
            connection.sync()

            def find_preview():
                for wid in values(root, "_NET_CLIENT_LIST"):
                    window = connection.create_resource_object("window", wid)
                    if values(window, "_NET_WM_PID") == [probe.pid]:
                        return window
                return None

            preview = wait_until(find_preview)
            above = connection.intern_atom("_NET_WM_STATE_ABOVE")
            wait_until(lambda: above in values(preview, "_NET_WM_STATE"))
            print("模拟全局按键已触发截图和置顶预览")
            for _ in range(2):
                normal = root.create_window(50, 50, 320, 200, 0, X.CopyFromParent,
                                            X.InputOutput, X.CopyFromParent)
                normal.set_wm_name("Lumenshot focus check")
                normal.map()
                connection.sync()
                wait_until(lambda: normal.id in values(root, "_NET_CLIENT_LIST"))
                normal.set_input_focus(X.RevertToParent, X.CurrentTime)
                connection.sync()
                def above_normal():
                    stack = values(root, "_NET_CLIENT_LIST_STACKING")
                    return (connection.get_input_focus().focus == normal and
                            normal.id in stack and preview.id in stack and
                            stack.index(preview.id) > stack.index(normal.id))
                wait_until(above_normal)
            print("两个普通窗口获得焦点后，预览均保持在其上方")
        finally:
            probe.terminate()
            connection.close()


if __name__ == "__main__":
    check()
