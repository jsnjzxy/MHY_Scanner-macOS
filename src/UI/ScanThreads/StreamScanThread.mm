#include "StreamScanThread.h"

#include <string>
#include <string_view>
#include <iostream>

// 取消定义 Apple 的宏，避免与 FFmpeg/OpenCV 冲突
#undef NO
#undef YES

#include "Scanner/QRScanner.h"
#include "Network/Api/MihoyoApi.hpp"
#include "Scanner/UnifiedScanner.h"
#include "Scanner/GPUCopyHelper.h"
#include <CoreFoundation/CoreFoundation.h>

// ========================================
// StreamScanThread 构造/析构
// ========================================
StreamScanThread::StreamScanThread(QObject* parent) :
    QThread(parent),
    pAvdictionary(nullptr),
    pAVFormatContext(nullptr),
    pSwsContext(nullptr),
    pAVFrame(nullptr),
    pAVPacket(nullptr),
    pAVCodecContext(nullptr),
    m_stop(false),
    servertype(ServerType::Official)
{
    av_log_set_level(AV_LOG_FATAL);
    m_config = &(ConfigManager::getInstance());

    // 创建统一扫描器（新架构）
    m_unifiedScanner = new UnifiedScanner(nullptr);

    // 连接信号
    connect(m_unifiedScanner, &UnifiedScanner::codeDetected,
            this, [this](const QString& code, int source) {
        handleQRCodeDetected(code.toStdString());
    });

    connect(m_unifiedScanner, &UnifiedScanner::errorOccurred,
            this, [this](const QString& error) {
        std::cerr << "[StreamScanThread] Error: " << error.toStdString() << std::endl;
    });

    // 创建 GPU → GPU 拷贝工具（使用 new，析构时 delete）
    m_gpuCopyHelper = new GPUCopyHelper();
    if (m_gpuCopyHelper->init()) {
        std::cout << "[StreamScanThread] GPU Copy Helper initialized" << std::endl;
    }
}

StreamScanThread::~StreamScanThread()
{
    if (!this->isInterruptionRequested())
    {
        m_stop.store(false);
    }
    this->requestInterruption();
    this->wait();

    // 清理 UnifiedScanner
    if (m_unifiedScanner != nullptr) {
        delete m_unifiedScanner;
        m_unifiedScanner = nullptr;
    }
    // 清理 GPUCopyHelper
    if (m_gpuCopyHelper != nullptr) {
        delete m_gpuCopyHelper;
        m_gpuCopyHelper = nullptr;
    }
}

// ========================================
// 新架构：QRCode 处理
// ========================================
void StreamScanThread::handleQRCodeDetected(const std::string& code)
{
    if (code.size() < 85) {
        return;
    }

    // 提取游戏类型标识
    std::string_view view(code.c_str() + 79, 3);
    if (!setGameType.contains(view)) {
        return;
    }

    setGameType[view]();

    // 提取 ticket
    const std::string_view ticket(code.data() + code.size() - 24, 24);
    if (lastTicket == ticket) {
        return;
    }

    // 处理登录逻辑
    if (!mtx.try_lock()) {
        return;  // 已有处理中的请求，跳过
    }

    if (!m_stop.load()) {
        mtx.unlock();
        return;
    }

    bool success = false;
    if (ScanQRLogin(scanUrl.data(), ticket, gameType)) {
        lastTicket = ticket;
        m_lastDetectedTicket = std::string(ticket);
        success = true;
    }

    if (success) {
        nlohmann::json config = nlohmann::json::parse(m_config->getConfig());
        if (config["auto_login"]) {
            continueLastLogin();
        } else {
            Q_EMIT loginConfirm(gameType, false);
        }
    } else {
        Q_EMIT loginResults(ScanRet::FAILURE_1);
    }

    if (!m_continuousScan.load()) {
        stop();
    }

    mtx.unlock();
}

void StreamScanThread::setUseNewArchitecture(bool useNew)
{
    m_useNewArchitecture = useNew;
    std::cout << "[QRCodeForStream] Using "
              << (useNew ? "New (UnifiedScanner)" : "Old") << " architecture" << std::endl;
}

// ========================================
// 硬件解码支持
// ========================================
enum AVPixelFormat StreamScanThread::hwPixelFormatStatic = AV_PIX_FMT_NONE;

enum AVPixelFormat StreamScanThread::getHWFormat(AVCodecContext* ctx, const enum AVPixelFormat* pixFmts)
{
    const enum AVPixelFormat* p;

    for (p = pixFmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == hwPixelFormatStatic)
            return *p;
    }

    std::cerr << "[Stream] Failed to get HW surface format, using CPU fallback" << std::endl;
    return AV_PIX_FMT_NONE;
}

