#include "SmsLoginTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QThreadPool>

#include "Widgets/StyleManager.h"

#include "Network/Api/MihoyoApi.hpp"
#include "Common/Constants.h"

// UI 常量
namespace SmsUIConstants {
    constexpr int InputHeight = 40;
    constexpr int ButtonHeight = 40;
    constexpr int SendButtonWidth = 80;
    constexpr int DefaultMargin = 20;
    constexpr int ContentMarginH = 30;
    constexpr int ContentMarginTop = 40;
    constexpr int DefaultSpacing = 16;
    constexpr int ButtonAreaTopMargin = 30;
    constexpr int ButtonSpacing = 50;
}

// 短信登录状态
struct SmsLoginState {
    std::string phoneNumber;
    std::string actionType;
} static gSmsState;

SmsLoginTab::SmsLoginTab(QWidget* parent)
    : LoginTab(parent)
{
    setupUI();
}

SmsLoginTab::~SmsLoginTab()
{
    if (m_countdownTimer) {
        m_countdownTimer->stop();
    }
}

void SmsLoginTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(SmsUIConstants::DefaultSpacing);
    mainLayout->setContentsMargins(SmsUIConstants::ContentMarginH,
                                   SmsUIConstants::ContentMarginTop,
                                   SmsUIConstants::ContentMarginH,
                                   SmsUIConstants::DefaultMargin);

    // 手机号输入区域
    QHBoxLayout* phoneLayout = new QHBoxLayout();
    phoneLayout->setSpacing(SmsUIConstants::DefaultSpacing);

    m_areaCodeLabel = new QLabel(this);
    m_areaCodeLabel->setAlignment(Qt::AlignCenter);
    m_areaCodeLabel->setFont(StyleManager::instance().getBodyFont());
    m_areaCodeLabel->setText("+86");
    m_areaCodeLabel->setFixedWidth(50);
    phoneLayout->addWidget(m_areaCodeLabel);

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setFixedHeight(SmsUIConstants::InputHeight);
    m_phoneEdit->setFont(StyleManager::instance().getBodyFont());
    QRegularExpression regex("^[1][3-9][0-9]{9}$");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(regex, this);
    m_phoneEdit->setValidator(validator);
    m_phoneEdit->setPlaceholderText("请输入手机号码");
    m_phoneEdit->setClearButtonEnabled(true);
    phoneLayout->addWidget(m_phoneEdit);

    m_sendButton = new QPushButton(this);
    m_sendButton->setFixedSize(SmsUIConstants::SendButtonWidth, SmsUIConstants::ButtonHeight);
    m_sendButton->setFont(StyleManager::instance().getButtonFont());
    m_sendButton->setText("发送");
    m_sendButton->setEnabled(false);
    phoneLayout->addWidget(m_sendButton);

    mainLayout->addLayout(phoneLayout);

    // 验证码输入区域
    QHBoxLayout* verifyLayout = new QHBoxLayout();
    m_verifyCodeEdit = new QLineEdit(this);
    m_verifyCodeEdit->setFixedHeight(SmsUIConstants::InputHeight);
    m_verifyCodeEdit->setFont(StyleManager::instance().getBodyFont());
    m_verifyCodeEdit->setPlaceholderText("请输入收到的验证码");
    m_verifyCodeEdit->setClearButtonEnabled(true);
    verifyLayout->addWidget(m_verifyCodeEdit);

    mainLayout->addLayout(verifyLayout);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(SmsUIConstants::ButtonSpacing);
    buttonLayout->setContentsMargins(0, SmsUIConstants::ButtonAreaTopMargin, 0, 0);

    m_confirmButton = new QPushButton(this);
    m_confirmButton->setMinimumHeight(SmsUIConstants::ButtonHeight);
    m_confirmButton->setFont(StyleManager::instance().getButtonFont());
    m_confirmButton->setText("确认");
    m_confirmButton->setEnabled(false);
    buttonLayout->addWidget(m_confirmButton);

    m_resetButton = new QPushButton(this);
    m_resetButton->setMinimumHeight(SmsUIConstants::ButtonHeight);
    m_resetButton->setFont(StyleManager::instance().getButtonFont());
    m_resetButton->setText("重置");
    m_resetButton->setProperty("secondary", true);
    buttonLayout->addWidget(m_resetButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // 倒计时定时器
    m_countdownTimer = new QTimer(this);

    // 信号连接
    connect(m_sendButton, &QPushButton::clicked, this, &SmsLoginTab::onSendButtonClicked);
    connect(m_confirmButton, &QPushButton::clicked, this, &SmsLoginTab::onConfirmButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &SmsLoginTab::reset);
    connect(m_countdownTimer, &QTimer::timeout, this, &SmsLoginTab::onTimerTimeout);
    connect(m_phoneEdit, &QLineEdit::textChanged, this, &SmsLoginTab::onPhoneTextChanged);
}

