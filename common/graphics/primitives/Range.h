#pragma once

#include <util/math.h>

namespace graphics {
  class Range {
    public:
      static inline Range from(float left, float right) {
        return _from(util::fmin(left, right), util::fmax(left, right));
      }

      static inline Range fromLeft(float left, float width) {
        return _fromLeft(left, util::fmax(0, width));
      }
      inline Range fromLeft(float width) const {
        return fromLeft(left(), width);
      }

      static inline Range fromRight(float right, float width) {
        return _fromRight(right, util::fmax(0, width));
      }
      inline Range fromRight(float width) const {
        return fromRight(right(), width);
      }

      static inline Range fromSide(float side, float width) {
        return width > 0 ? _fromLeft(side, width) : _fromRight(side, width);
      }

      static inline Range fromCenterSpan(float center, float span) {
        return _fromCenterSpan(center, util::fabs(span));
      }
      static inline Range fromCenterWidth(float center, float width) {
        return _fromCenterWidth(center, util::fabs(width));
      }

      inline Range atCenter(float c) const {
        return _fromCenterSpan(c, halfWidth());
      }
      inline Range atCenter(const Range &other) const {
        return atCenter(other.center());
      }

      inline float clamp(float v) const {
        return util::fclamp(v, left(), right());
      }
      inline Range clamp(const Range &other) const {
        return _from(clamp(other.left()), clamp(other.right()));
      }
      inline Range clampLeft(const Range &other) const {
        return _fromLeft(clamp(other.left()), other.width());
      }
      inline Range clampRight(const Range &other) const {
        return _fromRight(clamp(other.right()), other.width());
      }

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
      static inline Range _from(float left, float right) {
        return Range { left, right };
      }

      static inline Range _fromLeft(float left, float width) {
        return Range { left, left + width };
      }
      inline Range _fromLeft(float width) const {
        return _fromLeft(left(), width);
      }

      static inline Range _fromRight(float right, float width) {
        return Range { right - width, right };
      }
      inline Range _fromRight(float width) const {
        return _fromRight(right(), width);
      }

      static inline Range _fromCenterSpan(float center, float span)   {
        return Range { center - span, center + span };
      }
      static inline Range _fromCenterWidth(float center, float width) {
        return _fromCenterSpan(center, width / 2.0f);
      }

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