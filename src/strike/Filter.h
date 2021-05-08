#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <util.h>
#include <biquad.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace strike {

  #define STRIKE_FILTER_MODE_LOWPASS 1
  #define STRIKE_FILTER_MODE_HIGHPASS 2
  #define STRIKE_FILTER_MODE_BANDPASS 3

  class Filter : public od::Object {
    public:
      Filter() {
        addInput(mLeftIn);
        addInput(mRightIn);
        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mQ);

        addOutput(mLeftOut);
        addOutput(mRightOut);

        addOption(mMode);
      }

      virtual ~Filter() { }

#ifndef SWIGLUA
      virtual void process();

      template <biquad::FilterType FT> void processType() {
        float *inL  = mLeftIn.buffer();
        float *inR  = mRightIn.buffer();
        float *vpo  = mVpo.buffer();
        float *f0   = mF0.buffer();
        float *gain = mGain.buffer();
        float *q    = mQ.buffer();

        float *outL = mLeftOut.buffer();
        float *outR = mRightOut.buffer();

        util::simd::clamp sClpUnit { -1.0f, 1.0f };
        util::simd::clamp sClpQ    { 0.707107f, 30.0f };
        util::simd::vpo   sVpo     { };
        util::simd::tanh  sTanh    { };

        biquad::simd::Constants sBc { globalConfig.sampleRate };

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto vInL  = vld1q_f32(inL  + i);
          auto vInR  = vld1q_f32(inR  + i);
          auto vVpo  = vld1q_f32(vpo  + i);
          auto vF0   = vld1q_f32(f0   + i);
          auto vGain = vld1q_f32(gain + i);
          auto vQ    = vld1q_f32(q    + i);

          biquad::simd::Coefficients<FT> sBcf {
            sBc,
            sVpo.process(sClpUnit.process(vVpo), vF0),
            sClpQ.process(sClpQ.min + vQ)
          };

          auto left  = vGain * vInL;
          auto right = vGain * vInR;

          auto vOutL = mFilterLeft.process(sBc, sBcf, left);
          auto vOutR = mFilterRight.process(sBc, sBcf, right);

          vst1q_f32(outL + i, sTanh.process(vOutL));
          vst1q_f32(outR + i, sTanh.process(vOutR));
        }
      }

      od::Inlet  mLeftIn   { "Left In" };
      od::Inlet  mRightIn  { "Right In" };
      od::Inlet  mVpo      { "V/Oct" };
      od::Inlet  mF0       { "Fundamental" };
      od::Inlet  mGain     { "Gain" };
      od::Inlet  mQ        { "Resonance" };
      od::Outlet mLeftOut  { "Left Out" };
      od::Outlet mRightOut { "Right Out" };

      od::Option mMode { "Mode", STRIKE_FILTER_MODE_LOWPASS };
#endif

    private:
      biquad::simd::Filter mFilterLeft;
      biquad::simd::Filter mFilterRight;
  };
}
