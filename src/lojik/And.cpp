#include <And.h>
#include <od/constants.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  And::And() {
    addInput(mIn);
    addInput(mGate);
    addOutput(mOut);

    addOption(mSense);
  }

  And::~And() { }

  void And::process() {
    float *in   = mIn.buffer();
    float *gate = mGate.buffer();
    float *out  = mOut.buffer();

    float32x4_t sense = vdupq_n_f32(getSense(mSense));
    float32x4_t fZero = vdupq_n_f32(0);
    uint32x4_t  uZero = vdupq_n_u32(0);

    for (int i = 0; i < FRAMELENGTH; i += 4) {
      float32x4_t loadIn   = vld1q_f32(in + i);
      float32x4_t loadGate = vld1q_f32(gate + i);

      uint32x4_t isInHigh   = vcgtq_f32(loadIn, sense);
      uint32x4_t isGateHigh = vcgtq_f32(loadGate, fZero);

      // (l * r) > 0
      uint32x4_t _and = vcgtq_u32(vmulq_u32(isInHigh, isGateHigh), uZero);
      vst1q_f32(out + i, vcvtq_n_f32_u32(_and, 32));
    }
  }
}