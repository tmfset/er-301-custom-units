#pragma once

#include <od/objects/Object.h>
#include <od/config.h>
#include <osc.h>

namespace strike {
  class Arc : public od::Object {
    public:
      Arc() {
        addInput(mIn);
        addOutput(mOut);

        addOutput(mEof);
        addOutput(mEor);

        addInput(mRise);
        addInput(mFall);
        addInput(mLoop);
        addInput(mHeight);
        addInput(mBend);

        addOption(mBendMode);
      }

      virtual ~Arc() { }

#ifndef SWIGLUA
      virtual void process();

      inline void processInternal() {
        const float *in = mIn.buffer();
        float *out = mOut.buffer();

        float *eof = mEof.buffer();
        float *eor = mEor.buffer();

        const float *rise   = mRise.buffer();
        const float *fall   = mFall.buffer();
        const float *loop   = mLoop.buffer();
        const float *height = mHeight.buffer();
        const float *bend   = mBend.buffer();

        osc::shape::BendMode bendMode = static_cast<osc::shape::BendMode>(mBendMode.value());

        for (int i = 0; i < FRAMELENGTH; i += 4) {
          auto _in       = vld1q_f32(in     + i);
          auto _loop     = vld1q_f32(loop   + i);
          auto _rise     = vld1q_f32(rise   + i);
          auto _fall     = vld1q_f32(fall   + i);
          auto _bend     = vld1q_f32(bend   + i);
          auto _height   = vld1q_f32(height + i);

          auto f = osc::Frequency::riseFall( _rise, _fall);
          auto b = osc::shape::Bend { bendMode, _bend };
          vst1q_f32(out + i, mEnvelope.process<osc::shape::FIN_SHAPE_POW4>(f, b, _in, _loop) * _height);

          vst1q_f32(eof + i, mEnvelope.eof());
          vst1q_f32(eor + i, mEnvelope.eor());
        }
      }

      od::Inlet  mIn  { "In" };
      od::Outlet mOut { "Out" };

      od::Outlet mEof { "EOF" };
      od::Outlet mEor { "EOR" };

      od::Inlet mRise   { "Rise" };
      od::Inlet mFall   { "Fall" };
      od::Inlet mLoop   { "Loop" };
      od::Inlet mHeight { "Height" };
      od::Inlet mBend   { "Bend" };

      od::Option mBendMode { "Bend Mode", osc::shape::BEND_NORMAL };
#endif
    private:
      osc::AD mEnvelope;
  };
}