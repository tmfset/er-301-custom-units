#pragma once

#include <hal/simd.h>
#include <util.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace sloop {
  class Slew {
    public:
      Slew() {}

      Slew(float value, float rise, float fall) :
        mValue(value),
        mRise(rise),
        mFall(fall) {}

      Slew(const Slew &other, float rise, float fall) :
        mValue(other.mValue),
        mRise(rise),
        mFall(fall) {}

      // inline float32x4_t moveNeon(float32x4_t target) {
      //   float fValue[4], fTarget[4];
      //   vst1q_f32(fTarget, target);
      //   for (int i = 0; i < 4; i++) {
      //     float target = fTarget[i];
      //     if (mValue == target) {
      //       fValue[i] = target;
      //       continue;
      //     }

      //     mValue = fValue[i] = mValue < target ?
      //       fmin(target, mValue + mRise) :
      //       fmax(target, mValue - mFall);
          
      //     //logDebug(1, "t=%f v=%f", target, mValue);
      //   }
      //   return vld1q_f32(fValue);
      // }

      inline float move(float target) {
        if (mValue != target) {
          mValue = mValue < target ?
            fmin(target, mValue + mRise) :
            fmax(target, mValue - mFall);
        }

        return mValue;
      }

      float value() { return mValue; }

      void setValue(float v) {
        mValue = v;
      }

      void setRiseFall(float rise, float fall) {
        mRise = rise;
        mFall = fall;
      }

    private:
      float mValue = 0;
      float mRise = 1;
      float mFall = 1;
  };
}