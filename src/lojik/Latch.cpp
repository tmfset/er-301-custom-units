#include <Latch.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  Latch::Latch() {
    addInput(mIn);
    addInput(mReset);
    addOutput(mOut);

    addOption(mSense);
  }

  Latch::~Latch() { }

  void Latch::process() {
    float *in    = mIn.buffer();
    float *reset = mReset.buffer();
    float *out   = mOut.buffer();

    float32x4_t sense = vdupq_n_f32(getSense(mSense));
    float32x4_t zero  = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadIn    = vld1q_f32(in + i);
      float32x4_t loadReset = vld1q_f32(reset + i);

      uint32_t isInHigh[4], isResetHigh[4];
      vst1q_u32(isInHigh,    vcgtq_f32(loadIn, sense));
      vst1q_u32(isResetHigh, vcgtq_f32(loadReset, zero));

      for (int j = 0; j < 4; j++) {
        if (isResetHigh[j]) mCurrent = 0.0f;
        if (isInHigh[j]) mCurrent = 1.0f;

        out[i + j] = mCurrent;
      }
    }
  }
}