#!/bin/bash
# 版本号更新脚本
# 使用方法: ./scripts/update_version.sh <新版本号>
# 示例: ./scripts/update_version.sh 1.1.16

set -e

VERSION="$1"

if [ -z "$VERSION" ]; then
    echo "错误: 请提供新版本号"
    echo "使用方法: $0 <新版本号>"
    echo "示例: $0 1.1.16"
    exit 1
fi

# 验证版本号格式 (MAJOR.MINOR.MICRO)
if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "错误: 版本号格式不正确，应为 MAJOR.MINOR.MICRO 格式，例如: 1.1.16"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "====================================="
echo "  版本号更新工具"
echo "====================================="
echo ""
echo "当前目录: $PROJECT_DIR"
echo "新版本号: $VERSION"
echo ""

# 提取版本号各部分
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
MICRO=$(echo "$VERSION" | cut -d. -f3)

UPDATED=0
SKIPPED=0
FAILED=0

# ========================================
# CMakeLists.txt
# ========================================
CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
    cp "$CMAKE_FILE" "$CMAKE_FILE.bak"
    # 使用 awk 逐行处理
    if awk -v major="$MAJOR" -v minor="$MINOR" -v micro="$MICRO" '
        /^set\(MAJOR_VERSION [0-9]+\)$/ { print "set(MAJOR_VERSION " major ")"; next }
        /^set\(MINOR_VERSION [0-9]+\)$/ { print "set(MINOR_VERSION " minor ")"; next }
        /^set\(MICRO_VERSION [0-9]+\)$/ { print "set(MICRO_VERSION " micro ")"; next }
        { print }
    ' "$CMAKE_FILE.bak" > "$CMAKE_FILE"; then
        rm "$CMAKE_FILE.bak"
        echo "✅ 更新: CMakeLists.txt"
        ((UPDATED++))
    else
        mv "$CMAKE_FILE.bak" "$CMAKE_FILE"
        echo "❌ 失败: CMakeLists.txt"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: CMakeLists.txt (不存在)"
    ((SKIPPED++))
fi

# ========================================
# vcpkg.json
# ========================================
VCPKG_FILE="$PROJECT_DIR/vcpkg.json"
if [ -f "$VCPKG_FILE" ]; then
    cp "$VCPKG_FILE" "$VCPKG_FILE.bak"
    if sed -i '' 's|"version": "[0-9.]*"|"version": "'"$VERSION"'"|g' "$VCPKG_FILE"; then
        rm "$VCPKG_FILE.bak"
        echo "✅ 更新: vcpkg.json"
        ((UPDATED++))
    else
        mv "$VCPKG_FILE.bak" "$VCPKG_FILE"
        echo "❌ 失败: vcpkg.json"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: vcpkg.json (不存在)"
    ((SKIPPED++))
fi

# ========================================
# src/UI/Main.cpp
# ========================================
MAIN_CPP="$PROJECT_DIR/src/UI/Main.cpp"
if [ -f "$MAIN_CPP" ]; then
    cp "$MAIN_CPP" "$MAIN_CPP.bak"
    if sed -i '' 's|setApplicationVersion("[0-9.]*")|setApplicationVersion("'"$VERSION"'")|g' "$MAIN_CPP"; then
        rm "$MAIN_CPP.bak"
        echo "✅ 更新: src/UI/Main.cpp"
        ((UPDATED++))
    else
        mv "$MAIN_CPP.bak" "$MAIN_CPP"
        echo "❌ 失败: src/UI/Main.cpp"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: src/UI/Main.cpp (不存在)"
    ((SKIPPED++))
fi

# ========================================
# src/Resources/Info.plist
# ========================================
INFO_PLIST="$PROJECT_DIR/src/Resources/Info.plist"
if [ -f "$INFO_PLIST" ]; then
    cp "$INFO_PLIST" "$INFO_PLIST.bak"
    # 使用 Python 只更新 CFBundleShortVersionString
    python3 -c "
import re
version = '$VERSION'
with open('$INFO_PLIST', 'r') as f:
    content = f.read()
# 只更新 CFBundleShortVersionString 对应的字符串值
pattern = r'(<key>CFBundleShortVersionString</key>\n\t<string>)[0-9.]+(</string>)'
replacement = r'\g<1>' + version + r'\g<2>'
content = re.sub(pattern, replacement, content)
with open('$INFO_PLIST', 'w') as f:
    f.write(content)
"
    rm "$INFO_PLIST.bak"
    echo "✅ 更新: src/Resources/Info.plist"
    ((UPDATED++))
else
    echo "⚠️  跳过: src/Resources/Info.plist (不存在)"
    ((SKIPPED++))
fi

