#pragma once

#include <od/config.h>
#include <od/objects/Object.h>
#include <hal/simd.h>
#include <voice.h>
#include <array>
#include <sstream>
#include "util.h"
#include "Observable.h"
#include "filter.h"

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

#define LANES 4

#define VOICES GROUPS * LANES

namespace polygon {
  template <int GROUPS>
  class Polygon : public Observable {
    public:
      Polygon() {
        mRRControl.init(*this, 0);

        for (int i = 0; i < GROUPS; i++) {
          group(i).init(*this, i);
        }

        addOutput(mOutLeft);
        addOutput(mOutRight);
        addParameter(mGain);
        addOption(mAgcEnabled);
        addParameter(mAgc);

        addInput(mPitchF0);
        addInput(mFilterF0);

        addOption(mFilterTrack);
        addParameter(mFilterVpo);
        addParameter(mFilterEnv);
        addParameter(mResonance);

        addParameter(mRise);
        addParameter(mFall);

        addOption(mRRVpoTrack);
        addParameter(mRRCount);
        addParameter(mRRStride);
        addParameter(mRRTotal);
        addParameter(mGateThresh);

        addInput(mHardSync);
        addParameter(mDetune);
        addParameter(mMix);
        addParameter(mMixEnv);

        addParameter(mShape);
        addParameter(mShapeEnv);

        addParameter(mPanOffset);
        addParameter(mPanWidth);
      }

#ifndef SWIGLUA

