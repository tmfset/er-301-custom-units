#include <Register.h>
#include <od/extras/Random.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sstream>

namespace lojik {
  Register::Register(int max, bool randomize) {
    addInput(mIn);
    addOutput(mOut);
    addInput(mLength);
    addInput(mStride);
    addInput(mClock);
    addInput(mCapture);
    addInput(mShift);
    addInput(mReset);
    addInput(mScatter);
    addInput(mGain);

    mState.setMax(max);
    if (randomize) triggerRandomizeAll();
  }

  Register::~Register() { }

  void Register::process() {
    this->processTriggers();

    float *in         = mIn.buffer();
    float *out        = mOut.buffer();
    float *length     = mLength.buffer();
    float *stride     = mStride.buffer();
    float *clock      = mClock.buffer();
    float *capture    = mCapture.buffer();
    float *shift      = mShift.buffer();
    float *reset      = mReset.buffer();
    float *scatter    = mScatter.buffer();
    float *gain       = mGain.buffer();

    float32x4_t fZero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      int32_t iLength[4], iStride[4];
      uint32_t isClockHigh[4], isCaptureHigh[4], isShiftHigh[4], isResetHigh[4], isScatterHigh[4];

      vst1q_s32(iLength,       vcvtq_s32_f32(vld1q_f32(length + i)));
      vst1q_s32(iStride,       vcvtq_s32_f32(vld1q_f32(stride + i)));
      vst1q_u32(isClockHigh,   vcgtq_f32(vld1q_f32(clock   + i), fZero));
      vst1q_u32(isCaptureHigh, vcgtq_f32(vld1q_f32(capture + i), fZero));
      vst1q_u32(isShiftHigh,   vcgtq_f32(vld1q_f32(shift   + i), fZero));
      vst1q_u32(isResetHigh,   vcgtq_f32(vld1q_f32(reset   + i), fZero));
      vst1q_u32(isScatterHigh, vcgtq_f32(vld1q_f32(scatter + i), fZero));

      for (int j = 0; j < 4; j++) {
        if (!isClockHigh[j]) mTriggerable = true;
        bool isTrigger = mTriggerable && isClockHigh[j];

        mState.setLSG(iLength[j], iStride[j], gain[i + j]);

        if (isTrigger) {
          mTriggerable = false;
          if (isShiftHigh[j]) mState.shift(); else mState.step();
          if (isResetHigh[j]) mState.reset();
        }

        uint32_t index = mState.current();

        if (isTrigger) {
          if (isCaptureHigh[j]) {
            if (isScatterHigh[j]) {
              mState.randomize(index);
            } else {
              mState.set(index, in[i + j]);
            }
          }
        }

        out[i + j] = mState.get(index);
      }
    }
  }

  inline void Register::processTriggers() {
    if (mTriggerZeroWindow) {
      mState.zeroWindow();
      mTriggerZeroWindow = false;
    }

    if (mTriggerZeroAll) {
      mState.zeroAll();
      mTriggerZeroAll = false;
    }

    if (mTriggerRandomizeWindow) {
      mState.randomizeWindow();
      mTriggerRandomizeWindow = false;
    }

    if (mTriggerRandomizeAll) {
      mState.randomizeAll();
      mTriggerRandomizeAll = false;
    }

    // Deserialize last to prefer user data.
    if (mTriggerDeserialize) {
      mState  = mBuffer;
      mBuffer = {};
      mTriggerDeserialize = false;
    }
  }
}