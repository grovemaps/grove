#!/usr/bin/env python3
"""Checks that every change Grove makes to an upstream file names its module, and lists the hooks by module.

Grove's own code lives in modules/<module>/. An upstream file only gets small hooks, each naming its module:
    // Grove[logos]: chains' logos never hide each other.
(or /* */, <!-- -->, # comments), or naming it by what it does: a path into the module (#include
"modules/logos/...", "see modules/logos/..."), checking its switch (grove::Feature::Logos, the registry says which module) or
Stock (grove::IsStock(), the core module). A changed block passes when it names a module or a line at most
MARKER_REACH lines above or below it does. Generated files (drawing rules, icon atlases, strings...) are left out.

    modules/core/tools/check_hooks.py            # check; exit 1 on unmarked blocks
    modules/core/tools/check_hooks.py --list     # the hooks by module
"""
import collections
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
BASE_FILE = os.path.join(ROOT, "modules", "look", "tools", "classic_base.txt")
MARKER = re.compile(r"Grove\[([a-z0-9_]+)\]")
PATH = re.compile(r"\bmodules/([a-z0-9_]+)/")
FEATURE = re.compile(r"Feature::(\w+)")
STOCK = re.compile(r"\bIsStock\(\)")
MARKER_REACH = 5

# Generated, or not code: checked by their generators and tests instead.
SKIP = re.compile(
    r"^(modules/|3party/|\.gitmodules$|GROVE\.md$|\.github/|data/drules_|data/(colors|patterns)\.txt$|data/symbols|"
    r"data/sound-strings/|data/strings/|data/vulkan_shaders/|data/styles/.*/priorities_.*\.prio\.txt$|"
    r"data/copyright\.html$|android/app/src/main/res/values-[^/]+/strings\.xml$|"
    r"android/.*/res/values/strings\.xml$|iphone/.*\.strings$|android/sdk/src/main/assets/)"
)


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True, text=True).stdout


def feature_modules():
    """{Feature enumerator: module} from the registry."""
    text = open(os.path.join(ROOT, "modules", "core", "platform", "features.cpp"), encoding="utf-8").read()
    return dict(re.findall(r'\{(\w+), G::\w+, "([a-z0-9_]+)"', text))


def named_modules(line, features):
    names = MARKER.findall(line) + PATH.findall(line)
    names += [features[f] for f in FEATURE.findall(line) if f in features]
    if STOCK.search(line):
        names.append("core")
    return names


def modules():
    return {d for d in os.listdir(os.path.join(ROOT, "modules")) if os.path.isdir(os.path.join(ROOT, "modules", d))}


def changed_files(base):
    out = git("diff", "--name-status", "--no-renames", base, "--")
    return [line.split("\t", 1)[1] for line in out.splitlines() if line.startswith("M\t")]


def hunks(base, path):
    """(new start line, new line count, added lines) of each changed block, against the working tree."""
    out = git("diff", "-U0", base, "--", path)
    result = []
    for block in re.split(r"^(?=@@ )", out, flags=re.M)[1:]:
        m = re.match(r"@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@", block)
        start, count = int(m.group(1)), int(m.group(2) if m.group(2) is not None else 1)
        added = [l[1:] for l in block.splitlines()[1:] if l.startswith("+")]
        result.append((start, count, added))
    return result


def main():
    base = open(BASE_FILE).read().strip()
    known = modules()
    features = feature_modules()
    hooks = collections.defaultdict(list)
    unmarked, unknown = [], []
    for path in changed_files(base):
        if SKIP.match(path):
            continue
        lines = open(os.path.join(ROOT, path), encoding="utf-8", errors="replace").read().split("\n")
        for start, count, added in hunks(base, path):
            first = max(start - 1, 0)
            window = lines[max(first - MARKER_REACH, 0) : first + max(count, 1) + MARKER_REACH]
            found = [m for l in window for m in named_modules(l, features)]
            if not found:
                unmarked.append(f"{path}:{start}")
                continue
            for module in dict.fromkeys(found):
                if module not in known:
                    unknown.append(f"{path}:{start} names {module}")
                hooks[module].append(f"{path}:{start}")
    if "--list" in sys.argv:
        for module in sorted(hooks):
            print(module)
            for hook in dict.fromkeys(hooks[module]):
                print("  " + hook)
        return 0
    for problem in unmarked:
        print(f"unmarked change: {problem}")
    for problem in unknown:
        print(f"unknown module: {problem}")
    total = sum(len(h) for h in hooks.values())
    print(f"{total} marked hooks in {len(hooks)} modules, {len(unmarked)} unmarked, {len(unknown)} unknown")
    return 1 if unmarked or unknown else 0


if __name__ == "__main__":
    sys.exit(main())
