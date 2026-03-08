// QRScanner.mm - Apple Vision 框架实现
// 此文件使用 Objective-C++ 编译 (.mm)

#include "QRScanner.h"

#ifdef __APPLE__
#import <Vision/Vision.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreImage/CoreImage.h>

#include <mutex>
#include <iostream>
#include <vector>

class QRScanner::Impl
{
public:
    Impl() : m_reusedPixelBuffer(nullptr)
    {
        std::cout << "[QRScanner] Using Apple Vision framework (GPU/Neural Engine)" << std::endl;
    }

    ~Impl()
    {
        // 清理复用的 pixelBuffer
        if (m_reusedPixelBuffer)
        {
            CVPixelBufferRelease(m_reusedPixelBuffer);
        }
    }

    // 优化版本：复用 CVPixelBuffer，避免频繁分配内存
    bool detectQRCodeFromBGRA(const uint8_t* bgraData, int width, int height, int stride, std::string& qrCode)
    {
        // 检查输入数据有效性
        if (bgraData == nullptr || width <= 0 || height <= 0 || stride <= 0)
        {
            return false;
        }

        // 优化：对于大图像，先降采样减少处理量（Vision 框架会创建大缓冲区）
        const int MAX_WIDTH = 1920;
        int processWidth = width;
        int processHeight = height;
        int processStride = stride;

        std::vector<uint8_t> downsampledBuffer;

        if (width > MAX_WIDTH)
        {
            float scale = static_cast<float>(MAX_WIDTH) / width;
            processWidth = MAX_WIDTH;
            processHeight = static_cast<int>(height * scale);
            processStride = processWidth * 4;  // BGRA 格式

            downsampledBuffer.resize(processHeight * processStride);

            // 简单的降采样（取每个块的第一个像素）
            float xScale = static_cast<float>(width) / processWidth;
            float yScale = static_cast<float>(height) / processHeight;

            for (int y = 0; y < processHeight; y++)
            {
                int srcY = static_cast<int>(y * yScale);
                for (int x = 0; x < processWidth; x++)
                {
                    int srcX = static_cast<int>(x * xScale);
                    downsampledBuffer[y * processStride + x * 4 + 0] = bgraData[srcY * stride + srcX * 4 + 0]; // B
                    downsampledBuffer[y * processStride + x * 4 + 1] = bgraData[srcY * stride + srcX * 4 + 1]; // G
                    downsampledBuffer[y * processStride + x * 4 + 2] = bgraData[srcY * stride + srcX * 4 + 2]; // R
                    downsampledBuffer[y * processStride + x * 4 + 3] = bgraData[srcY * stride + srcX * 4 + 3]; // A
                }
            }

            bgraData = downsampledBuffer.data();
        }

        @autoreleasepool
        {
            CVPixelBufferRef pixelBuffer = nullptr;

            // 如果尺寸匹配，复用之前的 CVPixelBuffer
            if (m_reusedPixelBuffer)
            {
                size_t reusedWidth = CVPixelBufferGetWidth(m_reusedPixelBuffer);
                size_t reusedHeight = CVPixelBufferGetHeight(m_reusedPixelBuffer);
                if (reusedWidth == static_cast<size_t>(processWidth) && reusedHeight == static_cast<size_t>(processHeight))
                {
                    // 复用 pixelBuffer，只需要复制数据
                    pixelBuffer = m_reusedPixelBuffer;
                    CFRetain(pixelBuffer); // 增加引用计数，因为后面会 release
                }
                else
                {
                    // 尺寸不匹配，释放旧的
                    CVPixelBufferRelease(m_reusedPixelBuffer);
                    m_reusedPixelBuffer = nullptr;
                }
            }

            // 需要创建新的 CVPixelBuffer
            if (pixelBuffer == nullptr)
            {
                CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(
                    kCFAllocatorDefault, 4,
                    &kCFTypeDictionaryKeyCallBacks,
                    &kCFTypeDictionaryValueCallBacks);

                SInt32 pixelFormat = kCVPixelFormatType_32BGRA;
                CFNumberRef pixelFormatRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pixelFormat);
                CFDictionarySetValue(attributes, kCVPixelBufferPixelFormatTypeKey, pixelFormatRef);
                CFRelease(pixelFormatRef);

                CFDictionarySetValue(attributes, kCVPixelBufferCGImageCompatibilityKey, kCFBooleanTrue);
                CFDictionarySetValue(attributes, kCVPixelBufferCGBitmapContextCompatibilityKey, kCFBooleanTrue);

                CVReturn status = CVPixelBufferCreate(
                    kCFAllocatorDefault, processWidth, processHeight,
                    kCVPixelFormatType_32BGRA, attributes, &pixelBuffer);

                CFRelease(attributes);

                if (status != kCVReturnSuccess || pixelBuffer == nullptr)
                {
                    return false;
                }

                // 保存用于下次复用
                m_reusedPixelBuffer = pixelBuffer;
                CFRetain(pixelBuffer); // 增加引用计数
            }

            // 复制数据到 pixelBuffer
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
            size_t bufferBytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

            // 如果 processStride 与 bufferBytesPerRow 相同，直接复制
            if (processStride == static_cast<int>(bufferBytesPerRow))
            {
                memcpy(bufferData, bgraData, processHeight * processStride);
            }
            else
            {
                // 逐行复制
                for (int y = 0; y < processHeight; y++)
                {
                    memcpy(
                        static_cast<uint8_t*>(bufferData) + y * bufferBytesPerRow,
                        bgraData + y * processStride,
                        std::min(processStride, static_cast<int>(bufferBytesPerRow)));
                }
            }

            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

            // 使用 VNImageRequestHandler 直接从 CVPixelBuffer 识别
            VNImageRequestHandler* handler = [[VNImageRequestHandler alloc]
                initWithCVPixelBuffer:pixelBuffer options:@{}];

            VNDetectBarcodesRequest* request = [[VNDetectBarcodesRequest alloc] init];
            request.symbologies = @[VNBarcodeSymbologyQR];

            NSError* error = nil;
            auto startTime = std::chrono::high_resolution_clock::now();
            BOOL success = [handler performRequests:@[request] error:&error];
            auto endTime = std::chrono::high_resolution_clock::now();

            // 释放 pixelBuffer（但 m_reusedPixelBuffer 仍然保留）
            CVPixelBufferRelease(pixelBuffer);

            if (!success || request.results == nil || request.results.count == 0)
            {
                qrCode.clear();
                // 输出未检测到的时间
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                std::cout << "[Vision] " << static_cast<float>(duration) / 1000000
                          << "s decode: (empty)" << std::endl;
                return false;
            }

            VNBarcodeObservation* obs = request.results.firstObject;
            if (obs.payloadStringValue != nil)
            {
                qrCode = obs.payloadStringValue.UTF8String;
                // 输出检测到的时间
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                std::cout << "[Vision] " << static_cast<float>(duration) / 1000000
                          << "s decode: " << qrCode.substr(0, std::min<size_t>(50, qrCode.length())) << "..." << std::endl;
                return true;
            }
        }
        return false;
    }

    // GPU 直连版本：直接从 CVPixelBuffer 识别，零拷贝
    bool detectQRCodeFromCVPixelBuffer(CVPixelBufferRef pixelBuffer, std::string& qrCode)
    {
        @autoreleasepool
        {
            if (pixelBuffer == nullptr)
            {
                return false;
            }

            // 直接使用 GPU 的 CVPixelBuffer，零拷贝
            VNImageRequestHandler* handler = [[VNImageRequestHandler alloc]
                initWithCVPixelBuffer:pixelBuffer options:@{}];

            VNDetectBarcodesRequest* request = [[VNDetectBarcodesRequest alloc] init];
            request.symbologies = @[VNBarcodeSymbologyQR];

            NSError* error = nil;
            auto startTime = std::chrono::high_resolution_clock::now();
            BOOL success = [handler performRequests:@[request] error:&error];
            auto endTime = std::chrono::high_resolution_clock::now();

            if (!success || request.results == nil || request.results.count == 0)
            {
                qrCode.clear();
                // 输出未检测到的时间
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                std::cout << "[QRScanner GPU Vision] " << static_cast<float>(duration) / 1000000
                          << "s decode: (empty)" << std::endl;
                return false;
            }

            VNBarcodeObservation* obs = request.results.firstObject;
            if (obs.payloadStringValue != nil)
            {
                qrCode = obs.payloadStringValue.UTF8String;
                // 输出检测到的时间
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                std::cout << "[QRScanner GPU Vision] " << static_cast<float>(duration) / 1000000
                          << "s decode: " << qrCode.substr(0, std::min<size_t>(50, qrCode.length())) << "..." << std::endl;
                return true;
            }
        }
        return false;
    }

