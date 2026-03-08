#include "ScreenCapture.h"
#include "ScreenCaptureKitWrapper.h"

// 取消定义 Objective-C 宏，避免与 OpenCV 冲突
#undef NO
#undef YES

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>

using cv::Mat;

// 检查屏幕录制权限
bool ScreenCapture::hasScreenCapturePermission()
{
    return CGPreflightScreenCaptureAccess();
}

// 请求屏幕录制权限
void ScreenCapture::requestScreenCapturePermission()
{
    if (CGPreflightScreenCaptureAccess()) {
        return;
    }
    CGRequestScreenCaptureAccess();
}

/* 获取屏幕缩放值 */
double ScreenCapture::getZoom()
{
    // macOS Retina 屏幕缩放
    CGDirectDisplayID mainDisplay = CGMainDisplayID();

    // 获取像素尺寸
    size_t pixelWidth = CGDisplayPixelsWide(mainDisplay);
    size_t pixelHeight = CGDisplayPixelsHigh(mainDisplay);

    // 获取点尺寸
    CGSize screenSize = CGDisplayScreenSize(mainDisplay);

    // 计算 PPI (每英寸像素数)
    double ppi = (pixelWidth / screenSize.width) * 25.4;

    // Retina 屏幕通常有 2x 缩放
    if (ppi >= 200) {
        return 2.0;
    } else if (ppi >= 260) {
        return 2.0;
    }

    return 1.0;
}

ScreenCapture::ScreenCapture()
{
    // 获取主显示器 ID
    m_displayID = CGMainDisplayID();

    // 预分配 BGRA 缓冲区
    m_bufferValid = false;

    // 自动启用 ScreenCaptureKit
    m_sckWrapper = std::make_unique<ScreenCaptureKitWrapper>();
    if (m_sckWrapper->init(m_displayID)) {
        std::cout << "[ScreenCapture] ScreenCaptureKit enabled" << std::endl;
    } else {
        std::cerr << "[ScreenCapture] Failed to initialize ScreenCaptureKit" << std::endl;
    }
}

ScreenCapture::~ScreenCapture()
{
}

/* 获取整个屏幕的截图 */
Mat ScreenCapture::getScreenshot()
{
    // 使用 ScreenCaptureKit 获取最新帧
    if (m_sckWrapper && m_sckWrapper->hasFrame()) {
        @autoreleasepool {
            CVPixelBufferRef pixelBuffer = m_sckWrapper->getLatestFrame();
            if (pixelBuffer != nullptr) {
                CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

                void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
                size_t width = CVPixelBufferGetWidth(pixelBuffer);
                size_t height = CVPixelBufferGetHeight(pixelBuffer);
                size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

                // 创建 BGRA 格式的 Mat
                Mat bgrImage(height, width, CV_8UC4);

                // 复制数据（考虑可能的 padding）
                for (size_t y = 0; y < height; y++) {
                    memcpy(
                        bgrImage.data + y * bgrImage.step,
                        static_cast<uint8_t*>(bufferData) + y * bytesPerRow,
                        static_cast<int>(bgrImage.step) < static_cast<int>(bytesPerRow) ? static_cast<int>(bgrImage.step) : static_cast<int>(bytesPerRow)
                    );
                }

                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);

                // 转换 BGRA 到 BGR (OpenCV 默认格式)
                Mat result;
                cv::cvtColor(bgrImage, result, cv::COLOR_BGRA2BGR);

                return result;
            }
        }
    }

    // ScreenCaptureKit 不可用时返回空 Mat
    return Mat();
}

/** @brief 获取指定范围的屏幕截图
 * @param x 图像左上角的 X 坐标
 * @param y 图像左上角的 Y 坐标
 * @param width 图像宽度
 * @param height 图像高度
 */
Mat ScreenCapture::getScreenshot(int x, int y, int width, int height)
{
    // 使用 ScreenCaptureKit 获取并裁剪
    if (m_sckWrapper && m_sckWrapper->hasFrame()) {
        @autoreleasepool {
            CVPixelBufferRef pixelBuffer = m_sckWrapper->getLatestFrame();
            if (pixelBuffer != nullptr) {
                CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

                void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
                size_t fullWidth = CVPixelBufferGetWidth(pixelBuffer);
                size_t fullHeight = CVPixelBufferGetHeight(pixelBuffer);
                size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

                // 检查裁剪区域是否有效
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (x + width > (int)fullWidth) width = (int)fullWidth - x;
                if (y + height > (int)fullHeight) height = (int)fullHeight - y;

                // 创建 BGRA 格式的 Mat
                Mat bgrImage(height, width, CV_8UC4);

                // 逐行复制裁剪区域
                for (size_t yy = 0; yy < (size_t)height; yy++) {
                    size_t srcY = y + yy;
                    memcpy(
                        bgrImage.data + yy * bgrImage.step,
                        static_cast<uint8_t*>(bufferData) + srcY * bytesPerRow + x * 4,
                        width * 4
                    );
                }

                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);

                // 转换 BGRA 到 BGR
                Mat result;
                cv::cvtColor(bgrImage, result, cv::COLOR_BGRA2BGR);
                return result;
            }
        }
    }
    return Mat();
}

