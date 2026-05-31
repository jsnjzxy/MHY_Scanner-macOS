#pragma once

#include <functional>

#include <QMutex>
#include <QThread>

#include "Common/Types.h"
#include "Config/ConfigManager.h"
#include "SDK/MihoyoSDK.h"
#include "Scanner/ScannerBase.hpp"
#include "Scanner/UnifiedScanner.h"

class ScreenScanThread final :
    public QThread,
    public ScannerBase
{
    Q_OBJECT
public:
    ScreenScanThread(QObject* parent);
    ~ScreenScanThread();
    Q_DISABLE_COPY_MOVE(ScreenScanThread)

    void setLoginInfo(const std::string& stoken, const std::string& mid);
    void setLoginInfo(const std::string& stoken, const std::string& mid, const std::string& name);
    void setServerType(const ServerType servertype);
    void setContinuousScan(bool enabled);
    void continueLastLogin();
    void run();
    void stop();

    // 在主线程初始化 ScreenCaptureKit（必须在主线程调用）
    void initScreenCapture();

signals:
    void loginResults(const ScanRet ret);
    void loginConfirm(const GameType gameType, bool b);
    void permissionDenied();

private:
    MihoyoSDK m;
    ConfigManager* m_config;
    void LoginOfficial();
    void LoginBH3BiliBili();
    std::atomic<bool> m_stop;
    std::atomic<bool> m_continuousScan{false};
    std::string m_name;
    std::string m_tk;  // Passport API 所需的 tk
    GameType m_gametype{ GameType::UNKNOW };
    ServerType servertype{};
    ScanRet ret{ ScanRet::UNKNOW };
    const int threadNumber{ 1 };

    UnifiedScanner* m_unifiedScanner{ nullptr };
    std::string m_lastDetectedTicket;

    void handleQRCodeDetected(const std::string& code);
};
