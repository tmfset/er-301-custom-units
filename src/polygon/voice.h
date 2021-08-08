#pragma once

#include <od/config.h>
#include <osc.h>
#include <filter.h>
#include <env.h>
#include <vector>
#include "util.h"
#include <hal/simd.h>
#include <hal/neon.h>

namespace voice {
  struct EightRound {
    inline void setTotal(int total) {
      mTotal = util::clamp(total, 1, 8);
    }

    inline uint32x4x2_t process(const uint32_t gate, const int count) {
      auto _gate = vdupq_n_u32(gate);

      uint32_t active[8];
      vst1q_u32(active + 0, vdupq_n_u32(0));
      vst1q_u32(active + 4, vdupq_n_u32(0));

      auto current = mCurrent;

      auto next = ~gate;
      if (mTrigger.read(next)) {
        current = (current + 1) % mTotal;
      }

      for (int i = 0; i < count; i++) {
        active[(current + i) % mTotal] = 0xffffffff;
      }

      mCurrent = current;

      auto _active = vld2q_u32(active);
      _active.val[0] = _active.val[0] * _gate;
      _active.val[1] = _active.val[1] * _gate;
      return _active;
    }

    //inline uint32x4_t roundAD() { return mActive.val[0] & mGate; }
    //inline uint32x4_t roundEH() { return mActive.val[1] & mGate; }

    //uint32x4_t mGate;
    //uint32x4x2_t mActive;

    int mCurrent = 0;
    int mTotal = 8;
    util::Trigger mTrigger;
  };

  struct FourRound {
    inline void process(
      const uint32_t gate
    ) {
      uint32_t active[4];
      vst1q_u32(active, vdupq_n_u32(0));

      if (mTrigger.read(~gate)) {
        mCurrent = (mCurrent + 1) % 4;
      }
      active[mCurrent] = 0xffffffff;

      mGate = vandq_u32(vld1q_u32(active), vdupq_n_u32(gate));
    }

    inline uint32x4_t gate() const { return mGate; }

    int mCurrent = 0;
    uint32x4_t mGate = vdupq_n_u32(0);
    util::Trigger mTrigger;
  };

  namespace four {
    struct SharedParams {
      float32x4_t mRise;
      float32x4_t mFall;
      float32x4_t mShapeEnv;
      float32x4_t mLevelEnv;
      float32x4_t mPanEnv;
    };

    struct SharedConfig {
      inline void configure(const SharedParams& params) {
        mEnvCoeff.configure(params.mRise, params.mFall);
        mAgcCoeff.configure(vdupq_n_f32(globalConfig.samplePeriod), params.mFall + params.mRise);
        mShapeEnv.set(params.mShapeEnv);
        mLevelEnv.set(params.mLevelEnv);
        mPanEnv.set(params.mPanEnv);
      }

      inline void track(const uint32x4_t gate, const SharedConfig& other) {
        mEnvCoeff.track(gate, other.mEnvCoeff);
        mShapeEnv.track(gate, other.mShapeEnv);
        mLevelEnv.track(gate, other.mLevelEnv);
        mPanEnv.track(gate, other.mPanEnv);
      }

      env::four::Coefficients mEnvCoeff;
      env::four::Coefficients mAgcCoeff;
      util::four::TrackAndHold mShapeEnv { 0 };
      util::four::TrackAndHold mLevelEnv { 0 };
      util::four::TrackAndHold mPanEnv { 0 };
    };

    struct VoiceParams {
      inline void add(const VoiceParams& other) {
        mVpo    = mVpo + other.mVpo;
        mDetune = mDetune + other.mDetune;
        mCutoff = mCutoff + other.mCutoff;
        mShape  = mShape + other.mShape;
        mLevel  = mLevel + other.mLevel;
        mPan    = mPan + other.mPan;
      }

      float32x4_t mVpo;
      float32x4_t mDetune;
      float32x4_t mCutoff;
      float32x4_t mShape;
      float32x4_t mLevel;
      float32x4_t mPan;
    };

    struct VoiceConfig {
      inline void configure(const VoiceParams& params) {
        mVpo.configure(params.mVpo);
        mDetune.configure(params.mVpo + params.mDetune);
        mCutoff.configure(params.mCutoff);
        mShape.set(params.mShape);
        mLevel.set(params.mLevel);
        mPan.set(params.mPan);
      }

      inline void track(const uint32x4_t gate, const VoiceConfig& other) {
        mVpo.track(gate, other.mVpo);
        mDetune.track(gate, other.mDetune);
        mCutoff.track(gate, other.mCutoff);
        mShape.track(gate, other.mShape);
        mLevel.track(gate, other.mLevel);
        mPan.track(gate, other.mPan);
      }

      util::four::Vpo mVpo;
      util::four::Vpo mDetune;
      util::four::Vpo mCutoff;
      util::four::TrackAndHold mShape { 0 };
      util::four::TrackAndHold mLevel { 0 };
      util::four::TrackAndHold mPan { 0 };

      inline float32x4_t cutoff(const float32x4_t env, const float32x4_t f0) {
        return mCutoff.freq(f0) * env;
      }

