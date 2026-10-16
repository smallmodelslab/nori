// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/layout_controller.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <utility>

#include "base/auto_reset.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace nori {

namespace {

TabId IdOf(const tabs::TabInterface* tab) {
  return tab->GetHandle().raw_value();
}

gfx::Rect ToRect(const Rect& rect) {
  return gfx::Rect(rect.x, rect.y, rect.width, rect.height);
}

}  // namespace

DEFINE_USER_DATA(LayoutController);

LayoutController::LayoutController(BrowserWindowInterface* browser,
                                   views::View* host)
    : browser_(browser),
      host_(host),
      scoped_unowned_user_data_(browser->GetUnownedUserDataHost(), *this) {
  TabStripModel* model = browser_->GetTabStripModel();
  model->AddObserver(this);

  std::vector<TabId> tabs;
  for (int i = 0; i < model->count(); ++i) {
    tabs.push_back(IdOf(model->GetTabAtIndex(i)));
  }
  Insert(tabs);
  if (std::optional<TabId> active = ActiveTab()) {
    layout_ = FocusTab(std::move(layout_), *active);
  }
  SetViewportSize(host_->size());
}

LayoutController::~LayoutController() {
  StopAnimating();
  browser_->GetTabStripModel()->RemoveObserver(this);
  for (auto& [tab, peer] : peers_) {
    host_->RemoveChildViewT(peer.view.ExtractAsDangling());
  }
}

// static
LayoutController* LayoutController::From(BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

void LayoutController::Focus(Direction direction) {
  insert_anchor_.reset();
  SetLayout(nori::Focus(layout_, direction));
  SyncTabStrip();
}

void LayoutController::ToggleMaximize() {
  SetLayout(nori::ToggleMaximize(layout_));
}

void LayoutController::OpenWithPlacement(Placement placement,
                                         base::FunctionRef<void()> open) {
  // Tabs are inserted, and so placed, before opening returns.
  insert_anchor_.reset();
  base::AutoReset<std::optional<Placement>> override(&placement_override_,
                                                     placement);
  open();
}

void LayoutController::SetOptions(const LayoutOptions& options) {
  if (options == options_) {
    return;
  }
  options_ = options;
  geometry_.gap = options.gap;
  SetLayout(layout_);
}

void LayoutController::Release(content::WebContents* contents) {
  for (auto& [tab, peer] : peers_) {
    if (peer.view->web_contents() == contents) {
      peer.view->SetWebContents(nullptr);
    }
  }
}

void LayoutController::AddChildLayouts(
    const gfx::Size& size,
    views::View* active_container,
    std::vector<views::ChildLayout>& layouts) const {
  std::vector<Tile> tiles;
  if (size == gfx::Size(geometry_.width, geometry_.height)) {
    tiles = CurrentTiles(base::TimeTicks::Now());
  } else {
    // Views also asks for minimum and hypothetical sizes. Project those without
    // changing the live viewport, focus, scroll position, or springs.
    Geometry geometry = geometry_;
    geometry.width = size.width();
    geometry.height = size.height();
    const Layout projected = ScrollIntoView(layout_, geometry);
    tiles = Tiles(projected, geometry,
                  projected.workspaces[projected.focused].scroll,
                  StackScroll(projected, geometry));
  }

  const std::optional<TabId> active = ActiveTab();
  // An active tab that is out of view waits just past the right edge.
  gfx::Rect active_bounds(gfx::Point(size.width(), 0), size);
  std::set<TabId> placed;
  for (const Tile& tile : tiles) {
    if (tile.tab == active) {
      active_bounds = ToRect(tile.bounds);
    } else if (auto it = peers_.find(tile.tab); it != peers_.end()) {
      layouts.emplace_back(it->second.view.get(), true, ToRect(tile.bounds));
      placed.insert(tile.tab);
    }
  }
  for (auto& [tab, peer] : peers_) {
    if (!placed.contains(tab)) {
      layouts.emplace_back(peer.view.get(), false, gfx::Rect());
    }
  }
  layouts.emplace_back(active_container, true, active_bounds);
}

void LayoutController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  switch (change.type()) {
    case TabStripModelChange::kInserted: {
      std::vector<TabId> tabs;
      for (const auto& inserted : change.GetInsert()->contents) {
        tabs.push_back(IdOf(inserted.tab));
      }
      Insert(tabs);
      ScheduleSyncTabStrip();
      break;
    }
    case TabStripModelChange::kRemoved: {
      Layout layout = layout_;
      for (const auto& removed : change.GetRemove()->contents) {
        const TabId tab = IdOf(removed.tab);
        layout = Remove(std::move(layout), tab);
        if (insert_anchor_ == tab) {
          insert_anchor_.reset();
        }
      }
      SetLayout(std::move(layout));
      // The layout, not the tab strip, decides which neighbour takes focus.
      ScheduleSyncTabStrip();
      return;
    }
    case TabStripModelChange::kMoved:
      if (!syncing_tab_strip_) {
        ScheduleSyncTabStrip();
      }
      break;
    case TabStripModelChange::kReplaced:
    case TabStripModelChange::kSelectionOnly:
      break;
  }

  if (selection.active_tab_changed() && selection.new_tab) {
    insert_anchor_.reset();
    SetLayout(FocusTab(layout_, IdOf(selection.new_tab)));
    // A native view transfer can queue focus for the previous page. Keep the
    // new selection authoritative until that handoff has finished.
    if (!syncing_tab_strip_) {
      ScheduleSyncTabStrip();
    }
  } else {
    UpdatePeers();
    Relayout();
  }
}

