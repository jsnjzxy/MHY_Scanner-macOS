#pragma once

#ifdef __APPLE__

#include "UnifiedFrameBuffer.h"
#include "VisionProcessor.h"
#include <QObject>
#include <QThread>
#include <QString>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

// 前向声明
class ScreenCaptureKitWrapper;

/**
 * @brief 统一扫码器 - 整合录屏和直播流双源
 *
 * 架构特点：
 * - 统一帧缓冲区接收两个源的帧
 * - 单一 VisionProcessor 处理所有帧
 * - 可配置源优先级
 * - 自动内存管理和重置
 */
class UnifiedScanner : public QObject
{
    Q_OBJECT

public:
    explicit UnifiedScanner(QObject* parent = nullptr);
    ~UnifiedScanner() override;

    // 禁止拷贝和移动
    UnifiedScanner(const UnifiedScanner&) = delete;
    UnifiedScanner& operator=(const UnifiedScanner&) = delete;
    UnifiedScanner(UnifiedScanner&&) = delete;
    UnifiedScanner& operator=(UnifiedScanner&&) = delete;

    /**
     * @brief 配置录屏源
     * @param enable 是否启用
     * @param displayID 显示器 ID，默认主显示器
     */
    void enableScreenSource(bool enable, CGDirectDisplayID displayID = kCGNullDirectDisplay);

    /**
     * @brief 配置直播流源
     * @param url 流地址
     * @param headers HTTP 头
     */
    void enableStreamSource(const std::string& url, const std::map<std::string, std::string>& headers = {});

    /**
     * @brief 禁用直播流源
     */
    void disableStreamSource();

    /**
     * @brief 开始扫描
     */
    void start();

    /**
     * @brief 停止扫描
     */
    void stop();

    /**
     * @brief 设置连续扫码模式
     * @param enabled true：识别到码后继续扫描；false：识别到码后停止
     */
    void setContinuousScan(bool enabled);

    /**
     * @brief 设置源优先级
     * @param streamFirst true：优先直播流；false：优先录屏
     */
    void setStreamPriority(bool streamFirst);

    /**
     * @brief 重置（释放缓存）
     */
    void reset();

    /**
     * @brief 设置最小置信度
     */
    void setMinimumConfidence(float confidence);

    /**
     * @brief 统计信息
     */
    struct ScannerStats
    {
        uint64_t screenFrames{0};      // 录屏帧数
        uint64_t streamFrames{0};      // 直播流帧数
        uint64_t processedFrames{0};   // 已处理帧数
        uint64_t codesDetected{0};     // 识别到的码数量
        uint64_t framesOverwritten{0}; // 被覆盖的帧数
        uint64_t visionTimeUs{0};      // Vision 处理时间（微秒）
        uint64_t memoryMB{0};          // 内存占用（MB）
    };
    ScannerStats getStats() const;

    /**
     * @brief 从 FFmpeg 回调中推送帧（由 QRCodeForStream 调用）
     */
    void pushStreamFrame(CVPixelBufferRef frame, int width, int height);

    /**
     * @brief 检查是否正在运行
     */
    bool isRunning() const { return m_running.load(); }

Q_SIGNALS:
    /**
     * @brief 识别到二维码信号
     * @param code 二维码内容
     * @param source 来源：1=录屏，2=直播流
     */
    void codeDetected(const QString& code, int source);

    /**
     * @brief 统计信息更新信号
     */
    void statsUpdated(const ScannerStats& stats);

    /**
     * @brief 错误信号
     */
    void errorOccurred(const QString& error);

private:
    void processNextFrame();
    void handleQRCodeDetected(const std::string& code, FrameSource source);

    // 帧源
    std::unique_ptr<ScreenCaptureKitWrapper> m_screenCapture;
    CGDirectDisplayID m_displayID{kCGNullDirectDisplay};
    std::string m_streamUrl;
    std::map<std::string, std::string> m_streamHeaders;
    std::atomic<bool> m_screenEnabled{false};
    std::atomic<bool> m_streamEnabled{false};

    // 处理管道
    std::unique_ptr<UnifiedFrameBuffer> m_frameBuffer;
    std::unique_ptr<VisionProcessor> m_visionProcessor;

    // 处理线程
    std::thread m_processThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_continuousScan{false};
    std::atomic<bool> m_streamPriority{true};  // 默认直播流优先

    // 统计
    mutable std::atomic<uint64_t> m_codesDetected{0};
    std::atomic<uint64_t> m_processedFrames{0};
    std::atomic<uint64_t> m_streamFramesReceived{0};  // 接收到的流帧数
    std::string m_lastDetectedCode;  // 避免重复上报同一码

    // 同步
    mutable std::mutex m_statsMutex;
};

#endif // __APPLE__
