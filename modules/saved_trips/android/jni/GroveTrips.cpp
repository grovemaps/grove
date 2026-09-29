#include "app/organicmaps/sdk/Framework.hpp"

// Grove: routes saved as tracks keep their stops, see RoutingManager::GroveTripOfTrack.
extern "C"
{
JNIEXPORT jint Java_app_organicmaps_sdk_GroveTrips_nativeGetRouter(JNIEnv *, jclass, jlong trackId)
{
  auto const router = frm()->GetRoutingManager().GroveTripOfTrack(static_cast<kml::TrackId>(trackId));
  return router ? static_cast<jint>(*router) : -1;
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_GroveTrips_nativeRestore(JNIEnv *, jclass, jlong trackId)
{
  return static_cast<jboolean>(frm()->GetRoutingManager().GroveRestoreTrip(static_cast<kml::TrackId>(trackId)));
}
}
