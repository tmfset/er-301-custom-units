#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  class Or : public od::Object {
    public:
      Or() {
        addInput(mIn);
        addOutput(mOut);
        addInput(mGate);
        addOption(mSense);
      }

      virtual ~Or() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        const float *in  = mIn.buffer();
        const float *gate = mGate.buffer();
        float *out   = mOut.buffer();

        const auto sense = common::getSense(mSense);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          const auto _or = vorrq_u32(
            vcgtq_f32(vld1q_f32(in + i), vdupq_n_f32(sense)),
            vcgtq_f32(vld1q_f32(gate + i), vdupq_n_f32(0))
          );

          vst1q_f32(out + i, vcvtq_n_f32_u32(_or, 32));
        }
      }

      od::Inlet  mIn    { "In" };
      od::Outlet mOut   { "Out" };
      od::Inlet  mGate  { "Gate" };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif
  };
}
