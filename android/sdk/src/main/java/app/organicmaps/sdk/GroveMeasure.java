package app.organicmaps.sdk;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

// Grove: a two-finger tap shows the distance between the fingers, see modules/measure_tap/map/measure.hpp.
public final class GroveMeasure
{
  private GroveMeasure() {}

  public interface Listener
  {
    // Screen pixels of the map surface; called on the UI thread.
    @Keep
    void onDistance(float x1, float y1, float x2, float y2, @NonNull String distance);
  }

  public static native void nativeSetListener(@Nullable Listener listener);
}
