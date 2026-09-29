package app.organicmaps.sdk;

// Grove: shaded relief (hillshading) over the map, see modules/relief/map/relief.hpp.
public final class GroveRelief
{
  private GroveRelief() {}

  public static native boolean nativeIsEnabled();

  public static native void nativeSetEnabled(boolean enabled);
}
