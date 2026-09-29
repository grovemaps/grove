package app.organicmaps.sdk;

import androidx.annotation.NonNull;

// Grove: the modules' switches, see modules/core/platform/features.hpp. A module is named by its folder,
// modules/<module>/; hooks in upstream's Android code check it with GroveFeatures.isOn("<module>").
public final class GroveFeatures
{
  private GroveFeatures() {}

  // Whether the module runs: Stock is off and its switch on (as at start, for one that needs a restart).
  public static boolean isOn(@NonNull String module)
  {
    return nativeIsOn(module);
  }

  // The modules in the registry's order.
  @NonNull
  public static native String[] nativeModules();

  // {group name, group icon, icon, about}.
  @NonNull
  public static native String[] nativeTexts(@NonNull String module);

  public static native boolean nativeNeedsRestart(@NonNull String module);

  public static native boolean nativeIsOn(@NonNull String module);

  // The saved switch, as the settings show it.
  public static native boolean nativeIsSwitchedOn(@NonNull String module);

  public static native void nativeSetSwitch(@NonNull String module, boolean on);

  // Stock: only Organic Maps' own code runs. Read at start.
  public static native boolean nativeIsStock();

  public static native boolean nativeIsStockSaved();

  public static native void nativeSetStock(boolean stock);
}
