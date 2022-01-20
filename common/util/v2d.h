#pragma once

#include <util/math.h>

class v2d {
  public:
    inline v2d() : mX(0), mY(0) {}
    inline v2d(float x, float y) : mX(x), mY(y) { }

    static inline v2d ofX(float x) { return v2d(x, 0); }
    static inline v2d ofY(float y) { return v2d(0, y); }

    static inline v2d of(float x, float y) { return v2d(x, y); }
    static inline v2d of(float v) { return of(v, v); }

    static inline v2d ar(float a, float r) { return of(sinf(a), cosf(a)) * r; }

    static v2d zero() { return v2d(); };

    inline v2d invert() const { return of(1.0f / mX, 1.0f / mY); }

    inline v2d negate() const { return of(-mX, -mY); }
    inline v2d operator-() const { return negate(); }

    inline v2d offset(float x, float y) const { return offset(of(x, y)); }
    inline v2d offset(const v2d &by)    const { return of(mX + by.mX, mY + by.mY); }
    inline v2d operator+(const v2d &by) const { return offset(by); }
    inline v2d operator-(const v2d &by) const { return offset(-by); }

    inline v2d scale(const v2d &by)    const { return of(mX * by.mX, mY * by.mY); }
    inline v2d scale(float x, float y) const { return scale(of(x, y)); }
    inline v2d scale(float by)         const { return scale(of(by)); }
    inline v2d scaleX(float by)        const { return of(mX * by, mY); }
    inline v2d scaleY(float by)        const { return of(mX, mY * by); }

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

    inline v2d lerp(const v2d &to, float by) const {
      return of(
        util::flerpi(mX, to.mX, by),
        util::flerpi(mY, to.mY, by)
      );
    }

    inline v2d swap() const { return of(mY, mX); }
    inline v2d rotateCW() const { return of(mY, -mX); }
    inline v2d rotateCCW() const { return of(-mY, mX); }

    inline v2d atX(float x)      const { return of(x, mY); }
    inline v2d atX(const v2d &v) const { return atX(v.x()); }
    inline v2d atY(float y)      const { return of(mX, y); }
    inline v2d atY(const v2d &v) const { return atY(v.y()); }

    inline v2d offsetX(float by)     const { return atX(mX + by); }
    inline v2d offsetX(const v2d &v) const { return offsetX(v.x()); }
    inline v2d offsetY(float by)     const { return atY(mY + by); }
    inline v2d offsetY(const v2d &v) const { return offsetY(v.y()); }

    inline float x() const { return mX; }
    inline float y() const { return mY; }

  private:
    float mX;
    float mY;
};