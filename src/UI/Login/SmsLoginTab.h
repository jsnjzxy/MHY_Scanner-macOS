#pragma once

#include "LoginTab.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#ifdef __APPLE__
// 前向声明
class GeetestDialog;
#endif

/**
 * @brief 短信登录 Tab
 */
class SmsLoginTab : public LoginTab
{
    Q_OBJECT

public:
    explicit SmsLoginTab(QWidget* parent = nullptr);
    ~SmsLoginTab() override;

    void reset() override;
    bool validate() const override;

signals:
    /**
     * @brief 登录成功
     * @param name 用户名
     * @param token 令牌
     * @param uid 用户ID
     * @param mid 中间值
     * @param type 账号类型
     */
    void loginSuccess(const std::string& name, const std::string& token,
                      const std::string& uid, const std::string& mid,
                      const std::string& type);

private slots:
    void onSendButtonClicked();
    void onConfirmButtonClicked();
    void onTimerTimeout();
    void onPhoneTextChanged(const QString& text);
    void onGeetestVerifyCompleted(const QString& lotNumber, const QString& passToken,
                                   const QString& captchaOutput, const QString& genTime);

private:
    void setupUI();
    void enableControls(bool enabled);
    void showGeetestDialog(const QString& gt, const QString& sessionId);
    void sendCaptchaWithGeetest(const QString& lotNumber, const QString& passToken,
                                 const QString& captchaOutput, const QString& genTime);

    // UI 控件
    QLabel* m_areaCodeLabel{nullptr};
    QLineEdit* m_phoneEdit{nullptr};
    QPushButton* m_sendButton{nullptr};
    QLineEdit* m_verifyCodeEdit{nullptr};
    QPushButton* m_confirmButton{nullptr};
    QPushButton* m_resetButton{nullptr};
    QTimer* m_countdownTimer{nullptr};

    // 状态
    int m_remainingSeconds{0};
    std::string m_phoneNumber;
    std::string m_actionType;

    // 极验验证状态
    std::string m_sessionId;
    std::string m_gt;

#ifdef __APPLE__
    GeetestDialog* m_geetestDialog{nullptr};
#endif
};
