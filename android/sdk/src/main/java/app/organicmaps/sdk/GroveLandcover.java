package app.organicmaps.sdk;

// Grove: land cover when zoomed out, see modules/landcover/map/landcover.hpp.
public final class GroveLandcover
{
  private GroveLandcover() {}

  public static native boolean nativeIsEnabled();

  public static native void nativeSetEnabled(boolean enabled);
}
