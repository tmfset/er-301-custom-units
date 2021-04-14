#pragma once

#include <od/audio/Sample.h>
#include <hal/simd.h>

namespace sloop {
  inline int mod(int a, int n) {
    return ((a % n) + n) % n;
  }

  inline int max(int left, int right) {
    return (left > right) ? left : right;
  }

  inline int min(int left, int right) {
    return (left < right) ? left : right;
  }

  inline float fmin(float left, float right) {
    return (left < right) ? left : right;
  }

  inline float fmax(float left, float right) {
    return (left > right) ? left : right;
  }

  inline int clamp(int value, int min, int max) {
    return (value > max) ? max : (value < min) ? min : value;
  }

  inline float fclampMax(float value, float max) {
    return (value > max) ? max : value;
  }

  inline float fclampMin(float value, float min) {
    return (value < min) ? min : value;
  }

  inline float fclamp(float value, float min, float max) {
    return fclampMax(fclampMin(value, min), max);
  }

  inline float flerp(float by, float left, float right) {
    return left * by + right * (1.0f - by);
  }

  inline float32x4_t readSample(od::Sample *sample, const int *index, int channel) {
    float32x4_t out = vdupq_n_f32(0);
    out = vsetq_lane_f32(sample->get(index[0], channel), out, 0);
    out = vsetq_lane_f32(sample->get(index[1], channel), out, 1);
    out = vsetq_lane_f32(sample->get(index[2], channel), out, 2);
    out = vsetq_lane_f32(sample->get(index[3], channel), out, 3);
    return out;
  }

  inline void writeSample(od::Sample *sample, const int *index, int channel, float32x4_t value) {
    sample->set(index[0], channel, vgetq_lane_f32(value, 0));
    sample->set(index[1], channel, vgetq_lane_f32(value, 1));
    sample->set(index[2], channel, vgetq_lane_f32(value, 2));
    sample->set(index[3], channel, vgetq_lane_f32(value, 3));
  }

  struct Complement {
    float32x4_t mValue;
    float32x4_t mComplement;

    Complement() {}

    Complement(float32x4_t value, float32x4_t one) {
      mValue      = value;
      mComplement = one - value;
    }

    inline float32x4_t lerp(const float32x4_t &from, const float32x4_t &to) const {
      return vmlaq_f32(from * mValue, to, mComplement);
    }

    inline float32x4_t ease(const float32x4_t &from) const {
      return vmlaq_f32(mComplement, from, mValue);
    }
  };
}