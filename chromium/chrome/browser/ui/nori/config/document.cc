// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/document.h"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <utility>
#include <variant>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "third_party/tomlplusplus/toml.hpp"

namespace nori {

namespace {

constexpr std::string_view kHeader =
    "# Nori's configuration. Changes apply as soon as the file is saved.\n"
    "# Every setting is listed at its default; delete one to go back to it.\n"
    "# Keys are written like \"ctrl+shift+n\", with ctrl, alt, shift and cmd.\n";

bool IsContinuationByte(char c) {
  return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

// The byte offset of `position`, whose column counts code points.
std::optional<size_t> OffsetOf(std::string_view text,
                               const toml::source_position& position) {
  size_t offset = 0;
  for (uint32_t line = 1; line < position.line; ++line) {
    const size_t newline = text.find('\n', offset);
    if (newline == std::string_view::npos) {
      return std::nullopt;
    }
    offset = newline + 1;
  }
  for (uint32_t column = 1; column < position.column; ++column) {
    if (offset >= text.size()) {
      return std::nullopt;
    }
    do {
      ++offset;
    } while (offset < text.size() && IsContinuationByte(text[offset]));
  }
  return offset;
}

struct ByteRange {
  size_t begin;
  size_t end;
};

std::optional<ByteRange> RangeOf(std::string_view text,
                                 const toml::source_region& region) {
  std::optional<size_t> begin = OffsetOf(text, region.begin);
  std::optional<size_t> end = OffsetOf(text, region.end);
  if (!begin || !end || *end < *begin) {
    return std::nullopt;
  }
  return ByteRange{*begin, *end};
}

std::string_view TableOf(std::string_view path) {
  const size_t dot = path.rfind('.');
  return dot == std::string_view::npos ? std::string_view()
                                       : path.substr(0, dot);
}

std::string_view KeyOf(std::string_view path) {
  const size_t dot = path.rfind('.');
  return dot == std::string_view::npos ? path : path.substr(dot + 1);
}

// Reads `node` as the type `setting` wants, or returns an error message.
std::variant<SettingValue, std::string> ValueOf(const Setting& setting,
                                                const toml::node& node) {
  switch (setting.type) {
    case SettingType::kNumber:
      if (node.is_number()) {
        return SettingValue(*node.value<double>());
      }
      return std::string("must be a number");
    case SettingType::kInteger:
      if (node.is_integer()) {
        return SettingValue(node.as_integer()->get());
      }
      return std::string("must be a whole number");
    case SettingType::kChoice:
    case SettingType::kShortcut:
      if (node.is_string()) {
        return SettingValue(std::string(node.as_string()->get()));
      }
      return std::string("must be a string in quotes");
  }
}

struct Leaf {
  std::string path;
  toml::source_region key;
  raw_ptr<const toml::node> value;
};

void CollectLeaves(const toml::table& table,
                   std::string_view prefix,
                   std::vector<Leaf>& leaves) {
  for (const auto& [key, node] : table) {
    std::string path = prefix.empty()
                           ? std::string(key.str())
                           : base::StrCat({prefix, ".", key.str()});
    if (const toml::table* child = node.as_table()) {
      CollectLeaves(*child, path, leaves);
    } else {
      leaves.push_back({std::move(path), key.source(), &node});
    }
  }
}

Node ReadLeaf(std::string_view text, const Leaf& leaf, Config& config) {
  Node node;
  node.path = leaf.path;
  node.line = static_cast<int>(leaf.key.begin.line);
  if (std::optional<ByteRange> range = RangeOf(text, leaf.value->source())) {
    node.literal =
        std::string(text.substr(range->begin, range->end - range->begin));
  }
  node.setting = FindSetting(leaf.path);
  if (!node.setting) {
    node.error = "is not a nori setting";
    return node;
  }

  auto value = ValueOf(*node.setting, *leaf.value);
  if (const std::string* error = std::get_if<std::string>(&value)) {
    node.error = *error;
    return node;
  }
  const SettingValue& setting_value = std::get<SettingValue>(value);
  node.error = Validate(*node.setting, setting_value);
  if (!node.error) {
    node.setting->apply(config, *node.setting, setting_value);
  }
  return node;
}

Document ParseOnto(Config base, std::string_view text) {
  Document document;
  document.config = std::move(base);

  toml::parse_result result = toml::parse(text);
  if (!result) {
    const toml::parse_error& error = result.error();
    document.syntax_error = SyntaxError{
        static_cast<int>(error.source().begin.line),
        static_cast<int>(error.source().begin.column),
        std::string(error.description())};
    return document;
  }

  std::vector<Leaf> leaves;
  CollectLeaves(result.table(), "", leaves);
  std::ranges::sort(leaves, {}, [](const Leaf& leaf) {
    return std::tuple(leaf.key.begin.line, leaf.key.begin.column);
  });
  for (const Leaf& leaf : leaves) {
    document.nodes.push_back(ReadLeaf(text, leaf, document.config));
  }
  return document;
}

std::string Comment(const Setting& setting) {
  std::string comment = base::StrCat({"# ", setting.description});
  if (!setting.options.empty()) {
    std::vector<std::string> quoted;
    for (std::string_view option : setting.options) {
      quoted.push_back(base::StrCat({"\"", option, "\""}));
    }
    base::StrAppend(&comment,
                    {" One of ", base::JoinString(quoted, ", "), "."});
  }
  return comment;
}

bool IsSingleValue(std::string_view literal) {
  if (literal.find_first_of("\r\n") != std::string_view::npos) {
    return false;
  }
  toml::parse_result result = toml::parse(base::StrCat({"value = ", literal}));
  return result && result.table().size() == 1 &&
         !result.table().begin()->second.is_table();
}

}  // namespace

Document Parse(std::string_view text) {
  return ParseOnto(DefaultConfig(), text);
}

std::string DefaultText() {
  std::string text(kHeader);
  std::string_view table;
  for (const Setting& setting : Settings()) {
    if (std::string_view setting_table = TableOf(setting.path);
        setting_table != table) {
      table = setting_table;
      base::StrAppend(&text, {"\n[", table, "]\n"});
    }
    base::StrAppend(&text, {Comment(setting), "\n", KeyOf(setting.path),
                            " = ", setting.default_literal, "\n"});
  }
  return text;
}

Config DefaultConfig() {
  static const base::NoDestructor<Config> config(
      ParseOnto(Config(), DefaultText()).config);
  return *config;
}

std::optional<std::string> ReplaceValue(std::string_view text,
                                        std::string_view path,
                                        std::string_view literal) {
  if (!IsSingleValue(literal)) {
    return std::nullopt;
  }
  toml::parse_result result = toml::parse(text);
  if (!result) {
    return std::nullopt;
  }
  const toml::node* node = result.table().at_path(path).node();
  if (!node || node->is_table()) {
    return std::nullopt;
  }
  std::optional<ByteRange> range = RangeOf(text, node->source());
  if (!range) {
    return std::nullopt;
  }

  std::string replaced = base::StrCat(
      {text.substr(0, range->begin), literal, text.substr(range->end)});
  if (!toml::parse(replaced)) {
    return std::nullopt;
  }
  return replaced;
}

}  // namespace nori
