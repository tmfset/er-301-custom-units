#pragma once

#include <od/graphics/FrameBuffer.h>
#include <util/v2d.h>

namespace graphics {
  struct Circle {
    inline Circle(const v2d &c, float r) :
      center(c),
      radius(r) { }

    static inline Circle cr(const v2d &c, float r) {
      return Circle(c, r);
    }

    inline Circle offset(const v2d &by) const {
      return cr(center + by, radius);
    }

    inline Circle scale(float by) const {
      return cr(center, radius * by);
    }

    inline Circle grow(float by) const {
      return cr(center, radius + by);
    }

    inline Circle quantize() const {
      return cr(center.quantize(), radius);
    }

    inline v2d pointAtTheta(float theta) const {
      return center + v2d::ar(theta, radius);
    }

    inline void trace(od::FrameBuffer &fb, od::Color color) const {
      fb.circle(color, center.x(), center.y(), radius);
    }

    inline void fill(od::FrameBuffer &fb, od::Color color) const {
      fb.fillCircle(color, center.x(), center.y(), radius);
    }

    const v2d center;
    const float radius;
  };
}