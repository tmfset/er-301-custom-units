#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>

namespace lojik {
  class Latch : public od::Object {
    public:
      Latch() {
        addInput(mIn);
        addInput(mReset);
        addOutput(mOut);

        addOption(mSense);
      }

      virtual ~Latch() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in    = mIn.buffer();
        float *reset = mReset.buffer();
        float *out   = mOut.buffer();

        float32x4_t sense = vdupq_n_f32(common::getSense(mSense));
        float32x4_t zero  = vdupq_n_f32(0.0f);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t loadIn    = vld1q_f32(in + i);
          float32x4_t loadReset = vld1q_f32(reset + i);

          uint32_t isInHigh[4], isResetHigh[4];
          vst1q_u32(isInHigh,    vcgtq_f32(loadIn, sense));
          vst1q_u32(isResetHigh, vcgtq_f32(loadReset, zero));

          for (int j = 0; j < 4; j++) {
            if (isResetHigh[j]) mCurrent = 0.0f;
            if (isInHigh[j]) mCurrent = 1.0f;

            out[i + j] = mCurrent;
          }
        }
      }

      od::Inlet  mIn    { "In" };
      od::Inlet  mReset { "Reset" };
      od::Outlet mOut   { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      float mCurrent = 0.0f;
  };
}