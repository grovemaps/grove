package app.organicmaps.sdk;

// Grove: the performance boost switch, see libs/platform/grove_performance.hpp. Takes effect after a restart.
public final class GrovePerformance
{
  private GrovePerformance() {}

  public static native boolean nativeIsEnabled();

  public static native void nativeSetEnabled(boolean enabled);
}
