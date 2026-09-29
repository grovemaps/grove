package app.organicmaps.settings;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

// Grove[core]: an emoji as an icon, the modules' symbols from the registry.
final class EmojiDrawable extends Drawable
{
  private final String mEmoji;
  private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final int mSize;

  EmojiDrawable(@NonNull Context context, @NonNull String emoji)
  {
    this(context, emoji, 24);
  }

  EmojiDrawable(@NonNull Context context, @NonNull String emoji, int sizeDp)
  {
    mEmoji = emoji;
    mSize = Math.round(sizeDp * context.getResources().getDisplayMetrics().density);
    mPaint.setTextSize(mSize * 0.85f);
    mPaint.setTextAlign(Paint.Align.CENTER);
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    final Rect bounds = getBounds();
    final Paint.FontMetrics metrics = mPaint.getFontMetrics();
    final float baseline = bounds.exactCenterY() - (metrics.ascent + metrics.descent) / 2;
    canvas.drawText(mEmoji, bounds.exactCenterX(), baseline, mPaint);
  }

  @Override
  public int getIntrinsicWidth()
  {
    return mSize;
  }

  @Override
  public int getIntrinsicHeight()
  {
    return mSize;
  }

  @Override
  public void setAlpha(int alpha)
  {
    mPaint.setAlpha(alpha);
  }

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter)
  {}

  @Override
  public int getOpacity()
  {
    return PixelFormat.TRANSLUCENT;
  }
}
