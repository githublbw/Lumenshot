"""Only run through check-panel-x11.sh on its private, synthetic display."""
import math
import os
import statistics
import subprocess
import sys
import time
from pathlib import Path

import gi
gi.require_version("Gdk", "3.0")
from gi.repository import Gdk

proc = Path("/proc") / str(int(sys.argv[1]))
root = Gdk.get_default_root_window()
subprocess.run(["xsetroot", "-solid", "white"], check=True)


def pixel():
    return Gdk.pixbuf_get_from_window(root, 5, 200, 1, 1).get_pixels()[:3]


def wait_pixel(check):
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        if check(pixel()):
            return
        time.sleep(.002)
    raise RuntimeError("Synthetic screen did not reach expected paint state")


def rss():
    return int((proc / "statm").read_text().split()[1]) * os.sysconf("SC_PAGE_SIZE") / 2**20


def cpu():
    fields = (proc / "stat").read_text().rsplit(")", 1)[1].split()
    return sum(map(int, fields[11:13])) / os.sysconf("SC_CLK_TCK")


if os.environ.get("MEASURE_IDLE") == "1":
    cpu_start, start = cpu(), time.monotonic()
    memory = []
    for _ in range(60):
        time.sleep(1)
        memory.append(rss())
    print(f"idle cpu={(cpu()-cpu_start)*100/(time.monotonic()-start):.3f}% "
          f"rss mean={statistics.mean(memory):.1f} max={max(memory):.1f} MiB", flush=True)

latency, memory = [], []
background = pixel()
assert min(background) > 240
for _ in range(30):
    start = time.monotonic()
    subprocess.run(["xdotool", "key", "--delay", "0", "ctrl+alt+a"], check=True)
    wait_pixel(lambda p: p != background)
    latency.append((time.monotonic()-start)*1000)
    subprocess.run(["xdotool", "key", "Escape"], check=True)
    wait_pixel(lambda p: p == background)
    time.sleep(.04)
    memory.append(round(rss(), 1))
print(f"{root.get_width()}x{root.get_height()} n={len(latency)} "
      f"p50={statistics.median(latency):.1f} p95={sorted(latency)[math.ceil(.95*len(latency))-1]:.1f} "
      f"max={max(latency):.1f} ms; RSS@1,10,20,30={[memory[i] for i in (0,9,19,29)]}")

# Move the right edge over fresh white pixels. Sample the interior just behind
# the edge so we time a visible repaint, not simply receipt of the motion event.
subprocess.run(["xdotool", "key", "--delay", "0", "ctrl+alt+a"], check=True)
wait_pixel(lambda p: p != background)
subprocess.run(["xdotool", "mousemove", "50", "150", "mousedown", "1"], check=True)
paints = []
start = time.monotonic()
for i in range(60):
    time.sleep(max(0, start+i/60-time.monotonic()))
    x = 100+20*i
    subprocess.run(["xdotool", "mousemove", str(x), "320"], check=True)
    deadline = time.monotonic()+2
    while Gdk.pixbuf_get_from_window(root, x-5, 210, 1, 1).get_pixels()[:3] != background:
        if time.monotonic() >= deadline:
            raise RuntimeError("Drag did not repaint")
        time.sleep(.001)
    paints.append(time.monotonic())
subprocess.run(["xdotool", "mouseup", "1", "key", "Escape"], check=True)
intervals = [(b-a)*1000 for a,b in zip(paints,paints[1:])]
print(f"selection drag n={len(intervals)} p95={sorted(intervals)[math.ceil(.95*len(intervals))-1]:.1f} "
      f"max={max(intervals):.1f} ms")
