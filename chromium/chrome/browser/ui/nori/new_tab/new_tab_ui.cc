// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.
#include "chrome/browser/ui/nori/new_tab/new_tab_ui.h"

#include <memory>
#include "base/functional/bind.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/nori/new_tab/grit/nori_new_tab_resources.h"
#include "chrome/browser/ui/nori/new_tab/grit/nori_new_tab_resources_map.h"
#include "chrome/browser/ui/nori/views/popup.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/webui/webui_util.h"

namespace nori {
namespace {
class NewTabHandler : public content::WebUIMessageHandler {
 public:
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "openCommandPalette", base::BindRepeating(&NewTabHandler::OpenPalette,
                                                  base::Unretained(this)));
  }

 private:
  void OpenPalette(const base::ListValue&) {
    if (auto* browser =
            GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
                web_ui()->GetWebContents()))
      TogglePopup(browser);
  }
};
}  // namespace

NewTabUI::NewTabUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), "nori-newtab");
  webui::SetupWebUIDataSource(source, kNoriNewTabResources,
                              IDR_NORI_NEW_TAB_INDEX_HTML);
  web_ui->AddMessageHandler(std::make_unique<NewTabHandler>());
}
NewTabUI::~NewTabUI() = default;
}  // namespace nori
