#!/usr/bin/env python3
"""Renders a map view with the Vulkan (or OpenGL) backend, headless, to check Android's default graphics path.

    tools/grove/render_vulkan.py LAT LON ZOOM OUT.png [--api Vulkan|OpenGL] [--wait 90]

Runs dev_sandbox (build target dev_sandbox) on a virtual X display with Mesa's software drivers (lavapipe for
Vulkan: packages mesa-vulkan-drivers, xvfb, imagemagick) and captures the screen. dev_sandbox opens at the last
saved viewport, so the script writes it into the settings first. Maps come from $GROVE_MAPS (default
build-grove/desktop-data); download them once with tools/grove/render_screens.sh. The imgui control panel shows
in the top left corner.
"""

import argparse
import math
import os
import re
import subprocess
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SETTINGS = os.path.expanduser("~/.config/OrganicMaps/settings.ini")
WIDTH, HEIGHT = 1600, 1000
DISPLAY = ":57"


def viewport(lat, lon, zoom):
    """The "ScreenClipRect" setting: origin, angle, then the rect relative to the origin, in mercator units."""
    x = lon
    y = math.degrees(math.log(math.tan(math.pi / 4 + math.radians(lat) / 2)))
    # At zoom z a 256 px tile spans 360 / 2^z mercator units; dev_sandbox draws at scale 1.
    half_w = 360 / 2 ** zoom * WIDTH / 256 / 2
    half_h = half_w * HEIGHT / WIDTH
    return f"{x - half_w:.9f} {y - half_h:.9f} 0 0 0 {2 * half_w:.9f} {2 * half_h:.9f}"


def set_setting(key, value):
    lines = open(SETTINGS).read().splitlines() if os.path.exists(SETTINGS) else []
    lines = [l for l in lines if not l.startswith(key + "=")] + [f"{key}={value}"]
    os.makedirs(os.path.dirname(SETTINGS), exist_ok=True)
    open(SETTINGS, "w").write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("lat", type=float)
    parser.add_argument("lon", type=float)
    parser.add_argument("zoom", type=float)
    parser.add_argument("out")
    parser.add_argument("--api", default="Vulkan", choices=["Vulkan", "OpenGL"])
    parser.add_argument("--wait", type=int, default=90, help="seconds to let tiles load and render")
    args = parser.parse_args()

    set_setting("ScreenClipRect", viewport(args.lat, args.lon, args.zoom))
    set_setting("EulaAccepted", "true")

    app = os.path.join(os.environ.get("GROVE_BUILD", os.path.join(ROOT, "build-grove")), "OMapsDevSandbox")
    maps = os.environ.get("GROVE_MAPS", os.path.join(ROOT, "build-grove", "desktop-data"))
    env = dict(os.environ, DISPLAY=DISPLAY)
    if args.api == "OpenGL":
        env["GROVE_DEV_SANDBOX_API"] = "OpenGL"

    xvfb = subprocess.Popen(["Xvfb", DISPLAY, "-screen", "0", f"{WIDTH}x{HEIGHT}x24", "-nolisten", "tcp"])
    log_path = os.path.splitext(args.out)[0] + ".log"
    try:
        time.sleep(2)
        with open(log_path, "w") as log:
            sandbox = subprocess.Popen([app, f"--resources_path={ROOT}/data/", f"--data_path={maps}/"],
                                       env=env, stdout=log, stderr=subprocess.STDOUT)
            time.sleep(args.wait)
            subprocess.run(["import", "-display", DISPLAY, "-window", "root", args.out], check=True)
            sandbox.terminate()
            try:
                sandbox.wait(10)
            except subprocess.TimeoutExpired:
                sandbox.kill()
    finally:
        xvfb.terminate()

    log = open(log_path, errors="replace").read()
    errors = re.findall(r"^E\(.*$", log, re.M)
    api = re.search(r"Api version = (\w+)|ApiVersion[^\n]*", log)
    print(f"{args.out}: {len(errors)} logged errors" + (f"; {api.group(0)}" if api else ""))
    for e in errors[:10]:
        print("  " + e[:200])


if __name__ == "__main__":
    main()
