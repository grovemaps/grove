#!/usr/bin/env python3
"""Build the icon atlases (data/symbols/<res>/{light,dark}/symbols.*) with Grove's category colors.

Use this instead of tools/unix/generate_symbols.sh. Upstream SVGs in
data/styles/default/{light,dark}/symbols are never edited: each run copies them to a temp dir,
recolors the copies and runs skin_generator_tool on them.

An icon gets the color of the label category (data/styles/grove/icon-colors.txt) shared by the place
types that use it. Categories are read back from the compiled data/drules_default.txt, so run
tools/grove/generate_drules.sh first. Icons used by types of several categories, or whose SVG has more
than one non-white/black color, keep their upstream colors and are listed with --verbose.

  tools/grove/generate_symbols.py            # recolor and build all atlases
  tools/grove/generate_symbols.py --dry-run  # only report which icons would change
"""

import argparse
import collections
import fnmatch
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DATA = os.path.join(ROOT, 'data')
STYLES = os.path.join(DATA, 'styles')
# Same resolutions and sizes as tools/unix/generate_symbols.sh.
RESOLUTIONS = [('mdpi', 18), ('hdpi', 27), ('xhdpi', 36), ('6plus', 43), ('xxhdpi', 54), ('xxxhdpi', 64)]
THEMES = ['light', 'dark']

# 6 units wide (a medium icon is 18): about 10 px at 2x, like Apple Maps' dots, with a thin ring.
DOT_SVG = """<svg version="1.1" viewBox="0 0 10 10" width="6" height="6" xmlns="http://www.w3.org/2000/svg">
 <circle cx="5" cy="5" r="5" fill="{ring}" opacity=".9"/>
 <circle cx="5" cy="5" r="4" fill="{fill}"/>
</svg>
"""

# Colors inside fill/stroke/stop-color attributes and style declarations.
PAINT = re.compile(r'((?:fill|stroke|stop-color)\s*[:=]\s*"?\s*)(#(?:[0-9A-Fa-f]{6}|[0-9A-Fa-f]{3}))\b')


def norm(color):
    c = color.lstrip('#').upper()
    return '#' + (''.join(ch * 2 for ch in c) if len(c) == 3 else c)


def read_icon_colors():
    """Returns ({color key: {theme: color}}, [(icon glob, color key, recolor all)]) from icon-colors.txt."""
    colors, rules = {}, []
    with open(os.path.join(STYLES, 'grove', 'icon-colors.txt')) as f:
        for line in f:
            m = re.match(r'^(\w+)\s+(#[0-9A-Fa-f]{6})\s+(#[0-9A-Fa-f]{6})', line)
            if m:
                colors[m.group(1)] = {'light': m.group(2).upper(), 'dark': m.group(3).upper()}
            m = re.match(r'^icon\s+(\S+)\s+(\w+)(\s+all)?\b', line)
            if m:
                rules.append((m.group(1), m.group(2), bool(m.group(3))))
    for _, key, _ in rules:
        assert key in colors, f'icon-colors.txt: unknown color key {key}'
    return colors, rules


def read_label_variables():
    """Effective light-theme label colors: upstream colors.mapcss overridden by Grove's palette."""
    values = {}
    for path in ('default/light/colors.mapcss', 'grove/palette-light.mapcss'):
        with open(os.path.join(STYLES, path)) as f:
            for name, value in re.findall(r'^@(\w+_label)\s*:\s*(#[0-9A-Fa-f]{6})\s*;', f.read(), re.M):
                values[name] = value.upper()
    return values


def icon_categories(categories):
    """Maps icon name -> category, from the place types that use it in the compiled default style."""
    with open(os.path.join(DATA, 'drules_default.txt')) as f:
        text = f.read()
    # Color table: "  c123 #AARRGGBB #AARRGGBB" (light, dark); AA is transparency.
    table = {m.group(1): '#' + m.group(2)[2:].upper() for m in re.finditer(r'^  (c\d+) #(\w{8}) #\w{8}', text, re.M)}
    color_to_category = {}
    for name, value in read_label_variables().items():
        if name in categories:
            color_to_category.setdefault(value, name)
    votes = collections.defaultdict(collections.Counter)
    for block in re.split(r'^type ', text, flags=re.M)[1:]:
        symbols = set(re.findall(r'symbol name=([\w.-]+)', block))
        captions = re.findall(r'caption primary\[[^\]]*?\bcolor=(c\d+)', block)
        if not symbols or not captions:
            continue
        category = color_to_category.get(table[captions[-1]])  # highest zoom wins
        for s in symbols:
            votes[s][category] += 1
    # The leading category wins if it beats every other category and at least matches the uses with
    # an uncategorized (gray) label, e.g. shop-m: 33 shops vs 3 workshops. Ties keep upstream colors.
    result, ambiguous = {}, {}
    for icon, counter in votes.items():
        named = collections.Counter({c: n for c, n in counter.items() if c}).most_common()
        if not named:
            continue
        top, n = named[0]
        if (len(named) == 1 or n > named[1][1]) and n >= counter[None]:
            result[icon] = top
            if len(counter) > 1:
                ambiguous[icon] = (top, dict(counter))
        else:
            ambiguous[icon] = (None, dict(counter))
    return result, ambiguous


def is_neutral(color):
    """White, black and near-black/white grays: the ring and glyph colors, never recolored."""
    rgb = [int(color[i:i + 2], 16) for i in (1, 3, 5)]
    return max(rgb) - min(rgb) <= 0x14 and (max(rgb) <= 0x33 or min(rgb) >= 0xEE)


