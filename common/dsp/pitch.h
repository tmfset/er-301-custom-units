#pragma once

#include <util/math.h>
#include <dsp/latch.h>

namespace dsp {
  #define LOG2 0.6931471805599453f
  #define VPO_LOG_MAX FULLSCALE_IN_VOLTS * LOG2

  inline float toVoltage(float x)   { return x * FULLSCALE_IN_VOLTS; }
  inline float fromVoltage(float x) { return x / FULLSCALE_IN_VOLTS; }
  inline float toCents(float x)     { return x * CENTS_PER_OCTAVE; }
  inline float fromCents(float x)   { return x / CENTS_PER_OCTAVE; }

  inline float toOctave(float x) {
    return (int)(x + FULLSCALE_IN_VOLTS) - FULLSCALE_IN_VOLTS;
  }

  namespace two {
    inline float32x2_t fclamp_freq(float32x2_t x) {
      return util::two::fclamp_n(x, 0.001, globalConfig.sampleRate / 4);
    }

    inline float32x2_t fscale_vpo(float32x2_t vpo) {
      return vmul_f32(vpo, vdup_n_f32(VPO_LOG_MAX));
    }

    inline float32x2_t vpo_scale(float32x2_t f0, float32x2_t vpo) {
      return vmul_f32(f0, util::two::exp_f32(vmul_f32(vpo, vdup_n_f32(VPO_LOG_MAX))));
    }

    inline float32x2_t vpo_scale_limited(float32x2_t f0, float32x2_t vpo) {
      return fclamp_freq(vpo_scale(f0, vpo));
    }
  }

  namespace four {
    inline float32x4_t toVoltage(float32x4_t v)   { return v * vdupq_n_f32(FULLSCALE_IN_VOLTS); }
    inline float32x4_t fromVoltage(float32x4_t v) { return v * vdupq_n_f32(1.0f / FULLSCALE_IN_VOLTS); }
    inline float32x4_t toCents(float32x4_t v)     { return v * vdupq_n_f32(CENTS_PER_OCTAVE); }
    inline float32x4_t fromCents(float32x4_t v)   { return v * vdupq_n_f32(1.0f / CENTS_PER_OCTAVE); }

    inline float32x4_t toOctave(float32x4_t v) {
      auto fiv = vdupq_n_f32(FULLSCALE_IN_VOLTS);
      return util::four::frtz(v + fiv) - fiv;
    }

    /**
      * Clamp a frequency value in the range [0.001 Hz, sampleRate / 4]
      */
    inline float32x4_t fclamp_freq(float32x4_t x) {
      return util::four::fclamp_n(x, 0.001, globalConfig.sampleRate / 4);
    }

    inline float32x4_t fscale_vpo(float32x4_t vpo) {
      return vpo * vdupq_n_f32(VPO_LOG_MAX);
    }

    inline float32x4_t vpo_scale(float32x4_t f0, float32x4_t vpo) {
      return f0 * util::simd::exp_f32(fscale_vpo(vpo));
    }

    inline float32x4_t vpo_scale_limited(float32x4_t f0, float32x4_t vpo) {
      return fclamp_freq(vpo_scale(f0, vpo));
    }

    inline float32x4_t fast_vpo_scale(float32x4_t f0, float32x4_t vpo) {
      return f0 * util::simd::fast_exp_f32(fscale_vpo(vpo));
    }

    inline float32x4_t fast_vpo_scale_limited(float32x4_t f0, float32x4_t vpo) {
      return fclamp_freq(fast_vpo_scale(f0, vpo));
    }

    /**
      * Volt Per Octave tracking + common DSP
      */
    struct Vpo {
      inline void track(const uint32x4_t gate, const Vpo &other) {
        mScale.track(gate, other.mScale);
      }

      inline float32x4_t freq(const float32x4_t f0) const {
        return f0 * mScale.value();
      }

      inline float32x4_t freqEnv(const float32x4_t f0, const float32x4_t env) const {
        return f0 * util::four::lerpi(vdupq_n_f32(1), mScale.value(), env);
      }

      inline float32x4_t delta(const float32x4_t f0) const {
        return freq(f0) * vdupq_n_f32(globalConfig.samplePeriod);
      }

      inline float32x4_t deltaOffset(const float32x4_t f0, const Vpo& offset) const {
        return delta(offset.freq(f0));
      }

      inline void configure(const float32x4_t vpo) {
        mScale.set(util::simd::exp_f32(fscale_vpo(vpo)));
      }

      TrackAndHold mScale { 1 };
    };

    class Pitch {
      public:
        inline Pitch() {}
        inline Pitch(float32x4_t o, float32x4_t c) : mOctave(o), mCents(c) { }

        static inline Pitch from(float value) {
          return from(vdupq_n_f32(value));
        }

        static inline Pitch from(float32x4_t value) {
          auto voltage = toVoltage(value);
          auto octave  = toOctave(voltage);
          return Pitch { octave, toCents(voltage - octave) };
        }

        inline float32x4_t octave() const { return mOctave; }
        inline float32x4_t cents()  const { return mCents; }

        inline Pitch atCents(float32x4_t cents) const {
          return Pitch(mOctave, cents);
        }

        inline Pitch atCents(float cents) const {
          return atCents(vdupq_n_f32(cents));
        }

        inline float32x4_t value() const {
          return fromVoltage(mOctave + fromCents(mCents));
        }

      private:
        float32x4_t mOctave = vdupq_n_f32(0);
        float32x4_t mCents = vdupq_n_f32(0);
    };
  }
}