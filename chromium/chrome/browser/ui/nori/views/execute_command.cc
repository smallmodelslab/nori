// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/execute_command.h"

#include <optional>

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/nori/commands/commands.h"
#include "chrome/browser/ui/nori/settings/settings_ui.h"
#include "chrome/browser/ui/nori/views/layout_controller.h"
#include "chrome/browser/ui/nori/views/popup.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

namespace nori {

namespace {

std::optional<Direction> FocusDirection(int id) {
  switch (id) {
    case IDC_NORI_FOCUS_LEFT:
      return Direction::kLeft;
    case IDC_NORI_FOCUS_RIGHT:
      return Direction::kRight;
    case IDC_NORI_FOCUS_UP:
      return Direction::kUp;
    case IDC_NORI_FOCUS_DOWN:
      return Direction::kDown;
    default:
      return std::nullopt;
  }
}

}  // namespace

bool ExecuteCommand(BrowserWindowInterface* browser, int id) {
  if (!FindCommand(id)) {
    return false;
  }
  if (id == IDC_NORI_POPUP) {
    TogglePopup(browser);
    return true;
  }
  if (id == IDC_NORI_SETTINGS) {
    ShowSingletonTab(browser, GURL(kSettingsURL));
    return true;
  }
  LayoutController* layout = LayoutController::From(browser);
  if (!layout) {
    return true;
  }
  if (std::optional<Direction> direction = FocusDirection(id)) {
    layout->Focus(*direction);
  } else if (id == IDC_NORI_TOGGLE_FULL_WIDTH) {
    layout->ToggleFullWidth();
  }
  return true;
}

}  // namespace nori
