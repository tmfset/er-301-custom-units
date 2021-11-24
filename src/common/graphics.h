#pragma once

#include <util.h>

namespace graphics {
  struct Box {
    inline Box(float _left, float _bottom, float _right, float _top) :
        left(_left),
        bottom(_bottom),
        right(_right),
        top(_top),
        width(_right - _left),
        height(_top - _bottom),
        centerX(_left + width * 0.5f),
        centerY(_bottom + height * 0.5f) { }

    static inline Box lbrt(float _left, float _bottom, float _right, float _top) {
      auto l = util::min(_left, _right);
      auto b = util::min(_bottom, _top);
      auto r = util::max(_left, _right);
      auto t = util::max(_bottom, _top);
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

    static inline Box cwh(float _x, float _y, float _width, float _height) {
      auto hWidth = _width / 2.0f;
      auto hHeight = _height / 2.0f;
      return lbrt(_x - hWidth, _y - hHeight, _x + hWidth, _y + hHeight);
    }

    static inline Box cs(float _x, float _y, float _size) {
      return cwh(_x, _y, _size, _size);
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

    inline Box center(float size) const {
      return cs(centerX, centerY, size);
    }

    inline Box centerOnX(float y, float size) const {
      return cs(centerX, y, size);
    }

    inline Box padRight(float by) const {
      return lbrt(left, bottom, right - by, top);
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

    inline Box offsetX(float by) const {
      return lbrt(left + by, bottom, right + by, top);
    }

    inline Box offsetY(float by) const {
      return lbrt(left, bottom + by, right, top + by);
    }

    inline Box intersect(const Box& other) const {
      return lbrt(
        util::fmax(left, other.left),
        util::fmax(bottom, other.bottom),
        util::fmin(right, other.right),
        util::fmin(top, other.top)
      );
    }

    inline Box recenter(float atX, float atY) const {
      return cwh(atX, atY, width, height);
    }

    inline Box recenterOn(const Box& other) const {
      return recenter(other.centerX, other.centerY);
    }

    inline Box recenterX(float atX) const {
      return cwh(atX, centerY, width, height);
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

    inline bool containsX(float x) const {
      return x > left && x < right;
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) {
      fb.fill(color, left, bottom, right, top);
    }

    inline void clear(od::FrameBuffer &fb) {
      fb.clear(left, bottom, right, top);
    }

    inline void line(od::FrameBuffer &fb, od::Color color) {
      fb.box(color, left, bottom, right, top);
    }

    inline void outline(od::FrameBuffer &fb, od::Color color) {
      fb.box(color, (int)left - 1, (int)bottom - 1, (int)right + 1, (int)top + 1);
    }

    const float left;
    const float bottom;
    const float right;
    const float top;

    const float width;
    const float height;

    const float centerX;
    const float centerY;
  };
}