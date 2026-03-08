#include "ScreenCaptureKitWrapper.h"

#ifdef __APPLE__

// 取消定义 Objective-C 宏，避免与 OpenCV 冲突
#undef NO
#undef YES

#include <iostream>

// SCStreamOutput 协议实现 - 持续接收帧数据
@interface SCKStreamOutput : NSObject <SCStreamOutput>
{
@public
    std::atomic<CVPixelBufferRef> frameBuffer;  // 原子指针，存储最新帧
    std::atomic<bool> hasNewFrame;             // 是否有新帧
    std::atomic<bool> streamStarted;            // 流是否已启动
}

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type;

@end

@implementation SCKStreamOutput

- (instancetype)init
{
    self = [super init];
    if (self) {
        frameBuffer.store(nullptr);
        hasNewFrame.store(false);
        streamStarted.store(false);
    }
    return self;
}

- (void)dealloc
{
    // 清理最后保存的帧
    CVPixelBufferRef buffer = frameBuffer.load();
    if (buffer != nullptr)
    {
        CVPixelBufferRelease(buffer);
        frameBuffer.store(nullptr);
    }
    // 调用父类的 dealloc
    [super dealloc];
}

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type
{
    // 使用自动释放池确保临时对象及时释放
    @autoreleasepool {
        if (sampleBuffer != nullptr && type == SCStreamOutputTypeScreen)
        {
            // 从 CMSampleBuffer 中提取 CVPixelBuffer
            CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
            if (imageBuffer != nullptr && CFGetTypeID(imageBuffer) == CVPixelBufferGetTypeID())
            {
                // 获取内存信息
                size_t width = CVPixelBufferGetWidth(imageBuffer);
                size_t height = CVPixelBufferGetHeight(imageBuffer);
                size_t dataSize = CVPixelBufferGetDataSize(imageBuffer);

                // 保留新帧
                CVPixelBufferRef newFrame = (CVPixelBufferRef)CFRetain(imageBuffer);

                // 交换原子指针，释放旧帧
                CVPixelBufferRef oldFrame = frameBuffer.exchange(newFrame);
                if (oldFrame != nullptr)
                {
                    CVPixelBufferRelease(oldFrame);
                }

                hasNewFrame.store(true);
                streamStarted.store(true);
            }
        }
    }
}

@end

// PIMPL 实现
struct ScreenCaptureKitWrapper::Impl
{
    SCStreamConfiguration* config;
    SCContentFilter* filter;
    SCKStreamOutput* streamOutput;
    SCStream* stream;
    CGDirectDisplayID displayID;
    bool initialized;
    bool running;

    Impl() : config(nullptr), filter(nullptr), streamOutput(nullptr),
              stream(nullptr), displayID(0), initialized(false), running(false) {}

    ~Impl()
    {
        // 注意：析构时不自动停止流，由 stop() 方法显式控制
    }

    void cleanup()
    {
        if (stream != nullptr && running)
        {
            // 停止流
            __block bool stopped = false;
            [stream stopCaptureWithCompletionHandler:^(NSError* error) {
                stopped = true;
            }];
            // 等待停止完成
            for (int i = 0; i < 50 && !stopped; i++)
            {
                usleep(10000);  // 10ms
            }
        }

        if (streamOutput) {
            [streamOutput release];
            streamOutput = nullptr;
        }
        if (stream) {
            [stream release];
            stream = nullptr;
        }
        if (config) {
            [config release];
            config = nullptr;
        }
        if (filter) {
            [filter release];
            filter = nullptr;
        }
    }
};

ScreenCaptureKitWrapper::ScreenCaptureKitWrapper()
    : pImpl(new Impl())
{
}

ScreenCaptureKitWrapper::~ScreenCaptureKitWrapper()
{
    stop();
    if (pImpl) {
        delete pImpl;
    }
}

// 异步获取 SCShareableContent 的辅助函数
static void getShareableContentForDisplay(CGDirectDisplayID displayID,
                                           void (^completion)(SCShareableContent* content)) {
    [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent *content, NSError *error) {
        if (error) {
            completion(nil);
        } else {
            completion(content);
        }
    }];
}

