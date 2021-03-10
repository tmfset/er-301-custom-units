#include <Register.h>
#include <od/extras/LookupTables.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  Register::Register() {
    addInput(mIn);
    addOutput(mOut);
    addInput(mLength);
    addInput(mClock);
    addInput(mCapture);
    addInput(mShift);
    addInput(mReset);
    addInput(mScatter);
    addInput(mGain);

    mData = new float[128];
  }

  Register::~Register() {
    delete [] mData;
  }

  void Register::process() {
    float *in         = mIn.buffer();
    float *out        = mOut.buffer();
    float *length     = mLength.buffer();
    float *clock      = mClock.buffer();
    float *capture    = mCapture.buffer();
    float *shift      = mShift.buffer();
    float *reset      = mReset.buffer();
    float *scatter    = mScatter.buffer();
    float *gain       = mGain.buffer();

    float32x4_t fZero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t vLength  = vld1q_f32(length + i);
      float32x4_t vClock   = vld1q_f32(clock + i);
      float32x4_t vCapture = vld1q_f32(capture + i);
      float32x4_t vShift   = vld1q_f32(shift + i);
      float32x4_t vReset   = vld1q_f32(reset + i);
      float32x4_t vScatter = vld1q_f32(scatter + i);

      int32x4_t  uLength       = vcvtq_s32_f32(vLength);
      uint32x4_t isClockHigh   = vcgtq_f32(vClock,   fZero);
      uint32x4_t isCaptureHigh = vcgtq_f32(vCapture, fZero);
      uint32x4_t isShiftHigh   = vcgtq_f32(vShift,   fZero);
      uint32x4_t isResetHigh   = vcgtq_f32(vReset,   fZero);
      uint32x4_t isScatterHigh = vcgtq_f32(vScatter, fZero);

      for (int j = 0; j < 4; j++) {
        if (!isClockHigh[j]) mWait = false;
        bool isTrigger = !mWait && isClockHigh[j];

        int32_t limit = CLAMP(1, 128, uLength[j]);

        if (isTrigger) {
          mWait = true;
          if (isShiftHigh[j]) this->shift(limit); else this->step(limit);
          if (isResetHigh[j]) this->reset();
        }

        uint32_t index = this->index(limit);

        if (isTrigger) {
          if (isCaptureHigh[j]) {
            if (isScatterHigh[j]) {
              mData[index] = od::Random::generateFloat(-1.0f, 1.0f) * gain[i];
            } else {
              mData[index] = in[i] * gain[i];
            }
          }
        }

        out[i + j] = mData[index];
      }
    }
  }

  inline int32_t Register::index(int32_t limit) {
    return (mStepCount + mShiftCount) % limit;
  }

  inline void Register::step(int32_t limit) {
    mStepCount = (mStepCount + 1) % limit;
  }

  inline void Register::shift(int32_t limit) {
    mShiftCount = (mShiftCount + 1) % limit;
  }

  inline void Register::reset() {
    mStepCount = 0;
  }
}