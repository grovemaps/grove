package app.organicmaps.sdk;

// Grove: the selected place's colour (its chain's logo colour or its category colour), see
// modules/place_title_color/map/place_color.hpp.
public final class GrovePlace
{
  private GrovePlace() {}

  // 0xRRGGBB, or 0 when the place has no colour.
  public static native int nativeGetSelectedColor();
}
