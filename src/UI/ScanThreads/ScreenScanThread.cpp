#include "ScreenScanThread.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <opencv2/opencv.hpp>

#include <nlohmann/json.hpp>

#include "Scanner/QRScanner.h"
#include "Scanner/ScreenCapture/ScreenCapture.h"
#include "Network/Api/MihoyoApi.hpp"
#include "Scanner/UnifiedScanner.h"

// 抢码优化：减少延迟到 16ms (约 60fps)，更快的响应速度
#define DELAYED 16

ScreenScanThread::ScreenScanThread(QObject* parent) :
    QThread(parent),
    m_stop(false)
{
    m_config = &ConfigManager::getInstance();
}

ScreenScanThread::~ScreenScanThread()
{
    if (!this->isInterruptionRequested())
    {
        m_stop.store(false);
    }
    this->requestInterruption();
    this->wait();

    if (m_unifiedScanner) {
        delete m_unifiedScanner;
        m_unifiedScanner = nullptr;
    }
}

void ScreenScanThread::initScreenCapture()
{
    // 在主线程初始化 UnifiedScanner
    if (m_unifiedScanner == nullptr) {
        m_unifiedScanner = new UnifiedScanner(nullptr);
        connect(m_unifiedScanner, &UnifiedScanner::codeDetected,
                this, [this](const QString& code, int source) {
                    handleQRCodeDetected(code.toStdString());
                });
        connect(m_unifiedScanner, &UnifiedScanner::errorOccurred,
                this, [this](const QString& error) {
                    std::cout << "[Screen] Error: " << error.toStdString() << std::endl;
                });
    }

    m_unifiedScanner->enableScreenSource(true);
}

void ScreenScanThread::handleQRCodeDetected(const std::string& code)
{
    // 避免重复上报同一码
    if (code == m_lastDetectedTicket) {
        return;
    }

    // 检查二维码长度和格式
    if (code.size() < 85) {
        return;
    }

    // 检查游戏类型
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

    m_lastDetectedTicket = code;

    // 检查是否有登录凭证
    if (stoken.empty() || mid.empty()) {
        std::cout << "[Screen] No account credentials available" << std::endl;
        emit loginResults(ScanRet::FAILURE_1);
        return;
    }

    // 1. 游戏特定 API 扫描，获取 tk
    std::string tk = ScanQRLogin(scanUrl.data(), ticket, gameType);
    if (tk.empty()) {
        std::cout << "[Screen] ScanQRLogin failed or no tk returned" << std::endl;
        emit loginResults(ScanRet::FAILURE_1);
        if (!m_continuousScan.load()) {
            stop();
        }
        return;
    }

    lastTicket = ticket;
    m_tk = tk;  // 保存 tk 用于确认

    // 2. Passport API 扫描
    if (!ScanGameQrcode(tk, stoken, mid)) {
        emit loginResults(ScanRet::FAILURE_1);
        if (!m_continuousScan.load()) {
            stop();
        }
        return;
    }

    nlohmann::json config = nlohmann::json::parse(m_config->getConfig());
    if (config["auto_login"]) {
        continueLastLogin();
    } else {
        emit loginConfirm(gameType, true);
    }

    // 非连续扫码模式下停止
    if (!m_continuousScan.load()) {
        stop();
    }
}

// macOS implementation using UnifiedScanner
void ScreenScanThread::LoginOfficial()
{
    // 启动统一扫描器
    m_unifiedScanner->enableScreenSource(true);
    m_unifiedScanner->setContinuousScan(m_continuousScan.load());
    m_unifiedScanner->start();

    // 等待识别到二维码或停止
    while (m_stop.load() && m_unifiedScanner->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ScreenScanThread::LoginBH3BiliBili()
{
    emit loginResults(ScanRet::FAILURE_1);
    stop();
}

void ScreenScanThread::setLoginInfo(const std::string& stoken, const std::string& mid)
{
    this->stoken = stoken;
    this->mid = mid;
}

void ScreenScanThread::setLoginInfo(const std::string& stoken, const std::string& mid, const std::string& name)
{
    this->stoken = stoken;
    this->mid = mid;
    this->m_name = name;
}

void ScreenScanThread::continueLastLogin()
{
    switch (servertype)
    {
        using enum ServerType;
    case Official:
    {
        // 使用 Passport API 确认
        bool b = ConfirmGameQrcode(m_tk, stoken, mid);
        if (b)
        {
            emit loginResults(ScanRet::SUCCESS);
        }
        else
        {
            emit loginResults(ScanRet::FAILURE_2);
        }
    }
    break;
    case BH3_BiliBili:
    {
        ret = m.scanConfirm();
        emit loginResults(ret);
    }
    break;
    default:
        break;
    }
}

void ScreenScanThread::run()
{
    m_stop.store(true);

    switch (servertype)
    {
    case ServerType::Official:
        LoginOfficial();
        break;
    case ServerType::BH3_BiliBili:
        LoginBH3BiliBili();
        break;
    default:
        break;
    }
}

void ScreenScanThread::setServerType(const ServerType servertype)
{
    this->servertype = servertype;
}

void ScreenScanThread::setContinuousScan(bool enabled)
{
    m_continuousScan.store(enabled);
    if (m_unifiedScanner) {
        m_unifiedScanner->setContinuousScan(enabled);
    }
}

void ScreenScanThread::stop()
{
    m_stop.store(false);
    if (m_unifiedScanner) {
        m_unifiedScanner->stop();
    }
}
