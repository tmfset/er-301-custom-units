#include <Trig.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  Trig::Trig() {
    addInput(mIn);
    addOutput(mOut);

    addOption(mSense);
  }

  Trig::~Trig() { }

  void Trig::process() {
    float *in  = mIn.buffer();
    float *out = mOut.buffer();

    float32x4_t sense = vdupq_n_f32(getSense(mSense));

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t v = vld1q_f32(in + i);

      uint32_t vc[4];
      vst1q_u32(vc, vcgtq_f32(v, sense));

      for (int j = 0; j < 4; j++) {
        if (!vc[j]) {
          mAllow = true;
        }

        if (vc[j] && mAllow) {
          mAllow = false;
          out[i + j] = 1.0f;
        } else {
          out[i + j] = 0.0f;
        }
      }
    }
  }
}