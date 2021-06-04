#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <od/objects/Object.h>
#include <sense.h>

namespace lojik {
  class Trig : public od::Object {
    public:
      Trig() {
        addInput(mIn);
        addOutput(mOut);

        addOption(mSense);
      }

      virtual ~Trig() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
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

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mCurrent = 0.0f;
      bool mAllow = true;
  };
}
