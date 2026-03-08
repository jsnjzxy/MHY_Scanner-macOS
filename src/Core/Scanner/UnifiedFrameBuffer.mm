#include "UnifiedFrameBuffer.h"
#include <iostream>
#include <cstring>

// UnifiedFrameBuffer.mm 是 Objective-C++ 文件，只在 macOS 上编译
// 所以不需要 #ifdef __APPLE__ 保护

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>

UnifiedFrameBuffer::UnifiedFrameBuffer()
    : m_semaphore(dispatch_semaphore_create(1))
{
    for (auto& slot : m_buffer) {
        slot.frame = nullptr;
        slot.valid.store(false, std::memory_order_relaxed);
    }
}

UnifiedFrameBuffer::~UnifiedFrameBuffer()
{
    clear();
    if (m_semaphore) {
        dispatch_release(m_semaphore);
    }
}

void UnifiedFrameBuffer::writeFrame(CVPixelBufferRef frame, FrameSource source, int width, int height)
{
    if (!frame) {
        return;
    }

    // 获取当前写入位置
    int currentWrite = m_writeIndex.load(std::memory_order_relaxed);
    int nextWrite = (currentWrite + 1) % BUFFER_SIZE;
    m_writeIndex.store(nextWrite, std::memory_order_relaxed);

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);

    // 释放旧帧（如果存在）
    if (m_buffer[currentWrite].frame) {
        CVPixelBufferRelease(m_buffer[currentWrite].frame);
        m_buffer[currentWrite].frame = nullptr;
        m_overwritten.fetch_add(1, std::memory_order_relaxed);
    }

    // 写入新帧（retain 增加引用计数）
    m_buffer[currentWrite].frame = CVPixelBufferRetain(frame);
    m_buffer[currentWrite].meta.source = source;
    m_buffer[currentWrite].meta.timestamp = CACurrentMediaTime() * 1000.0;  // 转为毫秒
    m_buffer[currentWrite].meta.width = width;
    m_buffer[currentWrite].meta.height = height;
    m_buffer[currentWrite].valid.store(true, std::memory_order_release);

    dispatch_semaphore_signal(m_semaphore);

    // 更新统计
    m_totalWritten.fetch_add(1, std::memory_order_relaxed);
    if (source == FrameSource::Screen) {
        m_screenFrames.fetch_add(1, std::memory_order_relaxed);
    } else {
        m_streamFrames.fetch_add(1, std::memory_order_relaxed);
    }
}

CVPixelBufferRef UnifiedFrameBuffer::readLatestFrame(FrameMetadata& outMeta, FrameSource priority)
{
    int currentWrite = m_writeIndex.load(std::memory_order_relaxed);

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);

    CVPixelBufferRef result = nullptr;

    // 首先尝试找优先源的帧（从最新往回找）
    if (priority != FrameSource::None) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int idx = (currentWrite - i + BUFFER_SIZE) % BUFFER_SIZE;
            if (m_buffer[idx].valid.load(std::memory_order_acquire) &&
                m_buffer[idx].meta.source == priority &&
                m_buffer[idx].frame != nullptr) {
                result = CVPixelBufferRetain(m_buffer[idx].frame);
                outMeta = m_buffer[idx].meta;
                break;
            }
        }
    }

    // 如果没找到优先源，取最新的有效帧（任意源）
    if (result == nullptr) {
        for (int i = 0; i < BUFFER_SIZE; i++) {
            int idx = (currentWrite - i + BUFFER_SIZE) % BUFFER_SIZE;
            if (m_buffer[idx].valid.load(std::memory_order_acquire) &&
                m_buffer[idx].frame != nullptr) {
                result = CVPixelBufferRetain(m_buffer[idx].frame);
                outMeta = m_buffer[idx].meta;
                break;
            }
        }
    }

    dispatch_semaphore_signal(m_semaphore);

    if (result) {
        m_totalRead.fetch_add(1, std::memory_order_relaxed);
    }

    return result;
}

void UnifiedFrameBuffer::releaseFrame(CVPixelBufferRef frame)
{
    if (frame) {
        CVPixelBufferRelease(frame);
    }
}

void UnifiedFrameBuffer::clear()
{
    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);

    for (auto& slot : m_buffer) {
        if (slot.frame) {
            CVPixelBufferRelease(slot.frame);
            slot.frame = nullptr;
        }
        slot.valid.store(false, std::memory_order_relaxed);
        std::memset(&slot.meta, 0, sizeof(FrameMetadata));
    }

    dispatch_semaphore_signal(m_semaphore);
}

UnifiedFrameBuffer::Stats UnifiedFrameBuffer::getStats() const
{
    Stats stats;
    stats.totalWritten = m_totalWritten.load(std::memory_order_relaxed);
    stats.totalRead = m_totalRead.load(std::memory_order_relaxed);
    stats.screenFrames = m_screenFrames.load(std::memory_order_relaxed);
    stats.streamFrames = m_streamFrames.load(std::memory_order_relaxed);
    stats.overwritten = m_overwritten.load(std::memory_order_relaxed);
    stats.writeIndex = m_writeIndex.load(std::memory_order_relaxed);

    // 统计有效帧数量
    int validCount = 0;
    for (const auto& slot : m_buffer) {
        if (slot.valid.load(std::memory_order_relaxed)) {
            validCount++;
        }
    }
    stats.validCount = validCount;

    return stats;
}
