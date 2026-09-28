#!/usr/bin/env python3
"""Switches the Android app from Roboto to Inter, the font of the map labels.

    tools/grove/android_fonts.py [--check]

Upstream names its fonts through three strings (values/donottranslate.xml: robotoRegular, robotoMedium,
robotoLight) used in layouts, styles and a few spans. This script points them at the Inter font families in
res/font, and hangs the app themes under the Grove themes of values/grove_fonts.xml, which set Material's text
appearances (buttons, headlines, body text) to Inter. Re-run it after syncing with upstream; it only touches what
still names Roboto; format the Java files it changes with clang-format. --check lists what would change and fails if anything would.

Android 8+ and AppCompat views get Inter; views the compat library doesn't cover on older Android keep Roboto.
"""

import argparse
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
APP = os.path.join(ROOT, "android", "app", "src", "main")

# Light isn't used for anything that needs to be thin; Inter Regular reads better there.
FONTS = {
    "@string/robotoRegular": "@font/inter",
    "@string/robotoMedium": "@font/inter_medium",
    "@string/robotoLight": "@font/inter",
}

# Themes and text appearances whose parent comes from the libraries: they get the Grove twin of that parent.
PARENTS = {
    '"MwmTheme" parent="Theme.MaterialComponents.DayNight.NoActionBar.Bridge"':
        '"MwmTheme" parent="Grove.Theme"',
    '"MwmTheme.AlertDialog" parent="Theme.MaterialComponents.DayNight.Dialog.Alert"':
        '"MwmTheme.AlertDialog" parent="Grove.Theme.AlertDialog"',
    '"MwmTheme.DialogFragment.Rounded" parent="ThemeOverlay.MaterialComponents.Dialog.Alert"':
        '"MwmTheme.DialogFragment.Rounded" parent="Grove.ThemeOverlay.Dialog"',
    '"MwmTheme.DialogFragment.Fullscreen" parent="Theme.AppCompat.DayNight"':
        '"MwmTheme.DialogFragment.Fullscreen" parent="Grove.Theme.AppCompat"',
    '"MwmTextAppearance.Toolbar.Title" parent="@style/TextAppearance.Widget.AppCompat.Toolbar.Title"':
        '"MwmTextAppearance.Toolbar.Title" parent="Grove.TextAppearance.Toolbar.Title"',
}

# Spans that name the font family as a string.
SPAN = re.compile(r"new TypefaceSpan\(context\.getResources\(\)\.getString\(R\.string\.robotoMedium\)\)")
SPAN_WITH = "GroveFonts.mediumSpan(context)"
SPAN_IMPORT = "import app.organicmaps.util.GroveFonts;"


def rewrite_xml(text):
    for old, new in FONTS.items():
        text = text.replace(old, new)
    for old, new in PARENTS.items():
        text = text.replace(old, new)
    return text


def rewrite_java(text):
    if not SPAN.search(text):
        return text
    text = SPAN.sub(SPAN_WITH, text)
    if SPAN_IMPORT not in text:
        # Keeps the imports sorted: after the last one that sorts before it.
        before = [m for m in re.finditer(r"^import [\w.]+;$", text, re.M) if m.group(0) < SPAN_IMPORT]
        at = before[-1].end()
        text = text[:at] + "\n" + SPAN_IMPORT + text[at:]
    if text.count("TypefaceSpan") == 1:
        text = text.replace("import android.text.style.TypefaceSpan;\n", "")
    return text


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    changed = []
    for folder, _, files in os.walk(APP):
        for name in files:
            path = os.path.join(folder, name)
            if name == "donottranslate.xml" or name == "grove_fonts.xml":
                continue
            if name.endswith(".xml") and os.sep + "res" + os.sep in path:
                rewrite = rewrite_xml
            elif name.endswith(".java"):
                rewrite = rewrite_java
            else:
                continue
            text = open(path, encoding="utf-8").read()
            new = rewrite(text)
            if new != text:
                changed.append(os.path.relpath(path, ROOT))
                if not args.check:
                    open(path, "w", encoding="utf-8").write(new)

    for path in changed:
        print(("needs update: " if args.check else "updated: ") + path)
    if args.check and changed:
        sys.exit(1)


if __name__ == "__main__":
    main()
