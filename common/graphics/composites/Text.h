#pragma once

#include <string>
#include <od/graphics/FrameBuffer.h>
#include <graphics/primitives/all.h>

namespace graphics {
  class Text {
    public:
      inline Text() { }
      inline Text(std::string str, int size) {
        update(str, size);
      }

      inline void draw(
        od::FrameBuffer &fb,
        od::Color color,
        const v2d &v,
        JustifyAlign ja,
        bool outline,
        bool clear = false
      ) const {
        auto position = Box::wh(mDimensions).justifyAlign(v, ja);
        if (clear) position.clear(fb);
        fb.text(color, position.left(), position.bottom(), mValue.c_str(), mSize);
        if (outline) position.outline(fb, WHITE, 2);
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
  };
}