private:
    CVPixelBufferRef m_reusedPixelBuffer;  // 复用的 CVPixelBuffer
};
#endif // __APPLE__

// ========================================
// QRScanner 实现
// ========================================

#include <chrono>

QRScanner::QRScanner() : useVision(false)
{
#ifdef __APPLE__
    pImpl = std::unique_ptr<Impl>(new Impl());
    useVision = true;
#endif
}

QRScanner::~QRScanner() = default;

void QRScanner::decodeSingle(const cv::Mat& img, std::string& qrCode)
{
#ifndef TESTSPEED
    auto startTime = std::chrono::high_resolution_clock::now();
#endif

#ifdef __APPLE__
    if (useVision && pImpl)
    {
        // 暂时不实现，使用 decodeFromBGRA 替代
        qrCode.clear();
    }
#else
    qrCode.clear();
#endif

#ifndef TESTSPEED
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    std::cout << "[QRScanner] " << static_cast<float>(duration) / 1000000
              << "s decode: " << (qrCode.empty() ? "(empty)" : qrCode.substr(0, std::min<size_t>(50, qrCode.length())) + "...") << std::endl;
#endif
}

void QRScanner::decodeMultiple(const cv::Mat& img, std::string& qrCode)
{
#ifdef __APPLE__
    if (useVision && pImpl)
    {
        // 暂时不实现，使用 decodeFromBGRA 替代
        qrCode.clear();
    }
#else
    qrCode.clear();
#endif
}

#ifdef __APPLE__
void QRScanner::decodeFromBGRA(const uint8_t* bgraData, int width, int height, int stride, std::string& qrCode)
{
    if (useVision && pImpl)
    {
        pImpl->detectQRCodeFromBGRA(bgraData, width, height, stride, qrCode);
    }
}

void QRScanner::decodeFromCVPixelBuffer(void* pixelBuffer, int width, int height, std::string& qrCode)
{
    if (useVision && pImpl)
    {
        pImpl->detectQRCodeFromCVPixelBuffer((CVPixelBufferRef)pixelBuffer, qrCode);
    }
}
#endif
