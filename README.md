# Nori

A Chromium browser with a niri-style scrolling layout. Pages sit in columns
that scroll left and right, columns stack tabs, and workspaces stack
vertically. There is no tab strip, toolbar or frame: the pages fill the window.
Everything is configured in one TOML file.

Nori is built on [Helium](https://github.com/imputnet/helium), so Chrome
extensions, the Chrome Web Store, uBlock Origin and 1Password's desktop
integration work as they do there.

## Keys

| Key | Action |
| --- | --- |
| Ctrl+N / Ctrl+I | Focus the column to the left / right |
| Ctrl+U / Ctrl+E | Focus the tab above / below, then the workspace above / below |
| Ctrl+Enter | Toggle the focused column between its width and the full window |
| Ctrl+B | Address, site settings and extensions for the focused tab |
| Ctrl+, | Nori settings |
| Escape | Close the Ctrl+B popup |

Ctrl rather than Cmd, so the usual browser shortcuts keep working. U, N, E and
I sit where W, A, S and D are for a Colemak typist.

A link's context menu opens it in a new tab to the right (a new column),
below or above (stacked in the same column). Cmd+click, middle-click and
`target=_blank` links follow `new_tab.link_placement`.

## Settings

Everything is set in `~/.config/nori/config.toml` (`$XDG_CONFIG_HOME/nori` if
set), which Nori writes with every setting and its default the first time it
runs and reloads whenever it changes.

Ctrl+, opens chrome://nori-settings: the file on the right, and on the left a
control for each setting, level with the line that sets it. Move a line in the
file and its control moves with it; change a control and only that value in
the file changes, comments and all.

## Building

Needs macOS on Apple silicon, Xcode 26 or later, about 80 GB of free disk, and
Homebrew packages `coreutils gnu-sed quilt ccache wget python@3.13`.

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

1. [Helium](https://github.com/imputnet/helium-macos), pinned as the
   `upstream/helium-macos` submodule, with its patches.
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
