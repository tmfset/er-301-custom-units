#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class And : public od::Object {
    public:
      And();
      virtual ~And();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mLeft{"Left"};
    od::Inlet mRight{"Right"};
    od::Outlet mOut{"Out"};
  #endif
  };
}
