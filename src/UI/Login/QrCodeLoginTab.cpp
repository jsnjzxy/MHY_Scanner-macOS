#include "QrCodeLoginTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThreadPool>
#include <QMetaObject>

#include "Widgets/StyleManager.h"
#include "Network/Api/MihoyoApi.hpp"
#include "../../Core/Utils/UtilMat.hpp"

// UI 常量
namespace QrUIConstants {
    constexpr int ButtonHeight = 40;
    constexpr int DefaultMargin = 20;
    constexpr int ContentMarginH = 30;
    constexpr int ContentMarginTop = 40;
    constexpr int DefaultSpacing = 16;
    constexpr int ButtonAreaTopMargin = 30;
    constexpr int ButtonSpacing = 50;
    constexpr int QRCodeSize = 280;
}

QrCodeLoginTab::QrCodeLoginTab(QWidget* parent)
    : LoginTab(parent)
{
    setupUI();
}

QrCodeLoginTab::~QrCodeLoginTab()
{
    m_qrCodePool.clear();
    stopTimer();
}

void QrCodeLoginTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(QrUIConstants::DefaultSpacing);
    mainLayout->setContentsMargins(QrUIConstants::ContentMarginH,
                                   QrUIConstants::ContentMarginTop,
                                   QrUIConstants::ContentMarginH,
                                   QrUIConstants::DefaultMargin);

    // 提示标签
    m_promptLabel = new QLabel(this);
    m_promptLabel->setText("打开米游社APP，扫一扫登录");
    m_promptLabel->setFont(StyleManager::instance().getBodyFont());
    m_promptLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_promptLabel);

    // 二维码区域 - 使用容器实现居中
    QHBoxLayout* qrCodeContainer = new QHBoxLayout();
    qrCodeContainer->addStretch();

    m_qrCodeLabel = new QLabel(this);
    m_qrCodeLabel->setFixedSize(QrUIConstants::QRCodeSize, QrUIConstants::QRCodeSize);
    m_qrCodeLabel->setAlignment(Qt::AlignCenter);
    m_qrCodeLabel->setText("二维码加载中");
    m_qrCodeLabel->setFont(StyleManager::instance().getBodyFont());
    m_qrCodeLabel->setStyleSheet("QLabel { background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 8px; }");
    qrCodeContainer->addWidget(m_qrCodeLabel);

    qrCodeContainer->addStretch();
    mainLayout->addLayout(qrCodeContainer);

    // 刷新按钮（默认隐藏）
    m_refreshButton = new QPushButton("二维码已过期\n点击刷新二维码", this);
    m_refreshButton->setFont(StyleManager::instance().getButtonFont());
    m_refreshButton->setFixedSize(QrUIConstants::QRCodeSize, QrUIConstants::QRCodeSize);
    m_refreshButton->setVisible(false);
    mainLayout->addWidget(m_refreshButton);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(QrUIConstants::ButtonSpacing);
    buttonLayout->setContentsMargins(0, QrUIConstants::ButtonAreaTopMargin, 0, 0);
    buttonLayout->addStretch();

    m_refreshButton2 = new QPushButton(this);
    m_refreshButton2->setText("刷新");
    m_refreshButton2->setMinimumHeight(QrUIConstants::ButtonHeight);
    m_refreshButton2->setFont(StyleManager::instance().getButtonFont());
    m_refreshButton2->setProperty("secondary", true);
    buttonLayout->addWidget(m_refreshButton2);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 定时器
    m_checkTimer = new QTimer(this);

    // 信号连接
    connect(m_refreshButton, &QPushButton::clicked, this, &QrCodeLoginTab::startLogin);
    connect(m_refreshButton2, &QPushButton::clicked, this, &QrCodeLoginTab::startLogin);
    connect(m_checkTimer, &QTimer::timeout, this, &QrCodeLoginTab::onTimerTimeout);
}

void QrCodeLoginTab::showEvent(QShowEvent* event)
{
    LoginTab::showEvent(event);
    startLogin();
}

void QrCodeLoginTab::reset()
{
    stopTimer();
    m_qrCodeLabel->setText("二维码加载中");
    m_qrCodeLabel->setPixmap(QPixmap());
    m_refreshButton->hide();
}