void LayoutController::OnAnimationStep(base::TimeTicks timestamp) {
  if (!scroll_.IsAnimatingAt(timestamp) && !stack_.IsAnimatingAt(timestamp) &&
      !resize_.IsAnimatingAt(timestamp)) {
    resize_from_.clear();
    StopAnimating();
  }
  UpdatePeers();
  Relayout();
}

void LayoutController::OnCompositingShuttingDown(ui::Compositor* compositor) {
  StopAnimating();
}

void LayoutController::Insert(const std::vector<TabId>& tabs) {
  TabStripModel* model = browser_->GetTabStripModel();
  Layout layout = layout_;
  const std::optional<TabId> focused = FocusedTab(layout);
  for (TabId tab : tabs) {
    if (insert_anchor_) {
      layout = FocusTab(std::move(layout), *insert_anchor_);
    }
    const int index = model->GetIndexOfTab(tabs::TabHandle(tab).Get());
    const bool has_opener =
        index != TabStripModel::kNoTab && model->GetOpenerOfTabAt(index);
    const Placement placement = placement_override_.value_or(
        has_opener ? options_.placement : Placement::kRight);
    layout = nori::Insert(std::move(layout), tab, placement);
    insert_anchor_ = tab;
  }
  if (focused) {
    layout = FocusTab(std::move(layout), *focused);
  }
  SetLayout(std::move(layout));
}

