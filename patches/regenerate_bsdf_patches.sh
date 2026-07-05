#!/usr/bin/env bash
# Regenerates binary bsdiff patches from smali source patches.
# Usage: ./patches/regenerate_bsdf_patches.sh /path/to/SGCAM_original.apk
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SMALI_PATCHES_DIR="$REPO_ROOT/patches/smali"
OUTPUT_DIR="$REPO_ROOT/patches"
WORKDIR="${WORKDIR:-/tmp/sgcam_rebuild}"

if [ $# -lt 1 ]; then
  echo "Usage: $0 <path-to-SGCAM-original.apk>"
  exit 1
fi
ORIGINAL_APK="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"

echo "[*] Preflight checks..."
for tool in apktool bsdiff unzip zip md5sum git; do
  command -v "$tool" >/dev/null 2>&1 || { echo "[!] Missing: $tool"; exit 1; }
done

echo "[1/7] Decompiling original APK..."
rm -rf "$WORKDIR"; mkdir -p "$WORKDIR"; cd "$WORKDIR"
apktool d -f -o sgcam_orig "$ORIGINAL_APK" 2>&1 | tail -3

echo "[2/7] Initializing patched copy..."
cp -r sgcam_orig sgcam_patched
cd sgcam_patched
git init --quiet
git add -A
git -c user.email=p@l -c user.name=p commit -m "init" --quiet

echo "[3/7] Applying smali patches..."
for p in "$SMALI_PATCHES_DIR"/*.patch; do
  if git am --3way --quiet "$p" 2>/dev/null; then
    echo "    [OK] $(basename "$p")"
  else
    echo "    [!] FAILED: $(basename "$p")"
    git am --abort 2>/dev/null || true
    exit 1
  fi
done

echo "[4/7] Recompiling with apktool..."
apktool b -f -o "$WORKDIR/sgcam_patched.apk" . 2>&1 | tail -3

echo "[5/7] Extracting DEX files..."
mkdir -p "$WORKDIR/dex_orig" "$WORKDIR/dex_patched"
(cd "$WORKDIR/dex_orig"   && unzip -qjo "$ORIGINAL_APK"               "classes.dex" "classes2.dex" "classes3.dex")
(cd "$WORKDIR/dex_patched" && unzip -qjo "$WORKDIR/sgcam_patched.apk" "classes.dex" "classes2.dex" "classes3.dex")

echo "[6/7] Generating bsdiff patches..."
mkdir -p "$OUTPUT_DIR"
for dex in classes classes2 classes3; do
  bsdiff "$WORKDIR/dex_orig/${dex}.dex" "$WORKDIR/dex_patched/${dex}.dex" "$OUTPUT_DIR/${dex}.patch.bsdf"
  echo "    [OK] ${dex}.patch.bsdf ($(stat -c%s "$OUTPUT_DIR/${dex}.patch.bsdf") bytes)"
done

echo "[7/7] Updating hashes.txt..."
(cd "$WORKDIR/dex_orig" && md5sum classes.dex classes2.dex classes3.dex) > "$OUTPUT_DIR/hashes.txt"
cat "$OUTPUT_DIR/hashes.txt"

# Save reference DEX files (gitignored)
cp "$WORKDIR/dex_orig/"*.dex    "$OUTPUT_DIR/" 2>/dev/null || true
cp "$WORKDIR/dex_patched/"*.dex "$OUTPUT_DIR/" 2>/dev/null || true

echo
echo "Done. Next: cd $REPO_ROOT && python3 build_module.py"
