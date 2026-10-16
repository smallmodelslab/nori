// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_CONFIG_SCHEMA_H_
#define CHROME_BROWSER_UI_NORI_CONFIG_SCHEMA_H_

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "chrome/browser/ui/nori/config/shortcut.h"
#include "chrome/browser/ui/nori/layout/layout.h"
#include "chrome/browser/ui/nori/layout/spring.h"

namespace nori {

// Everything config.toml can set, already validated.
struct Config {
  int gap = 0;
  SpringParams horizontal;
  SpringParams vertical;
  Placement link_placement = Placement::kRight;
  // Keyed by command name; see nori::Command::name.
  std::map<std::string, Shortcut, std::less<>> keys;

  bool operator==(const Config&) const = default;
};

enum class SettingType {
  kNumber,
  kInteger,
  // A string from `Setting::options`.
  kChoice,
  kShortcut,
};

using SettingValue = std::variant<double, int64_t, std::string>;

struct Setting {
  // Dotted TOML path, e.g. "layout.gaps".
  std::string path;
  SettingType type;
  // The default, written as a TOML value.
  std::string default_literal;
  // Inclusive range for numbers and integers.
  double min = 0;
  double max = 0;
  std::vector<std::string_view> options;
  std::string description;
  // Stores a value that has already been checked against the fields above.
  void (*apply)(Config& config,
                const Setting& setting,
                const SettingValue& value) = nullptr;
};

// In the order the default config file lists them.
const std::vector<Setting>& Settings();
const Setting* FindSetting(std::string_view path);

// Returns an error message, or nothing if `value` is valid for `setting`.
std::optional<std::string> Validate(const Setting& setting,
                                    const SettingValue& value);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_CONFIG_SCHEMA_H_