void LayoutController::SetLayout(Layout layout) {
  layout = ScrollIntoView(std::move(layout), geometry_);
  const base::TimeTicks now = base::TimeTicks::Now();
  const Workspace& focused = layout.workspaces[layout.focused];
  const Workspace& was_focused = layout_.workspaces[layout_.focused];

  // Workspaces above may have vanished, which moves every index; keep the
  // vertical motion attached to the workspace that is now focused.
  if (std::optional<size_t> before = FindWorkspace(layout_, focused.id)) {
    stack_.Shift(
        (static_cast<double>(layout.focused) - static_cast<double>(*before)) *
        geometry_.height);
  }

  const bool can_animate =
      host_->GetWidget() && host_->GetWidget()->IsVisible() &&
      geometry_.width > 0 && gfx::Animation::ShouldRenderRichAnimation();
  if (!can_animate) {
    scroll_.JumpTo(focused.scroll);
    stack_.JumpTo(StackScroll(layout, geometry_));
  } else {
    if (focused.id == was_focused.id) {
      scroll_.AnimateTo(focused.scroll, now, options_.horizontal);
    } else {
      scroll_.JumpTo(focused.scroll);
    }
    stack_.AnimateTo(StackScroll(layout, geometry_), now, options_.vertical);
  }

  if (can_animate && focused.id == was_focused.id &&
      focused.columns.size() == was_focused.columns.size() &&
      focused.focused == was_focused.focused && !focused.columns.empty()) {
    const Column& column = focused.columns[focused.focused];
    const Column& was_column = was_focused.columns[was_focused.focused];
    if (column.root != was_column.root ||
        column.maximized != was_column.maximized) {
      const auto before = CurrentTiles(now);
      resize_from_.clear();
      for (const Tile& tile : before)
        resize_from_.emplace(tile.tab, tile.bounds);
      resize_.JumpTo(0);
      SpringParams resize_params = options_.horizontal;
      resize_params.epsilon /=
          std::max(1, std::max(geometry_.width, geometry_.height));
      resize_.AnimateTo(1, now, resize_params);
    }
  } else {
    resize_from_.clear();
    resize_.JumpTo(1);
  }

  layout_ = std::move(layout);
  UpdatePeers();
  Relayout();
  if (scroll_.IsAnimatingAt(now) || stack_.IsAnimatingAt(now) ||
      resize_.IsAnimatingAt(now)) {
    StartAnimating();
  }
}

void LayoutController::SetViewportSize(const gfx::Size& size) {
  if (size == gfx::Size(geometry_.width, geometry_.height)) {
    return;
  }
  resize_from_.clear();
  resize_.JumpTo(1);
  geometry_.width = size.width();
  geometry_.height = size.height();
  layout_ = ScrollIntoView(std::move(layout_), geometry_);
  scroll_.JumpTo(layout_.workspaces[layout_.focused].scroll);
  stack_.JumpTo(StackScroll(layout_, geometry_));
  // Peers are views, which may not be added or removed mid-layout.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&LayoutController::UpdatePeers,
                                weak_factory_.GetWeakPtr()));
}

std::vector<Tile> LayoutController::CurrentTiles(base::TimeTicks now) const {
  auto tiles =
      Tiles(layout_, geometry_, scroll_.ValueAt(now), stack_.ValueAt(now));
  const double progress = std::clamp(resize_.ValueAt(now), 0.0, 1.0);
  for (Tile& tile : tiles) {
    if (auto it = resize_from_.find(tile.tab); it != resize_from_.end()) {
      const Rect& from = it->second;
      const auto blend = [progress](int a, int b) {
        return static_cast<int>(std::lround(a + (b - a) * progress));
      };
      tile.bounds = {blend(from.x, tile.bounds.x), blend(from.y, tile.bounds.y),
                     blend(from.width, tile.bounds.width),
                     blend(from.height, tile.bounds.height)};
    }
  }
  return tiles;
}

void LayoutController::UpdatePeers() {
  if (geometry_.width == 0) {
    return;
  }
  const std::optional<TabId> active = ActiveTab();
  std::set<TabId> wanted;
  for (const Tile& tile : CurrentTiles(base::TimeTicks::Now())) {
    if (tile.tab != active && ContentsFor(tile.tab)) {
      wanted.insert(tile.tab);
    }
  }

  for (auto it = peers_.begin(); it != peers_.end();) {
    if (wanted.contains(it->first)) {
      ++it;
      continue;
    }
    views::WebView* view = it->second.view.ExtractAsDangling();
    it = peers_.erase(it);
    host_->RemoveChildViewT(view);
  }

  for (TabId tab : wanted) {
    content::WebContents* contents = ContentsFor(tab);
    auto [it, created] = peers_.try_emplace(tab);
    Peer& peer = it->second;
    if (created) {
      peer.view = host_->AddChildView(
          std::make_unique<views::WebView>(browser_->GetProfile()));
      peer.focused =
          peer.view->AddWebContentsFocusedCallback(base::BindRepeating(
              &LayoutController::OnPeerFocused, base::Unretained(this)));
    }
    if (peer.view->web_contents() != contents) {
      peer.view->SetWebContents(contents);
    }
  }
}

