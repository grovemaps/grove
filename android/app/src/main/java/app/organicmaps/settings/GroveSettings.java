package app.organicmaps.settings;

import androidx.annotation.NonNull;
import androidx.preference.TwoStatePreference;
import app.organicmaps.sdk.GroveRelief;
import app.organicmaps.sdk.GroveReviews;

// Grove's own settings, kept out of upstream's SettingsPrefsFragment.
final class GroveSettings
{
  private GroveSettings() {}

  static void init(@NonNull SettingsPrefsFragment fragment)
  {
    final TwoStatePreference relief = fragment.getPreference("GroveRelief");
    relief.setChecked(GroveRelief.nativeIsEnabled());
    relief.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveRelief.nativeSetEnabled((Boolean) newValue);
      return true;
    });

    final TwoStatePreference reviews = fragment.getPreference("GroveReviews");
    reviews.setChecked(GroveReviews.nativeIsEnabled());
    reviews.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveReviews.nativeSetEnabled((Boolean) newValue);
      return true;
    });
  }
}
