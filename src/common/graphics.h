#pragma once

#include <util.h>

namespace graphics {
  struct Point {
    inline Point(float _x, float _y) :
      x(_x),
      y(_y) { }

    inline Point withX(float _x) const {
      return Point(_x, y);
    }

    inline Point withY(float _y) const {
      return Point(x, _y);
    }

    inline Point quantize() const {
      return Point(util::fhr(x), util::fhr(y));
    }

    inline void circle(od::FrameBuffer &fb, od::Color color, float radius) const {
      fb.circle(color, x, y, radius);
    }

    inline void fillCircle(od::FrameBuffer &fb, od::Color color, float radius) const {
      fb.fillCircle(color, x, y, radius);
    }

    inline void diamond(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, x - 1, y);
      fb.pixel(color, x + 1, y);
      fb.pixel(color, x, y - 1);
      fb.pixel(color, x, y + 1);
    }

    inline void dot(od::FrameBuffer &fb, od::Color color) const {
      fb.pixel(color, x, y);
    }

    float x;
    float y;
  };

  struct Box {
    inline Box(float _left, float _bottom, float _right, float _top) :
        left(_left),
        bottom(_bottom),
        right(_right),
        top(_top),
        width(_right - _left),
        height(_top - _bottom),
        center(Point(_left + width * 0.5f, _bottom + height * 0.5f)) { }

    static inline Box lbrt(float _left, float _bottom, float _right, float _top) {
      auto l = util::fmin(_left, _right);
      auto b = util::fmin(_bottom, _top);
      auto r = util::fmax(_left, _right);
      auto t = util::fmax(_bottom, _top);
      return Box(l, b, r, t);
    }

    static inline Box lbwh(float _left, float _bottom, float _width, float _height) {
      return lbrt(_left, _bottom, _left + _width, _bottom + _height);
    }

    static inline Box cwr(float _x, float _y, float _width, float _rise) {
      auto hWidth = _width / 2.0f;
      return lbrt(_x - hWidth, _y, _x + hWidth, _y + _rise);
    }

    static inline Box wh(float _width, float _height) {
      return lbwh(0, 0, _width, _height);
    }

    static inline Box cwh(const Point &c, float _width, float _height) {
      auto hWidth = _width / 2.0f;
      auto hHeight = _height / 2.0f;
      return lbrt(c.x - hWidth, c.y - hHeight, c.x + hWidth, c.y + hHeight);
    }

    static inline Box cs(const Point &c, float _size) {
      return cwh(c, _size, _size);
    }

    inline Box inner(float margin) const {
      return inner(margin, margin);
    }

    inline Box inner(float marginX, float marginY) const {
      return lbrt(
        left   + marginX,
        bottom + marginY,
        right  - marginX,
        top    - marginY
      );
    }

    inline Box centerOn(const Box& other) const {
      return cwh(other.center, width, height);
    }

    inline Box square(float size) const {
      return cs(center, size);
    }

    inline Box padRight(float by) const {
      return lbrt(left, bottom, right - by, top);
    }

    inline Box topLeftCorner(float w, float h) const {
      return lbrt(left, top - height * h, left + width * w, top);
    }

    inline Box divideTop(float by) const {
      return lbrt(left, top - height * by, right, top);
    }

    inline Box divideBottom(float by) const {
      return lbrt(left, bottom, right, bottom + height * by);
    }

    inline Box divideLeft(float by) const {
      return lbrt(left, bottom, left + width * by, top);
    }

    inline Box divideRight(float by) const {
      return lbrt(right - width * by, bottom, right, top);
    }

    inline Box withWidthFromLeft(float _width) const {
      return lbwh(left, bottom, _width, height);
    }

    inline Box offset(float byX, float byY) const {
      return lbrt(left + byX, bottom + byY, right + byX, top + byY);
    }

    inline Box offsetX(float by) const {
      return lbrt(left + by, bottom, right + by, top);
    }

    inline Box offsetY(float by) const {
      return lbrt(left, bottom + by, right, top + by);
    }

    inline Box quantizeSize() const {
      return cwh(center, util::fhr(width), util::fhr(height));
    }

    inline Box quantizeCenter() const {
      return cwh(center.quantize(), width, height);
    }

    inline Box quantize() const {
      return quantizeSize().quantizeCenter();
    }

    inline Box intersect(const Box& other) const {
      return lbrt(
        util::fmax(left, other.left),
        util::fmax(bottom, other.bottom),
        util::fmin(right, other.right),
        util::fmin(top, other.top)
      );
    }

    inline Box recenter(const Point &c) const {
      return cwh(c, width, height);
    }

    inline Box recenterOn(const Box& other) const {
      return recenter(other.center);
    }

    inline Box recenterY(float y) const {
      return recenter(center.withY(y));
    }

    inline Box recenterX(float x) const {
      return recenter(center.withX(x));
    }

    inline Box insert(const Box& other) const {
      auto l = util::fclamp(other.left, left, clampX(right - other.width));
      auto b = util::fclamp(other.bottom, bottom, clampY(top - other.height));
      auto r = util::fclamp(other.right, clampX(l + other.width), right);
      auto t = util::fclamp(other.top, clampY(b + other.height), top);
      return lbrt(l, b, r, t);
    }

    inline float clampX(float x) const {
      return util::fclamp(x, left, right);
    }

    inline float clampY(float y) const {
      return util::fclamp(y, bottom, top);
    }

    inline float minDimension() const {
      return util::fmin(width, height);
    }

    inline Box minSquare() const {
      return cs(center, minDimension());
    }

    inline Box scaleDiscrete(int w, int h) const {
      return cwh(center, width * w, height * h);
    }

    inline bool containsX(float x) const {
      return x > left && x < right;
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fill(color, left, bottom, right, top);
    }

    inline void clear(od::FrameBuffer &fb) const {
      fb.clear(left, bottom, right, top);
    }

    inline void line(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, left, bottom, right, top);
    }

    inline void outline(od::FrameBuffer &fb, od::Color color) const {
      fb.box(color, (int)left - 1, (int)bottom - 1, (int)right + 1, (int)top + 1);
    }

    inline void circle(od::FrameBuffer &fb, od::Color color) const {
      center.circle(fb, color, width / 2.0f);
    }

    inline void fillCircle(od::FrameBuffer &fb, od::Color color) const {
      center.fillCircle(fb, color, width / 2.0f);
    }

    const float left;
    const float bottom;
    const float right;
    const float top;

    const float width;
    const float height;

    const Point center;
  };
}