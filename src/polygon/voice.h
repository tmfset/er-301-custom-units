#pragma once

#include <od/config.h>
#include <osc.h>
#include <filter.h>
#include <env.h>
#include <vector>
#include "util.h"
#include <hal/simd.h>
#include <hal/neon.h>
#include <memory>

namespace voice {
  template <int sets>
  struct RoundRobin {
    inline RoundRobin() :
      mTotal(max()),
      mCount(1) { }

    inline int max() const { return sets * 4; }
    inline int index(int i) const { return i % mTotal; }

    inline void configure(int total, int count) {
      mTotal = util::clamp(total, 1, max());
      mCount = util::clamp(count, 1, mTotal);
    }

    inline void process(const uint32_t gate, uint32x4_t *out) {
      auto current = mCurrent;
      auto next = mTrigger.read(~gate);
      if (next)
        current = index(current + 1);
      mCurrent = current;

      auto _max = max();
      uint32_t active[_max];
      for (int i = 0; i < sets; i++) vst1q_u32(active + (i * 4), vdupq_n_u32(0));
      //for (int i = 0; i < _max; i++) active[i] = 0;
      //memset(active, 0, _max);
      for (int i = 0; i < mCount; i++)
        active[index(current + i)] = 0xffffffff;

      auto _gate = vdupq_n_u32(gate);
      for (int i = 0; i < sets; i++) {
        out[i] = vandq_u32(_gate, vld1q_u32(active + (i * 4)));
        //util::simd::print_u32(_gate);
        //util::simd::print_u32(out[i], true);
      }

      //util::simd::print_newline();
    }

    int mTotal, mCount;
    int mCurrent = 0;
    util::Trigger mTrigger;
  };

  namespace four {
    struct SharedParams {
      float32x4_t mDetune;
      float32x4_t mShape;
      float32x4_t mLevel;
      float32x4_t mCutoff;
      float32x4_t mRise;
      float32x4_t mFall;
      float32x4_t mShapeEnv;
      float32x4_t mLevelEnv;
      float32x4_t mPanEnv;
    };

    struct SharedConfig {
      inline void configure(const SharedParams& params) {
        mDetune.configure(params.mDetune);
        mShape.set(params.mShape);
        mLevel.set(params.mLevel);
        mCutoff.configure(params.mCutoff);
        mEnvCoeff.configure(params.mRise, params.mFall);
        mAgcCoeff.configure(vdupq_n_f32(globalConfig.samplePeriod), params.mFall + params.mRise);
        mShapeEnv.set(params.mShapeEnv);
        mLevelEnv.set(params.mLevelEnv);
        mPanEnv.set(params.mPanEnv);
      }

      inline void track(const uint32x4_t gate, const SharedConfig& other) {
        mDetune.track(gate, other.mDetune);
        mShape.track(gate, other.mShape);
        mLevel.track(gate, other.mLevel);
        mCutoff.track(gate, other.mCutoff);
        mEnvCoeff.track(gate, other.mEnvCoeff);
        mShapeEnv.track(gate, other.mShapeEnv);
        mLevelEnv.track(gate, other.mLevelEnv);
        mPanEnv.track(gate, other.mPanEnv);
      }

      util::four::Vpo mDetune;
      util::four::TrackAndHold mShape { 0 };
      util::four::TrackAndHold mLevel { 0 };
      util::four::Vpo mCutoff;
      env::four::Coefficients mEnvCoeff;
      env::four::Coefficients mAgcCoeff;
      util::four::TrackAndHold mShapeEnv { 0 };
      util::four::TrackAndHold mLevelEnv { 0 };
      util::four::TrackAndHold mPanEnv { 0 };

      inline float32x4_t cutoff(const float32x4_t env, const float32x4_t f0) const {
        return mCutoff.freq(f0) * env;
      }

      inline float32x4_t shape(const float32x4_t env) const {
        return mShape.value() + mShapeEnv.value() * env;
      }

      inline float32x4_t level(const float32x4_t env) const {
        return mLevel.value() + mLevelEnv.value() * env;
      }

      inline float32x4_t pan(const float32x4_t offset, const float32x4_t env) const {
        return offset + mPanEnv.value() * env;
      }
    };

    struct VoiceParams {
      inline void add(const VoiceParams& other) {
        mVpo = mVpo + other.mVpo;
        mPan = mPan + other.mPan;
      }

      float32x4_t mVpo;
      float32x4_t mPan;
    };

    struct VoiceConfig {
      inline void configure(const VoiceParams& params) {
        mVpo.configure(params.mVpo);
        mPan.set(params.mPan);
      }

      inline void track(const uint32x4_t gate, const VoiceConfig& other) {
        mVpo.track(gate, other.mVpo);
        mPan.track(gate, other.mPan);
      }

      util::four::Vpo mVpo;
      util::four::TrackAndHold mPan { 0 };
    };

    struct Pan {
      inline float32x4x2_t process(
        const float32x4_t signal,
        const float32x4_t amount
      ) {
        auto half = vdupq_n_f32(0.5);
        auto one  = vdupq_n_f32(1);

        auto rightAmount = amount * half + half;
        auto leftAmount  = one - rightAmount;

        float32x4x2_t output;
        output.val[0] = signal * leftAmount;
        output.val[1] = signal * rightAmount;
        return output;
      }
    };

