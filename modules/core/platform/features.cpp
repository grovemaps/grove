#include "modules/core/platform/features.hpp"

#include "platform/settings.hpp"

#include "base/assert.hpp"

#include <atomic>
#include <iterator>

namespace grove
{
namespace
{
using enum Feature;
using G = FeatureGroup;

FeatureInfo constexpr kFeatures[] = {
    {Look, G::Map, "GroveLook", true, true,
     "Grove's map style and icons: Apple-like colours, Guru Maps' terrain, CoMaps' cartography. Off: Organic Maps' "
     "own.",
     "indexer/map_style_reader.cpp; modules/look/styles, modules/look/tools/extra_styles.sh"},
    {Typography, G::Map, "GroveTypography", true, true,
     "Inter for map labels: semibold places, italic water, spaced capitals for streets and areas, Geist numbers.",
     "platform/platform.cpp, drape/glyph_manager.cpp, drape_frontend/apply_feature_functors.cpp, "
     "data/fonts/whitelist.txt; modules/typography/drape/text_style.hpp, "
     "modules/typography/drape_frontend/typography.hpp"},
    {Halos, G::Map, "GroveHalos", true, true, "Wider, softer label halos, like Mapy.com's white outlines.",
     "drape_frontend/visual_params.cpp"},
    {Buildings3d, G::Map, "GroveBuildings3d", true, false,
     "3D buildings lit from the upper left, walls darkening toward the ground, more solid.",
     "drape_frontend/frontend_renderer.cpp, shaders/GL/area3d.vsh.glsl, texturing3d.fsh.glsl, Metal/map.metal"},
    {SeeThroughBuildings, G::Map, "GroveSeeThrough", true, true,
     "3D buildings with a shop or café inside are see-through, as in Apple Maps.",
     "drape_frontend/rule_drawer.cpp, area_shape.hpp; modules/see_through_buildings/drape_frontend/buildings.hpp"},
    {PoiDots, G::Map, "GrovePoiDots", true, true, "A small dot where each place is, under its icon.",
     "drape_frontend/apply_feature_functors.cpp; modules/poi_dots/drape_frontend/poi_dot.hpp"},
    {CycleRouteLines, G::Map, "GroveCycleLines", true, true,
     "The cycling layer draws one solid line per road, coloured by its highest network, not a stripe per route.",
     "drape_frontend/relations_draw_info.cpp, rule_drawer.cpp, apply_feature_functors.cpp; "
     "modules/cycle_route_lines/drape_frontend/cycle_routes.hpp"},

    {Relief, G::Layers, "GroveRelief", true, false, "Shaded relief and elevation tints, as in Guru Maps.",
     "map/framework.cpp, drape_frontend/frontend_renderer.cpp, drape/drape_global.hpp; modules/relief/map/relief.cpp, "
     "modules/core/drape_frontend/raster_layers.hpp"},
    {Landcover, G::Layers, "GroveLandcover", true, false,
     "Forests, fields and ice from ESA WorldCover when zoomed out, where the maps have none.",
     "map/framework.cpp, drape_frontend/frontend_renderer.cpp, drape/drape_global.hpp; "
     "modules/landcover/map/landcover.cpp"},
    {Landuse11, G::Layers, "GroveLanduse11", true, true, "The maps' own forests and fields from zoom 11, not 12.",
     "map/framework.cpp; modules/landuse_11/map/landcover_reading.hpp"},
    {OfflineLayers, G::Layers, "GroveOfflineLayers", true, true,
     "Relief and land cover saved with each downloaded map, over Wi-Fi.",
     "map/framework.cpp, map/raster_tile_provider.cpp; modules/offline_layers/map/offline_layers.cpp"},
    {Contours, G::Layers, "GroveContours", true, true, "Contour lines on until switched off, as in Guru Maps.",
     "map/framework.cpp (LoadIsolinesEnabled)"},

    {Logos, G::Places, "GroveLogos", true, true,
     "Chains' logos on the map from zoom 14, and a map button to hide them (upstream's help and donate button).",
     "drape_frontend/tile_info.cpp, poi_symbol_shape.cpp, apply_feature_functors.cpp, drape/overlay_tree.cpp, "
     "texture_manager.cpp, map/framework.cpp, android MapButtonsController.java; "
     "modules/logos/drape_frontend/brand_layer.cpp, "
     "modules/logos/map/brand_places.cpp"},
    {Elevation, G::Places, "GroveElevation", true, false, "The height of a tapped point on its card.",
     "android PlacePageView.java, place_page_latlon.xml; modules/elevation/map/elevation.cpp"},
    {Reviews, G::Places, "GroveReviews", true, false,
     "Mangrove's open reviews on place cards, with a trend when a place got better or worse.",
     "android PlacePageView.java, place_page_details.xml; modules/reviews/map/reviews.cpp"},
    {Tripadvisor, G::Places, "GroveTripadvisor", true, false,
     "Tripadvisor ratings and reviews on place cards, with your own API key.",
     "android PlacePageView.java, place_page_details.xml; modules/tripadvisor/map/tripadvisor.cpp"},
    {SearchColors, G::Places, "GroveSearchColors", true, false, "Search categories in the colours of their map icons.",
     "android CategoriesAdapter.java; GroveCategoryIcons.java"},
    {PlaceTitleColor, G::Places, "GrovePlaceTitleColor", true, false,
     "A place card's title in the place's colour: its chain's logo colour or its category's.",
     "android PlacePageView.java; GrovePlaceCard.java, modules/place_title_color/map/place_color.cpp"},

    {TiltGesture, G::Gestures, "GroveTilt", true, false,
     "Two fingers sliding up or down tilt the map into 3D, at any zoom.",
     "drape_frontend/user_event_stream.cpp, navigator.cpp; modules/tilt_gesture/drape_frontend/gestures.hpp"},
    {MeasureTap, G::Gestures, "GroveMeasure", true, false,
     "A two-finger tap shows the distance between the fingers, instead of zooming out.",
     "drape_frontend/frontend_renderer.cpp, android MapButtonsController.java; modules/measure_tap/map/measure.cpp"},

    {MergeJogs, G::Navigation, "GroveMergeJogs", true, true,
     "No \"turn left, turn right\" where you go straight: jogs of bike paths, crossings and squares merged.",
     "routing/index_router.cpp, route.hpp; modules/merge_jogs/routing/turns.cpp"},
    {WayKinds, G::Navigation, "GroveWayKinds", true, false,
     "The voice names bike paths, paths, stairs and tracks a turn leads onto.",
     "routing/route.cpp, directions_engine.cpp, routing_session.cpp, turns_tts_text.cpp; "
     "modules/way_kinds/routing/way_kind.cpp"},
    {QuietRecalculating, G::Navigation, "GroveQuietRecalc", true, false, "\"Recalculating\" once a minute at most.",
     "routing/routing_session.cpp"},
    {BikeHeading, G::Navigation, "GroveBikeHeading", true, true, "A rebuilt bike route starts the way the rider goes.",
     "routing/routing_settings.cpp"},
    {PreferCycleRoutes, G::Navigation, "GroveCycleRoutes", true, false,
     "Bike routes prefer signed cycle routes, for a small detour at most.",
     "routing/geometry.cpp; modules/prefer_cycle_routes/routing/cycle_routes.cpp"},
    {NavigationColors, G::Navigation, "GroveNavigationColors", false, true,
     "Grove's colours while driving, instead of Organic Maps' muted ones.", "indexer/map_style_reader.cpp"},
    {SavedTrips, G::Navigation, "GroveTrips", true, false,
     "A saved route keeps its stops, and can be navigated again from its track.",
     "map/routing_manager.cpp, bookmark_manager.cpp, android place_page_preview.xml; GroveTripRow.java"},

    {UiFont, G::App, "GroveUiFont", true, true, "Geist as the app's font, instead of Roboto.",
     "android res/layout, res/values (fontFamily); values/grove_fonts.xml"},
    {SettingsSections, G::App, "GroveSettingsSections", true, false,
     "Settings in illustrated sections, instead of one long list.",
     "android prefs_main.xml, SettingsPrefsFragment.java; GroveSettings.java"},
    {PerformanceBoost, G::App, "GrovePerformance", false, true,
     "More threads read map tiles (all cores but two, 3 to 6) and make land cover (4, not 2).",
     "drape_frontend/read_manager.cpp; modules/landcover/map/landcover.cpp, "
     "modules/performance_boost/platform/performance.hpp"},
};
static_assert(std::size(kFeatures) == static_cast<size_t>(Feature::Count));

std::string_view constexpr kStockKey = "GroveStock";
// The look is a choice of names: "organicmaps" is Organic Maps' own.
std::string_view constexpr kClassicLook = "organicmaps";

uint64_t Bit(Feature feature)
{
  return uint64_t{1} << static_cast<unsigned>(feature);
}

bool ReadSwitch(FeatureInfo const & info)
{
  if (info.m_feature == Look)
  {
    std::string look;
    settings::TryGet(info.m_key, look);
    return look != kClassicLook;
  }
  bool on = info.m_default;
  settings::TryGet(info.m_key, on);
  return on;
}

uint64_t ReadSwitches()
{
  uint64_t bits = 0;
  for (auto const & info : kFeatures)
    if (ReadSwitch(info))
      bits |= Bit(info.m_feature);
  return bits;
}

bool ReadStock()
{
  bool stock = false;
  settings::TryGet(kStockKey, stock);
  return stock;
}

struct State
{
  // As at start, for features that need a restart and Stock.
  uint64_t const m_atStart = ReadSwitches();
  bool const m_stock = ReadStock();
  std::atomic<uint64_t> m_live{m_atStart};
};

State & GetState()
{
  static State state;
  return state;
}
}  // namespace

std::span<FeatureInfo const> Features()
{
  return kFeatures;
}

FeatureInfo const & Info(Feature feature)
{
  ASSERT_LESS(feature, Feature::Count, ());
  auto const & info = kFeatures[static_cast<size_t>(feature)];
  ASSERT_EQUAL(info.m_feature, feature, ());
  return info;
}

bool IsOn(Feature feature)
{
  auto const & state = GetState();
  if (state.m_stock)
    return false;
  return (Info(feature).m_restart ? state.m_atStart : state.m_live.load(std::memory_order_relaxed)) & Bit(feature);
}

bool IsSwitchedOn(Feature feature)
{
  return GetState().m_live.load(std::memory_order_relaxed) & Bit(feature);
}

void SetSwitch(Feature feature, bool on)
{
  auto const & info = Info(feature);
  if (feature == Look)
    settings::Set(info.m_key, std::string(on ? "grove" : kClassicLook));
  else
    settings::Set(info.m_key, on);
  if (on)
    GetState().m_live.fetch_or(Bit(feature), std::memory_order_relaxed);
  else
    GetState().m_live.fetch_and(~Bit(feature), std::memory_order_relaxed);
}

bool IsStock()
{
  return GetState().m_stock;
}

bool IsStockSaved()
{
  return ReadStock();
}

void SetStock(bool stock)
{
  settings::Set(kStockKey, stock);
}

std::string DebugPrint(Feature feature)
{
  return std::string(Info(feature).m_key);
}
}  // namespace grove
