#pragma once

#include <od/graphics/FrameBuffer.h>

#include <graphics/composites/FollowableValue.h>
#include <graphics/composites/Text.h>

namespace graphics {
  class FollowableText {
    public:
      inline FollowableText(FollowableValue &value) :
        mValue(value) { }

      inline FollowableText(FollowableValue &value, int size) :
          mValue(value) {
        setSize(size);
      }

      void setPrecision(int precision) {
        mForceRefresh = true;
        mPrecision    = precision;
      }

      void setClear(bool clear) {
        mText.setClear(clear);
      }

      void setOutline(bool outline) {
        mText.setOutline(outline);
      }

      void setSize(int size) {
        mText.setSize(size);
      }

      inline Box draw(
        od::FrameBuffer &fb,
        od::Color color,
        const Box &world,
        JustifyAlign justifyAlign
      ) {
        prepareToSuppressZeros();
        refresh();
        return mText.draw(fb, color, world, justifyAlign);
      }

    private:
      inline void prepareToSuppressZeros() {
        if (mTimeToSuppressZeros > 0) {
          mTimeToSuppressZeros--;
          mForceRefresh = suppressZeros();
        }
      }

      inline bool suppressZeros() {
        return mTimeToSuppressZeros == 0;
      }

      inline void refresh() {
        bool isRefreshed = mValue.refresh();
        if (isRefreshed) mTimeToSuppressZeros = GRAPHICS_REFRESH_RATE;

        bool shouldRefresh = mForceRefresh || isRefreshed;
        if (!shouldRefresh) return;

        mForceRefresh = false;
        mText.setText(mValue.toString(mPrecision, suppressZeros()));
      }

      FollowableValue &mValue;
      Text  mText { "" };

      bool  mForceRefresh = true;
      int   mTimeToSuppressZeros = 0;
      int   mPrecision = 0;
  };
}