#!/usr/bin/env node
/**
 * 抖音 a_bogus 签名 Node 入口
 * 供 MHY_Scanner C++ 通过子进程调用，仅依赖 Node.js + jsdom，不依赖 Python。
 *
 * 用法:
 *   node get_abogus_node.js                # 从 stdin 读取 JSON
 *   node get_abogus_node.js <json_path>    # 从文件读取 JSON（兼容旧方式）
 *
 * JSON 格式: { "params": "aid=6383&...", "user_agent": "Mozilla/5.0 ..." }
 * 输出: a_bogus 字符串到 stdout（单行），错误信息到 stderr
 */
const fs = require('fs');
const path = require('path');

// 将 a_bogus.js 内的 console 重定向到 stderr，保证 stdout 只输出结果
const origLog = console.log;
const origWarn = console.warn;
const origError = console.error;
console.log = (...a) => { origError.apply(console, a); };
console.warn = (...a) => { origError.apply(console, a); };
console.error = (...a) => { origError.apply(console, a); };

/**
 * 读取输入数据
 * @returns {{params: string, user_agent: string}}
 */
function readInput() {
    const inputPath = process.argv[2];

    if (inputPath) {
        // 从文件读取（兼容旧方式）
        try {
            const content = fs.readFileSync(inputPath, 'utf8');
            return JSON.parse(content);
        } catch (e) {
            console.error('ERROR: 无法读取或解析文件:', e.message);
            process.exit(1);
        }
    }

    // 从 stdin 读取
    return new Promise((resolve, reject) => {
        let data = '';
        process.stdin.setEncoding('utf8');

        process.stdin.on('data', chunk => {
            data += chunk;
        });

        process.stdin.on('end', () => {
            try {
                resolve(JSON.parse(data));
            } catch (e) {
                reject(new Error('无法解析 stdin JSON: ' + e.message));
            }
        });

        process.stdin.on('error', e => {
            reject(new Error('读取 stdin 失败: ' + e.message));
        });
    });
}

/**
 * 计算 a_bogus
 * @param {string} params
 * @param {string} userAgent
 * @returns {string}
 */
function computeAbogus(params, userAgent) {
    // 加载 a_bogus.js（会设置 window/document 并定义 get_a_bogus）
    require('./a_bogus.js');

    const get_a_bogus_fn = global.get_a_bogus;
    if (typeof get_a_bogus_fn !== 'function') {
        throw new Error('get_a_bogus 未定义');
    }

    const arg = [0, 1, 0, params || '', '', userAgent || ''];
    return get_a_bogus_fn(arg);
}

// 主逻辑
(async function main() {
    try {
        const input = await readInput();
        const params = input.params != null ? String(input.params) : '';
        const userAgent = input.user_agent != null ? String(input.user_agent) : '';

        if (!params) {
            console.error('ERROR: 缺少 params 字段');
            process.exit(1);
        }

        const result = computeAbogus(params, userAgent);

        if (result != null && result !== '') {
            // 仅输出结果到 stdout
            process.stdout.write(result + '\n');
        } else {
            console.error('ERROR: get_a_bogus 返回空');
            process.exit(1);
        }
    } catch (e) {
        console.error('ERROR:', e.message);
        process.exit(1);
    }
})();
