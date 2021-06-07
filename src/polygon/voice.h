#pragma once

#include <osc.h>
#include <filter.h>
#include <vector>

namespace voice {
  class Control {
    public:
      inline Control(uint32_t index) : mIndex(index) {
        mVpo = vdupq_n_f32(0);
        mDetune = vdupq_n_f32(0);
        mTrig = vdupq_n_u32(0);
      }

      inline void process(
        uint32x4_t index,
        uint32x4_t gate,
        float32x4_t vpo,
        float32x4_t detune
      ) {
        auto active = vceqq_u32(index, vdupq_n_u32(mIndex));
        auto track  = vandq_u32(active, gate);

        uint32_t _track[4];
        vst1q_u32(_track, track);

        mVpo    = util::simd::vtnh(_track, vgetq_lane_f32(mVpo, 3), vpo);
        mDetune = util::simd::vtnh(_track, vgetq_lane_f32(mDetune, 3), detune);
        mTrig   = track;
      }

      inline float32x4_t primary(float32x4_t f0) const {
        return tune(f0, vdupq_n_f32(0));
      }

      inline float32x4_t secondary(float32x4_t f0) const {
        return tune(f0, mDetune);
      }

      inline float32x4_t sub(float32x4_t f0) const {
        return tune(f0, vdupq_n_f32(-0.2f));
      }

      inline uint32x4_t trig() const {
        return mTrig;
      }

      inline float32x4_t vpo() const { return mVpo; }
      inline float32x4_t detune() const { return mDetune; }

    private:
      inline float32x4_t tune(float32x4_t f0, float32x4_t offset) const {
        auto sp = vdupq_n_f32(globalConfig.samplePeriod);
        auto f = util::simd::vpo_scale(mVpo + offset, f0);
        return f * sp;
      }

      uint32_t mIndex;
      float32x4_t mVpo;
      float32x4_t mDetune;
      uint32x4_t mTrig;
  };

  class Voice {
    public:
      inline Voice() { }

      inline float32x4_t process(
        const Control &control,
        const float32x4_t f0,
        const float32x4_t rise,
        const float32x4_t fall
      ) {
        auto sync = vdupq_n_u32(0);
        auto _width = vdupq_n_f32(0.5);

        auto sp    = vdupq_n_f32(globalConfig.samplePeriod);
        auto one   = vdupq_n_f32(1);
        auto width = vminq_f32(one - sp, vmaxq_f32(_width, sp));

        auto vpo = control.vpo();

        mThreePhase.processOscillator(
          util::simd::vpo_scale(vpo, f0) * sp,
          util::simd::vpo_scale(vpo + control.detune(), f0) * sp,
          util::simd::vpo_scale(vpo + vdupq_n_f32(-0.2f), f0) * sp,
          sync
        );

        auto primary = osc::shape::triangle(
          mThreePhase.phaseA(),//mOscPrimary.oscillator(control.primary(f0), sync),
          width
        );

        auto secondary = osc::shape::triangle(
          mThreePhase.phaseB(),//mOscSecondary.oscillator(control.secondary(f0), sync),
          width
        );

        auto sub = osc::shape::pulse(
          mThreePhase.phaseC(),//mOscSub.oscillator(control.sub(f0), sync),
          width
        );

        // auto p = primary * vdupq_n_f32(2) - vdupq_n_f32(1);
        // auto s = secondary * vdupq_n_f32(2) - vdupq_n_f32(1);
        // auto b = sub * vdupq_n_f32(2) - vdupq_n_f32(1)

        auto envFreq = osc::Frequency::riseFall(rise, fall);
        // auto env = osc::shape::fin<osc::shape::FIN_SHAPE_POW2>(
        //   mEnvelope.envelope(envFreq.delta(), control.trig()),
        //   envFreq.width(),
        //   osc::shape::Bend { osc::shape::BEND_NORMAL, vdupq_n_f32(-0.5f) }
        // );

        auto env = osc::shape::triangle(
          mEnvelope.envelope(envFreq.delta(), control.trig()),
          envFreq.width()
          //osc::shape::Bend { osc::shape::BEND_NORMAL, vdupq_n_f32(-0.5f) }
        );

        auto sum = (((primary + secondary + sub) * vdupq_n_f32(2)) - vdupq_n_f32(3)) * vdupq_n_f32(1.0f/3.0f) * env;

        mFilterCf.update(
          env * vdupq_n_f32(0.5),
          vdupq_n_f32(50),
          vdupq_n_f32(0)//,
          //filter::biquad::LOWPASS
        );

        mFilter.process(mFilterCf, sum);

        return util::simd::tanh(mFilter.lowpass());
      }

    private:
      osc::ThreePhase mThreePhase;
      //osc::Phase mOscPrimary;
      //osc::Phase mOscSecondary;
      //osc::Phase mOscSub;
      osc::Phase mEnvelope;
      filter::svf::Coefficients mFilterCf;
      filter::svf::Filter mFilter;
  };
}