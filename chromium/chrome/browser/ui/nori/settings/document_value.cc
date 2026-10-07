// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/settings/document_value.h"

#include <string>

#include "chrome/browser/ui/nori/config/document.h"

namespace nori {

namespace {

std::string_view TypeName(SettingType type) {
  switch (type) {
    case SettingType::kNumber:
      return "number";
    case SettingType::kInteger:
      return "integer";
    case SettingType::kChoice:
      return "choice";
    case SettingType::kShortcut:
      return "shortcut";
  }
}

base::DictValue SettingToValue(const Setting& setting) {
  base::DictValue value;
  value.Set("type", TypeName(setting.type));
  value.Set("default", setting.default_literal);
  value.Set("description", setting.description);
  switch (setting.type) {
    case SettingType::kNumber:
    case SettingType::kInteger:
      value.Set("min", setting.min);
      value.Set("max", setting.max);
      break;
    case SettingType::kChoice: {
      base::ListValue options;
      for (std::string_view option : setting.options) {
        options.Append(option);
      }
      value.Set("options", std::move(options));
      break;
    }
    case SettingType::kShortcut:
      break;
  }
  return value;
}

base::DictValue NodeToValue(const Node& node) {
  base::DictValue value;
  value.Set("path", node.path);
  value.Set("line", node.line);
  value.Set("literal", node.literal);
  if (node.error) {
    value.Set("error", *node.error);
  }
  if (node.setting) {
    value.Set("setting", SettingToValue(*node.setting));
  }
  return value;
}

}  // namespace

base::DictValue DocumentToValue(std::string_view text,
                                const Document& document) {
  base::DictValue value;
  value.Set("text", text);
  if (const std::optional<SyntaxError>& error = document.syntax_error) {
    value.Set("syntaxError", base::DictValue()
                                 .Set("line", error->line)
                                 .Set("column", error->column)
                                 .Set("message", error->message));
  }
  base::ListValue nodes;
  for (const Node& node : document.nodes) {
    nodes.Append(NodeToValue(node));
  }
  value.Set("nodes", std::move(nodes));
  return value;
}

}  // namespace nori
