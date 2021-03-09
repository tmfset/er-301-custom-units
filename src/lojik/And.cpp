#include <And.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  And::And() {
    addInput(mLeft);
    addInput(mRight);
    addOutput(mOut);
  }

  And::~And() { }

  void And::process() {
    float *left  = mLeft.buffer();
    float *right = mRight.buffer();
    float *out   = mOut.buffer();

    float32x4_t zero = vdupq_n_f32(0.0f);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t l = vld1q_f32(left + i);
      float32x4_t r = vld1q_f32(right + i);

      // Multiply and check if the result is non-zero.
      float32x4_t m = vmulq_f32(l, r);
      vst1q_f32(out + i, vcvtq_n_f32_u32(vcgtq_f32(m, zero), 32));
    }
  }
}