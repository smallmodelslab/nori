// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/config/document.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"

namespace nori {
namespace {

const Node* FindNode(const Document& document, std::string_view path) {
  for (const Node& node : document.nodes) {
    if (node.path == path) {
      return &node;
    }
  }
  return nullptr;
}

TEST(NoriConfigTest, TheDefaultFileListsEverySettingWithoutErrors) {
  const Document document = Parse(DefaultText());

  ASSERT_FALSE(document.syntax_error);
  ASSERT_EQ(document.nodes.size(), Settings().size());
  for (const Node& node : document.nodes) {
    EXPECT_TRUE(node.setting) << node.path;
    EXPECT_FALSE(node.error) << node.path << ": " << *node.error;
  }
  EXPECT_EQ(document.config, DefaultConfig());
}

TEST(NoriConfigTest, DefaultsShowTwoColumnsAndTheColemakKeys) {
  const Config config = DefaultConfig();

  EXPECT_EQ(config.column_width, 0.5);
  EXPECT_EQ(config.gap, 0);
  EXPECT_EQ(config.horizontal.stiffness, 800);
  EXPECT_EQ(config.link_placement, Placement::kRight);
  EXPECT_EQ(config.keys.at("focus_left"),
            (Shortcut{ui::VKEY_N, ui::EF_CONTROL_DOWN}));
  EXPECT_EQ(config.keys.at("toggle_full_width"),
            (Shortcut{ui::VKEY_RETURN, ui::EF_CONTROL_DOWN}));
}

TEST(NoriConfigTest, ValidValuesOverrideTheDefaults) {
  const Document document = Parse(
      "[layout]\n"
      "default_column_width = 1\n"
      "[new_tab]\n"
      "link_placement = \"below\"\n"
      "[keys]\n"
      "focus_left = \"ctrl+shift+h\"\n");

  EXPECT_EQ(document.config.column_width, 1.0);
  EXPECT_EQ(document.config.link_placement, Placement::kBelow);
  EXPECT_EQ(document.config.keys.at("focus_left"),
            (Shortcut{ui::VKEY_H, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN}));
  EXPECT_EQ(document.config.keys.at("focus_right"),
            DefaultConfig().keys.at("focus_right"));
}

TEST(NoriConfigTest, InvalidValuesAreReportedAndKeepTheirDefault) {
  const Document document = Parse(
      "[layout]\n"
      "gaps = -4\n"
      "default_column_width = \"wide\"\n"
      "[new_tab]\n"
      "link_placement = \"left\"\n"
      "colour = 3\n");

  EXPECT_EQ(document.config, DefaultConfig());
  ASSERT_EQ(document.nodes.size(), 4u);
  for (const Node& node : document.nodes) {
    EXPECT_TRUE(node.error) << node.path;
  }
  EXPECT_FALSE(FindNode(document, "new_tab.colour")->setting);
}

TEST(NoriConfigTest, NodesAreInFileOrderWithTheirLines) {
  const Document document = Parse(
      "# Comment\n"
      "[layout]\n"
      "gaps = 8\n"
      "default_column_width = 0.75\n"
      "\n"
      "[animation.horizontal]\n"
      "stiffness = 1200 # snappier\n");

  ASSERT_EQ(document.nodes.size(), 3u);
  EXPECT_EQ(document.nodes[0].path, "layout.gaps");
  EXPECT_EQ(document.nodes[0].line, 3);
  EXPECT_EQ(document.nodes[1].path, "layout.default_column_width");
  EXPECT_EQ(document.nodes[1].line, 4);
  EXPECT_EQ(document.nodes[1].literal, "0.75");
  EXPECT_EQ(document.nodes[2].path, "animation.horizontal.stiffness");
  EXPECT_EQ(document.nodes[2].line, 7);
  EXPECT_EQ(document.nodes[2].literal, "1200");
}

TEST(NoriConfigTest, ASyntaxErrorGivesItsPositionAndNoNodes) {
  const Document document = Parse("[layout]\ngaps = = 2\n");

  ASSERT_TRUE(document.syntax_error);
  EXPECT_EQ(document.syntax_error->line, 2);
  EXPECT_TRUE(document.nodes.empty());
  EXPECT_EQ(document.config, DefaultConfig());
}

TEST(NoriConfigTest, ReplacingAValueKeepsEverythingElse) {
  const std::string text =
      "# Mine\n"
      "[layout]\n"
      "gaps   =  0   # tight\n"
      "default_column_width = 0.5\n"
      "[keys]\n"
      "popup = \"ctrl+b\" # é\n"
      "focus_up = \"ctrl+u\"\n";

  EXPECT_EQ(ReplaceValue(text, "layout.gaps", "12"),
            "# Mine\n"
            "[layout]\n"
            "gaps   =  12   # tight\n"
            "default_column_width = 0.5\n"
            "[keys]\n"
            "popup = \"ctrl+b\" # é\n"
            "focus_up = \"ctrl+u\"\n");
  EXPECT_EQ(ReplaceValue(text, "keys.focus_up", "\"cmd+é\""),
            "# Mine\n"
            "[layout]\n"
            "gaps   =  0   # tight\n"
            "default_column_width = 0.5\n"
            "[keys]\n"
            "popup = \"ctrl+b\" # é\n"
            "focus_up = \"cmd+é\"\n");
}

TEST(NoriConfigTest, ReplacingRejectsMissingKeysAndNonValues) {
  const std::string text = "[layout]\ngaps = 0\n";

  EXPECT_FALSE(ReplaceValue(text, "layout.default_column_width", "0.5"));
  EXPECT_FALSE(ReplaceValue(text, "layout", "1"));
  EXPECT_FALSE(ReplaceValue(text, "layout.gaps", "1\nextra = 2"));
  EXPECT_FALSE(ReplaceValue(text, "layout.gaps", "1 2"));
  EXPECT_FALSE(ReplaceValue("gaps = = 0", "gaps", "1"));
}

}  // namespace
}  // namespace nori
