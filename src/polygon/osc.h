#pragma once

#include <od/config.h>
#include <math.h>
#include <hal/simd.h>
#include <util.h>

namespace osc {
  class Frequency {
    public:
      static inline Frequency riseFall(const float32x4_t _r, const float32x4_t _f) {
        auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        auto m = vdupq_n_f32(0.001);
        auto r = vmaxq_f32(_r, m);
        auto f = vmaxq_f32(_f, m);
        auto pI = util::simd::invert(r + f);
        return Frequency { pI * sp, pI * r };
      }

      static inline Frequency periodWidth(const float32x4_t p, const float32x4_t w) {
        auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        auto pI = util::simd::invert(vmaxq_f32(p, sp));
        return Frequency { pI * sp, w };
      }

      static inline Frequency vpoWidth(const float32x4_t vpo, const float32x4_t f0, const float32x4_t w) {
        return freqWidth(util::simd::vpo_scale(vpo, f0), w);
      }

      static inline Frequency freqWidth(const float32x4_t f, const float32x4_t w) {
        auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        return Frequency { f * sp, w };
      }

      inline float32x4_t delta() const { return mDelta; }
      inline float32x4_t width() const { return mWidth; }

    private:
      inline Frequency(const float32x4_t _delta, const float32x4_t _width) {
        mDelta = _delta;
        mWidth = _width;
      }

      float32x4_t mDelta, mWidth;
  };

  struct ThreePhase {
    inline ThreePhase() {
      for (int i = 0; i < 4; i++) {
        mPhaseA[i] = 0;
        mPhaseB[i] = 0;
        mPhaseC[i] = 0;
      }
    }

    inline void processOscillator(
      const float32x4_t deltaA,
      const float32x4_t deltaB,
      const float32x4_t deltaC,
      const uint32x4_t sync
    ) {
      uint32_t _sync[4];
      vst1q_u32(_sync, sync);

      float _scale[4],
            baseA = mPhaseA[3],
            baseB = mPhaseB[3],
            baseC = mPhaseC[3];

      for (int i = 0, s = 1; i < 4; i++, s++) {
        if (_sync[i]) { baseA = 0; baseB = 0; baseC = 0; s = 0; }
        mPhaseA[i] = baseA;
        mPhaseB[i] = baseB;
        mPhaseC[i] = baseC;
        _scale[i] = s;
      }

      auto scale = vld1q_f32(_scale);
      auto pA = vmlaq_f32(vld1q_f32(mPhaseA), deltaA, scale);
      auto pB = vmlaq_f32(vld1q_f32(mPhaseB), deltaB, scale);
      auto pC = vmlaq_f32(vld1q_f32(mPhaseC), deltaC, scale);

      vst1q_f32(mPhaseA, pA - util::simd::floor(pA));
      vst1q_f32(mPhaseB, pB - util::simd::floor(pB));
      vst1q_f32(mPhaseC, pC - util::simd::floor(pC));
    }

    float32x4_t phaseA() const { return vld1q_f32(mPhaseA); }
    float32x4_t phaseB() const { return vld1q_f32(mPhaseB); }
    float32x4_t phaseC() const { return vld1q_f32(mPhaseC); }

    float mPhaseA[4];
    float mPhaseB[4];
    float mPhaseC[4];
  };

