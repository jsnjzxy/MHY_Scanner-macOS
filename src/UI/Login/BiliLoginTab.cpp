#include "BiliLoginTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThreadPool>
#include <QMetaObject>

#include "Widgets/StyleManager.h"
#include "Network/Api/BilibiliApi.hpp"

// UI 常量
namespace BiliUIConstants {
    constexpr int ButtonHeight = 40;
    constexpr int DefaultMargin = 20;
    constexpr int ContentMarginH = 30;
    constexpr int ContentMarginTop = 40;
    constexpr int DefaultSpacing = 16;
    constexpr int ButtonAreaTopMargin = 30;
    constexpr int ButtonSpacing = 50;
}

BiliLoginTab::BiliLoginTab(QWidget* parent)
    : LoginTab(parent)
{
    setupUI();
}

BiliLoginTab::~BiliLoginTab()
{
}

void BiliLoginTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(BiliUIConstants::DefaultSpacing);
    mainLayout->setContentsMargins(BiliUIConstants::ContentMarginH,
                                   BiliUIConstants::ContentMarginTop,
                                   BiliUIConstants::ContentMarginH,
                                   BiliUIConstants::DefaultMargin);

    // 输入区域
    QVBoxLayout* inputLayout = new QVBoxLayout();
    inputLayout->setSpacing(BiliUIConstants::DefaultSpacing);

    m_accountEdit = new QLineEdit(this);
    m_accountEdit->setFixedHeight(BiliUIConstants::ButtonHeight);
    m_accountEdit->setFont(StyleManager::instance().getBodyFont());
    m_accountEdit->setPlaceholderText("请输入账号...");
    m_accountEdit->setClearButtonEnabled(true);
    inputLayout->addWidget(m_accountEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setFixedHeight(BiliUIConstants::ButtonHeight);
    m_passwordEdit->setFont(StyleManager::instance().getBodyFont());
    m_passwordEdit->setPlaceholderText("请输入密码...");
    m_passwordEdit->setClearButtonEnabled(true);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    inputLayout->addWidget(m_passwordEdit);

    mainLayout->addLayout(inputLayout);

    // 显示密码复选框
    m_showPasswordCheckbox = new QCheckBox(this);
    m_showPasswordCheckbox->setText("显示密码");
    m_showPasswordCheckbox->setFont(StyleManager::instance().getSmallFont());
    mainLayout->addWidget(m_showPasswordCheckbox);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(BiliUIConstants::ButtonSpacing);
    buttonLayout->setContentsMargins(0, BiliUIConstants::ButtonAreaTopMargin, 0, 0);

    m_loginButton = new QPushButton(this);
    m_loginButton->setText("登录");
    m_loginButton->setMinimumHeight(BiliUIConstants::ButtonHeight);
    m_loginButton->setFont(StyleManager::instance().getButtonFont());
    m_loginButton->setEnabled(false);
    buttonLayout->addWidget(m_loginButton);

    m_resetButton = new QPushButton(this);
    m_resetButton->setText("重置");
    m_resetButton->setMinimumHeight(BiliUIConstants::ButtonHeight);
    m_resetButton->setFont(StyleManager::instance().getButtonFont());
    m_resetButton->setProperty("secondary", true);
    buttonLayout->addWidget(m_resetButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // 信号连接
    connect(m_loginButton, &QPushButton::clicked, this, &BiliLoginTab::onLoginButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &BiliLoginTab::reset);
    connect(m_showPasswordCheckbox, &QCheckBox::checkStateChanged, this, &BiliLoginTab::onShowPasswordChanged);
    connect(m_accountEdit, &QLineEdit::textChanged, this, &BiliLoginTab::onTextChanged);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &BiliLoginTab::onTextChanged);
}

void BiliLoginTab::reset()
{
    m_accountEdit->clear();
    m_passwordEdit->clear();
    m_showPasswordCheckbox->setChecked(false);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
}

bool BiliLoginTab::validate() const
{
    return !m_accountEdit->text().trimmed().isEmpty() && !m_passwordEdit->text().isEmpty();
}

void BiliLoginTab::onTextChanged()
{
    m_loginButton->setEnabled(validate());
}

void BiliLoginTab::onShowPasswordChanged(int state)
{
    if (state == Qt::Checked) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_passwordEdit->setInputMethodHints(Qt::ImhHiddenText);
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
    }
}

void BiliLoginTab::onLoginButtonClicked()
{
    QThreadPool::globalInstance()->start([this]() {
        auto result = BH3API::BILI::LoginByPassWord(m_accountEdit->text().toStdString(),
                                                      m_passwordEdit->text().toStdString());
        QMetaObject::invokeMethod(this, [this, result]() {
            handleLoginResult(result);
        });
    });
}

void BiliLoginTab::handleLoginResult(const auto& result)
{
    if (result.code == 0) {
        emit loginSuccess(result.uname, result.access_key, result.uid, "", "崩坏3B服");
    } else if (result.code == 200000) {
        emit showMessageRequested("B站账号密码登录需要验证码支持，请使用 Cookie 登录");
    } else if (result.code == 500002) {
        emit showMessageRequested("密码错误");
    } else if (result.code == -502) {
        emit showMessageRequested("账号错误");
    } else {
        emit showMessageRequested("登录失败，错误码: " + QString::number(result.code));
    }
}
