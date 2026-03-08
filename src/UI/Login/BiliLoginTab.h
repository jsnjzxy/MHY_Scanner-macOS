#pragma once

#include "LoginTab.h"

#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

/**
 * @brief Bilibili 崩坏3 登录 Tab
 */
class BiliLoginTab : public LoginTab
{
    Q_OBJECT

public:
    explicit BiliLoginTab(QWidget* parent = nullptr);
    ~BiliLoginTab() override;

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
    void onShowPasswordChanged(int state);
    void onTextChanged();

private:
    void setupUI();
    void handleLoginResult(const auto& result);

    // UI 控件
    QLineEdit* m_accountEdit{nullptr};
    QLineEdit* m_passwordEdit{nullptr};
    QCheckBox* m_showPasswordCheckbox{nullptr};
    QPushButton* m_loginButton{nullptr};
    QPushButton* m_resetButton{nullptr};
};
