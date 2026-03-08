#include "MainWindow.h"
#include "Widgets/StyleManager.h"
#include "Scanner/ScreenCapture/ScreenCapture.h"

#include <fstream>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#endif

#include <QMessageBox>
#include <QWindow>
#include <QRegularExpressionValidator>
#include <QStringList>
#include <QDir>
#include <QThreadPool>
#include <nlohmann/json.hpp>
#include <QDesktopServices>
#include <QMenu>

#include "SDK/MihoyoSDK.h"
#include "Network/Api/MihoyoApi.hpp"
#include "Network/Api/BilibiliApi.hpp"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    t1(this),
    t2(this)
{
    ui.setupUi(this);

    // 设置表格样式
    ui.tableWidget->setStyleSheet(StyleManager::instance().getTableWidgetStyle());

    connect(ui.action1_3, &QAction::triggered, this, &MainWindow::AddAccount);
    connect(ui.action1_4, &QAction::triggered, this, &MainWindow::SetDefaultAccount);
    connect(ui.action2_3, &QAction::triggered, this, &MainWindow::DeleteAccount);
    connect(ui.action1_2, &QAction::triggered, this, [this]() {
        AboutDialog windowAbout(this);
        windowAbout.exec();
    });
    connect(ui.action2_2, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/Theresa-0328/MHY_Scanner/issues"));
    });
    connect(ui.action1_5, &QAction::triggered, this, []() {
        // macOS: 使用与 ConfigDate 相同的路径逻辑
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
        {
            std::filesystem::path exePath(path);
            std::filesystem::path contentsDir = exePath.parent_path().parent_path();
            std::filesystem::path configDir = contentsDir / "Resources" / "Config";
            // 确保 Config 目录存在
            if (!std::filesystem::exists(configDir))
            {
                std::filesystem::create_directories(configDir);
            }
            QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(configDir.string())));
        }
    });
    connect(ui.pBtstartScreen, &QPushButton::clicked, this, &MainWindow::pBtstartScreen);
    connect(this, &MainWindow::StopScanner, this, &MainWindow::pBtStop);
    connect(this, &MainWindow::StartScanScreen, this, [&]() {
        ui.pBtstartScreen->setText("监视屏幕中");
        ui.pBtstartScreen->setEnabled(true);
    });
    connect(this, &MainWindow::AccountError, this, [&]() {
        failure();
        pBtStop();
    });
    connect(this, &MainWindow::StartScanLive, this, [&]() {
        ui.pBtStream->setText("监视直播中");
        ui.pBtStream->setEnabled(true);
    });
    connect(this, &MainWindow::LiveStreamLinkError, this, [&](LiveStreamStatus status) {
        liveIdError(status);
    });
    connect(this, &MainWindow::AccountNotSelected, this, [&]() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("提示");
        msgBox.setText("没有选择任何账号");
        StyleManager::setupMessageBox(msgBox);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
        pBtStop();
    });
    connect(ui.checkBoxAutoScreen, &QCheckBox::clicked, this, &MainWindow::checkBoxAutoScreen);
    connect(ui.checkBoxAutoExit, &QCheckBox::clicked, this, &MainWindow::checkBoxAutoExit);
    connect(ui.checkBoxAutoLogin, &QCheckBox::clicked, this, &MainWindow::checkBoxAutoLogin);
    connect(ui.checkBoxContinuousScan, &QCheckBox::clicked, this, &MainWindow::checkBoxContinuousScan);
    connect(ui.pBtStream, &QPushButton::clicked, this, &MainWindow::pBtStream);
    connect(ui.tableWidget, &QTableWidget::cellClicked, this, &MainWindow::getInfo);
    connect(&t1, &ScreenScanThread::loginResults, this, &MainWindow::islogin);
    connect(&t1, &ScreenScanThread::loginConfirm, this, &MainWindow::loginConfirmTip);
#ifdef __APPLE__
    connect(&t1, &ScreenScanThread::permissionDenied, this, [this]() {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("权限不足");
        msgBox.setText("需要屏幕录制权限才能监视屏幕。\n\n请在「系统设置 → 隐私与安全性 → 屏幕录制」中\n为本应用授予权限，然后重新启动应用。");
        StyleManager::setupMessageBox(msgBox);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
    });
