#include <DLatch.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <util.h>

namespace lojik {
  DLatch::DLatch() {
    addInput(mIn);
    addInput(mClock);
    addInput(mReset);
    addOutput(mOut);
  }

  DLatch::~DLatch() { }

  void DLatch::process() {
    float *in    = mIn.buffer();
    float *clock = mClock.buffer();
    float *reset = mReset.buffer();
    float *out   = mOut.buffer();

    float32x4_t zero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t c = vld1q_f32(clock + i);
      float32x4_t r = vld1q_f32(reset + i);

      uint32_t cc[4], rc[4];
      vst1q_u32(cc, vcgtq_f32(c, zero));
      vst1q_u32(rc, vcgtq_f32(r, zero));

      for (int j = 0; j < 4; j++) {
        if (!cc[j] || rc[j]) {
          mCatch = true;
        }

        out[i + j] = mCurrent;

        // Delay by one sample so we can form a chain.
        if (cc[j] && mCatch) {
          mCatch = false;
          mCurrent = in[i + j];
        }
      }
    }
  }
}