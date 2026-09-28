#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_brand_places.hpp"

// Grove: the map's brands button, see libs/map/grove_brand_places.hpp.
extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveBrands_nativeAreShown(JNIEnv *, jclass)
{
  return static_cast<jboolean>(grove::AreBrandsShown());
}

JNIEXPORT void Java_app_organicmaps_sdk_GroveBrands_nativeSetShown(JNIEnv *, jclass, jboolean shown)
{
  grove::SetBrandsShown(frm()->GetDrapeEngine(), static_cast<bool>(shown));
}
}
