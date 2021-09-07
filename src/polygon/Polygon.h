#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>
#include <vector>
#include <sstream>
#include "util.h"

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
            ( std::ostringstream() << std::dec << x ) ).str()

#define POLYGON_SETS 3

namespace polygon {
  class Polygon : public od::Object {
    public:
      Polygon(bool stereo) :
        mStereo(stereo) {

        for (int i = 0; i < POLYGON_SETS; i++) {
          auto set = VoiceSet { i };
          addVoiceSet(set);
          mVoiceSets.push_back(set);
        }

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addParameter(mGain);
        addOption(mAgcEnabled);
        addParameter(mAgc);

        addInput(mPitchF0);
        addInput(mFilterF0);
        addParameter(mRise);
        addParameter(mFall);

        addInput(mRRGate);
        addParameter(mRRVpo);
        addParameter(mRRCount);
        addParameter(mRRStride);
        addParameter(mRRTotal);
        addParameter(mGateThresh);

        addParameter(mDetune);
        addParameter(mLevel);
        addParameter(mLevelEnv);

        addParameter(mShape);
        addParameter(mShapeEnv);

        addParameter(mPanOffset);
        addParameter(mPanWidth);
      }

#ifndef SWIGLUA
      virtual void process();

      void processInternal() {
        float *gates[POLYGON_SETS * 4];
        for (int i = 0; i < POLYGON_SETS; i++) {
          auto j = i * 4;
          gates[j + 0] = mVoiceSets[i].mA.mGate->buffer();
          gates[j + 1] = mVoiceSets[i].mB.mGate->buffer();
          gates[j + 2] = mVoiceSets[i].mC.mGate->buffer();
          gates[j + 3] = mVoiceSets[i].mD.mGate->buffer();
        }

        auto outLeft    = mOutLeft.buffer();
        auto outRight   = mOutRight.buffer();
        auto gain       = vdup_n_f32(mGain.value());
        auto agcEnabled = isAgcEnabled() ? vdup_n_u32(0xffffffff) : vdup_n_u32(0);

        auto pitchF0  = mPitchF0.buffer();
        auto filterF0 = mFilterF0.buffer();

        const auto rrConst = voice::RoundRobinConstants<POLYGON_SETS> {
          (int)mRRTotal.value(),
          (int)mRRCount.value(),
          (int)mRRStride.value()
        };

        const auto sharedConfig = voice::four::SharedConfig {
          vdupq_n_f32(mDetune.value()),
          vdupq_n_f32(mShape.value()),
          vdupq_n_f32(mLevel.value()),
          vdupq_n_f32(mRise.value()),
          vdupq_n_f32(mFall.value()),
          vdupq_n_f32(mShapeEnv.value()),
          vdupq_n_f32(mLevelEnv.value()),
          vdupq_n_f32(mPanOffset.value())
        };

        const auto rrGate = mRRGate.buffer();
        const auto rrVpo = vdupq_n_f32(mRRVpo.value());

        const auto panOffset = vdupq_n_f32(mPanOffset.value());
        const auto panWidth  = vdupq_n_f32(mPanWidth.value());

        voice::four::VoiceConfig configs[POLYGON_SETS];
        for (int i = 0; i < POLYGON_SETS; i++) {
          configs[i] = voice::four::VoiceConfig {
            mVoiceSets[i].vpo() + rrVpo,
            mVoiceSets[i].pan(panOffset, panWidth)
          };
        }

        for (int i = 0; i < FRAMELENGTH; i++) {
          uint32x4_t _gates[POLYGON_SETS];
          mRoundRobin.process(
            (rrGate[i] > 0.0f ? 0xffffffff : 0) | mManualRRGate,
            rrConst,
            _gates
          );

          for (int j = 0; j < POLYGON_SETS; j++) {
            auto offset = j * 4;
            _gates[j] = _gates[j] | vcgtq_f32(
              util::simd::makeq_f32(
                gates[offset + 0][i], gates[offset + 1][i],
                gates[offset + 2][i], gates[offset + 3][i]
              ),
              vdupq_n_f32(0)
            ) | mVoiceSets[j].mManualGate;

            mVoiceSets[j].markGate(_gates[j]);
            mVoiceSets[j].markEnv(mVoices.env(j));
          }

          auto signal = mVoices.process(
            _gates,
            configs,
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
        inline Voice(int n) {
            mPanIndex = n % 2 ? -n : n + 1;
            mPanSpace = mPanIndex / ((float)POLYGON_SETS * 4.0f);
            mGate = new od::Inlet { SSTR("Gate" << n) };
            mVpo  = new od::Parameter { SSTR("V/Oct" << n) };
          }

        od::Inlet *mGate;
        od::Parameter *mVpo;
        int mPanIndex;
        float mPanSpace;
      };

      inline void addVoice(Voice &voice) {
        addInputFromHeap(voice.mGate);
        addParameterFromHeap(voice.mVpo);
      }

      struct VoiceSet {
        inline VoiceSet(int n) :
          mA(1 + n * 4),
          mB(2 + n * 4),
          mC(3 + n * 4),
          mD(4 + n * 4) { }

        inline float32x4_t vpo() {
          return util::simd::makeq_f32(
            mA.mVpo->value(), mB.mVpo->value(),
            mC.mVpo->value(), mD.mVpo->value()
          );
        }

        inline float32x4_t pan(
          float32x4_t offset,
          float32x4_t width
        ) {
          auto space = util::simd::makeq_f32(
            mA.mPanSpace, mB.mPanSpace,
            mC.mPanSpace, mD.mPanSpace
          );

          //auto dir = vcgtq_f32(space, vdupq_n_f32(0));
          //auto sign = vbslq_f32(dir, vdupq_n_f32(1), vdupq_n_f32(-1));

          return offset + space * width;
        }

        inline void markGate(uint32x4_t gate) {
          mGateCount += vshrq_n_u32(gate, 31);
        }

        inline void markEnv(float32x4_t env) {
          mVoiceEnv = env;
        }

        inline void markManualGate(int lane) {
          switch (lane) {
            case 0: mManualGate = vsetq_lane_u32(0xffffffff, mManualGate, 0); break;
            case 1: mManualGate = vsetq_lane_u32(0xffffffff, mManualGate, 1); break;
            case 2: mManualGate = vsetq_lane_u32(0xffffffff, mManualGate, 2); break;
            case 3: mManualGate = vsetq_lane_u32(0xffffffff, mManualGate, 3); break;
          }
        }

        inline void releaseManualGates() {
          mManualGate = vdupq_n_u32(0);
        }

        inline uint32_t getGateCount(int lane) {
          switch (lane) {
            case 0: return vgetq_lane_u32(mGateCount, 0);
            case 1: return vgetq_lane_u32(mGateCount, 1);
            case 2: return vgetq_lane_u32(mGateCount, 2);
            case 3: return vgetq_lane_u32(mGateCount, 3);
          }
          return 0;
        }

        inline float32_t getEnvLevel(int lane) {
          switch (lane) {
            case 0: return vgetq_lane_f32(mVoiceEnv, 0);
            case 1: return vgetq_lane_f32(mVoiceEnv, 1);
            case 2: return vgetq_lane_f32(mVoiceEnv, 2);
            case 3: return vgetq_lane_f32(mVoiceEnv, 3);
          }
          return 0;
        }

        uint32x4_t mGateCount = vdupq_n_u32(0);
        float32x4_t mVoiceEnv = vdupq_n_f32(0);
        uint32x4_t mManualGate = vdupq_n_u32(0);

        Voice mA, mB, mC, mD;
      };

      inline void addVoiceSet(VoiceSet &set) {
        addVoice(set.mA);
        addVoice(set.mB);
        addVoice(set.mC);
        addVoice(set.mD);
      }

      od::Outlet    mOutLeft    { "Out1" };
      od::Outlet    mOutRight   { "Out2" };
      od::Parameter mGain       { "Output Gain", 1 };
      od::Option    mAgcEnabled { "Enable AGC", 1 };
      od::Parameter mAgc        { "AGC" };

      od::Inlet     mPitchF0    { "Pitch Fundamental" };
      od::Inlet     mFilterF0   { "Filter Fundamental" };
      od::Parameter mRise       { "Rise" };
      od::Parameter mFall       { "Fall" };

      od::Inlet     mRRGate     { "RR Gate" };
      od::Parameter mRRVpo      { "RR V/Oct" };
      od::Parameter mRRCount    { "RR Count", 1 };
      od::Parameter mRRStride   { "RR Stride", 1 };
      od::Parameter mRRTotal    { "RR Total", POLYGON_SETS * 4 };
      od::Parameter mGateThresh { "Gate Threshold", 0.1 };

      od::Parameter mDetune    { "Detune" };
      od::Parameter mLevel     { "Level" };
      od::Parameter mLevelEnv  { "Level Env" };

      od::Parameter mShape     { "Shape" };
      od::Parameter mShapeEnv  { "Shape Env" };

      od::Parameter mPanOffset { "Pan Offset" };
      od::Parameter mPanWidth  { "Pan Width" };

#endif

    int getSetCount() {
      return POLYGON_SETS;
    }

    int getVoiceCount() {
      return POLYGON_SETS * 4;
    }

    int getGateCount(int index) {
      const int wrapped = index % getVoiceCount();
      return mVoiceSets.at(wrapped / 4).getGateCount(wrapped % 4);
    }

    float getVoiceEnv(int index) {
      const int wrapped = index % getVoiceCount();
      return mVoiceSets.at(wrapped / 4).getEnvLevel(wrapped % 4);
    }

    void markRRManualGate() {
      mManualRRGate = 0xffffffff;
    }

    void markManualGate(int index) {
      const int wrapped = index % getVoiceCount();
      mVoiceSets.at(wrapped / 4).markManualGate(wrapped % 4);
    }

    void releaseManualGates() {
      mManualRRGate = 0;
      for (int i = 0; i < POLYGON_SETS; i++) {
        mVoiceSets.at(i).releaseManualGates();
      }
    }

    bool isAgcEnabled() {
      return mAgcEnabled.value() == 1;
    }

    void toggleAgc() {
      if (isAgcEnabled()) mAgcEnabled.set(2);
      else                mAgcEnabled.set(1);
    }

    private:
      std::vector<VoiceSet> mVoiceSets;
      uint32_t mManualRRGate = 0;
      const bool mStereo;
      voice::RoundRobin<POLYGON_SETS> mRoundRobin;
      voice::MultiVoice<POLYGON_SETS> mVoices;
  };
}