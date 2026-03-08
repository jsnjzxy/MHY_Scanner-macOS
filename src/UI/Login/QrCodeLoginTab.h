#pragma once

#include "LoginTab.h"

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QImage>
#include <QThreadPool>

#include <opencv2/opencv.hpp>

/**
 * @brief 扫码登录 Tab
 */
class QrCodeLoginTab : public LoginTab
{
    Q_OBJECT

public:
    explicit QrCodeLoginTab(QWidget* parent = nullptr);
    ~QrCodeLoginTab() override;

    void reset() override;
    bool validate() const override;

    /**
     * @brief 开始二维码登录流程
     */
    void startLogin();

    /**
     * @brief 停止定时器
     */
    void stopTimer();

signals:
    /**
     * @brief 登录成功
     */
    void loginSuccess(const std::string& name, const std::string& token,
                      const std::string& uid, const std::string& mid,
                      const std::string& type);

    /**
     * @brief 请求刷新二维码
     */
    void refreshRequested();

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onTimerTimeout();
    void onRefreshButtonClicked();

private:
    void setupUI();
    void checkLoginState();
    void generateQRCode();

    // UI 控件
    QLabel* m_promptLabel{nullptr};
    QLabel* m_qrCodeLabel{nullptr};
    QPushButton* m_refreshButton{nullptr};  // 二维码过期时的覆盖按钮
    QPushButton* m_refreshButton2{nullptr}; // 底部刷新按钮

    // 状态
    QTimer* m_checkTimer{nullptr};
    QImage m_qrCodeImage{305, 305, QImage::Format_Indexed8};
    cv::Mat m_qrCodeMat;
    std::string m_ticket;
    std::atomic_bool m_allowDrawQRCode{false};
    inline static QThreadPool m_qrCodePool{};
};