#endif
    connect(&t2, &StreamScanThread::loginResults, this, &MainWindow::islogin);
    connect(&t2, &StreamScanThread::loginConfirm, this, &MainWindow::loginConfirmTip);
    connect(&configinitload, &configInitLoad::userinfoTrue, this, &MainWindow::configInitUpdate);
    connect(ui.tableWidget, &QTableWidget::itemChanged, this, &MainWindow::updateNote);
    // 连接拖拽交换信号
    connect(ui.tableWidget, &DraggableTableWidget::rowsSwapped, this, &MainWindow::onRowsSwapped);
    // 连接右键菜单信号
    connect(ui.tableWidget, &DraggableTableWidget::contextMenuRequested, this, &MainWindow::onTableContextMenu);
    connect(ui.comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        userinfo["live_platform"] = index;
        m_config->updateConfig(userinfo.dump());
    });

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    o.start();
    ui.tableWidget->setColumnCount(5);
    QStringList header;
    header << "序号"
           << "UID"
           << "用户名"
           << "类型"
           << "备注";
    ui.tableWidget->setHorizontalHeaderLabels(header);
    ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui.tableWidget->setColumnWidth(0, 50);
    ui.tableWidget->setColumnWidth(1, 100);
    ui.tableWidget->setColumnWidth(2, 100);
    ui.tableWidget->setColumnWidth(3, 100);
    ui.tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    ui.tableWidget->verticalHeader()->setVisible(false);
    ui.tableWidget->horizontalHeader()->setFont(StyleManager::instance().getSmallFont());
    ui.tableWidget->setAlternatingRowColors(true);

    ui.label_3->setText(SCAN_VER);

    ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);

    ui.lineEditLiveId->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]+$"), this));
    ui.lineEditLiveId->setClearButtonEnabled(true);

    configinitload.start();

    //trrlog::SetLogFile("log.txt");
    //trrlog::Log_debug("UI Initialization completed");
}

MainWindow::~MainWindow()
{
    t1.stop();
    t2.stop();
}

void MainWindow::insertTableItems(QString uid, QString userName, QString type, QString notes)
{
    QTableWidgetItem* item[5]{};
    int nCount = ui.tableWidget->rowCount();
    ui.tableWidget->insertRow(nCount);
    item[0] = new QTableWidgetItem(QString("%1").arg(nCount + 1));
    ui.tableWidget->setItem(nCount, 0, item[0]);
    item[1] = new QTableWidgetItem(uid);
    ui.tableWidget->setItem(nCount, 1, item[1]);
    item[2] = new QTableWidgetItem(userName);
    ui.tableWidget->setItem(nCount, 2, item[2]);
    item[3] = new QTableWidgetItem(type);
    ui.tableWidget->setItem(nCount, 3, item[3]);
    item[4] = new QTableWidgetItem(notes);
    ui.tableWidget->setItem(nCount, 4, item[4]);

    for (int i = 0; i < 4; i++)
    {
        QTableWidgetItem* item1 = ui.tableWidget->item(nCount, i);
        item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
    }
}

void MainWindow::AddAccount()
{
    if (t1.isRunning() || t2.isRunning())
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("错误");
        msgBox.setText("请先停止识别！");
        StyleManager::setupMessageBox(msgBox);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
        return;
    }
    LoginDialog* newLoginDialog = new LoginDialog(this);
    QMetaObject::Connection conn = connect(newLoginDialog, &LoginDialog::loginSuccess, this, [this, newLoginDialog](const std::string name, const std::string token, const std::string uid, const std::string mid, const std::string type) {
        // 先断开连接，防止信号重复触发
        newLoginDialog->disconnect();

        if (checkDuplicates(uid.data()))
        {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("提示");
            msgBox.setText("该账号已添加，无需重复添加");
            StyleManager::setupMessageBox(msgBox);
            msgBox.addButton("确定", QMessageBox::AcceptRole);
            msgBox.exec();
            return;
        }
        insertTableItems(QString::fromStdString(uid), QString::fromStdString(name), QString::fromStdString(type), "");
        QThreadPool::globalInstance()->start([this, token, uid, name, type, mid] {
            int num{ userinfo["num"] };
            userinfo["account"][num]["access_key"] = token;
            userinfo["account"][num]["uid"] = uid;
            userinfo["account"][num]["name"] = name;
            userinfo["account"][num]["type"] = type;
            userinfo["account"][num]["note"] = "";
            userinfo["account"][num]["mid"] = mid;
            userinfo["num"] = num + 1;
            m_config->updateConfig(userinfo.dump());
        });
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("提示");
        msgBox.setText("添加成功");
        StyleManager::setupMessageBox(msgBox);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
    });
    newLoginDialog->show();
}

