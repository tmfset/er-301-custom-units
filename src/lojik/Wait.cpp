#include <Wait.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <util.h>

namespace lojik {
  Wait::Wait() {
    addInput(mIn);
    addInput(mWait);
    addInput(mClock);
    addInput(mReset);
    addOutput(mOut);
  }

  Wait::~Wait() { }

  void Wait::process() {
    float *in    = mIn.buffer();
    float *wait  = mWait.buffer();
    float *clock = mClock.buffer();
    float *reset = mReset.buffer();
    float *out   = mOut.buffer();

    float32x4_t zero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadWait  = vld1q_f32(wait + i);
      float32x4_t loadClock = vld1q_f32(clock + i);
      float32x4_t loadReset = vld1q_f32(reset + i);

      int32_t iWait[4];
      uint32_t isClockHigh[4], isResetHigh[4];

      vst1q_s32(iWait,      vcvtq_s32_f32(loadWait));
      vst1q_u32(isClockHigh, vcgtq_f32(loadClock, zero));
      vst1q_u32(isResetHigh, vcgtq_f32(loadReset, zero));

      for (int j = 0; j < 4; j++) {
        if (!isClockHigh[j]) {
          mAllowStep = true;
        } else if (mAllowStep) {
          mCount     = mod(mCount + 1, clamp(iWait[j] + 1, 1, 65536));
          mAllowStep = false;
        }

        if (isResetHigh[j]) mCount = 0;

        out[i + j] = mCount ? 0.0f : in[i + j];
      }
    }
  }
}
