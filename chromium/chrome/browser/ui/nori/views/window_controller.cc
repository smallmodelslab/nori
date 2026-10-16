// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/window_controller.h"

#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_shortcuts/browser_shortcut_service_factory.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/nori/config/config_store.h"
#include "chrome/browser/ui/nori/views/apply_keys.h"

namespace nori {

namespace {

LayoutOptions LayoutOptionsFor(const Config& config) {
  LayoutOptions options;
  options.gap = config.gap;
  options.horizontal = config.horizontal;
  options.vertical = config.vertical;
  options.placement = config.link_placement;
  return options;
}

}  // namespace

WindowController::WindowController(BrowserWindowInterface* browser,
                                   views::View* contents_host,
                                   views::View* top_container)
    : browser_(browser),
      layout_(browser, contents_host),
      popup_(browser, top_container),
      config_subscription_(ConfigStore::Get().SubscribeToConfig(
          base::BindRepeating(&WindowController::OnConfigChanged,
                              base::Unretained(this)))) {}

WindowController::~WindowController() = default;

void WindowController::OnConfigChanged(const Config& config) {
  layout_.SetOptions(LayoutOptionsFor(config));
  if (auto* shortcuts =
          browser_shortcuts::BrowserShortcutServiceFactory::GetForProfile(
              browser_->GetProfile())) {
    ApplyKeys(config.keys, *shortcuts);
  }
}

}  // namespace nori