void MainWindow::pBtstartScreen(bool clicked)
{
    if (!clicked)
    {
        emit StopScanner();
        return;
    }

#ifdef __APPLE__
    // macOS: 在主线程检测屏幕录制权限
    if (!ScreenCapture::hasScreenCapturePermission())
    {
        ScreenCapture::requestScreenCapturePermission();
        return;
    }
    // 在主线程初始化 ScreenCaptureKit
    t1.initScreenCapture();
#endif

    //FIXME 没有及时更新
    if (countA == -1)
    {
        emit AccountNotSelected();
        return;
    }

    ui.pBtstartScreen->setEnabled(false);
    ui.pBtStream->setEnabled(false);
    ui.pBtstartScreen->setText("加载中。。。");
    QApplication::processEvents();
    QThreadPool::globalInstance()->start([&, clicked]() {
        if (std::string type = userinfo["account"][countA]["type"].get<std::string>(); type == "官服")
        {
            std::string stoken = userinfo["account"][countA]["access_key"].get<std::string>();
            std::string uid = userinfo["account"][countA]["uid"].get<std::string>();
            std::string mid = userinfo["account"][countA]["mid"].get<std::string>();
            std::string gameToken;
            auto result = GetGameTokenByStoken(stoken, mid);
            if (result)
            {
                gameToken = *result;
            }
            else
            {
                emit AccountError();
                return;
            }
            t1.setServerType(ServerType::Official);
            t1.setLoginInfo(uid, gameToken);
        }
        else if (type == "崩坏3B服")
        {
            std::string stoken{ userinfo["account"][countA]["access_key"].get<std::string>() };
            std::string uid{ userinfo["account"][countA]["uid"].get<std::string>() };
            //可用性检查
            auto result{ BH3API::BILI::GetUserInfo(uid, stoken) };
            if (result.code != 0)
            {
                emit AccountError();
                return;
            }
            t1.setServerType(ServerType::BH3_BiliBili);
            t1.setLoginInfo(uid, stoken, result.uname);
        }
        t1.start();
        emit StartScanScreen();
    });
}

void MainWindow::pBtStream(bool clicked)
{
    ui.pBtstartScreen->setEnabled(false);
    ui.pBtStream->setEnabled(false);
    ui.pBtStream->setText("加载中。。。");
    QApplication::processEvents();

    QThreadPool::globalInstance()->start([&, clicked]() {
        if (!clicked)
        {
            emit StopScanner();
            return;
        }
        if (countA == -1)
        {
            emit AccountNotSelected();
            return;
        }
        std::string stream_link;
        std::map<std::string, std::string> heards;
        //检查直播间状态
        if (!GetStreamLink(ui.lineEditLiveId->text().toStdString(), stream_link, heards))
        {
            emit StopScanner();
            return;
        }
        else
        {
            t2.setUrl(stream_link, heards);
        }
        if (const std::string& type = userinfo["account"][countA]["type"]; type == "官服")
        {
            std::string stoken = userinfo["account"][countA]["access_key"].get<std::string>();
            std::string uid = userinfo["account"][countA]["uid"].get<std::string>();
            std::string mid = userinfo["account"][countA]["mid"].get<std::string>();
            std::string gameToken;
            auto result = GetGameTokenByStoken(stoken, mid);
            if (result)
            {
                gameToken = *result;
            }
            else
            {
                emit AccountError();
                return;
            }
            t2.setServerType(ServerType::Official);
            t2.setLoginInfo(uid, gameToken);
        }
        else if (type == "崩坏3B服")
        {
            std::string stoken{ userinfo["account"][countA]["access_key"].get<std::string>() };
            std::string uid{ userinfo["account"][countA]["uid"].get<std::string>() };
            //可用性检查
            auto result{ BH3API::BILI::GetUserInfo(uid, stoken) };
            if (result.code != 0)
            {
                emit AccountError();
                return;
            }
            t2.setServerType(ServerType::BH3_BiliBili);
            t2.setLoginInfo(uid, stoken, result.uname);
        }
        t2.start();
        emit StartScanLive();
    });
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    t1.stop();
    t2.stop();
}

