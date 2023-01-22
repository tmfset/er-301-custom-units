#pragma once

#include <util/math.h>

namespace dsp {
  namespace two {
    struct ThreeWay {
      inline static ThreeWay punit(float32x2_t mix) {
        mix = util::two::fclamp_punit(mix);
        auto center = vdup_n_f32(0.5f);
        auto select = vclt_f32(mix, center);
        return ThreeWay { select, util::two::twice(vabd_f32(mix, center)) };
      }

      inline ThreeWay(uint32_t select, float degree) :
        mSelect(vdup_n_u32(select)),
        mDegree(vdup_n_f32(degree)) { }

      inline ThreeWay(uint32x2_t select, float32x2_t degree) :
        mSelect(select),
        mDegree(degree) { }

      inline float32x2_t mix(float32x2_t bottom, float32x2_t middle, float32x2_t top) const {
        auto out = vbsl_f32(mSelect, bottom, top);
        return util::two::lerpi(middle, out, mDegree);
      }

      const uint32x2_t mSelect;
      const float32x2_t mDegree;
    };

    struct ThreeWayMix {
      inline ThreeWayMix(float min, float max, float center) :
        mMin(vdup_n_f32(min)),
        mMax(vdup_n_f32(max)),
        mCenter(vdup_n_f32(center)),
        mScaleLow(vdup_n_f32(1.0f / fabs(center - min))),
        mScaleHigh(vdup_n_f32(1.0f / fabs(center - max))) { }

      inline ThreeWay prepare(float32x2_t mix) const {
        auto select = vclt_f32(mix, mCenter);
        auto scale = vbsl_f32(select, mScaleLow, mScaleHigh);
        return ThreeWay { select, vmul_f32(scale, vabd_f32(mix, mCenter)) };
      }

      const float32x2_t mMin;
      const float32x2_t mMax;
      const float32x2_t mCenter;
      const float32x2_t mScaleLow;
      const float32x2_t mScaleHigh;
    };
  }

  namespace four {
    struct ThreeWay {
      inline static ThreeWay punit(float32x4_t mix) {
        mix = util::four::fclamp_punit(mix);
        auto center = vdupq_n_f32(0.5f);
        auto select = vcltq_f32(mix, center);
        return ThreeWay { select, util::four::twice(vabdq_f32(mix, center)) };
      }

      inline ThreeWay(uint32_t select, float degree) :
        mSelect(vdupq_n_u32(select)),
        mDegree(vdupq_n_f32(degree)) { }

      inline ThreeWay(uint32x4_t select, float32x4_t degree) :
        mSelect(select),
        mDegree(degree) { }

      inline float32x4_t mix(float32x4_t bottom, float32x4_t middle, float32x4_t top) const {
        auto out = vbslq_f32(mSelect, bottom, top);
        return util::four::lerpi(middle, out, mDegree);
      }

      const uint32x4_t mSelect;
      const float32x4_t mDegree;
    };

    struct ThreeWayMix {
      inline ThreeWayMix(float min, float max, float center) :
        mMin(vdupq_n_f32(min)),
        mMax(vdupq_n_f32(max)),
        mCenter(vdupq_n_f32(center)),
        mScaleLow(vdupq_n_f32(1.0f / fabs(center - min))),
        mScaleHigh(vdupq_n_f32(1.0f / fabs(center - max))) { }

      inline ThreeWay prepare(float32x4_t mix) const {
        mix = util::four::fclamp(mix, mMin, mMax);
        auto select = vcltq_f32(mix, mCenter);
        auto scale = vbslq_f32(select, mScaleLow, mScaleHigh);
        return ThreeWay { select, scale * vabdq_f32(mix, mCenter) };
      }

      const float32x4_t mMin;
      const float32x4_t mMax;
      const float32x4_t mCenter;
      const float32x4_t mScaleLow;
      const float32x4_t mScaleHigh;
    };
  }
}