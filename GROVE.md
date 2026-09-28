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
| `libs/platform/platform.cpp` | adds Inter Medium and its italic, semibold and Geist twins (`fonts/08_inter_*.ttf`, `fonts/08_geist_medium.ttf`) to the bundled font list |
| `libs/drape/glyph_manager.cpp` | keeps the Inter twins out of unicode block selection; `ShapeText` reads a label's typography marker (`libs/drape/grove_text_style.hpp`, Grove): twin fonts, letter spacing |
| `libs/map/framework.cpp` | tile feature reading goes through `libs/map/grove_landcover_reading.hpp` (Grove), which adds the zoom 12 index at zoom 11 (landcover); creates the chains' place lists (`libs/map/grove_brand_places.hpp`, Grove) for the logo layer |
| `libs/drape_frontend/apply_feature_functors.cpp` | after each place icon, `grove::InsertPoiDot` (`libs/drape_frontend/grove_poi_dot.hpp`, Grove) adds its dot; `grove::StyleCaption`, `grove::StylePathText` and `grove::StyleNumber` (house numbers, road shields; `libs/drape_frontend/grove_typography.hpp`, Grove) pick each label's typography; road shields are sized from the styled text |
| `libs/shaders/GL/area3d.vsh.glsl`, `texturing3d.fsh.glsl`, `libs/shaders/Metal/map.metal` (`vsArea3d`, `fsArea3d`) | 3D building lighting, see "Depth" below; `data/vulkan_shaders/*` are regenerated from the GL files |
| `libs/drape_frontend/frontend_renderer.cpp` | 3D buildings at 90% opacity instead of 70%; Grove's raster layers (`libs/drape_frontend/grove_raster_layers.hpp`, Grove) get their tiles routed to them; land cover is drawn before the 2D layer, relief after it |
| `libs/drape/drape_global.hpp` | adds `BackgroundMode::Relief` and `Landcover`, which give those layers' tiles their own texture pools |
| `libs/drape_frontend/visual_params.cpp` | wider, softer label halos, like Mapy.com's white outlines (outline threshold 0.13, softness 0.025; was 0.2 and 0.01) |
| `android/app/.../maplayer/MapButtonsController.java` | the top-left map button (upstream's help and donation button) shows and hides brand logos (`GroveBrandsButton.java`); help and donating stay in the main menu |
| `data/copyright.html` | credits ESA WorldCover, Mangrove and the Terrain Tiles |
| `libs/map/framework.cpp`, `framework.hpp` | creates the relief tile provider (`libs/map/grove_relief.cpp`, Grove) before the drape engine |
| `libs/drape/texture_manager.cpp`, `.hpp` | owns the brand logo texture (`libs/drape/grove_brand_texture.hpp`, Grove); symbols named `brand:<Wikidata id>` come from it |
| `libs/drape_frontend/rule_drawer.cpp`, `area_shape.hpp`; `texturing3d.fsh.glsl`, `map.metal` (`vsArea3d`) | 3D buildings with a place inside are see-through (`libs/drape_frontend/grove_buildings.hpp`, Grove): RuleDrawer holds a tile's 3D buildings until its places are read; the 3D shaders use the colour's alpha |
| `libs/map/framework.cpp` (`LoadIsolinesEnabled`) | contour lines are on until switched off in the layers menu, as in Guru Maps; the maps have them only where there is terrain |
| `libs/drape_frontend/user_event_stream.cpp`, `navigator.cpp` | two fingers sliding up or down together tilt the map into 3D at any zoom (`libs/drape_frontend/grove_gestures.hpp`, Grove); other two-finger moves zoom and rotate as before |
| `libs/drape_frontend/frontend_renderer.cpp` (`OnTwoFingersTap`), `user_event_stream.hpp` | a two-finger tap shows the distance between the fingers instead of zooming out, as in Guru Maps (`libs/drape_frontend/grove_measure.hpp`, `libs/map/grove_measure.cpp`, Android `GroveMeasureOverlay.java`: a line and a label that fade after 3 s) |
| `android/app/.../maplayer/MapButtonsController.java` (again) | adds the distance overlay under the map buttons |
| `libs/drape_frontend/tile_info.cpp` | after a tile's features, draws its logo layer (`grove::DrawBrandLayer`, `libs/drape_frontend/grove_brand_layer.hpp`, Grove) |
| `libs/drape_frontend/poi_symbol_shape.cpp`, `libs/drape/overlay_tree.cpp` | logos get `grove::kBrandPriority`, above every map label and icon, and never hide each other |
| `libs/drape_frontend/apply_feature_functors.cpp` (again) | chains get no category icon (the logo layer draws theirs), and from zoom 16 their name under the logo (`libs/drape_frontend/grove_brands.hpp`, Grove) |
| `libs/drape_frontend/relations_draw_info.{hpp,cpp}`, `rule_drawer.cpp`, `apply_feature_functors.cpp` (again) | the cycling layer draws one solid line per road, coloured by the road's highest cycle network, instead of a stripe per route (`libs/drape_frontend/grove_cycle_routes.hpp`, Grove) |
| `libs/routing/geometry.cpp` | bicycle routing raises the weight of roads in signed cycle routes (`libs/routing/grove_cycle_routes.hpp`, Grove) |
| `android/app/src/main/res/xml/prefs_main.xml` (again) | "Prefer cycle routes" switch under Navigation (`GroveSettings.java`, JNI `GroveCycleRoutes.cpp`) |
| `android/app/build.gradle` | stores `grove_brands.bin`, `grove_landcover_world.bin` and `grove_reviews.bin` uncompressed, so they are read in place |
| `android/app/src/main/res/**`, `RoutingBottomMenuController.java` | Roboto references point to the app font (Geist, Inter for Greek), and the app themes hang under `values/grove_fonts.xml`; written by `tools/grove/android_fonts.py`, see "Fonts" |
| `android/app/src/main/res/layout/place_page_details.xml`, `PlacePageView.java` | a container for the Mangrove reviews section and the one line that shows it (`GroveReviewsFragment`) |
| `data/fonts/whitelist.txt` | Inter for Latin, Greek and Cyrillic blocks; drops the system Roboto entries for those blocks (a whitelisted system font loads last and would win) |

Grove style files in `data/styles/grove/`:

- `palette-light.mapcss` holds the Apple Maps colors measured from the reference screenshots, with Organic Maps' contrast: amber motorways and trunks, yellow primaries, white streets, buildings with a defined border on a warm background (Apple's grey roads on grey read as grey on grey).
- `palette-dark.mapcss` holds dark-mode values. For now it only has POI label colors, derived from the Apple light hues.
- `poi-label-colors.mapcss` colors POI labels by category. It's ported from CoMaps' `Icons_Label_Colors.mapcss` (see its header for the source commit and the one line dropped). Grove's changes are at the end.
- `apple-look.mapcss` holds rule overrides for the Apple look in the default style: no nature-reserve hatching, house numbers only from zoom 19, fewer road signs when zoomed out, smaller district names. The outdoors style doesn't import it, so hikers still see reserves.
- `paths.mapcss` (imported by `apple-look.mapcss`) redraws paths after Apple Maps and Mapy.com: footways and pedestrian streets solid white with a thin warm grey casing (upstream's near-white dashes read as faint dots), paths and tracks warm brown with longer dashes, cycleways solid violet. Shared foot and cycle ways keep their dashes, so the violet cycle line shows in the gaps.
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
| road shields, house numbers, contour heights | Geist Medium (`08_geist_medium.ttf`), the app's font; only for ASCII text, so a label never mixes fonts |

Spaced capitals only apply to scripts with letter case, so Arabic, Chinese and similar names stay as they are. The style rides in front of the label text as one private-use character, which `GlyphManager::ShapeText` takes off (`libs/drape/grove_text_style.hpp`). That keeps the upstream text layout code untouched. The italic, semibold and Geist files serve no unicode block of their own, so unstyled text never picks them.

Geist was considered for the labels and left out: it has no Greek and only part of the Cyrillic and Vietnamese blocks (Geist 1.8, npm `geist` 1.7.2), so place names would switch fonts mid-word.

The Android app uses Geist instead (SIL Open Font License, `data/fonts/geist_LICENSE.txt`), which sets the interface apart from the map: Regular for text, Medium where upstream uses `sans-serif-medium` (buttons, titles), SemiBold and Bold for bold text. Layouts and styles name the families `@font/ui` and `@font/ui_medium`. For translations using letters Geist lacks but Inter has (Greek for now), `res/font-<language>/` switches the families to Inter (`android/app/src/main/res/font/inter_*`; Medium and SemiBold link to the map's files). Scripts neither font covers (Arabic, CJK, Hindi...) use the system fonts either way.

`tools/grove/android_fonts.py` writes all of this: it replaces upstream's Roboto references (`@string/robotoRegular`, `robotoMedium`, `robotoLight`) in layouts, styles and spans, sets the app themes' parents to the Grove themes in `values/grove_fonts.xml`, which give Material's text appearances the app font, and writes the per-language font folders from the translations' letters. After syncing with upstream or adding translations, run it again (`--check` lists what is out of date), then clang-format the Java files it changed. Android 8+ and the AppCompat views get the new fonts; plain framework views on older Android keep Roboto.

## Depth

3D buildings are lit like a shaded relief map: the sun comes from the upper left of the screen, roofs are brightest, walls facing the sun are almost as bright and walls in shade drop to 72%. Walls also darken toward the ground (to 80% at the base), which makes buildings read as standing on the street. Buildings are 90% opaque instead of upstream's 70%.

The lighting lives in the shaders, which exist twice: GLSL (`libs/shaders/GL`, also compiled for Vulkan) and Metal (`libs/shaders/Metal/map.metal`). Keep both in step. After changing a GL shader, regenerate the Vulkan pack and commit it separately as `[shaders] Regenerated`. Use the NDK's `glslc` (r29 reproduces the committed pack byte for byte; Ubuntu's `glslc` doesn't):

```
LD_LIBRARY_PATH=<dir with libc++.so> python3 libs/shaders/vulkan_shaders_preprocessor.py libs/shaders/GL shader_index.txt \
  shaders_lib.glsl data/vulkan_shaders <ndk>/shader-tools/linux-x86_64/glslc empty
```

(`tools/unix/generate_vulkan_shaders.sh` does the same when it finds the NDK.)

## See-through buildings

3D buildings with a place inside them (a shop, café, museum: a point with an icon or name inside the footprint) are drawn at about a third of the usual opacity, so the place's dot and the ground show through, as in Apple Maps; other buildings stay solid. A tile's features come in id order, so buildings are held back until the tile's places are read (`grove::HollowBuildings`). The 3D building shaders multiply the colour's alpha into the layer's opacity, which is how a building gets its own; `data/vulkan_shaders` are regenerated from the GL files.

## Relief

Hills and mountains are shaded, as in Guru Maps: light comes mostly from the northwest and partly from the west and north (multidirectional hillshading), so ridges read whatever way they run. Slopes facing away from it get a near-black shadow (up to 60% opacity), which darkens the map's colours like Guru's multiplied shading (a forest in shadow turns deep green, not grey); slopes facing it a warm light (up to 30%); steep ground darkens further whichever way it faces (slope shading, up to 24%), for depth. Flat ground gets nothing. Elevation is lightly smoothed first, since zoomed in the elevation tiles come in steps that strong shading would turn into staircases. Ground above 300 m also gets an elevation tint, as on printed physical maps and in Guru Maps: warm tan in the hills (up to 18% from 1,500 m), paler grey-brown above 2,500 m (`grove::ElevationTint`). Water and flat countries like the Netherlands look unchanged.

- **Data:** [Terrarium elevation tiles](https://github.com/tilezen/joerd/blob/master/docs/formats.md#terrarium) (Tilezen/Mapzen on AWS Open Data: worldwide, free, no key, zoom 0–15; [attribution](https://github.com/tilezen/joerd/blob/master/docs/attribution.md)). Upstream's `RasterTileProvider` downloads them and caches up to 200 MB in `grove_relief/` in the app's data folder.
- **Shading:** `grove::ShadeRelief` (`libs/map/grove_relief.cpp`) turns each elevation tile into a transparent overlay tile with Horn's slope method. Zoomed out, terrain is exaggerated (up to 4×) so it doesn't look flat. The cache keeps the raw elevation, so shading changes need no new downloads.
- **Drawing:** Grove's relief layer, an instance of upstream's raster tile renderer (`libs/drape_frontend/grove_raster_layers.hpp`), draws the overlay with normal alpha blending after areas and roads, under 3D buildings, routes, icons and labels. It works on OpenGL, Vulkan and Metal without shader changes.
- **Switch:** Android Settings → "General settings" → "Shaded relief", under "3D buildings" (`GroveSettings.java`, JNI in `android/sdk/.../GroveRelief.cpp`), on by default. It takes effect at once: off stops downloads and frees the relief textures. The value is the `GroveRelief` settings key (`grove::SetReliefEnabled`). iOS and the desktop app have no switch yet.
- **Online for now:** relief tiles download while browsing, which tells Amazon's servers which areas are viewed; the switch's summary says so.
- **Next: height data from the maps.** Relief should come from elevation data shipped with the downloaded maps, so it works offline and leaks nothing. Organic Maps already builds its contour lines from SRTM elevation data (`topography_generator_tool`); the same source can produce elevation tiles per map region. Only the tile source changes: `ShadeRelief` and the relief layer stay as they are.

## Land cover

Zoomed out (zoom 11 down to the whole world), where Organic Maps' map files have no forests, fields or heath (zoom 9 and out only has the world map file, zoom 10 the countries' simplest shapes), the map is coloured by land cover: forests green, fields and grass paler, towns grey, bare ground and snow, in Grove's palette at its zoom 11 shades (dark ones in the dark style). The map's own areas are drawn over it and take over when zoomed in; from zoom 12 it is gone.

- **Data:** [ESA WorldCover 2021](https://esa-worldcover.org) (10 m, CC BY 4.0, credited in the app's copyright page), Cloud-Optimized GeoTIFFs on AWS Open Data: one file per 3° square, with overviews at 150, 300 and 600 m. `libs/map/grove_landcover.cpp` reads a square's header (its first 32 KB) and the one overview tile a map tile needs, with HTTP range requests, and caches both in `grove_landcover/` in the app's data folder (a square's header and its 600 m tile are about 100 KB; its whole file is up to 100 MB). Open sea has no files.
- **Colours:** each map tile pixel averages 2×2 samples of the classes' colours, which smooths the 150–600 m pixels. Water is left transparent, since the map draws it.
- **Drawing:** Grove's land cover layer (`libs/drape_frontend/grove_raster_layers.hpp`), drawn over the background and under the map's areas.
- **Zoom 6 and out:** a tile there spans dozens of squares, so these zooms come from a bundled pack instead, offline: `tools/grove/landcover_world.py` reads every square's 600 m overview, mosaics them at 0.025° and cuts web mercator tiles for zooms 1–6 as 8-bit PNGs of WorldCover classes, which the app colours for the light and dark styles (`data/grove_landcover_world.bin` and `.txt`, about 4.4 MB, stored uncompressed in the APK). Small lakes take the class of the land around them, since zoomed out the map doesn't draw them and they would show as background-coloured dots.
- **Switch:** Android Settings → "General settings" → "Land cover" (`GroveLandcover` settings key), on by default. Like relief, from zoom 7 it downloads while browsing, which tells Amazon's servers which areas are viewed.

## Cycle routes

- **Layer:** upstream's cycling layer draws a 3 dp stripe per route in the route's own colour (purple if it has none), side by side and dashed like the road under it. With the Dutch and Belgian node networks that covers whole towns in rainbow bands. Grove draws one solid line per road, in the colour of the highest cycle network the road belongs to, as Mapy.com does: deep magenta for national and international routes (`icn`, `ncn`), magenta for regional ones and node networks (`rcn`, translucent at zoom 12), light pink for local routes (`lcn`) and brown for mountain bike trails, both from zoom 14. Roads whose routes aren't drawn at a zoom keep the map style's visibility. Hiking and transit lines are unchanged.
- **Routing:** bicycle routes prefer roads of signed cycle routes, which are usually quieter and nicer: their weight speed is raised by 20% (national, regional) or 10% (local), capped at the model's maximum so A* stays exact; ETAs are unchanged. Stronger factors sent a ride across Amsterdam 48% further round the regional routes, so the preference only buys short detours (`routing_integration_tests/grove_cycle_routes_test.cpp`, with the Utrecht and Amsterdam maps). Upstream's alternative route (the other strategy) still offers the direct way. Switch: Settings → Navigation → "Prefer cycle routes" (`GroveCycleRoutes` settings key), on by default, from the next route.

## Brand logos

Places of chains (Albert Heijn, McDonald's, Lidl...) show the chain's logo instead of the category icon: the logo itself, transparent around it, with a soft halo like map labels (dark for logos drawn in white). Places without a known brand keep the Apple-style category icons.

- **Logo layer:** logos show from zoom 12, before buildings do, for every chain place in view, whether or not the map's style draws its category yet (`libs/drape_frontend/grove_brand_layer.hpp`). They are pinned: no label or icon hides them, and they never hide each other, so they don't come and go while zooming. Places closer than a logo's width are bundled into one row of logos centered on them, one per chain and at most three, most common chains first; zooming in splits the row. Zoomed out they thin out: at zoom 12 rows hold two logos and a chain shows once within six logo widths, at 13 within four, at 14 within three; from 15 every place shows. From zoom 16 the chain's name shows under its logo, and gives way to other labels. Tapping a logo opens its place.
- **Brands button:** the top-left map button shows and hides the logos (settings key `GroveBrands`, `grove::SetBrandsShown`): hidden, chains show like other places, with their category icons and names. It is accent-coloured while logos show.
- **Chains' place lists:** the downloaded maps only contain a place from the zoom where upstream's style draws it (see "Map data limits"), so the layer doesn't read tiles' features: `libs/map/grove_brand_places.cpp` reads each map file once, in the background, when its area first shows, and keeps the list of its chain places in `grove_brand_places/<map>.bin` in the app's data folder, until the map or the logo pack changes. That takes about 1 s for Amsterdam's map on a desktop; meanwhile the map shows no logos there, then redraws.


- **Data:** `tools/grove/brand_logos.py` builds `data/grove_brands.txt` (index) and `data/grove_brands.bin` (logo PNGs with halo, 96 px). Brands come from the [Name Suggestion Index](https://github.com/osmlab/name-suggestion-index) (names and countries), kept if at least 25 OpenStreetMap places carry their `brand:wikidata` ([taginfo](https://taginfo.openstreetmap.org/keys/brand:wikidata)). Logos are Wikidata's "small logo or icon", "icon" or "logo image", as 250 px PNG thumbnails from Wikimedia Commons, which only hosts free or public-domain files.
- **Only readable logos:** wordmarks wider than 1.8:1 (Primark, Starbucks, Hugo Boss) are unreadable at icon size, so those brands keep their category icons; compact marks (McDonald's M, AH, Dunkin', Shell) become badges. JPEG "logos" full of colours are usually photos of shop signs and are skipped too.
- **Downloading:** everything is cached in `build-grove/brands-cache`, so reruns fetch only what is new, and `--cached-only` rebuilds the pack offline. Wikimedia blocks clients that go faster than about one request a second for 10 minutes (HTTP 429 with `Retry-After: 600`), and only serves its standard thumbnail sizes (https://w.wiki/GHai) to scripts; the script keeps to both, so a first full run takes about an hour. Commit the regenerated pack separately as `[brands] Regenerated`.
- **Current pack is partial:** it was built in a cloud session that Wikimedia blocked after the 1,072 most used brands' logos, so it holds 278 badges (1.5 MB) of the 3,834 brands with a logo. Run `tools/grove/brand_logos.py` from another connection to add the rest.
- **Trademarks:** the logo files are free of copyright but remain their owners' trademarks. They are shown only to mark where a chain's shops are.
- **Matching:** by the place's name, or its leading words ("Albert Heijn Dam" matches "Albert Heijn"). The maps keep a place's `brand` tag only when it differs from its name (the generator drops it otherwise, which is most places), and reading it for every place would slow tile loading. `BrandPack::Find` looks it up by name (any case) and also requires the place's type to be one of the brand's (from the Name Suggestion Index, e.g. `shop-supermarket`), so an independent shop that shares a chain's name stays plain. A brand listed for the place's country (the map file name up to the first `_`, e.g. `Netherlands`) wins over a worldwide one, so the Dutch, Swiss and Danish Coops each get their own logo.
- **Rendering:** badges load on first use into one 2048×1024 texture of 32 dp slots (`BrandTexture`), larger than category icons, downscaled to the screen density: 441 slots at 3× (16 MB). When it is full, further brands keep category icons until the app restarts.
- **Platforms:** the desktop app reads the pack from `data/`; Android through the symlinks `android/sdk/src/main/assets/grove_brands.*` (Android packages only data files linked there). iOS lists data files one by one in its Xcode project, so it needs the two files added there; until then it shows category icons.

## Place cards

On Android, tapping a place opens a card whose title is in the place's colour: its chain's logo colour (worked out by `tools/grove/brand_logos.py` as the logo's most common clear colour), otherwise its category colour, the label colour of its type in the current day or night style, which is also its icon circle's colour, darkened (day) or lightened (night) until it reads. Places without an icon keep the plain title. Tinting the whole card was tried and dropped: it looked muddy. Code: `libs/map/grove_place_color.cpp`, JNI `android/sdk/.../GrovePlace.cpp`, card `android/app/.../placepage/GrovePlaceCard.java`, one call in `PlacePageView.refreshPreview`.

## Reviews

Place cards of shops, restaurants, hotels, museums and other places people review (CoMaps' list of about 260 types) show their reviews from [Mangrove](https://mangrove.reviews), an open review commons (CC BY 4.0): the average stars, the number of reviews, the five newest (tap one to read all of it), and a link to write one on Mangrove's site. Ported from CoMaps, with one difference: CoMaps builds the reviews into its own map files, matched to OpenStreetMap ids on its servers, while Grove uses Organic Maps' map files. So Grove matches reviews to places itself, from two sources: a pack of all of Mangrove's reviews with a geo subject bundled with the app, shown at once and offline, then Mangrove's API (`/geo`, the reviews in a small box around the place), which adds the newest when online. It keeps the reviews whose subject is the place: Mangrove names a place by a geo URI with its name and how far it reaches (`geo:52.3562,4.9115?q=Veganees&u=50`), so a review matches when its point lies within both places' reach plus 30 m and the names agree (any case), unless one has no name.

- **Bundled pack:** `tools/grove/mangrove_reviews.py` asks Mangrove for every geo review, box by box, and writes them by 1° cell: `data/grove_reviews.bin` (a zlib block of tab-separated lines per cell) and `data/grove_reviews.txt` (the index). About 8,500 reviews in 1 MB (September 2026). The APK workflow rebuilds it before each build, so every APK carries the newest reviews; the committed pack stays if Mangrove can't be reached. Rebuild and commit it now and then as `[reviews] Regenerated`.
- **Trend:** when at least two rated reviews from the last six months and two older ones differ by half a star or more, the card shows "Last 6 months: ★ 2.0 ↓ (3)" in red, or green with ↑, so a place that got worse or better stands out (`PlaceReviews::RecentTrend`).
- **Code:** `libs/map/grove_reviews.cpp` (subject, matching, pack, request, trend; tests in `map_tests/grove_reviews_tests.cpp`), JNI `android/sdk/.../GroveReviews.cpp`, section `android/app/.../placepage/sections/GroveReviewsFragment.java`.
- **Privacy:** the bundled reviews need no network; for the newest, it sends the place's position to Mangrove when its card opens. Settings → "Reviews from Mangrove" switches it off (key `GroveReviews`, on by default).
- **Strings:** `reviews_by_mangrove` and `add_review` with CoMaps' translations, `pref_mangrove_reviews_summary` and `reviews_last_6_months` new.
- **Not ported yet:** CoMaps sends some types to MapComplete's review themes, which needs the place's OpenStreetMap id; Grove sends all to Mangrove. iOS and the desktop app show no reviews yet.

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