void MainWindow::showEvent(QShowEvent* event)
{
}

void MainWindow::islogin(const ScanRet ret)
{
    QMessageBox* messageBox = new QMessageBox(this);
    StyleManager::setupMessageBox(*messageBox);
    auto Show_QMessageBox = [&](const QString& title, const QString& text) {
        messageBox->setWindowTitle(title);
        messageBox->setText(text);
        messageBox->addButton("确定", QMessageBox::AcceptRole);
        messageBox->show();
    };

    // 连续扫码模式下，成功后继续监视，失败也继续
    bool continuousScan = ui.checkBoxContinuousScan->isChecked();

    if (ret == ScanRet::SUCCESS && userinfo["auto_exit"].is_boolean() && (bool)userinfo["auto_exit"])
    {
        exit(0);
    }

    // 连续扫码模式：成功或失败都继续监视
    // 但直播断开时需要恢复按钮状态并弹窗提示
    if (continuousScan)
    {
        if (ret == ScanRet::SUCCESS)
        {
            // 成功，静默继续监视
            return;
        }
        // 直播断开时，恢复按钮状态并弹窗提示
        if (ret == ScanRet::LIVESTOP)
        {
            pBtStop();
            SetWindowToFront();
            Show_QMessageBox("提示", "直播中断!");
            return;
        }
        // 其他失败，继续监视，不弹窗
        return;
    }

    // 非连续扫码模式：原有逻辑
    pBtStop();
    SetWindowToFront();
    switch (ret)
    {
    case ScanRet::UNKNOW:
        break;
    case ScanRet::FAILURE_1:
        Show_QMessageBox("提示", "扫码失败!");
        break;
    case ScanRet::FAILURE_2:
        Show_QMessageBox("提示", "扫码二次确认失败!");
        break;
    case ScanRet::LIVESTOP:
        Show_QMessageBox("提示", "直播中断!");
        break;
    case ScanRet::STREAMERROR:
        Show_QMessageBox("提示", "直播流初始化失败!");
        break;
    case ScanRet::SUCCESS:
        Show_QMessageBox("提示", "扫码成功!");
        break;
    default:
        break;
    }
}

void MainWindow::loginConfirmTip(const GameType gameType, bool b)
{
    QString info("正在使用账号" + ui.lineEditUname->text());
    switch (gameType)
    {
    case GameType::Honkai3:
        info += "\n登录崩坏3\n";
        break;
    case GameType::Honkai3_BiliBili:
        info += "\n登录BiliBili崩坏3\n";
        break;
    case GameType::Genshin:
        info += "\n登录原神\n";
        break;
    case GameType::HonkaiStarRail:
        info += "\n登录星穹铁道\n";
        break;
    case GameType::ZenlessZoneZero:
        info += "\n登录绝区零\n";
        break;
    default:
        break;
    }

    bool autoLogin = ui.checkBoxAutoLogin->isChecked();

    // 只有自动二次确认才自动确认，连续扫码不影响二次确认行为
    if (autoLogin)
    {
        QThreadPool::globalInstance()->start([this, b] {
            if (b)
            {
                t1.continueLastLogin();
            }
            else
            {
                t2.continueLastLogin();
            }
        });
        return;
    }

    // 非自动确认：弹窗让用户确认
    SetWindowToFront();
    QMessageBox* messageBox = new QMessageBox(this);
    StyleManager::setupMessageBox(*messageBox);
    messageBox->setWindowTitle("登录确认");
    messageBox->setText(info + "确认登录？");
    QAbstractButton* yesButton = messageBox->addButton("登录", QMessageBox::YesRole);
    QAbstractButton* noButton = messageBox->addButton("取消", QMessageBox::NoRole);
    messageBox->exec();

    bool continuousScan = ui.checkBoxContinuousScan->isChecked();

    // 连续扫码模式下不停止监视
    if (!continuousScan)
    {
        pBtStop();
    }

    if (messageBox->clickedButton() != yesButton)
    {
        return;
    }
    QThreadPool::globalInstance()->start([this, b] {
        if (b)
        {
            t1.continueLastLogin();
        }
        else
        {
            t2.continueLastLogin();
        }
    });
}

