#pragma once

#include <util/math.h>

namespace graphics {
  class Range {
    public:
      /**
       * Safe ctors to build valid ranges.
       */
      static inline Range lr(float l, float r) { return _lr(util::fmin(l, r), util::fmax(l, r)); }
      static inline Range lw(float l, float w) { return _lw(l, util::fmax(0, w)); }
      static inline Range rw(float r, float w) { return _rw(r, util::fmax(0, w)); }
      static inline Range cs(float c, float s) { return _cs(c, util::fabs(s)); }
      static inline Range cw(float c, float w) { return _cw(c, util::fabs(w)); }
      static inline Range sw(float s, float w) { return w > 0 ? _lw(s, w) : _rw(s, w); }

      inline Range fromLeft(float width) const { return lw(left(), width); }
      inline Range fromRight(float width) const { return rw(right(), width); }

      /**
       * Move, preserving size.
       */
      inline Range atLeft(float l)   const { return _lw(l, width()); }
      inline Range atRight(float r)  const { return _rw(r, width()); }
      inline Range atCenter(float c) const { return _cw(c, width()); }

      /**
       * Align within another range.
       */
      inline Range alignLeft(const Range &r)   const { return atLeft(r.left()); }
      inline Range alignRight(const Range &r)  const { return atRight(r.right()); }
      inline Range alignCenter(const Range &r) const { return atCenter(r.center()); }

      inline Range justify(const Range &within, od::Justification j) const {
        switch (j) {
          case od::justifyLeft:   return alignLeft(within);
          case od::justifyCenter: return alignCenter(within);
          case od::justifyRight:  return alignRight(within);
        }
        return alignLeft(within);
      }

      /**
       * Clamp things with this range.
       */
      inline float clamp(float v)             const { return util::fclamp(v, left(), right()); }
      inline Range clamp(const Range &r)      const { return _lw(clamp(r.left()), clamp(r.right())); }
      inline Range clampLeft(const Range &r)  const { return _lw(clamp(r.left()), r.width()); }
      inline Range clampRight(const Range &r) const { return _rw(clamp(r.right()), r.width()); }

      /**
       * Fit the other range inside this one, ensuring:
       * 
       * if it's too far left, move it right,
       * if it's too far right, move it left,
       * and if it's too large, shrink it down.
       */
      inline Range insert(const Range &other) const {
        return _fromLeft(util::max(0, width() - other.width()))
          .clampLeft(other)
          .fromLeft(util::fmin(width(), other.width()));
      }

      inline bool contains(float v) const {
        return v >= left() && v <= right();
      }

      inline float segment(float parts, float pad) const {
        float remaining = util::max(0, width() - (parts - 1) * pad);
        return util::fdr(remaining / (float)parts);
      }

      static inline Range fromSegment(float parts, float pad, float segment) {
        return Range { 0, segment * parts + (parts - 1) * pad };
      }

      inline float left()      const { return mLeft; }
      inline float right()     const { return mRight; }
      inline float width()     const { return mWidth; }
      inline float halfWidth() const { return mHalfWidth; }
      inline float center()    const { return left() + halfWidth(); }

    private:
      /**
       * Unsafe ctors, to build from valid values.
       */
      static inline Range _lr(float l, float r) { return Range { l, r }; }
      static inline Range _lw(float l, float w) { return Range { l, l + w }; }
      static inline Range _rw(float r, float w) { return Range { r - w, r }; }
      static inline Range _cs(float c, float s) { return Range { c - s, c + s }; }
      static inline Range _cw(float c, float w) { return _cs(c, w / 2.0f); }

      inline Range _fromLeft(float width) const { return _lw(left(), width); }
      inline Range _fromRight(float width) const { return _rw(right(), width); }

      inline Range(float left, float right) :
        mLeft(left),
        mRight(right),
        mWidth(mRight - mLeft),
        mHalfWidth(mWidth / 2.0f) { }

      float mLeft = 0;
      float mRight = 0;
      float mWidth = 0;
      float mHalfWidth = 0;
  };
}