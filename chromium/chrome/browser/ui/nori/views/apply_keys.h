// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_APPLY_KEYS_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_APPLY_KEYS_H_

#include <functional>
#include <map>
#include <string>

#include "chrome/browser/ui/nori/config/shortcut.h"

namespace browser_shortcuts {
class BrowserShortcutService;
}  // namespace browser_shortcuts

namespace nori {

// Makes each nori command in `keys` (keyed by command name) answer to exactly
// its configured shortcut. config.toml is the source of truth, so this
// overrides changes made on Helium's shortcuts page.
void ApplyKeys(const std::map<std::string, Shortcut, std::less<>>& keys,
               browser_shortcuts::BrowserShortcutService& service);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_APPLY_KEYS_H_
