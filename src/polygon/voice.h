#pragma once

#include <osc.h>
#include <filter.h>
#include <slew.h>
#include <vector>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace voice {
  struct Trigger {
    inline uint32_t read(const uint32_t high) {
      mTrigger = high & mEnable;
      mEnable = ~high;
      return mTrigger;
    }

    uint32_t mEnable  = 0;
    uint32_t mTrigger = 0;
  };

  struct RoundRobin {
    inline RoundRobin(int total) {
      mTotal = total;
    }

    inline int total() const { return mTotal; }

    inline uint32x4_t active(int index) const {
      return vceqq_s32(mIndex, vdupq_n_s32(index));
    }

    inline void process(uint32x4_t gate) {
      uint32_t _gate[4];
      vst1q_u32(_gate, vmvnq_u32(gate));

      int32_t _index[4], _current = vgetq_lane_s32(mIndex, 3);

      for (int i = 0; i < 4; i++) {
        if (mTrigger.read(_gate[i])) {
          _current += 1;
        }
        _index[i] = _current % mTotal;
      }

      mIndex = vld1q_s32(_index);
    }

    Trigger mTrigger;
    int32x4_t mIndex;
    int mTotal;
  };

  struct Control {
    inline Control(uint32_t index) {
      mIndex = index;
      mVpo = vdupq_n_f32(0);
    }

    inline void process(
      const RoundRobin& roundRobin,
      uint32x4_t gate,
      float32x4_t vpo
    ) {
      auto active = roundRobin.active(mIndex);
      auto track  = vandq_u32(active, gate);

      uint32_t _track[4];
      vst1q_u32(_track, track);

      float _vpo[4], _cVpo = vgetq_lane_f32(mVpo, 3);
      vst1q_f32(_vpo, vpo);

      for (int i = 0; i < 4; i++) {
        if (_track[i]) _cVpo = _vpo[i];
        _vpo[i] = _cVpo;
      }

      mVpo    = vld1q_f32(_vpo);
      mGate   = track;
    }

    inline float32x4_t gate() const { return vcvtq_n_f32_u32(mGate, 32); }
    inline float32x4_t vpo() const { return mVpo; }

    uint32_t mIndex;
    float32x4_t mVpo;
    uint32x4_t mGate;
  };

  class Voice {
    public:
      inline Voice(int index) : mControl(index) { }

      inline void setLPG(float rise, float fall, float height) {
        mEnvelope.setRiseFall(rise, fall);
        mHeight = height;
      }

      inline void control(
        const RoundRobin& roundRobin,
        uint32x4_t gate,
        float32x4_t vpo
      ) {
        mControl.process(roundRobin, gate, vpo);
      }

      inline float32x4_t process(
        const float32x4_t f0,
        const osc::shape::TSP& shape,
        const float32x4_t subLevel,
        const float32x4_t subDivide
      ) {
        auto sp  = vdupq_n_f32(globalConfig.samplePeriod);
        auto vpo = mControl.vpo();
        auto delta = util::simd::vpo_scale(vpo, f0) * sp;

        mPhase.process(
          delta,
          subDivide,
          vdupq_n_u32(0)
        );

        auto primary = shape.process(mPhase.one());
        auto sub     = shape.process(mPhase.sub());

        primary = primary * vdupq_n_f32(2) - vdupq_n_f32(1);
        sub     = sub * vdupq_n_f32(2) - vdupq_n_f32(1);
        auto scale = util::simd::invert(vdupq_n_f32(1) + subLevel);
        auto mix   = (primary + sub * subLevel) * scale;

        auto env = mEnvelope.process(mControl.gate());
        mEnv = env;

        mFilter.setFrequency(env * vdupq_n_f32(mHeight));
        return util::simd::atan(mFilter.process(mix * env)) * vdupq_n_f32(1.24f);
      }

      inline float32x4_t getEnvLevel() const { return mEnv; }

    private:
      float32x4_t mEnv;
      Control mControl;
      osc::SubPhase mPhase;
      slew::Slew mEnvelope;
      filter::onepole::Filter mFilter;
      float mHeight = 0;
  };
}