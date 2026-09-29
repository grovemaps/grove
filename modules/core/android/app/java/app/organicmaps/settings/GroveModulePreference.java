package app.organicmaps.settings;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceViewHolder;
import androidx.preference.SwitchPreferenceCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;

// Grove[core]: a module's row on Grove's page: its symbol, name, ⓘ and switch. A module read at start shows ↻.
final class GroveModulePreference extends SwitchPreferenceCompat
{
  private final String mModule;

  GroveModulePreference(@NonNull Context context, @NonNull String module, @NonNull String icon, @NonNull String title)
  {
    super(context);
    mModule = module;
    setKey("grove_module_" + module);
    setPersistent(false);
    setIcon(new EmojiDrawable(context, icon));
    final boolean restart = GroveFeatures.nativeNeedsRestart(module);
    setTitle(restart ? title + "  ↻" : title);
    setWidgetLayoutResource(R.layout.grove_module_widget);
    setChecked(GroveFeatures.nativeIsSwitchedOn(module));
    setOnPreferenceChangeListener((preference, newValue) -> {
      GroveFeatures.nativeSetSwitch(module, (Boolean) newValue);
      if (restart)
        GroveSettings.offerRestart(context);
      return true;
    });
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);
    holder.findViewById(R.id.grove_module_info).setOnClickListener(v -> GroveModuleSheet.show(getContext(), mModule));
  }
}
