#pragma once

#ifdef __APPLE__

#include <CoreVideo/CoreVideo.h>
#include <memory>

// 前向声明 Objective-C 类型
#ifdef __OBJC__
@class CIContext;
#else
typedef struct objc_object CIContext;
#endif

/**
 * @brief GPU → GPU 拷贝工具类（Pimpl 模式）
 *
 * 功能：
 * - 使用 CoreImage 在 GPU 上复制 CVPixelBuffer
 * - 断开与 VideoToolbox/VTDecoder 的关联
 * - 解决 VTDecoderXPCService 内存泄漏问题
 */
class GPUCopyHelper {
public:
    GPUCopyHelper();
    ~GPUCopyHelper();

    // 禁止拷贝和移动
    GPUCopyHelper(const GPUCopyHelper&) = delete;
    GPUCopyHelper& operator=(const GPUCopyHelper&) = delete;

    /**
     * @brief 初始化
     * @return true 成功
     */
    bool init();

    /**
     * @brief 复制 CVPixelBuffer（GPU → GPU）
     * @param source 源 CVPixelBuffer
     * @return 新的 CVPixelBuffer（需要调用者释放）
     */
    CVPixelBufferRef copyPixelBuffer(CVPixelBufferRef source);

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

private:
    // Pimpl 模式：隐藏 Objective-C 实现细节
    class Impl;
    Impl* m_impl;
    bool m_initialized = false;
};

#endif // __APPLE__
