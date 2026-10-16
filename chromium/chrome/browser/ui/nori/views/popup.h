// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/view_shadow.h"

class BrowserWindowInterface;

namespace views {
class FocusManager;
class Widget;
}  // namespace views

namespace nori {

// Owns the native command palette and the address/site-controls popup for one
// window. The latter floats Chromium's own toolbar so omnibox suggestions,
// site information and extension actions keep their native behavior.
class Popup : public ui::AcceleratorTarget, public views::ViewObserver {
 public:
  DECLARE_USER_DATA(Popup);

  Popup(BrowserWindowInterface* browser, views::View* top_container);
  Popup(const Popup&) = delete;
  Popup& operator=(const Popup&) = delete;
  ~Popup() override;

  void TogglePalette();

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
  base::WeakPtr<views::Widget> palette_;
  ui::ScopedUnownedUserData<Popup> scoped_unowned_user_data_;
  raw_ptr<views::FocusManager> focus_manager_ = nullptr;
  base::ScopedObservation<views::View, views::ViewObserver> observation_{this};
};

// Toggles the command palette.
void TogglePopup(BrowserWindowInterface* browser);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_POPUP_H_
