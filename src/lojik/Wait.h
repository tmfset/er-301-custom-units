#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <sense.h>
#include <OneTime.h>
#include <util.h>

#define WAIT_MODE_LOW  1
#define WAIT_MODE_HIGH 2

namespace lojik {
  class Wait : public od::Object {
    public:
      Wait() {
        addInput(mIn);
        addInput(mCount);
        addInput(mInvert);
        addInput(mArm);
        addOutput(mOut);

        addOption(mSense);
      }

      virtual ~Wait() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *in     = mIn.buffer();
        float *count  = mCount.buffer();
        float *invert = mInvert.buffer();
        float *arm    = mArm.buffer();
        float *out    = mOut.buffer();

        const auto sense = vdupq_n_f32(getSense(mSense));
        const auto zero = vdupq_n_f32(0);

        auto enable = mEnable;
        auto trigger = mTrigger;
        auto isArmed = mIsArmed;
        auto step = mStep;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          float32x4_t loadIn     = vld1q_f32(in + i);
          float32x4_t loadCount  = vld1q_f32(count + i);
          float32x4_t loadInvert = vld1q_f32(invert + i);
          float32x4_t loadArm    = vld1q_f32(arm + i);

          int32_t iCount[4];
          uint32_t isInHigh[4], isArmHigh[4];

          vst1q_s32(iCount,    vcvtq_s32_f32(loadCount));
          vst1q_u32(isArmHigh, vcgtq_f32(loadArm, zero));
          vst1q_u32(isInHigh,  vcgtq_f32(loadIn, sense));

          int32_t _step[4];
          for (int j = 0; j < 4; j++) {
            auto high = isInHigh[j];
            trigger = high & enable;
            enable = ~high;

            isArmed = isArmed | isArmHigh[j];
            if (trigger & isArmed) { step += 1; }

            if (step > iCount[j]) {
              step = 0;
              isArmed = 0;
            }

            _step[j] = step;
          }

          auto open = vcgtq_s32(vld1q_s32(_step), vdupq_n_s32(0));
          auto inverted = vcgtq_f32(loadInvert, zero);
          auto select = vbslq_u32(inverted, open, vmvnq_u32(open));
          auto o = vbslq_f32(select, loadIn, zero);
          vst1q_f32(out + i, o);
        }

        mEnable = enable;
        mTrigger = trigger;
        mIsArmed = isArmed;
        mStep = step;
      }

      od::Inlet  mIn     { "In" };
      od::Inlet  mCount  { "Count" };
      od::Inlet  mInvert { "Invert" };
      od::Inlet  mArm    { "Arm" };
      od::Outlet mOut    { "Out" };

      od::Option mSense { "Sense", INPUT_SENSE_LOW };
#endif

    private:
      uint32_t mEnable  = 0;
      uint32_t mTrigger = 0;
      uint32_t mIsArmed = 0;
      int mStep = 0;
  };
}
