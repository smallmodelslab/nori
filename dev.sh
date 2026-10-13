#!/usr/bin/env bash
# Nori development commands. Load with `source dev.sh`, then run `nori`.
#
# Nori is Helium plus an overlay of new files (chromium/) and a quilt series of
# small hooks (patches/). Helium's own tooling downloads and prepares Chromium;
# nori only adds its layer on top.

_nori_root="$(cd "$(dirname "${BASH_SOURCE[0]:-${(%):-%x}}")" && pwd)"
case "$(uname -s)" in
  Linux) _nori_platform=linux ;;
  *) _nori_platform=macos ;;
esac
_nori_helium="$_nori_root/upstream/helium-$_nori_platform"
_nori_build="${NORI_BUILD_DIR:-$HOME/.local/share/nori/build}"
_nori_venv="$_nori_build/venv"
_nori_src="$_nori_helium/build/src"
_nori_out="$_nori_src/out/Default"
if [ "$_nori_platform" = linux ]; then
  _nori_profile="${XDG_CONFIG_HOME:-$HOME/.config}/nori-dev"
else
  _nori_profile="$HOME/Library/Application Support/org.smallmodelslab.nori.dev"
fi

# Helium's dev.sh locates itself through $0, which only resolves when it is
# sourced by zsh from its own directory.
_nori_he() {
  (cd "$_nori_helium" && PATH="$_nori_venv/bin:$PATH" zsh -c 'source ./dev.sh && he "$@"' he "$@")
}

# Runs a function from Helium's Linux build scripts, which build into the same
# tree layout as the macOS ones.
_nori_hl() {
  (cd "$_nori_helium" && PATH="$_nori_venv/bin:$PATH" bash -c '
    source scripts/shared.sh && setup_environment && "$@"' nori-hl "$@")
}

_nori_quilt() {
  (cd "$_nori_src" &&
    QUILT_PATCHES="$_nori_root/patches" \
    QUILT_SERIES=series \
    QUILT_PATCH_OPTS="--unified --reject-format=unified" \
    QUILT_DIFF_ARGS="-p ab --no-timestamps --no-index" \
    QUILT_REFRESH_ARGS="-p ab --no-timestamps --no-index" \
    quilt --quiltrc - "$@")
}

_nori_prepare_host() {
  git -C "$_nori_root" submodule update --init --recursive
  mkdir -p "$_nori_build/helium"
  if [ ! -e "$_nori_helium/build" ]; then
    ln -s "$_nori_build/helium" "$_nori_helium/build"
  fi
  if [ ! -x "$_nori_venv/bin/python3" ]; then
    "$(command -v python3.13 || command -v python3)" -m venv "$_nori_venv"
    "$_nori_venv/bin/pip" install --quiet httplib2==0.22.0 requests pillow
  fi
}

_nori_apply_helium_patches() {
  local helium_core="$_nori_helium/helium-chromium"
  if [ -f "$_nori_src/.nori-helium-patched" ]; then
    return
  fi
  "$_nori_venv/bin/python3" "$helium_core/utils/patches.py" apply \
    "$_nori_src" "$helium_core/patches" "$_nori_helium/patches"
  touch "$_nori_src/.nori-helium-patched"
}

# Linux counterparts of Helium's macOS presetup and configure, following
# helium-linux's development setup: a component build, no domain substitution.
_nori_presetup_linux() {
  if [ -d "$_nori_src/out" ]; then
    echo "$_nori_src/out already exists" >&2
    return
  fi
  _nori_hl fetch_sources false false
  "$_nori_venv/bin/python3" "$_nori_helium/helium-chromium/utils/prune_binaries.py" \
    "$_nori_src" "$_nori_helium/helium-chromium/pruning.list"
  _nori_hl helium_resources
  _nori_hl write_gn_args
  sed -i s/is_official_build/is_component_build/ "$_nori_out/args.gn"
  _nori_hl setup_toolchain
  _nori_hl helium_version
}

nori_setup() {
  set -e
  _nori_prepare_host
  if [ "$_nori_platform" = linux ]; then _nori_presetup_linux; else _nori_he presetup; fi
  _nori_apply_helium_patches
  nori_sync
  _nori_quilt push -a
  if [ "$_nori_platform" = linux ]; then _nori_hl gn_gen; else _nori_he configure; fi
}

nori_sync() {
  "$_nori_venv/bin/python3" "$_nori_root/devutils/sync_overlay.py" \
    "$_nori_root/chromium" "$_nori_src"
}

nori_build() {
  nori_sync || return
  if [ "$_nori_platform" = linux ]; then _nori_hl build -k 0; else _nori_he build; fi
}

# Builds the given ninja targets with Helium's toolchain.
_nori_ninja() {
  if [ "$_nori_platform" = linux ]; then
    (cd "$_nori_src" && SISO_PATH="$_nori_src/third_party/siso/cipd/siso" \
      python3 third_party/depot_tools/autoninja.py -C "$_nori_out" "$@")
    return
  fi
  (cd "$_nori_helium" && PATH="$_nori_venv/bin:$PATH" zsh -c '
    source ./dev.sh &&
      cd "$_src_dir" &&
      SISO_PATH="$_siso_path" python3 "$_depot_tools_dir/autoninja.py" \
        -C "$_out_dir" "$@"' nori-ninja "$@")
}

nori_test() {
  nori_sync &&
    _nori_ninja chrome/browser/ui/nori:nori_unittests &&
    "$_nori_out/nori_unittests" "$@"
}

nori_run() {
  if [ "$_nori_platform" = linux ]; then
    "$_nori_out/nori" --user-data-dir="$_nori_profile" --enable-ui-devtools "$@"
    return
  fi
  "$_nori_out/Nori.app/Contents/MacOS/Nori" \
    --user-data-dir="$_nori_profile" \
    --enable-ui-devtools \
    --use-mock-keychain \
    --disable-features=DialMediaRouteProvider \
    "$@"
}

nori() {
  local command="${1:-}"
  [ $# -gt 0 ] && shift
  case "$command" in
    setup) (nori_setup) ;;
    sync) nori_sync ;;
    build) nori_build ;;
    test) nori_test "$@" ;;
    run) nori_run "$@" ;;
    patch) _nori_quilt "$@" ;;
    he) _nori_he "$@" ;;
    *)
      cat >&2 <<'EOF'
usage: nori <command>
  setup        download Chromium, apply Helium, the overlay and nori's patches, run gn
  sync         link chromium/ into the source tree (new files only)
  build        sync, then build a development Nori.app
  test [flags] sync, then build and run nori_unittests (gtest flags pass through)
  run [flags]  run the development build with its own profile
  patch ...    run quilt on nori's patch series (new, add, refresh, push, pop)
  he ...       run a Helium dev command (see `nori he`)
EOF
      return 2
      ;;
  esac
}
