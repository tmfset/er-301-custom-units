#pragma once

#include <od/objects/Object.h>

namespace lojik {
  class Or : public od::Object {
    public:
      Or();
      virtual ~Or();

  #ifndef SWIGLUA
    virtual void process();
    od::Inlet mLeft{"Left"};
    od::Inlet mRight{"Right"};
    od::Outlet mOut{"Out"};
  #endif
  };
}
