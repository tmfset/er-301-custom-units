#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <sstream>
#include <vector>
#include <util.h>
#include <osc.h>
#include <filter.h>
#include <stdlib.h>

namespace strike {
  class Strike : public od::Object {
    public:
      Strike(bool stereo) {
        mChannelCount = stereo ? 2 : 1;

        mFilter.reserve(mChannelCount);
        for (int channel = 0; channel < mChannelCount; channel++) {
          std::ostringstream inName;
          inName << "In" << channel + 1;
          addInputFromHeap(new od::Inlet { inName.str() });

          std::ostringstream outName;
          outName << "Out" << channel + 1;
          addOutputFromHeap(new od::Outlet { outName.str() });

          mFilter.push_back(filter::biquad::Filter<2> {});
        }

        addOutput(mEof);
        addOutput(mEor);
        addOutput(mEnv);

        addInput(mTrig);
        addInput(mLoop);
        addInput(mRise);
        addInput(mFall);
        addInput(mBend);
        addInput(mHeight);

        addOption(mBendMode);
      }

      virtual ~Strike() { }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        int cc = mChannelCount;
        float *in[cc], *out[cc];
        filter::biquad::Filter<2> *filter[cc];
        for (int channel = 0; channel < cc; channel++) {
          in[channel]  = getInput(channel)->buffer();
          out[channel] = getOutput(channel)->buffer();
          filter[channel] = &mFilter.at(channel);
        }

        float *eof = mEof.buffer();
        float *eor = mEor.buffer();
        float *env = mEnv.buffer();

        const float *trig   = mTrig.buffer();
        const float *loop   = mLoop.buffer();
        const float *rise   = mRise.buffer();
        const float *fall   = mFall.buffer();
        const float *bend   = mBend.buffer();
        const float *height = mHeight.buffer();

        osc::shape::BendMode bendMode = static_cast<osc::shape::BendMode>(mBendMode.value());

        const auto filterCutoff = vdupq_n_f32(27.5f);

        filter::biquad::Coefficients cf;

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto f = osc::Frequency::riseFall(
            vld1q_f32(rise + i),
            vld1q_f32(fall + i)
          );

          const osc::shape::Bend b { bendMode, vld1q_f32(bend + i), vdupq_n_f32(4) };
          auto e = mEnvelope.process(f, b,
            vcgtq_f32(vld1q_f32(trig + i), vdupq_n_f32(0)),
            vld1q_f32(loop + i));

          auto h = vld1q_f32(height + i);
          cf.update(
            e * h,
            filterCutoff,
            vdupq_n_f32(0),
            filter::biquad::LOWPASS
          );

          for (int c = 0; c < cc; c++) {
            filter[c]->process(cf, vld1q_f32(in[c] + i) * e);
            vst1q_f32(out[c] + i, filter[c]->mode12dB());
          }

          vst1q_f32(eof + i, mEnvelope.eof());
          vst1q_f32(eor + i, mEnvelope.eor());
          vst1q_f32(env + i, e);
        }
      }

      od::Outlet mEof { "EOF" };
      od::Outlet mEor { "EOR" };
      od::Outlet mEnv { "Env" };

      od::Inlet mTrig   { "Trig" };
      od::Inlet mLoop   { "Loop" };

      od::Inlet mRise   { "Rise" };
      od::Inlet mFall   { "Fall" };
      od::Inlet mBend   { "Bend" };
      od::Inlet mHeight { "Height" };

      od::Option mBendMode { "Bend Mode", osc::shape::BEND_NORMAL };
#endif
    private:
      int mChannelCount = 1;
      std::vector<filter::biquad::Filter<2>> mFilter;
      osc::AD mEnvelope;
  };
}