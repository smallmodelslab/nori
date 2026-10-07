// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_NORI_VIEWS_EXECUTE_COMMAND_H_
#define CHROME_BROWSER_UI_NORI_VIEWS_EXECUTE_COMMAND_H_

class BrowserWindowInterface;

namespace nori {

// Runs the nori command `id` in `browser`. Returns false if `id` is not a
// nori command.
bool ExecuteCommand(BrowserWindowInterface* browser, int id);

}  // namespace nori

#endif  // CHROME_BROWSER_UI_NORI_VIEWS_EXECUTE_COMMAND_H_
