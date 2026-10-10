// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/popup_bounds.h"

#include <algorithm>

#include "base/numerics/safe_conversions.h"

namespace nori {

gfx::Rect PopupBounds(const gfx::Rect& area, int height, double ratio) {
  const int width =
      std::clamp(area.width() - 2 * kPopupMargin, 0, kPopupMaxWidth);
  const int x = area.x() + (area.width() - width) / 2;
  if (ratio <= 0) {
    return gfx::Rect(x, area.y() - height, width, height);
  }
  const int slide =
      base::ClampRound((1 - std::min(ratio, 1.0)) * kPopupSlide);
  const int y = area.y() + (area.height() - height) / 2 - slide;
  return gfx::Rect(x, y, width, height);
}

}  // namespace nori