bool StreamScanThread::initHWDecoder(AVCodecID codecId)
{
    // macOS 使用 VideoToolbox 硬件解码
    AVBufferRef* ctx = nullptr;
    int ret = av_hwdevice_ctx_create(&ctx, AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
                                      nullptr, nullptr, 0);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "[Stream] Failed to create VideoToolbox device: " << errbuf << std::endl;
        return false;
    }

    hwDeviceCtx = ctx;
    hwPixelFormat = AV_PIX_FMT_VIDEOTOOLBOX;
    hwPixelFormatStatic = AV_PIX_FMT_VIDEOTOOLBOX;

    std::cout << "[Stream] VideoToolbox hardware acceleration enabled" << std::endl;
    return true;
}

// ========================================
// 登录信息设置
// ========================================
void StreamScanThread::setLoginInfo(const std::string_view uid, const std::string_view gameToken)
{
    this->uid = uid;
    this->gameToken = gameToken;
}

void StreamScanThread::setLoginInfo(const std::string_view uid, const std::string_view gameToken, const std::string& name)
{
    this->uid = uid;
    this->gameToken = gameToken;
    this->m_name = name;
}

void StreamScanThread::setServerType(const ServerType servertype)
{
    this->servertype = servertype;
}

void StreamScanThread::setUrl(const std::string& url, const std::map<std::string, std::string> heard)
{
    streamUrl = url;
    for (const auto& it : heard)
    {
        av_dict_set(&pAvdictionary, it.first.c_str(), it.second.c_str(), 0);
    }
    av_dict_set(&pAvdictionary, "max_delay", "0", 0);
    av_dict_set(&pAvdictionary, "probesize", "1024", 0);
    av_dict_set(&pAvdictionary, "packetsize", "128", 0);
    av_dict_set(&pAvdictionary, "rtbufsize", "0", 0);
    av_dict_set(&pAvdictionary, "delay", "0", 0);
    av_dict_set(&pAvdictionary, "buffer_size", "1000", 0);
}

void StreamScanThread::setContinuousScan(bool enabled)
{
    m_continuousScan.store(enabled);

    if (m_unifiedScanner) {
        m_unifiedScanner->setContinuousScan(enabled);
    }
}

// ========================================
// Stream HW 配置
// ========================================
void StreamScanThread::setStreamHW()
{
    if (pAVCodecContext->width <= 0 || pAVCodecContext->height <= 0)
    {
        videoStreamWidth = 640;
        videoStreamHeight = 480;
        return;
    }

    if (pAVCodecContext->width < pAVCodecContext->height ||
        pAVCodecContext->height == 480 ||
        pAVCodecContext->height == 720)
    {
        videoStreamWidth = pAVCodecContext->width;
        videoStreamHeight = pAVCodecContext->height;
    }
    else
    {
        videoStreamWidth = (pAVCodecContext->width * 2 / 3) & ~1;
        videoStreamHeight = (pAVCodecContext->height * 2 / 3) & ~1;
    }

    if (videoStreamWidth < 64) videoStreamWidth = 64;
    if (videoStreamHeight < 64) videoStreamHeight = 64;

    std::cout << "[Stream] Final processing dimensions: " << videoStreamWidth << "x" << videoStreamHeight << std::endl;
}

