#include "UnifiedScanner.h"
#include "ScreenCapture/ScreenCaptureKitWrapper.h"
#include <iostream>
#include <chrono>
#include <thread>

// UnifiedScanner.mm 是 Objective-C++ 文件，只在 macOS 上编译
// 所以不需要 #ifdef __APPLE__ 保护

#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#import <mach/mach.h>

// ========================================
// UnifiedScanner 实现
// ========================================
UnifiedScanner::UnifiedScanner(QObject* parent)
    : QObject(parent)
    , m_frameBuffer(std::make_unique<UnifiedFrameBuffer>())
    , m_visionProcessor(std::make_unique<VisionProcessor>())
    , m_displayID(CGMainDisplayID())
{
    // 设置 Vision 处理器回调
    m_visionProcessor->setResultCallback([this](const std::string& code) {
        m_codesDetected.fetch_add(1, std::memory_order_relaxed);

        // 获取当前优先级来确定来源（简化处理）
        FrameSource source = m_streamPriority.load() ? FrameSource::Stream : FrameSource::Screen;
        handleQRCodeDetected(code, source);
    });

    // 降低置信度阈值提高识别率（抢码场景）
    m_visionProcessor->setMinimumConfidence(0.25f);

    std::cout << "[UnifiedScanner] Initialized with Stream Priority" << std::endl;
}

UnifiedScanner::~UnifiedScanner()
{
    stop();
}

void UnifiedScanner::enableScreenSource(bool enable, CGDirectDisplayID displayID)
{
    if (displayID != kCGNullDirectDisplay) {
        m_displayID = displayID;
    }

    m_screenEnabled.store(enable);

    if (enable) {
        if (!m_screenCapture) {
            m_screenCapture = std::make_unique<ScreenCaptureKitWrapper>();
        }

        if (m_screenCapture->init(m_displayID)) {
            std::cout << "[UnifiedScanner] ScreenCaptureKit enabled on display " << m_displayID << std::endl;
        } else {
            Q_EMIT errorOccurred(QString::fromStdString("Failed to enable ScreenCaptureKit"));
            m_screenEnabled.store(false);
        }
    } else {
        if (m_screenCapture) {
            m_screenCapture->stop();
            std::cout << "[UnifiedScanner] ScreenCaptureKit disabled" << std::endl;
        }
    }
}

void UnifiedScanner::enableStreamSource(const std::string& url, const std::map<std::string, std::string>& headers)
{
    m_streamUrl = url;
    m_streamHeaders = headers;
    m_streamEnabled.store(true);

    std::cout << "[UnifiedScanner] Stream source enabled: "
              << url.substr(0, std::min<size_t>(50, url.length())) << "..." << std::endl;
}

void UnifiedScanner::disableStreamSource()
{
    m_streamEnabled.store(false);
    m_streamUrl.clear();
    m_streamHeaders.clear();

    std::cout << "[UnifiedScanner] Stream source disabled" << std::endl;
}

void UnifiedScanner::start()
{
    if (m_running.load()) {
        std::cout << "[UnifiedScanner] Already running" << std::endl;
        return;
    }

    m_running.store(true);

    // 启动处理线程
    m_processThread = std::thread([this]() {
        std::cout << "[UnifiedScanner] Process thread started" << std::endl;

        while (m_running.load()) {
            processNextFrame();

            // 控制处理频率，避免空转 CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(8));  // ~120fps check
        }

        std::cout << "[UnifiedScanner] Process thread stopped" << std::endl;
    });

    // 启用配置的源
    if (m_screenEnabled.load() && !m_screenCapture) {
        enableScreenSource(true, m_displayID);
    }

    std::cout << "[UnifiedScanner] Started" << std::endl;
}

void UnifiedScanner::stop()
{
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);

    if (m_processThread.joinable()) {
        m_processThread.join();
    }

    if (m_screenCapture) {
        m_screenCapture->stop();
    }

    m_frameBuffer->clear();
    m_lastDetectedCode.clear();

    std::cout << "[UnifiedScanner] Stopped" << std::endl;
}

void UnifiedScanner::setContinuousScan(bool enabled)
{
    m_continuousScan.store(enabled);
    std::cout << "[UnifiedScanner] Continuous scan: " << (enabled ? "enabled" : "disabled") << std::endl;
}

void UnifiedScanner::setStreamPriority(bool streamFirst)
{
    m_streamPriority.store(streamFirst);
    std::cout << "[UnifiedScanner] Priority: "
              << (streamFirst ? "Stream First" : "Screen First") << std::endl;
}

void UnifiedScanner::reset()
{
    std::cout << "[UnifiedScanner] Resetting..." << std::endl;
    m_frameBuffer->clear();
    m_visionProcessor->reset();
    m_lastDetectedCode.clear();
    m_codesDetected.store(0, std::memory_order_relaxed);
}