def recolor(svg, target, recolor_all=False):
    """Replaces the single category color of an icon, or every non-neutral color with recolor_all.
    Returns None when the icon has several colors and recolor_all isn't set."""
    found = {c for c in (norm(c) for _, c in PAINT.findall(svg)) if not is_neutral(c)}
    if len(found) != 1 and not recolor_all:
        return None
    return PAINT.sub(lambda m: m.group(1) + (target if norm(m.group(2)) in found else m.group(2)), svg)


def prepare_symbols(theme, icons, colors, recolor_all, tmp, verbose):
    src = os.path.join(STYLES, 'default', theme, 'symbols')
    dst = os.path.join(tmp, theme, 'symbols')
    shutil.copytree(src, dst, symlinks=True)
    changed, skipped = 0, []
    for icon, category in sorted(icons.items()):
        path = os.path.join(dst, icon + '.svg')
        if not os.path.exists(path):
            continue
        with open(path) as f:
            svg = f.read()
        out = recolor(svg, colors[category][theme], icon in recolor_all)
        if out is None:
            skipped.append(icon)
        elif out != svg:
            with open(path, 'w') as f:
                f.write(out)
            changed += 1
    # Place dots (see libs/drape_frontend/grove_poi_dot.hpp): a small circle of the icon's color with a soft ring,
    # shown where a displaced icon would otherwise leave nothing.
    ring = '#fff' if theme == 'light' else '#000'
    for icon, category in icons.items():
        with open(os.path.join(dst, icon + '-dot.svg'), 'w') as f:
            f.write(DOT_SVG.format(ring=ring, fill=colors[category][theme]))
    print(f'{theme}: recolored {changed} icons, kept {len(skipped)} multi-color icons as upstream, '
          f'added {len(icons)} dots')
    if verbose and skipped:
        print('  multi-color: ' + ' '.join(skipped))
    return dst


def build(skin_generator, symbols_dir, theme):
    for res, size in RESOLUTIONS:
        # skin_generator reads raster symbols from <symbolsDir>/png, as generate_symbols.sh sets up.
        png = os.path.join(symbols_dir, 'png')
        if os.path.lexists(png):
            os.remove(png)
        os.symlink(os.path.join(STYLES, 'default', theme, res), png)
        out_dir = os.path.join(DATA, 'symbols', res, theme)
        for f in os.listdir(out_dir):
            if f.startswith('symbols.'):
                os.remove(os.path.join(out_dir, f))
        r = subprocess.run([skin_generator, '--symbolWidth', str(size), '--symbolHeight', str(size),
                            '--symbolsDir', symbols_dir, '--skinName', os.path.join(out_dir, 'basic'), '--skinSuffix='],
                           capture_output=True, text=True)
        if r.returncode != 0:
            sys.exit(f'skin_generator_tool failed for {res}/{theme}:\n{r.stdout}{r.stderr}')
        # Lossless recompression, as upstream does with optipng; oxipng is a drop-in alternative.
        png_out = os.path.join(out_dir, 'symbols.png')
        if shutil.which('optipng'):
            subprocess.run(['optipng', '-quiet', '-zc9', '-zm8', '-zs0', '-f0', png_out], check=True)
        elif shutil.which('oxipng'):
            subprocess.run(['oxipng', '--quiet', '-o', '4', '--strip', 'safe', png_out], check=True)
        else:
            print('  warning: neither optipng nor oxipng found, atlases stay uncompressed')
        # Unlike generate_symbols.sh, no copy goes to data/symbols/<res>/design (gitignored, only used by the
        # desktop style designer): local Android builds would pack it into the APK.
        print(f'  built {res}/{theme}')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--skin-generator', default=os.environ.get('SKIN_GENERATOR'),
                        help='path to skin_generator_tool (default: build it in build-grove/)')
    parser.add_argument('--dry-run', action='store_true', help='only report which icons would be recolored')
    parser.add_argument('--verbose', action='store_true', help='list ambiguous and multi-color icons')
    args = parser.parse_args()

    colors, rules = read_icon_colors()
    icons, ambiguous = icon_categories(colors)
    all_icons = [f[:-4] for f in os.listdir(os.path.join(STYLES, 'default', 'light', 'symbols')) if f.endswith('.svg')]
    recolor_all = set()
    for pattern, key, everything in rules:
        for icon in fnmatch.filter(all_icons, pattern):
            icons[icon] = key
            if everything:
                recolor_all.add(icon)
    per_category = collections.Counter(icons.values())
    print(f'{len(icons)} icons get a Grove color: ' + ', '.join(f'{c} {n}' for c, n in per_category.most_common()))
    kept = [i for i, (winner, _) in ambiguous.items() if winner is None and i not in icons]
    print(f'{len(kept)} icons are shared by several categories and keep their upstream color')
    if args.verbose:
        for icon, (winner, votes) in sorted(ambiguous.items()):
            print(f'  {icon}: {votes} -> {icons.get(icon, "upstream color")}')

    skin_generator = args.skin_generator
    if not args.dry_run and not skin_generator:
        build_dir = os.path.join(ROOT, 'build-grove')
        subprocess.run(['cmake', '-S', ROOT, '-B', build_dir, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                        '-DBUILD_TESTING=OFF'], check=True, stdout=subprocess.DEVNULL)
        subprocess.run(['cmake', '--build', build_dir, '--target', 'skin_generator_tool'], check=True,
                       stdout=subprocess.DEVNULL)
        skin_generator = os.path.join(build_dir, 'skin_generator_tool')

    with tempfile.TemporaryDirectory() as tmp:
        for theme in THEMES:
            symbols_dir = prepare_symbols(theme, icons, colors, recolor_all, tmp, args.verbose)
            if not args.dry_run:
                build(skin_generator, symbols_dir, theme)


if __name__ == '__main__':
    sys.exit(main())
