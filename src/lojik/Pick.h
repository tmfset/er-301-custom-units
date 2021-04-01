#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Pick : public od::Object {
    public:
      Pick();
      virtual ~Pick();

#ifndef SWIGLUA
      virtual void process();
      od::Inlet  mIn   { "In" };
      od::Inlet  mAlt  { "Alt" };
      od::Inlet  mPick { "Pick" };
      od::Outlet mOut  { "Out" };
#endif
  };
}
