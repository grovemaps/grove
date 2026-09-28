# Grove

Working name for a map app that combines three sources:

| Source | Role | Where it lives | License |
|---|---|---|---|
| [Organic Maps](https://github.com/organicmaps/organicmaps) | Code base (`main` branch) | remote `organicmaps` | Apache-2.0 |
| [CoMaps](https://codeberg.org/comaps/comaps) | Features to port: trees, wetland types, Mangrove reviews, seamarks, extra styles | remote `comaps` | Apache-2.0 |
| [Guru Maps MapStyle](https://github.com/GuruMaps/MapStyle) | Base look: colors, zoom rules, terrain ramps | submodule `3party/gurumaps-mapstyle` | CC BY-NC-SA 4.0 |

Apple Maps sets the color palette.

## Remotes

- `organicmaps` is a blobless partial clone. It has full history, and file contents download when you need them. `main` tracks `organicmaps/master`.
- `comaps` has full history, but **it shares no commits with Organic Maps**, because CoMaps rewrote its history when it forked. You can't merge or cherry-pick from it. Port changes as patches instead:

  ```
  git diff main comaps/main -- data/styles/default
  git show comaps/main:libs/editor/review.cpp
  ```

## GitHub

- Repository: https://github.com/grovemaps/grove, a public fork of Organic Maps.
  - `grove` is the default branch and holds Grove's work.
  - `master` stays a plain copy of upstream Organic Maps (GitHub's "Sync fork" button updates it).
- Every push to `grove` runs `.github/workflows/grove-android.yaml`. It builds the F-Droid debug APK for arm64 and replaces the `grove-latest` release. Stable download link: https://github.com/grovemaps/grove/releases/download/grove-latest/grove-debug.apk
  - The APK is signed with `android/app/debug.keystore`, like local debug builds, so it installs over them as an update.
- Upstream's own workflows are disabled in the fork, so only Grove's build uses Actions.
- After syncing with a newer Organic Maps (below), `git push origin grove` publishes the merge and triggers a new build.

## Working on Grove on a new machine

- **Get the code:**
  - `git clone --recurse-submodules https://github.com/grovemaps/grove` (default branch `grove`).
  - Add the upstreams:
    - `git remote add organicmaps https://github.com/organicmaps/organicmaps.git`
    - `git remote add comaps https://codeberg.org/comaps/comaps.git`
- **Tools:**
  - Python 3.10+, CMake 3.22+, Ninja, `oxipng` or `optipng`.
  - Qt 6 with Core, Gui, Widgets, Xml, Svg, OpenGL, OpenGLWidgets and Network. It's needed for `skin_generator_tool` (icons) and the desktop app (renders).
  - Android: JDK 17, SDK platform 36, NDK 29.0.14206865; build with `cd android && ./gradlew -Parm64 assembleFdroidDebug`.
- **Style changes:**
  1. Edit `data/styles/grove/*`.
  2. Run `tools/grove/generate_drules.sh`, then `tools/grove/generate_symbols.py`.
  3. Commit the regenerated files separately as `[styles] Regenerated`.
- **Before pushing, check for logged errors.** The APKs are Debug builds, which stop on any logged error (LERROR) or failed ASSERT. Release desktop builds only log them. Run the desktop app with `--log_abort_level=E` in screenshot mode (see `tools/grove/render_screens.sh` for the flags) and make sure the log has no `E(` lines. On GitHub, the emulator test in `grove-android.yaml` refuses to publish an APK that doesn't survive start.
- **Known pitfalls:**
  - kothic rejects selectors on tag keys Organic Maps doesn't know.
  - CoMaps-only selectors like `[sport=...]` compile to `extra_tag` runtime conditions that crash Organic Maps Debug builds.
  - Drawing a new kind of area needs an entry in upstream's priorities files.
  - Organic Maps' downloadable maps only hold features from the zoom where upstream's style draws them (see "Map data limits").
- **Visual references:** the Apple Maps screenshots used for the palette aren't in the repo (Apple imagery). They're re-attached in chat when needed.

## Staying mergeable with Organic Maps

Grove must be able to take in new Organic Maps releases, so keep its changes out of upstream files wherever possible:

1. **New files, not edits.** Grove code and data go in Grove-owned files: `data/styles/grove/`, `tools/grove/`, and `grove_*` names elsewhere. An upstream file gets at most a small hook, such as an `@import` line, a flag or a single call.
2. **Override, don't rewrite.** Styles use kothic's rule that the last definition wins. `data/styles/grove/palette-light.mapcss` redefines the color variables and is imported right after upstream's colors. `default/light/colors.mapcss` stays byte-identical to upstream.
3. **Never hand-merge generated files.** `data/drules_*`, `colors.txt`, `patterns.txt`, `visibility.txt`, `classificator.txt` and `types.txt` are rebuilt by `tools/grove/generate_drules.sh`. It runs upstream's `tools/unix/generate_drules.sh`, then restores the `priorities_*.prio.txt` files, where Grove's overrides would otherwise only rewrite the zoom notes in comments. The icon atlases `data/symbols/*/*/symbols.*` are rebuilt by `tools/grove/generate_symbols.py`. **Don't run upstream's `tools/unix/generate_symbols.sh`**, because it would drop the Grove icon colors. Commit generated files separately as `[styles] Regenerated`.
4. **Small, prefixed commits:** `[grove]`, `[styles]`, `[android]` and so on. Each commit does one thing, so conflicts stay easy to read.

Current upstream hooks:

| Upstream file | Hook |
|---|---|
| `data/styles/default/light/style.mapcss` | imports `grove/palette-light.mapcss` after `colors.mapcss`, `grove/poi-label-colors.mapcss` after `Icons.mapcss`, and `grove/apple-look.mapcss` last |
| `data/styles/default/dark/style.mapcss` | imports `grove/palette-dark.mapcss` after `colors.mapcss`, `grove/poi-label-colors.mapcss` after `Icons.mapcss`, and `grove/apple-look.mapcss` last |
| `data/styles/outdoors/{light,dark}/colors.mapcss` | imports the matching Grove palette, placed before the outdoors-only overrides so those still win |
| `data/styles/outdoors/{light,dark}/style.mapcss` | imports `grove/poi-label-colors.mapcss` after `Icons.mapcss` |
| `libs/platform/platform.cpp` | adds `fonts/08_inter_medium.ttf` to the bundled font list |
| `libs/map/framework.cpp` | tile feature reading goes through `libs/map/grove_landcover_reading.hpp` (Grove), which adds the zoom 12 landcover at zoom 11 |
| `libs/drape_frontend/apply_feature_functors.cpp` | after each place icon, `grove::InsertPoiDot` (`libs/drape_frontend/grove_poi_dot.hpp`, Grove) adds its dot |
| `data/fonts/whitelist.txt` | Inter for Latin, Greek and Cyrillic blocks; drops the system Roboto entries for those blocks (a whitelisted system font loads last and would win) |

Grove style files in `data/styles/grove/`:

- `palette-light.mapcss` holds the Apple Maps colors measured from the reference screenshots.
- `palette-dark.mapcss` holds dark-mode values. For now it only has POI label colors, derived from the Apple light hues.
- `poi-label-colors.mapcss` colors POI labels by category. It's ported from CoMaps' `Icons_Label_Colors.mapcss` (see its header for the source commit and the one line dropped). Grove's changes are at the end.
- `apple-look.mapcss` holds rule overrides for the Apple look in the default style: no nature-reserve hatching, house numbers only from zoom 19, fewer road signs when zoomed out, smaller district names. The outdoors style doesn't import it, so hikers still see reserves.
- `declutter.mapcss` (imported by `apple-look.mapcss`) only switches upstream icons and names off at some zooms, so features appear later. Street names follow the road hierarchy (primary from zoom 11, residential from 15). Neighbourhood names start at 15, and churches, memorials and artworks two zooms later. Street furniture (bike racks, bins, barriers, vending machines, masts) only shows at zoom 18–19.
- `landcover.mapcss` (imported by `apple-look.mapcss`) is CoMaps' vegetation rules: separate colors per class, lighter when zoomed out. The Apple-hued values are in the palettes. The zoom 11 rules only take effect with Grove-built maps (see below).
- `icon-colors.txt` sets the icon circle color for each label category (measured from Apple Maps), plus explicit rules for icons without a category (transit, parking, fuel, EV charging).

### Icons

`tools/grove/generate_symbols.py` builds the icon atlases. It copies upstream's SVGs to a temp folder, recolors the copies and runs `skin_generator_tool` on them, so upstream SVGs are never edited. Each icon takes the color of the label category its place types share, read from the compiled `data/drules_default.txt`. Icons shared by several categories keep upstream colors (`--verbose` lists them). So new upstream icons are colored automatically.

It also writes an `<icon>-dot` symbol for every recolored icon: a small circle of the icon's color with a thin ring. The renderer draws that dot under the icon in the geometry layer, where nothing collides (`grove_poi_dot.hpp`). A visible icon covers its dot, and when a neighbour displaces the icon the dot stays, as in Apple Maps.

The script needs Qt 6 to build `skin_generator_tool` (`QT_PATH` in `/Volumes/grove/tools/env.sh`). It uses `optipng` or `oxipng` for lossless compression. With unmodified SVGs, this toolchain reproduces upstream's committed atlases pixel for pixel.

The vehicle (navigation) style isn't touched yet.

### Syncing with a newer Organic Maps

**By hand, while Grove's changes are uncommitted** (current setup):

```
source /Volumes/grove/tools/env.sh
git fetch organicmaps master
git log --oneline HEAD..organicmaps/master        # what's new; nothing listed = nothing to do
git checkout -- data/drules_*.bin data/drules_*.txt data/colors.txt data/symbols   # compiled files, rebuilt below
git stash push -m "grove: uncommitted changes"
git merge --ff-only organicmaps/master
git stash pop --index                             # conflicts here are the only ones to resolve by hand
git submodule update --init --recursive --depth 1
tools/grove/generate_drules.sh                    # upstream's generate_drules.sh, keeping priority files upstream's
tools/grove/generate_symbols.py                   # icons; needs the drules from the line above
```

Untracked Grove files (`GROVE.md`, `data/styles/grove/`, `tools/grove/`) aren't stashed and stay where they are. Use `--index` on `stash pop`, or the staged Guru submodule comes back unstaged.

**With commits** (once Grove's changes are committed):

```
source /Volumes/grove/tools/env.sh
tools/grove/sync_upstream.sh
```

The script fetches `organicmaps/master` and merges it. For conflicting generated files it takes upstream's copy, then regenerates them from the merged sources, which re-applies the Grove overrides. It then updates submodules and commits `[styles] Regenerated`. If a hand-written file conflicts, resolve it, commit the merge, then run `tools/grove/sync_upstream.sh --regenerate`.

## Map data limits

Organic Maps' downloadable maps only store a feature for the zooms where Organic Maps' own style draws it. So grass, meadow, heath and scrub exist from zoom 12, and farmland from zoom 14. A Grove style can color these from zoom 12 on (verified in Waterland), but not in the zoom 10–11 region views. Apple-like greenery there needs maps built with Grove's style (`generator_tool`).

Prototype (zoom 11): `grove_landcover_reading.hpp` makes zoom 11 tiles also read the zoom 12 feature index, so the fields and meadows stored from zoom 12 are drawn there. Zoom 11 uses the maps' zoom 12 geometry level, which those features have. Zoom 10 would need an engine change to borrow that geometry too. Cost: about 2.8× the frame time in the desktop app's software renderer; phone performance still to be measured.

## Fonts

Labels use Inter Medium (`data/fonts/08_inter_medium.ttf`, SIL Open Font License, see `08_inter_LICENSE.txt`) for Latin, Greek and Cyrillic, instead of Roboto. Other scripts keep upstream's Noto and fallback fonts.

## Checking the look without a phone

```
source /Volumes/grove/tools/env.sh
tools/grove/render_screens.sh [points file] [output dir]
```

This builds the Qt desktop app and renders screenshots in its screenshot mode. It defaults to `tools/grove/amsterdam-points.txt`, the spots of the Apple Maps reference screenshots, at 2000×1256 and 2× scale. Region maps download on first use into `/Volumes/grove/desktop-data`. The Mac is a virtual machine without GPU passthrough, so the app renders in software, about 2 minutes per frame.

## Styles

Organic Maps compiles `data/styles/*/{light,dark}/style.mapcss` (with `include/*.mapcss`) into `data/drules_*.bin` using kothic (`tools/kothic`). Guru uses a different MapCSS dialect for the GLMap engine. Its rules have to be translated into the Organic Maps dialect; they can't be dropped in as-is. Translated rules should go in a Grove include imported after upstream's includes, following rule 2 above.

Guru's hillshading and relief come from the GLMap engine, which renders DEM tiles using lookup images (`imhof5-1.jpg`, `imhof-dark.jpg`, `slope.jpg`). The Organic Maps renderer (`drape`) only draws contour lines (isolines). Hillshading needs new engine and data work.

## License

The Guru style is licensed **CC BY-NC-SA 4.0**. Anything derived from it must be non-commercial, credit Guru Maps and Globus.software, and share alike. Keep derived style files separate from the Apache-2.0 code.
