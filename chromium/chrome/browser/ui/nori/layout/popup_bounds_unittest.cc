// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/popup_bounds.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace nori {
namespace {

constexpr gfx::Rect kWindow(0, 0, 1600, 1000);
constexpr int kHeight = 40;

TEST(NoriPopupBoundsTest, ShownIsCentered) {
  EXPECT_EQ(PopupBounds(kWindow, kHeight, 1),
            gfx::Rect((1600 - kPopupMaxWidth) / 2, (1000 - kHeight) / 2,
                      kPopupMaxWidth, kHeight));
}

TEST(NoriPopupBoundsTest, NarrowWindowsKeepMargins) {
  const gfx::Rect bounds = PopupBounds(gfx::Rect(100, 50, 400, 300), kHeight, 1);
  EXPECT_EQ(bounds.x(), 100 + kPopupMargin);
  EXPECT_EQ(bounds.width(), 400 - 2 * kPopupMargin);
  EXPECT_EQ(bounds.y(), 50 + (300 - kHeight) / 2);
}

TEST(NoriPopupBoundsTest, HiddenWaitsAboveTheWindow) {
  const gfx::Rect bounds = PopupBounds(kWindow, kHeight, 0);
  EXPECT_EQ(bounds.bottom(), kWindow.y());
  EXPECT_EQ(bounds.height(), kHeight);
  EXPECT_EQ(bounds.x(), PopupBounds(kWindow, kHeight, 1).x());
}

TEST(NoriPopupBoundsTest, SlidesDownIntoPlace) {
  const int shown_y = PopupBounds(kWindow, kHeight, 1).y();
  EXPECT_EQ(PopupBounds(kWindow, kHeight, 0.01).y(), shown_y - kPopupSlide);
  EXPECT_EQ(PopupBounds(kWindow, kHeight, 0.5).y(), shown_y - kPopupSlide / 2);
}

TEST(NoriPopupBoundsTest, TinyWindowsGiveAnEmptyPopup) {
  EXPECT_EQ(PopupBounds(gfx::Rect(0, 0, 20, 20), kHeight, 1).width(), 0);
}

}  // namespace
}  // namespace nori
