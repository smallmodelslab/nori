# Nori

Nori is Helium with a different window: a niri-style scrolling layout instead of
a tab strip. This repository never contains Chromium source.

## Layers

1. `upstream/helium-macos` and `upstream/helium-linux` are submodules pinned to
   Helium releases that carry the same core repository (`helium-chromium`) as
   their own submodule. `dev.sh` uses the one for the platform it runs on.
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
nori test                  # the pure code, in seconds
nori build && nori run     # every change to the browser
nori patch new nori/<name>.patch
nori patch add <files>     # before editing an existing file
nori patch refresh
```

Edit patched files in the source tree (`nori patch add` first, `nori patch
refresh` after); edit overlay files in `chromium/`, which `nori sync` links in.
`nori patch push` / `pop` move through the series, which is applied in order:
branding, the platform's branding, layout, borderless, layout-view, commands,
popup, newtab, settings. macOS uses `patches/series` and Linux
`patches/series.linux`; a hook into a file only one platform's Helium changes
goes in that platform's patch, everything else is shared.

To move to a new Helium release, check out its tag in both upstream
submodules (they must pin the same `helium-chromium`), update their
`helium-chromium` submodules, `nori he reset` and `nori setup` on macOS (the
Linux build sets up again by itself), and refresh any nori patch that no longer
applies.

On Linux, `devutils/build-linux.sh` builds in Helium's Linux build image and
keeps the tree in `$NORI_BUILD_DIR` between builds; the Build Linux workflow
runs it on a dedicated builder.

This repository is a submodule of `wrkrsh/monorepo`. Commit and open pull
requests here, then commit the new `nori` gitlink in the monorepo.

## Code

- C++ follows Chromium style. Keep the layout model pure: value types in, value
  types out, no views or WebContents.
- The TOML file is the only source of settings; the C++ schema in
  `chromium/chrome/browser/ui/nori/config/` defines what it may contain.

Everything lives under `chromium/chrome/browser/ui/nori/`:

| Directory | Contents |
| --- | --- |
| `layout/` | The pure model: workspaces, columns, springs, popup bounds |
| `commands/` | The command table (ids, labels, default keys) and link menu items |
| `config/` | Schema, shortcuts, the toml++ document, and `ConfigStore` for the file |
| `views/` | Per-window glue: `WindowController`, `LayoutController`, the popup |
| `settings/` | chrome://nori-settings: handler, controller and plain HTML/CSS/JS |

The Ctrl+B popup is Helium's own toolbar, floated over the page by the
browser view layout, not a WebUI page: that keeps the omnibox, page info and
extension actions working unchanged. The settings page is plain files in a
hand-written grit `.grd`, so it needs none of Chromium's TypeScript toolchain.
