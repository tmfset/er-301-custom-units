#pragma once

#include <string>

#include <od/graphics/FrameBuffer.h>

#include <graphics/primitives/all.h>

namespace graphics {
  class Text {
    public:
      inline Text(std::string str) {
        mRefreshDimensions = true;
        mText              = str;
      }

      inline Text(std::string str, int size) {
        mRefreshDimensions = true;
        mText              = str;
        mSize              = size;
      }

      void setText(std::string str) {
        mRefreshDimensions = true;
        mText              = str;
      }

      void setSize(int size) {
        mRefreshDimensions = true;
        mSize              = size;
      }

      void setClear(bool clear) {
        mClear = clear;
      }

      void setOutline(bool outline) {
        mOutline = outline;
      }

      void setVertical(bool vertical) {
        mRefreshDimensions = true;
        mVertical          = vertical;
      }

      inline Box draw(
        od::FrameBuffer &fb,
        od::Color color,
        const Box &world,
        JustifyAlign justifyAlign
      ) {
        updateDimensions();

        auto _bounds = bounds(world, justifyAlign);
        if (mClear) _bounds.clear(fb);

        if (mVertical) fb.vtext(color, _bounds.left(), _bounds.bottom(), mText.c_str(), mSize);
        else fb.text(color, _bounds.left(), _bounds.bottom(), mText.c_str(), mSize);

        if (mOutline) _bounds.outline(fb, WHITE, 2);
        return _bounds;
      }

    private:
      inline Box bounds(const Box &world, JustifyAlign justifyAlign) const {
        auto dimensions = mVertical ? mDimensions.swap() : mDimensions;
        return Box::wh(dimensions).justifyAlign(world, justifyAlign);
      }

      inline void updateDimensions() {
        if (!mRefreshDimensions) return;
        mRefreshDimensions = false;

        int width, height;
        od::getTextSize(mText.c_str(), &width, &height, mSize);
        mDimensions = v2d::of(width, height);
      }

      std::string mText;

      int  mSize     = 10;
      bool mClear    = true;
      bool mOutline  = false;
      bool mVertical = false;

      bool mRefreshDimensions = true;
      v2d  mDimensions;
  };
}
