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
| Cmd+, | Settings |

Every key is set in `~/.config/nori/config.toml`.

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

## License

GPL-3.0, like Helium. See [LICENSE](LICENSE) and Helium's
[`LICENSE.ungoogled_chromium`](upstream/helium-macos/LICENSE.ungoogled_chromium).
