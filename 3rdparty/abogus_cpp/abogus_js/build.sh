#!/bin/bash
# 构建 get_abogus 可执行文件
# 使用: ./build.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "==> 安装依赖..."
npm install

echo "==> 使用 esbuild 打包..."
npx esbuild get_abogus_node.js --bundle --platform=node --outfile=bundle.js --minify

echo "==> 使用 pkg 编译可执行文件..."
pkg bundle.js --targets node18-macos-arm64 --output get_abogus --compress Brotli

echo "==> 清理临时文件..."
rm -f bundle.js

echo "==> 完成!"
ls -lh get_abogus
