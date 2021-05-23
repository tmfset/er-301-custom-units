#pragma once

#include <math.h>
#include <hal/simd.h>
#include <hal/neon.h>
#include <util.h>

namespace shape {
  enum BendMode {
    BEND_NORMAL = 1,
    BEND_INVERTED = 2
  };

  namespace simd {
    struct Bend {
      float32x4_t up, down;

      inline Bend(
        const float32x4_t _up,
        const float32x4_t _down
      ) {
        up   = util::simd::clamp_unit(_up);
        down = util::simd::clamp_unit(_down);
      }

      inline Bend(
        const BendMode mode,
        const float32x4_t _bend
      ) {
        auto b = util::simd::clamp_unit(_bend);
        switch (mode) {
          case BEND_NORMAL:
            up   = b;
            down = b;
            break;

          default:
            up   = b;
            down = vdupq_n_f32(-1) * b;
            break;
        }
      }
    };

    inline float32x4_t fin(
      const float32x4_t phase,
      const float32x4_t width,
      const Bend &bend
    ) {
      auto cwidth = util::simd::clamp_punit(width);

      auto rising  = vcvtq_n_f32_u32(vcaleq_f32(phase, cwidth), 32);
      auto falling = util::simd::lnot(rising);

      auto partial = util::simd::lerp(falling, rising, cwidth);

      auto distance = vabdq_f32(phase, cwidth);
      auto linearInv = distance * util::simd::invert(partial);
      auto linear = util::simd::lnot(linearInv);

      auto isRiseLog = util::simd::cgtqz_f32(bend.up);
      auto isFallLog = util::simd::cgtqz_f32(bend.down);
      auto isInvert = util::simd::lerp(isFallLog, isRiseLog, rising);
      auto forExp = util::simd::lerp(linear, linearInv, isInvert);

      auto riseBend = util::simd::magnitude(bend.up);
      auto fallBend = util::simd::magnitude(bend.down);
      auto bendAmount = util::simd::lerp(fallBend, riseBend, rising);

      const util::simd::exp_scale scale { 0.01, 1 };
      auto e = scale.processBase(forExp);
      auto ei = util::simd::lerp(e, util::simd::lnot(e), isInvert);
      auto o = util::simd::lerp(linear, ei, bendAmount);

      return o;
    }
  }
}