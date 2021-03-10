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

    for (int i = 0; i < FRAMELENGTH; i ++) {
      uint32_t limit = 4;//CLAMP(1, 128, static_cast<int>(length[i]));

      bool isClockHigh   = clock[i]   > 0.0f;
      bool isCaptureHigh = capture[i] > 0.0f;
      bool isShiftHigh   = shift[i]   > 0.0f;
      bool isResetHigh   = reset[i]   > 0.0f;
      bool isScatterHigh = scatter[i] > 0.0f;

      if (!isClockHigh) mWait = false;

      bool isTrigger = !mWait && isClockHigh;

      if (isTrigger) {
        mWait = true;

        if (isShiftHigh) this->shift(limit); else this->step(limit);
        if (isResetHigh) this->reset();
      }

      uint32_t index = this->index(limit);

      if (isTrigger) {
        if (isCaptureHigh) {
          if (isScatterHigh) {
            mData[index] = od::Random::generateFloat(-1.0f, 1.0f) * gain[i];
          } else {
            mData[index] = in[i] * gain[i];
          }
        }
      }

      out[i] = mData[index];
    }
  }

  inline uint32_t Register::index(uint32_t limit) {
    return (mStepCount + mShiftCount) % limit;
  }

  inline void Register::step(uint32_t limit) {
    mStepCount = (mStepCount + 1) % limit;
  }

  inline void Register::shift(uint32_t limit) {
    mShiftCount = (mShiftCount + 1) % limit;
  }

  inline void Register::reset() {
    mStepCount = 0;
  }
}