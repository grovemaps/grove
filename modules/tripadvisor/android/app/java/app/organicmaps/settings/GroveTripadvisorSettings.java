package app.organicmaps.settings;

import android.content.Context;
import android.text.InputType;
import android.widget.EditText;
import android.widget.FrameLayout;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import app.organicmaps.R;
import app.organicmaps.sdk.GroveTripadvisor;

// Grove[tripadvisor]: the user's own Tripadvisor API key, the module's ⚙ on Grove's page (GroveModuleSheet).
@Keep
public final class GroveTripadvisorSettings
{
  private GroveTripadvisorSettings() {}

  @Keep
  public static void open(@NonNull Context context)
  {
    final EditText input = new EditText(context);
    input.setSingleLine(true);
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
    input.setText(GroveTripadvisor.nativeGetKey());
    final FrameLayout frame = new FrameLayout(context);
    final int pad = Math.round(20 * context.getResources().getDisplayMetrics().density);
    frame.setPadding(pad, 0, pad, 0);
    frame.addView(input);
    new AlertDialog.Builder(context)
        .setTitle(R.string.tripadvisor_api_key)
        .setMessage(R.string.tripadvisor_api_key_summary)
        .setView(frame)
        .setPositiveButton(android.R.string.ok,
                           (dialog, which) -> GroveTripadvisor.nativeSetKey(input.getText().toString().trim()))
        .setNegativeButton(android.R.string.cancel, null)
        .show();
  }
}
