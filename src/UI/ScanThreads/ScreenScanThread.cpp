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
    std::cout << "[Screen] initScreenCapture called" << std::endl;
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

    std::cout << "[Screen] Enabling ScreenCaptureKit..." << std::endl;
    m_unifiedScanner->enableScreenSource(true);
    std::cout << "[Screen] ScreenCaptureKit enabled" << std::endl;
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

    // 检查 ticket 是否重复
    const std::string_view ticket(code.data() + code.size() - 24, 24);
    if (lastTicket == ticket) {
        return;
    }

    m_lastDetectedTicket = code;

    // 扫描登录
    if (ScanQRLogin(scanUrl.data(), ticket, gameType)) {
        lastTicket = ticket;
        nlohmann::json config;
        config = nlohmann::json::parse(m_config->getConfig());
        if (config["auto_login"]) {
            continueLastLogin();
        } else {
            emit loginConfirm(gameType, true);
        }
    } else {
        emit loginResults(ScanRet::FAILURE_1);
    }

    // 非连续扫码模式下停止
    if (!m_continuousScan.load()) {
        stop();
    }
}

// macOS implementation using UnifiedScanner
void ScreenScanThread::LoginOfficial()
{
    std::cout << "[Screen] Using UnifiedScanner Architecture" << std::endl;

    // 启动统一扫描器
    m_unifiedScanner->enableScreenSource(true);
    m_unifiedScanner->setContinuousScan(m_continuousScan.load());
    m_unifiedScanner->start();

    std::cout << "[Screen] Started, waiting for QR code..." << std::endl;

    // 等待识别到二维码或停止
    while (m_stop.load() && m_unifiedScanner->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[Screen] Stopped" << std::endl;
}

void ScreenScanThread::LoginBH3BiliBili()
{
    // BH3 Bilibili 使用旧架构（暂未完全实现）
    std::cout << "[Screen] BH3 Bilibili not yet implemented with UnifiedScanner" << std::endl;
    emit loginResults(ScanRet::FAILURE_1);
    stop();
}

void ScreenScanThread::setLoginInfo(const std::string& uid, const std::string& token)
{
    this->uid = uid;
    this->gameToken = token;
}

void ScreenScanThread::setLoginInfo(const std::string& uid, const std::string& token, const std::string& name)
{
    this->uid = uid;
    this->gameToken = token;
    this->m_name = name;
}

void ScreenScanThread::continueLastLogin()
{
    switch (servertype)
    {
        using enum ServerType;
    case Official:
    {
        bool b = ConfirmQRLogin(confirmUrl, uid, gameToken, lastTicket, gameType);
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
