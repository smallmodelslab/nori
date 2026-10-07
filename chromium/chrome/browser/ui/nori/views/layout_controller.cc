// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/layout_controller.h"

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

void LayoutController::ToggleFullWidth() {
  SetLayout(nori::ToggleFullWidth(layout_));
}

void LayoutController::OpenWithPlacement(Placement placement,
                                         base::FunctionRef<void()> open) {
  // Tabs are inserted, and so placed, before opening returns.
  base::AutoReset<std::optional<Placement>> override(&placement_override_,
                                                     placement);
  open();
}

void LayoutController::SetOptions(const LayoutOptions& options) {
  if (options == options_) {
    return;
  }
  options_ = options;
  geometry_.column_fraction = options.column_fraction;
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
    std::vector<views::ChildLayout>& layouts) {
  if (size != gfx::Size(geometry_.width, geometry_.height)) {
    SetGeometry(size);
  }

  const std::optional<TabId> active = ActiveTab();
  // An active tab that is out of view waits just past the right edge.
  gfx::Rect active_bounds(gfx::Point(size.width(), 0), size);
  std::set<TabId> placed;
  for (const Tile& tile : CurrentTiles(base::TimeTicks::Now())) {
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
  } else {
    UpdatePeers();
    Relayout();
  }
}

void LayoutController::OnAnimationStep(base::TimeTicks timestamp) {
  if (!scroll_.IsAnimatingAt(timestamp) && !stack_.IsAnimatingAt(timestamp)) {
    StopAnimating();
  }
  UpdatePeers();
  Relayout();
}

void LayoutController::OnCompositingShuttingDown(ui::Compositor* compositor) {
  StopAnimating();
}

void LayoutController::Insert(const std::vector<TabId>& tabs) {
  const Placement placement = placement_override_.value_or(options_.placement);

  Layout layout = layout_;
  const std::optional<TabId> focused = FocusedTab(layout);
  for (TabId tab : tabs) {
    if (insert_anchor_) {
      layout = FocusTab(std::move(layout), *insert_anchor_);
    }
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
    stack_.Shift((static_cast<double>(layout.focused) -
                  static_cast<double>(*before)) *
                 geometry_.height);
  }

  const bool can_animate = host_->GetWidget() &&
                           host_->GetWidget()->IsVisible() &&
                           geometry_.width > 0;
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

  layout_ = std::move(layout);
  UpdatePeers();
  Relayout();
  if (scroll_.IsAnimatingAt(now) || stack_.IsAnimatingAt(now)) {
    StartAnimating();
  }
}

void LayoutController::SetGeometry(const gfx::Size& size) {
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
  return Tiles(layout_, geometry_, scroll_.ValueAt(now), stack_.ValueAt(now));
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
      peer.focused = peer.view->AddWebContentsFocusedCallback(
          base::BindRepeating(&LayoutController::OnPeerFocused,
                              base::Unretained(this)));
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
    // Activating moves this page out of the peer that is notifying us.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(
                       [](base::WeakPtr<LayoutController> self, TabId tab) {
                         if (!self) {
                           return;
                         }
                         TabStripModel* model =
                             self->browser_->GetTabStripModel();
                         const int index =
                             model->GetIndexOfTab(tabs::TabHandle(tab).Get());
                         if (index != TabStripModel::kNoTab) {
                           model->ActivateTabAt(
                               index, TabStripUserGestureDetails(
                                          TabStripUserGestureDetails::
                                              GestureType::kMouse));
                         }
                       },
                       weak_factory_.GetWeakPtr(), tab));
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
  {
    base::AutoReset<bool> syncing(&syncing_tab_strip_, true);
    const std::vector<TabId> order = Order(layout_);
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
      const int from = model->GetIndexOfTab(tabs::TabHandle(order[i]).Get());
      if (from == TabStripModel::kNoTab || from == i) {
        continue;
      }
      // Pinned tabs and groups can refuse a position; leave the rest alone.
      if (model->MoveWebContentsAt(from, i, /*select_after_move=*/false) !=
          i) {
        break;
      }
    }
  }

  const std::optional<TabId> focused = FocusedTab(layout_);
  if (focused && focused != ActiveTab()) {
    const int index = model->GetIndexOfTab(tabs::TabHandle(*focused).Get());
    if (index != TabStripModel::kNoTab) {
      model->ActivateTabAt(index, TabStripUserGestureDetails(
                                      TabStripUserGestureDetails::GestureType::
                                          kKeyboard));
    }
  }
}

content::WebContents* LayoutController::ContentsFor(TabId tab) const {
  tabs::TabInterface* interface = tabs::TabHandle(tab).Get();
  if (!interface ||
      browser_->GetTabStripModel()->GetIndexOfTab(interface) ==
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
