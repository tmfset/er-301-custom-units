#pragma once

#include <od/objects/measurement/FifoProbe.h>
#include <od/graphics/Graphic.h>
#include <od/AudioThread.h>

namespace strike {
  class SimpleScope : public od::Graphic {
    public:
      SimpleScope(int left, int bottom, int width, int height, int color, bool fill) :
        od::Graphic(left, bottom, width, height) {
        mMax.resize(mWidth + 1);
        mMin.resize(mWidth + 1);
        mColor = color;
        mFill = fill;

        for (int i = 0; i < mWidth; i++) {
          mMax[i] = 0;
          mMin[i] = 0;
        }
      }

      virtual ~SimpleScope() {
        unwatch();
      }

      void watch(od::Outlet *pWatch) {
        unwatch();

        mpWatch = pWatch;
        mpWatch->attach();
        if (visible()) connect();
      }

      virtual void draw(od::FrameBuffer &fb) {
        Graphic::draw(fb);

        refresh();

        for (int i = 0; i < mWidth; i++) {
          drawSlice(fb, i);
        }
      }

      virtual void notifyHidden() {
        disconnect();
        Graphic::notifyHidden();
      }

      virtual void notifyVisible() {
        connect();
        Graphic::notifyVisible();
      }

    private:
      void drawSlice(od::FrameBuffer &fb, int i) {
        //auto color = GRAY5;
        auto center = mWorldBottom + (mHeight - 1) / 2.0f;

        auto x = mWorldLeft + i;
        auto max = center + mMax[i];
        auto min = center + mMin[i];

        if (mFill) {
          fb.vline(mColor - 12, x, center, min);
          fb.vline(mColor - 12, x, center, max);
          fb.vline(mColor - 12, x, min, max);
        } else {
          fb.vline(mColor - 6, x, min, max);
        }

        
        //fb.vline(mColor, x, min, center);
        fb.pixel(mColor, x, max);
        fb.pixel(mColor, x, min);
      }

      void refresh() {
        if (mRefreshCount == 0) {
          calculate();
        } else {
          mRefreshCount = (mRefreshCount + 1) % RefreshTime;
        }
      }

      void calculate() {
        if (!isConnected()) return;
        //mpProbe->setDecimation(16);

        auto size = (int)mpProbe->size();
        if (size == 0) return;

        auto signal = mpProbe->get(size);

        for (int i = 0; i < mWidth; i++) {
          mMax[i] = -mHeight;
          mMin[i] = mHeight;
        }

        auto halfSize = size / 2.0f;
        auto quarterSize = halfSize / 2.0f;

        auto xScale = mWidth / halfSize;
        auto yScale = mHeight / 2.0f;

        for (int i = 0; i < halfSize; i++) {
          auto x = (int)(i * xScale);
          auto y = (int)(signal[i + (int)quarterSize] * yScale);
          if (mMax[x] < y) mMax[x] = y;
          if (mMin[x] > y) mMin[x] = y;
        }

        // Remove gaps
        for (int i = 1; i < mWidth; i++) {
          mMax[i] = MAX(mMax[i], mMin[i - 1]);
          mMin[i] = MIN(mMin[i], mMax[i - 1]);
        }
      }

      bool visible() {
        return mVisibility == od::visibleState;
      }

      void connect() {
        if (isConnected()) return;
        if (!mpWatch) return;

        mpProbe = od::AudioThread::getFifoProbe();
        if (mpProbe) {
          od::AudioThread::connect(mpWatch, &mpProbe->mInput);
        }

        mRefreshCount = 0;
      }

      void disconnect() {
        if (!isConnected()) return;
        od::AudioThread::disconnect(&mpProbe->mInput);
        od::AudioThread::releaseFifoProbe(mpProbe);
        mpProbe = 0;
      }

      bool isConnected() {
        return mpProbe != 0;
      }

      void unwatch() {
        disconnect();

        if (mpWatch) {
          mpWatch->release();
          mpWatch = 0;
        }
      }

      od::Outlet *mpWatch = 0;
      od::FifoProbe *mpProbe = 0;
      std::vector<int> mMax;
      std::vector<int> mMin;
      int mRefreshCount = 0;
      int mColor = WHITE;
      bool mFill = false;
      static const int RefreshTime = 2;
  };
}