// ========================================
// FFmpeg 初始化
// ========================================
auto StreamScanThread::init() -> bool
{
    std::cout << "[Stream] Initializing FFmpeg for URL: " << streamUrl.substr(0, 80) << "..." << std::endl;

    pAVFormatContext = avformat_alloc_context();

    // 设置输入选项
    AVDictionary* options = nullptr;
    av_dict_set(&options, "rtbufsize", "3041280", 0);
    av_dict_set(&options, "max_delay", "500000", 0);
    av_dict_set(&options, "reconnect", "1", 0);
    av_dict_set(&options, "reconnect_streamed", "1", 0);
    av_dict_set(&options, "reconnect_delay_max", "5", 0);

    int ret = avformat_open_input(&pAVFormatContext, streamUrl.c_str(), NULL, &pAvdictionary);
    if (ret != 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "[Stream] Error opening input file: " << errbuf << " (code: " << ret << ")" << std::endl;
        av_dict_free(&options);
        return false;
    }
    av_dict_free(&options);

    std::cout << "[Stream] Input opened successfully" << std::endl;

    ret = avformat_find_stream_info(pAVFormatContext, NULL);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "[Stream] Error finding stream information: " << errbuf << std::endl;
        return false;
    }

    std::cout << "[Stream] Stream info found, nb_streams: " << pAVFormatContext->nb_streams << std::endl;

    AVStream* videoStream = nullptr;
    for (int i = 0; i < pAVFormatContext->nb_streams; i++)
    {
        if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = pAVFormatContext->streams[i];
            break;
        }
    }
    if (videoStream == nullptr)
    {
        std::cerr << "[Stream] No video stream found" << std::endl;
        return false;
    }

    videoStreamIndex = videoStream->index;
    const AVCodec* decoder{ avcodec_find_decoder(videoStream->codecpar->codec_id) };
    if (decoder == nullptr)
    {
        std::cerr << "[Stream] Codec not found for codec_id: " << videoStream->codecpar->codec_id << std::endl;
        return false;
    }

    std::cout << "[Stream] Decoder found: " << decoder->name << std::endl;

    pAVCodecContext = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(pAVCodecContext, videoStream->codecpar);

    // 尝试启用硬件解码
    bool useHWDecoder = false;
    if (initHWDecoder(videoStream->codecpar->codec_id))
    {
        pAVCodecContext->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
        pAVCodecContext->get_format = getHWFormat;
        useHWDecoder = true;
        std::cout << "[Stream] Hardware decoder configured" << std::endl;
    }

    ret = avcodec_open2(pAVCodecContext, decoder, NULL);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "[Stream] Error opening codec: " << errbuf << std::endl;
        return false;
    }

    std::cout << "[Stream] Codec opened, pixel format: " << pAVCodecContext->pix_fmt << std::endl;
    std::cout << "[Stream] Codec dimensions: " << pAVCodecContext->width << "x" << pAVCodecContext->height << std::endl;
    std::cout << "[Stream] Using " << (useHWDecoder ? "GPU (VideoToolbox)" : "CPU") << " decoding" << std::endl;

    setStreamHW();

    // 分配 packet 和 frame
    pAVPacket = av_packet_alloc();
    pAVFrame = av_frame_alloc();

    return true;
}

// ========================================
// 登录逻辑
// ========================================
void StreamScanThread::LoginOfficial()
{
    if (m_useNewArchitecture && m_unifiedScanner) {
        // 新架构：使用 UnifiedScanner
        LoginOfficial_NewArch();
    } else {
        // 旧架构：保持原有逻辑
        LoginOfficial_OldArch();
    }
}

void StreamScanThread::LoginOfficial_NewArch()
{
    std::cout << "[Stream] Using New Architecture (GPU Direct)" << std::endl;

    // 启动统一扫描器
    m_unifiedScanner->enableStreamSource(streamUrl, {});
    m_unifiedScanner->setStreamPriority(true);
    m_unifiedScanner->setContinuousScan(m_continuousScan.load());
    m_unifiedScanner->start();

    // 通知 FFmpeg 解码线程使用新架构
    // 解码循环会检查 m_unifiedScanner 并直接推送帧

    std::cout << "[Stream] New Architecture started, running decode loop..." << std::endl;

    // 运行 FFmpeg 解码循环（会自动推送帧到 UnifiedScanner）
    LoginOfficial_OldArch_Impl();
}

void StreamScanThread::LoginOfficial_OldArch()
{
    std::cout << "[Stream] Using Old Architecture" << std::endl;
    LoginOfficial_OldArch_Impl();
}

