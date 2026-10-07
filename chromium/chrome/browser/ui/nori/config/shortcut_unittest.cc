// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/shortcut.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"

namespace nori {
namespace {

TEST(NoriShortcutTest, ParsesModifiersAndKeysInAnyCase) {
  EXPECT_EQ(ParseShortcut("ctrl+n"),
            (Shortcut{ui::VKEY_N, ui::EF_CONTROL_DOWN}));
  EXPECT_EQ(ParseShortcut(" Ctrl + Shift + Enter "),
            (Shortcut{ui::VKEY_RETURN, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN}));
  EXPECT_EQ(ParseShortcut("opt+cmd+f12"),
            (Shortcut{ui::VKEY_F12, ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN}));
  EXPECT_EQ(ParseShortcut("comma"), (Shortcut{ui::VKEY_OEM_COMMA, 0}));
}

TEST(NoriShortcutTest, RejectsWhatIsNotAShortcut) {
  EXPECT_FALSE(ParseShortcut(""));
  EXPECT_FALSE(ParseShortcut("ctrl+"));
  EXPECT_FALSE(ParseShortcut("ctrl"));
  EXPECT_FALSE(ParseShortcut("ctrl+ctrl+n"));
  EXPECT_FALSE(ParseShortcut("hyper+n"));
  EXPECT_FALSE(ParseShortcut("ctrl+n+e"));
  EXPECT_FALSE(ParseShortcut("f25"));
}

TEST(NoriShortcutTest, FormatsCanonicallySoParsingRoundTrips) {
  const Shortcut shortcut{ui::VKEY_LEFT, ui::EF_COMMAND_DOWN |
                                             ui::EF_SHIFT_DOWN |
                                             ui::EF_CONTROL_DOWN};

  EXPECT_EQ(FormatShortcut(shortcut), "ctrl+shift+cmd+left");
  EXPECT_EQ(ParseShortcut(FormatShortcut(shortcut)), shortcut);
}

}  // namespace
}  // namespace nori