bool QrCodeLoginTab::validate() const
{
    return true; // 二维码登录不需要输入验证
}

void QrCodeLoginTab::stopTimer()
{
    if (m_checkTimer) {
        m_checkTimer->stop();
    }
}

void QrCodeLoginTab::startLogin()
{
    stopTimer();
    m_refreshButton->hide();
    m_allowDrawQRCode.store(true);
    m_qrCodeLabel->setText("二维码加载中");
    m_qrCodeLabel->setPixmap(QPixmap());
    m_qrCodePool.clear();

    m_qrCodePool.start([this]() {
        try {
            m_allowDrawQRCode.store(false);
            const std::string qrcodeString = GetLoginQrcodeUrl();
            m_ticket = std::string(qrcodeString.data() + qrcodeString.size() - 24, 24);
            m_qrCodeMat = createQrCodeToCvMat(qrcodeString);

            QImage image = CV_8UC1_MatToQImage(m_qrCodeMat);
            if (m_allowDrawQRCode.load()) {
                return;
            }

            QMetaObject::invokeMethod(this, [this, image]() {
                m_qrCodeImage = image;
                QPixmap pixmap = QPixmap::fromImage(m_qrCodeImage);
                QPixmap scaledPixmap = pixmap.scaled(m_qrCodeLabel->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);
                m_qrCodeLabel->setPixmap(scaledPixmap);
                // 启动检查定时器
                if (m_checkTimer && !m_checkTimer->isActive()) {
                    m_checkTimer->start(1000);
                }
            }, Qt::QueuedConnection);

        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(this, [this, e]() {
                emit showMessageRequested(QString("获取二维码失败: %1").arg(e.what()));
            });
        } catch (...) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("获取二维码失败: 未知错误");
            });
        }
    });
}

void QrCodeLoginTab::onTimerTimeout()
{
    QThreadPool::globalInstance()->start([this]() {
        checkLoginState();
    });
}

void QrCodeLoginTab::onRefreshButtonClicked()
{
    startLogin();
}

void QrCodeLoginTab::checkLoginState()
{
    try {
        auto result = GetQRCodeState(m_ticket);

        switch (result.StateType) {
        case decltype(result.StateType)::Init:
            break;

        case decltype(result.StateType)::Scanned:
            QMetaObject::invokeMethod(this, [this]() {
                m_qrCodeLabel->setText("正在登录\n\n请在手机上点击「确认登录」");
            });
            break;

        case decltype(result.StateType)::Confirmed: {
            auto resultStoken = getStokenByGameToken(result.uid, result.token);
            if (resultStoken.has_value()) {
                std::string name = getMysUserName(result.uid);
                const auto& [mid, stoken] = *resultStoken;
                // 先发射登录成功信号，让对话框关闭
                QMetaObject::invokeMethod(this, [this, name, stoken, uid = result.uid, mid]() {
                    emit loginSuccess(name, stoken, uid, mid, "官服");
                }, Qt::QueuedConnection);
                // 然后更新 UI（注意：此时对话框可能已关闭）
                m_qrCodeLabel->setText("登录成功！");
                stopTimer();
            } else {
                QMetaObject::invokeMethod(this, [this]() {
                    emit showMessageRequested("获取STOKEN失败！");
                });
            }
            return;
        }

        case decltype(result.StateType)::Expired: {
            QMetaObject::invokeMethod(this, [this]() {
                // 显示过期二维码
                QImage expiredImage = CV_8UC1_MatToQImage(m_qrCodeMat - cv::Scalar(200));
                QPixmap pixmap = QPixmap::fromImage(expiredImage);
                QPixmap scaledPixmap = pixmap.scaled(m_qrCodeLabel->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);
                m_qrCodeLabel->setPixmap(scaledPixmap);
                m_refreshButton->setVisible(true);
                stopTimer();
            });
            return;
        }

        default:
            break;
        }

    } catch (const std::exception& e) {
        QMetaObject::invokeMethod(this, [this, e]() {
            emit showMessageRequested(QString("检查登录状态失败: %1").arg(e.what()));
        });
    } catch (...) {
        QMetaObject::invokeMethod(this, [this]() {
            emit showMessageRequested("检查登录状态失败: 未知错误");
        });
    }
}
