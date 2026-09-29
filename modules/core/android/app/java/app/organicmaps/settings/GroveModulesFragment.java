package app.organicmaps.settings;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceScreen;
import androidx.preference.SwitchPreferenceCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveFeatures;
import java.util.ArrayList;
import java.util.List;

// Grove[core]: every Grove module with its switch, by group, and Stock on top, for plain Organic Maps. The ⓘ of a
// module (GroveModuleSheet) tells what it does and where its code is. The registry is
// modules/core/platform/features.hpp; nothing here is written per module.
public class GroveModulesFragment extends BaseXmlSettingsFragment
{
  private final List<GroveModulePreference> mModules = new ArrayList<>();

  @Override
  protected int getXmlResources()
  {
    return R.xml.grove_modules;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    final Context context = requireContext();
    final PreferenceScreen screen = getPreferenceScreen();

    final SwitchPreferenceCompat stock = new SwitchPreferenceCompat(context);
    stock.setKey("GroveStock");
    stock.setPersistent(false);
    stock.setIcon(new EmojiDrawable(context, "🍃"));
    stock.setTitle("Organic Maps");
    stock.setSummary(R.string.grove_stock_summary);
    stock.setChecked(GroveFeatures.nativeIsStockSaved());
    stock.setOnPreferenceChangeListener((preference, newValue) -> {
      GroveFeatures.nativeSetStock((Boolean) newValue);
      updateEnabled((Boolean) newValue);
      GroveSettings.offerRestart(context);
      return true;
    });
    screen.addPreference(stock);

    PreferenceCategory group = null;
    String groupName = null;
    for (String module : GroveFeatures.nativeModules())
    {
      final String[] texts = GroveFeatures.nativeTexts(module);
      if (!texts[0].equals(groupName))
      {
        groupName = texts[0];
        group = new PreferenceCategory(context);
        group.setTitle(texts[1] + "  " + groupName);
        group.setIconSpaceReserved(true);
        screen.addPreference(group);
      }
      final GroveModulePreference pref = new GroveModulePreference(context, module, texts[2], title(module));
      group.addPreference(pref);
      mModules.add(pref);
    }
    updateEnabled(stock.isChecked());
  }

  private void updateEnabled(boolean stock)
  {
    for (GroveModulePreference pref : mModules)
      pref.setEnabled(!stock);
  }

  // "buildings_3d" -> "Buildings 3D", as modules/README.md names them.
  @NonNull
  static String title(@NonNull String module)
  {
    final String words = module.replace('_', ' ');
    return (Character.toUpperCase(words.charAt(0)) + words.substring(1)).replace("Poi ", "POI ").replace(" 3d", " 3D");
  }
}
