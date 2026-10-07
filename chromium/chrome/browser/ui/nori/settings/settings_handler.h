// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_HANDLER_H_
#define CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_HANDLER_H_

#include "base/callback_list.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace nori {

// Connects chrome://nori-settings to config.toml. The page edits the file
// either as a whole (the text editor) or one value at a time (the controls),
// and hears about every change, including ones made outside the browser.
// The page's text is the one being edited, so that a value set from a
// control never loses typing that has not been saved yet.
//
//   getDocument()                  -> the document, see DocumentToValue()
//   setText(text)
//   setValue(text, path, literal)  -> `text` with the value replaced and
//                                     saved, or null if it could not be
//   "document-changed"             <- the document, whenever the file changes
class SettingsHandler : public content::WebUIMessageHandler {
 public:
  SettingsHandler();
  SettingsHandler(const SettingsHandler&) = delete;
  SettingsHandler& operator=(const SettingsHandler&) = delete;
  ~SettingsHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  void HandleGetDocument(const base::ListValue& args);
  void HandleSetText(const base::ListValue& args);
  void HandleSetValue(const base::ListValue& args);
  void OnTextChanged();

  base::CallbackListSubscription text_subscription_;
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_SETTINGS_SETTINGS_HANDLER_H_
