# 构建指南

本文档介绍如何在 macOS 上从源码构建 MHY_Scanner。

## 系统要求

- macOS 15.0 (Sequoia) 或更高版本
- Apple Silicon (M1/M2/M3) 或 Intel Mac
- Xcode Command Line Tools
- Homebrew

## 安装构建工具

```bash
# 更新 Homebrew
brew update

# 构建工具
brew install cmake ninja pkg-config

# vcpkg 构建所需的 autotools
brew install autoconf autoconf-archive automake libtool

# 可选开发工具
brew install git clang-format
```

## 安装 vcpkg

```bash
# 克隆 vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh

# 设置环境变量 (添加到 ~/.zshrc)
export VCPKG_ROOT=~/vcpkg
```

## 可选：加速下载

使用清华大学镜像源加速 vcpkg 下载：

```bash
export X_VCPKG_ASSET_SOURCES="x-azurl,https://mirrors.tuna.tsinghua.edu.cn/vcpkg/assets/"
```

## 构建项目

```bash
# 运行构建脚本
./scripts/build_mac_vcpkg.sh Release
```

构建完成后，应用位于：
```
Release_build/bin/Release/MHY_Scanner.app
```

## DMG 打包

```bash
./scripts/package_dmg.sh Release
```

## 抖音直播间环境（可选）

使用抖音直播间功能前，需安装 Node.js：

```bash
# 安装 Node.js
brew install node

# 验证
node -v
```

## 故障排除

### Qt6 未找到

```bash
# 检查 vcpkg 安装
ls ~/vcpkg/installed/arm64-osx/share/Qt6

# 确认 VCPKG_ROOT 环境变量
echo $VCPKG_ROOT
```

### OpenCV 问题

如果 OpenCV 链接失败，确认 vcpkg 已正确安装：

```bash
vcpkg list opencv
```

### 权限错误

```bash
# 给脚本添加执行权限
chmod +x scripts/build_mac_vcpkg.sh
chmod +x scripts/package_dmg.sh
chmod +x scripts/check_deps.sh
```

## 依赖说明

项目使用 vcpkg 静态链接，主要依赖：

| 依赖 | 用途 |
|------|------|
| Qt 6 | GUI 框架 (Core, Widgets, Gui, Multimedia, MultimediaWidgets) |
| OpenCV | 图像处理 (core, imgproc, imgcodecs, highgui) |
| FFmpeg | 视频流处理 (avcodec, avformat, swscale 等) |
| cURL | HTTP 请求 |
| OpenSSL | 加密 |
| nlohmann/json | JSON 处理 |
| nayuki-qr-code-generator | QR 码生成 |

### 本地第三方库

- **abogus_cpp**: 抖音 a_bogus 算法实现，位于 `3rdparty/abogus_cpp/`

## vcpkg 依赖配置

`vcpkg.json` 配置：

```json
{
  "name": "mhy-scanner",
  "version": "1.1.15",
  "dependencies": [
    { "name": "qtbase", "features": ["dbus", "gui", "widgets"] },
    { "name": "qtdeclarative" },
    { "name": "qtquickcontrols2" },
    { "name": "qtwebchannel" },
    { "name": "qtmultimedia" },
    { "name": "opencv1.1.15" },
    { "name": "ffmpeg", "features": ["avcodec", "avdevice", "avfilter", "avformat", "swresample", "swscale"] },
    { "name": "curl", "features": ["ssl"] },
    { "name": "openssl" },
    { "name": "gtest" },
    { "name": "nlohmann-json" }
  ]
}
```
