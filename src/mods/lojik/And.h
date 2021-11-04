#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  class And : public od::Object {
    public:
      And() {
        addInput(mIn);
        addOutput(mOut);
        addInput(mGate);
        addOption(mSense);
      }

      virtual ~And() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        const float *in   = mIn.buffer();
        const float *gate = mGate.buffer();
        float *out  = mOut.buffer();

        const auto sense = getSense(mSense);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          const auto _and = vandq_u32(
            vcgtq_f32(vld1q_f32(in + i), vdupq_n_f32(sense)),
            vcgtq_f32(vld1q_f32(gate + i), vdupq_n_f32(0))
          );

          vst1q_f32(out + i, vcvtq_n_f32_u32(_and, 32));
        }
      }

      od::Inlet  mIn    { "In" };
      od::Outlet mOut   { "Out" };
      od::Inlet  mGate  { "Gate" };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif
  };
}
