#pragma once

#include <od/graphics/Graphic.h>
#include <od/graphics/constants.h>
#include <od/graphics/text/Utils.h>
#include <vector>

namespace strike {
  class MultiOptionView : public od::Graphic {
    public:
      MultiOptionView() : od::Graphic(0, 0, 128, 64) { }
      virtual ~MultiOptionView() {}

      void addChoice(int lane, const std::string &text) {
        while ((int)mLanes.size() < lane + 1) {
          mLanes.push_back(std::vector<std::string> {});
          mLaneSelections.push_back(0);
        }

        mLanes.at(lane).push_back(text);
      }

      void setActiveLane(int lane) {
        mActiveLane = lane;
      }

      void setLaneSelection(int lane, int choice) {
        while ((int)mLaneSelections.size() < lane + 1) {
          mLaneSelections.push_back(0);
        }

        mLaneSelections.at(lane) = choice;
      }

      int getActiveLane() {
        return mActiveLane;
      }

      void advanceActiveLane() {
        auto next = mActiveLane + 1;
        setActiveLane(next % mLanes.size());
      }

    private:
      int mActiveLane = 0;
      std::vector<std::vector<std::string>> mLanes;
      std::vector<int> mLaneSelections;
      void draw(od::FrameBuffer &fb) {
        auto height = 64;
        auto width = 128;
        auto margin = 2;
        auto pad = 2;

        auto textSize = 10;

        auto totalLanes = (int)mLanes.size();

        auto innerHeight = height - (2 * margin);
        auto innerWidth  = width - (2 * margin);
        auto laneHeight = innerHeight / totalLanes;
        auto laneWidth  = innerWidth;

        #define LANE_CENTER(lane) margin + (laneHeight / 2.0f) * ((lane * 2) + 1)
        #define CHOICE_CENTER(choice, total) margin + (laneWidth / (total * 2.0f)) * ((choice * 2) + 1)
        #define LANE_TOP(center) center + (laneHeight - pad) / 2.0f
        #define LANE_BOTTOM(center) center - (laneHeight - pad) / 2.0f

        auto activeLaneCenter = LANE_CENTER(mActiveLane);
        auto activeLaneBottom = LANE_BOTTOM(activeLaneCenter);
        auto activeLaneTop    = LANE_TOP(activeLaneCenter);

        fb.hline(WHITE, margin, margin + innerWidth, activeLaneTop, 1);
        fb.hline(WHITE, margin, margin + innerWidth, activeLaneBottom, 1);
        fb.vline(WHITE, margin, activeLaneTop, activeLaneBottom, 1);
        fb.vline(WHITE, margin + innerWidth, activeLaneTop, activeLaneBottom, 1);

        for (int lane = 0; lane < totalLanes; lane++) {
          auto laneCenter = LANE_CENTER(lane);

          auto choices = mLanes.at(lane);
          auto choicesCount = (int)choices.size();
          auto selection = mLaneSelections.at(lane);

          auto boxWidth  = (laneWidth / (float)choicesCount) - ((choicesCount - 1) * 2.0f * pad);
          auto boxHeight = laneHeight - ((totalLanes - 1) * 4.0f * pad);

          for (int choice = 0; choice < choicesCount; choice++) {
            auto choiceCenter = CHOICE_CENTER(choice, choicesCount);
            auto text = choices.at(choice).c_str();
            auto width = od::getStringWidth(text, textSize);
            fb.text(WHITE, choiceCenter - width / 2, laneCenter, text, textSize, ALIGN_MIDDLE);

            if (choice == selection) {
              fb.box(
                WHITE,
                choiceCenter - boxWidth / 2.0f,
                laneCenter - boxHeight / 2.0f,
                choiceCenter + boxWidth / 2.0f,
                laneCenter + boxHeight / 2.0f
              );
            }
          }
        }
      }
  };
}