#!/bin/bash
# MHY_Scanner macOS 构建脚本（使用 CMakePresets）

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ========================================
# 加载环境变量配置（如果存在）
# ========================================
ENV_FILE="$SCRIPT_DIR/.env.local"
if [ -f "$ENV_FILE" ]; then
    set -a
    source "$ENV_FILE" 2>/dev/null || true
    set +a
fi

# ========================================
# 配置 vcpkg 镜像（仅在本地环境使用，GitHub Actions 使用默认源）
# ========================================
if [ -z "$GITHUB_ACTIONS" ]; then
    export X_VCPKG_ASSET_SOURCES="x-azurl,https://mirrors.tuna.tsinghua.edu.cn/vcpkg/assets/"
fi

# ========================================
# 配置 vcpkg 二进制缓存（加速编译）
# ========================================
export VCPKG_DEFAULT_BINARY_CACHE="${VCPKG_DEFAULT_BINARY_CACHE:-$HOME/.vcpkg/cache}"
export VCPKG_BINARY_SOURCES="${VCPKG_BINARY_SOURCES:-default;files,$VCPKG_DEFAULT_BINARY_CACHE,readwrite}"

# ========================================
# 检查构建工具依赖
# ========================================
check_dependencies() {
    local missing_deps=()
    local required_tools=("cmake" "ninja" "pkg-config")

    echo "检查构建工具..."

    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_deps+=("$tool")
        fi
    done

    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo "错误: 缺少以下构建工具: ${missing_deps[*]}"
        echo "请运行: brew install cmake ninja pkg-config"
        exit 1
    fi

    echo "所有构建工具已安装 ✓"
    echo ""
}

# ========================================
# 主流程
# ========================================
echo "=========================================="
echo "MHY_Scanner macOS 构建脚本"
echo "=========================================="

check_dependencies

BUILD_TYPE="${1:-Release}"

if [ "$BUILD_TYPE" = "-h" ] || [ "$BUILD_TYPE" = "--help" ]; then
    echo "用法: $0 [Debug|Release|RelWithDebInfo|clean]"
    echo ""
    echo "选项:"
    echo "  Debug          - 调试构建"
    echo "  Release        - 发布构建 (默认)"
    echo "  RelWithDebInfo - 带调试信息的发布构建"
    echo "  clean          - 清理构建目录"
    exit 0
fi

if [ "$BUILD_TYPE" = "clean" ]; then
    echo "清理构建目录..."
    rm -rf Release_build Debug_build RelWithDebInfo_build build out
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
    echo "清理完成"
    exit 0
fi

echo "构建类型: $BUILD_TYPE"
echo ""

# 使用 CMakePresets 配置和构建
cmake --preset "$BUILD_TYPE"
cmake --build --preset "$BUILD_TYPE"

echo ""
echo "=========================================="
echo "构建成功！"
echo "=========================================="

# 查找生成的 app
if [ -f "${BUILD_TYPE}_build/bin/${BUILD_TYPE}/MHY_Scanner.app/Contents/MacOS/MHY_Scanner" ]; then
    APP_PATH="${SCRIPT_DIR}/${BUILD_TYPE}_build/bin/${BUILD_TYPE}/MHY_Scanner.app"
    echo "应用位置: $APP_PATH"
    echo ""

    # Ad-hoc 签名
    echo "=========================================="
    echo "Ad-hoc 签名..."
    echo "=========================================="
    xattr -cr "$APP_PATH" 2>/dev/null || true
    codesign --force --deep -s - "$APP_PATH" 2>&1 || true
    codesign -v "$APP_PATH" 2>&1 || true
    echo ""
    echo "运行应用: open $APP_PATH"
else
    echo "未找到应用包"
fi
