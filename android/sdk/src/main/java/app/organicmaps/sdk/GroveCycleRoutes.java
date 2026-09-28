package app.organicmaps.sdk;

// Grove: bicycle routing prefers cycle routes, see libs/routing/grove_cycle_routes.hpp.
public final class GroveCycleRoutes
{
  private GroveCycleRoutes() {}

  public static native boolean nativeIsEnabled();

  public static native void nativeSetEnabled(boolean enabled);
}
