# Nori

A Chromium browser with a scrolling, tiled workspace. Ordinary tabs fill the
window. Split a page to the right or below, nest splits, and close any pane to
give its space back to its sibling. There is no tab strip or permanent toolbar.

Nori is built on [Helium](https://github.com/imputnet/helium), so Chrome
extensions, the Chrome Web Store, uBlock Origin and 1Password's desktop
integration work as they do there.

## Keys

| Key | Action |
| --- | --- |
| Ctrl+B / Cmd+B | Search tabs, browser commands, addresses, and the web |
| Ctrl+D | Split the focused pane to the right |
| Ctrl+Shift+D | Split the focused pane below |
| Ctrl+W / Cmd+W | Close the focused tab; its sibling fills the space |
| Ctrl+T / Cmd+T | Open a new tab that fills the window |
| Ctrl+N / Ctrl+I | Focus left / right |
| Ctrl+U / Ctrl+E | Focus above / below, then the workspace above / below |
| Ctrl+Enter | Maximize the focused pane, or restore its splits |
| Ctrl+L / Cmd+L | Address, site settings, and extensions for the focused tab |
| Ctrl+, | Nori settings |
| Escape | Dismiss the command palette |

Cmd shortcuts apply on macOS. The palette lists the actual configured keys.
Type to filter, use the arrow keys to select, and press Enter to run a command
or switch to a tab. Enter an address or search to browse. Browser commands come
from the browser's shortcut and menu models, including actions without a key.

U, N, E and I sit where W, A, S and D are for a Colemak typist. A link's context
menu opens it in a new full-window tab to the right, or splits the focused pane
above or below. Cmd+click, middle-click and `target=_blank` links follow
`new_tab.link_placement`.

## Settings

Everything is set in `~/.config/nori/config.toml` (`$XDG_CONFIG_HOME/nori` if
set), which Nori writes with every setting and its default the first time it
runs and reloads whenever it changes.

Ctrl+, opens chrome://nori-settings: the file on the right, and on the left a
control for each setting, level with the line that sets it. Move a line in the
file and its control moves with it; change a control and only that value in
the file changes, comments and all.

## Building

On macOS: Apple silicon, Xcode 26 or later, about 80 GB of free disk, and
Homebrew packages `coreutils gnu-sed quilt ccache wget python@3.13`.

On Linux: Docker and about 150 GB of free disk. `NORI_BUILD_DIR=<dir>
devutils/build-linux.sh` builds in Helium's Linux build image; later runs only
rebuild what changed. The commands below also work inside that image.

```sh
source dev.sh
nori setup   # downloads Chromium 154 and prepares it; takes a while
nori build   # the first build takes a few hours, later ones minutes
nori run
```

The Chromium tree lives outside this repository, in `$NORI_BUILD_DIR`
(default `~/.local/share/nori/build`).

## Development

Nori is three layers on top of Chromium:

1. [Helium](https://github.com/imputnet/helium), pinned as the
   `upstream/helium-macos` and `upstream/helium-linux` submodules, with their
   patches.
2. `chromium/`, nori's own code, as new files at their Chromium paths.
   `nori sync` links them into the tree and refuses to shadow an existing file.
3. `patches/`, a quilt series of small hooks that wire that code into existing
   Chromium and Helium files.

```sh
nori test                        # build and run nori_unittests
nori patch new nori/<name>.patch # start a hook
nori patch add <file>            # before editing an existing file
nori patch refresh               # after
```

## License

GPL-3.0, like Helium, which is GPL-3.0 on top of Chromium's BSD-licensed code.
Anyone distributing a Nori build must offer its source under the same terms;
this repository and the pinned Helium submodule are that source. See
[LICENSE](LICENSE) and Helium's
[`LICENSE.ungoogled_chromium`](upstream/helium-macos/LICENSE.ungoogled_chromium).
