# MHY_Scanner-macOS
MHY扫码登录器（mac版本），支持从直播流抢码。

### **版本 - v1.1.14 (macOS 适配版)**

> 本项目是 [MHY_Scanner](https://github.com/Theresa-0328/MHY_Scanner) 的 macOS 适配版本，原项目为 Windows 平台开发。

## 功能和特性
- 从屏幕自动获取二维码登录，适用于大部分登录情景，不适用于在竞争激烈时抢码。
- 从直播流获取二维码登录，适用于抢码登录情景。
- 可选启动后自动开始识别屏幕和识别完成后自动退出，无需登录后手动切窗口关闭。
- 表格化管理多账号，方便切换游戏账号。

## 系统要求
- macOS 13.0 (Ventura) 或更高版本
- Apple Silicon (M1/M2/M3/M4) 或 Intel Mac

## 安装与使用

### 使用说明

首次打开可能需要在"系统设置 → 隐私与安全性"中点击"仍要打开"
首次运行时，macOS 会请求**屏幕录制权限**，请务必点击"允许"，否则无法正常识别屏幕二维码。

1. 点击菜单栏 **账号管理->添加账号**，添加你的账号。
2. 双击账号对应的备注单元格可以添加自定义备注。
3. 点击 **监视屏幕** 就可以自动识别任意显示在屏幕上的二维码并自动登录。
4. 选择你需要的直播平台，在当前直播间输入框输入 `RID`，点击 **监视直播间** 就可以自动识别该直播间显示的二维码并自动登录。
5. 正在执行的任务的按钮会高亮显示，再次点击会停止。

`RID` 是纯数字，一般从直播间链接中获得：

|                平台                |           `<RID>` 位置            |
| :--------------------------------: | :-------------------------------: |
|  [抖音](https://live.douyin.com/)  |  `https://live.douyin.com/<RID>`  |
| [B 站](https://live.bilibili.com/) | `https://live.bilibili.com/<RID>` |


目前没有进行大量测试，如果有任何建议和问题欢迎提 Issues。

## macOS 版本特性

| 特性 | Windows 版本 | macOS 版本 |
|:----:|:------------:|:----------:|
| 应用格式 | .exe | .app Bundle |
| Web 控件 | WebView2 | QWebEngineView |
| 屏幕截图 | DXGI | screencapture / CoreGraphics |
| 构建系统 | MSBuild + NuGet | CMake + Homebrew |
| 权限模型 | 无特殊权限 | 需屏幕录制权限 |

## 依赖
- Qt v6.8.0
- OpenCV v4.8.0
- FFmpeg v6.0
- curl v8.2.1
- OpenSSL v3.x
- CMake v3.20+

## 参考和感谢
- [Theresa-0328/MHY_Scanner](https://github.com/Theresa-0328/MHY_Scanner)
