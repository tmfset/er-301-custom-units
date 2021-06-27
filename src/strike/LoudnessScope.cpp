#include <LoudnessScope.h>
#include <od/AudioThread.h>
#include <od/config.h>
#include <hal/log.h>

namespace strike {
  void LoudnessScope::calculate() {
    int i, x, y, n;
    float dx, dy;
    float *values;

    if (mpProbe == 0) return;

    //mpProbe->setDecimation(16);

    n = (int)mpProbe->size();
    if (n == 0)
      return;
    values = mpProbe->get(n);

    int n2 = n / 2; // 8
    int n4 = n2 / 2; // 4

    int imin = n2 - n4; // 4
    int imax = imin + n2; // 12

    for (i = 0; i < mWidth; i++)
    {
      mMaximums[i] = -mHeight;
      mMinimums[i] = mHeight;
    }

    dx = (mWidth) / (float)n2; // w / 8
    dy = (mHeight - 1) / 2.0f;
    for (i = imin; i < imax; i++)
    {
      x = (int)((i - imin) * dx);
      //y = (int)((mOffset + values[i]) * dy);
      y = (int)(dy * (60.0f + values[i]) / 72.0f);
      if (mMaximums[x] < y)
      {
        mMaximums[x] = y;
      }
      if (mMinimums[x] > y)
      {
        mMinimums[x] = y;
      }
    }

    // Remove any gaps
    for (i = 1; i < mWidth; i++)
    {
      mMaximums[i] = MAX(mMaximums[i], mMinimums[i - 1]);
      mMinimums[i] = MIN(mMinimums[i], mMaximums[i - 1]);
    }
  }

  void LoudnessScope::draw(od::FrameBuffer &fb) {
    Graphic::draw(fb);

    // if (mLeftBorder) {
      
    // }

    // fb.vline(mBorderColor, mWorldLeft, mWorldBottom,
    //            mWorldBottom + mHeight);

    // fb.vline(mBorderColor, mWorldLeft + mWidth - 1, mWorldBottom,
    //            mWorldBottom + mHeight);

    // if (mRightBorder) {
    //   fb.vline(mBorderColor, mWorldLeft + mWidth - 1, mWorldBottom,
    //            mWorldBottom + mHeight);
    // }

    int i, x0, x1, y0, y1;
    int mid = mWorldBottom + mHeight / 2;

    //x0 = mWorldLeft;
    //x1 = x0 + mWidth - 1;

    //y0 = mid;
    //fb.hline(GRAY3, x0, x1, y0);
    //y0 = mWorldBottom + mHeight / 4;
    //fb.hline(GRAY3, x0, x1, y0, 2);
   // y0 = mWorldBottom + (3 * mHeight) / 4;
    //fb.hline(GRAY3, x0, x1, y0, 2);

    if (mpProbe == 0) {
      if (mShowStatus) {
        fb.text(WHITE, mWorldLeft + mWidth / 2 - 5, mid + 4, "No", 10);
        fb.text(WHITE, mWorldLeft + mWidth / 2 - 10, mid - 10, "Signal",
                10);
      }
      return;
    }

    if (mCalculateCount == RefreshTime) {
      calculate();
      mCalculateCount = 0;
    } else {
      mCalculateCount++;
    }

    od::Color color;
    if (fb.mIsMonoChrome) {
      color = WHITE;
    } else {
      color = GRAY3;
    }
    for (i = 0; i < mWidth; i++) {
      x0 = mWorldLeft + i;
      y0 = mid + mMinimums[i];
      y1 = mid + mMaximums[i];
      //fb.vline(color, x0, y0, y1);
      fb.vline(color, x0, y0, mWorldBottom);
      fb.pixel(mForeground, x0, y0);
      //fb.pixel(mForeground, x0, y1);
    }
  }
}
