// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/settings/document_value.h"

#include <string>
#include <string_view>

#include "chrome/browser/ui/nori/config/document.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace nori {
namespace {

base::DictValue ValueOf(std::string_view text) {
  return DocumentToValue(text, Parse(text));
}

const base::DictValue* FindNode(const base::DictValue& document,
                                std::string_view path) {
  for (const base::Value& node : *document.FindList("nodes")) {
    if (*node.GetDict().FindString("path") == path) {
      return &node.GetDict();
    }
  }
  return nullptr;
}

TEST(NoriDocumentValueTest, CarriesTheTextAndEveryNodeOfTheDefaultFile) {
  const std::string text = DefaultText();
  const base::DictValue document = ValueOf(text);

  EXPECT_EQ(*document.FindString("text"), text);
  EXPECT_FALSE(document.Find("syntaxError"));
  EXPECT_EQ(document.FindList("nodes")->size(), Settings().size());
}

TEST(NoriDocumentValueTest, NumbersCarryTheirRange) {
  const base::DictValue document = ValueOf("[layout]\ngaps = 8\n");
  const base::DictValue* node = FindNode(document, "layout.gaps");

  ASSERT_TRUE(node);
  EXPECT_EQ(node->FindInt("line"), 2);
  EXPECT_EQ(*node->FindString("literal"), "8");
  const base::DictValue* setting = node->FindDict("setting");
  ASSERT_TRUE(setting);
  EXPECT_EQ(*setting->FindString("type"), "integer");
  EXPECT_EQ(setting->FindDouble("min"), 0);
  EXPECT_EQ(setting->FindDouble("max"), 200);
  EXPECT_FALSE(setting->Find("options"));
}

TEST(NoriDocumentValueTest, ChoicesCarryTheirOptions) {
  const base::DictValue document =
      ValueOf("[new_tab]\nlink_placement = \"below\"\n");
  const base::DictValue* setting =
      FindNode(document, "new_tab.link_placement")->FindDict("setting");

  EXPECT_EQ(*setting->FindString("type"), "choice");
  const base::ListValue* options = setting->FindList("options");
  ASSERT_TRUE(options);
  ASSERT_EQ(options->size(), 3u);
  EXPECT_EQ((*options)[0].GetString(), "above");
  EXPECT_FALSE(setting->Find("min"));
}

TEST(NoriDocumentValueTest, ErrorsAndUnknownKeysStayOnTheirNodes) {
  const base::DictValue document = ValueOf("[layout]\ngaps = 999\nzoom = 2\n");

  EXPECT_TRUE(FindNode(document, "layout.gaps")->FindString("error"));
  const base::DictValue* unknown = FindNode(document, "layout.zoom");
  ASSERT_TRUE(unknown);
  EXPECT_FALSE(unknown->Find("setting"));
}

TEST(NoriDocumentValueTest, SyntaxErrorsReplaceTheNodes) {
  const base::DictValue document = ValueOf("[layout\n");

  const base::DictValue* error = document.FindDict("syntaxError");
  ASSERT_TRUE(error);
  EXPECT_EQ(error->FindInt("line"), 1);
  EXPECT_TRUE(error->FindString("message"));
  EXPECT_TRUE(document.FindList("nodes")->empty());
}

}  // namespace
}  // namespace nori
