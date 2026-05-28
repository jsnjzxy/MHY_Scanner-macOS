#!/bin/bash
# 本地 CI 模拟构建脚本
# 用于推送前验证构建是否通过
# 用法: bash scripts/local-ci.sh [arm64|x64]

set -e

echo "=========================================="
echo "本地 CI 验证构建"
echo "=========================================="

# 解析架构参数
ARCH="${1:-arm64}"
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="arm64-osx"
    CMAKE_ARCH="arm64"
elif [ "$ARCH" = "x64" ]; then
    TRIPLET="x64-osx"
    CMAKE_ARCH="x86_64"
else
    echo "错误: 不支持的架构 '$ARCH'"
    echo "用法: $0 [arm64|x64]"
    exit 1
fi

echo "目标架构: $ARCH ($TRIPLET)"

# 检查 vcpkg
if [ -z "$VCPKG_ROOT" ]; then
    echo "错误: VCPKG_ROOT 未设置"
    echo "请先设置: export VCPKG_ROOT=/path/to/vcpkg"
    exit 1
fi

# 设置环境变量
export VCPKG_INSTALLED_DIR="$(pwd)/vcpkg_installed"
export VCPKG_TARGET_TRIPLET="$TRIPLET"
export CMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH"

PRESET="Release-CI"

echo ""
echo "[1/4] 下载 get_abogus..."
GET_ABOGUS_DIR="3rdparty/abogus_cpp/js"
mkdir -p "$GET_ABOGUS_DIR"

if [ ! -f "$GET_ABOGUS_DIR/get_abogus" ]; then
    DOWNLOAD_URL="https://github.com/jsnjzxy/abogus_cpp/releases/latest/download/get_abogus-macos-${ARCH}.tar.gz"
    echo "下载: $DOWNLOAD_URL"
    curl -L -o /tmp/get_abogus.tar.gz "$DOWNLOAD_URL"
    tar -xzf /tmp/get_abogus.tar.gz -C "$GET_ABOGUS_DIR/"
    chmod +x "$GET_ABOGUS_DIR/get_abogus"
    echo "✅ get_abogus 已下载"
else
    echo "✅ get_abogus 已存在，跳过下载"
fi

echo ""
echo "[2/4] 配置 CMake..."
cmake --preset $PRESET \
    -DVCPKG_INSTALLED_DIR="$VCPKG_INSTALLED_DIR" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH"

echo ""
echo "[3/4] 构建项目..."
cmake --build --preset $PRESET

echo ""
echo "[4/4] 检查产物..."
APP_PATH="${PRESET}_build/bin/Release/MHY_Scanner.app"
if [ -d "$APP_PATH" ]; then
    echo "✅ 构建成功: $APP_PATH"
    ls -la "${PRESET}_build/bin/Release/"

    # 检查架构
    echo ""
    echo "验证架构:"
    file "$APP_PATH/Contents/MacOS/MHY_Scanner"
else
    echo "❌ 构建失败: App 未生成"
    exit 1
fi

echo ""
echo "=========================================="
echo "✅ 本地验证通过，可以推送"
echo "=========================================="
