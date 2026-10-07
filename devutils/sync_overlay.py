#!/usr/bin/env python3
"""Links nori's overlay of new files into the Chromium source tree.

The overlay may only add files. Changes to existing Chromium or Helium files
belong in patches/, so a file that already exists upstream is an error. The
manifest records what nori linked, so files removed from the overlay are also
removed from the tree.
"""

from pathlib import Path
import sys

MANIFEST = ".nori-overlay"


def overlay_files(overlay: Path) -> set[Path]:
    return {
        path.relative_to(overlay)
        for path in overlay.rglob("*")
        if path.is_file() and path.name != ".DS_Store"
    }


def sync(overlay: Path, tree: Path) -> None:
    manifest = tree / MANIFEST
    linked = {Path(line) for line in manifest.read_text().splitlines()} if manifest.exists() else set()
    wanted = overlay_files(overlay)

    for stale in linked - wanted:
        (tree / stale).unlink(missing_ok=True)

    for relative in sorted(wanted):
        target = tree / relative
        source = overlay / relative
        if target.is_symlink() and target.resolve() == source.resolve():
            continue
        if target.exists() or target.is_symlink():
            if relative not in linked:
                sys.exit(f"overlay file already exists upstream, use a patch instead: {relative}")
            target.unlink()
        target.parent.mkdir(parents=True, exist_ok=True)
        target.symlink_to(source.resolve())

    manifest.write_text("".join(f"{path}\n" for path in sorted(wanted)))
    print(f"nori overlay: {len(wanted)} files linked")


if __name__ == "__main__":
    sync(Path(sys.argv[1]), Path(sys.argv[2]))
