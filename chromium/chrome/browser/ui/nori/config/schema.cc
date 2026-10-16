// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/schema.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/ui/nori/commands/commands.h"

namespace nori {

namespace {

using Apply = void (*)(Config&, const Setting&, const SettingValue&);

constexpr std::string_view kKeysTable = "keys.";

double Number(const SettingValue& value) {
  return std::get<double>(value);
}

const std::string& String(const SettingValue& value) {
  return std::get<std::string>(value);
}

std::optional<Placement> PlacementFor(std::string_view name) {
  if (name == "above") {
    return Placement::kAbove;
  }
  if (name == "below") {
    return Placement::kBelow;
  }
  if (name == "right") {
    return Placement::kRight;
  }
  return std::nullopt;
}

Setting MakeSetting(std::string path,
                    SettingType type,
                    std::string default_literal,
                    std::string description,
                    Apply apply) {
  Setting setting;
  setting.path = std::move(path);
  setting.type = type;
  setting.default_literal = std::move(default_literal);
  setting.description = std::move(description);
  setting.apply = apply;
  return setting;
}

Setting NumberSetting(std::string path,
                      SettingType type,
                      std::string default_literal,
                      double min,
                      double max,
                      std::string description,
                      Apply apply) {
  Setting setting =
      MakeSetting(std::move(path), type, std::move(default_literal),
                  std::move(description), apply);
  setting.min = min;
  setting.max = max;
  return setting;
}

Setting ChoiceSetting(std::string path,
                      std::string default_literal,
                      std::vector<std::string_view> options,
                      std::string description,
                      Apply apply) {
  Setting setting =
      MakeSetting(std::move(path), SettingType::kChoice,
                  std::move(default_literal), std::move(description), apply);
  setting.options = std::move(options);
  return setting;
}

std::string Quoted(std::string_view text) {
  return base::StrCat({"\"", text, "\""});
}

std::vector<Setting> BuildSettings() {
  std::vector<Setting> settings;
  settings.push_back(NumberSetting(
      "layout.gaps", SettingType::kInteger, "0", 0, 200,
      "Space between columns and split panes, in pixels.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.gap = static_cast<int>(std::get<int64_t>(value));
      }));

  settings.push_back(NumberSetting(
      "animation.horizontal.stiffness", SettingType::kNumber, "800", 1, 10000,
      "Spring stiffness when moving between columns.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.horizontal.stiffness = Number(value);
      }));
  settings.push_back(NumberSetting(
      "animation.horizontal.damping_ratio", SettingType::kNumber, "1.0", 0.1,
      10, "1.0 settles without overshooting; lower values bounce.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.horizontal.damping_ratio = Number(value);
      }));
  settings.push_back(NumberSetting(
      "animation.horizontal.epsilon", SettingType::kNumber, "0.25", 0.001, 10,
      "How close to its target, in pixels, the view counts as settled.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.horizontal.epsilon = Number(value);
      }));
  settings.push_back(NumberSetting(
      "animation.vertical.stiffness", SettingType::kNumber, "800", 1, 10000,
      "Spring stiffness when moving between workspaces.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.vertical.stiffness = Number(value);
      }));
  settings.push_back(NumberSetting(
      "animation.vertical.damping_ratio", SettingType::kNumber, "1.0", 0.1, 10,
      "1.0 settles without overshooting; lower values bounce.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.vertical.damping_ratio = Number(value);
      }));
  settings.push_back(NumberSetting(
      "animation.vertical.epsilon", SettingType::kNumber, "0.25", 0.001, 10,
      "How close to its target, in pixels, the view counts as settled.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.vertical.epsilon = Number(value);
      }));

  settings.push_back(ChoiceSetting(
      "new_tab.link_placement", Quoted("right"), {"above", "below", "right"},
      "Where links opened in a new tab go, next to the tab they came from.",
      [](Config& config, const Setting&, const SettingValue& value) {
        config.link_placement = *PlacementFor(String(value));
      }));

  for (const Command& command : Commands()) {
    settings.push_back(MakeSetting(
        base::StrCat({kKeysTable, command.name}), SettingType::kShortcut,
        Quoted(FormatShortcut({command.key, command.modifiers})),
        std::string(command.description),
        [](Config& config, const Setting& setting, const SettingValue& value) {
          config.keys.insert_or_assign(setting.path.substr(kKeysTable.size()),
                                       *ParseShortcut(String(value)));
        }));
  }
  return settings;
}

}  // namespace

const std::vector<Setting>& Settings() {
  static const base::NoDestructor<std::vector<Setting>> settings(
      BuildSettings());
  return *settings;
}

const Setting* FindSetting(std::string_view path) {
  const std::vector<Setting>& settings = Settings();
  auto it = std::ranges::find(settings, path, &Setting::path);
  return it == settings.end() ? nullptr : &*it;
}

std::optional<std::string> Validate(const Setting& setting,
                                    const SettingValue& value) {
  switch (setting.type) {
    case SettingType::kNumber:
    case SettingType::kInteger: {
      const double number = setting.type == SettingType::kNumber
                                ? Number(value)
                                : static_cast<double>(std::get<int64_t>(value));
      if (number < setting.min || number > setting.max) {
        return base::StrCat({"must be between ",
                             base::NumberToString(setting.min), " and ",
                             base::NumberToString(setting.max)});
      }
      return std::nullopt;
    }
    case SettingType::kChoice:
      if (!std::ranges::contains(setting.options, String(value))) {
        return "must be one of the listed values";
      }
      return std::nullopt;
    case SettingType::kShortcut:
      if (!ParseShortcut(String(value))) {
        return "must be a shortcut such as \"ctrl+shift+n\"";
      }
      return std::nullopt;
  }
}

}  // namespace nori
