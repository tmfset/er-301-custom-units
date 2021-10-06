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

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

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

    inline bool isArmed(int current, int query) {
      for (int i = 0; i < mCount; i++)
        if (index(current + i) == query)
          return true;
      return false;
    }

    inline bool isNext(int current, int query) {
      for (int i = 0; i < mCount; i++)
        if (index(current + mStride + i) == query)
          return true;
      return false;
    }

    int mTotal, mCount, mStride;
  };

  template <int GROUPS>
  struct RoundRobin {
    inline void process(
      const uint32_t gate,
      const RoundRobinConstants<GROUPS>& c,
      std::array<uint32x4_t, GROUPS>& out
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
        float32x4_t filterEnv,
        float32x4_t resonance,
        float32x4_t filterVpo,
        float32x4_t pan,
        uint32x4_t  filterTrack
      ) {
        mDetune.configure(detune);
        mFilterMax.configure(filterEnv);
        mFilterVpo.configure(filterVpo);
        mEnvCoeff.configure(rise, fall);
        mAgcCoeff.configure(vdup_n_f32(globalConfig.samplePeriod), vget_low_f32(fall));
        mShape = shape;
        mLevel = level;
        mShapeEnv = shapeEnv;
        mLevelEnv = levelEnv;
        mResonance = util::simd::exp_n_scale(resonance, 0.70710678118f, 100.0f);
        mPan = pan;
        mFilterTrack = filterTrack;
      }

      util::four::Vpo mDetune;
      util::four::Vpo mFilterMax;
      util::four::Vpo mFilterVpo;
      env::four::Coefficients mEnvCoeff;
      env::two::Coefficients mAgcCoeff;

      float32x4_t mShape       = vdupq_n_f32(0);
      float32x4_t mLevel       = vdupq_n_f32(0);
      float32x4_t mShapeEnv    = vdupq_n_f32(0);
      float32x4_t mLevelEnv    = vdupq_n_f32(0);
      float32x4_t mResonance   = vdupq_n_f32(0);
      float32x4_t mPan         = vdupq_n_f32(0);
      uint32x4_t  mFilterTrack = vdupq_n_u32(0);

      inline float32x4_t cutoff(const float32x4_t env, const float32x4_t f0) const {
        return mFilterMax.freqEnv(f0, env);
      }

      inline float32x4_t resonance() const {
        return mResonance;
      }

      inline float32x4_t shape(const float32x4_t env) const {
        return util::simd::clamp_unit(mShape + mShapeEnv * env);
      }

      inline float32x4_t level(const float32x4_t env) const {
        return util::simd::clamp_punit(mLevel + mLevelEnv * env);
      }

      inline float32x4_t pan(const float32x4_t offset) const {
        return util::simd::clamp_unit(mPan + offset);
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
      float32x4_t mPan = vdupq_n_f32(0);
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
        mFilterVpo.track(gate, shared.mFilterVpo);
        mFilterTrack.track(shared.mFilterTrack, mVpo);
        mPan = config.mPan;
        mEnvCoeff.track(gate, shared.mEnvCoeff);
      }

      inline float32x4_t primaryDelta(const float32x4_t f0) const {
        return mVpo.delta(f0);
      }

      inline float32x4_t secondaryDelta(const float32x4_t f0) const {
        return mDetune.deltaOffset(f0, mVpo);
      }

      inline float32x4_t cutoff(const float32x4_t f0) const {
        return util::simd::clamp_n(
          mFilterVpo.freq(mFilterTrack.freq(f0)),
          1,
          globalConfig.sampleRate / 2
        );
      }

      util::four::Vpo mVpo;
      util::four::Vpo mDetune;
      util::four::Vpo mFilterVpo;
      util::four::Vpo mFilterTrack;
      float32x4_t mPan = vdupq_n_f32(0);
      env::four::Coefficients mEnvCoeff;
    };

    struct Pan {
      static inline float32x4x2_t process(
        const float32x4_t signal,
        const float32x4_t amount
      ) {
        auto half = vdupq_n_f32(0.5);
        auto one  = vdupq_n_f32(1);

        auto rightAmount = amount * half + half;
        auto leftAmount  = one - rightAmount;

        return {{
          signal * leftAmount,
          signal * rightAmount
        }};
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

        const auto t2p = osc::shape::TriangleToPulse { shape };

        auto primary   = mPhase1.process(delta1, _sync);
        primary        = t2p.process(primary) * two - one;
        auto secondary = mPhase2.process(delta2, _sync);
        secondary      = t2p.process(secondary) * two - one;

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
        const VoiceConfig& config,
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
        auto pan = shared.pan(mTracked.mPan);

        mEnvLevel = env;
        mEnvPanLevel = Pan::process(env, pan);

        mFilter.configure(
          mTracked.cutoff(shared.cutoff(env, filterF0)),
          shared.resonance()
        );

        auto mix = mOscillator.process(
          mTracked.primaryDelta(pitchF0),
          mTracked.secondaryDelta(pitchF0),
          shared.shape(env),
          shared.level(env),
          gate
        );

        auto filtered = mFilter.process(mix);
        return Pan::process(filtered * env, pan);
      }

      float32x4_t mEnvLevel = vdupq_n_f32(0);
      float32x4x2_t mEnvPanLevel = {{ vdupq_n_f32(0), vdupq_n_f32(0) }};

      inline float32x4_t getEnvLevel() { return mEnvLevel; }
      inline float32x4x2_t getEnvPanLevel() { return mEnvPanLevel; }

      Oscillator mOscillator;
      env::four::SlewEnvelope mEnvelope;
      filter::svf::four::Lowpass mFilter;
      VoiceTrack mTracked;
    };
  }

  template <int GROUPS>
  struct MultiVoice {
    inline float32x2_t process(
      const std::array<uint32x4_t, GROUPS>& gates,
      const std::array<four::VoiceConfig, GROUPS>& configs,
      const float32x4_t pf0,
      const float32x4_t ff0,
      const float32x2_t gain,
      const uint32x2_t agcEnabled,
      const four::SharedConfig &shared
    ) {
      float32x2_t envSum = vdup_n_f32(0);
      float32x2_t signal = vdup_n_f32(0);
      for (int i = 0; i < GROUPS; i++) {
        auto gate = gates[i];
        auto config = configs[i];
        four::Voice& v = voice(i);

        v.track(gate, config, shared);

        auto out = v.process(gate, pf0, ff0, shared);
        signal = vadd_f32(signal, util::simd::make_f32(
          util::simd::sumq_f32(out.val[0]),
          util::simd::sumq_f32(out.val[1])
        ));

        auto envPan = v.getEnvPanLevel();
        envSum = vadd_f32(envSum, util::simd::make_f32(
          util::simd::sumq_f32(envPan.val[0]),
          util::simd::sumq_f32(envPan.val[1])
        ));
      }

      signal = mDCBlocker.process(signal);

      envSum = vmax_f32(envSum, vdup_n_f32(1));
      auto agc = util::simd::invert2(envSum);
      mAppliedAgc = agc;

      auto appliedGain = vbsl_f32(agcEnabled, vmul_f32(agc, gain), gain);
      return vmul_f32(appliedGain, signal);
    }

    inline float agcDb() {
      auto x = vmul_f32(vpadd_f32(mAppliedAgc, mAppliedAgc), vdup_n_f32(0.5));
      return util::toDecibels(vget_lane_f32(x, 0));
    }

    inline four::Voice& voice(int group) {
      return mVoices[group % GROUPS];
    }

    inline float32x4_t envLevel(int g) {
      return voice(g).getEnvLevel();
    }

    std::array<four::Voice, GROUPS> mVoices;
    float32x2_t mAppliedAgc = vdup_n_f32(1);
    filter::onepole::two::DCBlocker mDCBlocker;
  };
}