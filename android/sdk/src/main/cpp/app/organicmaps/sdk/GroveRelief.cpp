#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_relief.hpp"

// Grove: the shaded relief switch, see libs/map/grove_relief.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveRelief_nativeIsEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::IsReliefEnabled());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveRelief_nativeSetEnabled(JNIEnv *, jclass, jboolean enabled)
{
  grove::SetReliefEnabled(frm()->GetDrapeEngine(), static_cast<bool>(enabled));
}
}
