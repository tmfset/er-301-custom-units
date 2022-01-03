#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>

namespace lojik {
  class DLatch : public od::Object {
    public:
      DLatch() {
        addInput(mIn);
        addInput(mClock);
        addInput(mReset);
        addOutput(mOut);
      }

      virtual ~DLatch() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in    = mIn.buffer();
        float *clock = mClock.buffer();
        float *reset = mReset.buffer();
        float *out   = mOut.buffer();

        float32x4_t zero = vdupq_n_f32(0.0f);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t c = vld1q_f32(clock + i);
          float32x4_t r = vld1q_f32(reset + i);

          uint32_t cc[4], rc[4];
          vst1q_u32(cc, vcgtq_f32(c, zero));
          vst1q_u32(rc, vcgtq_f32(r, zero));

          for (int j = 0; j < 4; j++) {
            if (!cc[j] || rc[j]) {
              mCatch = true;
            }

            out[i + j] = mCurrent;

            // Delay by one sample so we can form a chain.
            if (cc[j] && mCatch) {
              mCatch = false;
              mCurrent = in[i + j];
            }
          }
        }
      }

      od::Inlet  mIn    { "In" };
      od::Inlet  mClock { "Clock" };
      od::Inlet  mReset { "Reset" };
      od::Outlet mOut   { "Out" };
#endif

    private:
      float mCurrent = 0.0f;
      bool mCatch = true;
  };
}