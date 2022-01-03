#pragma once

#include <graphics/primitives/all.h>

namespace graphics {
  struct IFader {
    inline IFader(float _x, float _bottom, float _top, float _width) :
      x(_x),
      span(2),
      width(_width),
      height(_top - _bottom),
      bottom(_bottom),
      center(_bottom + height / 2.0f),
      top(_top) { }

    static inline IFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return IFader(c.x(), c.y() - wh2.y(), c.y() + wh2.y(), wh2.x());
    }

    static inline IFader box(const Box &b) {
      return IFader(b.centerX(), b.bottom(), b.top(), b.width());
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.vline(color, x, bottom, top);
      fb.hline(color, x - span, x + span, bottom);
      fb.hline(color, x - span, x + span, top);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.hline(color, x - (span + 2), x + (span + 2), y);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto y = center + (height / 2.0f) * value;
      fb.box(color, x - (span + 1), y - 1, x + (span + 1), y + 1);
      return y;
    }

    const float x;

    const int span;
    const float width;
    const float height;

    const float bottom;
    const float center;
    const float top;
  };

  struct HFader {
    inline HFader(float _y, float _left, float _right, float _height) :
      y(_y),
      hSpan(1),
      tSpan(util::fclamp(_height / 2.0f, hSpan, hSpan + 1)),
      aSpan(util::fclamp(_height / 2.0f, tSpan, tSpan + 1)),
      width(_right - _left),
      height(_height),
      left(_left),
      center(_left + width / 2.0f),
      right(_right) { }

    static inline HFader cwh(const v2d &c, const v2d &wh) {
      auto wh2 = wh * 0.5;
      return HFader(c.y(), c.x() - wh2.x(), c.x() + wh2.x(), wh2.y());
    }

    static inline HFader box(const Box &b) {
      return HFader(b.centerY(), b.left(), b.right(), b.height() / 2.0f);
    }

    inline void draw(od::FrameBuffer &fb, od::Color color) const {
      fb.hline(color, left, right, y);
      fb.vline(color, left, y - hSpan, y);
      fb.vline(color, right, y, y + hSpan);
    }

    inline float drawActual(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.vline(color, x, y - aSpan, y + aSpan);
      return y;
    }

    inline float drawTarget(od::FrameBuffer &fb, od::Color color, float value) const {
      auto x = util::fhr(center + (width / 2.0f) * value);
      fb.box(color, x - 1, y - tSpan, x + 1, y + tSpan);
      return x;
    }

    const float y;

    const int hSpan;
    const int tSpan;
    const int aSpan;

    const float width;
    const float height;

    const float left;
    const float center;
    const float right;
  };
}