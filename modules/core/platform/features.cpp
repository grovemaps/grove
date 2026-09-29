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
    {Look, G::Map, "look", "GroveLook", true, true,
     "Grove's map style and icons: Apple-like colours, Guru Maps' terrain, CoMaps' cartography. Off: Organic Maps' "
     "own."},
    {Typography, G::Map, "typography", "GroveTypography", true, true,
     "Inter for map labels: semibold places, italic water, spaced capitals for streets and areas, Geist numbers."},
    {Halos, G::Map, "halos", "GroveHalos", true, true, "Wider, softer label halos, like Mapy.com's white outlines."},
    {Buildings3d, G::Map, "buildings_3d", "GroveBuildings3d", true, false,
     "3D buildings lit from the upper left, walls darkening toward the ground, more solid."},
    {SeeThroughBuildings, G::Map, "see_through_buildings", "GroveSeeThrough", true, true,
     "3D buildings with a shop or café inside are see-through, as in Apple Maps."},
    {PoiDots, G::Map, "poi_dots", "GrovePoiDots", true, true, "A small dot where each place is, under its icon."},
    {CycleRouteLines, G::Map, "cycle_route_lines", "GroveCycleLines", true, true,
     "The cycling layer draws one solid line per road, coloured by its highest network, not a stripe per route."},

    {Relief, G::Layers, "relief", "GroveRelief", true, false, "Shaded relief and elevation tints, as in Guru Maps."},
    {Landcover, G::Layers, "landcover", "GroveLandcover", true, false,
     "Forests, fields and ice from ESA WorldCover when zoomed out, where the maps have none."},
    {Landuse11, G::Layers, "landuse_11", "GroveLanduse11", true, true,
     "The maps' own forests and fields from zoom 11, not 12."},
    {OfflineLayers, G::Layers, "offline_layers", "GroveOfflineLayers", true, true,
     "Relief and land cover saved with each downloaded map, over Wi-Fi."},
    {Contours, G::Layers, "contours", "GroveContours", true, true,
     "Contour lines on until switched off, as in Guru Maps."},

    {Logos, G::Places, "logos", "GroveLogos", true, true,
     "Chains' logos on the map from zoom 14, and a map button to hide them (upstream's help and donate button)."},
    {Elevation, G::Places, "elevation", "GroveElevation", true, false, "The height of a tapped point on its card."},
    {Reviews, G::Places, "reviews", "GroveReviews", true, false,
     "Mangrove's open reviews on place cards, with a trend when a place got better or worse."},
    {Tripadvisor, G::Places, "tripadvisor", "GroveTripadvisor", true, false,
     "Tripadvisor ratings and reviews on place cards, with your own API key."},
    {SearchColors, G::Places, "search_colors", "GroveSearchColors", true, false,
     "Search categories in the colours of their map icons."},
    {PlaceTitleColor, G::Places, "place_title_color", "GrovePlaceTitleColor", true, false,
     "A place card's title in the place's colour: its chain's logo colour or its category's."},

    {TiltGesture, G::Gestures, "tilt_gesture", "GroveTilt", true, false,
     "Two fingers sliding up or down tilt the map into 3D, at any zoom."},
    {MeasureTap, G::Gestures, "measure_tap", "GroveMeasure", true, false,
     "A two-finger tap shows the distance between the fingers, instead of zooming out."},

    {MergeJogs, G::Navigation, "merge_jogs", "GroveMergeJogs", true, true,
     "No \"turn left, turn right\" where you go straight: jogs of bike paths, crossings and squares merged."},
    {WayKinds, G::Navigation, "way_kinds", "GroveWayKinds", true, false,
     "The voice names bike paths, paths, stairs and tracks a turn leads onto."},
    {QuietRecalculating, G::Navigation, "quiet_recalculating", "GroveQuietRecalc", true, false,
     "\"Recalculating\" once a minute at most."},
    {BikeHeading, G::Navigation, "bike_heading", "GroveBikeHeading", true, true,
     "A rebuilt bike route starts the way the rider goes."},
    {PreferCycleRoutes, G::Navigation, "prefer_cycle_routes", "GroveCycleRoutes", true, false,
     "Bike routes prefer signed cycle routes, for a small detour at most."},
    {NavigationColors, G::Navigation, "navigation_colors", "GroveNavigationColors", false, true,
     "Grove's colours while driving, instead of Organic Maps' muted ones."},
    {SavedTrips, G::Navigation, "saved_trips", "GroveTrips", true, false,
     "A saved route keeps its stops, and can be navigated again from its track."},

    {UiFont, G::App, "ui_font", "GroveUiFont", true, true, "Geist as the app's font, instead of Roboto."},
    {SettingsSections, G::App, "settings_sections", "GroveSettingsSections", true, false,
     "Settings in illustrated sections, instead of one long list."},
    {PerformanceBoost, G::App, "performance_boost", "GrovePerformance", false, true,
     "More threads read map tiles (all cores but two, 3 to 6) and make land cover (4, not 2)."},
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