# ========================================
# src/Resources/MHY_Scanner.rc
# ========================================
RC_FILE="$PROJECT_DIR/src/Resources/MHY_Scanner.rc"
if [ -f "$RC_FILE" ]; then
    cp "$RC_FILE" "$RC_FILE.bak"
    if sed -i '' 's|VALUE "ProductVersion", "[0-9.]*"|VALUE "ProductVersion", "'"$VERSION"'"|g' "$RC_FILE"; then
        rm "$RC_FILE.bak"
        echo "✅ 更新: src/Resources/MHY_Scanner.rc"
        ((UPDATED++))
    else
        mv "$RC_FILE.bak" "$RC_FILE"
        echo "❌ 失败: src/Resources/MHY_Scanner.rc"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: src/Resources/MHY_Scanner.rc (不存在)"
    ((SKIPPED++))
fi

# ========================================
# docs/BUILDING.md
# ========================================
BUILDING_MD="$PROJECT_DIR/docs/BUILDING.md"
if [ -f "$BUILDING_MD" ]; then
    cp "$BUILDING_MD" "$BUILDING_MD.bak"
    if perl -i.bak -pe 's|"version": "[0-9.]*"|"version": "'"$VERSION"'"|g; s|v[0-9.]*"|v'"$VERSION"'"|g' "$BUILDING_MD"; then
        rm "$BUILDING_MD.bak" "$BUILDING_MD.bak.bak" 2>/dev/null || true
        echo "✅ 更新: docs/BUILDING.md"
        ((UPDATED++))
    else
        mv "$BUILDING_MD.bak" "$BUILDING_MD"
        echo "❌ 失败: docs/BUILDING.md"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: docs/BUILDING.md (不存在)"
    ((SKIPPED++))
fi

# ========================================
# docs/DEVELOPMENT.md
# ========================================
DEV_MD="$PROJECT_DIR/docs/DEVELOPMENT.md"
if [ -f "$DEV_MD" ]; then
    cp "$DEV_MD" "$DEV_MD.bak"
    if perl -i.bak -pe 's|"version": "[0-9.]*"|"version": "'"$VERSION"'"|g' "$DEV_MD"; then
        rm "$DEV_MD.bak" "$DEV_MD.bak.bak" 2>/dev/null || true
        echo "✅ 更新: docs/DEVELOPMENT.md"
        ((UPDATED++))
    else
        mv "$DEV_MD.bak" "$DEV_MD"
        echo "❌ 失败: docs/DEVELOPMENT.md"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: docs/DEVELOPMENT.md (不存在)"
    ((SKIPPED++))
fi

# ========================================
# README.md
# ========================================
README="$PROJECT_DIR/README.md"
if [ -f "$README" ]; then
    cp "$README" "$README.bak"
    if perl -i.bak -pe 's|### \*\*版本 - v[0-9.]*\*\*|### **版本 - v'"$VERSION"'**|g' "$README"; then
        rm "$README.bak" "$README.bak.bak" 2>/dev/null || true
        echo "✅ 更新: README.md"
        ((UPDATED++))
    else
        mv "$README.bak" "$README"
        echo "❌ 失败: README.md"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: README.md (不存在)"
    ((SKIPPED++))
fi

# ========================================
# scripts/check_deps.sh
# ========================================
CHECK_DEPS="$PROJECT_DIR/scripts/check_deps.sh"
if [ -f "$CHECK_DEPS" ]; then
    cp "$CHECK_DEPS" "$CHECK_DEPS.bak"
    if sed -i '' 's|MHY_Scanner-v[0-9.]*\.dmg|MHY_Scanner-v'"$VERSION"'.dmg|g' "$CHECK_DEPS"; then
        rm "$CHECK_DEPS.bak"
        echo "✅ 更新: scripts/check_deps.sh"
        ((UPDATED++))
    else
        mv "$CHECK_DEPS.bak" "$CHECK_DEPS"
        echo "❌ 失败: scripts/check_deps.sh"
        ((FAILED++))
    fi
else
    echo "⚠️  跳过: scripts/check_deps.sh (不存在)"
    ((SKIPPED++))
fi

# ========================================
# 输出结果
# ========================================
echo ""
echo "====================================="
echo "  更新完成"
echo "====================================="
echo "✅ 更新成功: $UPDATED 个文件"
echo "⚠️  跳过: $SKIPPED 个文件"
echo "❌ 更新失败: $FAILED 个文件"
echo ""
echo "当前版本: v$VERSION"
echo ""

if [ $FAILED -gt 0 ]; then
    echo "⚠️  部分文件更新失败，请检查上述错误"
    exit 1
fi

echo "提示: 记得更新 CHANGELOG.md 或提交说明"
