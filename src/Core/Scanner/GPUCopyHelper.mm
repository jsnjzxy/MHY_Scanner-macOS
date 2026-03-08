#include "GPUCopyHelper.h"

#ifdef __APPLE__

#include <iostream>
#include <cstring>

#import <Foundation/Foundation.h>
#import <CoreImage/CoreImage.h>
#import <QuartzCore/QuartzCore.h>

// Pimpl 实现类
class GPUCopyHelper::Impl {
public:
    ~Impl() {
        if (m_ciContext) {
            m_ciContext = nil;
        }
        if (m_pixelBufferPool) {
            CVPixelBufferPoolRelease(m_pixelBufferPool);
            m_pixelBufferPool = nullptr;
        }
    }

    bool init() {
        m_ciContext = [[CIContext alloc] initWithOptions:@{kCIContextPriorityRequestLow: @0}];
        if (!m_ciContext) {
            std::cerr << "[GPUCopy] Failed to create CIContext" << std::endl;
            return false;
        }
        std::cout << "[GPUCopy] Initialized with CoreImage GPU context" << std::endl;
        return true;
    }

    bool ensurePixelBufferPool(int width, int height) {
        if (m_pixelBufferPool && m_poolWidth == width && m_poolHeight == height) {
            return true;
        }
        if (m_pixelBufferPool) {
            CVPixelBufferPoolRelease(m_pixelBufferPool);
        }

        NSDictionary* poolAttrs = @{
            (NSString*)kCVPixelBufferPoolMinimumBufferCountKey: @3,
            (NSString*)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey: @{}
        };

        NSDictionary* bufferAttrs = @{
            (NSString*)kCVPixelBufferWidthKey: @(width),
            (NSString*)kCVPixelBufferHeightKey: @(height),
            (NSString*)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey: @{}
        };

        CVReturn cvRet = CVPixelBufferPoolCreate(
            kCFAllocatorDefault,
            (__bridge CFDictionaryRef)poolAttrs,
            (__bridge CFDictionaryRef)bufferAttrs,
            &m_pixelBufferPool
        );

        if (cvRet == kCVReturnSuccess && m_pixelBufferPool) {
            m_poolWidth = width;
            m_poolHeight = height;
            return true;
        }
        return false;
    }

    CVPixelBufferRef copyPixelBuffer(CVPixelBufferRef source) {
        if (!source) return nullptr;
        if (!m_ciContext) return nullptr;

        int width = (int)CVPixelBufferGetWidth(source);
        int height = (int)CVPixelBufferGetHeight(source);

        if (!ensurePixelBufferPool(width, height)) {
            return nullptr;
        }

        CVPixelBufferRef dest = nullptr;
        CVReturn cvRet = CVPixelBufferPoolCreatePixelBuffer(
            kCFAllocatorDefault,
            m_pixelBufferPool,
            &dest
        );

        if (cvRet != kCVReturnSuccess || !dest) {
            return nullptr;
        }

        @autoreleasepool {
            CIImage* sourceImage = [CIImage imageWithCVPixelBuffer:source];
            if (sourceImage) {
                [m_ciContext render:sourceImage toCVPixelBuffer:dest];
                return dest;
            }
        }

        // Fallback: CPU 拷贝
        CVPixelBufferLockBaseAddress(source, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferLockBaseAddress(dest, 0);

        const uint8_t* src = (const uint8_t*)CVPixelBufferGetBaseAddress(source);
        uint8_t* dst = (uint8_t*)CVPixelBufferGetBaseAddress(dest);

        size_t srcBytesPerRow = CVPixelBufferGetBytesPerRow(source);
        size_t dstBytesPerRow = CVPixelBufferGetBytesPerRow(dest);
        size_t bytesPerRow = std::min(srcBytesPerRow, dstBytesPerRow);

        for (int y = 0; y < height; y++) {
            memcpy(dst + y * dstBytesPerRow, src + y * srcBytesPerRow, bytesPerRow);
        }

        CVPixelBufferUnlockBaseAddress(dest, 0);
        CVPixelBufferUnlockBaseAddress(source, kCVPixelBufferLock_ReadOnly);

        return dest;
    }

    // CoreImage 上下文
    __strong CIContext* m_ciContext = nil;

    // CVPixelBuffer 池（复用缓冲区）
    CVPixelBufferPoolRef m_pixelBufferPool = nullptr;
    int m_poolWidth = 0;
    int m_poolHeight = 0;
};

// GPUCopyHelper 实现
GPUCopyHelper::GPUCopyHelper()
    : m_impl(nullptr), m_initialized(false)
{
}

GPUCopyHelper::~GPUCopyHelper()
{
    delete m_impl;
}

bool GPUCopyHelper::init()
{
    m_impl = new Impl();
    m_initialized = m_impl->init();
    return m_initialized;
}

CVPixelBufferRef GPUCopyHelper::copyPixelBuffer(CVPixelBufferRef source)
{
    if (!m_impl) return nullptr;
    return m_impl->copyPixelBuffer(source);
}

#endif // __APPLE__