void LayoutController::OnPeerFocused(views::WebView* view) {
  for (const auto& [tab, peer] : peers_) {
    if (peer.view != view) {
      continue;
    }
    // Native focus also fires while Chromium transfers the active tab between
    // views. It must not override the neighbour Nori is about to activate.
    const std::optional<TabId> active = ActiveTab();
    if (sync_scheduled_ || syncing_tab_strip_ || tab == active) {
      return;
    }
    // Activating moves this page out of the peer that is notifying us.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](base::WeakPtr<LayoutController> self, TabId tab,
               std::optional<TabId> active) {
              if (!self || self->sync_scheduled_ || self->syncing_tab_strip_ ||
                  self->ActiveTab() != active) {
                return;
              }
              TabStripModel* model = self->browser_->GetTabStripModel();
              const int index =
                  model->GetIndexOfTab(tabs::TabHandle(tab).Get());
              if (index != TabStripModel::kNoTab) {
                model->ActivateTabAt(
                    index,
                    TabStripUserGestureDetails(
                        TabStripUserGestureDetails::GestureType::kMouse));
              }
            },
            weak_factory_.GetWeakPtr(), tab, active));
    return;
  }
}

void LayoutController::StartAnimating() {
  if (compositor_ || !host_->GetWidget()) {
    return;
  }
  compositor_ = host_->GetWidget()->GetCompositor();
  if (compositor_) {
    compositor_->AddAnimationObserver(this);
  }
}

void LayoutController::StopAnimating() {
  if (compositor_) {
    compositor_->RemoveAnimationObserver(this);
    compositor_ = nullptr;
  }
}

void LayoutController::Relayout() {
  host_->InvalidateLayout();
  if (views::Widget* widget = host_->GetWidget()) {
    widget->LayoutRootViewIfNecessary();
  }
}

void LayoutController::ScheduleSyncTabStrip() {
  if (sync_scheduled_) {
    return;
  }
  sync_scheduled_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&LayoutController::SyncTabStrip,
                                weak_factory_.GetWeakPtr()));
}

void LayoutController::SyncTabStrip() {
  sync_scheduled_ = false;
  TabStripModel* model = browser_->GetTabStripModel();
  base::AutoReset<bool> syncing(&syncing_tab_strip_, true);
  {
    const std::vector<TabId> order = Order(layout_);
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
      const int from = model->GetIndexOfTab(tabs::TabHandle(order[i]).Get());
      if (from == TabStripModel::kNoTab || from == i) {
        continue;
      }
      // Pinned tabs and groups can refuse a position; leave the rest alone.
      if (model->MoveWebContentsAt(from, i, /*select_after_move=*/false) != i) {
        break;
      }
    }
  }

  const std::optional<TabId> focused = FocusedTab(layout_);
  if (focused && focused != ActiveTab()) {
    const int index = model->GetIndexOfTab(tabs::TabHandle(*focused).Get());
    if (index != TabStripModel::kNoTab) {
      model->ActivateTabAt(
          index, TabStripUserGestureDetails(
                     TabStripUserGestureDetails::GestureType::kKeyboard));
    }
  }
}

content::WebContents* LayoutController::ContentsFor(TabId tab) const {
  tabs::TabInterface* interface = tabs::TabHandle(tab).Get();
  if (!interface || browser_->GetTabStripModel()->GetIndexOfTab(interface) ==
                        TabStripModel::kNoTab) {
    return nullptr;
  }
  return interface->GetContents();
}

std::optional<TabId> LayoutController::ActiveTab() const {
  const tabs::TabInterface* tab = browser_->GetTabStripModel()->GetActiveTab();
  if (!tab) {
    return std::nullopt;
  }
  return IdOf(tab);
}

}  // namespace nori
