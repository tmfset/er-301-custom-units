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
        addVoice(mVoiceRR);

        addVoice(mVoiceA);
        addVoice(mVoiceB);
        addVoice(mVoiceC);
        addVoice(mVoiceD);

        addVoice(mVoiceE);
        addVoice(mVoiceF);
        addVoice(mVoiceG);
        addVoice(mVoiceH);

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addParameter(mGain);
        addOption(mAgcEnabled);
        addParameter(mAgc);

        addInput(mPitchF0);
        addInput(mFilterF0);

        addParameter(mRise);
        addParameter(mFall);
        addParameter(mCutoffEnv);
        addParameter(mShapeEnv);
        addParameter(mLevelEnv);
        addParameter(mPanEnv);
      }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {

        auto gateRR = mVoiceRR.mGate.buffer();

        auto gateA = mVoiceA.mGate.buffer();
        auto gateB = mVoiceB.mGate.buffer();
        auto gateC = mVoiceC.mGate.buffer();
        auto gateD = mVoiceD.mGate.buffer();

        auto gateE = mVoiceE.mGate.buffer();
        auto gateF = mVoiceF.mGate.buffer();
        auto gateG = mVoiceG.mGate.buffer();
        auto gateH = mVoiceH.mGate.buffer();

        auto outLeft    = mOutLeft.buffer();
        auto outRight   = mOutRight.buffer();
        auto gain       = mGain.value();
        auto agcEnabled = isAgcEnabled();

        auto pitchF0  = mPitchF0.buffer();
        auto filterF0 = mFilterF0.buffer();

        auto rise      = mRise.value();
        auto fall      = mFall.value();
        auto cutoffEnv = mCutoffEnv.value();
        auto shapeEnv  = mShapeEnv.value();
        auto levelEnv  = mLevelEnv.value();
        auto panEnv    = mPanEnv.value();

        auto paramsRR = voice::four::Parameters {
          vdupq_n_f32(mVoiceRR.mVpo.value()),
          vdupq_n_f32(mVoiceRR.mDetune.value()),
          vdupq_n_f32(mVoiceRR.mCutoff.value()),
          vdupq_n_f32(mVoiceRR.mShape.value()),
          vdupq_n_f32(mVoiceRR.mLevel.value()),
          vdupq_n_f32(mVoiceRR.mPan.value()),
          vdupq_n_f32(rise),
          vdupq_n_f32(fall),
          vdupq_n_f32(cutoffEnv),
          vdupq_n_f32(shapeEnv),
          vdupq_n_f32(levelEnv),
          vdupq_n_f32(panEnv)
        };

        auto paramsAD = voice::four::Parameters {
          vpo(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          detune(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          cutoff(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          shape(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          level(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          pan(mVoiceA, mVoiceB, mVoiceC, mVoiceD),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0)
        };

        auto paramsEH = voice::four::Parameters {
          vpo(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          detune(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          cutoff(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          shape(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          level(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          pan(mVoiceE, mVoiceF, mVoiceG, mVoiceH),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0),
          vdupq_n_f32(0)
        };

        mVoices.configure(paramsRR, paramsAD, paramsEH);

        auto zero = vdupq_n_f32(0);

        for (int i = 0; i < FRAMELENGTH; i++) {
          auto _gateRR = gateRR[i] > 0.0f ? 0xffffffff : 0;
          auto _gateAD = vcgtq_f32(util::simd::makeq_f32(gateA[i], gateB[i], gateC[i], gateD[i]), zero);
          auto _gateEH = vcgtq_f32(util::simd::makeq_f32(gateE[i], gateF[i], gateG[i], gateH[i]), zero);

          auto _pitchF0  = vdupq_n_f32(pitchF0[i]);
          auto _filterF0 = vdupq_n_f32(filterF0[i]);

          mVoices.process(
            _gateRR,
            _gateAD,
            _gateEH,
            _pitchF0,
            _filterF0
          );

          outLeft[i] = mVoices.left() * gain;
          outRight[i] = mVoices.right() * gain;
        }
      }

      struct Voice {
        inline Voice(const std::string &name) :
          mGate("Gate " + name),
          mVpo("V/Oct " + name),
          mDetune("Detune " + name),
          mCutoff("Cutoff " + name),
          mShape("Shape " + name),
          mLevel("Level " + name),
          mPan("Pan " + name) { }

        od::Inlet mGate;
        od::Parameter mVpo;
        od::Parameter mDetune;
        od::Parameter mCutoff;
        od::Parameter mShape;
        od::Parameter mLevel;
        od::Parameter mPan;
      };

      inline void addVoice(Voice &voice) {
        addInput(voice.mGate);
        addParameter(voice.mVpo);
        addParameter(voice.mDetune);
        addParameter(voice.mCutoff);
        addParameter(voice.mShape);
        addParameter(voice.mLevel);
        addParameter(voice.mPan);
      }

      static inline float32x4_t vparam(od::Parameter &a, od::Parameter &b, od::Parameter &c, od::Parameter &d) {
        return util::simd::makeq_f32(a.value(), b.value(), c.value(), d.value());
      }

      static inline float32x4_t vpo(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mVpo, b.mVpo, c.mVpo, d.mVpo);
      }

      static inline float32x4_t detune(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mDetune, b.mDetune, c.mDetune, d.mDetune);
      }

      static inline float32x4_t cutoff(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mCutoff, b.mCutoff, c.mCutoff, d.mCutoff);
      }

      static inline float32x4_t shape(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mShape, b.mShape, c.mShape, d.mShape);
      }

      static inline float32x4_t level(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mLevel, b.mLevel, c.mLevel, d.mLevel);
      }

      static inline float32x4_t pan(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mPan, b.mPan, c.mPan, d.mPan);
      }

      Voice mVoiceRR { "RR" };

      Voice mVoiceA { "A" };
      Voice mVoiceB { "B" };
      Voice mVoiceC { "C" };
      Voice mVoiceD { "D" };

      Voice mVoiceE { "E" };
      Voice mVoiceF { "F" };
      Voice mVoiceG { "G" };
      Voice mVoiceH { "H" };

      od::Outlet    mOutLeft    { "Out1" };
      od::Outlet    mOutRight   { "Out2" };
      od::Parameter mGain       { "Output Gain", 1 };
      od::Option    mAgcEnabled { "Enable AGC", 1 };
      od::Parameter mAgc        { "AGC" };

      od::Inlet mPitchF0  { "Pitch Fundamental" };
      od::Inlet mFilterF0 { "Filter Fundamental" };

      od::Parameter mRise      { "Rise" };
      od::Parameter mFall      { "Fall" };
      od::Parameter mCutoffEnv { "Cutoff Env" };
      od::Parameter mShapeEnv  { "Shape Env" };
      od::Parameter mLevelEnv  { "Level Env" };
      od::Parameter mPanEnv    { "Pan Env" };

#endif

    bool isAgcEnabled() {
      return mAgcEnabled.value() == 1;
    }

    void toggleAgc() {
      if (isAgcEnabled()) mAgcEnabled.set(2);
      else                mAgcEnabled.set(1);
    }

    private:
      voice::EightVoice mVoices;
  };
}