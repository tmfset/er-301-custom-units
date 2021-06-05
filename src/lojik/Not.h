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
        const float *in  = mIn.buffer();
        float *out = mOut.buffer();

        const auto sense = getSense(mSense);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _not = vmvnq_u32(vcgtq_f32(vld1q_f32(in + i), vdupq_n_f32(sense)));
          vst1q_f32(out + i, vcvtq_n_f32_u32(_not, 32));
        }
      }

      od::Inlet  mIn    { "In" };
      od::Outlet mOut   { "Out" };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif
  };
}
