#pragma once

#include "routing_common/vehicle_model.hpp"

class FeatureType;

namespace grove
{
// Bicycle routing prefers roads of signed cycle routes (network=icn, ncn, rcn, lcn: national routes, node networks,
// local routes): they are usually quieter and nicer than the roads around them. Such a road's weight speed is
// raised a little, so a route takes a short detour to use it and a far longer one never; its ETA speed is unchanged.
// Stronger factors (1.3) sent a ride across Amsterdam 48% further round the city's regional routes. Mountain bike
// trails get nothing. The settings switch is "GroveCycleRoutes", on by default, and applies from the next route.
bool PreferCycleRoutes();
void SetPreferCycleRoutes(bool prefer);

// Weight factor for a road in a cycle route of the network, 1 if none: icn/ncn/rcn 1.2, lcn and others 1.1.
double CycleRouteFactor(std::string_view network);

// Raises the weight speeds of a bicycle road in cycle routes, to at most the model's max weight speed (which keeps
// the A* heuristic admissible). Does nothing for other vehicles or with the switch off.
void ApplyCycleRoutes(routing::VehicleModelInterface const & model, FeatureType & feature, routing::SpeedKMpH & forward,
                      routing::SpeedKMpH & backward);
}  // namespace grove
