#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  class Not : public od::Object {
    public:
      Not() {
        addInput(mIn);
        addOutput(mOut);

        addOption(mSense);
      }

      virtual ~Not() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in  = mIn.buffer();
        float *out = mOut.buffer();

        float32x4_t sense = vdupq_n_f32(getSense(mSense));
        uint32x4_t  zero  = vdupq_n_u32(0);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          // 0 || !(v > 0)
          float32x4_t v = vld1q_f32(in + i);
          uint32x4_t  n = vornq_u32(zero, vcgtq_f32(v, sense));

          vst1q_f32(out + i, vcvtq_n_f32_u32(n, 32));
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif
  };
}
