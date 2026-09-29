# Style editor

Customise the map while looking at it: touch anything to see what it is and where its colour comes from, choose a
preset for it or pick your own colour, and see the change at once.

## What it looks like

- 🎨 opens a sheet over the live map; dragged down, it leaves most of the map in view.
- **Touch the map** to pick: the elements under the finger are listed ("residential road", "park", "street label"),
  each with its colour swatch.
- **All elements, grouped**: roads and paths, water, land, buildings, borders, labels, icons, relief, land cover.
  Each row shows its swatch and a badge for where its colour comes from (Grove, Organic Maps, CoMaps, Apple, ✎
  custom); a dot marks what differs from the preset you started from.
- **Presets at two levels**: chips at the top set the whole map; each category can take its own, e.g.
  "Roads: Apple grey" over Grove.
- **Fine tuning**: a colour picker (wheel, hex, and the same element's colour in every preset as quick picks).
  Changes show on the map at once; press and hold to compare with the original; undo; reset per element, category
  or all. Light and dark map are edited apart.
- **Save and share**: a named style, exported as a small file, which the APK builder (../apk_builder) takes too.

## How it could work

- Grove's palette (`modules/look/styles/palette-*.mapcss`), Organic Maps' `colors.mapcss` and CoMaps' use the same
  colour variables, so every preset has a value for every element. The style build would write a palette index:
  each variable's category, name and colour per preset, and the compiled rules' colours that come from it.
- Colours are swapped in the loaded drawing rules and the colour texture at runtime (drape adds colours to its
  texture on the fly), then the tiles are read again: a second, no rebuild.
- Touching the map uses the feature under the finger (as the place card does) and its type's rules to find the
  variables it is drawn with.
- A custom style is a list of variable → colour overrides, per light and dark.

## Open questions

- Line widths and zoom ranges too, or colours only at first?
- Where one variable colours several elements (a shared grey), should the editor split it?
