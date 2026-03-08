#pragma once

#include "LoginTab.h"

#include <QLineEdit>
#include <QPushButton>

/**
 * @brief Cookie 登录 Tab
 */
class CookieLoginTab : public LoginTab
{
    Q_OBJECT

public:
    explicit CookieLoginTab(QWidget* parent = nullptr);
    ~CookieLoginTab() override;

    void reset() override;
    bool validate() const override;

signals:
    /**
     * @brief 登录成功
     */
    void loginSuccess(const std::string& name, const std::string& token,
                      const std::string& uid, const std::string& mid,
                      const std::string& type);

private slots:
    void onLoginButtonClicked();
    void onCookieTextChanged(const QString& text);

private:
    void setupUI();

    // UI 控件
    QLineEdit* m_cookieEdit{nullptr};
    QPushButton* m_loginButton{nullptr};
    QPushButton* m_resetButton{nullptr};
};
