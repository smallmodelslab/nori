// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_CONFIG_DOCUMENT_H_
#define CHROME_BROWSER_UI_NORI_CONFIG_DOCUMENT_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/nori/config/schema.h"

// The only part of nori that reads TOML. Everything here is a pure function of
// the text it is given.
namespace nori {

// One key/value pair in the file.
struct Node {
  std::string path;
  // 1-based line of the key.
  int line = 0;
  // The value as written in the file.
  std::string literal;
  // Null for keys nori does not know.
  raw_ptr<const Setting> setting = nullptr;
  std::optional<std::string> error;
};

struct SyntaxError {
  int line = 0;
  int column = 0;
  std::string message;
};

struct Document {
  // Defaults, overridden by every valid value in the file.
  Config config;
  // In file order.
  std::vector<Node> nodes;
  // When set, `config` is all defaults and `nodes` is empty.
  std::optional<SyntaxError> syntax_error;
};

Document Parse(std::string_view text);

// A file with every setting at its default, each under its description.
std::string DefaultText();
Config DefaultConfig();

// Replaces the value at `path` with `literal`, a TOML value, keeping the rest
// of `text` byte for byte. Returns nothing if `path` is not in `text` or the
// result would not parse.
std::optional<std::string> ReplaceValue(std::string_view text,
                                        std::string_view path,
                                        std::string_view literal);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_CONFIG_DOCUMENT_H_