void UnifiedScanner::setMinimumConfidence(float confidence)
{
    m_visionProcessor->setMinimumConfidence(confidence);
    std::cout << "[UnifiedScanner] Minimum confidence: " << confidence << std::endl;
}

UnifiedScanner::ScannerStats UnifiedScanner::getStats() const
{
    ScannerStats stats;

    auto bufferStats = m_frameBuffer->getStats();
    auto visionStats = m_visionProcessor->getStats();

    stats.screenFrames = bufferStats.screenFrames;
    stats.streamFrames = bufferStats.streamFrames;
    stats.framesOverwritten = bufferStats.overwritten;
    stats.processedFrames = m_processedFrames.load(std::memory_order_relaxed);
    stats.codesDetected = m_codesDetected.load(std::memory_order_relaxed);
    stats.visionTimeUs = visionStats.lastProcessTimeUs;

    // 获取内存信息
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        stats.memoryMB = info.resident_size / (1024 * 1024);
    }

    return stats;
}

void UnifiedScanner::pushStreamFrame(CVPixelBufferRef frame, int width, int height)
{
    if (!frame || !m_running.load() || !m_streamEnabled.load()) {
        return;
    }

    // 每 60 帧输出一次日志（与 Screen 格式一致）
    static int streamLogCounter = 0;
    if (++streamLogCounter >= 60) {
        auto stats = m_frameBuffer->getStats();
        std::cout << "[Stream] Frame: " << stats.streamFrames
                  << ", Size: " << width << "x" << height
                  << ", DataSize: " << (CVPixelBufferGetDataSize(frame) / 1024 / 1024) << "MB"
                  << ", buffer: " << stats.validCount << ", processed: " << m_processedFrames.load(std::memory_order_relaxed) << std::endl;
        streamLogCounter = 0;
    }

    // 直接写入缓冲区
    m_frameBuffer->writeFrame(frame, FrameSource::Stream, width, height);
    m_streamFramesReceived.fetch_add(1, std::memory_order_relaxed);
}

void UnifiedScanner::processNextFrame()
{
    // 1. 从 ScreenCaptureKit 获取帧
    if (m_screenEnabled.load() && m_screenCapture && m_screenCapture->hasFrame()) {
        CVPixelBufferRef frame = m_screenCapture->getLatestFrame();
        if (frame) {
            int width = static_cast<int>(CVPixelBufferGetWidth(frame));
            int height = static_cast<int>(CVPixelBufferGetHeight(frame));
            m_frameBuffer->writeFrame(frame, FrameSource::Screen, width, height);
            CVPixelBufferRelease(frame);

            // 每 60 帧输出一次日志（与 Stream 格式一致）
            static int screenLogCounter = 0;
            if (++screenLogCounter >= 60) {
                auto stats = m_frameBuffer->getStats();
                std::cout << "[Screen] Frame: " << stats.screenFrames
                          << ", Size: " << width << "x" << height
                          << ", DataSize: " << (CVPixelBufferGetDataSize(frame) / 1024 / 1024) << "MB"
                          << ", buffer: " << stats.validCount << ", processed: " << m_processedFrames.load(std::memory_order_relaxed) << std::endl;
                screenLogCounter = 0;
            }
        }
    }

    // 2. 如果 Vision 正在处理上一帧，跳过
    if (m_visionProcessor->isProcessing()) {
        return;
    }

    // 3. 读取最新帧处理
    FrameMetadata meta;
    FrameSource priority = m_streamPriority.load() ? FrameSource::Stream : FrameSource::Screen;
    CVPixelBufferRef frame = m_frameBuffer->readLatestFrame(meta, priority);

    if (frame) {
        m_processedFrames.fetch_add(1, std::memory_order_relaxed);
        m_visionProcessor->processFrame(frame);
        m_frameBuffer->releaseFrame(frame);
    }
}

void UnifiedScanner::handleQRCodeDetected(const std::string& code, FrameSource source)
{
    // 避免重复上报同一码（短时间内）
    static CFTimeInterval lastReportTime = 0;
    CFTimeInterval currentTime = CACurrentMediaTime() * 1000.0;

    if (code == m_lastDetectedCode && (currentTime - lastReportTime) < 1000.0) {
        return;  // 同一码 1 秒内只报一次
    }

    m_lastDetectedCode = code;
    lastReportTime = currentTime;

    std::cout << "[" << (source == FrameSource::Stream ? "Stream" : "Screen") << "] Detected: "
              << code.substr(0, std::min<size_t>(50, code.length())) << "..." << std::endl;

    Q_EMIT codeDetected(QString::fromStdString(code), static_cast<int>(source));

    // 非连续模式下，停止扫描
    if (!m_continuousScan.load()) {
        stop();
    }
}
