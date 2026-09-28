#include "routing/grove_cycle_routes.hpp"

#include "routing_common/bicycle_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/route_relation.hpp"

#include "platform/settings.hpp"

#include <algorithm>
#include <atomic>

namespace grove
{
namespace
{
std::string_view constexpr kSettingsKey = "GroveCycleRoutes";

std::atomic<bool> & Prefer()
{
  static std::atomic<bool> prefer = []
  {
    bool value = true;
    settings::TryGet(kSettingsKey, value);
    return value;
  }();
  return prefer;
}
}  // namespace

bool PreferCycleRoutes()
{
  return Prefer();
}

void SetPreferCycleRoutes(bool prefer)
{
  settings::Set(kSettingsKey, prefer);
  Prefer() = prefer;
}

double CycleRouteFactor(std::string_view network)
{
  return network == "icn" || network == "ncn" || network == "rcn" ? 1.2 : 1.1;
}

void ApplyCycleRoutes(routing::VehicleModelInterface const & model, FeatureType & feature, routing::SpeedKMpH & forward,
                      routing::SpeedKMpH & backward)
{
  if (!PreferCycleRoutes() || !dynamic_cast<routing::BicycleModel const *>(&model))
    return;

  double factor = 1;
  for (uint32_t relID : feature.GetRelations())
  {
    auto rel = feature.ReadRelation(relID);
    if (rel.GetType() == feature::RouteRelationBase::Type::Bicycle)
      factor = std::max(factor, CycleRouteFactor(rel.GetRel().GetNetwork()));
  }
  if (factor == 1)
    return;

  double const maxSpeed = model.GetMaxWeightSpeed();
  for (auto * speed : {&forward, &backward})
    speed->m_weight = std::max(speed->m_weight, std::min(speed->m_weight * factor, maxSpeed));
}
}  // namespace grove
