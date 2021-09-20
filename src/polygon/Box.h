#pragma once

#include "util.h"

namespace polygon {
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
      return Box(_left, _bottom, _right, _top);
    }

    static inline Box lbwh(float _left, float _bottom, float _width, float _height) {
      return lbrt(_left, _bottom, _left + _width, _bottom + _height);
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

    inline float minDimension() const {
      return util::fmin(width, height);
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