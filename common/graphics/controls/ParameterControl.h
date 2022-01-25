#pragma once

#include <od/objects/Parameter.h>

#include <ui/dial/state.h>

namespace graphics {
  class ParameterControl {
    public:
      inline ParameterControl(od::Parameter *param) :
          mpParameter(param) {
        mpParameter->attach();
      }

      inline ~ParameterControl() {
        mpParameter->release();
      }

      void setValueInUnits(float value) {
        mDisplay.setValueInUnits(value, true);
      }

      void save() {
        mDialState.save();
      }

      void zero() {
        mDialState.zero();
        setValueInUnits(mDialState.value());
      }

      void restore() {
        mDialState.restore();
        setValueInUnits(mDialState.value());
      }

      void encoder(int change, bool shifted, bool fine) {
        if (mDisplay.hasMoved()) {
          mDialState.set(mDisplay.lastValueInUnits());
        }
        mDialState.move(change, shifted, fine);
        setValueInUnits(mDialState.value());
      }

    private:
      od::Parameter *mpParameter;
      ui::dial::State mDialState;
  };
}