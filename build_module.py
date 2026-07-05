#!/usr/bin/env python3
"""
Build script for SGCAM DEX Patcher KernelSU module.

Produces a KernelSU/Magisk module that patches the installed SGCAM APK
at install time by applying bsdiff patches to the DEX files.

Usage:
  python3 build_module.py
"""

import os
import shutil
import subprocess
import sys
import zipfile
import re

ROOT = os.path.dirname(os.path.abspath(__file__))
MODULE_BASE = os.path.join(ROOT, 'ModuleBase')
TOOLS_DIR = os.path.join(ROOT, 'tools')
PATCHES_DIR = os.path.join(ROOT, 'patches')
OUTPUT_DIR = os.path.join(ROOT, 'output')

def main():
    print("[*] Building SGCAM DEX Patcher module...")
    
    # Verify all required files exist
    required = [
        (TOOLS_DIR, 'bspatch'),
        (TOOLS_DIR, 'zip'),
        (PATCHES_DIR, 'hashes.txt'),
        (PATCHES_DIR, 'classes.patch.bsdf'),
        (PATCHES_DIR, 'classes2.patch.bsdf'),
        (PATCHES_DIR, 'classes3.patch.bsdf'),
    ]
    
    for dirpath, filename in required:
        path = os.path.join(dirpath, filename)
        if not os.path.exists(path):
            print(f"[!] ERROR: Required file not found: {path}")
            sys.exit(1)
        size = os.path.getsize(path)
        print(f"    [✓] {filename} ({size:,} bytes)")
    
    # Create temp build directory
    tmp_dir = MODULE_BASE + 'Temp'
    if os.path.isdir(tmp_dir):
        shutil.rmtree(tmp_dir)
    shutil.copytree(MODULE_BASE, tmp_dir)
    
    # Create required directories in build
    os.makedirs(os.path.join(tmp_dir, 'system/bin'), exist_ok=True)
    os.makedirs(os.path.join(tmp_dir, 'system/etc/sgcam-patches'), exist_ok=True)
    
    # Copy binaries
    shutil.copy(os.path.join(TOOLS_DIR, 'bspatch'), os.path.join(tmp_dir, 'system/bin/bspatch'))
    shutil.copy(os.path.join(TOOLS_DIR, 'zip'), os.path.join(tmp_dir, 'system/bin/zip'))
    
    # Copy patches
    shutil.copy(os.path.join(PATCHES_DIR, 'hashes.txt'), os.path.join(tmp_dir, 'system/etc/sgcam-patches/hashes.txt'))
    shutil.copy(os.path.join(PATCHES_DIR, 'classes.patch.bsdf'), os.path.join(tmp_dir, 'system/etc/sgcam-patches/classes.patch.bsdf'))
    shutil.copy(os.path.join(PATCHES_DIR, 'classes2.patch.bsdf'), os.path.join(tmp_dir, 'system/etc/sgcam-patches/classes2.patch.bsdf'))
    shutil.copy(os.path.join(PATCHES_DIR, 'classes3.patch.bsdf'), os.path.join(tmp_dir, 'system/etc/sgcam-patches/classes3.patch.bsdf'))
    
    # Set permissions
    os.chmod(os.path.join(tmp_dir, 'system/bin/bspatch'), 0o755)
    os.chmod(os.path.join(tmp_dir, 'system/bin/zip'), 0o755)
    
    # Create module zip
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    zip_path = os.path.join(OUTPUT_DIR, 'SGCAM_DEX_Patcher.zip')
    
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for dirpath, dirnames, filenames in os.walk(tmp_dir):
            for filename in filenames:
                file_path = os.path.join(dirpath, filename)
                arcname = os.path.relpath(file_path, tmp_dir)
                zf.write(file_path, arcname)
                print(f"    added: {arcname}")
    
    # Clean up
    shutil.rmtree(tmp_dir)
    
    print()
    print(f"[✓] Module built: {zip_path}")
    print(f"    Size: {os.path.getsize(zip_path):,} bytes")
    print()
    print("=" * 60)
    print("Install via KernelSU Manager or Magisk:")
    print("  1. Push the zip to your device")
    print("  2. Install in KernelSU Manager / Magisk")
    print("  3. Ensure SGCAM is installed BEFORE module")
    print("  4. Reboot or force-stop SGCAM after install")
    print("=" * 60)

if __name__ == '__main__':
    main()
