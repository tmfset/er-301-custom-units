#pragma once

#include "util.h"
#include <hal/constants.h>
#include "ScaleBook.h"

namespace common {
  class ScaleQuantizer {
    public:
      ScaleQuantizer() {}

      const ScaleBook& scaleBook() const {
        return mScaleBook;
      }

      void setCurrentScale(int c) {
        mCurrentScale = c;
      }

      const Scale& currentScale() const {
        return mScaleBook.scale(mCurrentScale);
      }

      float detectedCentValue() const {
        return mDetection.detectedCents();
      }


      int detectedOctaveValue() const {
        return mDetection.detectedOctave();
      }

      float quantizedCentValue() const {
        return mDetection.quantizedCents();
      }

      inline float quantize(float value) {
        return Detection(currentScale(), value).quantized();
      }

      inline float quantizeAndDetect(float value) {
        mDetection = Detection(currentScale(), value);
        return mDetection.quantized();
      }

    private:
      struct Detection {
        inline Detection() { }

        inline Detection(const Scale &scale, float value) {
          float voltage  = FULLSCALE_IN_VOLTS * value;
          incomingOctave = util::fdr(voltage);
          incomingPitch  = voltage - incomingOctave;
          outgoingPitch  = scale.quantizePitch(incomingPitch);
        }

        inline int detectedOctave() const {
          return incomingOctave;
        }

        inline float detectedCents() const {
          return incomingPitch * 1200.0f;
        }

        inline float quantizedCents() const {
          return outgoingPitch * 1200.0f;
        }

        inline float quantized() const {
          return (outgoingPitch + incomingOctave) * (1.0f / FULLSCALE_IN_VOLTS);
        }

        float incomingPitch = 0;
        int   incomingOctave = 0;
        float outgoingPitch = 0;
      };

      ScaleBook mScaleBook;
      Detection mDetection;
      int mCurrentScale = 0;
  };
}