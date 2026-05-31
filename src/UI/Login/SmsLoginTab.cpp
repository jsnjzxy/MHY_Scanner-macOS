#include "SmsLoginTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QThreadPool>
#include <QMetaObject>
#include <QByteArray>
#include <iostream>

#include "Widgets/StyleManager.h"

#include "Network/Api/MihoyoApi.hpp"
#include "Common/Constants.h"

#ifdef __APPLE__
#include "GeetestDialog.h"
#endif

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
    std::string sessionId;
    std::string gt;
    std::string mid;
    std::string tokenType1;
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
#ifdef __APPLE__
    if (m_geetestDialog) {
        delete m_geetestDialog;
    }
#endif
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
    gSmsState = {};
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

    gSmsState.phoneNumber = m_phoneEdit->text().toStdString();

    QThreadPool::globalInstance()->start([this] {
        auto result = CreateLoginCaptcha(gSmsState.phoneNumber);

        if (result.retcode == 0) {
            gSmsState.actionType = result.action_type;
            QMetaObject::invokeMethod(this, [this]() { enableControls(true); });
        } else if (result.retcode == -3101 && result.mmt_type != 0) {
            // 需要极验验证
            gSmsState.sessionId = result.session_id;
            gSmsState.gt = result.gt;

            QMetaObject::invokeMethod(this, [this, result]() {
#ifdef __APPLE__
                // 显示极验验证对话框
                showGeetestDialog(
                    QString::fromStdString(result.gt),
                    QString::fromStdString(result.session_id)
                );
#else
                emit showMessageRequested("当前需要验证码，请使用 Cookie 登录或稍后重试");
                enableControls(false);
                m_sendButton->setEnabled(true);
#endif
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

#ifdef __APPLE__
void SmsLoginTab::showGeetestDialog(const QString& gt, const QString& sessionId)
{
    if (!m_geetestDialog) {
        m_geetestDialog = new GeetestDialog(this);
        connect(m_geetestDialog, &GeetestDialog::verifyCompleted,
                this, [this](const GeetestDialog::VerifyResult& result) {
            if (result.success) {
                onGeetestVerifyCompleted(result.lotNumber, result.passToken,
                                          result.captchaOutput, result.genTime);
            } else {
                emit showMessageRequested(result.errorMessage.isEmpty()
                    ? QStringLiteral("验证失败") : result.errorMessage);
                m_sendButton->setEnabled(true);
            }
        });
    }

    m_geetestDialog->setParams(gt, QString(), sessionId, true);
    m_geetestDialog->startVerify();
    m_geetestDialog->exec();
}

void SmsLoginTab::onGeetestVerifyCompleted(const QString& lotNumber, const QString& passToken,
                                            const QString& captchaOutput, const QString& genTime)
{
    // 保存验证结果
    m_sessionId = gSmsState.sessionId;
    m_gt = gSmsState.gt;

    std::cout << "[SmsLoginTab] onGeetestVerifyCompleted:" << std::endl;
    std::cout << "  m_sessionId: " << m_sessionId << std::endl;
    std::cout << "  m_gt: " << m_gt << std::endl;
    std::cout << "  gSmsState.sessionId: " << gSmsState.sessionId << std::endl;

    // 带验证结果发送短信
    sendCaptchaWithGeetest(lotNumber, passToken, captchaOutput, genTime);
}

void SmsLoginTab::sendCaptchaWithGeetest(const QString& lotNumber, const QString& passToken,
                                           const QString& captchaOutput, const QString& genTime)
{
    // 调试：确认参数值
    std::cout << "[SmsLoginTab] sendCaptchaWithGeetest:" << std::endl;
    std::cout << "  m_sessionId: '" << m_sessionId << "'" << std::endl;
    std::cout << "  m_gt: '" << m_gt << "'" << std::endl;
    std::cout << "  lotNumber: " << lotNumber.toStdString() << std::endl;
    std::cout << "  passToken: " << passToken.toStdString() << std::endl;
    std::cout << "  genTime: " << genTime.toStdString() << std::endl;

    // 构建 Aigis 数据（米游社格式）
    // 格式: {session_id};{base64(json)}
    // 注意：使用 ordered_json 保持字段顺序与米游社一致

    nlohmann::json userInfo;
    userInfo["session_id"] = m_sessionId;

    // 使用 ordered_json 保持插入顺序
    nlohmann::ordered_json aigisJson;
    aigisJson["captcha_output"] = captchaOutput.toStdString();
    aigisJson["userInfo"] = userInfo.dump();
    aigisJson["gen_time"] = genTime.toStdString();
    aigisJson["lot_number"] = lotNumber.toStdString();
    aigisJson["pass_token"] = passToken.toStdString();
    aigisJson["captcha_id"] = m_gt;

    // Base64 编码
    std::string jsonStr = aigisJson.dump();
    // TODO: 使用 Qt 自带的 Base64 编码
    // std::string base64Str = base64_encode(jsonStr);
    std::string base64Str = QByteArray::fromStdString(jsonStr).toBase64().toStdString();

    // 最终格式: {session_id};{base64}
    std::string aigisStr = m_sessionId + ";" + base64Str;

    // 调试输出
    std::cout << "[SmsLoginTab] Aigis JSON (full): " << jsonStr << std::endl;
    std::cout << "[SmsLoginTab] Aigis header (first 150 chars): " << aigisStr.substr(0, 150) << "..." << std::endl;

    QThreadPool::globalInstance()->start([this, aigisStr] {
        auto result = CreateLoginCaptcha(gSmsState.phoneNumber, aigisStr);

        if (result.retcode == 0) {
            gSmsState.actionType = result.action_type;
            QMetaObject::invokeMethod(this, [this]() {
                enableControls(true);
                emit showMessageRequested(QStringLiteral("验证码已发送"));
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
#endif

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
            // 保存 token_type=1 的 token
            gSmsState.tokenType1 = result.data.V2Token;
            gSmsState.mid = result.data.mid;

            // 调用 exchange 转换为 token_type=4
            auto exchangeResult = ExchangeToken(result.data.mid, result.data.V2Token, 1, 4);

            if (exchangeResult.retcode == 0) {
                const std::string name = getMysUserName(result.data.aid);
                QMetaObject::invokeMethod(this, [this, name, exchangeResult, result]() {
                    emit loginSuccess(name, exchangeResult.token, result.data.aid, result.data.mid, "官服");
                });
            } else {
                QMetaObject::invokeMethod(this, [this, retcode = exchangeResult.retcode]() {
                    emit showMessageRequested("Token转换失败，错误码: " + QString::number(retcode));
                });
            }
        } else {
            QMetaObject::invokeMethod(this, [this, retcode = result.retcode]() {
                emit showMessageRequested("登录失败，错误码: " + QString::number(retcode));
            });
        }
    });
}
