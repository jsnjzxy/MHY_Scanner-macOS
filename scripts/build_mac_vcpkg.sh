#!/bin/bash
# MHY_Scanner macOS 构建脚本
# 用法: bash scripts/build_mac_vcpkg.sh [Debug|Release] [arm64|x64] [--dev]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "MHY_Scanner macOS 构建脚本"
echo "=========================================="

# ========================================
# 解析参数
# ========================================
BUILD_TYPE="Release"
ARCH="arm64"
DEV_MODE="OFF"

while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|Release|RelWithDebInfo)
            BUILD_TYPE="$1"
            shift
            ;;
        arm64|x64)
            ARCH="$1"
            shift
            ;;
        --dev|-d)
            DEV_MODE="ON"
            shift
            ;;
        -h|--help)
            echo "用法: $0 [构建类型] [架构] [--dev]"
            echo ""
            echo "构建类型: Debug, Release (默认), RelWithDebInfo"
            echo "架构:     arm64 (默认), x64"
            echo "--dev:    开发模式，启用代理支持（用于抓包调试）"
            echo ""
            echo "示例:"
            echo "  $0                      # Release arm64"
            echo "  $0 Debug                # Debug arm64"
            echo "  $0 Release x64          # Release x64 (交叉编译)"
            echo "  $0 Debug arm64 --dev    # Debug arm64 开发模式"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

# 设置架构相关变量
if [ "$ARCH" = "arm64" ]; then
    TRIPLET="arm64-osx"
    CMAKE_ARCH="arm64"
else
    TRIPLET="x64-osx"
    CMAKE_ARCH="x86_64"
fi

echo "构建类型: $BUILD_TYPE"
echo "目标架构: $ARCH ($TRIPLET)"
echo "开发模式: $DEV_MODE"
echo ""

# ========================================
# 检查依赖
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

    echo "✅ 所有构建工具已安装"
    echo ""
}

check_dependencies

# ========================================
# 检查 vcpkg
# ========================================
if [ -z "$VCPKG_ROOT" ]; then
    echo "错误: VCPKG_ROOT 未设置"
    echo "请先设置: export VCPKG_ROOT=/path/to/vcpkg"
    echo ""
    echo "安装 vcpkg:"
    echo "  git clone https://github.com/microsoft/vcpkg.git ~/vcpkg"
    echo "  ~/vcpkg/bootstrap-vcpkg.sh"
    exit 1
fi

# 配置 vcpkg 镜像（本地环境使用清华源）
if [ -z "$GITHUB_ACTIONS" ]; then
    export X_VCPKG_ASSET_SOURCES="x-azurl,https://mirrors.tuna.tsinghua.edu.cn/vcpkg/assets/"
fi

# ========================================
# 下载 get_abogus
# ========================================
echo "检查 get_abogus..."
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
    # 检查架构是否匹配
    CURRENT_ARCH=$(file "$GET_ABOGUS_DIR/get_abogus" | grep -o 'arm64\|x86_64' | head -1)
    EXPECTED_ARCH=$([ "$ARCH" = "arm64" ] && echo "arm64" || echo "x86_64")

    if [ "$CURRENT_ARCH" != "$EXPECTED_ARCH" ]; then
        echo "⚠️  get_abogus 架构不匹配 (当前: $CURRENT_ARCH, 需要: $EXPECTED_ARCH)"
        echo "重新下载..."
        rm -f "$GET_ABOGUS_DIR/get_abogus"
        DOWNLOAD_URL="https://github.com/jsnjzxy/abogus_cpp/releases/latest/download/get_abogus-macos-${ARCH}.tar.gz"
        curl -L -o /tmp/get_abogus.tar.gz "$DOWNLOAD_URL"
        tar -xzf /tmp/get_abogus.tar.gz -C "$GET_ABOGUS_DIR/"
        chmod +x "$GET_ABOGUS_DIR/get_abogus"
        echo "✅ get_abogus 已更新"
    else
        echo "✅ get_abogus 已存在"
    fi
fi

echo ""

# ========================================
# 构建
# ========================================
echo "=========================================="
echo "开始构建..."
echo "=========================================="

# 设置环境变量
export VCPKG_INSTALLED_DIR="${VCPKG_INSTALLED_DIR:-$SCRIPT_DIR/vcpkg_installed}"
export VCPKG_TARGET_TRIPLET="$TRIPLET"
export CMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH"

# 选择 preset
if [ "$DEV_MODE" = "ON" ]; then
    PRESET="Debug"
    CMAKE_ARGS="-DDEV=ON"
else
    PRESET="$BUILD_TYPE"
    CMAKE_ARGS=""
fi

# 配置
cmake --preset "$PRESET" \
    -DVCPKG_INSTALLED_DIR="$VCPKG_INSTALLED_DIR" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH" \
    $CMAKE_ARGS

# 构建
cmake --build --preset "$PRESET"

echo ""
echo "=========================================="
echo "构建成功！"
echo "=========================================="

# 查找生成的 app
APP_PATH="${PRESET}_build/bin/${PRESET}/MHY_Scanner.app"
if [ -d "$APP_PATH" ]; then
    echo "应用位置: $APP_PATH"

    # 复制 get_abogus 到 app bundle
    mkdir -p "$APP_PATH/Contents/Resources/js"
    cp "$GET_ABOGUS_DIR/get_abogus" "$APP_PATH/Contents/Resources/js/"
    chmod +x "$APP_PATH/Contents/Resources/js/get_abogus"

    # Ad-hoc 签名
    echo ""
    echo "Ad-hoc 签名..."
    xattr -cr "$APP_PATH" 2>/dev/null || true
    codesign --force --deep -s - "$APP_PATH" 2>/dev/null || true

    echo ""
    echo "验证架构:"
    file "$APP_PATH/Contents/MacOS/MHY_Scanner"

    echo ""
    if [ "$DEV_MODE" = "ON" ]; then
        echo "🔧 开发模式已启用，支持抓包调试"
        echo "   开启 Charles 后运行应用即可抓包"
    fi
    echo ""
    echo "运行应用: open \"$APP_PATH\""
else
    echo "未找到应用包"
fi
