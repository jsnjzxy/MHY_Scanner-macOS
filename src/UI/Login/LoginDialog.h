#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QThreadPool>

#include "SmsLoginTab.h"
#include "QrCodeLoginTab.h"
#include "CookieLoginTab.h"
#include "BiliLoginTab.h"

/**
 * @brief 登录对话框
 *
 * 整合四种登录方式的对话框：
 * - 短信登录
 * - 扫码登录
 * - Cookie 登录
 * - Bilibili 登录
 */
class LoginDialog : public QWidget
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override;

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

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;  // ESC 键关闭

private slots:
    void onTabChanged(int index);
    void onCloseButtonClicked();
    void onLoginSuccess(const std::string& name, const std::string& token,
                        const std::string& uid, const std::string& mid,
                        const std::string& type);

private:
    void setupUI();
    void setupTitleBar();
    void showMessage(const QString& message);

    // UI 控件
    QTabWidget* m_tabWidget{nullptr};
    QPushButton* m_closeButton{nullptr};

    // 登录 Tabs
    SmsLoginTab* m_smsTab{nullptr};
    QrCodeLoginTab* m_qrCodeTab{nullptr};
    CookieLoginTab* m_cookieTab{nullptr};
    BiliLoginTab* m_biliTab{nullptr};

    inline static QThreadPool m_qrCodePool{};
};
