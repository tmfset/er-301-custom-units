#include <Wait.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  Wait::Wait() {
    addInput(mIn);
    addInput(mCount);
    addInput(mInvert);
    addInput(mArm);
    addOutput(mOut);

    addOption(mSense);
  }

  Wait::~Wait() { }

  void Wait::process() {
    float *in     = mIn.buffer();
    float *count  = mCount.buffer();
    float *invert = mInvert.buffer();
    float *arm    = mArm.buffer();
    float *out    = mOut.buffer();

    float32x4_t sense = vdupq_n_f32(getSense(mSense));
    float32x4_t zero  = vdupq_n_f32(0);

    OneTime trigSwitch { mTrigSwitch, false };
    bool isArmed = mIsArmed;
    int step = mStep;

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadIn     = vld1q_f32(in + i);
      float32x4_t loadCount  = vld1q_f32(count + i);
      float32x4_t loadInvert = vld1q_f32(invert + i);
      float32x4_t loadArm    = vld1q_f32(arm + i);

      int32_t iCount[4];
      uint32_t isInHigh[4], isInvertHigh[4], isArmHigh[4];

      vst1q_s32(iCount,       vcvtq_s32_f32(loadCount));
      vst1q_u32(isInHigh,     vcgtq_f32(loadIn,     sense));
      vst1q_u32(isInvertHigh, vcgtq_f32(loadInvert, zero));
      vst1q_u32(isArmHigh,    vcgtq_f32(loadArm,    zero));

      for (int j = 0; j < 4; j++) {
        trigSwitch.mark(isInHigh[j]);
        bool doStep = trigSwitch.read();

        if (isArmHigh[j]) {
          isArmed = true;
        }

        if (doStep && isArmed) {
          step = step + 1;
        }

        if (step > iCount[j]) {
          step = 0;
          isArmed = false;
        }

        float value = 0.0f;
        if (isInvertHigh[j]) {
          value = step ? in[i + j] : 0.0f;
        } else {
          value = step ? 0.0f : in[i + j];
        }

        out[i + j] = value;
      }
    }

    mTrigSwitch = trigSwitch;
    mIsArmed    = isArmed;
    mStep       = step;
  }
}
