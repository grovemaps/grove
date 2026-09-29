#include <jni.h>

#include "modules/elevation/map/elevation.hpp"

#include <limits>

// Grove: the height of any point, see modules/elevation/map/elevation.hpp.
extern "C"
{
JNIEXPORT jdouble Java_app_organicmaps_sdk_GroveElevation_nativeGet(JNIEnv *, jclass, jdouble lat, jdouble lon)
{
  return grove::GetElevation({lat, lon}).value_or(std::numeric_limits<double>::quiet_NaN());
}
}