      inline float32x4_t shape(const SharedConfig& shared, const float32x4_t env) {
        return mShape.value() + shared.mShapeEnv.value() * env;
      }

      inline float32x4_t level(const SharedConfig& shared, const float32x4_t env) {
        return mLevel.value() + shared.mLevelEnv.value() * env;
      }

      inline float32x4_t pan(const SharedConfig& shared, const float32x4_t env) {
        return mPan.value() + shared.mPanEnv.value() * env;
      }
    };

    struct Pan {
      inline void configure(float32x4_t amount) {
        auto half = vdupq_n_f32(0.5);
        auto one  = vdupq_n_f32(1);

        mRight = amount * half + half;
        mLeft  = one - mRight;
      }

      inline float32x4_t left(const float32x4_t value) const { return value * mLeft; }
      inline float32x4_t right(const float32x4_t value) const { return value * mRight; }

      float32x4_t mLeft = vdupq_n_f32(0);
      float32x4_t mRight = vdupq_n_f32(0);
    };

    struct Oscillator {
      inline float32x4_t process(
        const float32x4_t delta1,
        const float32x4_t delta2,
        const float32x4_t shape,
        const float32x4_t level
      ) {
        auto uzero = vdupq_n_u32(0);
        auto one   = vdupq_n_f32(1);
        auto two   = vdupq_n_f32(2);

        auto primary = mPhase1.process(delta1, uzero);
        primary      = osc::shape::triangleToPulse(primary, shape);
        primary      = primary * two - one;

        auto secondary = mPhase2.process(delta2, uzero);
        secondary      = osc::shape::triangleToPulse(secondary, shape);
        secondary      = secondary * two - one;

        auto mixScale = util::simd::invert(one + level);
        return (primary + secondary * level) * mixScale;
      }

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

      inline float32x2_t process(
        const uint32x4_t gate,
        const float32x4_t pitchF0,
        const float32x4_t filterF0
      ) {
        auto env    = mEnvelope.process(gate, mShared.mEnvCoeff);

        auto cutoff = mConfig.cutoff(env, filterF0);
        auto shape  = mConfig.shape(mShared, env);
        auto level  = mConfig.level(mShared, env);
        auto pan    = mConfig.pan(mShared, env);

        mPan.configure(pan);
        mFilter.configure(cutoff);

        auto mix = mOscillator.process(
          mConfig.mVpo.delta(pitchF0),
          mConfig.mDetune.delta(pitchF0),
          shape,
          level
        );

        mSignal = mFilter.process(mix * env);
      }

      inline float32x4_t mono() const { return mSignal; }
      inline float32x4_t left() const { return mPan.left(mSignal); }
      inline float32x4_t right() const { return mPan.right(mSignal); }

      float32x4_t mSignal;

      Pan mPan;
      Oscillator mOscillator;
      env::four::SlewEnvelope mEnvelope;
      filter::svf::four::Lowpass mFilter;
      VoiceConfig mConfig;
      SharedConfig mShared;
    };
  }

  struct EightVoice {
    inline void configure(
      const four::SharedParams& shared,
      const four::VoiceParams& paramsRR,
      four::VoiceParams& paramsAD,
      four::VoiceParams& paramsEH
    ) {
      paramsAD.add(paramsRR);
      paramsEH.add(paramsRR);
      mShared.configure(shared);
      mConfigAD.configure(paramsAD);
      mConfigEH.configure(paramsEH);
    }

    inline void process(
      const uint32_t gateRR,
      const uint32x4_t directGateAD,
      const uint32x4_t directGateEH,
      const float32x4_t pitchF0,
      const float32x4_t filterF0
    ) {
      auto gate = mRound.process(gateRR, 1);
      auto gateAD = gate.val[0];//mRound.roundAD() | directGateAD;
      auto gateEH = gate.val[1];//mRound.roundEH() | directGateEH;

      mVoiceAD.track(gateAD, mConfigAD, mShared);
      mVoiceEH.track(gateEH, mConfigEH, mShared);

      mVoiceAD.process(gateAD, pitchF0, filterF0);
      mVoiceEH.process(gateEH, pitchF0, filterF0);
    }

    inline float mono() const {
      auto monoAD = mVoiceAD.mono();
      auto monoEH = mVoiceEH.mono();
      auto mix = util::simd::sumq_f32(monoAD + monoEH);
      return mix;
    }

    inline float32x2_t stereo() const {
      return util::simd::make_f32(
        util::simd::sumq_f32(mVoiceAD.left() + mVoiceEH.left()),
        util::simd::sumq_f32(mVoiceAD.right() + mVoiceEH.right())
      );
    }

    inline float32x2_t stereoAgc(float32x4_t gain, uint32x4_t agcEnabled) {
      auto signal = stereo();

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

    float32x4_t mAppliedAgc;
    four::SharedConfig mShared;
    four::VoiceConfig mConfigAD;
    four::VoiceConfig mConfigEH;
    EightRound mRound;
    four::Voice mVoiceAD;
    four::Voice mVoiceEH;
    env::four::EnvFollower mAgcFollower;
  };
}