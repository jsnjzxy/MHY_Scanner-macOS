#pragma once

#ifdef __APPLE__

#include <CoreVideo/CoreVideo.h>
#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <memory>

/**
 * @brief 帧来源类型
 */
enum class FrameSource : uint8_t
{
    None = 0,
    Screen = 1,     // ScreenCaptureKit
    Stream = 2,     // FFmpeg VideoToolbox
};

/**
 * @brief 帧元数据
 */
struct FrameMetadata
{
    FrameSource source{FrameSource::None};
    CFTimeInterval timestamp{0};  // 时间戳（毫秒）
    int width{0};
    int height{0};
};

/**
 * @brief 环形帧缓冲区
 *
 * 特性：
 * - 无锁原子操作写入
 * - 自动覆盖旧帧
 * - 支持按优先级读取
 * - 零拷贝传递 CVPixelBuffer
 */
class UnifiedFrameBuffer
{
public:
    static constexpr int BUFFER_SIZE = 5;  // 环形缓冲区大小（可调整）

    UnifiedFrameBuffer();
    ~UnifiedFrameBuffer();

    /**
     * @brief 写入帧（无锁，自动覆盖旧帧）
     * @param frame CVPixelBufferRef，调用者保持所有权，函数内部会 retain
     * @param source 帧来源
     * @param width 帧宽度
     * @param height 帧高度
     */
    void writeFrame(CVPixelBufferRef frame, FrameSource source, int width, int height);

    /**
     * @brief 读取最新帧
     * @param outMeta 输出帧元数据
     * @param priority 优先返回的源，None 则返回任意最新帧
     * @return CVPixelBufferRef，调用者负责 release
     */
    CVPixelBufferRef readLatestFrame(FrameMetadata& outMeta, FrameSource priority = FrameSource::Stream);

    /**
     * @brief 释放帧引用
     */
    void releaseFrame(CVPixelBufferRef frame);

    /**
     * @brief 清空缓冲区
     */
    void clear();

    /**
     * @brief 缓冲区统计信息
     */
    struct Stats
    {
        uint64_t totalWritten{0};   // 总写入帧数
        uint64_t totalRead{0};     // 总读取帧数
        uint64_t screenFrames{0};  // 录屏帧数
        uint64_t streamFrames{0};  // 直播流帧数
        uint64_t overwritten{0};    // 被覆盖的帧数
        int writeIndex{0};         // 当前写入索引
        int validCount{0};         // 有效帧数量
    };

    Stats getStats() const;

private:
    struct FrameSlot
    {
        CVPixelBufferRef frame{nullptr};
        FrameMetadata meta{};
        std::atomic<bool> valid{false};
    };

    std::array<FrameSlot, BUFFER_SIZE> m_buffer;
    std::atomic<int> m_writeIndex{0};
    std::atomic<uint64_t> m_totalWritten{0};
    std::atomic<uint64_t> m_totalRead{0};
    std::atomic<uint64_t> m_screenFrames{0};
    std::atomic<uint64_t> m_streamFrames{0};
    std::atomic<uint64_t> m_overwritten{0};

    // 用于保护多槽操作的信号量（非阻塞路径尽量不用）
    dispatch_semaphore_t m_semaphore;
};

#endif // __APPLE__
