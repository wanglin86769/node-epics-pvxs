#!/bin/bash
# Run from prebuilds/darwin-x64/ after copying pvxs.node + shared libraries.
# Requires: install_name_tool, otool
#
# Normalizes versioned .dylib names to short names, rewrites install names and
# dependency paths to @loader_path so the bundle is relocatable.
# Runtime does NOT set DYLD_LIBRARY_PATH.
#
# Typical copy-in names (any one per library is enough):
#   libpvxs.1.5.dylib | libpvxs.dylib
#   libCom.3.25.0.dylib | libCom.dylib
#   libevent_core-2.1.7.dylib | libevent_core.dylib
#   libevent_pthreads-2.1.7.dylib | libevent_pthreads.dylib
#
# Recheck:
#   otool -L pvxs.node *.dylib
#   (no absolute EPICS paths; deps should use @loader_path/...)

set -euo pipefail

if ! command -v install_name_tool >/dev/null 2>&1; then
  echo "install_name_tool is required (Xcode CLT)" >&2
  exit 1
fi

if [ ! -f pvxs.node ]; then
  echo "run this script from prebuilds/darwin-x64/ (pvxs.node not found)" >&2
  exit 1
fi

rename_to() {
  local dest="$1"
  shift
  local src
  for src in "$@"; do
    if [ -e "$src" ] || [ -L "$src" ]; then
      if [ "$src" != "$dest" ]; then
        rm -f "$dest"
        mv -f "$src" "$dest"
        echo "rename $src -> $dest"
      fi
      local other
      for other in "$@"; do
        if [ "$other" != "$dest" ] && { [ -e "$other" ] || [ -L "$other" ]; }; then
          rm -f "$other"
          echo "remove leftover $other"
        fi
      done
      return 0
    fi
  done
  if [ -f "$dest" ]; then
    return 0
  fi
  echo "missing library: need $dest (or one of: $*)" >&2
  exit 1
}

# Map any LC_LOAD_DYLIB / id path that refers to a bundled lib -> @loader_path/<short>
# Uses basename matching so absolute build-machine paths are rewritten automatically.
rewrite_deps() {
  local file="$1"
  local line path base new
  # Skip the "filename:" header line from otool -L
  while IFS= read -r line; do
    path=$(echo "$line" | awk '{print $1}')
    [ -n "$path" ] || continue
    case "$path" in
      @loader_path/*|@rpath/*|/usr/lib/*|/System/*) continue ;;
    esac
    base=$(basename "$path")
    new=""
    case "$base" in
      libpvxs*.dylib) new="@loader_path/libpvxs.dylib" ;;
      libCom*.dylib) new="@loader_path/libCom.dylib" ;;
      libevent_core*.dylib) new="@loader_path/libevent_core.dylib" ;;
      libevent_pthreads*.dylib) new="@loader_path/libevent_pthreads.dylib" ;;
      *) continue ;;
    esac
    if [ "$path" != "$new" ]; then
      install_name_tool -change "$path" "$new" "$file"
      echo "change $path -> $new  ($file)"
    fi
  done < <(otool -L "$file" | tail -n +2)
}

echo "==> rename to short names"
rename_to libpvxs.dylib \
  libpvxs.1.5.dylib libpvxs.1.dylib libpvxs.dylib
rename_to libCom.dylib \
  libCom.3.25.0.dylib libCom.3.dylib libCom.dylib
rename_to libevent_core.dylib \
  libevent_core-2.1.7.dylib libevent_core-2.1.dylib libevent_core.dylib
rename_to libevent_pthreads.dylib \
  libevent_pthreads-2.1.7.dylib libevent_pthreads-2.1.dylib libevent_pthreads.dylib

echo "==> set install id (@loader_path)"
for f in libpvxs.dylib libCom.dylib libevent_core.dylib libevent_pthreads.dylib; do
  install_name_tool -id "@loader_path/$f" "$f"
  echo "id @loader_path/$f"
done

echo "==> rewrite dependency paths"
for f in pvxs.node libpvxs.dylib libCom.dylib libevent_core.dylib libevent_pthreads.dylib; do
  rewrite_deps "$f"
done

echo "done."
echo "recheck: otool -L pvxs.node *.dylib"
echo "self-test from package root: node -e \"console.log(require('.').version())\""
