# 开发指南

本文档介绍 MHY_Scanner 项目的开发环境配置、项目结构和代码规范。

## 项目概述

MHY_Scanner 是米哈游系列游戏的二维码自动登录器，支持从屏幕和直播流中识别二维码并自动登录。

- **平台**: 仅支持 macOS 15.0+
- **构建系统**: CMake + vcpkg
- **链接方式**: 静态链接

## 项目结构

```
MHY_Scanner/
├── CMakeLists.txt                      # 主构建配置
├── vcpkg.json                          # vcpkg 依赖配置
├── vcpkg-configuration.json            # vcpkg 配置
├── Brewfile                            # macOS 构建工具
├── scripts/                            # 构建脚本
│   ├── build_mac_vcpkg.sh            # macOS 构建脚本
│   ├── package_dmg.sh                # DMG 打包脚本
│   └── check_deps.sh                 # 依赖检查脚本
├── src/
│   ├── Core/                           # 核心库
│   │   ├── Common/                    # 公共定义
│   │   │   ├── Types.h               # 枚举和类型定义
│   │   │   ├── Constants.h           # URL 常量
│   │   │   ├── Common.h              # 兼容层
│   │   │   └── salt.json             # 米哈游盐值参考文件
│   │   ├── Network/                   # 网络通信
│   │   │   ├── HttpClient.h/cpp       # HTTP 客户端
│   │   │   └── Api/                   # API 接口
│   │   │       ├── MihoyoApi.hpp      # 米哈游登录 API
│   │   │       ├── BilibiliApi.hpp    # B 站登录 API
│   │   │       └── LiveStreamApi.h/cpp # 直播流 API
│   │   ├── Utils/                     # 工具类
│   │   │   ├── StringUtil.hpp         # 字符串工具
│   │   │   ├── TimeUtil.hpp           # 时间工具
│   │   │   ├── UUIDUtil.hpp           # UUID 生成
│   │   │   ├── CookieParser.hpp       # Cookie 解析
│   │   │   ├── CryptoUtils.h/cpp      # 加密工具
│   │   │   └── UtilMat.hpp            # OpenCV 工具
│   │   ├── Scanner/                   # 扫码器（Objective-C++）
│   │   │   ├── QRScanner.h/mm        # 二维码识别
│   │   │   ├── UnifiedScanner.h/mm   # 统一扫码器
│   │   │   ├── UnifiedFrameBuffer.h/mm # 帧缓冲区
│   │   │   ├── VisionProcessor.h/mm   # Vision 处理器
│   │   │   ├── GPUCopyHelper.h/mm     # GPU 拷贝
│   │   │   └── ScreenCapture/         # 屏幕捕获
│   │   │       ├── ScreenCapture.h/mm
│   │   │       └── ScreenCaptureKitWrapper.h/mm
│   │   ├── SDK/                       # SDK 模块
│   │   │   └── MihoyoSDK.h/cpp        # 米哈游 SDK 封装
│   │   └── Config/                    # 配置模块
│   │       └── ConfigManager.h/cpp    # 配置管理
│   ├── UI/                             # 用户界面
│   │   ├── Main/                      # 主窗口
│   │   │   └── MainWindow.h/cpp       # 主窗口
│   │   ├── Login/                     # 登录界面
│   │   │   ├── LoginDialog.h/cpp      # 登录对话框
│   │   │   ├── LoginTab.cpp           # 登录 Tab 基类
│   │   │   ├── SmsLoginTab.h/cpp      # 短信登录 Tab
│   │   │   ├── QrCodeLoginTab.h/cpp   # 扫码登录 Tab
│   │   │   ├── CookieLoginTab.h/cpp   # Cookie 登录 Tab
│   │   │   └── BiliLoginTab.h/cpp     # B 站登录 Tab
│   │   ├── About/                     # 关于窗口
│   │   │   └── AboutDialog.h/cpp      # 关于窗口
│   │   ├── Widgets/                   # 自定义组件
│   │   │   ├── DraggableTableWidget.h # 可拖拽表格
│   │   │   └── StyleManager.h/cpp     # 样式管理器
│   │   ├── ScanThreads/               # 扫码线程
│   │   │   ├── ScreenScanThread.h/cpp # 屏幕扫码线程
│   │   │   └── StreamScanThread.h/mm  # 直播流扫码线程
│   │   └── Main.cpp                   # 应用入口
│   └── Resources/                     # 资源文件
├── docs/                               # 文档目录
│   ├── BUILDING.md                    # 构建指南
│   ├── DEVELOPMENT.md                 # 开发指南
│   └── images/                        # 文档图片
└── 3rdparty/                           # 第三方库
    └── abogus_cpp/                     # 抖音 a_bogus 算法
        ├── abogus.cpp                  # C++ 实现
        ├── abogus.h                    # 头文件
        └── abogus_js/                  # Node.js 实现
            └── get_abogus              # 可执行文件
```

