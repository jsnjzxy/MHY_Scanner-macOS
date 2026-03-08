#!/bin/bash
# MHY_Scanner macOS DMG 打包脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

BUILD_TYPE="${1:-Release}"
DMG_NAME="MHY_Scanner-${BUILD_TYPE}"
APP_PATH=""

# 查找应用
for config_dir in "Release_build/bin/Release" "Release_build/bin/Debug" "Release_build/bin/RelWithDebInfo"; do
    if [ -f "$config_dir/MHY_Scanner.app/Contents/MacOS/MHY_Scanner" ]; then
        APP_PATH="$config_dir/MHY_Scanner.app"
        break
    fi
done

if [ -z "$APP_PATH" ]; then
    echo "ERROR: App not found"
    echo "Please run: bash scripts/build_mac_vcpkg.sh $BUILD_TYPE"
    exit 1
fi

echo "=========================================="
echo "MHY_Scanner DMG Package Script"
echo "=========================================="
echo "App: $APP_PATH"

# 签名
echo ""
echo "Signing app..."
xattr -cr "$APP_PATH" 2>/dev/null || true
codesign --force --deep -s - "$APP_PATH"
codesign -v "$APP_PATH" && echo "OK: Signature verified"

# 创建临时 DMG 目录
TEMP_DMG_DIR="dmg_temp"
rm -rf "$TEMP_DMG_DIR"
mkdir -p "$TEMP_DMG_DIR"

# 复制应用
cp -R "$APP_PATH" "$TEMP_DMG_DIR/"

# 使用 create-dmg 创建 DMG
DMG_PATH="$DMG_NAME.dmg"
echo ""
echo "Creating DMG with create-dmg..."

if command -v create-dmg &> /dev/null; then
    # 使用 create-dmg
    create-dmg \
        --volname "MHY_Scanner" \
        --window-pos 200 120 \
        --window-size 500 350 \
        --icon-size 80 \
        --app-drop-link 350 150 \
        "$DMG_PATH" \
        "$TEMP_DMG_DIR"
else
    # 回退到 hdiutil create
    hdiutil create -volname "MHY_Scanner" \
        -srcfolder "$TEMP_DMG_DIR" \
        -ov -format UDZO \
        "$DMG_PATH"
fi

rm -rf "$TEMP_DMG_DIR"

echo ""
echo "OK: DMG created: $DMG_PATH"
ls -la "$DMG_PATH"
