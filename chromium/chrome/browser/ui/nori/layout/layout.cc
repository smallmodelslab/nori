// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/layout/layout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nori {

namespace {

Pane* FindPane(Pane& pane, TabId tab) {
  if (pane.children.empty())
    return pane.tab == tab ? &pane : nullptr;
  for (Pane& child : pane.children) {
    if (Pane* found = FindPane(child, tab))
      return found;
  }
  return nullptr;
}

const Pane* FindPane(const Pane& pane, TabId tab) {
  if (pane.children.empty())
    return pane.tab == tab ? &pane : nullptr;
  for (const Pane& child : pane.children) {
    if (const Pane* found = FindPane(child, tab))
      return found;
  }
  return nullptr;
}

void AppendOrder(const Pane& pane, std::vector<TabId>& order) {
  if (pane.children.empty()) {
    order.push_back(pane.tab);
  } else {
    for (const Pane& child : pane.children)
      AppendOrder(child, order);
  }
}

// Collapsing a branch gives its entire rectangle to the surviving sibling.
bool RemovePane(Pane& pane, TabId tab, TabId& focused) {
  if (pane.children.empty())
    return pane.tab == tab;
  for (size_t i = 0; i < pane.children.size(); ++i) {
    if (RemovePane(pane.children[i], tab, focused)) {
      Pane sibling = std::move(pane.children[1 - i]);
      pane = std::move(sibling);
      if (focused == tab) {
        const Pane* first = &pane;
        while (!first->children.empty())
          first = &first->children.front();
        focused = first->tab;
      }
      return false;
    }
  }
  return false;
}

void AppendTiles(const Pane& pane,
                 const Rect& bounds,
                 int gap,
                 std::vector<Tile>& tiles) {
  if (pane.children.empty()) {
    tiles.push_back({pane.tab, bounds});
    return;
  }
  const int length = pane.horizontal ? bounds.width : bounds.height;
  const int spacing = std::min(gap, std::max(0, length - 2));
  const int first = (length - spacing) / 2;
  Rect a = bounds;
  Rect b = bounds;
  if (pane.horizontal) {
    a.width = first;
    b.x += first + spacing;
    b.width -= first + spacing;
  } else {
    a.height = first;
    b.y += first + spacing;
    b.height -= first + spacing;
  }
  AppendTiles(pane.children[0], a, gap, tiles);
  AppendTiles(pane.children[1], b, gap, tiles);
}

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
      if (!column.root) {
        continue;
      }
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
    const int width = geometry.width;
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
      if (FindPane(*columns[c].root, tab)) {
        return Location{w, c};
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
  return column.focused;
}

std::vector<TabId> Order(const Layout& layout) {
  std::vector<TabId> order;
  for (const Workspace& workspace : layout.workspaces) {
    for (const Column& column : workspace.columns) {
      AppendOrder(*column.root, order);
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
    workspace.columns.push_back(
        Column{.root = Pane{.tab = tab}, .focused = tab});
    return Normalize(std::move(layout));
  }

  Column& column = FocusedColumn(workspace);
  if (placement == Placement::kRight) {
    workspace.columns.insert(workspace.columns.begin() + workspace.focused + 1,
                             Column{.root = Pane{.tab = tab}, .focused = tab});
  } else {
    Pane* target = FindPane(*column.root, column.focused);
    Pane previous = std::move(*target);
    *target = Pane{.horizontal = placement == Placement::kSplitRight};
    target->children = {std::move(previous), Pane{.tab = tab}};
    if (placement == Placement::kAbove) {
      std::swap(target->children[0], target->children[1]);
    }
    column.maximized = false;
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
  if (column.root->children.empty()) {
    column.root.reset();
  } else {
    RemovePane(*column.root, tab, column.focused);
  }
  return Normalize(std::move(layout));
}

Layout Focus(Layout layout, Direction direction) {
  Workspace& workspace = layout.workspaces[layout.focused];
  const bool empty = IsEmpty(workspace);
  if (!empty) {
    Column& column = FocusedColumn(workspace);
    // Find the closest pane in the requested direction within the split tree.
    std::vector<Tile> panes;
    AppendTiles(*column.root, {0, 0, 10000, 10000}, 0, panes);
    auto current = std::ranges::find(panes, column.focused, &Tile::tab);
    const bool horizontal =
        direction == Direction::kLeft || direction == Direction::kRight;
    const bool positive =
        direction == Direction::kRight || direction == Direction::kDown;
    const Rect& from = current->bounds;
    const Tile* best = nullptr;
    int best_distance = 0;
    for (const Tile& candidate : panes) {
      const Rect& to = candidate.bounds;
      const int along = horizontal ? to.x - from.x : to.y - from.y;
      const bool overlaps =
          horizontal ? to.y < from.y + from.height && to.y + to.height > from.y
                     : to.x < from.x + from.width && to.x + to.width > from.x;
      if (candidate.tab == column.focused || !overlaps ||
          (positive ? along <= 0 : along >= 0)) {
        continue;
      }
      if (!best || std::abs(along) < best_distance) {
        best = &candidate;
        best_distance = std::abs(along);
      }
    }
    if (best) {
      column.focused = best->tab;
      return layout;
    }
  }
  switch (direction) {
    case Direction::kLeft:
      if (workspace.focused > 0)
        --workspace.focused;
      break;
    case Direction::kRight:
      if (workspace.focused + 1 < workspace.columns.size())
        ++workspace.focused;
      break;
    case Direction::kUp:
      if (layout.focused > 0)
        --layout.focused;
      break;
    case Direction::kDown:
      if (layout.focused + 1 < layout.workspaces.size())
        ++layout.focused;
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
  FocusedColumn(workspace).focused = tab;
  return Normalize(std::move(layout));
}

Layout ToggleMaximize(Layout layout) {
  Workspace& workspace = layout.workspaces[layout.focused];
  if (!IsEmpty(workspace)) {
    FocusedColumn(workspace).maximized = !FocusedColumn(workspace).maximized;
  }
  return layout;
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
      const int width = geometry.width;
      if (left < geometry.width && left + width > 0) {
        const Rect bounds{left, top, width, geometry.height};
        if (column.maximized) {
          tiles.push_back({column.focused, bounds});
        } else {
          AppendTiles(*column.root, bounds, geometry.gap, tiles);
        }
      }
      left += width + geometry.gap;
    }
  }
  return tiles;
}

}  // namespace nori
