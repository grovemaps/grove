#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace grove
{
// Every change Grove makes to Organic Maps, each behind its own switch. Each hook in an upstream file checks its
// feature, and runs upstream's own code when it is off: with every switch off, or Stock on, only upstream's code runs
// (GROVE.md, "Features").
enum class Feature : uint8_t
{
  // Map
  Look,
  Typography,
  Halos,
  Buildings3d,
  SeeThroughBuildings,
  PoiDots,
  CycleRouteLines,
  // Layers
  Relief,
  Landcover,
  Landuse11,
  OfflineLayers,
  Contours,
  // Places
  Logos,
  Elevation,
  Reviews,
  Tripadvisor,
  SearchColors,
  PlaceTitleColor,
  // Gestures
  TiltGesture,
  MeasureTap,
  // Navigation
  MergeJogs,
  WayKinds,
  QuietRecalculating,
  BikeHeading,
  PreferCycleRoutes,
  NavigationColors,
  SavedTrips,
  // App
  UiFont,
  SettingsSections,
  PerformanceBoost,

  Count
};

enum class FeatureGroup : uint8_t
{
  Map,
  Layers,
  Places,
  Gestures,
  Navigation,
  App,
};

struct FeatureInfo
{
  Feature m_feature;
  FeatureGroup m_group;
  std::string_view m_module;  // Its folder, modules/<module>/, and its name in the app's strings.
  std::string_view m_key;     // Settings key.
  bool m_default;             // The switch for new users.
  bool m_restart;             // Read once at start: a change takes effect after a restart.
  // One line; modules/<module>/README.md and data/grove_modules.txt tell where its code is.
  std::string_view m_about;
};

std::span<FeatureInfo const> Features();
FeatureInfo const & Info(Feature feature);

// Whether the feature runs: Grove isn't stock, and the feature's switch is on (as at start, for one that needs a
// restart).
bool IsOn(Feature feature);

// The saved switch, as the settings show it.
bool IsSwitchedOn(Feature feature);
void SetSwitch(Feature feature, bool on);

// Stock: every feature off, for plain Organic Maps. Read once at start.
bool IsStock();
bool IsStockSaved();
void SetStock(bool stock);

std::string DebugPrint(Feature feature);
}  // namespace grove
