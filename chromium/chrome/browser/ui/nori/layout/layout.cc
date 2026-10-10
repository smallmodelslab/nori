// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/layout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nori {

namespace {

bool IsEmpty(const Workspace& workspace) {
  return workspace.columns.empty();
}

// Drops empty columns and workspaces, keeps focus on the same tab or on the
// neighbour that slid into its place, and restores the trailing empty
// workspace.
Layout Normalize(Layout layout) {
  Layout result;
  result.workspaces.clear();

  for (size_t w = 0; w < layout.workspaces.size(); ++w) {
    Workspace& workspace = layout.workspaces[w];
    Workspace kept;
    kept.id = workspace.id;
    kept.scroll = workspace.scroll;
    for (size_t c = 0; c < workspace.columns.size(); ++c) {
      Column& column = workspace.columns[c];
      if (column.tabs.empty()) {
        continue;
      }
      column.focused = std::min(column.focused, column.tabs.size() - 1);
      if (c < workspace.focused) {
        ++kept.focused;
      }
      kept.columns.push_back(std::move(column));
    }
    if (!kept.columns.empty()) {
      kept.focused = std::min(kept.focused, kept.columns.size() - 1);
    } else {
      kept.focused = 0;
    }

    if (IsEmpty(kept) && w != layout.focused) {
      continue;
    }
    if (w == layout.focused) {
      result.focused = result.workspaces.size();
    }
    result.workspaces.push_back(std::move(kept));
  }

  if (!IsEmpty(result.workspaces.back())) {
    Workspace trailing;
    for (const Workspace& workspace : layout.workspaces) {
      trailing.id = std::max(trailing.id, workspace.id + 1);
    }
    result.workspaces.push_back(std::move(trailing));
  }
  return result;
}

Column& FocusedColumn(Workspace& workspace) {
  return workspace.columns[workspace.focused];
}

// Moves `scroll` as little as possible to show the focused column, without
// scrolling past the last column.
int ScrollToShow(const Workspace& workspace,
                 const Geometry& geometry,
                 int scroll) {
  int left = 0;
  int right = 0;
  int total = 0;
  for (size_t c = 0; c < workspace.columns.size(); ++c) {
    const int width = ColumnWidth(workspace.columns[c], geometry);
    if (c == workspace.focused) {
      left = total;
      right = total + width;
    }
    total += width + (c + 1 < workspace.columns.size() ? geometry.gap : 0);
  }
  if (right - left >= geometry.width || left < scroll) {
    scroll = left;
  } else if (right > scroll + geometry.width) {
    scroll = right - geometry.width;
  }
  return std::clamp(scroll, 0, std::max(0, total - geometry.width));
}

}  // namespace

std::optional<Location> Find(const Layout& layout, TabId tab) {
  for (size_t w = 0; w < layout.workspaces.size(); ++w) {
    const auto& columns = layout.workspaces[w].columns;
    for (size_t c = 0; c < columns.size(); ++c) {
      const auto& tabs = columns[c].tabs;
      auto it = std::ranges::find(tabs, tab);
      if (it != tabs.end()) {
        return Location{w, c, static_cast<size_t>(it - tabs.begin())};
      }
    }
  }
  return std::nullopt;
}

std::optional<size_t> FindWorkspace(const Layout& layout, int id) {
  auto it = std::ranges::find(layout.workspaces, id, &Workspace::id);
  if (it == layout.workspaces.end()) {
    return std::nullopt;
  }
  return static_cast<size_t>(it - layout.workspaces.begin());
}

std::optional<TabId> FocusedTab(const Layout& layout) {
  const Workspace& workspace = layout.workspaces[layout.focused];
  if (IsEmpty(workspace)) {
    return std::nullopt;
  }
  const Column& column = workspace.columns[workspace.focused];
  return column.tabs[column.focused];
}

std::vector<TabId> Order(const Layout& layout) {
  std::vector<TabId> order;
  for (const Workspace& workspace : layout.workspaces) {
    for (const Column& column : workspace.columns) {
      order.insert(order.end(), column.tabs.begin(), column.tabs.end());
    }
  }
  return order;
}

