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

    void setLoginInfo(const std::string& uid, const std::string& token);
    void setLoginInfo(const std::string& uid, const std::string& token, const std::string& name);
    void setServerType(const ServerType servertype);
    void setContinuousScan(bool enabled);  // 设置连续扫码模式
    void continueLastLogin();
    void run();
    void stop();

    // 在主线程初始化 ScreenCaptureKit（必须在主线程调用）
    void initScreenCapture();

signals:
    void loginResults(const ScanRet ret);
    void loginConfirm(const GameType gameType, bool b);
    void permissionDenied();  // 屏幕录制权限被拒绝

private:
    MihoyoSDK m;
    ConfigManager* m_config;
    void LoginOfficial();
    void LoginBH3BiliBili();
    std::atomic<bool> m_stop;
    std::atomic<bool> m_continuousScan{false};  // 连续扫码模式
    std::string m_name;
    GameType m_gametype{ GameType::UNKNOW };
    ServerType servertype{};
    ScanRet ret{ ScanRet::UNKNOW };
    const int threadNumber{ 1 };

    UnifiedScanner* m_unifiedScanner{ nullptr };
    std::string m_lastDetectedTicket;  // 避免重复上报

    /**
     * @brief 处理识别到的二维码
     */
    void handleQRCodeDetected(const std::string& code);
};
