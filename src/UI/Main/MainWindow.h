#pragma once

#include <vector>

#include <nlohmann/json.hpp>
#include <QRunnable>
#include <QMainWindow>

#include "Common/Types.h"
#include "Config/ConfigManager.h"
#include "Network/Api/LiveStreamApi.h"
#include "ScanThreads/ScreenScanThread.h"
#include "ScanThreads/StreamScanThread.h"
#include "ui_MainWindow.h"
#include "Login/LoginDialog.h"
#include "About/AboutDialog.h"
#include "Widgets/DraggableTableWidget.h"

class OnlineUpdate :
    public QThread
{
public:
    OnlineUpdate() = default;
    void run();
    ~OnlineUpdate();
};

class configInitLoad :
    public QThread
{
    Q_OBJECT
public:
    configInitLoad() = default;
    void run();
    ~configInitLoad();
signals:
    void userinfoTrue(bool b);

private:
    ConfigManager* m_config = &ConfigManager::getInstance();
};

class MainWindow :
    public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void insertTableItems(QString uid, QString userName, QString type, QString notes);
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
public slots:
    void AddAccount();
    void SetDefaultAccount();
    void DeleteAccount();

    void pBtstartScreen(bool clicked);
    void pBtStream(bool clicked);
    void pBtStop();

    void checkBoxAutoScreen(bool clicked);
    void checkBoxAutoExit(bool clicked);
    void checkBoxAutoLogin(bool clicked);
    void checkBoxContinuousScan(bool clicked);

    void islogin(const ScanRet ret);
    void loginConfirmTip(const GameType gameType, bool b);
    void getInfo(int x, int y);
    void configInitUpdate(bool b);
    void updateNote(QTableWidgetItem* item);
    void onRowsSwapped(int fromRow, int toRow);
    void onTableContextMenu(const QPoint& pos);
signals:
    void StopScanner();
    void AccountError();
    void LiveStreamLinkError(LiveStreamStatus status);
    void AccountNotSelected();
    void StartScanScreen();
    void StartScanLive();

private:
    void failure();
    int countA = -1;
    Ui::MainWindow ui;
    ConfigManager* m_config = &ConfigManager::getInstance();
    nlohmann::json userinfo;
    ScreenScanThread t1;
    StreamScanThread t2;
    void liveIdError(const LiveStreamStatus status);
    int getSelectedRowIndex();
    bool checkDuplicates(const std::string uid);
    bool GetStreamLink(const std::string& roomid, std::string& url, std::map<std::string, std::string>& heards);
    OnlineUpdate o;
    configInitLoad configinitload;
    void SetWindowToFront();
    LoginDialog* windowLogin{};
};