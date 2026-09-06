#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build/release}"
APPDIR="${2:-$ROOT/package/linux/ArenaDDSOptimizer.AppDir}"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/scalable/apps" "$APPDIR/usr/share/arena-dds-optimizer/presets"
BIN="$(find "$BUILD_DIR" -type f -name ArenaDDSOptimizer -perm -111 | head -n 1)"
if [[ -z "$BIN" ]]; then echo "ArenaDDSOptimizer binary not found in $BUILD_DIR" >&2; exit 2; fi
cp "$BIN" "$APPDIR/usr/bin/ArenaDDSOptimizer"
cp "$ROOT/resources/ArenaDDSOptimizer.desktop" "$APPDIR/usr/share/applications/"
cp "$ROOT/resources/ArenaDDSOptimizer.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/"
cp -a "$ROOT/presets/." "$APPDIR/usr/share/arena-dds-optimizer/presets/"
cp "$ROOT/README.md" "$ROOT/README_RU.md" "$ROOT/CHANGELOG.md" "$APPDIR/"
[[ -f "$ROOT/LICENSE" ]] && cp "$ROOT/LICENSE" "$APPDIR/"
echo "$APPDIR"
