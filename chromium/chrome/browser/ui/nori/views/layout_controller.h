// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_LAYOUT_CONTROLLER_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_LAYOUT_CONTROLLER_H_

#include <map>
#include <optional>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/nori/layout/layout.h"
#include "chrome/browser/ui/nori/layout/spring.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "ui/compositor/compositor_animation_observer.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/proposed_layout.h"

class BrowserWindowInterface;

namespace content {
class WebContents;
}  // namespace content

namespace ui {
class Compositor;
}  // namespace ui

namespace views {
class View;
class WebView;
}  // namespace views

namespace nori {

struct LayoutOptions {
  int gap = 0;
  SpringParams horizontal;
  SpringParams vertical;
  // Where new tabs go, relative to the focused tab.
  Placement placement = Placement::kRight;

  bool operator==(const LayoutOptions&) const = default;
};

// Runs the scrolling layout for one browser window. It keeps a Layout in step
// with the tab strip, shows every tab that is in view, and animates focus
// changes.
//
// The focused tab stays in Chrome's own active contents container, so
// everything that follows the active tab keeps working. Other tabs in view
// are shown by plain web views ("peers") inside the same host.
class LayoutController : public TabStripModelObserver,
                         public ui::CompositorAnimationObserver {
 public:
  DECLARE_USER_DATA(LayoutController);

  // `host` is the view that holds the active contents container; peers are
  // added to it as children.
  LayoutController(BrowserWindowInterface* browser, views::View* host);
  LayoutController(const LayoutController&) = delete;
  LayoutController& operator=(const LayoutController&) = delete;
  ~LayoutController() override;

  static LayoutController* From(BrowserWindowInterface* browser);

  void Focus(Direction direction);
  void ToggleMaximize();
  // Runs `open`, putting any tab it opens at `placement` instead of
  // `LayoutOptions::placement`.
  void OpenWithPlacement(Placement placement, base::FunctionRef<void()> open);
  void SetOptions(const LayoutOptions& options);

  // Called before `contents` moves into the active container, so that no
  // peer still holds it.
  void Release(content::WebContents* contents);

  // Lays out `active_container` and the peers within a host of `size`.
  void AddChildLayouts(const gfx::Size& size,
                       views::View* active_container,
                       std::vector<views::ChildLayout>& layouts);

 private:
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // ui::CompositorAnimationObserver:
  void OnAnimationStep(base::TimeTicks timestamp) override;
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

  void Insert(const std::vector<TabId>& tabs);
  // Replaces the layout, scrolls it into view, and animates the view from
  // where it is now.
  void SetLayout(Layout layout);
  void SetGeometry(const gfx::Size& size);
  std::vector<Tile> CurrentTiles(base::TimeTicks now) const;

  // Gives every visible tab other than the active one a peer.
  void UpdatePeers();
  void OnPeerFocused(views::WebView* peer);
  void StartAnimating();
  void StopAnimating();
  void Relayout();

  // Puts the tab strip in layout order and activates the focused tab. Posted,
  // because the tab strip may not be changed while it notifies observers.
  void ScheduleSyncTabStrip();
  void SyncTabStrip();

  content::WebContents* ContentsFor(TabId tab) const;
  std::optional<TabId> ActiveTab() const;

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<views::View> host_;

  Layout layout_;
  Geometry geometry_;
  LayoutOptions options_;
  std::optional<Placement> placement_override_;
  // Tabs opened in a row go after one another rather than each next to the
  // focused tab. Reset whenever focus moves.
  std::optional<TabId> insert_anchor_;

  Motion scroll_;
  Motion stack_;
  Motion resize_;
  std::map<TabId, Rect> resize_from_;
  raw_ptr<ui::Compositor> compositor_ = nullptr;

  struct Peer {
    raw_ptr<views::WebView> view;
    base::CallbackListSubscription focused;
  };
  std::map<TabId, Peer> peers_;

  bool syncing_tab_strip_ = false;
  bool sync_scheduled_ = false;

  ui::ScopedUnownedUserData<LayoutController> scoped_unowned_user_data_;
  base::WeakPtrFactory<LayoutController> weak_factory_{this};
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_LAYOUT_CONTROLLER_H_
