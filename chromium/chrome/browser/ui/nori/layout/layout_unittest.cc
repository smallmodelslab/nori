// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/layout.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace nori {
namespace {

Layout TwoColumns() {
  Layout layout = Insert(Layout(), 1, Placement::kRight);
  layout = Insert(std::move(layout), 2, Placement::kRight);
  return FocusTab(std::move(layout), 2);
}

TEST(NoriLayoutTest, PlacesNewTabsAroundTheFocusedOne) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kAbove);
  layout = Insert(std::move(layout), 4, Placement::kBelow);

  EXPECT_EQ(Order(layout), (std::vector<TabId>{1, 3, 2, 4}));
  EXPECT_EQ(FocusedTab(layout), 2);
}

TEST(NoriLayoutTest, KeepsOneEmptyWorkspaceAtTheBottom) {
  Layout layout = TwoColumns();
  ASSERT_EQ(layout.workspaces.size(), 2u);
  EXPECT_TRUE(layout.workspaces.back().columns.empty());

  layout = Focus(std::move(layout), Direction::kDown);
  layout = Insert(std::move(layout), 3, Placement::kRight);
  ASSERT_EQ(layout.workspaces.size(), 3u);
  EXPECT_TRUE(layout.workspaces.back().columns.empty());

  layout = Remove(std::move(layout), 3);
  EXPECT_EQ(layout.workspaces.size(), 2u);
}

TEST(NoriLayoutTest, UpAndDownLeaveTheColumnForTheNextWorkspace) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kBelow);
  layout = Focus(std::move(layout), Direction::kDown);
  layout = Focus(std::move(layout), Direction::kDown);
  layout = Insert(std::move(layout), 4, Placement::kRight);
  EXPECT_EQ(Find(layout, 4)->workspace, 1u);

  layout = Focus(std::move(layout), Direction::kUp);
  EXPECT_EQ(FocusedTab(layout), 3);
  layout = Focus(std::move(layout), Direction::kUp);
  EXPECT_EQ(FocusedTab(layout), 2);
}

TEST(NoriLayoutTest, LeavingAnEmptyWorkspaceRemovesIt) {
  Layout layout = Focus(TwoColumns(), Direction::kDown);
  layout = Focus(std::move(layout), Direction::kDown);
  ASSERT_EQ(FocusedTab(layout), std::nullopt);

  layout = Focus(std::move(layout), Direction::kUp);
  EXPECT_EQ(layout.workspaces.size(), 2u);
  EXPECT_EQ(FocusedTab(layout), 2);
}

TEST(NoriLayoutTest, ClosingTheFocusedColumnFocusesItsNeighbour) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kRight);
  layout = Remove(std::move(layout), 2);
  EXPECT_EQ(FocusedTab(layout), 3);
  layout = Remove(std::move(layout), 3);
  EXPECT_EQ(FocusedTab(layout), 1);
}

TEST(NoriLayoutTest, TogglesFullWidthOnTheFocusedColumnOnly) {
  Layout layout = ToggleFullWidth(TwoColumns());
  EXPECT_FALSE(layout.workspaces[0].columns[0].full_width);
  EXPECT_TRUE(layout.workspaces[0].columns[1].full_width);
  layout = ToggleFullWidth(std::move(layout));
  EXPECT_FALSE(layout.workspaces[0].columns[1].full_width);
}

TEST(NoriLayoutTest, ScrollsOnlyAsFarAsNeededToShowTheFocusedColumn) {
  const std::vector<int> widths = {500, 500, 500};
  EXPECT_EQ(ScrollOffset(widths, 0, 1, 1000, 0), 0);
  EXPECT_EQ(ScrollOffset(widths, 0, 2, 1000, 0), 500);
  EXPECT_EQ(ScrollOffset(widths, 0, 0, 1000, 500), 0);
  EXPECT_EQ(ScrollOffset({500, 1000}, 0, 1, 1000, 0), 500);
}

}  // namespace
}  // namespace nori