/* 获取整个屏幕的 BGRA 截图（优化版本，跳过 BGR 转换，使用预分配缓冲区） */
Mat ScreenCapture::getScreenshotBGRA()
{
    // 使用 ScreenCaptureKit 获取最新帧
    if (m_sckWrapper && m_sckWrapper->hasFrame()) {
        @autoreleasepool {
            CVPixelBufferRef pixelBuffer = m_sckWrapper->getLatestFrame();
            if (pixelBuffer != nullptr) {
                CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

                void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
                size_t width = CVPixelBufferGetWidth(pixelBuffer);
                size_t height = CVPixelBufferGetHeight(pixelBuffer);
                size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

                Mat result(height, width, CV_8UC4);
                for (size_t y = 0; y < height; y++) {
                    memcpy(
                        result.data + y * result.step,
                        static_cast<uint8_t*>(bufferData) + y * bytesPerRow,
                        static_cast<int>(result.step) < static_cast<int>(bytesPerRow) ? static_cast<int>(result.step) : static_cast<int>(bytesPerRow)
                    );
                }

                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);
                return result;
            }
        }
    }
    return Mat();
}

/* 抢码优化版本：直接返回缓存引用，零拷贝 */
const cv::Mat& ScreenCapture::getScreenshotBGRANoCopy()
{
    // 使用自动释放池确保及时清理
    @autoreleasepool {
        // 使用 ScreenCaptureKit 获取最新帧
        if (m_sckWrapper) {
            CVPixelBufferRef pixelBuffer = m_sckWrapper->getLatestFrame();
            if (pixelBuffer != nullptr) {
                // 锁定并直接读取到缓存缓冲区
                CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

                void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
                size_t width = CVPixelBufferGetWidth(pixelBuffer);
                size_t height = CVPixelBufferGetHeight(pixelBuffer);
                size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

                // 如果缓冲区尺寸不匹配，重新分配
                if (!m_bufferValid || m_cachedBGRA.cols != static_cast<int>(width) ||
                    m_cachedBGRA.rows != static_cast<int>(height)) {
                    m_cachedBGRA.create(height, width, CV_8UC4);
                    m_bufferValid = true;
                }

                // 直接复制到缓存
                for (size_t y = 0; y < height; y++) {
                    memcpy(
                        m_cachedBGRA.data + y * m_cachedBGRA.step,
                        static_cast<uint8_t*>(bufferData) + y * bytesPerRow,
                        static_cast<int>(m_cachedBGRA.step) < static_cast<int>(bytesPerRow) ? static_cast<int>(m_cachedBGRA.step) : static_cast<int>(bytesPerRow)
                    );
                }

                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);

                return m_cachedBGRA;
            }
        }
    }

    // ScreenCaptureKit 不可用时返回空 Mat
    static cv::Mat emptyMat;
    return emptyMat;
}

/* 获取指定区域的 BGRA 截图（优化版本，跳过 BGR 转换） */
Mat ScreenCapture::getScreenshotBGRA(int x, int y, int width, int height)
{
    // 使用 ScreenCaptureKit 获取并裁剪
    if (m_sckWrapper && m_sckWrapper->hasFrame()) {
        @autoreleasepool {
            CVPixelBufferRef pixelBuffer = m_sckWrapper->getLatestFrame();
            if (pixelBuffer != nullptr) {
                CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

                void* bufferData = CVPixelBufferGetBaseAddress(pixelBuffer);
                size_t fullWidth = CVPixelBufferGetWidth(pixelBuffer);
                size_t fullHeight = CVPixelBufferGetHeight(pixelBuffer);
                size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

                // 检查裁剪区域
                if (x < 0) x = 0;
                if (y < 0) y = 0;
                if (x + width > (int)fullWidth) width = (int)fullWidth - x;
                if (y + height > (int)fullHeight) height = (int)fullHeight - y;

                Mat bgraImage(height, width, CV_8UC4);
                for (size_t yy = 0; yy < (size_t)height; yy++) {
                    size_t srcY = y + yy;
                    memcpy(
                        bgraImage.data + yy * bgraImage.step,
                        static_cast<uint8_t*>(bufferData) + srcY * bytesPerRow + x * 4,
                        width * 4
                    );
                }

                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);
                return bgraImage;
            }
        }
    }
    return Mat();
}
