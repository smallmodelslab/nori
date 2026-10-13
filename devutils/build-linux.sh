#!/usr/bin/env bash
# Builds Nori for Linux in Helium's build image, keeping the Chromium tree and
# its build output in $NORI_BUILD_DIR between builds: a full setup only when
# the pinned Helium or the setup itself changes, otherwise an incremental build
# of what changed.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${NORI_BUILD_DIR:?Set NORI_BUILD_DIR to a persistent directory.}"
mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
helium="$root/upstream/helium-linux"

git -C "$root" submodule update --init --recursive upstream/helium-linux
# Helium's image creates a builder user with these ids, which cannot be root's.
ids=()
if [ "$(id -u)" -ne 0 ]; then ids=(--build-arg "UID=$(id -u)" --build-arg "GID=$(id -g)"); fi
docker buildx build --load -t chromium-builder:trixie-slim "${ids[@]}" \
  -f "$helium/docker/build.Dockerfile" "$helium/docker"
docker buildx build --load -t nori-linux-builder -f "$root/devutils/linux-build.Dockerfile" "$root/devutils"

# A cancelled run leaves its container building; never let two share the tree.
docker rm -f nori-linux-build >/dev/null 2>&1 || true
# The same paths inside and outside the container, so the overlay's links resolve.
docker run --rm -i --init --name nori-linux-build -u "$(id -u):$(id -g)" \
  -v "$root:$root" -v "$build_dir:$build_dir" -w "$root" \
  -e NORI_BUILD_DIR="$build_dir" nori-linux-builder bash -s <<'IN_CONTAINER'
set -euo pipefail
source dev.sh
setup="$(git rev-parse HEAD:upstream/helium-linux) $(declare -f nori_setup _nori_presetup_linux _nori_apply_helium_patches _nori_hl | sha256sum | cut -d' ' -f1)"
stamp="$NORI_BUILD_DIR/setup.stamp"
applied="$NORI_BUILD_DIR/series.applied"
series() { # each patch in the series with its hash
  while read -r patch; do
    [ -n "$patch" ] && echo "$patch $(sha256sum "patches/$patch" | cut -d' ' -f1)"
  done < "patches/$_nori_series"
}
# Links this checkout to the kept tree before looking for it.
_nori_prepare_host
if [ "$(cat "$stamp" 2>/dev/null)" != "$setup" ] || [ ! -f "$_nori_out/build.ninja" ]; then
  rm -rf "$NORI_BUILD_DIR/helium" "$stamp" "$applied"
  nori setup
  echo "$setup" > "$stamp"
else
  # Re-apply only from the first patch that changed, so the files unchanged
  # patches touch keep their times and nothing that includes them recompiles.
  mapfile -t old < <(cat "$applied" 2>/dev/null || true)
  mapfile -t new < <(series)
  keep=0
  while [ "$keep" -lt "${#old[@]}" ] && [ "$keep" -lt "${#new[@]}" ] && [ "${old[$keep]}" = "${new[$keep]}" ]; do
    keep=$((keep + 1))
  done
  # quilt restores what the patches above did from its own backups. Exit
  # status 2: nothing to pop or push.
  if [ "$keep" -eq 0 ]; then
    _nori_quilt pop -a -f || [ $? -eq 2 ]
  else
    _nori_quilt pop -f "${new[$((keep - 1))]%% *}" || [ $? -eq 2 ]
  fi
  # Never record more than is applied, so a failed push is retried next time.
  if [ "$keep" -gt 0 ]; then printf '%s\n' "${old[@]:0:keep}"; fi > "$applied"
  nori_sync
  _nori_quilt push -a || [ $? -eq 2 ]
fi
series > "$applied"
nori_build
IN_CONTAINER
