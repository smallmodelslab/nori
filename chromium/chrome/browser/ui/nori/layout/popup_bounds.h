// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_LAYOUT_POPUP_BOUNDS_H_
#define CHROME_BROWSER_UI_NORI_LAYOUT_POPUP_BOUNDS_H_

#include "ui/gfx/geometry/rect.h"

namespace nori {

inline constexpr int kPopupMaxWidth = 720;
// Smallest gap between the popup and the sides of the window.
inline constexpr int kPopupMargin = 16;
// How far the popup slides down into place as it appears.
inline constexpr int kPopupSlide = 12;

// Where the Ctrl+B popup, `height` tall, sits in `area` once `ratio` of its
// reveal animation has run: centered, sliding down into place. Hidden, it
// waits just above `area`, where it can still take focus.
gfx::Rect PopupBounds(const gfx::Rect& area, int height, double ratio);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_LAYOUT_POPUP_BOUNDS_H_
