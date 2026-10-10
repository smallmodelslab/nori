// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/view_shadow.h"

class BrowserWindowInterface;

namespace views {
class FocusManager;
}  // namespace views

namespace nori {

// The Ctrl+B popup is the browser's own top chrome (omnibox, page info,
// extension actions) floating in the middle of the window. It is revealed
// exactly while it holds focus or one of its bubbles is open; BrowserView
// runs the reveal and its layout places the popup with PopupBounds().
//
// This styles `top_container` to float above the page, and closes it on
// Escape once the omnibox has nothing left to revert.
class Popup : public ui::AcceleratorTarget, public views::ViewObserver {
 public:
  Popup(BrowserWindowInterface* browser, views::View* top_container);
  Popup(const Popup&) = delete;
  Popup& operator=(const Popup&) = delete;
  ~Popup() override;

  // Fades the popup with its reveal animation.
  void SetRevealRatio(double ratio);

  // ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

  // views::ViewObserver:
  void OnViewAddedToWidget(views::View* view) override;
  void OnViewRemovedFromWidget(views::View* view) override;

 private:
  void RegisterEscape();
  void UnregisterEscape();

  const raw_ptr<BrowserWindowInterface> browser_;
  const raw_ptr<views::View> top_container_;
  views::ViewShadow shadow_;
  raw_ptr<views::FocusManager> focus_manager_ = nullptr;
  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

// Opens the popup with the URL selected, or gives focus back to the page if
// the popup has it, which closes it.
void TogglePopup(BrowserWindowInterface* browser);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_
