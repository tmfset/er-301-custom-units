#pragma once

#include <od/graphics/FrameBuffer.h>

#include <util/v2d.h>

namespace graphics {
  struct Point {
    inline Point(float x, float y) : v(v2d::of(x, y)) { }
    inline Point(const v2d &_v) : v(_v) { }

    static inline Point of(const v2d &_v) {
      return Point(_v);
    }

    inline Point offset(const Point &by) const {
      return Point(v + by.v);
    }

    inline Point scale(float by) const {
      return Point(v * by);
    }

    inline Point offsetX(float byX) const {
      return Point(v + v2d::of(byX, 0));
    }

    inline Point offsetY(float byY) const {
      return Point(v + v2d::of(0, byY));
    }

    inline Point atX(float x) const {
      return Point(v.atX(x));
    }

    inline Point atY(float y) const {
      return Point(v.atY(y));
    }

    inline Point quantize() const {
      return Point(v.quantize());
    }

    inline void diamond(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x() - 1, v.y());
      fb.pixel(color, v.x() + 1, v.y());
      fb.pixel(color, v.x(), v.y() - 1);
      fb.pixel(color, v.x(), v.y() + 1);
    }

    inline void dot(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());
    }

    inline void arrowLeft(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() + 1, v.y() - 1);
      fb.pixel(color, v.x() + 2, v.y() - 2);

      fb.pixel(color, v.x() + 1, v.y() + 1);
      fb.pixel(color, v.x() + 2, v.y() + 2);
    }

    inline void arrowsLeft(od::FrameBuffer &fb, od::Color color, int count, int step) const {
      v2d p = v;
      for (int i = 0; i < count; i++) {
        Point(p).arrowLeft(fb, color);
        p = p + v2d::of(step, 0);
      }
    }

    inline void arrowRight(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() - 1, v.y() - 1);
      fb.pixel(color, v.x() - 2, v.y() - 2);

      fb.pixel(color, v.x() - 1, v.y() + 1);
      fb.pixel(color, v.x() - 2, v.y() + 2);
    }

    inline void arrowsRight(od::FrameBuffer &fb, od::Color color, int count, int step) const {
      v2d p = v;
      for (int i = 0; i < count; i++) {
        Point(p).arrowRight(fb, color);
        p = p + v2d::of(-step, 0);
      }
    }

    inline void arrowDown(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() - 1, v.y() + 1);
      fb.pixel(color, v.x() - 2, v.y() + 2);

      fb.pixel(color, v.x() + 1, v.y() + 1);
      fb.pixel(color, v.x() + 2, v.y() + 2);
    }

    inline void arrowsDown(od::FrameBuffer &fb, od::Color color, int count, int step, float fade = 1) const {
      v2d p = v;
      float c = color;
      for (int i = 0; i < count; i++) {
        Point(p).arrowDown(fb, c);
        p = p + v2d::of(0, step);
        c = c * fade;
      }
    }

    inline void arrowUp(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, v.x(), v.y());

      fb.pixel(color, v.x() - 1, v.y() - 1);
      fb.pixel(color, v.x() - 2, v.y() - 2);

      fb.pixel(color, v.x() + 1, v.y() - 1);
      fb.pixel(color, v.x() + 2, v.y() - 2);
    }

    inline void arrowsUp(od::FrameBuffer &fb, od::Color color, int count, int step, float fade = 1) const {
      v2d p = v;
      float c = color;
      for (int i = 0; i < count; i++) {
        Point(p).arrowUp(fb, c);
        p = p + v2d::of(0, -step);
        c = c * fade;
      }
    }

    const v2d v;
  };
}