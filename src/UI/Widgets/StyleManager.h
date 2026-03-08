#pragma once

#include <QString>
#include <QFont>
#include <QMessageBox>
#include <QDialogButtonBox>

/**
 * @brief 统一样式管理器
 *
 * 提供应用程序全局样式表和各控件专用样式。
 * 使用单例模式，确保样式一致性。
 */
class StyleManager
{
public:
    static StyleManager& instance();

    // 获取全局样式表
    QString getGlobalStyleSheet() const;

    // 获取特定控件样式
    QString getLineEditStyle() const;
    QString getPushButtonStyle() const;
    QString getTabWidgetStyle() const;
    QString getTableWidgetStyle() const;
    QString getCheckBoxStyle() const;
    QString getComboBoxStyle() const;
    QString getLabelStyle() const;
    QString getMenuBarStyle() const;
    QString getMessageBoxStyle() const;

    // 字体
    QFont getTitleFont() const;      // 14px - 标题/Tab标签
    QFont getBodyFont() const;       // 13px - 正文/输入框
    QFont getButtonFont() const;     // 13px - 按钮
    QFont getSmallFont() const;      // 12px - 提示文字/表头

    // 尺寸常量
    int inputHeight() const { return 40; }
    int buttonHeight() const { return 40; }
    int buttonMinWidth() const { return 100; }
    int defaultSpacing() const { return 12; }
    int defaultMargin() const { return 20; }
    int contentMarginH() const { return 30; }      // Tab内容水平边距
    int contentMarginTop() const { return 20; }    // Tab内容顶部边距
    int buttonAreaTopMargin() const { return 30; } // 按钮区域顶部边距

    // 弹窗辅助方法：设置样式并居中按钮
    static void setupMessageBox(QMessageBox& msgBox)
    {
        msgBox.setStyleSheet(instance().getMessageBoxStyle());
        // 查找 QDialogButtonBox 并设置按钮居中
        for (auto* child : msgBox.children())
        {
            if (auto* buttonBox = qobject_cast<QDialogButtonBox*>(child))
            {
                buttonBox->setCenterButtons(true);
                break;
            }
        }
    }

private:
    StyleManager() = default;
    ~StyleManager() = default;
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
};
