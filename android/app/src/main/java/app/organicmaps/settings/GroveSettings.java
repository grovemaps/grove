package app.organicmaps.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import androidx.preference.TwoStatePreference;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveCycleRoutes;
import app.organicmaps.sdk.GroveLandcover;
import app.organicmaps.sdk.GrovePerformance;
import app.organicmaps.sdk.GroveRelief;
import app.organicmaps.sdk.GroveReviews;
import app.organicmaps.sdk.GroveTripadvisor;

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

    final TwoStatePreference landcover = fragment.getPreference("GroveLandcover");
    landcover.setChecked(GroveLandcover.nativeIsEnabled());
    landcover.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveLandcover.nativeSetEnabled((Boolean) newValue);
      return true;
    });

    final TwoStatePreference cycleRoutes = fragment.getPreference("GroveCycleRoutes");
    cycleRoutes.setChecked(GroveCycleRoutes.nativeIsEnabled());
    cycleRoutes.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveCycleRoutes.nativeSetEnabled((Boolean) newValue);
      return true;
    });

    final TwoStatePreference performance = fragment.getPreference("GrovePerformance");
    performance.setChecked(GrovePerformance.nativeIsEnabled());
    performance.setOnPreferenceChangeListener((preference, newValue) -> {
      GrovePerformance.nativeSetEnabled((Boolean) newValue);
      new AlertDialog.Builder(fragment.requireContext())
          .setMessage(R.string.pref_performance_boost_restart)
          .setPositiveButton(R.string.restart, (dialog, which) -> restart(fragment.requireContext()))
          .setNegativeButton(R.string.later, null)
          .show();
      return true;
    });

    final EditTextPreference tripadvisor = fragment.getPreference("GroveTripadvisorKey");
    tripadvisor.setText(GroveTripadvisor.nativeGetKey());
    tripadvisor.setOnBindEditTextListener(editText -> {
      editText.setSingleLine(true);
      editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
    });
    tripadvisor.setSummaryProvider(preference -> {
      final String key = GroveTripadvisor.nativeGetKey();
      return key.isEmpty() ? fragment.getString(R.string.api_key_not_set)
                           : fragment.getString(R.string.api_key_set, key.substring(Math.max(0, key.length() - 4)));
    });
    tripadvisor.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveTripadvisor.nativeSetKey(((String) newValue).trim());
      return true;
    });

    final TwoStatePreference reviews = fragment.getPreference("GroveReviews");
    reviews.setChecked(GroveReviews.nativeIsEnabled());
    reviews.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveReviews.nativeSetEnabled((Boolean) newValue);
      return true;
    });
  }

  // Starts the app anew: the performance boost sizes thread pools at start.
  private static void restart(@NonNull Context context)
  {
    final Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
    if (launch == null)
      return;
    context.startActivity(Intent.makeRestartActivityTask(launch.getComponent()));
    Runtime.getRuntime().exit(0);
  }

  // Settings are grouped in sections: the start page shows a card for each, and a section's page only its group of
  // preferences, headed by an illustration. Every page holds all preferences, so upstream's setup finds them.
  static final String ARG_SECTION = "grove_section";
  private static final String[] SECTIONS = {"map", "navigation", "places", "privacy", "app", "about"};

  static void showSection(@NonNull SettingsPrefsFragment fragment)
  {
    final Bundle args = fragment.getArguments();
    final String section = args == null ? null : args.getString(ARG_SECTION);
    final PreferenceScreen screen = fragment.getPreferenceScreen();
    final Preference profile = fragment.findPreference(fragment.getString(R.string.pref_osm_profile));
    if (profile != null)
      profile.setVisible(section == null);
    for (String s : SECTIONS)
    {
      final Preference card = screen.findPreference("grove_section_" + s);
      final Preference group = screen.findPreference("grove_group_" + s);
      if (card != null)
      {
        card.setVisible(section == null);
        card.setOnPreferenceClickListener(preference -> {
          final Bundle sectionArgs = new Bundle();
          sectionArgs.putString(ARG_SECTION, s);
          fragment.openSection(String.valueOf(preference.getTitle()), sectionArgs);
          return true;
        });
      }
      if (group != null)
        group.setVisible(s.equals(section));
    }
  }
}
