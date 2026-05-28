# MHY_Scanner-macOS

米哈游游戏二维码自动登录器（macOS 版），支持从屏幕和直播流中识别二维码并自动登录。

## 技术栈

- **C++23**: 核心代码
- **Objective-C++**: macOS 平台特定功能（.mm 文件）
- **Qt 6**: GUI 框架
- **OpenCV**: 图像处理
- **FFmpeg**: 视频流处理
- **Vision**: macOS 原生二维码识别
- **ScreenCaptureKit**: macOS 屏幕捕获

## 构建系统

- **CMake 3.26+**: 构建系统
- **vcpkg**: 依赖管理（manifest 模式）
- **Ninja**: 构建工具

## 系统要求

- macOS 15.0 (Sequoia) 或更高版本
- Apple Silicon (M1/M2/M3/M4) 或 Intel Mac

## 快速构建

```bash
# 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# 构建
./scripts/build_mac_vcpkg.sh Release
```

详见 [docs/BUILDING.md](docs/BUILDING.md)

## 项目结构

```
src/
├── Core/           # 核心库（网络、扫码、加密等）
│   ├── Common/     # 公共定义、常量
│   ├── Network/    # HTTP 客户端、API 接口
│   ├── Scanner/    # 二维码扫描器（Objective-C++）
│   ├── SDK/        # 米哈游 SDK 封装
│   └── Utils/      # 工具类
├── UI/             # Qt 用户界面
│   ├── Main/       # 主窗口
│   ├── Login/      # 登录对话框
│   ├── Widgets/    # 自定义控件
│   └── ScanThreads/ # 扫码线程
└── Resources/      # 资源文件

3rdparty/
└── abogus_cpp/     # 抖音 a_bogus 签名算法
```

## 代码风格

- 4 空格缩进
- 类名：PascalCase（如 `UnifiedScanner`）
- 成员变量：m_ 前缀（如 `m_frameBuffer`）
- 函数：camelCase（如 `writeFrame`）

## 依赖

| 依赖 | 用途 |
|------|------|
| Qt 6 | GUI 框架 |
| OpenCV | 图像处理 |
| FFmpeg | 视频流处理 |
| cpr | HTTP 请求 |
| OpenSSL | 加密 |
| nlohmann-json | JSON 处理 |
| nayuki-qr-code-generator | QR 码生成 |

## 常用命令

```bash
# 构建（Release）
./scripts/build_mac_vcpkg.sh Release

# 构建（Debug，支持抓包调试）
./scripts/build_mac_vcpkg.sh Debug --dev

# 本地 CI 验证
./scripts/local-ci.sh arm64

# 运行
open Release_build/bin/Release/MHY_Scanner.app
```

## 相关文档

- [构建指南](docs/BUILDING.md)
- [开发指南](docs/DEVELOPMENT.md)
