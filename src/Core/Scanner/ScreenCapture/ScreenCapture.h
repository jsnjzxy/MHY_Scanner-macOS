#pragma once

#include <opencv2/core.hpp>
#include <algorithm>

#include <CoreGraphics/CoreGraphics.h>
#include <memory>
#include <atomic>

// 前向声明
class ScreenCaptureKitWrapper;

class ScreenCapture
{
public:
    ScreenCapture();
    ~ScreenCapture();

    double static getZoom();
    cv::Mat getScreenshot();
    cv::Mat getScreenshot(int x, int y, int width, int height);

    // 检查是否有屏幕录制权限
    static bool hasScreenCapturePermission();
    // 请求屏幕录制权限（会打开系统偏好设置）
    static void requestScreenCapturePermission();

    // 直接获取 BGRA 格式截图（跳过 BGR 转换）
    cv::Mat getScreenshotBGRA();
    cv::Mat getScreenshotBGRA(int x, int y, int width, int height);

    // 抢码优化：直接返回预分配缓冲区的引用（避免 clone）
    // 注意：调用者必须在下次调用前使用完毕
    const cv::Mat& getScreenshotBGRANoCopy();

private:
    CGDirectDisplayID m_displayID;
    cv::Mat m_cachedBGRA;  // 预分配的 BGRA 缓冲区
    bool m_bufferValid{ false };  // 缓冲区是否有效

    // ScreenCaptureKit
    std::unique_ptr<ScreenCaptureKitWrapper> m_sckWrapper;
};
