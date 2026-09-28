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
- Finished, tested work is merged into `grove` right away (the owner's standing choice), so every feature reaches the APK.

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
  - A new file in `data/` reaches the Android app only with a symlink in `android/sdk/src/main/assets/`.
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
| `libs/platform/platform.cpp` | adds Inter Medium and its italic and semibold twins (`fonts/08_inter_*.ttf`) to the bundled font list |
| `libs/drape/glyph_manager.cpp` | keeps the Inter twins out of unicode block selection; `ShapeText` reads a label's typography marker (`libs/drape/grove_text_style.hpp`, Grove): twin fonts, letter spacing |
| `libs/map/framework.cpp` | tile feature reading goes through `libs/map/grove_landcover_reading.hpp` (Grove), which adds the zoom 12 index at zoom 11 (landcover) and the zoom 16 index at zoom 15 (chains' logos) |
| `libs/drape_frontend/apply_feature_functors.cpp` | after each place icon, `grove::InsertPoiDot` (`libs/drape_frontend/grove_poi_dot.hpp`, Grove) adds its dot; `grove::StyleCaption` and `grove::StylePathText` (`libs/drape_frontend/grove_typography.hpp`, Grove) pick each label's typography |
| `libs/shaders/GL/area3d.vsh.glsl`, `texturing3d.fsh.glsl`, `libs/shaders/Metal/map.metal` (`vsArea3d`, `fsArea3d`) | 3D building lighting, see "Depth" below; `data/vulkan_shaders/*` are regenerated from the GL files |
| `libs/drape_frontend/frontend_renderer.cpp` | 3D buildings at 90% opacity instead of 70%; the relief layer (`libs/drape_frontend/grove_relief.hpp`, Grove) gets its tiles routed to it and is drawn after the 2D layer |
| `libs/drape/drape_global.hpp` | adds `BackgroundMode::Relief`, which gives relief tiles their own texture pool |
| `libs/map/framework.cpp`, `framework.hpp` | creates the relief tile provider (`libs/map/grove_relief.cpp`, Grove) before the drape engine |
| `libs/drape/texture_manager.cpp`, `.hpp` | owns the brand logo texture (`libs/drape/grove_brand_texture.hpp`, Grove); symbols named `brand:<Wikidata id>` come from it |
| `libs/drape_frontend/rule_drawer.cpp` | draws chains' logos a zoom early (`grove::EarlyLogoRule`), and nothing else of the places read only for that |
| `libs/drape_frontend/apply_feature_functors.cpp` (again) | `grove::UseBrandBadge` (`libs/drape_frontend/grove_brands.hpp`, Grove) swaps a chain's category icon for its logo badge |
| `android/app/build.gradle` | stores `grove_brands.bin` uncompressed, so it is read in place |
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

Labels use Inter 4.1 Medium (`data/fonts/08_inter_medium.ttf`, SIL Open Font License, see `08_inter_LICENSE.txt`) for Latin, Greek and Cyrillic, instead of Roboto. Other scripts keep upstream's Noto and fallback fonts.

Typography that the drawing rules can't express is chosen in code, by feature type (`libs/drape_frontend/grove_typography.hpp`):

| Labels | Style |
|---|---|
| places with an icon (POIs), cities, towns, countries | semibold (`08_inter_semibold.ttf`) |
| states, suburbs, quarters, neighbourhoods, streets | spaced capitals: upper case, +0.09 em letter spacing; street names at 90% size |
| water: seas, bays, lakes, rivers, canals | italic (`08_inter_medium_italic.ttf`) |

Spaced capitals only apply to scripts with letter case, so Arabic, Chinese and similar names stay as they are. The style rides in front of the label text as one private-use character, which `GlyphManager::ShapeText` takes off (`libs/drape/grove_text_style.hpp`). That keeps the upstream text layout code untouched. The italic and semibold files serve no unicode block of their own, so unstyled text never picks them.

Geist was considered instead of Inter and left out: it has no Greek, about half of Cyrillic and little of Latin Extended-B and Vietnamese (Geist 1.7.2), so names in those scripts would switch fonts mid-word.

## Depth

3D buildings are lit like a shaded relief map: the sun comes from the upper left of the screen, roofs are brightest, walls facing the sun are almost as bright and walls in shade drop to 72%. Walls also darken toward the ground (to 80% at the base), which makes buildings read as standing on the street. Buildings are 90% opaque instead of upstream's 70%.

The lighting lives in the shaders, which exist twice: GLSL (`libs/shaders/GL`, also compiled for Vulkan) and Metal (`libs/shaders/Metal/map.metal`). Keep both in step. After changing a GL shader, regenerate the Vulkan pack and commit it separately as `[shaders] Regenerated`. Use the NDK's `glslc` (r29 reproduces the committed pack byte for byte; Ubuntu's `glslc` doesn't):

```
LD_LIBRARY_PATH=<dir with libc++.so> python3 libs/shaders/vulkan_shaders_preprocessor.py libs/shaders/GL shader_index.txt \
  shaders_lib.glsl data/vulkan_shaders <ndk>/shader-tools/linux-x86_64/glslc empty
```

(`tools/unix/generate_vulkan_shaders.sh` does the same when it finds the NDK.)

## Relief

Hills and mountains are shaded, as in Guru and Apple Maps: slopes facing away from a northwest sun get a cool grey-blue shadow, slopes facing it a warm light, flat ground nothing. Water and flat countries like the Netherlands look unchanged.

- **Data:** [Terrarium elevation tiles](https://github.com/tilezen/joerd/blob/master/docs/formats.md#terrarium) (Tilezen/Mapzen on AWS Open Data: worldwide, free, no key, zoom 0–15; [attribution](https://github.com/tilezen/joerd/blob/master/docs/attribution.md)). Upstream's `RasterTileProvider` downloads them and caches up to 200 MB in `grove_relief/` in the app's data folder.
- **Shading:** `grove::ShadeRelief` (`libs/map/grove_relief.cpp`) turns each elevation tile into a transparent overlay tile with Horn's slope method. Zoomed out, terrain is exaggerated (up to 4×) so it doesn't look flat. The cache keeps the raw elevation, so shading changes need no new downloads.
- **Drawing:** a second instance of upstream's raster tile renderer (`libs/drape_frontend/grove_relief.hpp`) draws the overlay with normal alpha blending after areas and roads, under 3D buildings, routes, icons and labels. It works on OpenGL, Vulkan and Metal without shader changes.
- **Switch:** Android Settings → "General settings" → "Shaded relief", under "3D buildings" (`GroveSettings.java`, JNI in `android/sdk/.../GroveRelief.cpp`), on by default. It takes effect at once: off stops downloads and frees the relief textures. The value is the `GroveRelief` settings key (`grove::SetReliefEnabled`). iOS and the desktop app have no switch yet.
- **Online for now:** relief tiles download while browsing, which tells Amazon's servers which areas are viewed; the switch's summary says so.
- **Next: height data from the maps.** Relief should come from elevation data shipped with the downloaded maps, so it works offline and leaks nothing. Organic Maps already builds its contour lines from SRTM elevation data (`topography_generator_tool`); the same source can produce elevation tiles per map region. Only the tile source changes: `ShadeRelief` and the relief layer stay as they are.

## Brand logos

Places of chains (Albert Heijn, McDonald's, Lidl...) show the chain's logo instead of the category icon: the logo itself, transparent around it, with a soft halo like map labels (dark for logos drawn in white). It shows at every zoom where the icon shows, from the zoom where the name shows if that is earlier, and from zoom 15 for places first drawn at 16 (supermarkets, hotels...). Places without a known brand keep the Apple-style category icons. The downloaded maps only contain a place from the zoom where upstream's style draws it (see "Map data limits"), so zoom 15 also reads the zoom 16 index (`libs/map/grove_landcover_reading.hpp`); what only that index has is drawn only as logos (`grove::BorrowedFeatures`, `EarlyLogoRule`), so zoom 15 gains chains, not clutter. Earlier than 15 would read 16 times the places per tile and slow the map. A displaced badge leaves the category-coloured dot, like any icon.

- **Data:** `tools/grove/brand_logos.py` builds `data/grove_brands.txt` (index) and `data/grove_brands.bin` (logo PNGs with halo, 96 px). Brands come from the [Name Suggestion Index](https://github.com/osmlab/name-suggestion-index) (names and countries), kept if at least 25 OpenStreetMap places carry their `brand:wikidata` ([taginfo](https://taginfo.openstreetmap.org/keys/brand:wikidata)). Logos are Wikidata's "small logo or icon", "icon" or "logo image", as 250 px PNG thumbnails from Wikimedia Commons, which only hosts free or public-domain files.
- **Only readable logos:** wordmarks wider than 1.8:1 (Primark, Starbucks, Hugo Boss) are unreadable at icon size, so those brands keep their category icons; compact marks (McDonald's M, AH, Dunkin', Shell) become badges. JPEG "logos" full of colours are usually photos of shop signs and are skipped too.
- **Downloading:** everything is cached in `build-grove/brands-cache`, so reruns fetch only what is new, and `--cached-only` rebuilds the pack offline. Wikimedia blocks clients that go faster than about one request a second for 10 minutes (HTTP 429 with `Retry-After: 600`), and only serves its standard thumbnail sizes (https://w.wiki/GHai) to scripts; the script keeps to both, so a first full run takes about an hour. Commit the regenerated pack separately as `[brands] Regenerated`.
- **Current pack is partial:** it was built in a cloud session that Wikimedia blocked after the 1,072 most used brands' logos, so it holds 278 badges (1.5 MB) of the 3,834 brands with a logo. Run `tools/grove/brand_logos.py` from another connection to add the rest.
- **Trademarks:** the logo files are free of copyright but remain their owners' trademarks. They are shown only to mark where a chain's shops are.
- **Matching:** by the place's name, or its leading words ("Albert Heijn Dam" matches "Albert Heijn"). The maps keep a place's `brand` tag only when it differs from its name (the generator drops it otherwise, which is most places), and reading it for every place would slow tile loading. `BrandPack::Find` looks it up by name (any case) and also requires the place's type to be one of the brand's (from the Name Suggestion Index, e.g. `shop-supermarket`), so an independent shop that shares a chain's name stays plain. A brand listed for the place's country (the map file name up to the first `_`, e.g. `Netherlands`) wins over a worldwide one, so the Dutch, Swiss and Danish Coops each get their own logo.
- **Rendering:** badges load on first use into one 2048×1024 texture of 32 dp slots (`BrandTexture`), larger than category icons, downscaled to the screen density: 441 slots at 3× (16 MB). When it is full, further brands keep category icons until the app restarts.
- **Platforms:** the desktop app reads the pack from `data/`; Android through the symlinks `android/sdk/src/main/assets/grove_brands.*` (Android packages only data files linked there). iOS lists data files one by one in its Xcode project, so it needs the two files added there; until then it shows category icons.

## Place cards

On Android, tapping a place opens a card tinted in the place's colour: its chain's logo colour (worked out by `tools/grove/brand_logos.py` as the logo's most common clear colour), otherwise its category colour, the label colour of its type in the current day or night style, which is also its icon circle's colour. Places without an icon keep a plain card. The whole card gets a 14% tint, every section painted in the card or section-gap colour included (re-applied as sections load or the card is pulled up); the title takes the full colour, darkened (day) or lightened (night) until it reads. Code: `libs/map/grove_place_color.cpp`, JNI `android/sdk/.../GrovePlace.cpp`, card `android/app/.../placepage/GrovePlaceCard.java`, one call in `PlacePageView.refreshPreview`.

## Checking the look without a phone

```
source /Volumes/grove/tools/env.sh
tools/grove/render_screens.sh [points file] [output dir]
```

This builds the Qt desktop app and renders screenshots in its screenshot mode. On Linux without a display it runs the app under `xvfb-run` with Mesa's software OpenGL (packages `xvfb`, Qt 6 dev); the first run needs `EulaAccepted=true` in `~/.config/OrganicMaps/settings.ini`, or the app waits on the license dialog. For tilted 3D views add `Allow3d=true` and `Buildings3d=true` to the same file; the desktop app then starts in perspective (only the first point of a run renders, the next one waits forever). Under Xvfb the app can hang in Mesa's buffer swap after its last screenshot, so the script stops it once the log says `state: Done`. It defaults to `tools/grove/amsterdam-points.txt`, the spots of the Apple Maps reference screenshots, at 2000×1256 and 2× scale. Region maps download on first use into `/Volumes/grove/desktop-data`. The Mac is a virtual machine without GPU passthrough, so the app renders in software, about 2 minutes per frame.

### Vulkan, as on Android

Android uses Vulkan by default; the desktop app only OpenGL. `tools/grove/render_vulkan.py LAT LON ZOOM out.png` renders a view with `dev_sandbox` (build target `dev_sandbox`) on Mesa's software Vulkan driver (packages `mesa-vulkan-drivers`, `xvfb`, `imagemagick`), using the maps downloaded by `render_screens.sh`. Pass `--api OpenGL` to compare. A Debug build (`build-grove-debug`) stops at failed assertions like the Android debug APK.

## Styles

Organic Maps compiles `data/styles/*/{light,dark}/style.mapcss` (with `include/*.mapcss`) into `data/drules_*.bin` using kothic (`tools/kothic`). Guru uses a different MapCSS dialect for the GLMap engine. Its rules have to be translated into the Organic Maps dialect; they can't be dropped in as-is. Translated rules should go in a Grove include imported after upstream's includes, following rule 2 above.

Guru's hillshading and relief come from the GLMap engine, which renders DEM tiles using lookup images (`imhof5-1.jpg`, `imhof-dark.jpg`, `slope.jpg`). The Organic Maps renderer (`drape`) only draws contour lines (isolines). Hillshading needs new engine and data work.

## License

The Guru style is licensed **CC BY-NC-SA 4.0**. Anything derived from it must be non-commercial, credit Guru Maps and Globus.software, and share alike. Keep derived style files separate from the Apache-2.0 code.
