#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>

namespace lojik {
  class Pick : public od::Object {
    public:
      Pick() {
        addInput(mIn);
        addInput(mAlt);
        addInput(mPick);
        addOutput(mOut);
      }

      virtual ~Pick() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in   = mIn.buffer();
        float *alt  = mAlt.buffer();
        float *pick = mPick.buffer();
        float *out  = mOut.buffer();

        float32x4_t zero = vdupq_n_f32(0);

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          uint32_t pc[4];
          vst1q_u32(pc, vcgtq_f32(vld1q_f32(pick + i), zero));
          for (int j = 0; j < 4; j++) {
            float next = 0.0f;
            if (pc[j]) next = alt[i + j];
            else       next = in[i + j];
            out[i + j] = next;
          }
        }
      }

      od::Inlet  mIn   { "In" };
      od::Inlet  mAlt  { "Alt" };
      od::Inlet  mPick { "Pick" };
      od::Outlet mOut  { "Out" };
#endif
  };
}
