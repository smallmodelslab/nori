// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/config_store.h"

#include <optional>
#include <utility>

#include "base/environment.h"
#include "base/files/file_path_watcher.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "base/task/thread_pool.h"

namespace nori {

// Reads, writes and watches the file on a sequence that may block.
class ConfigStore::File {
 public:
  explicit File(base::RepeatingCallback<void(std::string)> on_read)
      : path_(ConfigStore::Path()), on_read_(std::move(on_read)) {
    if (!base::PathExists(path_) &&
        !(base::CreateDirectory(path_.DirName()) &&
          base::WriteFile(path_, DefaultText()))) {
      LOG(ERROR) << "Could not create " << path_;
    }
    if (!watcher_.Watch(path_, base::FilePathWatcher::Type::kNonRecursive,
                        base::BindRepeating(&File::OnChanged,
                                            base::Unretained(this)))) {
      LOG(ERROR) << "Could not watch " << path_;
    }
    Read();
  }

  void Write(const std::string& text) {
    if (!base::ImportantFileWriter::WriteFileAtomically(path_, text)) {
      LOG(ERROR) << "Could not write " << path_;
    }
    // Don't wait for the watcher, which may report the change late or not at
    // all after an atomic replace.
    Read();
  }

 private:
  void OnChanged(const base::FilePath&, bool error) {
    if (!error) {
      Read();
    }
  }

  void Read() {
    std::string text;
    if (base::ReadFileToString(path_, &text)) {
      on_read_.Run(std::move(text));
    }
  }

  const base::FilePath path_;
  const base::RepeatingCallback<void(std::string)> on_read_;
  base::FilePathWatcher watcher_;
};

ConfigStore::ConfigStore()
    : config_(DefaultConfig()),
      file_(base::ThreadPool::CreateSequencedTaskRunner(
                {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
                 base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
            base::BindPostTaskToCurrentDefault(base::BindRepeating(
                &ConfigStore::OnRead, base::Unretained(this)))) {}

ConfigStore::~ConfigStore() = default;

// static
ConfigStore& ConfigStore::Get() {
  static base::NoDestructor<ConfigStore> store;
  return *store;
}

// static
base::FilePath ConfigStore::Path() {
  std::optional<std::string> config_home =
      base::Environment::Create()->GetVar("XDG_CONFIG_HOME");
  const base::FilePath root = config_home && !config_home->empty()
                                  ? base::FilePath(*config_home)
                                  : base::GetHomeDir().Append(".config");
  return root.Append("nori").Append("config.toml");
}

base::CallbackListSubscription ConfigStore::SubscribeToConfig(
    base::RepeatingCallback<void(const Config&)> callback) {
  callback.Run(config_);
  return config_callbacks_.Add(std::move(callback));
}

base::CallbackListSubscription ConfigStore::SubscribeToText(
    base::RepeatingClosure callback) {
  return text_callbacks_.Add(std::move(callback));
}

void ConfigStore::Write(std::string text) {
  file_.AsyncCall(&File::Write).WithArgs(std::move(text));
}

void ConfigStore::OnRead(std::string text) {
  if (text == text_) {
    return;
  }
  text_ = std::move(text);
  document_ = Parse(text_);
  text_callbacks_.Notify();
  if (!document_.syntax_error && document_.config != config_) {
    config_ = document_.config;
    config_callbacks_.Notify(config_);
  }
}

}  // namespace nori