Layout Insert(Layout layout, TabId tab, Placement placement) {
  if (Find(layout, tab)) {
    return layout;
  }
  Workspace& workspace = layout.workspaces[layout.focused];
  if (IsEmpty(workspace)) {
    workspace.columns.push_back(Column{.tabs = {tab}});
    return Normalize(std::move(layout));
  }

  Column& column = FocusedColumn(workspace);
  switch (placement) {
    case Placement::kAbove:
      column.tabs.insert(column.tabs.begin() + column.focused, tab);
      ++column.focused;
      break;
    case Placement::kBelow:
      column.tabs.insert(column.tabs.begin() + column.focused + 1, tab);
      break;
    case Placement::kRight:
      workspace.columns.insert(workspace.columns.begin() + workspace.focused + 1,
                               Column{.tabs = {tab}});
      break;
  }
  return Normalize(std::move(layout));
}

Layout Remove(Layout layout, TabId tab) {
  std::optional<Location> location = Find(layout, tab);
  if (!location) {
    return layout;
  }
  Column& column =
      layout.workspaces[location->workspace].columns[location->column];
  column.tabs.erase(column.tabs.begin() + location->tab);
  if (location->tab < column.focused) {
    --column.focused;
  }
  return Normalize(std::move(layout));
}

Layout Focus(Layout layout, Direction direction) {
  Workspace& workspace = layout.workspaces[layout.focused];
  const bool empty = IsEmpty(workspace);
  switch (direction) {
    case Direction::kLeft:
      if (workspace.focused > 0) {
        --workspace.focused;
      }
      break;
    case Direction::kRight:
      if (workspace.focused + 1 < workspace.columns.size()) {
        ++workspace.focused;
      }
      break;
    case Direction::kUp:
      if (!empty && FocusedColumn(workspace).focused > 0) {
        --FocusedColumn(workspace).focused;
      } else if (layout.focused > 0) {
        --layout.focused;
      }
      break;
    case Direction::kDown:
      if (!empty && FocusedColumn(workspace).focused + 1 <
                        FocusedColumn(workspace).tabs.size()) {
        ++FocusedColumn(workspace).focused;
      } else if (layout.focused + 1 < layout.workspaces.size()) {
        ++layout.focused;
      }
      break;
  }
  return Normalize(std::move(layout));
}

Layout FocusTab(Layout layout, TabId tab) {
  std::optional<Location> location = Find(layout, tab);
  if (!location) {
    return layout;
  }
  layout.focused = location->workspace;
  Workspace& workspace = layout.workspaces[location->workspace];
  workspace.focused = location->column;
  FocusedColumn(workspace).focused = location->tab;
  return Normalize(std::move(layout));
}

Layout ToggleFullWidth(Layout layout) {
  Workspace& workspace = layout.workspaces[layout.focused];
  if (!IsEmpty(workspace)) {
    FocusedColumn(workspace).full_width = !FocusedColumn(workspace).full_width;
  }
  return layout;
}

int ColumnWidth(const Column& column, const Geometry& geometry) {
  if (column.full_width) {
    return geometry.width;
  }
  return static_cast<int>(std::lround(geometry.width * geometry.column_fraction));
}

Layout ScrollIntoView(Layout layout, const Geometry& geometry) {
  for (Workspace& workspace : layout.workspaces) {
    workspace.scroll = ScrollToShow(workspace, geometry, workspace.scroll);
  }
  return layout;
}

int StackScroll(const Layout& layout, const Geometry& geometry) {
  return static_cast<int>(layout.focused) * geometry.height;
}

std::vector<Tile> Tiles(const Layout& layout,
                        const Geometry& geometry,
                        double focused_scroll,
                        double stack_scroll) {
  std::vector<Tile> tiles;
  const int stack = static_cast<int>(std::lround(stack_scroll));
  for (size_t w = 0; w < layout.workspaces.size(); ++w) {
    const Workspace& workspace = layout.workspaces[w];
    const int top = static_cast<int>(w) * geometry.height - stack;
    if (top >= geometry.height || top + geometry.height <= 0) {
      continue;
    }
    const int scroll = w == layout.focused
                           ? static_cast<int>(std::lround(focused_scroll))
                           : workspace.scroll;
    int left = -scroll;
    for (const Column& column : workspace.columns) {
      const int width = ColumnWidth(column, geometry);
      if (left < geometry.width && left + width > 0) {
        const int count = static_cast<int>(column.tabs.size());
        for (int i = 0; i < count; ++i) {
          const int y = top + i * geometry.height / count;
          const int bottom = top + (i + 1) * geometry.height / count;
          tiles.push_back({column.tabs[i], {left, y, width, bottom - y}});
        }
      }
      left += width + geometry.gap;
    }
  }
  return tiles;
}

}  // namespace nori
