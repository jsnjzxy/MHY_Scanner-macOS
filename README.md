# MHY_Scanner-macOS

MHY扫码登录器（mac版本），支持从直播流抢码。

<img width="250" height="314" alt="image" src="https://github.com/user-attachments/assets/8ba6965f-9ab6-4ec0-960b-848621ec2ea0" />


### **版本 - v1.1.15**

## 重要提醒

最近在闲鱼等地发现有人在卖这个项目，希望没有人真的傻逼到花钱买免费的代码。如果你是购买来的，说明你被骗了，建议联系商家退款。最后，如果你真的想卖本项目赚钱，<a href="https://www.baidu.com/s?wd=%E5%AD%A4%E5%84%BF%E6%80%8E%E4%B9%88%E5%8A%9E%E6%88%B7%E5%8F%A3%E6%9C%AC">请点这里了解</a>

## 功能和特性

- 从屏幕自动获取二维码登录，适用于大部分登录情景，不适用于在竞争激烈时抢码
- 从直播流获取二维码登录，适用于抢码登录情景
- 可选启动后自动开始识别屏幕和识别完成后自动退出，无需登录后手动切窗口关闭
- 表格化管理多账号，方便切换游戏账号

## 系统要求

- **macOS 15.0** (Sequoia) 或更高版本
- Apple Silicon (M1/M2/M3/M4) 或 Intel Mac

## 安装与使用

### 下载预编译版本

[点击 Releases](https://github.com/jsnjzxy/MHY_Scanner-macOS/releases)选择最新版本下载解压

### 首次运行

首次打开可能需要在"系统设置 → 隐私与安全性"中点击"仍要打开"

首次运行时，macOS 会请求**屏幕录制权限**，请务必点击"允许"，否则无法正常识别屏幕二维码。

### 使用说明

1. 点击菜单栏 **账号管理 -> 添加账号**，添加你的账号
2. 双击账号对应的备注单元格可以添加自定义备注
3. 点击 **监视屏幕** 就可以自动识别任意显示在屏幕上的二维码并自动登录
4. 选择你需要的直播平台，在当前直播间输入框输入 `RID`，点击 **监视直播间** 就可以自动识别该直播间显示的二维码并自动登录
5. 正在执行的任务的按钮会高亮显示，再次点击会停止

`RID` 是纯数字，一般从直播间链接中获得：

|                平台                |           `<RID>` 位置            |
| :--------------------------------: | :-------------------------------: |
|  [抖音](https://live.douyin.com/)  |  `https://live.douyin.com/<RID>`  |
|  [B 站](https://live.bilibili.com/) | `https://live.bilibili.com/<RID>` |

目前没有进行大量测试，如果有任何建议和问题欢迎提 Issues。

## 构建项目

```bash
./scripts/build_mac_vcpkg.sh Release
```

构建完成后，应用位于 `Release_build/bin/Release/MHY_Scanner.app`

详见 [完整构建文档](docs/BUILDING.md)

## DMG 打包

```bash
./scripts/package_dmg.sh Release
```

## 抖音直播间环境（可选）

使用抖音直播间功能前，需安装 Node.js：

```bash
brew install node
```

详见 [构建文档](docs/BUILDING.md#抖音直播间环境可选)

## 技术栈

- **C++23**: 核心代码
- **Qt 6**: GUI 框架
- **OpenCV**: 图像处理和二维码识别
- **FFmpeg**: 视频流处理
- **Vision (macOS)**: 原生视觉识别框架
- **ScreenCaptureKit (macOS)**: 屏幕捕获

## 主要依赖

| 依赖 | 版本 |
|------|------|
| Qt 6 | 6.10+ |
| OpenCV | 4.12+ |
| FFmpeg | 6.0+ |
| cURL | 8.2+ |
| OpenSSL | 3.6+ |

## 本地第三方库

- **abogus_cpp**: 抖音 a_bogus 签名算法实现

## 参考和感谢

- [Theresa-0328/MHY_Scanner](https://github.com/Theresa-0328/MHY_Scanner)
