package app.organicmaps.sdk;

import androidx.annotation.NonNull;

// Grove: the map's look, "grove" or "organicmaps" (Organic Maps' own styles and icons); applied after a restart. See
// libs/indexer/map_style_reader.cpp.
public final class GroveLook
{
  private GroveLook() {}

  @NonNull
  public static native String nativeGet();

  public static native void nativeSet(@NonNull String look);
}
