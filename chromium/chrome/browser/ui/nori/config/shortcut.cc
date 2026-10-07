// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/shortcut.h"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "ui/events/event_constants.h"

namespace nori {

namespace {

using ModifierName = std::pair<std::string_view, int>;
using KeyName = std::pair<std::string_view, ui::KeyboardCode>;

// The first kCanonicalModifiers entries are the names FormatShortcut writes,
// in the order it writes them. The rest are accepted aliases.
constexpr size_t kCanonicalModifiers = 4;
constexpr auto kModifierNames = std::to_array<ModifierName>({
    {"ctrl", ui::EF_CONTROL_DOWN},
    {"alt", ui::EF_ALT_DOWN},
    {"shift", ui::EF_SHIFT_DOWN},
    {"cmd", ui::EF_COMMAND_DOWN},
    {"control", ui::EF_CONTROL_DOWN},
    {"opt", ui::EF_ALT_DOWN},
    {"option", ui::EF_ALT_DOWN},
    {"super", ui::EF_COMMAND_DOWN},
    {"meta", ui::EF_COMMAND_DOWN},
});

constexpr auto kNamedKeys = std::to_array<KeyName>({
        {"enter", ui::VKEY_RETURN},
        {"tab", ui::VKEY_TAB},
        {"space", ui::VKEY_SPACE},
        {"escape", ui::VKEY_ESCAPE},
        {"backspace", ui::VKEY_BACK},
        {"delete", ui::VKEY_DELETE},
        {"left", ui::VKEY_LEFT},
        {"right", ui::VKEY_RIGHT},
        {"up", ui::VKEY_UP},
        {"down", ui::VKEY_DOWN},
        {"home", ui::VKEY_HOME},
        {"end", ui::VKEY_END},
        {"pageup", ui::VKEY_PRIOR},
        {"pagedown", ui::VKEY_NEXT},
        {"comma", ui::VKEY_OEM_COMMA},
        {"period", ui::VKEY_OEM_PERIOD},
        {"slash", ui::VKEY_OEM_2},
        {"semicolon", ui::VKEY_OEM_1},
        {"quote", ui::VKEY_OEM_7},
        {"minus", ui::VKEY_OEM_MINUS},
        {"equal", ui::VKEY_OEM_PLUS},
        {"bracketleft", ui::VKEY_OEM_4},
        {"bracketright", ui::VKEY_OEM_6},
        {"backslash", ui::VKEY_OEM_5},
        {"backquote", ui::VKEY_OEM_3},
    });

std::optional<int> ModifierFor(std::string_view name) {
  auto it = std::ranges::find(kModifierNames, name, &ModifierName::first);
  if (it == kModifierNames.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<ui::KeyboardCode> KeyFor(std::string_view name) {
  if (name.size() == 1 && base::IsAsciiLower(name[0])) {
    return static_cast<ui::KeyboardCode>(ui::VKEY_A + name[0] - 'a');
  }
  if (name.size() == 1 && base::IsAsciiDigit(name[0])) {
    return static_cast<ui::KeyboardCode>(ui::VKEY_0 + name[0] - '0');
  }
  int number = 0;
  if (name.size() >= 2 && name[0] == 'f' &&
      base::StringToInt(name.substr(1), &number) && number >= 1 &&
      number <= 24) {
    return static_cast<ui::KeyboardCode>(ui::VKEY_F1 + number - 1);
  }
  auto it = std::ranges::find(kNamedKeys, name, &KeyName::first);
  if (it == kNamedKeys.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::string NameOfKey(ui::KeyboardCode key) {
  if (key >= ui::VKEY_A && key <= ui::VKEY_Z) {
    return std::string(1, static_cast<char>('a' + key - ui::VKEY_A));
  }
  if (key >= ui::VKEY_0 && key <= ui::VKEY_9) {
    return std::string(1, static_cast<char>('0' + key - ui::VKEY_0));
  }
  if (key >= ui::VKEY_F1 && key <= ui::VKEY_F24) {
    return "f" + base::NumberToString(key - ui::VKEY_F1 + 1);
  }
  auto it = std::ranges::find(kNamedKeys, key, &KeyName::second);
  return it == kNamedKeys.end() ? std::string() : std::string(it->first);
}

}  // namespace

std::optional<Shortcut> ParseShortcut(std::string_view text) {
  const std::string lower = base::ToLowerASCII(text);
  std::vector<std::string_view> parts = base::SplitStringPiece(
      lower, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.empty()) {
    return std::nullopt;
  }

  Shortcut shortcut;
  for (std::string_view part : base::span(parts).first(parts.size() - 1)) {
    std::optional<int> modifier = ModifierFor(part);
    if (!modifier || (shortcut.modifiers & *modifier)) {
      return std::nullopt;
    }
    shortcut.modifiers |= *modifier;
  }
  std::optional<ui::KeyboardCode> key = KeyFor(parts.back());
  if (!key) {
    return std::nullopt;
  }
  shortcut.key = *key;
  return shortcut;
}

std::string FormatShortcut(const Shortcut& shortcut) {
  std::vector<std::string> parts;
  for (const auto& [name, flag] :
       base::span(kModifierNames).first<kCanonicalModifiers>()) {
    if (shortcut.modifiers & flag) {
      parts.emplace_back(name);
    }
  }
  parts.push_back(NameOfKey(shortcut.key));
  return base::JoinString(parts, "+");
}

}  // namespace nori
