#pragma once

#include <QList>

#include <algorithm>
#include <cmath>

// Interface scale steps (APP-168) driven from the keyboard: Ctrl+= / Ctrl+-
// walk Theme.scaleSteps, Ctrl+0 goes back to 100 %. Pure, so the rule is
// tested without a window.
namespace heap::ui {

// The step after `current` in `direction` (> 0 up, < 0 down); 0 resets to 1.
// A value between two steps (one stored by an older build, or typed into
// state.json) snaps to the nearest step that way. The ends stay put.
inline double nextUiScale(double current, int direction, QList<double> steps) {
  if(direction == 0 || steps.isEmpty()) {
    return 1.0;
  }
  std::sort(steps.begin(), steps.end());
  constexpr double kEps = 1e-6;
  if(!std::isfinite(current)) {
    current = 1.0;
  }
  if(direction > 0) {
    for(const double s : steps) {
      if(s > current + kEps) {
        return s;
      }
    }
    return steps.last();
  }
  for(auto it = steps.crbegin(); it != steps.crend(); ++it) {
    if(*it < current - kEps) {
      return *it;
    }
  }
  return steps.first();
}

// The interface scale a system text size asks for (APP-183), while the user
// has not picked one in heap: the step of `steps` nearest to `textScale`
// (1.0 = 100 %), a tie going up. Larger text than the top step gets the top
// step; nothing unreadable or below 100 % shrinks heap.
inline double uiScaleForTextScale(double textScale, QList<double> steps) {
  if(steps.isEmpty() || !std::isfinite(textScale) || textScale <= 1.0) {
    return 1.0;
  }
  std::sort(steps.begin(), steps.end());
  double best = steps.first();
  for(const double s : steps) {
    if(std::abs(s - textScale) <= std::abs(best - textScale) + 1e-9) {
      best = s;
    }
  }
  return std::max(best, 1.0);
}

// The scale a screen starts heap at while the user has not picked one
// (APP-314). A desktop monitor whose logical pixels are small — 27"
// 2560×1440 at 100 % (109 ppi), 24" 1440p, 27" 4K at 150 % — gets 125 %, so
// a card title lands near 4 mm at arm's length; 110 % read as small there.
// Other unscaled screens keep 110 %, a screen the system already scales
// (a laptop) 100 %. An unknown size (0, or an EDID that says 3") keeps
// those two.
inline double screenDefaultScale(double logicalWidth, double logicalHeight, double diagonalInches, double pixelRatio) {
  constexpr double kDesktopInches = 20;
  constexpr double kSmallPixelPpi = 100;
  const double fallback = pixelRatio > 1.05 ? 1.0 : 1.1;
  if(!std::isfinite(diagonalInches) || diagonalInches < 10 || diagonalInches > 100 || logicalWidth <= 0 || logicalHeight <= 0) {
    return fallback;
  }
  const double ppi = std::hypot(logicalWidth, logicalHeight) / diagonalInches;
  return diagonalInches >= kDesktopInches && ppi >= kSmallPixelPpi ? 1.25 : fallback;
}

}  // namespace heap::ui
