// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/commands/commands.h"

#include <algorithm>
#include <array>

#include "chrome/app/chrome_command_ids.h"
#include "chrome/grit/generated_resources.h"
#include "ui/events/event_constants.h"

namespace nori {

namespace {

// Ctrl, not Cmd: Cmd stays with the usual browser shortcuts. U/N/E/I sit
// where W/A/S/D do on a QWERTY keyboard when typing Colemak.
constexpr auto kCommands = std::to_array<Command>({
    {IDC_NORI_FOCUS_LEFT, IDS_NORI_COMMAND_FOCUS_LEFT, "focus_left",
     "Focus the column to the left.", ui::VKEY_N, ui::EF_CONTROL_DOWN, true},
    {IDC_NORI_FOCUS_RIGHT, IDS_NORI_COMMAND_FOCUS_RIGHT, "focus_right",
     "Focus the column to the right.", ui::VKEY_I, ui::EF_CONTROL_DOWN, true},
    {IDC_NORI_FOCUS_UP, IDS_NORI_COMMAND_FOCUS_UP, "focus_up",
     "Focus the tab above, or the workspace above from the top tab.",
     ui::VKEY_U, ui::EF_CONTROL_DOWN, true},
    {IDC_NORI_FOCUS_DOWN, IDS_NORI_COMMAND_FOCUS_DOWN, "focus_down",
     "Focus the tab below, or the workspace below from the bottom tab.",
     ui::VKEY_E, ui::EF_CONTROL_DOWN, true},
    {IDC_NORI_MAXIMIZE, IDS_NORI_COMMAND_MAXIMIZE, "maximize",
     "Maximize the focused pane, or restore its splits.", ui::VKEY_RETURN,
     ui::EF_CONTROL_DOWN, false},
    {IDC_NORI_POPUP, IDS_NORI_COMMAND_POPUP, "popup",
     "Search tabs, browser commands, and the web.", ui::VKEY_B,
     ui::EF_CONTROL_DOWN, false},
    {IDC_NORI_SPLIT_RIGHT, IDS_NORI_COMMAND_SPLIT_RIGHT, "split_right",
     "Split the focused pane to the right.", ui::VKEY_D, ui::EF_CONTROL_DOWN,
     false},
    {IDC_NORI_SPLIT_DOWN, IDS_NORI_COMMAND_SPLIT_DOWN, "split_down",
     "Split the focused pane below.", ui::VKEY_D,
     ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN, false},
    {IDC_NORI_SETTINGS, IDS_NORI_COMMAND_SETTINGS, "settings",
     "Open these settings.", ui::VKEY_OEM_COMMA, ui::EF_CONTROL_DOWN, false},
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
