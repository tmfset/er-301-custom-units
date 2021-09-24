#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>
#include <array>
#include <sstream>
#include "util.h"
#include "Observable.h"

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
            ( std::ostringstream() << std::dec << x ) ).str()

#define LANES 4

#define VOICES GROUPS * LANES

namespace polygon {
  template <int GROUPS>
  class Polygon : public Observable {
    public:
      Polygon() {
        for (int i = 0; i < GROUPS; i++) {
          mGroups[i].init(*this, i);
        }

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addParameter(mGain);
        addOption(mAgcEnabled);
        addParameter(mAgc);

        addInput(mPitchF0);
        addInput(mPeak);
        addParameter(mRise);
        addParameter(mFall);

        addInput(mRRGate);
        addParameter(mRRVpo);
        addOption(mRRVpoTrack);
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

      void process() {
        float *gates[VOICES];
        for (int i = 0; i < VOICES; i++) {
          gates[i] = voice(i).gateBuffer();
        }

        auto outLeft    = mOutLeft.buffer();
        auto outRight   = mOutRight.buffer();
        auto gain       = vdup_n_f32(mGain.value());
        auto agcEnabled = vdup_n_u32(util::bcvt(isAgcEnabled()));
        auto vpoTracked = isVpoTracked();

        auto pitchF0 = mPitchF0.buffer();
        auto peak    = mPeak.buffer();

        const auto rrConst = voice::RoundRobinConstants<GROUPS> {
          (int)mRRTotal.value(),
          (int)mRRCount.value(),
          (int)mRRStride.value()
        };
        mRoundRobinConst = rrConst;

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
        const auto rrManualGate = mManualRRGate;
        const auto gateThresh = mGateThresh.value();

        const auto panOffset = mPanOffset.value();
        const auto panWidth  = mPanWidth.value();

        auto rrOffsetAll = vdupq_n_f32(0);

        if (vpoTracked) {
          markVpoOffset(mRoundRobin.mCurrent, mRRVpo.value());
        } else {
          rrOffsetAll = vdupq_n_f32(mRRVpo.value());
        }

        mAgc.hardSet(mVoices.agcDb());

        uint32x4_t manualGates[GROUPS];
        voice::four::VoiceConfig configs[GROUPS];
        for (int i = 0; i < GROUPS; i++) {
          Group &g = group(i);

          g.markEnvLevel(mVoices.envLevel(i));

          manualGates[i] = g.manualGate();
          configs[i] = voice::four::VoiceConfig { rrOffsetAll + g.vpo(), g.pan(panOffset, panWidth) };

          if (mReleaseManualGatesRequested) {
            mManualRRGate = 0;
            g.clearManualGates();
          }
        }

        mReleaseManualGatesRequested = false;

        for (int i = 0; i < FRAMELENGTH; i++) {
          uint32x4_t _gates[GROUPS];
          const uint32_t _rrGate = util::fcgt(rrGate[i], gateThresh) | rrManualGate;
          mRoundRobin.process(_rrGate, rrConst, _gates);

          for (int j = 0; j < GROUPS; j++) {
            auto offset = j * LANES;
            _gates[j] = _gates[j] | manualGates[j] | vcgtq_f32(
              util::simd::makeq_f32(
                gates[offset + 0][i], gates[offset + 1][i],
                gates[offset + 2][i], gates[offset + 3][i]
              ),
              vdupq_n_f32(gateThresh)
            );
          }

          auto signal = mVoices.process(
            _gates,
            configs,
            vdupq_n_f32(pitchF0[i]),
            vdupq_n_f32(peak[i]),
            gain,
            agcEnabled,
            sharedConfig
          );

          outLeft[i] = vget_lane_f32(signal, 0);
          outRight[i] = vget_lane_f32(signal, 1);
        }
      }

      od::Outlet    mOutLeft    { "Out1" };
      od::Outlet    mOutRight   { "Out2" };
      od::Parameter mGain       { "Output Gain", 1 };
      od::Option    mAgcEnabled { "Enable AGC", 1 };
      od::Parameter mAgc        { "AGC" };

      od::Inlet     mPitchF0    { "Pitch Fundamental" };
      od::Inlet     mPeak       { "Peak" };
      od::Parameter mRise       { "Rise" };
      od::Parameter mFall       { "Fall" };

      od::Inlet     mRRGate     { "RR Gate" };
      od::Parameter mRRVpo      { "RR V/Oct" };
      od::Option    mRRVpoTrack { "RR V/Oct Track", 1 };
      od::Parameter mRRCount    { "RR Count", 1 };
      od::Parameter mRRStride   { "RR Stride", 1 };
      od::Parameter mRRTotal    { "RR Total", VOICES };
      od::Parameter mGateThresh { "Gate Threshold", 0.1 };

      od::Parameter mDetune    { "Detune" };
      od::Parameter mLevel     { "Level" };
      od::Parameter mLevelEnv  { "Level Env" };

      od::Parameter mShape     { "Shape" };
      od::Parameter mShapeEnv  { "Shape Env" };

      od::Parameter mPanOffset { "Pan Offset" };
      od::Parameter mPanWidth  { "Pan Width" };

#endif

    bool isVoiceArmed(int i) {
      return mRoundRobinConst.isArmed(mRoundRobin.mCurrent, i);
    }

    int groups() { return GROUPS; }
    int voices() { return VOICES; }

    od::Parameter* vpoRoundRobin() {
      return &mRRVpo;
    }

    od::Parameter* vpoDirect(int index) {
      return voice(index).vpoDirect();
    }

    od::Parameter* vpoOffset(int index) {
      return voice(index).vpoOffset();
    }

    float envLevel(int voice) {
      return this->voice(voice).envLevel();
    }

    void markRRManualGate() {
      mManualRRGate = util::bcvt(true);
    }

    void markManualGate(int index) {
      voice(index).markManualGate();
    }

    void releaseManualGates() {
      mReleaseManualGatesRequested = true;
    }

    void markVpoOffset(int index, float value) {
      voice(index).markVpoOffset(value);
    }

    bool isVpoTracked() {
      return mRRVpoTrack.value() == 1;
    }

    bool isAgcEnabled() {
      return mAgcEnabled.value() == 1;
    }

    void toggleAgc() {
      if (isAgcEnabled()) mAgcEnabled.set(2);
      else                mAgcEnabled.set(1);
    }

    private:
      class Voice {
        public:
          inline void init(Polygon &obj, int group, int lane) {
            auto n = (group * LANES) + lane + 1;

            auto panDegree = n % 2 ? -n : n + 1;
            mPanSpace = panDegree / (float)obj.voices();

            mGate       = new od::Inlet     { SSTR("Gate" << n) };
            mVpoDirect  = new od::Parameter { SSTR("V/Oct" << n) };
            mVpoOffset  = new od::Parameter { SSTR("V/Oct Offset" << n) };

            obj.addInputFromHeap(mGate);
            obj.addParameterFromHeap(mVpoDirect);
            obj.addParameterFromHeap(mVpoOffset);
          }

          inline float vpo() { return mVpoDirect->value() + mVpoOffset->value(); }
          inline float pan(float offset, float width) { return offset + mPanSpace * width; }
          inline float envLevel() { return mEnvLevel; }
          inline float* gateBuffer() { return mGate->buffer(); }

          inline void markManualGate()  { mManualGate = true; }
          inline void clearManualGate() { mManualGate = false; }
          inline uint32_t manualGate()  { return util::bcvt(mManualGate); }

          inline void markVpoOffset(float value) { mVpoOffset->hardSet(value); }
          inline void markEnvLevel(float value) { mEnvLevel = value; }

          inline od::Parameter* vpoDirect() { return mVpoDirect; }
          inline od::Parameter* vpoOffset() { return mVpoOffset; }

        private:
          od::Inlet *mGate = 0;
          od::Parameter *mVpoDirect = 0;
          od::Parameter *mVpoOffset = 0;

          bool mManualGate = false;
          float mEnvLevel = 0;
          float mPanSpace = 0;
      };

      class Group {
        public:
          inline void init(Polygon &obj, int group) {
            for (int i = 0; i < LANES; i++) {
              mVoices[i].init(obj, group, i);
            }
          }

          Voice& voice(int lane) {
            return mVoices[lane % LANES];
          }

          inline float32x4_t vpo() {
            float _vpo[LANES];
            for (int i = 0; i < LANES; i++) {
              _vpo[i] = mVoices[i].vpo();
            }
            return vld1q_f32(_vpo);
          }

          inline float32x4_t pan(float offset, float width) {
            float _pan[LANES];
            for (int i = 0; i < LANES; i++) {
              _pan[i] = mVoices[i].pan(offset, width);
            }
            return vld1q_f32(_pan);
          }

          inline uint32x4_t manualGate() {
            uint32_t _gates[LANES];
            for (int i = 0; i < LANES; i++) {
              _gates[i] = mVoices[i].manualGate();
            }
            return vld1q_u32(_gates);
          }

          inline float32_t getEnvLevel(int lane) {
            return mVoices[lane].envLevel();
          }

          inline void markEnvLevel(float32x4_t env) {
            float _env[LANES];
            vst1q_f32(_env, env);
            for (int i = 0; i < LANES; i++) {
              mVoices[i].markEnvLevel(_env[i]);
            }
          }

          inline void markManualGate(int lane) {
            mVoices[lane].markManualGate();
          }

          inline void markVpoOffset(int lane, float value) {
            mVoices[lane].markVpoOffset(value);
          }

          inline void clearManualGates() {
            for (int i = 0; i < LANES; i++) {
              mVoices[i].clearManualGate();
            }
          }

        private:
          std::array<Voice, LANES> mVoices;
      };

      bool mReleaseManualGatesRequested = false;
      uint32_t mManualRRGate = 0;

      std::array<Group, GROUPS> mGroups;
      voice::RoundRobin<GROUPS> mRoundRobin;
      voice::RoundRobinConstants<GROUPS> mRoundRobinConst { GROUPS * 4, 1, 1 };
      voice::MultiVoice<GROUPS> mVoices;

      inline Group& group(int index) {
        return mGroups[index % GROUPS];
      }

      inline Voice& voice(int index) {
        return group(index / LANES).voice(index % LANES);
      }
  };
}