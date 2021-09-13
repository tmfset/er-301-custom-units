#pragma once

#include <od/config.h>
#include <osc.h>
#include <filter.h>
#include <env.h>
#include <array>
#include "util.h"
#include <hal/simd.h>
#include <hal/neon.h>
#include <memory>

namespace voice {

  template <int GROUPS>
  struct RoundRobinConstants {
    inline RoundRobinConstants(int total, int count, int stride) :
      mTotal(util::clamp(total, 1, max())),
      mCount(util::clamp(count, 1, mTotal)),
      mStride(stride < 1 ? 1 : stride) { }

    inline int max() const { return GROUPS * 4; }
    inline int index(int i) const { return i % mTotal; }
    inline int next(int current) const { return index(current + mStride); }

    int mTotal, mCount, mStride;
  };

  template <int GROUPS>
  struct RoundRobin {
    inline void process(
      const uint32_t gate,
      const RoundRobinConstants<GROUPS>& c,
      uint32x4_t *out
    ) {
      auto current = mCurrent;
      auto step = mTrigger.read(~gate);
      if (step) current = c.next(current);
      mCurrent = current;

      auto _max = c.max();
      uint32_t active[_max];
      for (int i = 0; i < GROUPS; i++)
        vst1q_u32(active + (i * 4), vdupq_n_u32(0));

      for (int i = 0; i < c.mCount; i++)
        active[c.index(current + i)] = 0xffffffff;

      auto _gate = vdupq_n_u32(gate);
      for (int i = 0; i < GROUPS; i++) {
        out[i] = vandq_u32(_gate, vld1q_u32(active + (i * 4)));
      }
    }

    int mCurrent = 0;
    util::Trigger mTrigger;
  };

  namespace four {
    struct SharedConfig {
      inline SharedConfig(
        float32x4_t detune,
        float32x4_t shape,
        float32x4_t level,
        float32x4_t rise,
        float32x4_t fall,
        float32x4_t shapeEnv,
        float32x4_t levelEnv,
        float32x4_t pan
      ) {
        mDetune.configure(detune);
        mEnvCoeff.configure(rise, fall);
        mAgcCoeff.configure(vdup_n_f32(globalConfig.samplePeriod), vget_low_f32(fall + rise));
        mShape = shape;
        mLevel = level;
        mShapeEnv = shapeEnv;
        mLevelEnv = levelEnv;
        mPan = pan;
      }

      util::four::Vpo mDetune;
      env::four::Coefficients mEnvCoeff;
      env::two::Coefficients mAgcCoeff;

      float32x4_t mShape;
      float32x4_t mLevel;
      float32x4_t mShapeEnv;
      float32x4_t mLevelEnv;
      float32x4_t mPan;

      inline float32x4_t cutoff(const float32x4_t env, const float32x4_t f0) const {
        return f0 * env;
      }

      inline float32x4_t shape(const float32x4_t env) const {
        return mShape + mShapeEnv * env;
      }

      inline float32x4_t level(const float32x4_t env) const {
        return mLevel + mLevelEnv * env;
      }

      inline float32x4_t pan(const float32x4_t offset) const {
        return mPan + offset;
      }
    };

    struct VoiceConfig {
      inline VoiceConfig() {}

      inline VoiceConfig(
        const float32x4_t vpo,
        const float32x4_t pan
      ) {
        mVpo.configure(vpo);
        mPan = pan;
      }

      util::four::Vpo mVpo;
      float32x4_t mPan;
    };

    struct VoiceTrack {
      inline VoiceTrack() {}

      inline void track(
        const uint32x4_t gate,
        const VoiceConfig& config,
        const SharedConfig& shared
      ) {
        mVpo.track(gate, config.mVpo);
        mDetune.track(gate, shared.mDetune);
        mPan = config.mPan;
        mEnvCoeff.track(gate, shared.mEnvCoeff);
      }

      util::four::Vpo mVpo;
      util::four::Vpo mDetune;
      float32x4_t mPan;
      env::four::Coefficients mEnvCoeff;
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
        mTracked.track(gate, config, shared);
      }

      inline float32x4x2_t process(
        const uint32x4_t gate,
        const float32x4_t pitchF0,
        const float32x4_t filterF0,
        const SharedConfig& shared
      ) {
        auto env = mEnvelope.process(gate, mTracked.mEnvCoeff);
        mEnvLevel = env;

        mFilter.configure(
          shared.cutoff(env, filterF0)
        );

        auto mix = mOscillator.process(
          mTracked.mVpo.delta(pitchF0),
          mTracked.mDetune.deltaOffset(pitchF0, mTracked.mVpo),
          shared.shape(env),
          shared.level(env),
          gate
        );

        return mPan.process(
          mFilter.process(mix * env),
          shared.pan(mTracked.mPan)
        );
      }

      float32x4_t mEnvLevel = vdupq_n_f32(0);
      float32x4_t getEnvLevel() { return mEnvLevel; }

      Pan mPan;
      Oscillator mOscillator;
      env::four::SlewEnvelope mEnvelope;
      filter::svf::four::Lowpass mFilter;
      VoiceTrack mTracked;
    };
  }

  template <int GROUPS>
  struct MultiVoice {
    inline float32x2_t process(
      const uint32x4_t* gates,
      const four::VoiceConfig* configs,
      const float32x4_t pf0,
      const float32x4_t ff0,
      const float32x2_t gain,
      const uint32x2_t agcEnabled,
      const four::SharedConfig &shared
    ) {
      float32x2_t signal = vdup_n_f32(0);
      for (int i = 0; i < GROUPS; i++) {
        mVoices[i].track(gates[i], configs[i], shared);

        auto out = mVoices[i].process(gates[i], pf0, ff0, shared);
        signal = vadd_f32(signal, util::simd::make_f32(
          util::simd::sumq_f32(out.val[0]),
          util::simd::sumq_f32(out.val[1])
        ));
      }

      auto agc = mAgcFollower.process(signal, shared.mAgcCoeff);
      agc = vmax_f32(agc, vdup_n_f32(1));
      agc = util::simd::invert2(agc);
      mAppliedAgc = agc;

      auto appliedGain = vbsl_f32(agcEnabled, vmul_f32(agc, gain), gain);
      return vmul_f32(appliedGain, signal);
    }

    inline float agcDb() {
      auto x = vmul_f32(vpadd_f32(mAppliedAgc, mAppliedAgc), vdup_n_f32(0.5));
      return util::toDecibels(vget_lane_f32(x, 0));
    }

    inline float32x4_t envLevel(int group) {
      return mVoices[group].getEnvLevel();
    }

    std::array<four::Voice, GROUPS> mVoices;
    float32x2_t mAppliedAgc;
    env::two::EnvFollower mAgcFollower;
  };
}