void MainWindow::checkBoxAutoScreen(bool clicked)
{
    int state = ui.checkBoxAutoScreen->checkState();
    if (!userinfo["last_account"].is_number_integer() || (int)userinfo["last_account"] == 0)
    {
        ui.checkBoxAutoScreen->setChecked(false);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("提示");
        msgBox.setText("你没有选择默认账号!");
        StyleManager::setupMessageBox(msgBox);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
        return;
    }
    if (state == Qt::Checked)
    {
        ui.checkBoxAutoScreen->setChecked(true);
        userinfo["auto_start"] = true;
    }
    else if (state == Qt::Unchecked)
    {
        ui.checkBoxAutoScreen->setChecked(false);
        userinfo["auto_start"] = false;
    }
    m_config->updateConfig(userinfo.dump());
}

void MainWindow::checkBoxAutoExit(bool clicked)
{
    int state = ui.checkBoxAutoExit->checkState();
    if (state == Qt::Checked)
    {
        userinfo["auto_exit"] = true;
        // 连续扫码与自动退出互斥，自动取消连续扫码
        ui.checkBoxContinuousScan->setChecked(false);
        userinfo["continuous_scan"] = false;
    }
    else if (state == Qt::Unchecked)
    {
        userinfo["auto_exit"] = false;
    }
    m_config->updateConfig(userinfo.dump());
}

void MainWindow::checkBoxAutoLogin(bool clicked)
{
    int state = ui.checkBoxAutoLogin->checkState();
    if (state == Qt::Checked)
    {
        userinfo["auto_login"] = true;
    }
    else if (state == Qt::Unchecked)
    {
        userinfo["auto_login"] = false;
    }
    m_config->updateConfig(userinfo.dump());
}

void MainWindow::checkBoxContinuousScan(bool clicked)
{
    int state = ui.checkBoxContinuousScan->checkState();
    if (state == Qt::Checked)
    {
        userinfo["continuous_scan"] = true;
        // 连续扫码与自动退出互斥，自动取消自动退出
        ui.checkBoxAutoExit->setChecked(false);
        userinfo["auto_exit"] = false;
        // 设置连续扫码模式
        t1.setContinuousScan(true);
        t2.setContinuousScan(true);
    }
    else if (state == Qt::Unchecked)
    {
        userinfo["continuous_scan"] = false;
        // 取消连续扫码模式
        t1.setContinuousScan(false);
        t2.setContinuousScan(false);
    }
    m_config->updateConfig(userinfo.dump());
}

void MainWindow::liveIdError(const LiveStreamStatus status)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("提示");
    StyleManager::setupMessageBox(msgBox);
    msgBox.addButton("确定", QMessageBox::AcceptRole);

    switch (status)
    {
        using enum LiveStreamStatus;
    case Absent:
    {
        msgBox.setText("直播间不存在!");
        msgBox.exec();
        return;
    }
    case NotLive:
    {
        msgBox.setText("直播间未开播！");
        msgBox.exec();
        return;
    }
    case Error:
    {
        msgBox.setText("直播间未知错误!");
        msgBox.exec();
        return;
    }
    default:
        return;
    }
}

int MainWindow::getSelectedRowIndex()
{
    QList<QTableWidgetItem*> item = ui.tableWidget->selectedItems();
    if (item.count() == 0)
    {
        return -1;
    }
    return ui.tableWidget->row(item.at(0));
}

bool MainWindow::checkDuplicates(const std::string uid)
{
    for (int i = 0; i < (int)userinfo["num"]; i++)
    {
        std::string m_uid = userinfo["account"][i]["uid"];
        if (uid == m_uid)
        {
            return true;
        }
    }
    return false;
}

