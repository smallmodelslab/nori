// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_COMMAND_PALETTE_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_COMMAND_PALETTE_H_

#include "base/memory/weak_ptr.h"

class BrowserWindowInterface;
namespace views {
class Widget;
}

namespace nori {
base::WeakPtr<views::Widget> ShowCommandPalette(
    BrowserWindowInterface* browser);
}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_COMMAND_PALETTE_H_
