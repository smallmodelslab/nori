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
if [ "$(cat "$stamp" 2>/dev/null)" != "$pin" ] || [ ! -f "$_nori_out/build.ninja" ]; then
  rm -rf "$NORI_BUILD_DIR/helium" "$stamp"
  nori setup
  echo "$pin" > "$stamp"
else
  # Re-apply nori's current series on the kept tree; quilt restores what the
  # previous series changed from its own backups.
  _nori_quilt pop -a -f || true
  nori_sync
  _nori_quilt push -a
fi
nori_build
IN_CONTAINER
