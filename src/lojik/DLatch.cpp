#include <DLatch.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

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

    float32x4_t fZero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t c = vld1q_f32(clock + i);
      float32x4_t r = vld1q_f32(reset + i);

      uint32_t cc[4], rc[4];
      vst1q_u32(cc, vcgtq_f32(c, fZero));
      vst1q_u32(rc, vcgtq_f32(r, fZero));

      for (int j = 0; j < 4; j++) {
        if (!cc[j] || rc[j]) {
          mAllow = 1;
        }

        out[i + j] = mCurrent;

        if (mAllow && cc[j]) {
          mAllow = 0;
          mCurrent = in[i + j];
        }
      }
    }
  }
}