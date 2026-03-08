#include "CookieLoginTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThreadPool>
#include <QMetaObject>
#include <ranges>

#include "Widgets/StyleManager.h"
#include "Utils/CookieParser.hpp"
#include "Network/Api/MihoyoApi.hpp"

// UI 常量
namespace CookieUIConstants {
    constexpr int ButtonHeight = 40;
    constexpr int DefaultMargin = 20;
    constexpr int ContentMarginH = 30;
    constexpr int ContentMarginTop = 40;
    constexpr int DefaultSpacing = 16;
    constexpr int ButtonAreaTopMargin = 30;
    constexpr int ButtonSpacing = 50;
}

CookieLoginTab::CookieLoginTab(QWidget* parent)
    : LoginTab(parent)
{
    setupUI();
}

CookieLoginTab::~CookieLoginTab()
{
}

void CookieLoginTab::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(CookieUIConstants::DefaultSpacing);
    mainLayout->setContentsMargins(CookieUIConstants::ContentMarginH,
                                   CookieUIConstants::ContentMarginTop,
                                   CookieUIConstants::ContentMarginH,
                                   CookieUIConstants::DefaultMargin);

    // Cookie 输入区域
    m_cookieEdit = new QLineEdit(this);
    m_cookieEdit->setFixedHeight(CookieUIConstants::ButtonHeight);
    m_cookieEdit->setFont(StyleManager::instance().getBodyFont());
    m_cookieEdit->setPlaceholderText("请输入 Cookie（以 stoken; 结尾）");
    m_cookieEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_cookieEdit);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(CookieUIConstants::ButtonSpacing);
    buttonLayout->setContentsMargins(0, CookieUIConstants::ButtonAreaTopMargin, 0, 0);

    m_loginButton = new QPushButton(this);
    m_loginButton->setText("登录");
    m_loginButton->setMinimumHeight(CookieUIConstants::ButtonHeight);
    m_loginButton->setFont(StyleManager::instance().getButtonFont());
    m_loginButton->setEnabled(false);
    buttonLayout->addWidget(m_loginButton);

    m_resetButton = new QPushButton(this);
    m_resetButton->setText("重置");
    m_resetButton->setMinimumHeight(CookieUIConstants::ButtonHeight);
    m_resetButton->setFont(StyleManager::instance().getButtonFont());
    m_resetButton->setProperty("secondary", true);
    buttonLayout->addWidget(m_resetButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // 信号连接
    connect(m_loginButton, &QPushButton::clicked, this, &CookieLoginTab::onLoginButtonClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &CookieLoginTab::reset);
    connect(m_cookieEdit, &QLineEdit::textChanged, this, &CookieLoginTab::onCookieTextChanged);
}

void CookieLoginTab::reset()
{
    m_cookieEdit->clear();
}

bool CookieLoginTab::validate() const
{
    QString text = m_cookieEdit->text().trimmed();
    return text.length() > 20 && text.contains("stoken");
}

void CookieLoginTab::onCookieTextChanged(const QString& text)
{
    m_loginButton->setEnabled(text.length() > 20 && text.contains("stoken"));
}

void CookieLoginTab::onLoginButtonClicked()
{
    QThreadPool::globalInstance()->start([this]() {
        std::string cookieString = m_cookieEdit->text().toStdString();
        CookieParser cp(cookieString);

        std::string uid{}, stoken{}, mid{};
        static constinit const std::array<std::string_view, 3> keys{
            "stuid", "ltuid", "account_id"
        };

        auto result = std::ranges::find_if(keys, [&cp, &uid](const std::string_view key) {
            if (auto value = cp[key]; value.has_value()) {
                uid = *value;
                return true;
            }
            return false;
        });

        if (result == keys.end()) {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("Cookie格式错误!");
            });
            return;
        }

        if (auto value = cp["stoken"]; value.has_value()) {
            stoken = *value;
        } else {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("Cookie格式错误!");
            });
            return;
        }

        if (auto value = cp["mid"]; value.has_value()) {
            mid = *value;
        } else {
            QMetaObject::invokeMethod(this, [this]() {
                emit showMessageRequested("Cookie格式错误!");
            });
            return;
        }

        std::string name = getMysUserName(uid);
        QMetaObject::invokeMethod(this, [this, name, stoken, uid, mid]() {
            emit loginSuccess(name, stoken, uid, mid, "官服");
        });
    });
}
