#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <compressor.h>

namespace strike {
  class CPR : public od::Object {
    public:
      CPR(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });
        }

        addInput(mThreshold);
        addInput(mRatio);
        addInput(mGain);
        addInput(mRise);
        addInput(mFall);

        addOutput(mFollow);
        addOutput(mEOF);
        addOutput(mEOR);
      }

      virtual ~CPR() { }

#ifndef SWIGLUA
      virtual void process();

      template <int CC>
      inline void processInternal() {
        float *in[CC], *out[CC];
        for (int channel = 0; channel < CC; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
        }

        const float *threshold = mThreshold.buffer();
        const float *ratio     = mRatio.buffer();
        const float *gain      = mGain.buffer();
        const float *rise      = mRise.buffer();
        const float *fall      = mFall.buffer();

        float *follow = mFollow.buffer();

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _threshold = vld1q_f32(threshold + i);
          auto _ratio     = vld1q_f32(ratio + i);
          auto _gain      = vld1q_f32(gain + i);
          auto _rise      = vld1q_f32(rise + i);
          auto _fall      = vld1q_f32(fall + i);

          float32x4_t signals[CC];
          for (int c = 0; c < CC; c++) {
            signals[c] = vld1q_f32(in[c] + i);
          }

          auto excite = mCompressor.excite<CC>(signals);
          mCompressor.process(excite, _threshold, _ratio, _rise, _fall);

          for (int c = 0; c < CC; c++) {
            vst1q_f32(out[c] + i, mCompressor.compress(signals[c]) * _gain);
          }

          vst1q_f32(follow + i, mCompressor.mAmount);
        }
      }

      od::Inlet mThreshold { "Threshold" };
      od::Inlet mRatio     { "Ratio" };
      od::Inlet mGain      { "Gain" };
      od::Inlet mRise      { "Rise" };
      od::Inlet mFall      { "Fall" };

      od::Outlet mFollow { "Follow" };
      od::Outlet mEOF    { "EOF" };
      od::Outlet mEOR    { "EOR" };
#endif

    private:
      int mChannelCount = 1;
      compressor::Compressor mCompressor;
  };
}
