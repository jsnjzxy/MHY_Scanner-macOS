#include "VisionProcessor.h"

// VisionProcessor.mm 是 Objective-C++ 文件，只在 macOS 上编译
// 所以不需要 #ifdef __APPLE__ 保护

#import <Vision/Vision.h>
#import <CoreImage/CoreImage.h>
#import <dispatch/dispatch.h>

#include <iostream>
#include <chrono>

// ========================================
// VisionProcessor::Impl 实现
// ========================================
class VisionProcessor::Impl
{
public:
    Impl()
        : m_barcodeRequest(nullptr)
        , m_ciContext(nil)
        , m_isProcessing(false)
        , m_minimumConfidence(0.3f)
    {
        // 创建复用的 CIContext（高优先级，抢码场景）
        m_ciContext = [[CIContext alloc] initWithOptions:@{kCIContextPriorityRequestLow: @NO}];

        // 创建复用的 VNDetectBarcodesRequest
        // 注意：这里使用 C++ lambda 捕获 this 指针，直接调用 C++ 方法
        m_barcodeRequest = [[VNDetectBarcodesRequest alloc] initWithCompletionHandler:^(VNRequest *request, NSError *error) {
            this->handleResult(request, error);
        }];

        m_barcodeRequest.symbologies = @[VNBarcodeSymbologyQR];
        m_barcodeRequest.revision = VNDetectBarcodesRequestRevision4;  // macOS 15+ 支持最新版本

        // 创建专用串行队列（避免多线程竞争）
        m_visionQueue = dispatch_queue_create("vision.processor.queue", DISPATCH_QUEUE_SERIAL);
    }

    ~Impl()
    {
        if (m_barcodeRequest) {
            [m_barcodeRequest release];
        }
        if (m_ciContext) {
            [m_ciContext release];
        }
        if (m_visionQueue) {
            dispatch_release(m_visionQueue);
        }
    }

    void setResultCallback(ResultCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_callback = std::move(callback);
    }

    void processFrame(CVPixelBufferRef pixelBuffer)
    {
        if (!pixelBuffer) {
            return;
        }

        // 如果正在处理上一帧，跳过（抢码场景宁可丢帧也不堆积）
        if (m_isProcessing.load(std::memory_order_acquire)) {
            m_skippedFrames.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        m_isProcessing.store(true, std::memory_order_release);
        CFRetain(pixelBuffer);  // 异步处理前保留引用

        // 在 Vision 专用队列处理
        dispatch_async(m_visionQueue, ^{
            @autoreleasepool {
                auto startTime = std::chrono::high_resolution_clock::now();

                VNImageRequestHandler* handler = [[VNImageRequestHandler alloc]
                    initWithCVPixelBuffer:pixelBuffer
                    options:@{VNImageOptionCIContext: m_ciContext}];

                NSError* error = nil;
                BOOL success = [handler performRequests:@[m_barcodeRequest] error:&error];

                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                m_lastProcessTimeUs.store(duration.count(), std::memory_order_relaxed);
                m_totalProcessed.fetch_add(1, std::memory_order_relaxed);

                // 显式释放 handler，确保 Vision 内部资源释放
                [handler release];
                handler = nil;

                CFRelease(pixelBuffer);
                m_isProcessing.store(false, std::memory_order_release);
            }
        });
    }

    bool isProcessing() const
    {
        return m_isProcessing.load(std::memory_order_acquire);
    }

    void reset()
    {
        // 释放旧请求和上下文
        if (m_barcodeRequest) {
            [m_barcodeRequest release];
        }
        if (m_ciContext) {
            [m_ciContext release];
        }

        // 重新创建（释放 Vision 内部缓存）
        m_ciContext = [[CIContext alloc] initWithOptions:@{kCIContextPriorityRequestLow: @NO}];

        m_barcodeRequest = [[VNDetectBarcodesRequest alloc] initWithCompletionHandler:^(VNRequest *request, NSError *error) {
            this->handleResult(request, error);
        }];

        m_barcodeRequest.symbologies = @[VNBarcodeSymbologyQR];
        m_barcodeRequest.revision = VNDetectBarcodesRequestRevision4;

        std::cout << "[VisionProcessor] Reset completed" << std::endl;
    }

    void setMinimumConfidence(float confidence)
    {
        m_minimumConfidence = std::max(0.0f, std::min(1.0f, confidence));
    }

    VisionProcessor::Stats getStats() const
    {
        return VisionProcessor::Stats {
            m_totalProcessed.load(std::memory_order_relaxed),
            m_successCount.load(std::memory_order_relaxed),
            m_lastProcessTimeUs.load(std::memory_order_relaxed)
        };
    }

private:
    void handleResult(VNRequest* request, NSError* error)
    {
        if (error) {
            std::cerr << "[VisionProcessor] Error: " << [error.localizedDescription UTF8String] << std::endl;
            return;
        }

        if (request.results.count > 0) {
            for (VNBarcodeObservation* obs in request.results) {
                // 检查置信度阈值
                if (obs.confidence >= m_minimumConfidence && obs.payloadStringValue) {
                    std::string code = obs.payloadStringValue.UTF8String;

                    std::lock_guard<std::mutex> lock(m_callbackMutex);
                    if (m_callback) {
                        m_callback(code);
                    }
                    // 找到第一个满足条件的就返回
                    return;
                }
            }
        }
    }

    // Objective-C 成员
    VNDetectBarcodesRequest* m_barcodeRequest;
    CIContext* m_ciContext;
    dispatch_queue_t m_visionQueue;

    // 状态
    std::atomic<bool> m_isProcessing;
    float m_minimumConfidence;

    // 统计
    std::atomic<uint64_t> m_totalProcessed{0};
    std::atomic<uint64_t> m_successCount{0};
    std::atomic<uint64_t> m_lastProcessTimeUs{0};
    std::atomic<uint64_t> m_skippedFrames{0};

    // 回调
    ResultCallback m_callback;
    mutable std::mutex m_callbackMutex;
};

// ========================================
// VisionProcessor 公共接口实现
// ========================================
VisionProcessor::VisionProcessor()
    : m_impl(std::make_unique<Impl>())
{
    std::cout << "[VisionProcessor] Initialized with VNRequest reuse" << std::endl;
}

VisionProcessor::~VisionProcessor() = default;

void VisionProcessor::setResultCallback(ResultCallback callback)
{
    m_impl->setResultCallback(std::move(callback));
}

void VisionProcessor::processFrame(CVPixelBufferRef pixelBuffer)
{
    m_impl->processFrame(pixelBuffer);
}

bool VisionProcessor::isProcessing() const
{
    return m_impl->isProcessing();
}

void VisionProcessor::reset()
{
    m_impl->reset();
}

void VisionProcessor::setMinimumConfidence(float confidence)
{
    m_impl->setMinimumConfidence(confidence);
}

VisionProcessor::Stats VisionProcessor::getStats() const
{
    return m_impl->getStats();
}
