#pragma once

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 生成抖音 a_bogus 签名
 *
 * 通过内部 Node.js 脚本计算签名，调用者需确保：
 * 1. 已安装 Node.js
 * 2. abogus_py 目录已正确部署并执行 npm install
 *
 * @param userAgent HTTP 请求的 User-Agent，必须与实际请求一致
 * @param params URL 查询参数字符串（如 "aid=6383&web_rid=xxx"）
 * @return 签名字符串指针，需调用 free_abogus 释放；失败返回 nullptr
 *
 * 线程安全：是（macOS 上会自动派发到主线程执行）
 */
char* get_abogus(const char* userAgent, const char* params);

/**
 * 释放 get_abogus 返回的字符串
 * @param ptr get_abogus 返回的指针
 */
void free_abogus(char* ptr);

#ifdef __cplusplus
}
#endif
