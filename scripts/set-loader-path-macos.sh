#!/bin/bash
# Deprecated alias — use normalize-macos-libs.sh
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
echo "note: set-loader-path-macos.sh is deprecated; running normalize-macos-libs.sh" >&2
exec bash "$here/normalize-macos-libs.sh"
