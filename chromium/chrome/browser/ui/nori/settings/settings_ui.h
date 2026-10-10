// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_UI_H_
#define CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_UI_H_

#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"

namespace nori {

inline constexpr char kSettingsHost[] = "nori-settings";
inline constexpr char kSettingsURL[] = "chrome://nori-settings/";

class SettingsUI;

class SettingsUIConfig : public content::DefaultWebUIConfig<SettingsUI> {
 public:
  SettingsUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kSettingsHost) {}
};

// chrome://nori-settings: config.toml on the right, a control for each
// setting on the left, level with the line that sets it.
class SettingsUI : public content::WebUIController {
 public:
  explicit SettingsUI(content::WebUI* web_ui);
  SettingsUI(const SettingsUI&) = delete;
  SettingsUI& operator=(const SettingsUI&) = delete;
  ~SettingsUI() override;
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_UI_H_