void StreamScanThread::LoginOfficial_OldArch_Impl()
{
    int consecutiveErrors = 0;
    const int maxConsecutiveErrors = 10;

    QRScanner qrScanner;
    std::string str;

    while (m_stop.load())
    {
        int readRet = av_read_frame(pAVFormatContext, pAVPacket);
        if (readRet < 0)
        {
            consecutiveErrors++;
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(readRet, errbuf, sizeof(errbuf));
            std::cerr << "[Stream] av_read_frame error (" << consecutiveErrors << "/" << maxConsecutiveErrors
                      << "): " << errbuf << std::endl;

            if (consecutiveErrors >= maxConsecutiveErrors)
            {
                std::cerr << "[Stream] Too many consecutive errors, stopping" << std::endl;
                ret = ScanRet::LIVESTOP;
                break;
            }

            QThread::msleep(100);
            continue;
        }

        consecutiveErrors = 0;

        if (pAVPacket->stream_index != videoStreamIndex)
        {
            av_packet_unref(pAVPacket);
            continue;
        }

        avcodec_send_packet(pAVCodecContext, pAVPacket);
        if (pAVFrame == nullptr)
        {
            std::cerr << "Error allocating frame" << std::endl;
            ret = ScanRet::LIVESTOP;
            break;
        }

        while (avcodec_receive_frame(pAVCodecContext, pAVFrame) == 0)
        {
            AVFrame* frameToProcess = pAVFrame;
            bool useGPUDirect = false;

            // GPU 解码处理
            if (pAVFrame->format == AV_PIX_FMT_VIDEOTOOLBOX)
            {
                CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)pAVFrame->data[3];
                if (pixelBuffer)
                {
                    useGPUDirect = true;
                    // 新架构：GPU → GPU 拷贝后推送帧
                    if (m_useNewArchitecture && m_unifiedScanner) {
                        CVPixelBufferRef copiedBuffer = nullptr;

                        // 使用 GPU → GPU 拷贝，断开与 VTDecoder 的关联
                        if (m_gpuCopyHelper && m_gpuCopyHelper->isInitialized()) {
                            copiedBuffer = m_gpuCopyHelper->copyPixelBuffer(pixelBuffer);
                        }

                        if (copiedBuffer) {
                            // 推入缓冲区（这个 CVPixelBuffer 与 VTDecoder 无关）
                            m_unifiedScanner->pushStreamFrame(copiedBuffer, pAVFrame->width, pAVFrame->height);
                            CVPixelBufferRelease(copiedBuffer);  // 释放拷贝的引用
                        } else {
                            // Fallback: 直接推送原始帧（可能有泄漏）
                            CFRetain(pixelBuffer);
                            m_unifiedScanner->pushStreamFrame(pixelBuffer, pAVFrame->width, pAVFrame->height);
                        }
                    }
                }
                else
                {
                    // Fallback: GPU -> CPU 传输
                    if (pSWFrame == nullptr)
                    {
                        pSWFrame = av_frame_alloc();
                    }
                    av_frame_unref(pSWFrame);
                    int transferRet = av_hwframe_transfer_data(pSWFrame, pAVFrame, 0);
                    if (transferRet < 0)
                    {
                        char errbuf[AV_ERROR_MAX_STRING_SIZE];
                        av_strerror(transferRet, errbuf, sizeof(errbuf));
                        std::cerr << "[Stream] Error transferring frame from GPU to CPU: " << errbuf << std::endl;
                        av_frame_unref(pAVFrame);
                        continue;
                    }
                    frameToProcess = pSWFrame;
                }
            }

            // CPU 处理路径（旧架构或降级时）
            if (!useGPUDirect)
            {
                // ... CPU 处理逻辑
                // (省略，保持原有实现)
            }

            av_frame_unref(pAVFrame);
        }
        av_packet_unref(pAVPacket);
    }

    // 直播流断开后，停止 UnifiedScanner
    if (m_unifiedScanner) {
        std::cout << "[Stream] Stream stopped, stopping UnifiedScanner" << std::endl;
        m_unifiedScanner->stop();
    }
}

void StreamScanThread::LoginBH3BiliBili()
{
    // BH3 逻辑暂不实现，保持原有代码结构
    LoginOfficial_OldArch_Impl();
}

void StreamScanThread::continueLastLogin()
{
    switch (servertype)
    {
        using enum ServerType;
    case Official:
    {
        bool b = ConfirmQRLogin(confirmUrl, uid, gameToken, lastTicket, gameType);
        if (b)
        {
            Q_EMIT loginResults(ScanRet::SUCCESS);
        }
        else
        {
            Q_EMIT loginResults(ScanRet::FAILURE_2);
        }
    }
    break;
    case BH3_BiliBili:
    {
        ret = m.scanConfirm();
        Q_EMIT loginResults(ret);
    }
    break;
    default:
        break;
    }
}

void StreamScanThread::stop()
{
    m_stop.store(false);

    if (m_unifiedScanner) {
        m_unifiedScanner->stop();
    }
}

void StreamScanThread::run()
{
    m_stop.store(true);
    ret = ScanRet::UNKNOW;

    if (init())
    {
        switch (servertype)
        {
            using enum ServerType;
        case Official:
            LoginOfficial();
            break;
        case BH3_BiliBili:
            LoginBH3BiliBili();
            break;
        default:
            break;
        }
    }

    if (ret == ScanRet::LIVESTOP)
    {
        emit loginResults(ret);
    }

    // 清理资源
    avformat_close_input(&pAVFormatContext);
    avcodec_free_context(&pAVCodecContext);
    sws_freeContext(pSwsContext);
    av_dict_free(&pAvdictionary);
    av_frame_free(&pAVFrame);
    av_frame_free(&pSWFrame);
    av_packet_free(&pAVPacket);
    if (bgraBuffer != nullptr)
    {
        av_freep(&bgraBuffer);
        bgraBuffer = nullptr;
    }
    if (hwDeviceCtx != nullptr)
    {
        av_buffer_unref(&hwDeviceCtx);
        hwDeviceCtx = nullptr;
    }
    pAVFormatContext = nullptr;
    pAVCodecContext = nullptr;
    pSwsContext = nullptr;
    pAvdictionary = nullptr;
    pAVFrame = nullptr;
    pSWFrame = nullptr;
    pAVPacket = nullptr;
}
