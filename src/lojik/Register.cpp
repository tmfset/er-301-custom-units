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
    for (int i = 0; i < 128; i++) {
      mData[i] = 0.0f;
    }
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
      int32_t iLength[4];
      uint32_t isClockHigh[4], isCaptureHigh[4], isShiftHigh[4], isResetHigh[4], isScatterHigh[4];

      vst1q_s32(iLength,       vcvtq_s32_f32(vld1q_f32(length + i)));
      vst1q_u32(isClockHigh,   vcgtq_f32(vld1q_f32(clock   + i), fZero));
      vst1q_u32(isCaptureHigh, vcgtq_f32(vld1q_f32(capture + i), fZero));
      vst1q_u32(isShiftHigh,   vcgtq_f32(vld1q_f32(shift   + i), fZero));
      vst1q_u32(isResetHigh,   vcgtq_f32(vld1q_f32(reset   + i), fZero));
      vst1q_u32(isScatterHigh, vcgtq_f32(vld1q_f32(scatter + i), fZero));

      for (int j = 0; j < 4; j++) {
        if (!isClockHigh[j]) mWait = false;
        bool isTrigger = !mWait && isClockHigh[j];

        int32_t limit  = this->clamp(iLength[j]);

        if (isTrigger) {
          mWait = true;
          if (isShiftHigh[j]) this->shift(limit); else this->step(limit);
          if (isResetHigh[j]) this->reset();
        }

        uint32_t index = this->index(limit);

        if (isTrigger) {
          if (isCaptureHigh[j]) {
            if (isScatterHigh[j]) {
              mData[index] = od::Random::generateFloat(-1.0f, 1.0f) * gain[i + j];
            } else {
              mData[index] = in[i + j] * gain[i + j];
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

  inline int32_t Register::clamp(int32_t value) {
    if (value > 128) return 128;
    if (value < 1) return 1;
    return value;
  }
}