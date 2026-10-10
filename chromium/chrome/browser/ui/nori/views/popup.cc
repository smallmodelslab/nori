// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/popup.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_edit_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/top_container_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/compositor/layer.h"
#include "ui/compositor_extra/shadow.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/focus/focus_manager.h"

namespace nori {

namespace {

constexpr int kCornerRadius = 12;
constexpr int kElevation = 16;

ui::Accelerator Escape() {
  return ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE);
}

bool Holds(const views::View& view, const views::View* focused) {
  return focused && view.Contains(focused);
}

void FocusPage(BrowserWindowInterface* browser) {
  if (content::WebContents* contents =
          browser->GetTabStripModel()->GetActiveWebContents()) {
    contents->Focus();
  }
}

// Whether Escape still has work to do in the omnibox: closing suggestions or
// reverting an edit.
bool OmniboxWantsEscape(BrowserWindowInterface* browser) {
  BrowserView* view = BrowserView::GetBrowserViewForBrowser(browser);
  LocationBar* location_bar = view ? view->GetLocationBar() : nullptr;
  OmniboxController* omnibox =
      location_bar ? location_bar->GetOmniboxController() : nullptr;
  return omnibox && (omnibox->IsPopupOpen() ||
                     omnibox->edit_model()->user_input_in_progress());
}

}  // namespace

Popup::Popup(BrowserWindowInterface* browser, views::View* top_container)
    : browser_(browser),
      top_container_(top_container),
      shadow_(top_container, kElevation) {
  // `shadow_` has made the container paint to a layer.
  ui::Layer* layer = top_container_->layer();
  layer->SetFillsBoundsOpaquely(false);
  layer->SetRoundedCornerRadius(gfx::RoundedCornersF(kCornerRadius));
  shadow_.SetRoundedCornerRadius(kCornerRadius);
  SetRevealRatio(0);

  observation_.Observe(top_container_);
  if (top_container_->GetWidget()) {
    RegisterEscape();
  }
}

Popup::~Popup() {
  UnregisterEscape();
}

void Popup::SetRevealRatio(double ratio) {
  const float opacity = static_cast<float>(ratio);
  top_container_->layer()->SetOpacity(opacity);
  if (ui::Shadow* shadow = shadow_.shadow()) {
    shadow->layer()->SetOpacity(opacity);
  }
}

// Accelerators run before the focused view sees the key, so declining here
// passes Escape on to the omnibox.
bool Popup::AcceleratorPressed(const ui::Accelerator& accelerator) {
  if (OmniboxWantsEscape(browser_)) {
    return false;
  }
  FocusPage(browser_);
  return true;
}

bool Popup::CanHandleAccelerators() const {
  return focus_manager_ &&
         Holds(*top_container_, focus_manager_->GetFocusedView());
}

void Popup::OnViewAddedToWidget(views::View* view) {
  RegisterEscape();
}

void Popup::OnViewRemovedFromWidget(views::View* view) {
  UnregisterEscape();
}

void Popup::RegisterEscape() {
  UnregisterEscape();
  focus_manager_ = top_container_->GetFocusManager();
  if (focus_manager_) {
    focus_manager_->RegisterAccelerator(
        Escape(), ui::AcceleratorManager::kNormalPriority, this);
  }
}

void Popup::UnregisterEscape() {
  if (focus_manager_) {
    focus_manager_->UnregisterAccelerators(this);
    focus_manager_ = nullptr;
  }
}

void TogglePopup(BrowserWindowInterface* browser) {
  BrowserView* view = BrowserView::GetBrowserViewForBrowser(browser);
  if (!view || !view->GetFocusManager()) {
    return;
  }
  if (Holds(*view->top_container(),
            view->GetFocusManager()->GetFocusedView())) {
    FocusPage(browser);
  } else {
    view->SetFocusToLocationBar(/*is_user_initiated=*/true);
  }
}

}  // namespace nori
