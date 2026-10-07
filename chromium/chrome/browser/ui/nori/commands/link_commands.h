// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_COMMANDS_LINK_COMMANDS_H_
#define CHROME_BROWSER_UI_NORI_COMMANDS_LINK_COMMANDS_H_

#include "base/containers/span.h"
#include "chrome/browser/ui/nori/layout/layout.h"

namespace nori {

// A link context menu item that opens the link in a new tab at `placement`.
// In nori these replace Chrome's single "Open link in new tab".
struct LinkCommand {
  int id;
  int label_id;
  Placement placement;
};

base::span<const LinkCommand> LinkCommands();

// Returns null if `id` is not a nori link command.
const LinkCommand* FindLinkCommand(int id);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_COMMANDS_LINK_COMMANDS_H_
