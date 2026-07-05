# Patch Notes

## v2 — bspatch fix + Toybox compatibility

**Date**: 2026-07-05

Critical bugfix release. v1 failed to install on Android 16 (SDK 36) due to two independent bugs: a segfault in the bundled `bspatch` (exit 139 before any patch was applied) and a false-failure in the APK integrity check (grepped for an InfoZIP footer that Toybox unzip doesn't emit). Both are fixed; install now completes end-to-end on affected devices.

### Breaking changes

None. Existing v1 installs can be upgraded in-place by flashing v2 over v1.

### Bug fixes

#### `tools/bspatch.c` — replaced with canonical bsdiff-4.3

The custom `bspatch.c` shipped in v1 had three bugs that combined to segfault before any patch could be read:

1. **`read_int64()` decoded big-endian**, but bsdiff writes lengths in host byte order (little-endian on x86_64 where patches were generated). For `classes.patch.bsdf` this yielded `len_control ≈ 1.9×10¹⁸`, causing `malloc()` to return NULL and the next `fread()` to SIGSEGV (exit 139).
2. **Integers were decoded as two's complement**, but bsdiff uses **signed-magnitude** encoding (bit 7 of byte 7 = sign, remaining 63 bits = magnitude). Mis-decoded `seek` values silently corrupted `old_pos` even when no crash occurred.
3. **The extra block was indexed by `new_pos - diff_len`** instead of being consumed as a separate sequential stream, producing corrupt output for any patch with multiple control tuples.

Replaced with the unmodified Colin Percival bsdiff-4.3 source, which uses the correct signed-magnitude `offtin()` and streaming `BZ2_bzReadOpen()` / `BZ2_bzRead()` for all three blocks.

Verified: applying `classes.patch.bsdf` to `original_classes.dex` now produces byte-for-byte identical output to the reference `patched_classes.dex` (md5 `5b77dc9d1804c1fce3f8137d7d8ba622`).

#### `ModuleBase/customize.sh` — Toybox unzip + MMT ordering

- **APK integrity check now uses `unzip -t`'s exit code directly.** The previous check greped the last line of output for the literal string `"No errors"`, an InfoZIP convention. Toybox unzip (Android 16 / SDK 36) doesn't emit that footer — it relies on the exit code — so the grep produced false failures and aborted a perfectly valid install. Output is now captured to a variable and only printed on actual failure (last 10 lines, for debugging).
- **Moved `MINAPI=33` and `set_permissions()` definitions above the `. $TMPDIR/functions.sh` source line.** MMT Extended calls `set_permissions()` inline at line 317 of `functions.sh`, so defining it after the source meant the call failed with `set_permissions: not found` (non-fatal — MMT's default perms already ran and `customize.sh` manually `chmod 0755`'d the binaries — but noisy).

### Improvements

#### `build_module.py`

- **Force-rebuild bspatch on every run** so source changes actually take effect (v1 would silently reuse a stale buggy binary).
- Use the resolved `$cc` variable in the compile command instead of the hardcoded string `aarch64-linux-android35-clang`, which broke when the compiler was found via a different candidate.
- Add `aarch64-linux-android-clang` (no API suffix) as a fallback candidate, and prefer `android33` over `android35` for broader NDK compatibility.
- Add `-O2` for a smaller, faster binary.
- Print the resolved compiler path and full compile command for easier debugging.
- Exit non-zero if `libbz2.a` / `bzlib.h` are missing (previously just warned and proceeded to fail in the compile step).
- Drop unused `ZIP_EXPECTED_SHA` constant.

### Compatibility

- **Android**: 13+ (API 33+), unchanged from v1
- **Architecture**: arm64-v8a only (aarch64 `bspatch` binary shipped)
- **SGCAM**: `com.samsung.android.scan3d` — must match the MD5 hashes in `patches/hashes.txt`:
  ```
  c4ec6933f53aae7662d7b632ea0703d7  classes.dex
  b6c5761b7d992c5a10cf5291b6e8b5ad  classes2.dex
  224bf0e08bb184d5699c6d7717bf3b7a  classes3.dex
  ```
- **Root**: KernelSU (v0.6.6+), KernelSU Next, APatch, or Magisk v20.4+

### Known issues

- The `boot-completed.sh` uses `nsenter -t 1 -m --` to enter PID 1's mount namespace. On some KernelSU Next builds, `nsenter` may not be available — the script falls back to a plain `mount -o bind` which may fail. If your bind mount doesn't survive reboot, check `logcat -s SGCAM_Patcher` for "Bind mount FAILED".
- The patched APK's v2/v3 signature block is destroyed by `zip -f -0` freshen. This is fine for the bind-mount approach (Android verified the signature at install time and doesn't re-verify at runtime for already-installed apps), but means you can't install the patched APK as a standalone package.

---

## v1 — initial release

**Date**: 2026-07-05 (earlier)

Initial public release of the SGCAM DEX Patcher KernelSU module.

### Features

- Bind-mount-based APK patching (no permanent modification)
- BSDIFF40 patches for `classes.dex`, `classes2.dex`, `classes3.dex`
- MD5 verification of installed SGCAM DEX files
- Boot-time bind mount restoration with `boot-completed.sh`
- Self-contained `bspatch` (statically linked) and `zip` binaries
- MMT Extended framework integration

### Known issues (fixed in v2)

- Custom `bspatch.c` segfaults on Android 16 (SDK 36) due to endianness and signed-encoding bugs.
- APK integrity check false-fails on Toybox unzip due to missing InfoZIP footer.
- `set_permissions` not found in install log (non-fatal) due to MMT source ordering.
