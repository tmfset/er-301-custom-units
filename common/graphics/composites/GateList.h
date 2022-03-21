#pragma once

#include <graphics/primitives/all.h>
#include <graphics/controls/ReadoutView.h>
#include <graphics/composites/Text.h>
#include <graphics/composites/ListWindow.h>
#include <vector>

namespace graphics {
  class GateList {
    public:
      GateList() {}
      virtual ~GateList() {}

      void addItem(std::string name) {
        mItems.push_back(Item { name });
      }

      void draw(od::FrameBuffer &fb, const Box &world) {
        auto length = mItems.size();
        auto current = 0;

        auto rightWorld = world.splitRight(0.5);
        auto vertical = world.vertical();

        auto primary = ListWindow::from(vertical, vertical.width(), 2)
          .scrollTo(current, length, od::justifyRight);

        for (int i = 0; i < length; i++) {
          if (!primary.vVisibleIndex(i)) continue;
          auto box = primary.vBox(world, i);

          Circle::cr(
            box.inLeft(mPrimaryRadius * 2).center(),
            mPrimaryRadius
          ).trace(fb, GRAY5);

          auto name = Text { "reset", 10 };
          name.setJustifyAlign(LEFT_BOTTOM);
          name.draw(fb, WHITE, box);

          // auto item = mItems.at(i);
          // item.mName.draw(fb, WHITE, box);
          // item.mPrimary.draw(fb, WHITE, box);
        }

        auto secondary = ListWindow::from(vertical, 6, 2)
          .scrollTo(current, length, od::justifyRight);

        for (int i = 0; i < length; i++) {
          if (!secondary.vVisibleIndex(i)) continue;
          auto box = secondary.vBox(rightWorld, i);

          auto color = i == current ? WHITE : GRAY5;
          box.minCircle().trace(fb, color);
          //box.trace(fb, WHITE);

          //auto item = mItems.at(i);
          //item.mSecondary.draw(fb, WHITE, box);
        }

        //rightWorld.trace(fb, WHITE);
      }

    private:
      struct Item {
        Item(std::string name) :
          mName(name, CENTER_MIDDLE, 12) {}

        Text mName;
      };

      int mPrimaryRadius = 8;

      std::vector<Item> mItems;
  };
}