bool MainWindow::GetStreamLink(const std::string& roomid, std::string& url, std::map<std::string, std::string>& heards)
{
    std::uint32_t live_type = ui.comboBox->currentIndex();
    switch (live_type)
    {
    case 0:
    {
        LiveDouyin live(roomid);
        if (LiveStreamStatus status = live.GetLiveStreamStatus(); status != LiveStreamStatus::Normal)
        {
            emit LiveStreamLinkError(status);
            return false;
        }
        url = live.GetLiveStreamLink();
        break;
    }
    case 1:
    {
        LiveBili live(roomid);
        if (LiveStreamStatus status = live.GetLiveStreamStatus(); status != LiveStreamStatus::Normal)
        {
            emit LiveStreamLinkError(status);
            return false;
        }
        heards = {
            { "user_agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) \
            Chrome/110.0.0.0 Safari/537.36 Edg/110.0.1587.41" },
            { "referer", "https://live.bilibili.com" }
        };
        url = live.GetLiveStreamLink();
        break;
    }
    default:
        break;
    }
    return true;
}
void MainWindow::SetWindowToFront()
{
    raise();
    activateWindow();
}
void MainWindow::failure()
{
    QMessageBox* messageBox = new QMessageBox(this);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->setWindowTitle("提示");
    messageBox->setText("登录状态失效，\n请重新添加账号！");
    StyleManager::setupMessageBox(*messageBox);
    messageBox->addButton("确定", QMessageBox::AcceptRole);
    messageBox->show();
}

void MainWindow::configInitUpdate(bool b)
{
    ui.tableWidget->blockSignals(true);
    if (!b)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("错误");
        StyleManager::setupMessageBox(msgBox);
        msgBox.setText("配置文件错误！\n重置配置文件为空？");
        QPushButton* yesBtn = msgBox.addButton("是", QMessageBox::YesRole);
        QPushButton* noBtn = msgBox.addButton("否", QMessageBox::NoRole);
        msgBox.exec();

        if (msgBox.clickedButton() == yesBtn)
        {
            m_config->defaultConfig();
        }
        else
        {
            QMessageBox errorMsg(this);
            errorMsg.setWindowTitle("错误");
            errorMsg.setText("配置文件错误！\n无法继续运行！");
            StyleManager::setupMessageBox(errorMsg);
            errorMsg.addButton("确定", QMessageBox::AcceptRole);
            errorMsg.exec();
            exit(1);
        }
    }
    userinfo = nlohmann::json::parse(m_config->getConfig());
    for (int i = 0; i < (int)userinfo["num"]; i++)
    {
        insertTableItems(
            QString::fromStdString(userinfo["account"][i]["uid"]),
            QString::fromStdString(userinfo["account"][i]["name"]),
            QString::fromStdString(userinfo["account"][i]["type"]),
            QString::fromStdString(userinfo["account"][i]["note"]));
    }
    // 自动选择默认账号
    if (static_cast<int>(userinfo["last_account"]) != 0)
    {
        countA = static_cast<int>(userinfo["last_account"]) - 1;
        ui.lineEditUname->setText(QString::fromStdString(userinfo["account"][countA]["name"].get<std::string>()));
        ui.tableWidget->selectRow(countA);
    }
    // 自动开始监视 - 使用安全的bool检查方式
    if (userinfo["auto_start"].is_boolean() && (bool)userinfo["auto_start"] && static_cast<int>(userinfo["last_account"]) != 0)
    {
        ui.pBtstartScreen->clicked(true);
        ui.pBtstartScreen->setChecked(true);
        ui.checkBoxAutoScreen->setChecked(true);
    }
    if (userinfo["auto_exit"].is_boolean() && (bool)userinfo["auto_exit"])
    {
        ui.checkBoxAutoExit->setChecked(true);
    }
    if (userinfo["auto_login"].is_boolean() && (bool)userinfo["auto_login"])
    {
        ui.checkBoxAutoLogin->setChecked(true);
    }
    if (userinfo["continuous_scan"].is_boolean() && (bool)userinfo["continuous_scan"])
    {
        ui.checkBoxContinuousScan->setChecked(true);
        t1.setContinuousScan(true);
        t2.setContinuousScan(true);
    }
    // 恢复上次选择的直播平台
    if (!userinfo["live_platform"].is_null())
    {
        ui.comboBox->setCurrentIndex(static_cast<int>(userinfo["live_platform"]));
    }
    ui.tableWidget->blockSignals(false);
}