    struct Oscillator {
      inline float32x4_t process(
        const float32x4_t delta1,
        const float32x4_t delta2,
        const float32x4_t shape,
        const float32x4_t level,
        const uint32x4_t sync
      ) {
        auto one   = vdupq_n_f32(1);
        auto two   = vdupq_n_f32(2);

        auto _sync = mSyncTrigger.read(sync);

        auto t2p = osc::shape::TriangleToPulse {};
        t2p.configure(shape);

        auto primary = mPhase1.process(delta1, _sync);
        primary      = t2p.process(primary);
        primary      = primary * two - one;

        auto secondary = mPhase2.process(delta2, _sync);
        secondary      = t2p.process(secondary);
        secondary      = secondary * two - one;

        auto mixScale = util::simd::invert(one + level);
        return (primary + secondary * level) * mixScale;
      }

      util::four::Trigger mSyncTrigger;
      osc::four::Phase mPhase1;
      osc::four::Phase mPhase2;
    };

    struct Voice {
      inline void track(
        const uint32x4_t gate,
        const VoiceConfig &config,
        const SharedConfig& shared
      ) {
        mConfig.track(gate, config);
        mShared.track(gate, shared);
      }

      inline float32x4x2_t process(
        const uint32x4_t gate,
        const float32x4_t pitchF0,
        const float32x4_t filterF0
      ) {
        auto env    = mEnvelope.process(gate, mShared.mEnvCoeff);

        auto cutoff = mShared.cutoff(env, filterF0);
        auto shape  = mShared.shape(env);
        auto level  = mShared.level(env);
        auto pan    = mShared.pan(mConfig.mPan.value(), env);

        mFilter.configure(cutoff);

        auto mix = mOscillator.process(
          mConfig.mVpo.delta(pitchF0),
          mShared.mDetune.deltaOffset(pitchF0, mConfig.mVpo),
          shape,
          level,
          gate
        );

        return mPan.process(
          mFilter.process(mix * env),
          pan
        );
      }

      Pan mPan;
      Oscillator mOscillator;
      env::four::SlewEnvelope mEnvelope;
      filter::svf::four::Lowpass mFilter;
      VoiceConfig mConfig;
      SharedConfig mShared;
    };
  }

  template <int sets>
  struct MultiVoice {
    inline MultiVoice() :
      mVoices(new four::Voice[sets]),
      mConfigs(new four::VoiceConfig[sets]) {
      for (int i = 0; i < sets; i++) {
        mVoices[i]  = four::Voice { };
        mConfigs[i] = four::VoiceConfig { };
      }
    }

    inline void configure(
      const four::VoiceParams &rr,
      four::VoiceParams* params,
      const four::SharedParams &shared
    ) {
      mShared.configure(shared);
      for (int i = 0; i < sets; i++) {
        params[i].add(rr);
        mConfigs[i].configure(params[i]);
      }
    }

    inline float32x2_t process(
      const uint32_t rr,
      float* gate,
      const float32x4_t pf0,
      const float32x4_t ff0,
      const float32x4_t gain,
      const uint32x4_t agcEnabled
    ) {
      uint32x4_t gates[sets];
      mRoundRobin.process(rr, gates);

      float32x2_t sum = vdup_n_f32(0);

      for (int i = 0; i < sets; i++) {
        auto direct = vcgtq_f32(vld1q_f32(gate + (i * 4)), vdupq_n_f32(0));
        gates[i] = gates[i] | direct;
        mVoices[i].track(gates[i], mConfigs[i], mShared);

        auto signal = mVoices[i].process(gates[i], pf0, ff0);
        sum = vadd_f32(sum, util::simd::make_f32(
          util::simd::sumq_f32(signal.val[0]),
          util::simd::sumq_f32(signal.val[1])
        ));
      }

      return stereoAgc(sum, gain, agcEnabled);
    }

    inline float32x2_t stereoAgc(float32x2_t signal, float32x4_t gain, uint32x4_t agcEnabled) {
      auto input = vcombine_f32(signal, signal);
      auto agc = mAgcFollower.process(input, mShared.mAgcCoeff);
      agc = vmaxq_f32(agc, vdupq_n_f32(1));
      agc = util::simd::invert(agc);
      mAppliedAgc = agc;

      auto appliedGain = vbslq_f32(agcEnabled, agc * gain, gain);
      return vmul_f32(vget_low_f32(appliedGain), signal);
    }

    inline float agcDb() {
      auto agcLeft = vgetq_lane_f32(mAppliedAgc, 0);
      auto agcRight = vgetq_lane_f32(mAppliedAgc, 1);
      auto avg = (agcLeft + agcRight) * 0.5f;
      return util::toDecibels(avg);
    }

    RoundRobin<sets> mRoundRobin;
    four::SharedConfig mShared;
    std::unique_ptr<four::Voice[]> mVoices;
    std::unique_ptr<four::VoiceConfig[]> mConfigs;

    float32x4_t mAppliedAgc;
    env::four::EnvFollower mAgcFollower;
  };
}