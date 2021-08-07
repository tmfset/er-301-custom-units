#pragma once

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

    inline void process(const uint32_t gate, const int count) {
      uint32_t active[8];
      vst1q_u32(active + 0, vdupq_n_u32(0));
      vst1q_u32(active + 4, vdupq_n_u32(0));

      auto next = ~gate;
      if (mTrigger.read(next)) mCurrent = (mCurrent + 1) % mTotal;

      for (int i = 0; i < count; i++) {
        active[(mCurrent + i) % mTotal] = 0xffffffff;
      }

      mActive = vld2q_u32(active);
    }

    inline uint32x4_t roundAD() { return vandq_u32(mActive.val[0], mGate); }
    inline uint32x4_t roundEH() { return vandq_u32(mActive.val[1], mGate); }

    uint32x4_t mGate;
    uint32x4x2_t mActive;

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
    struct Parameters {
      inline void add(const Parameters& other) {
        mVpo    = mVpo + other.mVpo;
        mDetune = mDetune + other.mDetune;
        mCutoff = mCutoff + other.mCutoff;
        mShape  = mShape + other.mShape;
        mLevel  = mLevel + other.mLevel;
        mPan    = mPan + other.mLevel;

        mRise      = mRise + other.mRise;
        mFall      = mFall + other.mFall;
        mCutoffEnv = mCutoffEnv + other.mCutoffEnv;
        mShapeEnv  = mShapeEnv + other.mShapeEnv;
        mLevelEnv  = mLevelEnv + other.mLevelEnv;
        mPanEnv    = mPanEnv + other.mPanEnv;
      }

      float32x4_t mVpo;
      float32x4_t mDetune;
      float32x4_t mCutoff;
      float32x4_t mShape;
      float32x4_t mLevel;
      float32x4_t mPan;

      float32x4_t mRise;
      float32x4_t mFall;
      float32x4_t mCutoffEnv;
      float32x4_t mShapeEnv;
      float32x4_t mLevelEnv;
      float32x4_t mPanEnv;
    };

    struct Configuration {
      inline void configure(const Parameters& params) {
        mVpo.configure(params.mVpo);
        mDetune.configure(params.mVpo + params.mDetune);
        mCutoff.configure(params.mCutoff);
        mShape.set(params.mShape);
        mLevel.set(params.mLevel);
        mPan.set(params.mPan);

        mEnvCoeff.configure(params.mRise, params.mFall);
        mCutoffEnv.set(params.mCutoffEnv);
        mShapeEnv.set(params.mShapeEnv);
        mLevelEnv.set(params.mLevelEnv);
        mPanEnv.set(params.mPanEnv);
      }

      inline void track(const uint32x4_t gate, const Configuration& other) {
        mVpo.track(gate, other.mVpo);
        mDetune.track(gate, other.mDetune);
        mCutoff.track(gate, other.mCutoff);
        mShape.track(gate, other.mShape);
        mLevel.track(gate, other.mLevel);
        mPan.track(gate, other.mPan);

        mEnvCoeff.track(gate, other.mEnvCoeff);
        mCutoffEnv.track(gate, other.mCutoffEnv);
        mShapeEnv.track(gate, other.mShapeEnv);
        mLevelEnv.track(gate, other.mLevelEnv);
        mPanEnv.track(gate, other.mPanEnv);
      }

      util::four::Vpo mVpo;
      util::four::Vpo mDetune;
      util::four::Vpo mCutoff;
      util::four::TrackAndHold mShape { 0 };
      util::four::TrackAndHold mLevel { 0 };
      util::four::TrackAndHold mPan { 0 };

      env::four::Coefficients mEnvCoeff;
      util::four::TrackAndHold mCutoffEnv { 0 };
      util::four::TrackAndHold mShapeEnv { 0 };
      util::four::TrackAndHold mLevelEnv { 0 };
      util::four::TrackAndHold mPanEnv { 0 };

      inline float32x4_t cutoff(const float32x4_t env, const float32x4_t f0) {
        return mCutoff.delta(f0) * env;
      }

      inline float32x4_t shape(const float32x4_t env) {
        return mShape.value() + mShapeEnv.value() * env;
      }

      inline float32x4_t level(const float32x4_t env) {
        return mLevel.value() + mLevelEnv.value() * env;
      }

      inline float32x4_t pan(const float32x4_t env) {
        return mPan.value() + mPanEnv.value() * env;
      }
    };

    struct Pan {
      inline void configure(const float32x4_t amount) {
        auto half = vdupq_n_f32(0.5);
        auto one  = vdupq_n_f32(1);

        auto normal = (amount + one) * half;
        mLeft  = one - normal;
        mRight = normal;
      }

      inline float32x4_t left(const float32x4_t value) { return value * mLeft; }
      inline float32x4_t right(const float32x4_t value) { return value * mRight; }

      float32x4_t mLeft;
      float32x4_t mRight;
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
      inline void track(const uint32x4_t gate, const Configuration &config) {
        mConfig.track(gate, config);
      }

      inline void process(
        const uint32x4_t gate,
        const float32x4_t pitchF0,
        const float32x4_t filterF0
      ) {
        auto env    = mEnvelope.process(gate, mConfig.mEnvCoeff);

        auto cutoff = mConfig.cutoff(env, filterF0);
        auto shape  = mConfig.shape(env);
        auto level  = mConfig.level(env);
        auto pan    = mConfig.pan(env);

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

      inline float32x4_t mono() { return mSignal; }
      inline float32x4_t left() { return mPan.left(mSignal); }
      inline float32x4_t right() { return mPan.right(mSignal); }

      float32x4_t mSignal;
      Pan mPan;
      Oscillator mOscillator;
      env::four::SlewEnvelope mEnvelope;
      filter::svf::four::Lowpass mFilter;
      Configuration mConfig;
    };
  }

  struct EightVoice {
    inline void configure(
      four::Parameters& paramsRR,
      four::Parameters& paramsAD,
      four::Parameters& paramsEH
    ) {
      paramsAD.add(paramsRR);
      paramsEH.add(paramsRR);
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
      mRound.process(gateRR, 1);
      auto gateAD = mRound.roundAD() | directGateAD;
      auto gateEH = mRound.roundEH() | directGateEH;

      mVoiceAD.track(gateAD, mConfigAD);
      mVoiceEH.track(gateEH, mConfigEH);

      mVoiceAD.process(gateAD, pitchF0, filterF0);
      mVoiceEH.process(gateEH, pitchF0, filterF0);
    }

    inline float mono() {
      auto monoAD = mVoiceAD.mono();
      auto monoEH = mVoiceEH.mono();
      auto mix = util::simd::sumq_f32(monoAD + monoEH);
      return mix;
    }

    inline float left() {
      auto leftAD = mVoiceAD.left();
      auto leftEH = mVoiceEH.left();
      auto mix = util::simd::sumq_f32(leftAD + leftEH);
      return mix;
    }

    inline float right() {
      auto rightAD = mVoiceAD.right();
      auto rightEH = mVoiceEH.right();
      auto mix = util::simd::sumq_f32(rightAD + rightEH);
      return mix;
    }

    four::Configuration mConfigAD;
    four::Configuration mConfigEH;
    EightRound mRound;
    four::Voice mVoiceAD;
    four::Voice mVoiceEH;
  };
}