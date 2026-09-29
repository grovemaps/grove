package app.organicmaps.sdk;

// Grove: routes saved as tracks keep their stops, see RoutingManager::GroveTripOfTrack.
public final class GroveTrips
{
  private GroveTrips() {}

  // The router of a track saved from a route with its stops (Router's values), or -1.
  public static native int nativeGetRouter(long trackId);

  // Makes the track's stops the saved route points; RoutingController.restoreRoute() then plans them.
  public static native boolean nativeRestore(long trackId);
}
