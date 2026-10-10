// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_COMMANDS_COMMANDS_H_
#define CHROME_BROWSER_UI_NORI_COMMANDS_COMMANDS_H_

#include <string_view>

#include "base/containers/span.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace nori {

// A browser command that nori adds, with its default shortcut. Every nori
// shortcut is reserved: it reaches the browser before the page sees it.
struct Command {
  int id;
  // Shown on Helium's shortcuts settings page.
  int label_id;
  // The key that rebinds it in the `[keys]` table of config.toml.
  std::string_view name;
  // Describes the key in config.toml and on chrome://nori-settings, which,
  // like the file, are in English only.
  std::string_view description;
  ui::KeyboardCode key;
  int modifiers;
  // Whether holding the shortcut repeats the command.
  bool repeatable;
};

base::span<const Command> Commands();

// Returns null if `id` is not a nori command.
const Command* FindCommand(int id);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_COMMANDS_COMMANDS_H_
