#!/bin/bash
# Run from prebuilds/linux-x64/ after copying pvxs.node + shared libraries.
# Requires: patchelf, readelf
#
# Normalizes versioned .so names to short names, rewrites NEEDED/SONAME,
# and sets RPATH/RUNPATH to $ORIGIN so the bundle is relocatable.
#
# Typical copy-in names (any one per library is enough):
#   libpvxs.so.1.5 | libpvxs.so
#   libCom.so.3.25.0 | libCom.so
#   libevent_core-2.1.so.7.0.1 | libevent_core-2.1.so.7 | libevent_core.so
#   libevent_pthreads-2.1.so.7.0.1 | libevent_pthreads-2.1.so.7 | libevent_pthreads.so
#
# Recheck:
#   readelf -d pvxs.node lib*.so | grep -E 'NEEDED|SONAME|RPATH|RUNPATH'
#   ldd pvxs.node

set -euo pipefail

if ! command -v patchelf >/dev/null 2>&1; then
  echo "patchelf is required (e.g. apt install patchelf)" >&2
  exit 1
fi

if [ ! -f pvxs.node ]; then
  echo "run this script from prebuilds/linux-x64/ (pvxs.node not found)" >&2
  exit 1
fi

# Move first existing candidate to short name (removes leftover versioned files/symlinks).
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
      # Drop other candidates so only the short name remains.
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

replace_needed_if() {
  local file="$1"
  local old="$2"
  local new="$3"
  if readelf -d "$file" 2>/dev/null | grep -F "Shared library: [$old]" >/dev/null; then
    patchelf --replace-needed "$old" "$new" "$file"
    echo "needed $old -> $new  ($file)"
  fi
}

echo "==> rename to short sonames"
rename_to libpvxs.so \
  libpvxs.so.1.5 libpvxs.so.1 libpvxs.so
rename_to libCom.so \
  libCom.so.3.25.0 libCom.so.3 libCom.so
rename_to libevent_core.so \
  libevent_core-2.1.so.7.0.1 libevent_core-2.1.so.7 libevent_core.so
rename_to libevent_pthreads.so \
  libevent_pthreads-2.1.so.7.0.1 libevent_pthreads-2.1.so.7 libevent_pthreads.so

echo "==> rewrite NEEDED"
replace_needed_if pvxs.node libpvxs.so.1.5 libpvxs.so
replace_needed_if pvxs.node libpvxs.so.1 libpvxs.so

replace_needed_if libpvxs.so libCom.so.3.25.0 libCom.so
replace_needed_if libpvxs.so libCom.so.3 libCom.so
replace_needed_if libpvxs.so libevent_core-2.1.so.7 libevent_core.so
replace_needed_if libpvxs.so libevent_pthreads-2.1.so.7 libevent_pthreads.so

replace_needed_if libevent_pthreads.so libevent_core-2.1.so.7 libevent_core.so

echo "==> set SONAME"
patchelf --set-soname libpvxs.so libpvxs.so
patchelf --set-soname libCom.so libCom.so
patchelf --set-soname libevent_core.so libevent_core.so
patchelf --set-soname libevent_pthreads.so libevent_pthreads.so
echo "soname ok"

echo "==> set RPATH \$ORIGIN"
for f in pvxs.node libpvxs.so libCom.so libevent_core.so libevent_pthreads.so; do
  patchelf --set-rpath '$ORIGIN' "$f"
  echo "rpath \$ORIGIN <- $f"
done

echo "done."
echo "recheck: readelf -d pvxs.node lib*.so | grep -E 'NEEDED|SONAME|RPATH|RUNPATH'"
echo "self-test from package root: node -e \"console.log(require('.').version())\""