void SmsLoginTab::reset()
{
    m_phoneEdit->clear();
    m_verifyCodeEdit->clear();
    m_sendButton->setEnabled(false);
    m_confirmButton->setEnabled(false);
    if (m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
        m_sendButton->setText("发送");
    }
}

bool SmsLoginTab::validate() const
{
    return m_phoneEdit->text().length() == 11 && !m_verifyCodeEdit->text().isEmpty();
}

void SmsLoginTab::enableControls(bool enabled)
{
    if (enabled) {
        m_sendButton->setEnabled(false);
        m_countdownTimer->start(1000);
        m_remainingSeconds = 60;
    }
    m_confirmButton->setEnabled(enabled);
}

void SmsLoginTab::onPhoneTextChanged(const QString& text)
{
    if (text.length() >= 11) {
        m_sendButton->setEnabled(true);
    } else {
        m_sendButton->setEnabled(false);
    }
}

void SmsLoginTab::onTimerTimeout()
{
    m_remainingSeconds--;
    m_sendButton->setText(QString::number(m_remainingSeconds) + "秒");
    if (m_remainingSeconds <= 0) {
        m_countdownTimer->stop();
        m_sendButton->setEnabled(true);
        m_sendButton->setText("发送");
    }
}

void SmsLoginTab::onSendButtonClicked()
{
    m_sendButton->setEnabled(false);
    m_verifyCodeEdit->clear();
    m_confirmButton->setEnabled(false);

    QThreadPool::globalInstance()->start([this] {
        std::string phoneNumber = m_phoneEdit->text().toStdString();
        auto result = CreateLoginCaptcha(phoneNumber);

        if (result.retcode == 0) {
            gSmsState.phoneNumber = phoneNumber;
            gSmsState.actionType = result.action_type;
            QMetaObject::invokeMethod(this, [this]() { enableControls(true); });
        } else if (result.retcode == -3101 && result.mmt_type != 0) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("当前需要验证码，请使用 Cookie 登录或稍后重试");
                enableControls(false);
                m_sendButton->setEnabled(true);
            });
        } else if (result.retcode == -3008) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("手机号错误");
                enableControls(false);
                m_sendButton->setEnabled(true);
            });
        } else if (result.retcode == -3006) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("操作过于频繁，请稍后再试");
                enableControls(false);
                m_sendButton->setEnabled(true);
            });
        } else {
            QMetaObject::invokeMethod(this, [this, retcode = result.retcode]() {
                emit showMessageRequested("发送失败，错误码: " + QString::number(retcode));
                enableControls(false);
                m_sendButton->setEnabled(true);
            });
        }
    });
}

void SmsLoginTab::onConfirmButtonClicked()
{
    QThreadPool::globalInstance()->start([this] {
        std::string verifyCode = m_verifyCodeEdit->text().toStdString();
        auto result = LoginByMobileCaptcha(gSmsState.actionType, gSmsState.phoneNumber, verifyCode);

        if (result.retcode == -3205) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("验证码错误");
            });
        } else if (result.retcode == 0) {
            const std::string name = getMysUserName(result.data.aid);
            QMetaObject::invokeMethod(this, [this, name, result]() {
                emit loginSuccess(name, result.data.V2Token, result.data.aid, result.data.mid, "官服");
            });
        } else {
            QMetaObject::invokeMethod(this, [this, retcode = result.retcode]() {
                emit showMessageRequested("登录失败，错误码: " + QString::number(retcode));
            });
        }
    });
}
