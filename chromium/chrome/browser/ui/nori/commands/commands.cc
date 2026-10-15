// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/commands/commands.h"

#include <algorithm>
#include <array>

#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/grit/generated_resources.h"
#include "ui/events/event_constants.h"

namespace nori {

namespace {

// Leave the platform browser shortcuts alone: Cmd on macOS, Ctrl on Linux.
// Alt+E replaces the Linux app-menu shortcut in the accelerator table.
#if BUILDFLAG(IS_MAC)
constexpr int kModifiers = ui::EF_CONTROL_DOWN;
#else
constexpr int kModifiers = ui::EF_ALT_DOWN;
#endif

// U/N/E/I sit where W/A/S/D do on a QWERTY keyboard when typing Colemak.
constexpr auto kCommands = std::to_array<Command>({
    {IDC_NORI_FOCUS_LEFT, IDS_NORI_COMMAND_FOCUS_LEFT, "focus_left",
     "Focus the column to the left.", ui::VKEY_N, kModifiers, true},
    {IDC_NORI_FOCUS_RIGHT, IDS_NORI_COMMAND_FOCUS_RIGHT, "focus_right",
     "Focus the column to the right.", ui::VKEY_I, kModifiers, true},
    {IDC_NORI_FOCUS_UP, IDS_NORI_COMMAND_FOCUS_UP, "focus_up",
     "Focus the tab above, or the workspace above from the top tab.",
     ui::VKEY_U, kModifiers, true},
    {IDC_NORI_FOCUS_DOWN, IDS_NORI_COMMAND_FOCUS_DOWN, "focus_down",
     "Focus the tab below, or the workspace below from the bottom tab.",
     ui::VKEY_E, kModifiers, true},
    {IDC_NORI_TOGGLE_FULL_WIDTH, IDS_NORI_COMMAND_TOGGLE_FULL_WIDTH,
     "toggle_full_width",
     "Switch the focused column between its width and the full window.",
     ui::VKEY_RETURN, kModifiers, false},
    {IDC_NORI_POPUP, IDS_NORI_COMMAND_POPUP, "popup",
     "Open the address, site settings and extensions for the focused tab.",
     ui::VKEY_B, kModifiers, false},
    {IDC_NORI_SETTINGS, IDS_NORI_COMMAND_SETTINGS, "settings",
     "Open these settings.", ui::VKEY_OEM_COMMA, kModifiers, false},
});

}  // namespace

base::span<const Command> Commands() {
  return kCommands;
}

const Command* FindCommand(int id) {
  auto it = std::ranges::find(kCommands, id, &Command::id);
  return it == kCommands.end() ? nullptr : &*it;
}

}  // namespace nori
