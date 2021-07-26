#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>
#include <sstream>
#include <vector>

namespace polygon {
  class Polygon : public od::Object {
    public:
      Polygon(size_t voiceCount) {
        mRoundRobin = new voice::RoundRobin(voiceCount);

        mVoices.reserve(voiceCount);
        mVoiceGates.reserve(voiceCount);
        mVoicePitches.reserve(voiceCount);

        for (int i = 0; i < mRoundRobin->total(); i++) {
          mVoices.push_back(voice::Voice { i });

          std::ostringstream gateName;
          gateName << "Gate" << i;
          auto voiceGate = new od::Inlet { gateName.str() };
          addInputFromHeap(voiceGate);
          mVoiceGates.push_back(voiceGate);

          std::ostringstream vpoName;
          vpoName << "V/Oct" << i;
          auto voicePitch = new od::Inlet { vpoName.str() };
          addInputFromHeap(voicePitch);
          mVoicePitches.push_back(voicePitch);
        }

        addInput(mGate);
        addInput(mVpo);
        addInput(mF0);
        addInput(mGain);
        addInput(mShape);
        addInput(mSubLevel);
        addInput(mSubDivide);
        addOutput(mOut);

        addParameter(mHeight);
        addParameter(mRise);
        addParameter(mFall);
      }

      virtual ~Polygon() {
        delete mRoundRobin;
      }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        auto out = mOut.buffer();

        const auto gate      = mGate.buffer();
        const auto vpo       = mVpo.buffer();
        const auto f0        = mF0.buffer();
        const auto gain      = mGain.buffer();
        const auto shape     = mShape.buffer();
        const auto subLevel  = mSubLevel.buffer();
        const auto subDivide = mSubDivide.buffer();

        const auto height = mHeight.value();
        const auto rise   = mRise.value();
        const auto fall   = mFall.value();

        int vc = mRoundRobin->total();
        float *vGate[vc], *vVpo[vc];
        for (int i = 0; i < vc; i++) {
          vGate[i] = mVoiceGates[i]->buffer();
          vVpo[i] = mVoicePitches[i]->buffer();
          mVoices.at(i).setLPG(rise, fall, height);
        }

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _gate = vcgtq_f32(vld1q_f32(gate + i), vdupq_n_f32(0));
          auto _vpo  = vld1q_f32(vpo + i);
          auto _gain = vld1q_f32(gain + i);

          auto _f0        = vld1q_f32(f0 + i);
          auto _shape     = osc::shape::TSP(vld1q_f32(shape + i));
          auto _subLevel  = vld1q_f32(subLevel + i);
          auto _subDivide = util::simd::invert(vmaxq_f32(vld1q_f32(subDivide + i), vdupq_n_f32(0.001)));

          auto _out = vdupq_n_f32(0);
          auto totalEnv = vdupq_n_f32(0);

          mRoundRobin->process(_gate);

          for (int v = 0; v < vc; v++) {
            voice::Voice& voice = mVoices.at(v);
            voice.control(*mRoundRobin, _gate, _vpo);
            auto voiceOut = voice.process(_f0, _shape, _subLevel, _subDivide);
            totalEnv += voice.getEnvLevel();
            _out = _out + voiceOut;
          }

          auto scale = util::simd::invert(vmaxq_f32(totalEnv, vdupq_n_f32(1)));

          vst1q_f32(out + i, _out * scale * _gain);
        }
      }

      od::Inlet  mGate      { "Gate" };
      od::Inlet  mVpo       { "V/Oct" };
      od::Inlet  mF0        { "Fundamental" };
      od::Inlet  mGain      { "Gain" };
      od::Inlet  mShape     { "Shape" };
      od::Inlet  mSubLevel  { "Sub Level" };
      od::Inlet  mSubDivide { "Sub Divide" };
      od::Outlet mOut       { "Out" };

      od::Parameter mHeight { "Height" };
      od::Parameter mRise   { "Rise" };
      od::Parameter mFall   { "Fall" };
#endif
    private:
      voice::RoundRobin *mRoundRobin = 0;
      std::vector<voice::Voice> mVoices;
      std::vector<od::Inlet*> mVoiceGates;
      std::vector<od::Inlet*> mVoicePitches;
  };
}