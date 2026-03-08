#!/bin/bash

# macOS App Bundle/DMG 依赖健康检查脚本
# 用于验证打包后的应用是否有依赖问题

set -e

# ========================================
# 配置
# ========================================
TARGET="$1"

if [ -z "$TARGET" ]; then
    echo "用法: $0 <App.app 路径 或 DMG 路径>"
    echo ""
    echo "示例:"
    echo "  $0 ./MHY_Scanner.app"
    echo "  $0 ./MHY_Scanner-v1.1.15.dmg"
    echo ""
    exit 1
fi

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS="${GREEN}✅${NC}"
FAIL="${RED}❌${NC}"
WARN="${YELLOW}⚠️ ${NC}"
INFO="${BLUE}ℹ️ ${NC}"

success() { echo -e "${GREEN}[OK]${NC} $1"; }
error() { echo -e "${RED}[FAIL]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }

# ========================================
# 解析目标
# ========================================
APP_PATH=""
TEMP_MOUNT=""

if [[ "$TARGET" == *.app ]]; then
    APP_PATH="$TARGET"
elif [[ "$TARGET" == *.dmg ]]; then
    # 挂载 DMG
    TEMP_MOUNT="/tmp/dmg_check_$$"
    mkdir -p "$TEMP_MOUNT"

    info "挂载 DMG: $TARGET"
    hdiutil attach "$TARGET" -readonly -nobrowse -mountpoint "$TEMP_MOUNT" -quiet 2>&1 | grep -v "^正在检查" | grep -v "^已验证" || true

    # 查找 .app
    APP_PATH=$(find "$TEMP_MOUNT" -name "*.app" -maxdepth 1 2>/dev/null | head -1)

    if [ -z "$APP_PATH" ]; then
        error "DMG 中未找到 .app 文件"
        hdiutil detach "$TEMP_MOUNT" -quiet 2>/dev/null || true
        exit 1
    fi

    info "找到应用: $(basename "$APP_PATH")"
else
    error "不支持的文件类型，必须是 .app 或 .dmg"
    exit 1
fi

# 检查 App Bundle 是否存在
if [ ! -d "$APP_PATH" ]; then
    error "应用不存在: $APP_PATH"
    [ -n "$TEMP_MOUNT" ] && hdiutil detach "$TEMP_MOUNT" -quiet 2>/dev/null || true
    exit 1
fi

APP_NAME=$(basename "$APP_PATH" .app)
FRAMEWORKS_DIR="$APP_PATH/Contents/Frameworks"
MAIN_BINARY="$APP_PATH/Contents/MacOS/$APP_NAME"

# ========================================
# 清理函数
# ========================================
cleanup() {
    if [ -n "$TEMP_MOUNT" ] && [ -d "$TEMP_MOUNT" ]; then
        info "卸载 DMG..."
        hdiutil detach "$TEMP_MOUNT" -quiet 2>/dev/null || true
    fi
}
trap cleanup EXIT

# ========================================
# 检查函数
# ========================================

# 检查硬编码路径
check_hardcoded_paths() {
    echo ""
    echo "========================================="
    echo "1. 硬编码路径检查"
    echo "========================================="

    # 获取所有外部依赖路径（排除系统路径和相对路径）
    local bad_paths=""
    bad_paths=$(find "$APP_PATH/Contents" -type f \( -name "*.dylib" -o -perm +111 \) -exec otool -L {} \; 2>/dev/null | grep -v -E ":$|@executable_path|@rpath|@loader_path|/System/Library|/usr/lib" | grep -E "^\s*/" | awk '{print $1}' | sort -u || true)

    # 排除 libdbus 的旧路径（这是正常的，因为 QtDBus 同时记录了新路径）
    bad_paths=$(echo "$bad_paths" | grep -v "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" || true)

    if [ -z "$bad_paths" ]; then
        success "无硬编码系统路径"
        return 0
    else
        echo -e "${FAIL} 发现硬编码路径:"
        echo "$bad_paths" | head -20
        return 1
    fi
}

# 检查 Homebrew 路径（除了 libdbus）
check_homebrew_paths() {
    echo ""
    echo "========================================="
    echo "2. Homebrew 路径检查"
    echo "========================================="

    local homebrew_deps=""
    homebrew_deps=$(find "$APP_PATH/Contents" -type f \( -name "*.dylib" -o -perm +111 \) -exec otool -L {} \; 2>/dev/null | grep -E "^\s*/opt/homebrew" | awk '{print $1}' | grep -v "libdbus-1.3.dylib" | sort -u || true)

    if [ -z "$homebrew_deps" ]; then
        success "无 Homebrew 路径"
        return 0
    else
        echo -e "${FAIL} 发现 Homebrew 路径:"
        echo "$homebrew_deps"
        return 1
    fi
}

# 检查 QtDBus 和 libdbus
check_qtdbus_libdbus() {
    echo ""
    echo "========================================="
    echo "3. QtDBus / libdbus 检查"
    echo "========================================="

    local qtdbus_bin="$FRAMEWORKS_DIR/QtDBus.framework/Versions/A/QtDBus"
    local issues=0

    if [ ! -d "$FRAMEWORKS_DIR/QtDBus.framework" ]; then
        info "QtDBus 框架不存在"
        return 0
    fi

    # 检查 QtDBus 是否引用 libdbus
    if [ -f "$qtdbus_bin" ]; then
        local dbus_dep=$(otool -L "$qtdbus_bin" 2>/dev/null | grep "libdbus" || true)

        if [ -n "$dbus_dep" ]; then
            if [[ "$dbus_dep" == *"@loader_path"* ]] || [[ "$dbus_dep" == *"@rpath"* ]]; then
                success "QtDBus 正确引用 ($dbus_dep)"
            else
                echo -e "${FAIL} QtDBus 依赖格式错误:"
                echo "   $dbus_dep"
                issues=1
            fi
        else
            info "QtDBus 未引用 libdbus"
        fi
    fi

    # 检查 libdbus 是否存在
    if [ -f "$FRAMEWORKS_DIR/libdbus-1.3.dylib" ]; then
        success "libdbus-1.3.dylib 存在"
    else
        echo -e "${FAIL} libdbus-1.3.dylib 缺失"
        issues=1
    fi

    return $issues
}

# 检查主程序 rpath
check_main_rpath() {
    echo ""
    echo "========================================="
    echo "4. 主程序 rpath 检查"
    echo "========================================="

    if [ ! -f "$MAIN_BINARY" ]; then
        echo -e "${WARN} 主可执行文件不存在: $MAIN_BINARY"
        return 1
    fi

    local rpaths=""
    rpaths=$(otool -l "$MAIN_BINARY" 2>/dev/null | grep -A 2 "LC_RPATH" | grep "path" | awk '{print $2}' || true)

    if [ -z "$rpaths" ]; then
        echo -e "${WARN} 未设置 rpath"
        return 1
    fi

    local has_homebrew=$(echo "$rpaths" | grep -c "/opt/homebrew" || true)
    local has_frameworks=$(echo "$rpaths" | grep -c "@executable_path/../Frameworks" || true)

    if [ "$has_homebrew" -gt 0 ]; then
        echo -e "${FAIL} rpath 包含 Homebrew 路径:"
        echo "$rpaths"
        return 1
    elif [ "$has_frameworks" -gt 0 ]; then
        success "rpath 正确 (@executable_path/../Frameworks)"
        return 0
    else
        echo -e "${WARN} rpath 没有包含 @executable_path/../Frameworks:"
        echo "$rpaths"
        return 1
    fi
}

# 检查应用签名
check_signature() {
    echo ""
    echo "========================================="
    echo "5. 应用签名检查"
    echo "========================================="

    local sig_info=$(codesign -dv "$APP_PATH" 2>&1)

    if echo "$sig_info" | grep -q "Format="; then
        success "应用已签名"

        local sig_type=$(echo "$sig_info" | grep "Format=" | cut -d= -f2 || echo "unknown")
        info "签名类型: $sig_type"

        return 0
    else
        echo -e "${FAIL} 应用未签名或签名无效"
        echo "详情: $sig_info"
        return 1
    fi
}

# 统计关键库
count_libraries() {
    echo ""
    echo "========================================="
    echo "6. 关键库统计"
    echo "========================================="

    local qt_count=0
    local opencv_count=0
    local ffmpeg_count=0

    if [ -d "$FRAMEWORKS_DIR" ]; then
        qt_count=$(ls -d "$FRAMEWORKS_DIR"/*.framework 2>/dev/null | wc -l | tr -d ' ' || echo "0")
        opencv_count=$(ls "$FRAMEWORKS_DIR"/libopencv_*.dylib 2>/dev/null | wc -l | tr -d ' ' || echo "0")
        ffmpeg_count=$(ls "$FRAMEWORKS_DIR"/libav*.dylib "$FRAMEWORKS_DIR"/libsw*.dylib "$FRAMEWORKS_DIR"/libpost*.dylib 2>/dev/null | wc -l | tr -d ' ' || echo "0")
    fi

    echo "Qt 框架:     $qt_count 个"
    echo "OpenCV:       $opencv_count 个"
    echo "FFmpeg:      $ffmpeg_count 个"

    # 检查是否合理
    local issues=0
    if [ "$qt_count" -lt 5 ]; then
        echo -e "${WARN} Qt 框架数量偏少"
        issues=1
    fi
    if [ "$opencv_count" -lt 10 ]; then
        echo -e "${WARN} OpenCV 库数量偏少"
        issues=1
    fi
    if [ "$ffmpeg_count" -lt 5 ]; then
        echo -e "${WARN} FFmpeg 库数量偏少"
        issues=1
    fi

    return $issues
}

# 检查缺失依赖
check_missing_deps() {
    echo ""
    echo "========================================="
    echo "7. 缺失依赖检查"
    echo "========================================="

    local issues=0

    # 检查主程序依赖
    if [ -f "$MAIN_BINARY" ]; then
        local deps=$(otool -L "$MAIN_BINARY" 2>/dev/null | grep -E "^\s*/" | awk '{print $1}' | grep -v -E "/System|/usr/lib" || true)

        while IFS= read -r dep; do
            [ -z "$dep" ] && continue

            # 跳过相对路径（由 rpath 解析）
            [[ "$dep" == @* ]] && continue

            # 转换绝对路径为相对路径
            local basename=$(basename "$dep")

            # 检查是否在 Frameworks 目录中
            if [[ "$dep" == "/opt/homebrew"* ]] || [[ "$dep" == "/usr/local"* ]]; then
                # 检查对应的库是否存在
                if [ ! -f "$FRAMEWORKS_DIR/$basename" ]; then
                    echo -e "${FAIL} 缺失依赖: $basename"
                    issues=1
                fi
            fi
        done <<< "$deps"
    fi

    if [ $issues -eq 0 ]; then
        success "无缺失依赖"
    fi

    return $issues
}

# 主可执行文件依赖详情
show_main_deps() {
    echo ""
    echo "========================================="
    echo "8. 主可执行文件依赖详情"
    echo "========================================="

    if [ ! -f "$MAIN_BINARY" ]; then
        echo -e "${WARN} 主可执行文件不存在"
        return
    fi

    info "主程序: $MAIN_BINARY"
    echo ""

    otool -L "$MAIN_BINARY" 2>/dev/null | head -50
}

# ========================================
# 主流程
# ========================================

echo ""
echo "========================================="
echo "  macOS App Bundle 依赖健康检查"
echo "========================================="
echo "目标: $TARGET"
echo "App:  $APP_NAME"
echo ""

# 执行所有检查
check_hardcoded_paths
check_homebrew_paths
check_qtdbus_libdbus
check_main_rpath
check_signature
count_libraries
check_missing_deps
show_main_deps

# 总结
echo ""
echo "========================================="
echo "  检查完成"
echo "========================================="
echo ""
echo "应用路径: $APP_PATH"
echo ""
echo "如需详细调试，可运行:"
echo "  otool -L \"$APP_PATH/Contents/MacOS/$APP_NAME\""
echo "  otool -L \"$APP_PATH/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus\""
echo ""
