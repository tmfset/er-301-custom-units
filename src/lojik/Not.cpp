#include <Not.h>
#include <od/extras/LookupTables.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>

namespace lojik {
  Not::Not() {
    addInput(mIn);
    addOutput(mOut);
  }

  Not::~Not() { }

  void Not::process() {
    float *in  = mIn.buffer();
    float *out = mOut.buffer();

    float32x4_t fZero = vdupq_n_f32(0.0f);
    uint32x4_t  uZero = vdupq_n_u32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t v = vld1q_f32(in + i);
      uint32x4_t  c = vcgtq_f32(v, fZero);
      uint32x4_t  n = vornq_u32(uZero, c);

      vst1q_f32(out + i, vcvtq_n_f32_u32(n, 32));
    }
  }
}