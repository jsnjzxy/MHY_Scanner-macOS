# 本地开发环境搭建指南

本文档介绍如何在 macOS 上搭建 MHY_Scanner 的本地开发环境。

## 系统要求

- macOS 15.0 (Sequoia) 或更高版本
- Apple Silicon (M1/M2/M3/M4) 或 Intel Mac
- Xcode Command Line Tools
- Homebrew

## 已知问题与解决方案

### Qt 6.10.1 与 Clang 21 编译错误

**问题描述**：在使用 Xcode 26.x / Clang 21.x 编译 Qt 6.10.1 时，会遇到以下错误：

```
error: implicitly declaring library function '__yield' with type 'void ()' [-Werror,-Wimplicit-function-declaration]
```

**原因**：Qt 源码中 `qyieldcpu.h` 缺少 `#include <arm_acle.h>` 头文件。

**解决方案**：需要为 vcpkg 的 Qt port 添加 patch 文件。

#### 步骤 1：创建 Patch 文件

创建文件 `~/vcpkg/buildtrees/versioning_/versions/qtbase/89d2fd203b771f2cf96f527cfc5afe5c73bb4a87/fix-yield.patch`：

```patch
diff --git a/src/corelib/thread/qyieldcpu.h b/src/corelib/thread/qyieldcpu.h
index 1234567..abcdefg 100644
--- a/src/corelib/thread/qyieldcpu.h
+++ b/src/corelib/thread/qyieldcpu.h
@@ -16,6 +16,10 @@
 #  endif
 void _mm_pause(void);       // the compiler recognizes as intrinsic
 #endif
+#if defined(Q_PROCESSOR_ARM_64) && defined(Q_CC_CLANG)
+#  include <arm_acle.h>
+#endif
+
 
 QT_BEGIN_NAMESPACE
```

#### 步骤 2：修改 Portfile

修改 `~/vcpkg/buildtrees/versioning_/versions/qtbase/89d2fd203b771f2cf96f527cfc5afe5c73bb4a87/portfile.cmake`：

在 `set(${PORT}_PATCHES` 列表开头添加：

```cmake
set(${PORT}_PATCHES
        fix-yield.patch
        allow_outside_prefix.patch
        ...
)
```

## 环境搭建步骤

### 1. 安装构建工具

```bash
# 安装 Homebrew（如果没有）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装构建工具
brew install cmake ninja pkg-config

# 安装 vcpkg 构建所需的 autotools
brew install autoconf autoconf-archive automake libtool
```

### 2. 安装 vcpkg

```bash
# 克隆 vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh

# 设置环境变量（添加到 ~/.zshrc 永久生效）
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.zshrc
source ~/.zshrc
```

### 3. 配置 vcpkg 镜像加速（可选）

```bash
# 使用清华大学镜像源加速下载
echo 'export X_VCPKG_ASSET_SOURCES="x-azurl,https://mirrors.tuna.tsinghua.edu.cn/vcpkg/assets/"' >> ~/.zshrc
source ~/.zshrc
```

### 4. 应用 Qt 补丁

按照上面"已知问题与解决方案"中的步骤应用 patch。

### 5. 构建项目

```bash
# 克隆项目（如果还没有）
git clone https://github.com/jsnjzxy/MHY_Scanner-macOS.git
cd MHY_Scanner-macOS

# 设置环境变量
export VCPKG_ROOT=~/vcpkg

# 方式一：使用 local-ci.sh（推荐，模拟 CI 流程）
./scripts/local-ci.sh arm64

# 方式二：使用 build_mac_vcpkg.sh
./scripts/build_mac_vcpkg.sh Release
```

### 6. 运行应用

```bash
# Release 版本
open Release-CI_build/bin/Release/MHY_Scanner.app

# 或使用 build_mac_vcpkg.sh 构建的版本
open Release_build/bin/Release/MHY_Scanner.app
```

## 常用命令

```bash
# Release 构建
./scripts/build_mac_vcpkg.sh Release

# Debug 构建（支持抓包调试）
./scripts/build_mac_vcpkg.sh Debug --dev

# 本地 CI 验证
./scripts/local-ci.sh arm64

# 运行应用
open Release_build/bin/Release/MHY_Scanner.app
```

## 故障排除

### 磁盘空间不足

vcpkg 编译 Qt 等大型库需要较多磁盘空间（约 10-20GB）。如果遇到 `No space left on device` 错误：

```bash
# 清理 vcpkg 构建缓存
rm -rf ~/vcpkg/buildtrees/*
rm -rf ~/vcpkg/packages/*
rm -rf ~/.cache/vcpkg/archives/*
```

### vcpkg 环境变量未设置

```bash
# 检查环境变量
echo $VCPKG_ROOT

# 如果为空，设置环境变量
export VCPKG_ROOT=~/vcpkg
```

### Qt 编译失败

确认已正确应用 patch：

```bash
# 检查 patch 文件是否存在
ls ~/vcpkg/buildtrees/versioning_/versions/qtbase/89d2fd203b771f2cf96f527cfc5afe5c73bb4a87/fix-yield.patch

# 检查 portfile.cmake 是否包含 fix-yield.patch
head -20 ~/vcpkg/buildtrees/versioning_/versions/qtbase/89d2fd203b771f2cf96f527cfc5afe5c73bb4a87/portfile.cmake
```

## 项目结构

```
MHY_Scanner-macOS/
├── src/
│   ├── Core/           # 核心库（网络、扫码、加密等）
│   └── UI/             # Qt 用户界面
├── scripts/
│   ├── build_mac_vcpkg.sh  # 构建脚本
│   └── local-ci.sh         # 本地 CI 验证脚本
├── vcpkg.json          # vcpkg 依赖配置
├── vcpkg-configuration.json  # vcpkg 版本配置
└── CMakePresets.json   # CMake 预设配置
```

## 相关文档

- [构建指南](BUILDING.md)
- [开发指南](DEVELOPMENT.md)
