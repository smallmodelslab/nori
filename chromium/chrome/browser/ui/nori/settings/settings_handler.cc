// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/settings/settings_handler.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "chrome/browser/ui/nori/config/config_store.h"
#include "chrome/browser/ui/nori/settings/document_value.h"
#include "content/public/browser/web_ui.h"

namespace nori {

namespace {

base::DictValue CurrentDocument() {
  const ConfigStore& store = ConfigStore::Get();
  base::DictValue document = DocumentToValue(store.text(), store.document());
  document.Set("file", ConfigStore::Path().value());
  return document;
}

}  // namespace

SettingsHandler::SettingsHandler() = default;
SettingsHandler::~SettingsHandler() = default;

void SettingsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getDocument", base::BindRepeating(&SettingsHandler::HandleGetDocument,
                                         base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setText", base::BindRepeating(&SettingsHandler::HandleSetText,
                                     base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setValue", base::BindRepeating(&SettingsHandler::HandleSetValue,
                                      base::Unretained(this)));
}

void SettingsHandler::OnJavascriptAllowed() {
  text_subscription_ = ConfigStore::Get().SubscribeToText(base::BindRepeating(
      &SettingsHandler::OnTextChanged, base::Unretained(this)));
}

void SettingsHandler::OnJavascriptDisallowed() {
  text_subscription_ = {};
}

void SettingsHandler::HandleGetDocument(const base::ListValue& args) {
  AllowJavascript();
  ResolveJavascriptCallback(args[0], CurrentDocument());
}

void SettingsHandler::HandleSetText(const base::ListValue& args) {
  if (const std::string* text = args[0].GetIfString()) {
    ConfigStore::Get().Write(*text);
  }
}

void SettingsHandler::HandleSetValue(const base::ListValue& args) {
  AllowJavascript();
  const std::string* text = args[1].GetIfString();
  const std::string* path = args[2].GetIfString();
  const std::string* literal = args[3].GetIfString();
  std::optional<std::string> edited;
  if (text && path && literal) {
    edited = ReplaceValue(*text, *path, *literal);
  }
  if (!edited) {
    ResolveJavascriptCallback(args[0], base::Value());
    return;
  }
  ConfigStore::Get().Write(*edited);
  ResolveJavascriptCallback(args[0], base::Value(std::move(*edited)));
}

void SettingsHandler::OnTextChanged() {
  FireWebUIListener("document-changed", CurrentDocument());
}

}  // namespace nori
