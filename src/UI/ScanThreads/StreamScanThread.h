#pragma once

#include <atomic>
#include <mutex>
#include <string_view>
#include <thread>
#include <memory>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
};

#include <QThread>
#include <QMutex>
#include <nlohmann/json.hpp>

#include <CoreVideo/CoreVideo.h>

#include "SDK/MihoyoSDK.h"
#include "Common/Types.h"
#include "Config/ConfigManager.h"
#include "Scanner/ScannerBase.hpp"

// 前向声明 UnifiedScanner，避免循环依赖
class UnifiedScanner;
class GPUCopyHelper;

class StreamScanThread final :
    public QThread,
    public ScannerBase
{
    Q_OBJECT
public:
    StreamScanThread(QObject* parent = nullptr);
    ~StreamScanThread();
    Q_DISABLE_COPY_MOVE(StreamScanThread)

    void setLoginInfo(const std::string_view uid, const std::string_view gameToken);
    void setLoginInfo(const std::string_view uid, const std::string_view gameToken, const std::string& name);
    void setServerType(const ServerType servertype);
    void setUrl(const std::string& url, const std::map<std::string, std::string> heard = {});
    void setContinuousScan(bool enabled);  // 设置连续扫码模式
    auto init() -> bool;
    void run();
    void stop();
    void continueLastLogin();

    /**
     * @brief 设置是否使用新架构
     * @param useNew true: 使用 UnifiedScanner 新架构；false: 使用旧架构
     */
    void setUseNewArchitecture(bool useNew);

Q_SIGNALS:
    void loginResults(const ScanRet ret);
    void loginConfirm(const GameType gameType, bool b);

private:
    std::mutex mtx;
    MihoyoSDK m;
    void LoginOfficial();
    void LoginBH3BiliBili();
    void setStreamHW();

    // 架构特定的登录方法
    void LoginOfficial_NewArch();
    void LoginOfficial_OldArch();

    // 旧架构实现
    void LoginOfficial_OldArch_Impl();

    // 硬件解码支持
    AVBufferRef* hwDeviceCtx{ nullptr };
    enum AVPixelFormat hwPixelFormat{ AV_PIX_FMT_NONE };
    static enum AVPixelFormat hwPixelFormatStatic;  // 静态成员供回调使用
    static enum AVPixelFormat getHWFormat(AVCodecContext* ctx, const enum AVPixelFormat* pixFmts);
    bool initHWDecoder(AVCodecID codecId);

    std::string streamUrl{};
    std::string m_name;
    ConfigManager* m_config;
    ServerType servertype;
    ScanRet ret = ScanRet::UNKNOW;
    AVDictionary* pAvdictionary;
    AVFormatContext* pAVFormatContext;
    AVCodecContext* pAVCodecContext;
    SwsContext* pSwsContext;
    AVFrame* pAVFrame;           // 解码后的帧 (CPU 或 GPU)
    AVFrame* pSWFrame{ nullptr }; // CPU 拷贝帧 (用于 GPU 解码时传输到 CPU)
    AVPacket* pAVPacket;
    int videoStreamIndex{ 0 };
    int videoStreamWidth{};
    int videoStreamHeight{};

    // 优化：直接使用 BGRA 缓冲区进行 QR 识别
    uint8_t* bgraBuffer{ nullptr };
    int bgraStride{ 0 };
    SwsContext* pSwsContextBGRA{ nullptr };  // YUV -> BGRA 转换上下文

    std::atomic<bool> m_stop;
    std::atomic<bool> m_continuousScan{false};  // 连续扫码模式

    // ========================================
    // 新架构支持
    // ========================================
    bool m_useNewArchitecture{true};  // 默认使用新架构

    // 使用原始指针 + 析构时删除，避免循环依赖问题
    UnifiedScanner* m_unifiedScanner{nullptr};
    std::string m_lastDetectedTicket;  // 避免重复上报

    // GPU → GPU 拷贝工具（断开与 VTDecoder 的关联）
    // 使用原始指针，避免在 .cpp 文件中包含 Objective-C 头文件
    GPUCopyHelper* m_gpuCopyHelper{nullptr};

    /**
     * @brief 处理识别到的二维码
     */
    void handleQRCodeDetected(const std::string& code);
};
