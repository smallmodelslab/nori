// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_CONFIG_CONFIG_STORE_H_
#define CHROME_BROWSER_UI_NORI_CONFIG_CONFIG_STORE_H_

#include <string>

#include "base/callback_list.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/threading/sequence_bound.h"
#include "chrome/browser/ui/nori/config/document.h"

namespace nori {

// config.toml, shared by every window and profile, kept up to date as the
// file changes on disk. On first run the file is created with every default.
// Used on the UI thread only.
class ConfigStore {
 public:
  ConfigStore(const ConfigStore&) = delete;
  ConfigStore& operator=(const ConfigStore&) = delete;

  static ConfigStore& Get();

  // $XDG_CONFIG_HOME/nori/config.toml, or ~/.config/nori/config.toml.
  static base::FilePath Path();

  // The last config without a syntax error, so that a half-typed edit does
  // not reset everything to the defaults.
  const Config& config() const { return config_; }
  const std::string& text() const { return text_; }
  const Document& document() const { return document_; }

  // Runs `callback` now, then whenever `config()` changes.
  base::CallbackListSubscription SubscribeToConfig(
      base::RepeatingCallback<void(const Config&)> callback);
  // Runs `callback` whenever `text()` changes.
  base::CallbackListSubscription SubscribeToText(
      base::RepeatingClosure callback);

  // Saves `text` to the file; the change then arrives like any other edit.
  void Write(std::string text);

 private:
  friend class base::NoDestructor<ConfigStore>;
  class File;

  ConfigStore();
  ~ConfigStore();

  void OnRead(std::string text);

  std::string text_;
  Document document_;
  Config config_;
  base::RepeatingCallbackList<void(const Config&)> config_callbacks_;
  base::RepeatingClosureList text_callbacks_;
  base::SequenceBound<File> file_;
};

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_CONFIG_CONFIG_STORE_H_
