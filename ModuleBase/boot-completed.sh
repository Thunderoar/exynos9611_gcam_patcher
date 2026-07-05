#!/system/bin/sh
# SGCAM DEX Patcher - Boot Completed Script
# Re-establishes the bind mount for the patched APK on every boot

while [ "$(getprop sys.boot_completed)" != "1" ]; do sleep 1; done
sleep 3

MODDIR="${0%/*}"
MODULE_NAME="SGCAM_DEX_Patcher"
MODULE_PATH="/data/adb/modules/$MODULE_NAME"
SGCAM_PKG="com.samsung.android.scan3d"

# Check if the module is disabled
if [ -f "$MODULE_PATH/disable" ] || [ ! -f "$MODULE_PATH/base_patched.apk" ]; then
  # Try to clean up any stale bind mount
  APK=$(pm path $SGCAM_PKG 2>/dev/null | grep base | head -1 | cut -d: -f2)
  if [ -n "$APK" ]; then
    nsenter -t 1 -m -- umount "$APK" 2>/dev/null
  fi
  exit 0
fi

APK=$(pm path $SGCAM_PKG 2>/dev/null | grep base | head -1 | cut -d: -f2)
if [ -z "$APK" ]; then
  log -p w -t "SGCAM_Patcher" "SGCAM not installed, skipping bind mount"
  exit 0
fi

NS=""
command -v nsenter >/dev/null 2>&1 && NS="nsenter -t 1 -m --"

# Check if bind mount is already active and correct
TGT_HASH=$($NS md5sum "$APK" 2>/dev/null | cut -d' ' -f1)
SRC_HASH=$(md5sum "$MODULE_PATH/base_patched.apk" 2>/dev/null | cut -d' ' -f1)

if [ -z "$SRC_HASH" ]; then
  log -p e -t "SGCAM_Patcher" "Patched APK not found at $MODULE_PATH/base_patched.apk"
  exit 1
fi

if [ "$TGT_HASH" != "$SRC_HASH" ]; then
  # Bind mount needs to be established or re-established
  $NS mount -o bind "$MODULE_PATH/base_patched.apk" "$APK" 2>/dev/null
  if [ $? -eq 0 ]; then
    log -p i -t "SGCAM_Patcher" "Bind mount established: $MODULE_PATH/base_patched.apk -> $APK"
    
    # Clear compiled cache in case it was compiled before our mount
    rm -rf /data/app/*/com.samsung.android.scan3d*/oat/ 2>/dev/null
    rm -f /data/dalvik-cache/arm64/*scan3d* 2>/dev/null
    rm -f /data/dalvik-cache/arm64/*camera2* 2>/dev/null
    
    # Restart camera services to pick up patched DEX
    am force-stop $SGCAM_PKG 2>/dev/null
  else
    log -p e -t "SGCAM_Patcher" "Bind mount FAILED"
  fi
else
  log -p d -t "SGCAM_Patcher" "Bind mount already active and correct"
fi
