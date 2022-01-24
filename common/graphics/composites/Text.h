#pragma once

#include <string>

#include <od/graphics/FrameBuffer.h>
#include <od/ui/DialState.h>
#include <graphics/primitives/all.h>

#include <ui/DialMap.h>

#include <util/Units.h>

namespace graphics {
  class Text {
    public:
      inline Text() { }
      inline Text(std::string str, int size) {
        update(str, size);
      }

      void setFontSize(int size)            { mSize         = size; }
      void setJustifyAlign(JustifyAlign ja) { mJustifyAlign = ja; }
      void setClear(bool clear)             { mClear        = clear; }
      void setOutline(bool outline)         { mOutline      = outline; }

      inline Box bounds(const Box &world) const {
        return Box::wh(mDimensions).justifyAlign(world, mJustifyAlign);
      }

      inline Box draw(
        od::FrameBuffer &fb,
        od::Color color,
        const Box &world
      ) const {
        auto _bounds = bounds(world);
        if (mClear) _bounds.clear(fb);
        fb.text(color, _bounds.left(), _bounds.bottom(), mValue.c_str(), mSize);
        if (mOutline) _bounds.outline(fb, WHITE, 2);
        return _bounds;
      }

      inline void update(std::string str) {
        update(str, mSize);
      }

      inline void update(std::string str, int size) {
        mValue = str;
        mSize = size;

        int width, height;
        od::getTextSize(mValue.c_str(), &width, &height, size);
        mDimensions = v2d::of(width, height);
      }

    private:
      std::string mValue;
      int mSize;
      v2d mDimensions;

      JustifyAlign mJustifyAlign = CENTER_MIDDLE;
      bool mClear = true;
      bool mOutline = false;
  };

  class ParameterDisplay {
    public:

      inline ParameterDisplay(od::Parameter *param) :
          mpParameter(param) {
        mpParameter->attach();
      }

      inline ~ParameterDisplay() {
        mpParameter->release();
      }

      inline float setValueInUnits(float v, bool hard) {
        auto fromUnits = util::convertFromUnits(v, mUnits);

        if (hard) mpParameter->hardSet(fromUnits);
        else      mpParameter->softSet(fromUnits);

        return fromUnits;
      }

      inline void setShowTarget(bool v) {
        mShowTarget = v;
      }

      inline void setUnits(util::Units v) {
        mUnits = v;
        mForceRefresh = true;
      }

      inline bool refresh() {
        auto current = currentValue();

        auto isChanged = mLastValue != current;
        auto shouldRefresh = mForceRefresh || isChanged;
        if (!shouldRefresh) return false;

        mForceRefresh = false;
        mLastValue = current;
        mLastValueInUnits = util::convertToUnits(mLastValue, mUnits);

        return true;
      }

      inline float lastValueInUnits() {
        return mLastValueInUnits;
      }

      inline std::string toString(int precision, bool suppressZeros) {
        return util::formatQuantity(mLastValueInUnits, mUnits, precision, suppressZeros);
      }

    private:
      inline float currentValue() {
        return mShowTarget ? mpParameter->target() : mpParameter->value();
      }

      od::Parameter *mpParameter;
      bool  mForceRefresh = false;
      float mLastValue = 0;
      float mLastValueInUnits = 0;

      bool mShowTarget = false;
      util::Units mUnits = util::unitNone;
  };

  class ParameterText {
    public:
      inline ParameterText(ParameterDisplay &display) :
        mDisplay(display) { }

      void setPrecision(int v) {
        mPrecision = v;
        mForceRefresh = true;
      }

      void setJustifyAlign(JustifyAlign ja) {
        mText.setJustifyAlign(ja);
      }

      void setClear(bool clear) {
        mText.setClear(clear);
      }

      void setOutline(bool outline) {
        mText.setOutline(outline);
      }

      void setFontSize(int size) {
        mText.setFontSize(size);
      }

      inline Box bounds(const Box &world) {
        return mText.bounds(world);
      }

      inline Box draw(od::FrameBuffer &fb, od::Color color, Box &world) {
        prepareToSuppressZeros();
        refresh();
        return mText.draw(fb, color, world);
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
        bool isRefreshed = mDisplay.refresh();
        if (isRefreshed) mTimeToSuppressZeros = GRAPHICS_REFRESH_RATE;

        bool shouldRefresh = mForceRefresh || isRefreshed;
        if (!shouldRefresh) return;

        mForceRefresh = false;
        mText.update(mDisplay.toString(mPrecision, suppressZeros()));
      }

      ParameterDisplay &mDisplay;

      Text  mText;
      bool  mForceRefresh = true;
      int   mTimeToSuppressZeros = 0;
      int   mPrecision = 0;
  };
}