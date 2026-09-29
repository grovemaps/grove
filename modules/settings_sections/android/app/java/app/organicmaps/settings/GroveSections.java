package app.organicmaps.settings;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.XmlRes;
import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;

// Grove[settings_sections]: settings in sections. The start page shows a card for each section, and a section's page
// only its group of preferences, headed by an illustration (GroveSectionPreference). Every page holds all of
// upstream's preferences (res/xml/grove_prefs_sections.xml), so upstream's setup finds them.
final class GroveSections
{
  static final String ARG_SECTION = "grove_section";

  private GroveSections() {}

  // The settings page: in sections, or upstream's list with the module off.
  @XmlRes
  static int xml(@XmlRes int upstream)
  {
    return GroveFeatures.isOn("settings_sections") ? R.xml.grove_prefs_sections : upstream;
  }

  static void show(@NonNull SettingsPrefsFragment fragment)
  {
    final PreferenceScreen screen = fragment.getPreferenceScreen();
    if (screen.findPreference("grove_section_map") == null)
      return;
    final Bundle args = fragment.getArguments();
    final String section = args == null ? null : args.getString(ARG_SECTION);
    final Preference profile = fragment.findPreference(fragment.getString(R.string.pref_osm_profile));
    if (profile != null)
      profile.setVisible(section == null);
    final Preference grove = screen.findPreference(GroveSettings.KEY);
    if (grove != null)
      grove.setVisible(section == null);
    // A section's group keeps upstream's category key where upstream has one.
    final String[][] sections = {{"map", "grove_group_map"},
                                 {"navigation", fragment.getString(R.string.pref_navigation)},
                                 {"privacy", fragment.getString(R.string.pref_privacy)},
                                 {"app", fragment.getString(R.string.pref_settings_general)},
                                 {"about", fragment.getString(R.string.pref_information)}};
    for (String[] s : sections)
    {
      final Preference card = screen.findPreference("grove_section_" + s[0]);
      final Preference group = screen.findPreference(s[1]);
      if (card != null)
      {
        card.setVisible(section == null);
        card.setOnPreferenceClickListener(preference -> {
          final Bundle sectionArgs = new Bundle();
          sectionArgs.putString(ARG_SECTION, s[0]);
          fragment.getSettingsActivity().stackFragment(SettingsPrefsFragment.class,
                                                       String.valueOf(preference.getTitle()), sectionArgs);
          return true;
        });
      }
      if (group != null)
        group.setVisible(s[0].equals(section));
    }
  }
}
