# SGCAM DEX Patcher - Uninstall Script
# Removes bind mount and restores original APK

SGCAM_PKG="com.samsung.android.scan3d"
APK=$(pm path $SGCAM_PKG 2>/dev/null | grep base | head -1 | cut -d: -f2)
if [ -n "$APK" ]; then
  nsenter -t 1 -m -- umount "$APK" 2>/dev/null
  # Clear dalvik-cache so it recompiles from original APK
  rm -rf /data/app/*/com.samsung.android.scan3d*/oat/ 2>/dev/null
  rm -f /data/dalvik-cache/arm64/*scan3d* 2>/dev/null
fi

echo "  [✓] SGCAM APK restored to original"
