#include <jni.h>

#include "map/grove_elevation.hpp"

#include <limits>

// Grove: the height of any point, see libs/map/grove_elevation.hpp.
extern "C"
{
JNIEXPORT jdouble Java_app_organicmaps_sdk_GroveElevation_nativeGet(JNIEnv *, jclass, jdouble lat, jdouble lon)
{
  return grove::GetElevation({lat, lon}).value_or(std::numeric_limits<double>::quiet_NaN());
}
}
