// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_WINDOW_CONTROLLER_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/nori/views/layout_controller.h"
#include "chrome/browser/ui/nori/views/popup.h"

class BrowserWindowInterface;

namespace views {
class View;
}  // namespace views

namespace nori {

struct Config;

// Everything nori adds to one browser window. Owned by the BrowserView.
class WindowController {
 public:
  // `contents_host` is the view that holds the window's page contents, and
  // `top_container` the browser chrome that nori shows as the Ctrl+B popup.
  WindowController(BrowserWindowInterface* browser,
                   views::View* contents_host,
                   views::View* top_container);
  WindowController(const WindowController&) = delete;
  WindowController& operator=(const WindowController&) = delete;
  ~WindowController();

  LayoutController& layout() { return layout_; }
  Popup& popup() { return popup_; }

 private:
  void OnConfigChanged(const Config& config);

  const raw_ptr<BrowserWindowInterface> browser_;
  LayoutController layout_;
  Popup popup_;
  base::CallbackListSubscription config_subscription_;
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_WINDOW_CONTROLLER_H_