  struct Phase {
    inline float32x4_t envelope(
      const float32x4_t delta,
      const uint32x4_t trig
    ) {
      bool t[4];
      trigger.readTriggers(t, trig);
      auto p = accumulate(phase, delta, t);
      p = vminq_f32(p, vdupq_n_f32(1));
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t oscillator(
      const float32x4_t delta,
      const uint32x4_t sync
    ) {
      uint32_t _sync[4];
      vst1q_u32(_sync, sync);
      //util::simd::cgt_as_bool(s, sync, vdupq_n_f32(0));
      float _base[4], _scale[4], base = phase;
      for (int i = 0, s = 1; i < 4; i++, s++) {
        if (_sync[i]) { base = 0; s = 0; }
        _base[i] = base;
        _scale[i] = s;
      }
      auto p = vmlaq_f32(vld1q_f32(_base), delta, vld1q_f32(_scale));
      p = p - util::simd::floor(p);
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t accumulate(
      const float _from,
      const float32x4_t delta,
      const bool *_sync
    ) const {
      float _base[4], _scale[4], base = phase;
      for (int i = 0, s = 1; i < 4; i++, s++) {
        if (_sync[i]) { base = 0; s = 0; }
        _base[i] = base;
        _scale[i] = s;
      }
      return vmlaq_f32(vld1q_f32(_base), delta, vld1q_f32(_scale));
    }

    util::Latch trigger;
    float phase = 1.0f;
  };

  namespace shape {
    enum BendMode {
      BEND_NORMAL = 1,
      BEND_INVERTED = 2
    };

    struct Bend {
      float32x4_t mUp, mDown;

      inline Bend(const float32x4_t up, const float32x4_t down) {
        mUp = util::simd::clamp_unit(up);
        mDown = util::simd::clamp_unit(down);
      }

      inline Bend(const BendMode mode, const float32x4_t bend) {
        auto b = util::simd::clamp_unit(bend);
        mUp = b;
        mDown = mode == BEND_NORMAL ? b : vnegq_f32(b);
      }

      inline float32x4_t up()   const { return mUp; }
      inline float32x4_t down() const { return mDown; }
    };

    enum FinShape {
      FIN_SHAPE_EXP,
      FIN_SHAPE_POW2,
      FIN_SHAPE_POW3,
      FIN_SHAPE_POW4
    };

    template <FinShape S>
    inline float32x4_t fin(
      const float32x4_t phase,
      const float32x4_t _width,
      const Bend &bend
      //float32x4_t *eof,
      //float32x4_t *eor
    ) {
      auto sp    = vdupq_n_f32(globalConfig.samplePeriod);
      auto zero  = vdupq_n_f32(0);
      auto one   = vdupq_n_f32(1);
      auto width = vminq_f32(one - sp, vmaxq_f32(_width, sp));

      auto rising  = vcaleq_f32(phase, width);
      //auto falling = vmvnq_u32(rising);

      //*eof = vcvtq_n_f32_u32(rising, 32);
      //*eor = vcvtq_n_f32_u32(falling, 32);

      auto select   = vbslq_f32(rising, bend.up(), bend.down());
      auto inverted = vcgtq_f32(select, zero);
      auto amount   = vabdq_f32(select, zero);

      auto distance = vabdq_f32(phase, width);
      auto length   = vbslq_f32(rising, width, one - width);
      auto linearc  = distance * util::simd::invert(length);
      auto linear   = one - linearc;

      auto x = vbslq_f32(inverted, linearc, linear);

      // Any zero to one function will do.
      float32x4_t shaped;
      switch (S) {
        case FIN_SHAPE_EXP: {
          #define FIN_EXP_DEGREE 0.01
          auto d  = vdupq_n_f32(FIN_EXP_DEGREE);
          auto dl = vdupq_n_f32(logf(FIN_EXP_DEGREE));
          auto xc = one - x;
          shaped = vmlsq_f32(util::simd::exp_f32(dl * xc), d, xc);
        } break;

        case FIN_SHAPE_POW2:
          shaped = x * x;
          break;

        case FIN_SHAPE_POW3:
          shaped = x * x * x;
          break;

        case FIN_SHAPE_POW4: {
          auto x2 = x * x;
          shaped = x2 * x2;
        } break;
      }

      auto bent = vbslq_f32(inverted, one - shaped, shaped);
      return util::simd::lerp(linear, bent, amount);
    }

    inline float32x4_t pulse(
      const float32x4_t phase,
      const float32x4_t width
    ) {
      return vbslq_f32(
        vcltq_f32(phase, width),
        vdupq_n_f32(1),
        vdupq_n_f32(0)
      );
    }

    inline float32x4_t saw(
      const float32x4_t phase
    ) {
      return phase;
    }

    struct Width {
      Width(const float32x4_t _width) {
        auto sp  = vdupq_n_f32(globalConfig.samplePeriod);
        auto one = vdupq_n_f32(1);

        mWidth = vminq_f32(one - sp, vmaxq_f32(_width, sp));
        mWidthComplement = one - mWidth;
        mWidthInverse = util::simd::invert(mWidth);
        mWidthComplementInverse = util::simd::invert(mWidthComplement);
      }

      float32x4_t mWidth;
      float32x4_t mWidthComplement;
      float32x4_t mWidthInverse;
      float32x4_t mWidthComplementInverse;
    };

    inline float32x4_t triangle(
      const float32x4_t phase,
      const float32x4_t width
    ) {
      //auto sp    = vdupq_n_f32(globalConfig.samplePeriod);
      auto one   = vdupq_n_f32(1);
      //auto width = vminq_f32(one - sp, vmaxq_f32(_width, sp));

      auto rising   = vcaleq_f32(phase, width);
      auto distance = vabdq_f32(phase, width);
      auto length   = vbslq_f32(rising, width, one - width);

      return one - distance * util::simd::invert(length);
    }

    inline float32x4_t triangle2(
      const float32x4_t phase,
      const Width &width
    ) {
      auto one   = vdupq_n_f32(1);

      auto rising   = vcaleq_f32(phase, width.mWidth);
      auto distance = vabdq_f32(phase, width.mWidth);
      auto length   = vbslq_f32(rising, width.mWidthInverse, width.mWidthComplementInverse);

      return one - distance * length;
    }
  }
}