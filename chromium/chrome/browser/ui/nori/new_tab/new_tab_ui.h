// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.
#ifndef CHROME_BROWSER_UI_NORI_NEW_TAB_NEW_TAB_UI_H_
#define CHROME_BROWSER_UI_NORI_NEW_TAB_NEW_TAB_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

namespace nori {
class NewTabUI : public content::WebUIController {
 public:
  explicit NewTabUI(content::WebUI* web_ui);
  ~NewTabUI() override;
};
class NewTabUIConfig : public content::DefaultWebUIConfig<NewTabUI> {
 public:
  NewTabUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, "nori-newtab") {}
};
}  // namespace nori
#endif  // CHROME_BROWSER_UI_NORI_NEW_TAB_NEW_TAB_UI_H_
