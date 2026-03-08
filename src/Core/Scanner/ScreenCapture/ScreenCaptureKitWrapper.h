#pragma once

#ifdef __APPLE__

#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>
#include <atomic>
#include <mutex>
#include <memory>

// ScreenCaptureKit 屏幕捕获封装
// 使用持久的 SCStream 实现连续捕获
class ScreenCaptureKitWrapper
{
public:
    ScreenCaptureKitWrapper();
    ~ScreenCaptureKitWrapper();

    // 初始化捕获（启动流）
    bool init(CGDirectDisplayID displayID);

    // 获取最新帧（返回 CVPixelBufferRef，调用者负责释放）
    // 从持久流中获取最新帧，不需要等待
    CVPixelBufferRef getLatestFrame();

    // 停止捕获
    void stop();

    // 检查是否正在运行
    bool isRunning() const;

    // 获取流状态
    bool hasFrame() const;

private:
    struct Impl;
    Impl* pImpl;
};

#endif // __APPLE__
