# SGCAM Smali Source Patches

Source-level smali patches that produce the modified DEX files used by the SGCAM DEX Patcher KernelSU module.

Generated via `git format-patch` from the [SGCAM smali source repo](https://github.com/Thunderoar/SGCAM_8.5.300.XX.10_STABLE_V24_SCAN3D_PACKAGE). The binary `*.patch.bsdf` files in the parent directory are derived from these via: `apktool d → git am → apktool b → bsdiff`.

## Patch inventory

| # | File | Summary |
|---|------|---------|
| 1 | `0001-docs-add-AGENTS.md-...patch` | Docs: AGENTS.md modification analysis |
| 2 | `0002-perf-replace-acquireNextImage-...patch` | Image reader: acquireLatestImage + null guard |
| 3 | `0003-chore-reduce-photosphere-...patch` | Photosphere: Medium resolution output |
| 4 | `0004-fix-move-RAW-DNG-processing-...patch` | RAW/DNG moved off UI thread (fixes ANR) |
| 5 | `0005-docs-add-comment-noting-dead-FPS-...patch` | Docs: dead FPS fix removed from TEMPLATE_PREVIEW |
| 6 | `0006-fix-add-Exynos-specific-guard-...patch` | Exynos: bypass AWB gain fill, null ColorSpaceTransform |
| 7 | `0007-perf-cache-SharedPreferences-...patch` | Cache SharedPreferences in static field |
| 8 | `0008-fix-Exynos-AWB-override-...patch` | Exynos: unity AWB gains, configurable black levels, preview lag fix v2 |

## Regenerate `*.patch.bsdf` from these smali patches

```bash
# From the repo root:
./patches/regenerate_bsdf_patches.sh /path/to/SGCAM_8.5.300.XX.10_STABLE_V24_SCAN3D_PACKAGE.apk
```

Prerequisites: `apktool` 2.11+, `bsdiff`, `zip`/`unzip`, `md5sum`, `git`.

The script will:
1. Decompile the original APK with apktool
2. Apply each smali patch in order via `git am`
3. Recompile with apktool
4. Extract DEX files from both original and patched APKs
5. Run `bsdiff` to produce `patches/*.patch.bsdf`
6. Update `patches/hashes.txt` with MD5 of the original DEX files

## Re-export patches from the smali repo after new edits

```bash
cd ~/scam/SGCAM_8.5.300.XX.10_STABLE_V24_SCAN3D_PACKAGE
REBASE_COMMIT=$(git log --pretty=format:"%H %s" | grep "Rebase: apktool" | awk "{print \$1}")
git format-patch -o ~/scam/sgcam_ksu_module/patches/smali/ ${REBASE_COMMIT}..HEAD
```

## See also

- `../README.md` — top-level repo README
- `../PATCH_NOTES.md` — release changelog
- [`AGENTS.md` in the smali repo](https://github.com/Thunderoar/SGCAM_8.5.300.XX.10_STABLE_V24_SCAN3D_PACKAGE/blob/master/AGENTS.md) — comprehensive modification analysis
