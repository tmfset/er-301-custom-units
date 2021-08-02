#pragma once

#include <osc.h>
#include <filter.h>
#include <env.h>
#include <vector>
#include <util.h>
#include <hal/simd.h>
#include <hal/neon.h>

namespace voice {
  struct EightRound {
    inline void setTotal(int total) {
      mTotal = util::clamp(total, 1, 8);
    }

    inline void process(const uint32_t gate) {
      uint32_t active[8];
      vst1q_u32(active + 0, vdupq_n_f32(0));
      vst1q_u32(active + 4, vdupq_n_f32(0));

      auto next = ~gate;
      if (mTrigger.read(next)) mCurrent = (mCurrent + 1) % mTotal;

      active[mCurrent] = 0xffffffff;
      mActive = vld2q_u32(active);
    }

    inline uint32x4_t one() { return vandq_u32(mActive.val[0], mGate); }
    inline uint32x4_t two() { return vandq_u32(mActive.val[1], mGate); }

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
    struct Globals {
      inline void track(const uint32x4_t gate, const Parameters& other) {
        mEnvCoeff.track(gate, other.mEnvCoeff);
        mVpoP.track(gate, other.mVpoP);
        mVpoS.track(gate, other.mVpoS);
        mPan.track(gate, other.mPan);
        mCutoff.track(gate, other.mCutoff);
        mShape.track(gate, other.mShape);
        mLevel.track(gate, other.mLevel);
        mEnvShape.track(gate, other.mEnvShape);
        mEnvPan.track(gate, other.mEnvPan);
      }

      inline void configure(
        const float32x4_t vpo,
        const float32x4_t detune,
        const float32x4_t level,
        const float32x4_t shape,
        const float32x4_t pan,
        const float32x4_t cutoff,
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        mEnvCoeff.configure(rise, fall);
        mVpoP.configure(vpo);
        mVpoS.configure(vpo + detune);
        mPan.set(pan);
        mCutoff.set(cutoff);
        mShape.set(shape);
        mLevel.set(level);
      }

      env::four::Coefficients  mEnvCoeff;
      util::four::Vpo          mVpoP;
      util::four::Vpo          mVpoS;
      util::four::TrackAndHold mPan      { 0 };
      util::four::TrackAndHold mCutoff   { 0 };
      util::four::TrackAndHold mShape    { 0 };
      util::four::TrackAndHold mLevel    { 0 };
    };

    struct Offsets {

      
    };

    struct Modulations {
      inline void configure(
        const float32x4_t f0,
        const float32x4_t cutoff,
        const float32x4_t envCutoff,
        const float32x4_t pan,
        const float32x4_t envPan,
        const float32x4_t shape,
        const float32x4_t envShape,
        const float32x4_t level,
        const float32x4_t envLevel,
      ) {
        mF0        = f0;
        mCutoff    = cutoff;
        mEnvCutoff = envCutoff;
        mPan       = pan;
        mEnvPan    = envPan;
        mShape     = shape;
        mEnvShape  = envShape;
        mLevel     = level;
        mEnvLevel  = envLevel;
      }

      inline float32x4_t pan(const Parameters &params, const float32x4_t env) {
        return modulate(params.mPan, mPan, env, mEnvPan);
      }

      inline float32x4_t shape(const Parameters &params, const float32x4_t env) {
        return modulate(params.mShape, mShape, env, mEnvShape);
      }

      inline float32x4_t level(const Parameters &params, const float32x4_t env) {
        return modulate(params.mLevel, mLevel, env, mEnvLevel);
      }

      inline float32x4_t cutoff(const Parameters &params, const float32x4_t env) {
        return modulate(params.mCutoff, mCutoff, env, mEnvCutoff);
      }

      inline float32x4_t modulate(
        const float32x4_t base,
        const float32x4_t wobble,
        const float32x4_t env,
        const float32x4_t amount
      ) {
        return base + wobble + env * amount;
      }

      float32x4_t mF0 = vdupq_n_f32(0);

      float32x4_t mCutoff = vdupq_n_f32(0);
      float32x4_t mEnvCutoff = vdupq_n_f32(0);

      float32x4_t mPan = vdupq_n_f32(0);
      float32x4_t mEnvPan = vdupq_n_f32(0);

      float32x4_t mShape = vdupq_n_f32(0);
      float32x4_t mEnvShape = vdupq_n_f32(0);

      float32x4_t mLevel = vdupq_n_f32(0);
      float32x4_t mEnvLevel = vdupq_n_f32(0);
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
        const float32x4_t f0,
        const util::four::Vpo &vpoP,
        const util::four::Vpo &vpoS,
        const float32x4_t shape,
        const float32x4_t level
      ) {
        auto uzero = vdupq_n_u32(0);
        auto one   = vdupq_n_f32(1);
        auto two   = vdupq_n_f32(2);

        auto primary = mPrimary.process(vpoP.delta(f0), uzero);
        primary      = osc::shape::triangleToPulse(primary, shape);
        primary      = primary * two - one;

        auto secondary = mSecondary.process(vpoS.delta(f0), uzero);
        secondary      = osc::shape::triangleToPulse(secondary, shape);
        secondary      = secondary * two - one;

        auto mixScale = util::simd::invert(one + level);
        return (primary + secondary * level) * mixScale;
      }

      osc::four::Phase mPrimary;
      osc::four::Phase mSecondary;
    };

    struct Voice {
      inline void track(
        const uint32x4_t gate,
        const Parameters &params
      ) {
        mParams.track(gate, params);
      }

      inline void process(
        const uint32x4_t gate,
        const Modulations &mod
      ) {
        auto env    = mEnvelope.process(gate, mParams.mEnvCoeff);
        auto pan    = mod.pan(mParams, env);
        auto shape  = mod.shape(mParams, env);
        auto level  = mod.level(mParams, env);
        auto cutoff = mod.cutoff(mParams, env);

        mPan.configure(pan);
        mFilter.configure(cutoff);

        auto mix = mOscillator.process(
          mod.mF0,
          mParams.mVpoP,
          mParams.mVpoS,
          shape,
          level,
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

      Parameters mParams;
    };
  }

  struct EightVoice {
    inline void configure(
      const float32x4_t vpo,
      const float32x4_t detune,
      const float32x4_t level,
      const float32x4_t shape,
      const float32x4_t pan,
      const float32x4_t cutoff,
      const float32x4_t rise,
      const float32x4_t fall
    ) {
      mParams.configure(
        vpo,
        detune,
        level,
        shape,
        pan,
        cutoff,
        rise,
        fall
      );
    }

    inline void modulate() {

    }

    inline void process(

    ) {

    }

    four::Parameters mParams;
    EightRound mRound;
    four::Voice mOne;
    four::Voice mTwo;
  };
}