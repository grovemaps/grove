#pragma once

#include <functional>
#include <string>

namespace grove
{
// Two-finger tap distance (modules/measure_tap/drape_frontend/measure.hpp): the platform gets the fingers' screen
// pixels and the formatted distance between them, on the UI thread.
using MeasureListener = std::function<void(float x1, float y1, float x2, float y2, std::string const & distance)>;

// Registers the drape side; call before the drape engine is created.
void InitMeasure();
void SetMeasureListener(MeasureListener listener);
}  // namespace grove
