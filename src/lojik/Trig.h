#pragma once

#include <od/config.h>
#include <hal/simd.h>
#include <od/objects/Object.h>
#include <sense.h>
#include <util.h>

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
        const float *in  = mIn.buffer();
        float *out = mOut.buffer();

        const auto sense = vdupq_n_f32(getSense(mSense));

        auto enable = mEnable;
        auto trigger = mTrigger;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _in = vld1q_f32(in + i);

          uint32_t _high[4];
          vst1q_u32(_high, vcgtq_f32(_in, sense));

          uint32_t _trig[4];
          for (int i = 0; i < 4; i++) {
            auto high = _high[i];

            trigger = high & enable;
            enable = ~high;

            _trig[i] = trigger;
          }

          auto _out = vcvtq_n_f32_u32(vld1q_u32(_trig), 32);
          vst1q_f32(out + i, _out);
        }

        mEnable = enable;
        mTrigger = trigger;
      }

      od::Inlet  mIn    { "In" };
      od::Outlet mOut   { "Out" };
      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      uint32_t mEnable  = 0;
      uint32_t mTrigger = 0;
  };
}
