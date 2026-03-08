#pragma once

#include <QWidget>
#include <QString>

/**
 * @brief 登录 Tab 基类
 *
 * 所有登录 Tab 的共同接口：
 * - reset(): 重置 Tab 状态
 * - validate(): 验证输入是否有效
 */
class LoginTab : public QWidget
{
    Q_OBJECT

public:
    explicit LoginTab(QWidget* parent = nullptr);
    ~LoginTab() override;

    /**
     * @brief 重置 Tab 到初始状态
     */
    virtual void reset() = 0;

    /**
     * @brief 验证输入是否有效
     * @return true 如果输入有效，false 否则
     */
    virtual bool validate() const = 0;

signals:
    /**
     * @brief 请求显示消息框
     * @param message 消息内容
     */
    void showMessageRequested(const QString& message);

    /**
     * @brief 输入验证状态改变
     * @param valid 是否有效
     */
    void validationChanged(bool valid);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
