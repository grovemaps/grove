#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"

#include "map/grove_place_color.hpp"

// Grove: the colour of the selected place's card, see libs/map/grove_place_color.hpp.
extern "C"
{
JNIEXPORT jint Java_app_organicmaps_sdk_GrovePlace_nativeGetSelectedColor(JNIEnv *, jclass)
{
  auto * f = frm();
  if (!f->HasPlacePageInfo())
    return 0;
  return static_cast<jint>(grove::GetPlaceColor(f->GetDataSource(), f->GetCurrentPlacePageInfo().GetID()));
}
}
