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
      Polygon() {
        size_t vc = 8;

        mVoiceGate.reserve(vc);
        mVoicePitch.reserve(vc);
        mVoicePan.reserve(vc);
        for (int i = 0; i < vc; i++) addVoiceFromHeap(i);

        addInput(mGate);
        addInput(mF0);
        addOutput(mOutLeft);
        addOutput(mOutRight);

        addParameter(mVpo);
        addParameter(mDetune);
        addParameter(mRise);
        addParameter(mFall);
        addParameter(mGain);

        addOption(mAgcEnabled);
        addOption(mAgc);

        addParameter(mShapeEnv);
        addParameter(mShapeBias);
        addInput(mShapeMod);

        addParameter(mLevelEnv);
        addParameter(mLevelBias);
        addInput(mLevelMod);

        addParameter(mPanEnv);
        addParameter(mPanBias);
        addInput(mPanMod);

        addParameter(mCutoffEnv);
        addParameter(mCutoffBias);
        addInput(mCutoffMod);
      }

      inline void addVoiceFromHeap(int i) {
        addVoiceControlFromHeap(mVoiceGate,  "Gate",  i);
        addVoiceControlFromHeap(mVoicePitch, "V/Oct", i);
        addVoiceControlFromHeap(mVoicePan,   "Pan",   i);
      }

      inline void addVoiceControlFromHeap(
        std::vector<od::Inlet*> &stash,
        const char *prefix,
        int i
      ) {
        std::ostringstream name;
        name << prefix << i;
        auto inlet = new od::Inlet { name.str() };

        stash.push_back(inlet);
        addInputFromHeap(inlet);
      }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        int vc = 8;
        float *vGate[vc], *vVpo[vc], *vPan[vc];
        for (int i = 0; i < vc; i++) {
          vGate[i] = mVoiceGate[i]->buffer();
          vVpo[i]  = mVoicePitch[i]->buffer();
          vPan[i]  = mVoicePan[i]->buffer();
        }

        auto gate     = mGate.buffer();
        auto f0       = mF0.buffer();
        auto outLeft  = mOutLeft.buffer();
        auto outRight = mOutRight.buffer();

        auto vpo    = mVpo.value();
        auto detune = mDetune.value();
        auto rise   = mRise.value();
        auto fall   = mFall.value();
        auto gain   = mGain.value();

        auto shapeEnv  = mShapeEnv.value();
        auto shapeBias = mShapeBias.value();
        auto shapeMod  = mShapeMod.buffer();

        auto levelEnv  = mLevelEnv.value();
        auto levelBias = mLevelBias.value();
        auto levelMod  = mLevelMod.buffer();

        auto panEnv  = mPanEnv.value();
        auto panBias = mPanBias.value();
        auto panMod  = mPanMod.buffer();

        auto cutoffEnv  = mCutoffEnv.value();
        auto cutoffBias = mCutoffBias.value();
        auto cutoffMod  = mCutoffMod.buffer();

        mVoices.setLPG(rise, fall, height);

        for (int i = 0; i < FRAMELENGTH; i++) {
          auto _gate = gate[i] > 0.0f ? 0xffffffff : 0;
          mRoundRobin.process(_gate);

          mVoices.control(
            mRoundRobin,
            vdupq_n_f32(vpo[i]),
            vdupq_n_u32(0)
          );

          auto voices = mVoices.process(
            vdupq_n_f32(f0[i]),
            osc::shape::TSP(vdupq_n_f32(shape[i])),
            vdupq_n_f32(subLevel[i]),
            vdupq_n_f32(subDivide[i])
          );

          outLeft[i] = util::simd::sumq_f32(voices) * gain;
          outRight[i] = outLeft[i];
        }
      }

      od::Inlet  mGate      { "Gate" };
      od::Inlet  mF0        { "Fundamental" };
      od::Outlet mOutLeft   { "Out1" };
      od::Outlet mOutRight  { "Out2" };

      od::Parameter mVpo    { "V/Oct" };
      od::Parameter mDetune { "Detune" };
      od::Parameter mRise   { "Rise" };
      od::Parameter mFall   { "Fall" };
      od::Parameter mGain   { "Output Gain" };

      od::Option    mAgcEnabled { "Enable AGC", 1 };
      od::Parameter mAgc        { "AGC" };

      od::Parameter mShapeEnv  { "Shape Env" };
      od::Parameter mShapeBias { "Shape Bias" };
      od::Inlet     mShapeMod  { "Shape Mod" };

      od::Parameter mLevelEnv  { "Level Env" };
      od::Parameter mLevelBias { "Level Bias" };
      od::Inlet     mLevelMod  { "Level Mod" };

      od::Parameter mPanEnv  { "Pan Env" };
      od::Parameter mPanBias { "Pan Bias" };
      od::Inlet     mPanMod  { "Pan Mod" };

      od::Parameter mCutoffEnv  { "Cutoff Env" };
      od::Parameter mCutoffBias { "Cutoff Bias" };
      od::Inlet     mCutoffMod  { "Cutoff Mod" };
      
#endif

    bool isAgcEnabled() {
      return mEnableAgc.value() == 1;
    }

    void toggleAgc() {
      if (isAgcEnabled()) mEnableAgc.set(2);
      else                mEnableAgc.set(1);
    }

    private:
      voice::FourRound mRoundRobin;
      voice::FourVoice mVoices;

      std::vector<od::Inlet*> mVoiceGate;
      std::vector<od::Inlet*> mVoicePitch;
      std::vector<od::Inlet*> mVoicePan;
  };
}