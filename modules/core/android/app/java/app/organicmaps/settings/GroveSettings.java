package app.organicmaps.settings;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;
import app.organicmaps.R;

// Grove[core]: the entry to Grove's page (GroveModulesFragment) on upstream's settings page. It is there even with
// Stock on, to switch back.
final class GroveSettings
{
  static final String KEY = "grove_section_grove";

  private GroveSettings() {}

  static void init(@NonNull SettingsPrefsFragment fragment)
  {
    // A settings section's page (GroveSections) has no entry.
    if (fragment.getArguments() != null && fragment.getArguments().containsKey(GroveSections.ARG_SECTION))
      return;
    Preference entry = fragment.findPreference(KEY);
    if (entry == null)
    {
      entry = new Preference(fragment.requireContext());
      entry.setKey(KEY);
      entry.setTitle("Grove");
      entry.setSummary(R.string.grove_settings_grove_summary);
      entry.setIcon(R.drawable.ic_grove);
      entry.setOrder(0);
      entry.setPersistent(false);
      fragment.getPreferenceScreen().addPreference(entry);
    }
    entry.setOnPreferenceClickListener(preference -> {
      fragment.getSettingsActivity().stackFragment(GroveModulesFragment.class, "Grove", null);
      return true;
    });
  }

  // For switches read at start.
  static void offerRestart(@NonNull Context context)
  {
    new AlertDialog.Builder(context)
        .setMessage(R.string.pref_performance_boost_restart)
        .setPositiveButton(R.string.restart, (dialog, which) -> restart(context))
        .setNegativeButton(R.string.later, null)
        .show();
  }

  // Starts the app anew.
  private static void restart(@NonNull Context context)
  {
    final Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
    if (launch == null)
      return;
    context.startActivity(Intent.makeRestartActivityTask(launch.getComponent()));
    Runtime.getRuntime().exit(0);
  }
}
