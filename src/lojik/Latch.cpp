#include <Latch.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  Latch::Latch() {
    addInput(mSet);
    addInput(mReset);
    addOutput(mOut);
  }

  Latch::~Latch() { }

  void Latch::process() {
    float *set   = mSet.buffer();
    float *reset = mReset.buffer();
    float *out   = mOut.buffer();

    float32x4_t fZero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t s = vld1q_f32(set + i);
      float32x4_t r = vld1q_f32(reset + i);

      uint32_t sc[4], rc[4];
      vst1q_u32(sc, vcgtq_f32(s, fZero));
      vst1q_u32(rc, vcgtq_f32(r, fZero));

      for (int j = 0; j < 4; j++) {
        if (rc[j]) {
          mCurrent = 0.0f;
        }

        if (sc[j]) {
          mCurrent = 1.0f;
        }

        out[i + j] = mCurrent;
      }
    }
  }
}