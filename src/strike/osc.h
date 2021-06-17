#pragma once

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

  struct Phase {
    inline float32x4_t envelope(
      const float32x4_t delta,
      const uint32x4_t trig,
      const float32x4_t loop
    ) {
      auto p = vdupq_n_f32(phase) + delta * cScale;

      uint32_t _sync[4], _syncDelta[4];
      vst1q_u32(_sync, trig);
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(p));

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        if (trigger.read(_sync[i])) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      p = p - vreinterpretq_f32_u32(vld1q_u32(_offset));
      auto isLoop = util::simd::cgtqz_f32(loop);
      p = vmlsq_f32(p, isLoop, util::simd::floor(p));
      p = vminq_f32(p, vdupq_n_f32(1));
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t envelopeSoftSync(
      const float32x4_t delta,
      const uint32x4_t trig,
      const float32x4_t loop,
      const float32x4_t width
    ) {
      auto p = vdupq_n_f32(phase) + delta * cScale;
      auto one = vdupq_n_f32(1);

      auto falling    = vcgtq_f32(p, width);
      auto distance   = vabdq_f32(p, width);
      auto length     = one - width;
      auto proportion = vminq_f32(distance * util::simd::invert(length), one);
      auto syncDelta  = distance + proportion * width;

      uint32_t _sync[4];
      vst1q_u32(_sync, trig);

      uint32_t _syncDelta[4], _falling[4];
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(syncDelta));
      vst1q_u32(_falling, falling);

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        auto doSync = _falling[i] & trigger.read(_sync[i]);
        if (doSync) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      p = p - vreinterpretq_f32_u32(vld1q_u32(_offset));
      auto isLoop = util::simd::cgtqz_f32(loop);
      p = vmlsq_f32(p, isLoop, util::simd::floor(p));
      p = vminq_f32(p, one);
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t envelopeSoftSyncBend(
      const float32x4_t delta,
      const uint32x4_t trig,
      const float32x4_t loop,
      const float32x4_t width,
      const uint32x4_t inverted,
      const float32x4_t bend
    ) {
      auto p = vdupq_n_f32(phase) + delta * cScale;
      auto one = vdupq_n_f32(1);

      auto falling    = vcgtq_f32(p, width);
      auto distance   = vabdq_f32(p, width);
      auto length     = one - width;
      auto proportion = vminq_f32(distance * util::simd::invert(length), one);

      auto bendScale = one + bend * vdupq_n_f32(4);

      auto iprop = vbslq_f32(inverted, one - proportion, proportion);
      iprop = util::simd::pow_f32(iprop, bendScale);
      iprop = one - iprop;
      iprop = util::simd::pow_f32(iprop, util::simd::invert(bendScale));
      iprop = vbslq_f32(inverted, iprop, one - iprop);

      auto syncDelta  = distance + iprop * width;

      uint32_t _sync[4];
      vst1q_u32(_sync, trig);

      uint32_t _syncDelta[4], _falling[4];
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(syncDelta));
      vst1q_u32(_falling, falling);

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        auto doSync = _falling[i] & trigger.read(_sync[i]);
        if (doSync) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      p = p - vreinterpretq_f32_u32(vld1q_u32(_offset));
      auto isLoop = util::simd::cgtqz_f32(loop);
      p = vmlsq_f32(p, isLoop, util::simd::floor(p));
      p = vminq_f32(p, one);
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t oscillator(
      const float32x4_t delta,
      const float32x4_t sync
    ) {
      auto p = vdupq_n_f32(phase) + delta * cScale;

      uint32_t _sync[4], _syncDelta[4];
      vst1q_u32(_sync, vcgtq_f32(sync, vdupq_n_f32(0)));
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(p));

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        if (_sync[i]) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      auto o = vreinterpretq_f32_u32(vld1q_u32(_offset));
      p = p - (util::simd::floor(p) + o);
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    inline float32x4_t oscillatorSoftSync(
      const float32x4_t delta,
      const float32x4_t sync,
      const float32x4_t width
    ) {
      auto p = vdupq_n_f32(phase) + delta * cScale;
      auto one = vdupq_n_f32(1);

      auto rising     = vcltq_f32(p, width);
      auto distance   = vabdq_f32(p, width);
      auto length     = vbslq_f32(rising, width, one - width);
      auto proportion = vminq_f32(distance * util::simd::invert(length), one);

      auto direction  = vbslq_f32(rising, vdupq_n_f32(1), vdupq_n_f32(-1));
      auto syncDelta  = (distance + proportion * (one - length)) * direction;

      uint32_t _sync[4], _syncDelta[4];
      vst1q_u32(_sync, vcgtq_f32(sync, vdupq_n_f32(0)));
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(syncDelta));

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        if (_sync[i]) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      auto o = vreinterpretq_f32_u32(vld1q_u32(_offset));
      p = vmaxq_f32(p + o, vdupq_n_f32(0));
      p = p - util::simd::floor(p);
      phase = vgetq_lane_f32(p, 3);
      return p;
    }

    const float32x4_t cScale = util::simd::makeq_f32(1, 2, 3, 4);
    util::Latch trigger;
    float phase = 1.0f;
  };

  namespace shape {
    enum BendMode {
      BEND_NORMAL = 1,
      BEND_INVERTED = 2
    };

    struct Bend {
      float32x4_t mUp, mDown, mAmount;
      uint32x4_t mInverted;

      inline Bend(const float32x4_t up, const float32x4_t down) {
        mUp = util::simd::clamp_unit(up);
        mDown = util::simd::clamp_unit(down);
        mInverted = vdupq_n_u32(0);
      }

      inline Bend(const BendMode mode, const float32x4_t bend) {
        auto b = util::simd::clamp_unit(bend);
        mUp = b;
        mDown = mode == BEND_NORMAL ? b : vnegq_f32(b);
        mAmount = vabdq_f32(bend, vdupq_n_f32(0));
        mInverted = mode == BEND_NORMAL ? vdupq_n_u32(0) : vcgtq_f32(bend, vdupq_n_f32(0));
      }

      inline float32x4_t up()   const { return mUp; }
      inline float32x4_t down() const { return mDown; }
      inline uint32x4_t inverted() const { return mInverted; }
      inline float32x4_t amount() const { return mAmount; }
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
      const Bend &bend,
      float32x4_t *eof,
      float32x4_t *eor
    ) {
      auto sp    = vdupq_n_f32(globalConfig.samplePeriod);
      auto zero  = vdupq_n_f32(0);
      auto one   = vdupq_n_f32(1);
      auto width = vminq_f32(one - sp, vmaxq_f32(_width, sp));

      auto rising  = vcaleq_f32(phase, width);
      auto falling = vmvnq_u32(rising);

      *eof = vcvtq_n_f32_u32(rising, 32);
      *eor = vcvtq_n_f32_u32(falling, 32);

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
          shaped = util::simd::pow_f32(x, one + amount * vdupq_n_f32(4));
          //auto x2 = x * x;
          //shaped = x2 * x2;
        } break;
      }

      auto bent = vbslq_f32(inverted, one - shaped, shaped);
      return bent;//util::simd::lerp(linear, bent, amount);
    }

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

    inline float32x4_t pulse(
      const float32x4_t phase,
      const float32x4_t width
    ) {
      return vbslq_f32(
        vcleq_f32(phase, width),
        vdupq_n_f32(1),
        vdupq_n_f32(0)
      );
    }

    inline float32x4_t pulseConst(
      const float32x4_t phase
    ) {
      return vcvtq_n_f32_u32(vcltq_f32(phase, vdupq_n_f32(0.5)), 32);
    }
  }

  struct Fin {
    Phase phase;

    template <shape::FinShape FS>
    inline float32x4_t process(
      const Frequency &freq,
      const shape::Bend &bend,
      const float32x4_t sync
    ) {
      float32x4_t eof, eor;
      return shape::fin<FS>(
        phase.oscillator(freq.delta(), sync),
        freq.width(),
        bend,
        &eof,
        &eor
      );
    }
  };

  struct Formant {
    Phase mOscPhase;
    float mEnvPhase = 1.0f;
    util::Latch mSyncTrigger;
    const float32x4_t cScale = util::simd::makeq_f32(1, 2, 3, 4);

    inline float32x4_t process(
      const float32x4_t f0,
      const float32x4_t vpo,
      const float32x4_t formant,
      const float32x4_t barrel,
      const float32x4_t sync,
      const uint32x4_t fixed
    ) {
      auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      auto one = vdupq_n_f32(1);

      auto freq = util::simd::vpo_scale(vpo, f0);
      auto pDelta = freq * sp;
      auto pulse = mOscPhase.oscillator(pDelta, sync);

      auto formantBase = vbslq_f32(fixed, freq, f0);
      auto delta = util::simd::vpo_scale_no_clamp(formant, formantBase) * sp;
      auto phase = vdupq_n_f32(mEnvPhase) + delta * cScale;
      auto width = vminq_f32(one - sp, vmaxq_f32(barrel, sp));

      auto falling    = vcgtq_f32(phase, width);
      auto distance   = vabdq_f32(phase, width);
      auto length     = one - width;
      auto proportion = vminq_f32(distance * util::simd::invert(length), one);
      auto syncDelta  = distance + proportion * width;

      uint32_t _sync[4];
      vst1q_u32(_sync, vcltq_f32(pulse, vdupq_n_f32(0.5)));

      uint32_t _syncDelta[4], _falling[4];
      vst1q_u32(_syncDelta, vreinterpretq_u32_f32(syncDelta));
      vst1q_u32(_falling, falling);

      uint32_t _offset[4], _o = 0;
      for (int i = 0; i < 4; i++) {
        auto doSync = _falling[i] & mSyncTrigger.read(_sync[i]);
        if (doSync) { _o = _syncDelta[i]; }
        _offset[i] = _o;
      }

      auto offset = vreinterpretq_f32_u32(vld1q_u32(_offset));
      phase = vminq_f32(phase - offset, one);
      mEnvPhase = vgetq_lane_f32(phase, 3);

      falling    = vcgtq_f32(phase, width);
      distance   = vabdq_f32(phase, width);
      length     = vbslq_f32(falling, one - width, width);
      return one - distance * util::simd::invert(length);
    }
  };

  struct Softy {
    Phase mPhase;

    inline float32x4_t process(
      const float32x4_t f0,
      const float32x4_t vpo,
      const float32x4_t _width,
      const float32x4_t sync
    ) {
      auto sp = vdupq_n_f32(globalConfig.samplePeriod);
      auto one = vdupq_n_f32(1);

      auto width = vminq_f32(one - sp, vmaxq_f32(_width, sp));
      auto freq = util::simd::vpo_scale(vpo, f0);
      auto phase = mPhase.oscillatorSoftSync(freq * sp, sync, width);

      auto falling    = vcgtq_f32(phase, width);
      auto distance   = vabdq_f32(phase, width);
      auto length     = vbslq_f32(falling, one - width, width);
      return one - distance * util::simd::invert(length);
    }
  };

  struct AD {
    Phase phase;
    float32x4_t mEof, mEor;

    inline float32x4_t eof() const { return mEof; }
    inline float32x4_t eor() const { return mEor; }

    template <shape::FinShape FS>
    inline float32x4_t process(
      const Frequency& freq,
      const shape::Bend &bend,
      const uint32x4_t trig,
      const float32x4_t loop
    ) {
      auto width = freq.width();
      return shape::fin<FS>(
        phase.envelopeSoftSyncBend(freq.delta(), trig, loop, width, bend.inverted(), bend.amount()),
        width,
        bend,
        &mEof,
        &mEor
      );

      return shape::fin<FS>(
        phase.envelopeSoftSync(freq.delta(), trig, loop, width),
        width,
        bend,
        &mEof,
        &mEor
      );
    }
  };
}