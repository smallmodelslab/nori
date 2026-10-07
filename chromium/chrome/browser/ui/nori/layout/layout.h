// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_LAYOUT_LAYOUT_H_
#define CHROME_BROWSER_UI_NORI_LAYOUT_LAYOUT_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// A niri-style scrolling layout. Workspaces stack vertically, each holds a
// horizontal strip of columns, and each column stacks tabs vertically. Every
// function takes a layout by value and returns the next one.
namespace nori {

using TabId = int32_t;

enum class Direction { kLeft, kRight, kUp, kDown };

// Where a new tab goes relative to the focused tab: stacked above or below it
// in its column, or in a new column to its right.
enum class Placement { kAbove, kBelow, kRight };

struct Column {
  std::vector<TabId> tabs;
  size_t focused = 0;
  bool full_width = false;

  bool operator==(const Column&) const = default;
};

struct Workspace {
  std::vector<Column> columns;
  size_t focused = 0;

  bool operator==(const Workspace&) const = default;
};

// Workspaces run top to bottom. The last one is always empty, so there is
// always somewhere new to go, and an empty workspace disappears once focus
// leaves it.
struct Layout {
  std::vector<Workspace> workspaces = {Workspace()};
  size_t focused = 0;

  bool operator==(const Layout&) const = default;
};

struct Location {
  size_t workspace = 0;
  size_t column = 0;
  size_t tab = 0;

  bool operator==(const Location&) const = default;
};

std::optional<Location> Find(const Layout& layout, TabId tab);
std::optional<TabId> FocusedTab(const Layout& layout);

// Every tab, by workspace, then column, then position in the column. The tab
// strip is kept in this order so extensions and tab cycling follow the layout.
std::vector<TabId> Order(const Layout& layout);

// Adds `tab` next to the focused tab without moving focus.
Layout Insert(Layout layout, TabId tab, Placement placement);
Layout Remove(Layout layout, TabId tab);

// Left and right move between columns. Up and down move within the column,
// then on to the workspace above or below.
Layout Focus(Layout layout, Direction direction);
Layout FocusTab(Layout layout, TabId tab);

// Toggles the focused column between its own width and the whole window.
Layout ToggleFullWidth(Layout layout);

// The scroll offset that keeps the focused column fully in view, moving as
// little as possible from `current`. Widths and offsets are in pixels.
int ScrollOffset(const std::vector<int>& column_widths,
                 int gap,
                 size_t focused,
                 int viewport,
                 int current);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_LAYOUT_LAYOUT_H_
