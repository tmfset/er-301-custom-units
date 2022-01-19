#pragma once

#include <ui/dial/dial.h>
#include <ui/DialMap.h>

namespace ui {
  namespace dial {
    class State {
      public:
        inline State() { }

        void setMap(DialMap *map) {
          if (mpMap) mpMap->release();
          mpMap = map;
          if (mpMap) mpMap->attach();
        }

        void save() {
          mPrevious = mCurrent;
        }

        void restore() {
          auto saved = mPrevious;
          save();
          mCurrent = saved;
        }

        void set(float value) {
          if (!mpMap) return;
          set(mpMap->positionAt(value));
        }

        void set(Position p) {
          mCurrent = p;
        }

        void zero() {
          if (!mpMap) return;
          set(mpMap->zero());
        }

        float value() {
          if (!mpMap) return 0;
          return mpMap->valueAt(mCurrent);
        }

        void move(int change, bool shifted, bool fine) {
          if (!mpMap) return;

          change = mHysteresis.process(change);
          if (change == 0) return;

          mpMap->move(mCurrent, change, shifted, fine);
        }

      private:
        DialMap *mpMap = 0;

        Position mCurrent;
        Position mPrevious;
        Hysteresis mHysteresis;
    };
  }
}