package app.organicmaps.sdk;

// Grove: the map's brands button, see modules/logos/map/brand_places.hpp.
public final class GroveBrands
{
  private GroveBrands() {}

  public static native boolean nativeAreShown();

  public static native void nativeSetShown(boolean shown);
}
