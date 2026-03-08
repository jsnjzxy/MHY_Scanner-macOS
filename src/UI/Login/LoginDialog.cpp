#include "LoginDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMetaObject>
#include <QThreadPool>
#include <QKeyEvent>

#include "Widgets/StyleManager.h"

LoginDialog::LoginDialog(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Dialog);
    setWindowModality(Qt::WindowModal);
    setMinimumSize(QSize(500, 580));
    setMaximumSize(QSize(500, 580));
    setWindowTitle("添加账号");

    setupUI();
}

LoginDialog::~LoginDialog()
{
    m_qrCodePool.clear();
}

void LoginDialog::setupUI()
{
    // 主垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 设置标题栏
    setupTitleBar();
    mainLayout->addLayout(static_cast<QHBoxLayout*>(layout()->takeAt(0)));

    // TabWidget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumSize(QSize(500, 530));
    m_tabWidget->setMaximumSize(QSize(500, 530));
    m_tabWidget->setFont(StyleManager::instance().getBodyFont());

    // 创建并添加各个 Tab
    m_smsTab = new SmsLoginTab(this);
    m_qrCodeTab = new QrCodeLoginTab(this);
    m_cookieTab = new CookieLoginTab(this);
    m_biliTab = new BiliLoginTab(this);

    m_tabWidget->addTab(m_smsTab, "短信登录");
    m_tabWidget->addTab(m_qrCodeTab, "扫码登录");
    m_tabWidget->addTab(m_cookieTab, "Cookie登录");
    m_tabWidget->addTab(m_biliTab, "Bilibili崩坏3登录");

    mainLayout->addWidget(m_tabWidget);

    // 信号连接
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &LoginDialog::onTabChanged);

    // 连接各个 Tab 的信号
    connect(m_smsTab, &SmsLoginTab::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(m_smsTab, &SmsLoginTab::showMessageRequested, this, &LoginDialog::showMessage);

    connect(m_qrCodeTab, &QrCodeLoginTab::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(m_qrCodeTab, &QrCodeLoginTab::showMessageRequested, this, &LoginDialog::showMessage);

    connect(m_cookieTab, &CookieLoginTab::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(m_cookieTab, &CookieLoginTab::showMessageRequested, this, &LoginDialog::showMessage);

    connect(m_biliTab, &BiliLoginTab::loginSuccess, this, &LoginDialog::onLoginSuccess);
    connect(m_biliTab, &BiliLoginTab::showMessageRequested, this, &LoginDialog::showMessage);
}

void LoginDialog::setupTitleBar()
{
    QHBoxLayout* titleBarLayout = new QHBoxLayout();
    titleBarLayout->setContentsMargins(15, 10, 15, 10);
    titleBarLayout->setSpacing(0);

    QLabel* titleLabel = new QLabel("添加账号", this);
    titleLabel->setFont(StyleManager::instance().getTitleFont());
    titleLabel->setStyleSheet("color: #333333; background: transparent;");
    titleBarLayout->addWidget(titleLabel);
    titleBarLayout->addStretch();

    // 右上角关闭按钮 - 简约风格
    m_closeButton = new QPushButton(this);
    m_closeButton->setFixedSize(20, 20);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setToolTip("关闭 (Esc)");
    m_closeButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            border-radius: 3px;
            background: #F0F0F0;
            color: #999999;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #FF4D4F;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background: #FF7875;
        }
    )");
    m_closeButton->setText("✕");
    titleBarLayout->addWidget(m_closeButton);

    connect(m_closeButton, &QPushButton::clicked, this, &LoginDialog::onCloseButtonClicked);

    // 将标题栏布局添加到主布局
    QVBoxLayout* mainLayout = static_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->insertLayout(0, titleBarLayout);
    }
}

void LoginDialog::closeEvent(QCloseEvent* event)
{
    deleteLater();
}

void LoginDialog::keyPressEvent(QKeyEvent* event)
{
    // ESC 键关闭窗口
    if (event->key() == Qt::Key_Escape) {
        m_qrCodeTab->stopTimer();
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void LoginDialog::onCloseButtonClicked()
{
    m_qrCodeTab->stopTimer();
    close();
}

void LoginDialog::onTabChanged(int index)
{
    m_qrCodeTab->stopTimer();

    // 当切换到扫码登录 Tab 时，开始登录流程
    if (index == 1) {  // 扫码登录是第二个 Tab
        m_qrCodeTab->startLogin();
    }
}

void LoginDialog::onLoginSuccess(const std::string& name, const std::string& token,
                                  const std::string& uid, const std::string& mid,
                                  const std::string& type)
{
    emit loginSuccess(name, token, uid, mid, type);
    close();
}

void LoginDialog::showMessage(const QString& message)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("提示");
    msgBox.setText(message);
    StyleManager::setupMessageBox(msgBox);
    msgBox.addButton("确定", QMessageBox::AcceptRole);
    msgBox.exec();
}