      void process() {
        float *gates[VOICES];
        for (int i = 0; i < VOICES; i++) {
          gates[i] = groupVoice(i).control().gateBuffer();
        }

        auto outLeft    = mOutLeft.buffer();
        auto outRight   = mOutRight.buffer();
        auto gain       = vdup_n_f32(mGain.value());

        auto agcEnabled = vdup_n_u32(util::bcvt(isAgcEnabled()));
        auto vpoTracked = isVpoTracked();

        auto pitchF0  = mPitchF0.buffer();
        auto filterF0 = mFilterF0.buffer();
        auto hardSync = mHardSync.buffer();

        const auto rrConst = voice::RoundRobinConstants<GROUPS> {
          (int)mRRTotal.value(),
          (int)mRRCount.value(),
          (int)mRRStride.value()
        };
        mRoundRobinConst = rrConst;

        const auto sharedConfig = voice::four::SharedConfig {
          vdupq_n_f32(mDetune.value()),
          vdupq_n_f32(mShape.value()),
          vdupq_n_f32(mMix.value()),
          vdupq_n_f32(mRise.value()),
          vdupq_n_f32(mFall.value()),
          vdupq_n_f32(mShapeEnv.value()),
          vdupq_n_f32(mMixEnv.value()),
          vdupq_n_f32(mFilterEnv.value()),
          vdupq_n_f32(mResonance.value()),
          vdupq_n_f32(mFilterVpo.value()),
          vdupq_n_f32(mPanOffset.value()),
          vdupq_n_u32(util::bcvt(isFilterTracked()))
        };

        const auto rrGate = mRRControl.gateBuffer();
        const auto rrManualGate = mRRControl.manualGate();
        const auto gateThresh = mGateThresh.value();

        const auto panOffset = mPanOffset.value();
        const auto panWidth  = mPanWidth.value();

        const auto rrVpo = mRRControl.vpo();
        const auto rrOffsetAll = vdupq_n_f32(vpoTracked ? 0 : rrVpo);
        if (vpoTracked) markVpoOffset(mRoundRobin.mCurrent + 1, rrVpo);

        mAgc.hardSet(mVoices.agcDb());

        std::array<uint32x4_t, GROUPS> manualGates;
        std::array<voice::four::VoiceConfig, GROUPS> configs;

        const bool releaseRequested = mReleaseManualGatesRequested;
        if (releaseRequested) mRRControl.clearManualGate();

        for (int i = 0; i < GROUPS; i++) {
          Group &g = group(i);

          g.markEnvLevel(mVoices.envLevel(i));

          manualGates[i] = g.manualGate();
          configs[i] = voice::four::VoiceConfig {
            rrOffsetAll + g.vpo(),
            g.pan(panOffset, panWidth)
          };

          if (releaseRequested) { g.clearManualGates(); }
        }

        mReleaseManualGatesRequested = false;

        auto _gateThresh = vdupq_n_f32(gateThresh);

        for (int i = 0; i < FRAMELENGTH; i++) {
          std::array<uint32x4_t, GROUPS> _gates;
          const uint32_t _rrGate = util::fcgt(rrGate[i], gateThresh) | rrManualGate;
          mRoundRobin.process(_rrGate, rrConst, _gates);

          for (int j = 0; j < GROUPS; j++) {
            const auto offset = j * LANES;
            _gates[j] = _gates[j] | manualGates[j] | vcgtq_f32(
              util::four::make(
                gates[offset + 0][i], gates[offset + 1][i],
                gates[offset + 2][i], gates[offset + 3][i]
              ),
              _gateThresh
            );
          }

          auto signal = mVoices.process(
            _gates,
            configs,
            vdupq_n_f32(pitchF0[i]),
            vdupq_n_f32(filterF0[i]),
            vcgtq_f32(vdupq_n_f32(hardSync[i]), vdupq_n_f32(0)),
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
      od::Inlet     mFilterF0   { "Filter Fundamental" };

      od::Option    mFilterTrack { "Filter Tracking", 2 };
      od::Parameter mFilterVpo   { "Filter V/Oct" };
      od::Parameter mFilterEnv   { "Filter Env" };
      od::Parameter mResonance   { "Resonance" };

      od::Parameter mRise       { "Rise" };
      od::Parameter mFall       { "Fall" };

      od::Option    mRRVpoTrack { "RR V/Oct Track", 2 };
      od::Parameter mRRCount    { "RR Count", 1 };
      od::Parameter mRRStride   { "RR Stride", 1 };
      od::Parameter mRRTotal    { "RR Total", VOICES };
      od::Parameter mGateThresh { "Gate Threshold", 0.1 };

      od::Inlet     mHardSync { "Hard Sync" };
      od::Parameter mDetune   { "Detune" };
      od::Parameter mMix      { "Mix" };
      od::Parameter mMixEnv   { "Mix Env" };

      od::Parameter mShape     { "Shape" };
      od::Parameter mShapeEnv  { "Shape Env" };

      od::Parameter mPanOffset { "Pan Offset" };
      od::Parameter mPanWidth  { "Pan Width" };

#endif

    bool isVoiceArmed(int i) {
      return mRoundRobinConst.isArmed(mRoundRobin.mCurrent, i);
    }

    bool isVoiceNext(int i) {
      return mRoundRobinConst.isNext(mRoundRobin.mCurrent, i);
    }

    int groups() { return GROUPS; }
    int voices() { return VOICES; }

    od::Parameter* vpoDirect(int index) {
      return voice(index).vpoDirect();
    }

    od::Parameter* vpoOffset(int index) {
      return voice(index).vpoOffset();
    }

    float envLevel(int voice) {
      return groupVoice(voice).envLevel();
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

    bool isFilterTracked() {
      return mFilterTrack.value() == 1;
    }

    bool isAgcEnabled() {
      return mAgcEnabled.value() == 1;
    }

    void toggleAgc() {
      if (isAgcEnabled()) mAgcEnabled.set(2);
      else                mAgcEnabled.set(1);
    }

    private:
      class VoiceControl {
        static std::string concat(const std::string &name, int id) {
          std::stringstream s;
          s << name << id;
          return s.str();
        }

        public:
          inline void init(Polygon &obj, int id) {
            mGate       = new od::Inlet     { concat("Gate", id) };
            mVpoDirect  = new od::Parameter { concat("V/Oct", id) };
            mVpoOffset  = new od::Parameter { concat("V/Oct Offset",id) };

            obj.addInputFromHeap(mGate);
            obj.addParameterFromHeap(mVpoDirect);
            obj.addParameterFromHeap(mVpoOffset);
          }

          inline float vpo() { return mVpoDirect->value() + mVpoOffset->value(); }
          inline float* gateBuffer() { return mGate->buffer(); }

          inline void markVpoOffset(float value) { mVpoOffset->hardSet(value); }
          inline void markManualGate()  { mManualGate = true; }
          inline void clearManualGate() { mManualGate = false; }
          inline uint32_t manualGate()  { return util::bcvt(mManualGate); }

          inline od::Parameter* vpoDirect() { return mVpoDirect; }
          inline od::Parameter* vpoOffset() { return mVpoOffset; }

        private:
          od::Inlet *mGate = 0;
          od::Parameter *mVpoDirect = 0;
          od::Parameter *mVpoOffset = 0;

          bool mManualGate = false;
      };

      class Voice {
        public:
          inline void init(Polygon &obj, int id) {
            auto panDegree = id % 2 ? -id : id + 1;
            mPanSpace = panDegree / (float)obj.voices();
            mControl.init(obj, id);
          }

          inline float pan(float offset, float width) { return offset + mPanSpace * width; }
          inline float envLevel() { return mEnvLevel; }
          inline void markEnvLevel(float value) { mEnvLevel = value; }

          inline VoiceControl& control() { return mControl; }

        private:
          VoiceControl mControl;

          float mEnvLevel = 0;
          float mPanSpace = 0;
      };

      class Group {
        public:
          inline void init(Polygon &obj, int group) {
            for (int lane = 0; lane < LANES; lane++) {
              voice(lane).init(obj, (group * LANES) + lane + 1);
            }
          }

          inline Voice& voice(int lane) {
            return mVoices[lane % LANES];
          }

          inline float32x4_t vpo() {
            float _vpo[LANES];
            for (int lane = 0; lane < LANES; lane++) {
              _vpo[lane] = voice(lane).control().vpo();
            }
            return vld1q_f32(_vpo);
          }

          inline float32x4_t pan(float offset, float width) {
            float _pan[LANES];
            for (int lane = 0; lane < LANES; lane++) {
              _pan[lane] = voice(lane).pan(offset, width);
            }
            return vld1q_f32(_pan);
          }

          inline uint32x4_t manualGate() {
            uint32_t _gates[LANES];
            for (int lane = 0; lane < LANES; lane++) {
              _gates[lane] = voice(lane).control().manualGate();
            }
            return vld1q_u32(_gates);
          }

          inline float32_t getEnvLevel(int lane) {
            return voice(lane).envLevel();
          }

          inline void markEnvLevel(float32x4_t env) {
            float _env[LANES];
            vst1q_f32(_env, env);
            for (int lane = 0; lane < LANES; lane++) {
              voice(lane).markEnvLevel(_env[lane]);
            }
          }

          inline void markManualGate(int lane) {
            voice(lane).control().markManualGate();
          }

          inline void markVpoOffset(int lane, float value) {
            voice(lane).control().markVpoOffset(value);
          }

          inline void clearManualGates() {
            for (int lane = 0; lane < LANES; lane++) {
              voice(lane).control().clearManualGate();
            }
          }

        private:
          std::array<Voice, LANES> mVoices;
      };

      bool mReleaseManualGatesRequested = false;

      VoiceControl mRRControl;
      std::array<Group, GROUPS> mGroups;
      voice::RoundRobin<GROUPS> mRoundRobin;
      voice::RoundRobinConstants<GROUPS> mRoundRobinConst { VOICES, 1, 1 };
      voice::MultiVoice<GROUPS> mVoices;

      inline Group& group(int index) {
        return mGroups[index % GROUPS];
      }

      inline VoiceControl& voice(int index) {
        if (index == 0) return mRRControl;
        return groupVoice(index - 1).control();
      }

      inline Voice& groupVoice(int index) {
        return group(index / LANES).voice(index % LANES);
      }
  };
}