bool ScreenCaptureKitWrapper::init(CGDirectDisplayID displayID)
{
    std::cout << "[ScreenCaptureKit] init called with displayID: " << displayID << std::endl;

    if (@available(macOS 12.3, *))
    {
        std::cout << "[ScreenCaptureKit] macOS 12.3+ available" << std::endl;

        pImpl->displayID = displayID;

        // 使用 CGDisplayBounds 获取尺寸
        CGRect displayRect = CGDisplayBounds(displayID);
        std::cout << "[ScreenCaptureKit] Display rect: " << (int)displayRect.size.width << "x" << (int)displayRect.size.height << std::endl;

        // 获取可共享内容（使用 autoreleasepool 管理）
        SCShareableContent* sharedContent = nil;
        {
            __block SCShareableContent* blockContent = nil;
            __block bool contentReady = false;

            std::cout << "[ScreenCaptureKit] Getting shareable content..." << std::endl;
            getShareableContentForDisplay(displayID, ^(SCShareableContent* content) {
                blockContent = [content retain];
                contentReady = true;
            });

            // 等待内容获取（最多等待 2 秒）
            for (int i = 0; i < 200; i++)
            {
                if (contentReady) break;
                usleep(10000);  // 10ms
            }
            sharedContent = blockContent;
        }

        std::cout << "[ScreenCaptureKit] Shareable content ready: " << (sharedContent != nil ? "YES" : "NO") << std::endl;

        if (sharedContent == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to get shareable content" << std::endl;
            return false;
        }

        std::cout << "[ScreenCaptureKit] Display count: " << [sharedContent.displays count] << std::endl;

        // 找到匹配的显示器
        SCDisplay* targetDisplay = nil;
        for (SCDisplay* display in sharedContent.displays)
        {
            std::cout << "[ScreenCaptureKit] Checking display, displayID: " << display.displayID << std::endl;
            if (display.displayID == displayID)
            {
                targetDisplay = display;
                break;
            }
        }

        if (targetDisplay == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to find display with ID " << displayID << std::endl;
            [sharedContent release];
            return false;
        }

        std::cout << "[ScreenCaptureKit] Creating SCStreamConfiguration..." << std::endl;
        // 创建捕获配置
        pImpl->config = [[SCStreamConfiguration alloc] init];
        if (pImpl->config == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to create SCStreamConfiguration" << std::endl;
            [sharedContent release];
            return false;
        }

        // 配置捕获参数
        pImpl->config.width = displayRect.size.width;
        pImpl->config.height = displayRect.size.height;
        pImpl->config.pixelFormat = kCVPixelFormatType_32BGRA;
        pImpl->config.minimumFrameInterval = CMTimeMake(1, 60);  // 60 FPS
        pImpl->config.showsCursor = false;  // 不捕获鼠标

        std::cout << "[ScreenCaptureKit] Creating SCContentFilter..." << std::endl;
        // 创建内容过滤器
        pImpl->filter = [[SCContentFilter alloc] initWithDisplay:targetDisplay excludingWindows:@[]];

        if (pImpl->filter == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to create SCContentFilter" << std::endl;
            [sharedContent release];
            pImpl->cleanup();
            return false;
        }

        std::cout << "[ScreenCaptureKit] Creating SCKStreamOutput..." << std::endl;
        // 创建流输出对象
        pImpl->streamOutput = [[SCKStreamOutput alloc] init];
        if (pImpl->streamOutput == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to create SCKStreamOutput" << std::endl;
            [sharedContent release];
            pImpl->cleanup();
            return false;
        }

        // 创建流
        pImpl->stream = [[SCStream alloc] initWithFilter:pImpl->filter
                                              configuration:pImpl->config
                                                     delegate:nil];

        if (pImpl->stream == nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to create SCStream" << std::endl;
            [sharedContent release];
            pImpl->cleanup();
            return false;
        }

        // 设置输出
        NSError* addOutputError = nil;
        BOOL addSuccess = [pImpl->stream addStreamOutput:pImpl->streamOutput
                                                     type:SCStreamOutputTypeScreen
                                       sampleHandlerQueue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0)
                                                       error:&addOutputError];

        if (!addSuccess) {
            std::cerr << "[ScreenCaptureKit] Failed to add stream output: "
                      << [addOutputError.localizedDescription UTF8String] << std::endl;
            [sharedContent release];
            pImpl->cleanup();
            return false;
        }

        // 启动流（异步）
        __block bool startSuccess = false;
        __block NSError* startError = nil;

        [pImpl->stream startCaptureWithCompletionHandler:^(NSError* error) {
            if (error == nil)
            {
                startSuccess = true;
            }
            else
            {
                startError = [error retain];
            }
        }];

        // 等待流启动（最多等待 500ms）
        for (int i = 0; i < 50; i++)
        {
            if (startSuccess || startError != nil)
            {
                break;
            }
            usleep(10000);  // 10ms
        }

        if (startError != nil)
        {
            std::cerr << "[ScreenCaptureKit] Failed to start stream: "
                      << [startError.localizedDescription UTF8String] << std::endl;
            [sharedContent release];
            pImpl->cleanup();
            return false;
        }

        // 释放 sharedContent
        [sharedContent release];

        pImpl->initialized = true;
        pImpl->running = true;

        std::cout << "[ScreenCaptureKit] Persistent stream initialized for display " << displayID
                  << " (" << (int)displayRect.size.width << "x" << (int)displayRect.size.height << ")" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "[ScreenCaptureKit] Requires macOS 12.3 or later" << std::endl;
        return false;
    }
}

CVPixelBufferRef ScreenCaptureKitWrapper::getLatestFrame()
{
    if (!pImpl->initialized || !pImpl->running)
    {
        return nullptr;
    }

    // 使用自动释放池确保及时清理
    @autoreleasepool {
        // 从持久流中获取最新帧（原子操作）
        CVPixelBufferRef result = pImpl->streamOutput->frameBuffer.load();
        if (result != nullptr)
        {
            // 保留引用（调用者负责释放）
            CFRetain(result);
            return result;
        }
    }
    return nullptr;
}

void ScreenCaptureKitWrapper::stop()
{
    if (pImpl->running && pImpl->initialized)
    {
        pImpl->running = false;
        pImpl->cleanup();
        std::cout << "[ScreenCaptureKit] Stream stopped" << std::endl;
    }
}

bool ScreenCaptureKitWrapper::isRunning() const
{
    return pImpl->running;
}

bool ScreenCaptureKitWrapper::hasFrame() const
{
    if (!pImpl->running || pImpl->streamOutput == nullptr)
    {
        return false;
    }
    return pImpl->streamOutput->hasNewFrame.load() &&
           pImpl->streamOutput->frameBuffer.load() != nullptr;
}

#endif // __APPLE__