## 技术栈

### 核心框架
- **Qt 6**: GUI 框架
  - Core: 核心功能
  - Widgets: 界面组件
  - Gui: 图形界面
  - Multimedia: 多媒体处理
  - MultimediaWidgets: 多媒体界面组件

### 图像和视频处理
- **OpenCV**: 图像处理 (core, imgproc, imgcodecs, highgui)
- **FFmpeg**: 视频流处理 (avcodec, avformat, swscale 等)
- **Vision (macOS)**: 视觉识别（二维码扫描）

### 网络和加密
- **cURL**: HTTP 请求处理
- **OpenSSL**: 加密和安全通信

### 其他依赖
- **nlohmann-json**: JSON 数据处理
- **nayuki-qr-code-generator**: QR 码生成
- **GTest**: 单元测试框架（可选）

### macOS 系统框架
- **Vision**: 视觉识别
- **CoreVideo**: 视频处理
- **ScreenCaptureKit**: 屏幕捕获（macOS 12.3+）
- **CoreGraphics**: 图形绘制
- **AVFoundation**: 音视频框架

### 本地第三方库
- **abogus_cpp**: 抖音 a_bogus 算法实现
  - abogus.cpp: C++ 实现
  - abogus_js/get_abogus: Node.js 可执行文件

## vcpkg 配置

### vcpkg.json

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
    { "name": "opencv" },
    { "name": "ffmpeg", "features": ["avcodec", "avdevice", "avfilter", "avformat", "swresample", "swscale"] },
    { "name": "curl", "features": ["ssl"] },
    { "name": "openssl" },
    { "name": "gtest" },
    { "name": "nlohmann-json" }
  ]
}
```

### 镜像配置

```bash
export X_VCPKG_ASSET_SOURCES="x-azurl,https://mirrors.tuna.tsinghua.edu.cn/vcpkg/assets/"
```

## salt.json 说明

`src/Core/Common/salt.json` 是米哈游登录加密盐值的参考文件，存储了各历史版本的加密盐值。

**注意**: 当前代码中盐值直接硬编码在 `MihoyoApi.hpp` 中，此文件仅作为参考和历史记录。

## 代码风格

### C++ 标准
- 使用 C++23 标准
- Objective-C++ 用于 macOS 平台特定功能（.mm 文件）

### 命名规范
- **Qt 类**: Q 前缀（如 `QMainWindow`）
- **项目类**: 驼峰命名（如 `UnifiedScanner`）
- **成员变量**: m_ 前缀（如 `m_frameBuffer`）
- **函数**: 驼峰命名（如 `writeFrame`）

### 缩进和格式
- 4 空格缩进
- 不使用制表符

## 开发流程

1. Fork 项目
2. 创建特性分支：`git checkout -b feature/your-feature`
3. 提交更改：`git commit -m "feat: add feature"`
4. 推送到分支：`git push origin feature/your-feature`
5. 创建 Pull Request

### 提交信息规范

```
feat: 添加新功能
fix: 修复 bug
docs: 更新文档
style: 代码格式调整
refactor: 重构代码
chore: 构建/工具链相关
```

## 构建和测试

### 构建

```bash
./scripts/build_mac_vcpkg.sh Release
```

详见 [构建文档](BUILDING.md)。

### 运行

```bash
open Release_build/bin/Release/MHY_Scanner.app
```

## 贡献指南

欢迎提交 Issue 和 Pull Request！

- 提交 Issue 前请先搜索已有问题
- Pull Request 请保持代码风格一致
- 大改动请先在 Issue 中讨论
