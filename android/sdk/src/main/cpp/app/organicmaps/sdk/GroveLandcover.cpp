#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_landcover.hpp"

// Grove: the land cover switch, see libs/map/grove_landcover.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveLandcover_nativeIsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::landcover::IsEnabled());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveLandcover_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::landcover::SetEnabled(frm()->GetDrapeEngine(), static_cast<bool>(enabled));
}
}
