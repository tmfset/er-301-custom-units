#pragma once

#include <graphics/primitives/all.h>
#include <graphics/controls/ReadoutView.h>
#include <graphics/composites/Text.h>
#include <graphics/composites/FollowableValue.h>
#include <graphics/composites/FollowableText.h>
#include <graphics/composites/ListWindow.h>
#include <vector>

namespace graphics {
  class ReadoutList {
    public:
      ReadoutList() {}
      virtual ~ReadoutList() {}

      // graphics::ReadoutView* getCursorController(int i) {
      //   if (i >= mItems.size()) return nullptr;
      //   return &(mItems.at(i).mPrimary);
      // }

      void addItem(std::string name, od::Followable &followable) {
        mItems.push_back(Item { name, followable });
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

          auto text = Text { "16", 12 };
          text.draw(fb, WHITE, box, LEFT_MIDDLE);

          auto name = Text { "length", 10 };
          name.draw(fb, WHITE, box, LEFT_BOTTOM);

          // auto item = mItems.at(i);
          // item.mName.draw(fb, WHITE, box, CENTER_MIDDLE);
          // item.mPrimary.draw(fb, WHITE, box, CENTER_MIDDLE);
        }

        auto secondary = ListWindow::from(vertical, 6, 2)
          .scrollTo(current, length, od::justifyRight);

        for (int i = 0; i < length; i++) {
          if (!secondary.vVisibleIndex(i)) continue;
          auto box = secondary.vBox(rightWorld, i);

          auto color = i == current ? WHITE : GRAY5;
          auto text = Text { "16", 5 };
          text.draw(fb, color, box, RIGHT_MIDDLE);

          //box.trace(fb, WHITE);

          //auto item = mItems.at(i);
          //item.mSecondary.draw(fb, WHITE, box, RIGHT_MIDDLE);
        }
      }

    private:
      struct Item {
        Item(std::string name, od::Followable &followable) :
          mName(name, 12),
          mValue(followable),
          mPrimary(mValue, 16),
          mSecondary(mValue, 5) { }

        Text             mName;
        FollowableValue mValue;

        FollowableText mPrimary;
        FollowableText mSecondary;
      };

      int mPrimaryTextSize = 16;
      int mSecondaryTextSize = 5;

      std::vector<Item> mItems;
  };
}