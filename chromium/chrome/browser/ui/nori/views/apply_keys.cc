// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/apply_keys.h"

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "chrome/browser/ui/browser_shortcuts/browser_shortcut_serialization.h"
#include "chrome/browser/ui/browser_shortcuts/browser_shortcut_service.h"
#include "chrome/browser/ui/nori/commands/commands.h"
#include "ui/base/accelerators/accelerator.h"

namespace nori {

namespace {

std::string Serialize(ui::KeyboardCode key, int modifiers) {
  return browser_shortcuts::AcceleratorToString(
      ui::Accelerator(key, modifiers));
}

std::vector<std::string> CurrentShortcuts(
    const browser_shortcuts::BrowserShortcutService& service,
    int command_id) {
  std::vector<std::string> shortcuts;
  const auto& commands = service.GetCommands();
  auto it = std::ranges::find(commands, command_id,
                              &browser_shortcuts::CommandInfo::command_id);
  if (it != commands.end()) {
    for (const browser_shortcuts::ShortcutInfo& shortcut : it->accelerators) {
      shortcuts.push_back(shortcut.serialized);
    }
  }
  return shortcuts;
}

}  // namespace

void ApplyKeys(const std::map<std::string, Shortcut, std::less<>>& keys,
               browser_shortcuts::BrowserShortcutService& service) {
  for (const Command& command : Commands()) {
    auto key = keys.find(command.name);
    if (key == keys.end()) {
      continue;
    }
    const std::string wanted = Serialize(key->second.key, key->second.modifiers);
    if (CurrentShortcuts(service, command.id) ==
        std::vector<std::string>{wanted}) {
      continue;
    }

    service.ResetAcceleratorsForCommand(command.id);
    const std::string fallback = Serialize(command.key, command.modifiers);
    if (wanted == fallback) {
      continue;
    }
    service.UnassignAcceleratorFromCommand(command.id, fallback);
    if (service.AssignAcceleratorToCommand(command.id, wanted) !=
        browser_shortcuts::AssignmentResult::kSuccess) {
      LOG(WARNING) << "nori: cannot use " << FormatShortcut(key->second)
                   << " for " << command.name;
    }
  }
}

}  // namespace nori