void MainWindow::updateNote(QTableWidgetItem* item)
{
    QString text = item->text();
    userinfo["account"][item->row()]["note"] = text.toStdString();
    m_config->updateConfig(userinfo.dump());
}

void MainWindow::onRowsSwapped(int fromRow, int toRow)
{
    // 确保行号有效
    if (fromRow < 0 || toRow < 0 || fromRow >= ui.tableWidget->rowCount() || toRow >= ui.tableWidget->rowCount())
    {
        return;
    }

    // 交换 JSON 配置中的账号数据
    nlohmann::json temp = userinfo["account"][fromRow];
    userinfo["account"][fromRow] = userinfo["account"][toRow];
    userinfo["account"][toRow] = temp;

    // 更新配置文件
    m_config->updateConfig(userinfo.dump());

    // 更新当前选中账号索引（如果需要）
    if (countA == fromRow)
    {
        countA = toRow;
    }
    else if (countA == toRow)
    {
        countA = fromRow;
    }

    // 更新默认账号索引
    int lastAccount = static_cast<int>(userinfo["last_account"]);
    if (lastAccount > 0)
    {
        if (lastAccount - 1 == fromRow)
        {
            userinfo["last_account"] = toRow + 1;
        }
        else if (lastAccount - 1 == toRow)
        {
            userinfo["last_account"] = fromRow + 1;
        }
        m_config->updateConfig(userinfo.dump());
    }
}

void MainWindow::onTableContextMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("账号操作"), this);

    QAction* addAction = contextMenu.addAction(tr("添加账号"));

    QTableWidgetItem* item = ui.tableWidget->itemAt(pos);
    int row = item ? item->row() : -1;

    if (item)
    {
        ui.tableWidget->selectRow(row);

        // 检查当前行是否是默认账号
        int lastAccount = static_cast<int>(userinfo["last_account"]);
        bool isDefault = (lastAccount == row + 1);

        QAction* defaultAction = nullptr;
        if (isDefault)
        {
            defaultAction = contextMenu.addAction(tr("取消默认账号"));
        }
        else
        {
            defaultAction = contextMenu.addAction(tr("设为默认账号"));
        }

        QAction* deleteAction = contextMenu.addAction(tr("删除账号"));

        connect(defaultAction, &QAction::triggered, this, [this, row, isDefault]() {
            if (isDefault)
            {
                userinfo["last_account"] = 0;
                m_config->updateConfig(userinfo.dump());
                QMessageBox msgBox(this);
                msgBox.setWindowTitle("提示");
                msgBox.setText("已取消默认账号");
                StyleManager::setupMessageBox(msgBox);
                msgBox.addButton("确定", QMessageBox::AcceptRole);
                msgBox.exec();
            }
            else
            {
                userinfo["last_account"] = row + 1;
                m_config->updateConfig(userinfo.dump());
                QMessageBox msgBox(this);
                msgBox.setWindowTitle("提示");
                msgBox.setText("已设置默认账号：" + QString::fromStdString(userinfo["account"][row]["name"].get<std::string>()));
                StyleManager::setupMessageBox(msgBox);
                msgBox.addButton("确定", QMessageBox::AcceptRole);
                msgBox.exec();
            }
        });

        connect(deleteAction, &QAction::triggered, this, [this, row]() {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("确认删除");
            msgBox.setText("确定要删除该账号吗？");
            StyleManager::setupMessageBox(msgBox);
            QPushButton* yesBtn = msgBox.addButton("是", QMessageBox::YesRole);
            QPushButton* noBtn = msgBox.addButton("否", QMessageBox::NoRole);
            msgBox.exec();

            if (msgBox.clickedButton() == yesBtn)
            {
                int num = static_cast<int>(userinfo["num"]);
                if (row < num)
                {
                    userinfo["account"].erase(row);
                    userinfo["num"] = num - 1;
                    m_config->updateConfig(userinfo.dump());
                    ui.tableWidget->removeRow(row);

                    // 更新序号列
                    for (int i = 0; i < ui.tableWidget->rowCount(); ++i)
                    {
                        QTableWidgetItem* numItem = ui.tableWidget->item(i, 0);
                        if (numItem)
                        {
                            numItem->setText(QString::number(i + 1));
                        }
                    }

                    // 更新默认账号索引
                    int lastAccount = static_cast<int>(userinfo["last_account"]);
                    if (lastAccount > 0)
                    {
                        if (lastAccount - 1 == row)
                        {
                            userinfo["last_account"] = 0;
                        }
                        else if (lastAccount - 1 > row)
                        {
                            userinfo["last_account"] = lastAccount - 1;
                        }
                        m_config->updateConfig(userinfo.dump());
                    }

                    // 更新当前选中账号索引
                    if (countA == row)
                    {
                        countA = -1;
                    }
                    else if (countA > row)
                    {
                        countA--;
                    }
                }
            }
        });
    }

    connect(addAction, &QAction::triggered, this, &MainWindow::AddAccount);

    // 在鼠标位置显示菜单
    contextMenu.exec(ui.tableWidget->mapToGlobal(pos));
}

