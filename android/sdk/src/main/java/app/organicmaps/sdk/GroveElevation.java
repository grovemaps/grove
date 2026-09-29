package app.organicmaps.sdk;

import androidx.annotation.WorkerThread;

// Grove: the height of any point, see modules/elevation/map/elevation.hpp.
public final class GroveElevation
{
  private GroveElevation() {}

  // Meters above sea level, or NaN when unknown. May download a tile: call off the main thread.
  @WorkerThread
  public static native double nativeGet(double lat, double lon);
}
