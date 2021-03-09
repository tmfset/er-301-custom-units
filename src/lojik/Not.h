#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Not : public od::Object {
    public:
      Not();
      virtual ~Not();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mIn{"In"};
    od::Outlet mOut{"Out"};
  #endif
  };
}
