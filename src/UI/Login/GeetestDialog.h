#pragma once

#ifdef __APPLE__

#include <QDialog>
#include <QString>
#include <functional>

/**
 * @brief 极验验证对话框
 *
 * 使用 WKWebView 加载极验验证页面，完成验证后返回结果。
 * 仅在 macOS 上可用。
 */
class GeetestDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 验证结果
     */
    struct VerifyResult
    {
        bool success{false};
        QString lotNumber;
        QString passToken;
        QString captchaOutput;
        QString genTime;
        QString captchaId;
        QString errorMessage;
    };

    using ResultCallback = std::function<void(const VerifyResult&)>;

    explicit GeetestDialog(QWidget* parent = nullptr);
    ~GeetestDialog() override;

    /**
     * @brief 设置验证参数
     * @param gt 极验 ID
     * @param challenge 挑战码（可选，GT3 需要）
     * @param sessionId 会话 ID
     * @param useV4 是否使用 GT4
     */
    void setParams(const QString& gt, const QString& challenge,
                   const QString& sessionId, bool useV4 = true);

    /**
     * @brief 设置结果回调
     */
    void setResultCallback(ResultCallback callback);

    /**
     * @brief 开始验证
     */
    void startVerify();

    /**
     * @brief 获取验证结果
     */
    VerifyResult result() const { return m_result; }

Q_SIGNALS:
    /**
     * @brief 验证完成信号
     */
    void verifyCompleted(const VerifyResult& result);

public:
    /**
     * @brief 解析验证结果（供 Objective-C 回调使用）
     */
    void parseVerifyResult(const QString& json);

private:
    void setupUI();
    void loadVerifyPage();
    void injectJavaScript();

    // 私有实现（PIMPL 模式隐藏 Objective-C 类型）
    class Private;
    std::unique_ptr<Private> m_private;

    QString m_gt;
    QString m_challenge;
    QString m_sessionId;
    bool m_useV4{true};

    VerifyResult m_result;
    ResultCallback m_callback;
};

#endif // __APPLE__
