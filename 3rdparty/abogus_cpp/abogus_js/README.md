# 抖音 a_bogus 签名模块

从 get_a_bogus 项目复制的算法，由 abogus.cpp 通过 Node 子进程调用，**仅依赖 Node.js**。

## 环境安装

```bash
# 1. 安装 Node.js
brew install node   # macOS
# 或从 https://nodejs.org/ 安装

# 2. 在本目录安装依赖（构建前或首次运行前执行一次）
cd 3rdparty/abogus_cpp/abogus_js
npm install
```

构建时 CMake 会将该目录复制到可执行文件旁；若复制后未包含 `node_modules`，需在**复制后的目录**再执行一次 `npm install`（例如在 `MHY_Scanner.app/Contents/MacOS/abogus_cpp/abogus_js` 下）。

## 文件说明

| 文件 | 说明 |
|------|------|
| `a_bogus.js` | 核心 a_bogus 算法实现（约 580KB，混淆代码），使用 linkedom |
| `get_abogus_node.js` | Node 入口脚本，接收 JSON 输入，输出签名 |
| `get_abogus` | 预编译的可执行文件（macOS arm64） |
| `build.sh` | 重新编译可执行文件的脚本 |
| `package.json` | npm 依赖配置，使用 linkedom |
| `package-lock.json` | npm 依赖锁定文件 |

## 运行方式

- 从终端或双击运行均可，只要系统 PATH 中能找到 `node` 即可。
- 若提示「Node 脚本执行失败」，请确认已安装 Node.js 且在 abogus_js 目录执行过 `npm install`。

## 重新编译可执行文件

如果需要重新编译 `get_abogus` 可执行文件：

```bash
cd 3rdparty/abogus_cpp/abogus_js
./build.sh
```

这将使用 pkg 打包生成独立的 macOS 可执行文件。

## 环境变量

| 变量 | 说明 |
|------|------|
| `MHY_ABOGUS_JS_PATH` | 指定 abogus_js 目录路径（优先） |
| `MHY_ABOGUS_PY_PATH` | 兼容旧变量名 |
| `MHY_NODE_PATH` | 指定 node 可执行文件路径 |

## 依赖说明

- **linkedom**: 轻量级 DOM 解析库，替代 jsdom
- **pkg**: 用于打包 Node.js 脚本为独立可执行文件
- **esbuild**: 用于打包和压缩
