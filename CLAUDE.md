# Nori

Nori is Helium with a different window: a niri-style scrolling layout instead of
a tab strip. This repository never contains Chromium source.

## Layers

1. `upstream/helium-macos` is a submodule pinned to a Helium macOS release; it
   carries Helium's core repository as its own submodule.
2. `chromium/` holds nori's code as new files, at the paths they take in the
   Chromium tree. `nori sync` links them in and refuses any file that already
   exists upstream.
3. `patches/` is a quilt series of small hooks into existing Chromium and
   Helium files. A patch only wires nori's code in; logic lives in `chromium/`.

Nori's patches apply after Helium's, so their context is Helium's tree.

## Working

```sh
source dev.sh
nori setup                 # once
nori build && nori run     # every change
nori patch new nori/<name>.patch
nori patch add <files>     # before editing an existing file
nori patch refresh
```

To move to a new Helium release, check out its tag in `upstream/helium-macos`,
update its `helium-chromium` submodule, `nori he reset`, `nori setup`, and
refresh any nori patch that no longer applies.

This repository is a submodule of `wrkrsh/monorepo`. Commit and open pull
requests here, then commit the new `nori` gitlink in the monorepo.

## Code

- C++ follows Chromium style. Keep the layout model pure: value types in, value
  types out, no views or WebContents.
- The TOML file is the only source of settings; the C++ schema in
  `chromium/chrome/browser/ui/nori/config/` defines what it may contain.
- Settings and the Ctrl+B popup are WebUI pages under
  `chromium/chrome/browser/resources/nori/`.
