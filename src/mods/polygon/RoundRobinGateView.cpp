#include <RoundRobinGateView.h>

namespace polygon {
  RoundRobinGateView::RoundRobinGateView(Observable &observable, int left, int bottom, int width, int height) :
      od::Graphic(left, bottom, width, height),
      mObservable(observable) {
    mObservable.attach();
  }
}