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

constexpr Geometry kWindow{.width = 1000, .height = 800};

int Scroll(const Layout& layout) {
  return layout.workspaces[layout.focused].scroll;
}

Layout FocusAndScroll(Layout layout, Direction direction) {
  return ScrollIntoView(Focus(std::move(layout), direction), kWindow);
}

TEST(NoriLayoutTest, ScrollsOnlyAsFarAsNeededToShowTheFocusedColumn) {
  Layout layout = ScrollIntoView(Insert(TwoColumns(), 3, Placement::kRight),
                                 kWindow);
  EXPECT_EQ(Scroll(layout), 0);

  layout = FocusAndScroll(std::move(layout), Direction::kRight);
  EXPECT_EQ(Scroll(layout), 500);
  layout = FocusAndScroll(std::move(layout), Direction::kLeft);
  EXPECT_EQ(Scroll(layout), 500);
  layout = FocusAndScroll(std::move(layout), Direction::kLeft);
  EXPECT_EQ(Scroll(layout), 0);
}

TEST(NoriLayoutTest, AFullWidthColumnFillsTheWindow) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kRight);
  layout = FocusAndScroll(std::move(layout), Direction::kRight);
  layout = ScrollIntoView(ToggleFullWidth(std::move(layout)), kWindow);

  EXPECT_EQ(Scroll(layout), 1000);
  EXPECT_EQ(Tiles(layout, kWindow, Scroll(layout), 0),
            (std::vector<Tile>{{3, {0, 0, 1000, 800}}}));
}

TEST(NoriLayoutTest, ClosingTheLastColumnDoesNotLeaveAGap) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kRight);
  layout = FocusAndScroll(std::move(layout), Direction::kRight);
  layout = ScrollIntoView(Remove(std::move(layout), 3), kWindow);
  EXPECT_EQ(Scroll(layout), 0);
}

TEST(NoriLayoutTest, TilesSplitAColumnAndFollowTheWorkspaceStack) {
  Layout layout = Insert(TwoColumns(), 3, Placement::kBelow);
  EXPECT_EQ(Tiles(layout, kWindow, 0, 0),
            (std::vector<Tile>{{1, {0, 0, 500, 800}},
                               {2, {500, 0, 500, 400}},
                               {3, {500, 400, 500, 400}}}));

  layout = Focus(Focus(std::move(layout), Direction::kDown), Direction::kDown);
  ASSERT_EQ(StackScroll(layout, kWindow), 800);
  EXPECT_EQ(Tiles(layout, kWindow, 0, 400),
            (std::vector<Tile>{{1, {0, -400, 500, 800}},
                               {2, {500, -400, 500, 400}},
                               {3, {500, 0, 500, 400}}}));
  EXPECT_TRUE(Tiles(layout, kWindow, 0, 800).empty());
}

}  // namespace
}  // namespace nori
