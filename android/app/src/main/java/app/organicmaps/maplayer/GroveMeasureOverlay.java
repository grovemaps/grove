package app.organicmaps.maplayer;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.GroveMeasure;
import app.organicmaps.util.UiUtils;

// Grove: after a two-finger tap, a line between the fingers with the distance in its middle, as in Guru Maps. It
// fades out after a few seconds and lets touches through.
final class GroveMeasureOverlay extends View implements GroveMeasure.Listener
{
  private static final long SHOW_MS = 3000;
  private static final long FADE_MS = 400;

  private final Paint mLine = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mDot = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mLabel = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mLabelText = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final int[] mLocation = new int[2];
  private float mX1, mY1, mX2, mY2;
  private String mDistance = "";

  private GroveMeasureOverlay(@NonNull Context context)
  {
    super(context);
    final float dp = getResources().getDisplayMetrics().density;
    mLine.setColor(Color.rgb(32, 32, 36));
    mLine.setStrokeWidth(2.5f * dp);
    mLine.setStrokeCap(Paint.Cap.ROUND);
    mDot.setColor(Color.rgb(32, 32, 36));
    mLabel.setColor(Color.WHITE);
    mLabel.setShadowLayer(3 * dp, 0, dp, Color.argb(80, 0, 0, 0));
    mLabelText.setColor(Color.rgb(32, 32, 36));
    mLabelText.setTextSize(15 * dp);
    mLabelText.setFakeBoldText(true);
    mLabelText.setTextAlign(Paint.Align.CENTER);
    setAlpha(0);
    setClickable(false);
  }

  // Adds the overlay under the map buttons and starts listening; call from the buttons fragment.
  static void attach(@NonNull ViewGroup frame)
  {
    final GroveMeasureOverlay overlay = new GroveMeasureOverlay(frame.getContext());
    frame.addView(overlay, 0,
                  new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    GroveMeasure.nativeSetListener(overlay);
  }

  static void detach()
  {
    GroveMeasure.nativeSetListener(null);
  }

  @Override
  public void onDistance(float x1, float y1, float x2, float y2, @NonNull String distance)
  {
    // The map surface fills the window; this view may be inset.
    getLocationInWindow(mLocation);
    mX1 = x1 - mLocation[0];
    mY1 = y1 - mLocation[1];
    mX2 = x2 - mLocation[0];
    mY2 = y2 - mLocation[1];
    mDistance = distance;
    UiUtils.show(this);
    animate().cancel();
    setAlpha(1);
    animate().alpha(0).setStartDelay(SHOW_MS).setDuration(FADE_MS);
    invalidate();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    if (mDistance.isEmpty())
      return;
    final float dp = getResources().getDisplayMetrics().density;
    canvas.drawLine(mX1, mY1, mX2, mY2, mLine);
    canvas.drawCircle(mX1, mY1, 5 * dp, mDot);
    canvas.drawCircle(mX2, mY2, 5 * dp, mDot);

    final float cx = (mX1 + mX2) / 2;
    final float cy = (mY1 + mY2) / 2;
    final float halfWidth = mLabelText.measureText(mDistance) / 2 + 10 * dp;
    final float halfHeight = 14 * dp;
    canvas.drawRoundRect(new RectF(cx - halfWidth, cy - halfHeight, cx + halfWidth, cy + halfHeight), halfHeight,
                         halfHeight, mLabel);
    canvas.drawText(mDistance, cx, cy - (mLabelText.ascent() + mLabelText.descent()) / 2, mLabelText);
  }
}
