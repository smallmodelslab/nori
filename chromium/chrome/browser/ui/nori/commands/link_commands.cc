// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/commands/link_commands.h"

#include <algorithm>
#include <array>

#include "chrome/app/chrome_command_ids.h"
#include "chrome/grit/generated_resources.h"

namespace nori {

namespace {

constexpr auto kLinkCommands = std::to_array<LinkCommand>({
    {IDC_CONTENT_CONTEXT_NORI_OPENLINK_RIGHT,
     IDS_NORI_CONTENT_CONTEXT_OPENLINK_RIGHT, Placement::kRight},
    {IDC_CONTENT_CONTEXT_NORI_OPENLINK_BELOW,
     IDS_NORI_CONTENT_CONTEXT_OPENLINK_BELOW, Placement::kBelow},
    {IDC_CONTENT_CONTEXT_NORI_OPENLINK_ABOVE,
     IDS_NORI_CONTENT_CONTEXT_OPENLINK_ABOVE, Placement::kAbove},
});

}  // namespace

base::span<const LinkCommand> LinkCommands() {
  return kLinkCommands;
}

const LinkCommand* FindLinkCommand(int id) {
  auto it = std::ranges::find(kLinkCommands, id, &LinkCommand::id);
  return it == kLinkCommands.end() ? nullptr : &*it;
}

}  // namespace nori
