#pragma once

#include <util/v2d.h>

namespace graphics {
  class Line {
    public:
      inline Line(const v2d &f, const v2d &t) :
        from(f), to(t) { }

      static inline Line of(const v2d &f, const v2d &t) { return Line(f, t); }

      inline Line retreat(const v2d &f) const { return of(f, from); }
      inline Line advance(const v2d &t) const { return of(to, t); }

      inline void trace(od::FrameBuffer &fb, od::Color color) const {
        fb.line(color, from.x(), from.y(), to.x(), to.y());
      }

    private:
      v2d from;
      v2d to;
  };
}