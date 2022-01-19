#pragma once

#include <od/extras/ReferenceCounted.h>
#include <ui/dial/dial.h>

namespace ui {
  class DialMap : public od::ReferenceCounted {
    public:
      DialMap();
      virtual ~DialMap();

      virtual float valueAt(const ui::dial::Position &p) const = 0;
      virtual ui::dial::Position positionAt(float value) const = 0;
      virtual ui::dial::Position zero() const = 0;
      virtual void move(ui::dial::Position &p, int change, bool shift, bool fine) = 0;
  };
}