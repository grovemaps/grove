#!/bin/bash
# Render map screenshots with the desktop app, to check Grove's style without a phone.
#
#   tools/grove/render_screens.sh [points file] [output dir]
#
# Points are "lat,lon,zoom" per line (default: tools/grove/amsterdam-points.txt, the spots of the
# Apple Maps reference screenshots). Output covers a 1000x628 pt Mac window: 1000x628 px at 1x by default,
# or 2000x1256 px with GROVE_SCALE=2 (4x slower: this Mac renders in software).
# Needed region maps are downloaded on first use into $GROVE_MAPS (default /Volumes/grove/desktop-data).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
POINTS="${1:-$ROOT/tools/grove/amsterdam-points.txt}"
OUT="${2:-$ROOT/build-grove/screens}"
MAPS="${GROVE_MAPS:-/Volumes/grove/desktop-data}"
BUILD="${GROVE_BUILD:-$ROOT/build-grove}"
APP="$BUILD/OrganicMaps.app/Contents/MacOS/OrganicMaps"
SCALE="${GROVE_SCALE:-1}"
RUN=()
if [ "$(uname)" = Linux ]; then
  APP="$BUILD/OrganicMaps"
  # Headless: a virtual X display with Mesa's software OpenGL (Qt's offscreen platform has no GL context).
  if [ -z "${DISPLAY:-}" ]; then
    RUN=(xvfb-run -a -s "-screen 0 $((1000 * SCALE + 200))x$((628 * SCALE + 200))x24")
  fi
fi

if [ ! -x "$APP" ]; then
  cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF > /dev/null
fi
cmake --build "$BUILD" --target desktop > /dev/null

mkdir -p "$OUT" "$MAPS"
rm -f "$OUT"/point_*.png
"${RUN[@]}" "$APP" --resources_path="$ROOT/data/" --data_path="$MAPS/" --points="$POINTS" --dst_path="$OUT/" \
  --width=$((1000 * SCALE)) --height=$((628 * SCALE)) --dpi_scale="$SCALE" > "$OUT/render.log" 2>&1
ls "$OUT"/point_*.png
