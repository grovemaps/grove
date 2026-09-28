#include <jni.h>

#include "routing/grove_cycle_routes.hpp"

// Grove: the cycle routes switch, see libs/routing/grove_cycle_routes.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveCycleRoutes_nativeIsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::PreferCycleRoutes());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveCycleRoutes_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::SetPreferCycleRoutes(static_cast<bool>(enabled));
}
}
