#!/usr/bin/env bash
# Builds Nori for Linux in Helium's build image, keeping the Chromium tree and
# its build output in $NORI_BUILD_DIR between builds: a full setup only when
# the pinned Helium changes, otherwise an incremental build of what changed.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${NORI_BUILD_DIR:?Set NORI_BUILD_DIR to a persistent directory.}"
helium="$root/upstream/helium-linux"

git -C "$root" submodule update --init --recursive upstream/helium-linux
docker buildx build --load -t chromium-builder:trixie-slim \
  --build-arg "UID=$(id -u)" --build-arg "GID=$(id -g)" \
  -f "$helium/docker/build.Dockerfile" "$helium/docker"
docker buildx build --load -t nori-linux-builder -f "$root/devutils/linux-build.Dockerfile" "$root/devutils"

mkdir -p "$build_dir"
# The same paths inside and outside the container, so the overlay's links resolve.
docker run --rm -i -u "$(id -u):$(id -g)" \
  -v "$root:$root" -v "$build_dir:$build_dir" -w "$root" \
  -e NORI_BUILD_DIR="$build_dir" nori-linux-builder bash -s <<'IN_CONTAINER'
set -euo pipefail
source dev.sh
pin="$(git rev-parse HEAD:upstream/helium-linux)"
stamp="$NORI_BUILD_DIR/helium-linux.pin"
applied="$NORI_BUILD_DIR/series.applied"
series() { # each patch in the series with its hash
  while read -r patch; do
    [ -n "$patch" ] && echo "$patch $(sha256sum "patches/$patch" | cut -d' ' -f1)"
  done < "patches/$_nori_series"
}
if [ "$(cat "$stamp" 2>/dev/null)" != "$pin" ] || [ ! -f "$_nori_out/build.ninja" ]; then
  rm -rf "$NORI_BUILD_DIR/helium" "$stamp" "$applied"
  nori setup
  echo "$pin" > "$stamp"
else
  # Links this checkout to the kept tree.
  _nori_prepare_host
  # Re-apply only from the first patch that changed, so the files unchanged
  # patches touch keep their times and nothing that includes them recompiles.
  mapfile -t old < <(cat "$applied" 2>/dev/null || true)
  mapfile -t new < <(series)
  keep=0
  while [ "$keep" -lt "${#old[@]}" ] && [ "$keep" -lt "${#new[@]}" ] && [ "${old[$keep]}" = "${new[$keep]}" ]; do
    keep=$((keep + 1))
  done
  if [ ! -f "$applied" ] || [ "$keep" -lt "${#old[@]}" ]; then
    # quilt restores what the patches above did from its own backups.
    if [ "$keep" -eq 0 ]; then _nori_quilt pop -a -f || true; else _nori_quilt pop -f "${new[$((keep - 1))]%% *}"; fi
  fi
  nori_sync
  # Exit status 2: the series was already fully applied.
  _nori_quilt push -a || [ $? -eq 2 ]
fi
series > "$applied"
nori_build
IN_CONTAINER
