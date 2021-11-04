#pragma once

#include <od/graphics/Graphic.h>
#include <math.h>
#include <Euclid.h>
#include <util.h>

#define BUILDOPT_VERBOSE
#define BUILDOPT_DEBUG_LEVEL 10
#include <hal/log.h>

namespace lojik {
  class EuclidCircle : public od::Graphic {
    public:
      EuclidCircle(int left, int bottom, int width, int height) : od::Graphic(left, bottom, width, height) { }
      virtual ~EuclidCircle() {
        if (mpEuclid) mpEuclid->release();
      }

  #ifndef SWIGLUA
      virtual void draw(od::FrameBuffer &fb) {
        const int CURSOR = 3;
        const int MARGIN = 2;

        int radius = MIN(mWidth / 2, mHeight / 2) - MARGIN - CURSOR;
        float outerRadius = radius;
        float innerRadius = radius * 0.75f;

        int x = mWorldLeft + mWidth / 2;
        int y = mWorldBottom + mHeight / 2;

        if (mpEuclid) {
          int length = mpEuclid->mLength.value();
          int step   = mpEuclid->getStep();
          int shift  = mod(mpEuclid->getShift(), length);
          if (length < 1) return;

          float width = M_PI * 2.0f / (float)length;
          float halfWidth = width / 2.0f;

          // Fill in the ring arcs.
          float fillRadius = outerRadius * 0.985f;
          for (int i = 0; i < length; i++) {
            if (!mpEuclid->isSet(i)) continue;
            float theta = width * (float)i;
            fillArc(fb, mFill, x, y, fillRadius, theta - halfWidth, theta + halfWidth);
          }

          // Draw bright lines to delineate steps.
          float delineateRadius = outerRadius * 0.98f;
          float delineateColor = mDelineate * (64.0f - (float)length) / 64.0f;
          for (int i = 0; i < length; i++) {
            float theta = width * (float)i;
            drawRadius(fb, delineateColor, x, y, delineateRadius, theta - halfWidth);
            //drawRadius(fb, delineateColor, x, y, delineateRadius, theta + halfWidth);
          }

          // Clear the center of the circle.
          fb.fillCircle(BLACK, x, y, innerRadius);

          // Draw lines around the wheel.
          fb.circle(mOutline, x, y, radius);
          fb.circle(mOutline, x, y, innerRadius);

          float markerRadius = radius * 0.125;
          float markerOffset = innerRadius - markerRadius;

          // Draw shift marker.
          if (shift > 0) {
            float theta = width * (float)shift;
            float xOffset = circleX(x, markerOffset, theta);
            float yOffset = circleY(y, markerOffset, theta);

            fb.circle(mMarkOutline, xOffset, yOffset, markerRadius);
          }

          // Draw step marker.
          if (true) {
            float theta = width * (float)step;
            float xOffset = circleX(x, markerOffset, theta);
            float yOffset = circleY(y, markerOffset, theta);

            fb.fillCircle(mMarkFill, xOffset, yOffset, markerRadius);
            fb.circle(mMarkOutline, xOffset, yOffset, markerRadius);
          }
        }
      }

  #endif

      void follow(Euclid *pEuclid) {
        if (mpEuclid) mpEuclid->release();
        mpEuclid = pEuclid;
        if (mpEuclid) mpEuclid->attach();
      }

    private:
      Euclid *mpEuclid = 0;

      od::Color mFill = GRAY5;
      od::Color mOutline = GRAY10;
      od::Color mMarkFill = GRAY8;
      od::Color mMarkOutline = WHITE;
      od::Color mDelineate = WHITE;

      float circleX(float x, float radius, float theta) {
        return (sinf(theta) * radius) + x;
      }

      float circleY(float y, float radius, float theta) {
        return (cosf(theta) * radius) + y;
      }

      void drawRadius(od::FrameBuffer &fb, od::Color color, float x, float y, float radius, float theta) {
        int toX = circleX(x, radius, theta);
        int toY = circleY(y, radius, theta);
        fb.line(color, x, y, toX, toY);
      }

      void fillArc(od::FrameBuffer &fb, od::Color color, float x, float y, float radius, float theta0, float theta1) {
        float dTheta    = fabs(theta1 - theta0);
        float arcLength = ceil(fabs(radius * dTheta));
        float resolution = arcLength * 16.0f; /// 4.0f;// * 10 * 2;

        for (float i = 0; i < resolution; i++) {
          float offset = dTheta * i / resolution;
          drawRadius(fb, color, x, y, radius, theta0 + offset);
        }
      }
  };
}