#!/usr/bin/env python3
"""Switches the Android app from Roboto to Geist, and to Inter for languages whose letters Geist lacks.

    tools/grove/android_fonts.py [--check]

The app's text uses the font families @font/ui and @font/ui_medium. In res/font they are Geist, which sets the
interface apart from the map labels (Inter). For each translation (values-*/strings.xml) that uses letters Geist
doesn't have but Inter does (Greek, for Geist 1.8), the script writes res/font-<language>/ with the same families
in Inter, so words don't switch fonts midway. Scripts neither font covers (Arabic, CJK, Hindi...) use the system
fonts either way.

Upstream names its fonts through three strings (values/donottranslate.xml: robotoRegular, robotoMedium,
robotoLight) used in layouts, styles and a few spans. The script points them at the ui families, and hangs the app
themes under the Grove themes of values/grove_fonts.xml, which set Material's text appearances (buttons, headlines,
body text) to them. Re-run it after syncing with upstream and after new translations, then format the Java files it
changed with clang-format. --check lists what would change and fails if anything would.

Android 8+ and AppCompat views get the new fonts; views the compat library doesn't cover on older Android keep
Roboto. Needs fontTools (pip install fonttools).
"""

import argparse
import glob
import html
import os
import re
import sys
import unicodedata

from fontTools.ttLib import TTFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
APP = os.path.join(ROOT, "android", "app", "src", "main")
RES = os.path.join(APP, "res")

# Light isn't used for anything that needs to be thin; the regular weight reads better there. The @font/inter
# names come from before the families were renamed.
FONTS = [
    (re.compile(r"@string/robotoRegular\b|@string/robotoLight\b|@font/inter\b"), "@font/ui"),
    (re.compile(r"@string/robotoMedium\b|@font/inter_medium\b"), "@font/ui_medium"),
]

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

# The font files of each family, by weight: text weight 400 and bold 700. Medium's bold is Bold, regular's SemiBold,
# which keeps bold text from getting heavy.
FAMILIES = {
    "geist": {"ui": ("geist_regular", "geist_semibold"), "ui_medium": ("geist_medium", "geist_bold")},
    "inter": {"ui": ("inter_regular", "inter_semibold"), "ui_medium": ("inter_medium_face", "inter_bold")},
}
FAMILY_XML = """<?xml version="1.0" encoding="utf-8"?>
<!-- Grove: written by tools/grove/android_fonts.py. -->
<font-family xmlns:android="http://schemas.android.com/apk/res/android">
  <font android:font="@font/{0}" android:fontStyle="normal" android:fontWeight="400" />
  <font android:font="@font/{1}" android:fontStyle="normal" android:fontWeight="700" />
</font-family>
"""


def rewrite_xml(text):
    for pattern, new in FONTS:
        text = pattern.sub(new, text)
    for old, new in PARENTS.items():
        text = text.replace(old, new)
    return text


def rewrite_java(text):
    text = text.replace("R.font.inter_medium", "R.font.ui_medium")
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


def letters(strings_xml):
    text = html.unescape(re.sub(r"<[^>]+>", " ", open(strings_xml, encoding="utf-8").read()))
    return {ord(c) for c in text if unicodedata.category(c)[0] in "LM" and ord(c) > 0x7F}


def family_files():
    """The wanted res/font*/ui*.xml contents: Geist by default, Inter for languages only Inter covers."""
    cmap = {name: set(TTFont(os.path.join(RES, "font", files["ui"][0] + ".ttf")).getBestCmap())
            for name, files in FAMILIES.items()}
    wanted = {"font": "geist"}
    for strings in sorted(glob.glob(os.path.join(RES, "values-*", "strings.xml"))):
        used = letters(strings)
        if not used <= cmap["geist"] and used <= cmap["inter"]:
            wanted["font-" + os.path.basename(os.path.dirname(strings))[len("values-"):]] = "inter"

    files = {}
    for folder, family in wanted.items():
        for name, faces in FAMILIES[family].items():
            files[os.path.join(RES, folder, name + ".xml")] = FAMILY_XML.format(*faces)
    # Folders of languages that no longer need Inter.
    for old in glob.glob(os.path.join(RES, "font-*", "ui*.xml")):
        files.setdefault(old, None)
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    changed = []
    for folder, _, names in os.walk(APP):
        for name in names:
            path = os.path.join(folder, name)
            if name == "donottranslate.xml":
                continue
            if name.endswith(".xml") and os.sep + "res" + os.sep in path and os.sep + "font" not in path:
                rewrite = rewrite_xml
            elif name.endswith(".java"):
                rewrite = rewrite_java
            else:
                continue
            text = open(path, encoding="utf-8").read()
            new = rewrite(text)
            if new != text:
                changed.append(path)
                if not args.check:
                    open(path, "w", encoding="utf-8").write(new)

    for path, content in family_files().items():
        current = open(path, encoding="utf-8").read() if os.path.exists(path) else None
        if current == content:
            continue
        changed.append(path)
        if args.check:
            continue
        if content is None:
            os.remove(path)
            if not os.listdir(os.path.dirname(path)):
                os.rmdir(os.path.dirname(path))
        else:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "w", encoding="utf-8").write(content)

    for path in changed:
        print(("needs update: " if args.check else "updated: ") + os.path.relpath(path, ROOT))
    if args.check and changed:
        sys.exit(1)


if __name__ == "__main__":
    main()
