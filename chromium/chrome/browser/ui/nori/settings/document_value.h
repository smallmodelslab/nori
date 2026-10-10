// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_SETTINGS_DOCUMENT_VALUE_H_
#define CHROME_BROWSER_UI_NORI_SETTINGS_DOCUMENT_VALUE_H_

#include <string_view>

#include "base/values.h"

namespace nori {

struct Document;

// `document`, parsed from `text`, as the settings page reads it:
//
//   {
//     text: string,
//     syntaxError?: {line, column, message},
//     nodes: [{
//       path, line, literal, error?,
//       setting?: {type, default, description, min?, max?, options?},
//     }],
//   }
//
// `type` is "number", "integer", "choice" or "shortcut". `setting` is left
// out for keys nori does not know.
base::DictValue DocumentToValue(std::string_view text,
                                const Document& document);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_SETTINGS_DOCUMENT_VALUE_H_