void OnlineUpdate::run()
{
    MihoyoSDK m;
    m.setOAServer();
}

OnlineUpdate::~OnlineUpdate()
{
    wait();
}

void configInitLoad::run()
{
    const std::string& config = m_config->getConfig();
    nlohmann::json data;
    try
    {
        data = nlohmann::json::parse(config);
        // 只验证核心字段是否存在
        int num = data["num"];
        for (int i = 0; i < num; i++)
        {
            [[maybe_unused]] std::string s = data["account"][i]["uid"];
            s = data["account"][i]["name"].get<std::string>();
            s = data["account"][i]["type"].get<std::string>();
            s = data["account"][i]["note"].get<std::string>();
            s = data["account"][i]["mid"].get<std::string>();
        };
        [[maybe_unused]] int i = data["last_account"];
        // 不验证可选的bool字段，它们可能不存在（旧配置文件）
        // 这些字段在configInitUpdate中会使用默认值
        emit userinfoTrue(true);
    }
    catch (...)
    {
        emit userinfoTrue(false);
    }
}

configInitLoad::~configInitLoad()
{
    wait();
}

void MainWindow::DeleteAccount()
{
    int row = ui.tableWidget->currentRow();
    if (row < 0)
    {
        emit AccountNotSelected();
        return;
    }

    nlohmann::json userinfo;
    userinfo = nlohmann::json::parse(m_config->getConfig());
    int num = static_cast<int>(userinfo["num"]);
    if (row >= num)
    {
        return;
    }
    userinfo["account"].erase(row);
    userinfo["num"] = num - 1;
    m_config->updateConfig(userinfo.dump());
    ui.tableWidget->removeRow(row);
}

void MainWindow::SetDefaultAccount()
{
    int row = ui.tableWidget->currentRow();
    if (row < 0)
    {
        emit AccountNotSelected();
        return;
    }
    userinfo["last_account"] = row + 1;
    m_config->updateConfig(userinfo.dump());
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("提示");
    msgBox.setText("已设置默认账号：" + QString::fromStdString(userinfo["account"][row]["name"].get<std::string>()));
    StyleManager::setupMessageBox(msgBox);
    msgBox.addButton("确定", QMessageBox::AcceptRole);
    msgBox.exec();
}

void MainWindow::getInfo(int row, int column)
{
    nlohmann::json userinfo;
    userinfo = nlohmann::json::parse(m_config->getConfig());
    int num = static_cast<int>(userinfo["num"]);
    if (row < 0 || row >= num)
    {
        return;
    }
    countA = row;
    ui.lineEditUname->setText(QString::fromStdString(userinfo["account"][row]["name"].get<std::string>()));
    if (static_cast<std::string>(userinfo["account"][row]["type"]) == "官服")
    {
        ui.comboBox->setCurrentIndex(0);
    }
    else
    {
        ui.comboBox->setCurrentIndex(1);
    }
}

void MainWindow::pBtStop()
{
    t1.stop();
    t2.stop();
    ui.pBtstartScreen->setText("监视屏幕");
    ui.pBtStream->setText("监视直播间");
    ui.pBtstartScreen->setChecked(false);
    ui.pBtStream->setChecked(false);
    ui.pBtstartScreen->setEnabled(true);
    ui.pBtStream->setEnabled(true);
}
