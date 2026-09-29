#!/bin/bash
# Build the drawing rules users can switch to (libs/indexer/map_style_reader.cpp):
# - the "Organic Maps" look (settings key "GroveLook"): Organic Maps' own styles and icons, untouched by Grove;
# - car navigation in Grove's colors (settings key "GroveNavigationColors"): data/styles/vehicle/grove-{light,dark}
#   into data/drules_vehicle_grove.bin. Navigation keeps Organic Maps' muted vehicle style by default.
#
#   tools/grove/extra_styles.sh     (tools/grove/generate_drules.sh runs it)
#
# Takes data/styles and data/symbols from the Organic Maps commit in tools/grove/classic_base.txt (the one Grove is
# based on; tools/grove/sync_upstream.sh updates it), compiles the styles against Grove's current data (the type
# mapping is upstream's) into data/drules_{default,outdoors,vehicle}_classic.bin, and copies the icon atlases to
# data/symbols-classic (Android densities only). Their colors and dash patterns are added to data/colors.txt and
# data/patterns.txt, which the engine loads for every style.
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1

cd "$(git rev-parse --show-toplevel)"
BASE="$(cat tools/grove/classic_base.txt)"
KOTHIC=tools/kothic/src
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

git archive "$BASE" data/styles data/symbols | tar -x -C "$TMP"
WORK="$TMP/work"
mkdir -p "$WORK"
cp data/mapcss-mapping.csv data/mapcss-dynamic.txt data/colors.txt data/patterns.txt "$WORK/"

for family in default outdoors vehicle; do
  for variant in light dark; do
    echo "Building the Organic Maps look: $family/$variant"
    python3 "$KOTHIC/libkomwm.py" -s "$TMP/data/styles/$family/$variant/style.mapcss" -o "$TMP/${family}_$variant" \
      -p "$TMP/data/styles/$family/include/" -d "$WORK" > /dev/null
  done
  python3 "$KOTHIC/merge_variants.py" "data/drules_${family}_classic" \
    light "$TMP/${family}_light.bin" dark "$TMP/${family}_dark.bin" > /dev/null
  rm -f "data/drules_${family}_classic.txt"
done
echo "Building car navigation in Grove's colors"
for variant in light dark; do
  python3 "$KOTHIC/libkomwm.py" -s "data/styles/vehicle/grove-$variant/style.mapcss" -o "$TMP/vehicle_grove_$variant" \
    -p data/styles/vehicle/include/ -d "$WORK" > /dev/null
done
python3 "$KOTHIC/merge_variants.py" data/drules_vehicle_grove \
  light "$TMP/vehicle_grove_light.bin" dark "$TMP/vehicle_grove_dark.bin" > /dev/null
rm -f data/drules_vehicle_grove.txt

# All of them draw the same types: the classificator must come out as Grove's.
cmp -s "$WORK/classificator.txt" data/classificator.txt || { echo "classificator differs from Grove's" >&2; exit 1; }
# Their colors and patterns, appended to Grove's.
cp "$WORK/colors.txt" "$WORK/patterns.txt" data/

rm -rf data/symbols-classic
for density in mdpi hdpi xhdpi xxhdpi xxxhdpi; do
  for variant in light dark; do
    mkdir -p "data/symbols-classic/$density/$variant"
    cp "$TMP/data/symbols/$density/$variant/symbols.png" "$TMP/data/symbols/$density/$variant/symbols.xml" \
      "data/symbols-classic/$density/$variant/"
  done
done
