// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_CONFIG_SHORTCUT_H_
#define CHROME_BROWSER_UI_NORI_CONFIG_SHORTCUT_H_

#include <optional>
#include <string>
#include <string_view>

#include "ui/events/keycodes/keyboard_codes.h"

namespace nori {

// A key with modifiers, written in config.toml as e.g. "ctrl+shift+n".
//
// Modifiers are ctrl, alt (or opt), shift and cmd. Keys are letters, digits,
// f1 to f24, and the names in shortcut.cc such as enter, tab or left. Letters
// name physical keys, as Chrome's accelerators do.
struct Shortcut {
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  int modifiers = 0;

  bool operator==(const Shortcut&) const = default;
};

std::optional<Shortcut> ParseShortcut(std::string_view text);
std::string FormatShortcut(const Shortcut& shortcut);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_CONFIG_SHORTCUT_H_
