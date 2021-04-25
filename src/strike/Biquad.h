#pragma once

#include <od/objects/Object.h>
#include <util.h>

namespace strike {
  class Biquad : public od::Object {
    public:
      Biquad() {
        addInput(mIn);
        addInput(mValue);
        addInput(mQ);
        addOutput(mOut);
      }

      virtual ~Biquad() { }

#ifndef SWIGLUA
      virtual void process();

      od::Inlet  mIn    { "In" };
      od::Inlet  mValue { "Value" };
      od::Inlet  mQ     { "Q" };
      od::Outlet mOut   { "Out" };
#endif
    private:
      simd_biquad mBiquad;
  };
}
