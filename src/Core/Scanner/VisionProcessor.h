#pragma once

#ifdef __APPLE__

#include <CoreVideo/CoreVideo.h>
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>

// 前向声明 Objective-C 类型
#ifdef __OBJC__
@class VNDetectBarcodesRequest;
@class CIContext;
#else
typedef struct objc_object VNDetectBarcodesRequest;
typedef struct objc_object CIContext;
#endif

/**
 * @brief Vision 处理器 - 复用 VNRequest 实现高性能二维码识别
 *
 * 特性：
 * - 全局复用 VNDetectBarcodesRequest（避免每帧创建）
 * - 复用 CIContext（避免重复创建 GPU 上下文）
 * - 零拷贝传递 CVPixelBuffer 到 Vision
 * - 异步处理 + 同步处理双模式
 */
class VisionProcessor
{
public:
    /**
     * @brief 结果回调函数类型
     * @param qrCode 识别到的二维码内容
     */
    using ResultCallback = std::function<void(const std::string& qrCode)>;

    VisionProcessor();
    ~VisionProcessor();

    // 禁止拷贝和移动
    VisionProcessor(const VisionProcessor&) = delete;
    VisionProcessor& operator=(const VisionProcessor&) = delete;
    VisionProcessor(VisionProcessor&&) = delete;
    VisionProcessor& operator=(VisionProcessor&&) = delete;

    /**
     * @brief 设置结果回调（异步处理时使用）
     */
    void setResultCallback(ResultCallback callback);

    /**
     * @brief 异步处理帧
     * @param pixelBuffer CVPixelBufferRef，函数内部不会修改引用计数
     * @note 结果通过 setResultCallback 设置的回调返回
     */
    void processFrame(CVPixelBufferRef pixelBuffer);

    /**
     * @brief 是否正在处理帧
     */
    bool isProcessing() const;

    /**
     * @brief 重置处理器（释放 Vision 内部缓存）
     */
    void reset();

    /**
     * @brief 设置最小置信度阈值
     * @param confidence 置信度值（0.0-1.0），默认 0.3
     */
    void setMinimumConfidence(float confidence);

    /**
     * @brief 获取处理统计信息
     */
    struct Stats
    {
        uint64_t totalProcessed{0};   // 总处理帧数
        uint64_t successCount{0};    // 成功识别数
        uint64_t lastProcessTimeUs{0}; // 上次处理时间（微秒）
    };
    Stats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // __APPLE__
