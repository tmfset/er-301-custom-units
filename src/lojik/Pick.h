#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>

namespace lojik {
  class Pick : public od::Object {
    public:
      Pick() {
        addInput(mIn);
        addOutput(mOut);
        addInput(mAlt);
        addInput(mPick);
      }

      virtual ~Pick() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in   = mIn.buffer();
        float *alt  = mAlt.buffer();
        float *pick = mPick.buffer();
        float *out  = mOut.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto p = vbslq_f32(
            vcgtq_f32(vld1q_f32(pick + i), vdupq_n_f32(0)),
            vld1q_f32(in + i),
            vld1q_f32(alt + i)
          );

          vst1q_f32(out + i, p);
        }
      }

      od::Inlet  mIn   { "In" };
      od::Outlet mOut  { "Out" };
      od::Inlet  mAlt  { "Alt" };
      od::Inlet  mPick { "Pick" };
#endif
  };
}
