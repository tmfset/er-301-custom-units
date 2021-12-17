#pragma once

#include "util.h"

namespace common {
  class v2d {
    public:
      inline v2d() : mX(0), mY(0) {}
      inline v2d(float x, float y) : mX(x), mY(y) { }

      static inline v2d of(float x, float y) { return v2d(x, y); }
      static inline v2d of(float v) { return of(v, v); }

      static inline v2d ar(float a, float r) { return of(sinf(a), cosf(a)) * r; }

      static v2d zero() { return v2d(); };

      inline v2d negate() const {
        return of(-mX, -mY);
      }

      inline v2d operator-() const {
        return negate();
      }

      inline v2d offset(const v2d &by) const { return of(mX + by.mX, mY + by.mY); }
      inline v2d operator+(const v2d &by) const { return offset(by); }
      inline v2d operator-(const v2d &by) const { return offset(-by); }

      inline v2d scale(const v2d &by) const { return of(mX * by.mX, mY * by.mY); }
      inline v2d scale(float by) const { return scale(of(by)); }

      inline v2d operator*(const v2d &by) const { return scale(by); }
      inline v2d operator*(float by) const { return scale(by); }
      inline v2d operator/(float by) const { return scale(1.0f / by); }

      inline v2d quantize() const {
        return of(util::fhr(mX), util::fhr(mY));
      }

      inline v2d max(const v2d &v) const {
        return of(util::fmax(mX, v.mX), util::fmax(mY, v.mY));
      }

      inline v2d min(const v2d &v) const {
        return of(util::fmin(mX, v.mX), util::fmin(mY, v.mY));
      }

      inline float minDimension() const {
        return util::fmin(mX, mY);
      }

      inline v2d abs() const {
        return of(util::fabs(mX), util::fabs(mY));
      }

      inline v2d clamp(const v2d &a, const v2d &b) const {
        return of(
          util::fclamp(mX, a.mX, b.mX),
          util::fclamp(mY, a.mY, b.mY)
        );
      }

      inline v2d atX(float _x) const { return of(_x, mY); }
      inline v2d atY(float _y) const { return of(mX, _y); }

      inline v2d offsetX(float by) const { return atX(mX + by); }
      inline v2d offsetY(float by) const { return atY(mY + by); }

      inline float x() const { return mX; }
      inline float y() const { return mY; }

    private:
      float mX;
      float mY;
  };
}