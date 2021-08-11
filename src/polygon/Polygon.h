#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>
#include <sstream>
#include <vector>
#include "util.h"

#define POLYGON_SETS 2

namespace polygon {
  class Polygon : public od::Object {
    public:
      Polygon(bool stereo) :
        mStereo(stereo) {

        addVoice(mVoiceRR);

        for (int i = 0; i < POLYGON_SETS; i++) {
          mFourVoice.push_back(FourVoice { char('A' + i) });
        }

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addParameter(mGain);
        addOption(mAgcEnabled);
        addParameter(mAgc);

        addInput(mPitchF0);
        addInput(mFilterF0);

        addParameter(mDetune);
        addParameter(mShape);
        addParameter(mLevel);

        addParameter(mRise);
        addParameter(mFall);
        addParameter(mShapeEnv);
        addParameter(mLevelEnv);
        addParameter(mPanEnv);
      }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {

        auto gateRR = mVoiceRR.mGate.buffer();

        float *gates[POLYGON_SETS * 4];
        for (int i = 0; i < POLYGON_SETS; i++) {
          auto j = i * 4;
          gates[j + 0] = mFourVoice[i].mA.mGate.buffer();
          gates[j + 1] = mFourVoice[i].mB.mGate.buffer();
          gates[j + 2] = mFourVoice[i].mC.mGate.buffer();
          gates[j + 3] = mFourVoice[i].mD.mGate.buffer();
        }

        auto outLeft    = mOutLeft.buffer();
        auto outRight   = mOutRight.buffer();
        auto gain       = vdup_n_f32(mGain.value());
        auto agcEnabled = isAgcEnabled() ? vdup_n_u32(0xffffffff) : vdup_n_u32(0);

        auto pitchF0  = mPitchF0.buffer();
        auto filterF0 = mFilterF0.buffer();

        const auto sharedConfig = voice::four::SharedConfig {
          voice::four::SharedParams {
            vdupq_n_f32(mDetune.value()),
            vdupq_n_f32(mShape.value()),
            vdupq_n_f32(mLevel.value()),
            vdupq_n_f32(0),
            vdupq_n_f32(mRise.value()),
            vdupq_n_f32(mFall.value()),
            vdupq_n_f32(mShapeEnv.value()),
            vdupq_n_f32(mLevelEnv.value()),
            vdupq_n_f32(mPanEnv.value())
          }
        };

        const auto paramsRR = voice::four::VoiceParams {
          vdupq_n_f32(mVoiceRR.mVpo.value()),
          vdupq_n_f32(mVoiceRR.mPan.value())
        };

        voice::four::VoiceParams params[POLYGON_SETS];
        for (int i = 0; i < POLYGON_SETS; i++) {
          params[i] = voice::four::VoiceParams {
            mFourVoice[i].vpo(),
            mFourVoice[i].pan()
          };
          params[i].add(paramsRR);
        }

        mVoices.configure(params);

        for (int i = 0; i < FRAMELENGTH; i++) {
          uint32x4_t _gates[POLYGON_SETS];
          for (int j = 0; j < POLYGON_SETS; j++) {
            auto offset = j * 4;
            _gates[j] = vcgtq_f32(
              util::simd::makeq_f32(
                gates[offset + 0][i],
                gates[offset + 1][i],
                gates[offset + 2][i],
                gates[offset + 3][i]
              ),
              vdupq_n_f32(0)
            );
          }

          auto signal = mVoices.process(
            gateRR[i] > 0.0f ? 0xffffffff : 0,
            _gates,
            vdupq_n_f32(pitchF0[i]),
            vdupq_n_f32(filterF0[i]),
            gain,
            agcEnabled,
            sharedConfig
          );

          outLeft[i] = vget_lane_f32(signal, 0);
          outRight[i] = vget_lane_f32(signal, 1);
        }

        mAgc.hardSet(mVoices.agcDb());
      }

      struct Voice {
        inline Voice(const std::string &name) :
          mGate("Gate " + name),
          mVpo("V/Oct " + name),
          mPan("Pan " + name) { }

        od::Inlet mGate;
        od::Parameter mVpo;
        od::Parameter mPan;
      };

      inline void addVoice(Voice &voice) {
        addInput(voice.mGate);
        addParameter(voice.mVpo);
        addParameter(voice.mPan);
      }

      struct FourVoice {
        inline FourVoice(const char &name) :
          mA(name + "A"),
          mB(name + "B"),
          mC(name + "C"),
          mD(name + "D") { }

        inline float32x4_t vpo() {
          return util::simd::makeq_f32(
            mA.mVpo.value(),
            mB.mVpo.value(),
            mC.mVpo.value(),
            mD.mVpo.value()
          );
        }

        inline float32x4_t pan() {
          return util::simd::makeq_f32(
            mA.mPan.value(),
            mB.mPan.value(),
            mC.mPan.value(),
            mD.mPan.value()
          );
        }

        Voice mA;
        Voice mB;
        Voice mC;
        Voice mD;
      };

      inline void addFourVoice(FourVoice &four) {
        addVoice(four.mA);
        addVoice(four.mB);
        addVoice(four.mC);
        addVoice(four.mD);
      }

      static inline float32x4_t vparam(od::Parameter &a, od::Parameter &b, od::Parameter &c, od::Parameter &d) {
        return util::simd::makeq_f32(a.value(), b.value(), c.value(), d.value());
      }

      static inline float32x4_t vpo(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mVpo, b.mVpo, c.mVpo, d.mVpo);
      }

      static inline float32x4_t pan(Voice& a, Voice& b, Voice& c, Voice& d) {
        return vparam(a.mPan, b.mPan, c.mPan, d.mPan);
      }

      Voice mVoiceRR { "RR" };

      std::vector<FourVoice> mFourVoice;

      od::Outlet    mOutLeft    { "Out1" };
      od::Outlet    mOutRight   { "Out2" };
      od::Parameter mGain       { "Output Gain", 1 };
      od::Option    mAgcEnabled { "Enable AGC", 1 };
      od::Parameter mAgc        { "AGC" };

      od::Inlet mPitchF0  { "Pitch Fundamental" };
      od::Inlet mFilterF0 { "Filter Fundamental" };

      od::Parameter mDetune    { "Detune" };
      od::Parameter mShape     { "Shape" };
      od::Parameter mLevel     { "Level" };

      od::Parameter mRise      { "Rise" };
      od::Parameter mFall      { "Fall" };
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
      const bool mStereo;
      voice::MultiVoice<POLYGON_SETS> mVoices;
  };
}