#include <Pick.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>

namespace lojik {
  Pick::Pick() {
    addInput(mLeft);
    addInput(mRight);
    addInput(mPick);
    addOutput(mOut);
  }

  Pick::~Pick() { }

  void Pick::process() {
    float *left  = mLeft.buffer();
    float *right = mRight.buffer();
    float *pick  = mPick.buffer();
    float *out   = mOut.buffer();

    float32x4_t zero = vdupq_n_f32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      uint32_t pc[4];
      vst1q_u32(pc, vcgtq_f32(vld1q_f32(pick + i), zero));
      for (int j = 0; j < 4; j++) {
        float next = 0.0f;
        if (pc[j]) next = right[i + j];
        else       next = left[i + j];
        out[i + j] = next;
      }
    }
  }
}
