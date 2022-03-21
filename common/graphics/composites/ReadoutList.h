#pragma once

#include <graphics/primitives/all.h>
#include <graphics/controls/ReadoutView.h>
#include <graphics/composites/Text.h>
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

      void addItem(std::string name, od::Parameter *param) {
        mItems.push_back(Item { name, param });
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
          text.setJustifyAlign(LEFT_MIDDLE);
          text.draw(fb, WHITE, box);

          auto name = Text { "length", 10 };
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
          auto text = Text { "16", 5 };
          text.setJustifyAlign(RIGHT_MIDDLE);
          text.draw(fb, color, box);

          //box.trace(fb, WHITE);

          //auto item = mItems.at(i);
          //item.mSecondary.draw(fb, WHITE, box);
        }
      }

    private:
      struct Item {
        Item(std::string name, od::Parameter *param) :
          mName(name, CENTER_MIDDLE, 12),
          mValue(param),
          mPrimary(mValue, CENTER_MIDDLE, 16),
          mSecondary(mValue, RIGHT_MIDDLE, 5) { }

        Text             mName;
        ParameterDisplay mValue;

        ParameterText mPrimary;
        ParameterText mSecondary;
      };

      int mPrimaryTextSize = 16;
      int mSecondaryTextSize = 5;

      std::vector<Item> mItems;
  };
}