#!/bin/bash
# MHY_Scanner macOS vcpkg 构建脚本

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
    local autotools=("autoconf" "automake" "libtoolize")

    echo "检查构建工具..."

    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_deps+=("$tool")
        fi
    done

    for tool in "${autotools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_deps+=("$tool")
        fi
    done

    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo "=========================================="
        echo "错误: 缺少以下构建工具:"
        echo "=========================================="
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "请运行以下命令安装:"
        echo "  brew install cmake ninja pkg-config autoconf automake libtool"
        echo ""
        exit 1
    fi

    echo "所有构建工具已安装 ✓"
    echo ""
}

# ========================================
# 检查 vcpkg
# ========================================
check_vcpkg() {
    echo "DEBUG: check_vcpkg called"
    echo "DEBUG: VCPKG_ROOT='$VCPKG_ROOT'"
    echo "DEBUG: HOME='$HOME'"
    if [ -z "$VCPKG_ROOT" ]; then
        echo "=========================================="
        echo "错误: VCPKG_ROOT 环境变量未设置"
        echo "=========================================="
        echo "请设置: export VCPKG_ROOT=/path/to/vcpkg"
        echo "例如: export VCPKG_ROOT=~/vcpkg"
        exit 1
    fi

    if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
        echo "=========================================="
        echo "错误: vcpkg 未找到于 $VCPKG_ROOT"
        echo "=========================================="
        exit 1
    fi

    echo "vcpkg 根目录: $VCPKG_ROOT"
}

# ========================================
# 主流程
# ========================================
echo "=========================================="
echo "MHY_Scanner macOS vcpkg 构建脚本"
echo "=========================================="
echo "DEBUG: Script start VCPKG_ROOT='$VCPKG_ROOT'"

check_dependencies
check_vcpkg

# 检测架构
if [[ $(uname -m) == 'arm64' ]]; then
    TRIPLET="arm64-osx"
    ARCH="arm64"
else
    TRIPLET="x64-osx"
    ARCH="x64"
fi

BUILD_TYPE="${1:-Release}"

if [ "$BUILD_TYPE" = "-h" ] || [ "$BUILD_TYPE" = "--help" ]; then
    echo "用法: $0 [Debug|Release|clean]"
    echo ""
    echo "选项:"
    echo "  Debug    - 调试构建"
    echo "  Release  - 发布构建 (默认)"
    echo "  clean    - 清理构建目录"
    exit 0
fi

if [ "$BUILD_TYPE" = "clean" ]; then
    echo "清理构建目录..."
    rm -rf Release_build Debug_build out/build out/install build
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
    echo "清理完成"
    exit 0
fi

echo "架构: $ARCH"
echo "Triplet: $TRIPLET"
echo "构建类型: $BUILD_TYPE"
echo ""

BUILD_DIR="${BUILD_TYPE}_build"
if [ -d "$BUILD_DIR" ]; then
    echo "清理旧的构建目录..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

echo ""
echo "=========================================="
echo "配置 CMake"
echo "=========================================="
echo "Triplet: $TRIPLET"
echo "二进制缓存: $VCPKG_DEFAULT_BINARY_CACHE"
echo ""

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -G "Ninja Multi-Config"

if [ $? -ne 0 ]; then
    echo "CMake 配置失败"
    exit 1
fi

# 编译
echo ""
echo "=========================================="
echo "编译项目"
echo "=========================================="
CORE_COUNT=$(sysctl -n hw.ncpu)
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$CORE_COUNT"

if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo ""
echo "=========================================="
echo "构建成功！"
echo "=========================================="
echo ""
echo "应用位置:"
if [ -f "$BUILD_DIR/bin/$BUILD_TYPE/MHY_Scanner.app/Contents/MacOS/MHY_Scanner" ]; then
    APP_PATH="$SCRIPT_DIR/$BUILD_DIR/bin/$BUILD_TYPE/MHY_Scanner.app"
    echo "  $APP_PATH"
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
    echo "  $SCRIPT_DIR/$BUILD_DIR/bin/MHY_Scanner.app"
fi
