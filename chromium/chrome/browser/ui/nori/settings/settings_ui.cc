// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/settings/settings_ui.h"

#include <memory>

#include "chrome/browser/ui/nori/settings/grit/nori_settings_resources.h"
#include "chrome/browser/ui/nori/settings/grit/nori_settings_resources_map.h"
#include "chrome/browser/ui/nori/settings/settings_handler.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

namespace nori {

SettingsUI::SettingsUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kSettingsHost);
  webui::SetupWebUIDataSource(source, kNoriSettingsResources,
                              IDR_NORI_SETTINGS_INDEX_HTML);
  web_ui->AddMessageHandler(std::make_unique<SettingsHandler>());
}

SettingsUI::~SettingsUI() = default;

}  // namespace nori
