#include "StyleManager.h"

StyleManager& StyleManager::instance()
{
    static StyleManager instance;
    return instance;
}

QString StyleManager::getGlobalStyleSheet() const
{
    return R"(
        /* 全局样式 */
        QWidget {
            font-family: "Microsoft YaHei", "微软雅黑", "PingFang SC", sans-serif;
            font-size: 13px;
            color: #333333;
        }

        QMainWindow {
            background-color: #F5F7FA;
        }

        QDialog {
            background-color: #F5F7FA;
        }

        /* 滚动条样式 */
        QScrollBar:vertical {
            width: 8px;
            background: transparent;
        }
        QScrollBar::handle:vertical {
            background: #C0C0C0;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #A0A0A0;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            height: 8px;
            background: transparent;
        }
        QScrollBar::handle:horizontal {
            background: #C0C0C0;
            border-radius: 4px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #A0A0A0;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )" + getLineEditStyle() + getPushButtonStyle() + getTabWidgetStyle() +
           getCheckBoxStyle() + getComboBoxStyle() + getTableWidgetStyle() +
           getMenuBarStyle() + getMessageBoxStyle();
}

QString StyleManager::getLineEditStyle() const
{
    return R"(
        QLineEdit {
            height: 40px;
            padding: 0 12px;
            border: 1px solid #D9D9D9;
            border-radius: 4px;
            background-color: #FFFFFF;
            color: #333333;
            font-size: 13px;
            selection-background-color: #D6E9F8;
        }
        QLineEdit:hover {
            border-color: #4A90D9;
        }
        QLineEdit:focus {
            border-color: #4A90D9;
            outline: none;
        }
        QLineEdit:disabled {
            background-color: #F5F5F5;
            color: #999999;
            border-color: #E0E0E0;
        }
        QLineEdit::placeholder {
            color: #BBBBBB;
        }
    )";
}

QString StyleManager::getPushButtonStyle() const
{
    return R"(
        QPushButton {
            height: 40px;
            min-width: 100px;
            padding: 0 16px;
            border: none;
            border-radius: 4px;
            background-color: #4A90D9;
            color: #FFFFFF;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3A7FC8;
        }
        QPushButton:pressed {
            background-color: #2D6CB5;
        }
        QPushButton:disabled {
            background-color: #A0C4E8;
            color: #FFFFFF;
        }

        /* 次要按钮样式 - 通过设置 property("secondary", true) 使用 */
        QPushButton[secondary="true"] {
            height: 40px;
            min-width: 100px;
            padding: 0 16px;
            border: 1px solid #D9D9D9;
            border-radius: 4px;
            background-color: #FFFFFF;
            color: #333333;
            font-size: 13px;
        }
        QPushButton[secondary="true"]:hover {
            border-color: #4A90D9;
            color: #4A90D9;
        }
        QPushButton[secondary="true"]:pressed {
            background-color: #F5F7FA;
        }
        QPushButton[secondary="true"]:disabled {
            background-color: #F5F5F5;
            color: #999999;
            border-color: #E0E0E0;
        }

        /* 选中状态按钮（用于监视按钮） */
        QPushButton[checked="true"] {
            background-color: #52C41A;
        }
        QPushButton[checked="true"]:hover {
            background-color: #45A818;
        }
    )";
}

QString StyleManager::getTabWidgetStyle() const
{
    return R"(
        QTabWidget::pane {
            border: 1px solid #E8E8E8;
            border-radius: 4px;
            background-color: #FFFFFF;
            margin-top: -1px;
        }
        QTabWidget::tab-bar {
            alignment: left;
        }
        QTabBar::tab {
            height: 36px;
            min-width: 80px;
            padding: 0 16px;
            margin-right: 4px;
            border: none;
            border-bottom: 2px solid transparent;
            background-color: transparent;
            color: #666666;
            font-size: 13px;
        }
        QTabBar::tab:hover {
            color: #4A90D9;
        }
        QTabBar::tab:selected {
            color: #4A90D9;
            border-bottom: 2px solid #4A90D9;
        }
        QTabBar::tab:!selected {
            margin-bottom: 0px;
        }
    )";
}

QString StyleManager::getTableWidgetStyle() const
{
    return R"(
        QTableWidget {
            border: 1px solid #E8E8E8;
            border-radius: 4px;
            background-color: #FFFFFF;
            gridline-color: #F0F0F0;
            font-size: 13px;
            color: #333333;
        }
        QTableWidget::item {
            height: 40px;
            padding: 0 8px;
            border: none;
            border-bottom: 1px solid #F0F0F0;
        }
        QTableWidget::item:hover {
            background-color: #E8F4FD;
        }
        QTableWidget::item:selected {
            background-color: #D6E9F8;
            color: #333333;
        }
        QHeaderView::section {
            height: 36px;
            padding: 0 8px;
            border: none;
            border-bottom: 1px solid #E8E8E8;
            background-color: #FAFAFA;
            color: #333333;
            font-size: 12px;
            font-weight: bold;
        }
        QHeaderView::section:hover {
            background-color: #F0F0F0;
        }
    )";
}

QString StyleManager::getCheckBoxStyle() const
{
    return R"(
        QCheckBox {
            spacing: 8px;
            color: #333333;
            font-size: 13px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 1px solid #D9D9D9;
            border-radius: 3px;
            background-color: #FFFFFF;
        }
        QCheckBox::indicator:hover {
            border-color: #4A90D9;
        }
        QCheckBox::indicator:checked {
            background-color: #4A90D9;
            border-color: #4A90D9;
        }
        QCheckBox::indicator:disabled {
            background-color: #F5F5F5;
            border-color: #E0E0E0;
        }
        QCheckBox:disabled {
            color: #999999;
        }
    )";
}

QString StyleManager::getComboBoxStyle() const
{
    return R"(
        QComboBox {
            height: 36px;
            padding: 0 12px;
            padding-right: 30px;
            border: 1px solid #D9D9D9;
            border-radius: 4px;
            background-color: #FFFFFF;
            color: #333333;
            font-size: 13px;
        }
        QComboBox:hover {
            border-color: #4A90D9;
        }
        QComboBox:focus {
            border-color: #4A90D9;
        }
        QComboBox::drop-down {
            width: 30px;
            border: none;
            subcontrol-position: center right;
        }
        QComboBox::down-arrow {
            width: 12px;
            height: 12px;
        }
        QComboBox QAbstractItemView {
            border: 1px solid #E8E8E8;
            border-radius: 4px;
            background-color: #FFFFFF;
            selection-background-color: #E8F4FD;
            selection-color: #333333;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            height: 32px;
            padding: 0 12px;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #E8F4FD;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #D6E9F8;
        }
        QComboBox:disabled {
            background-color: #F5F5F5;
            color: #999999;
        }
    )";
}

QString StyleManager::getLabelStyle() const
{
    return R"(
        QLabel {
            color: #333333;
            background-color: transparent;
        }
    )";
}

QString StyleManager::getMenuBarStyle() const
{
    return R"(
        QMenuBar {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E8E8E8;
            padding: 2px 4px;
            font-size: 13px;
        }
        QMenuBar::item {
            padding: 6px 12px;
            background-color: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #E8F4FD;
            color: #4A90D9;
        }
        QMenuBar::item:pressed {
            background-color: #D6E9F8;
        }
        QMenu {
            background-color: #FFFFFF;
            border: 1px solid #E8E8E8;
            border-radius: 4px;
            padding: 4px 0;
            font-size: 13px;
        }
        QMenu::item {
            padding: 8px 24px 8px 16px;
            border-radius: 0;
        }
        QMenu::item:selected {
            background-color: #E8F4FD;
            color: #4A90D9;
        }
        QMenu::separator {
            height: 1px;
            background-color: #E8E8E8;
            margin: 4px 8px;
        }
    )";
}

QFont StyleManager::getTitleFont() const
{
    QFont font("Microsoft YaHei", 14, QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QFont StyleManager::getBodyFont() const
{
    QFont font("Microsoft YaHei", 13, QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QFont StyleManager::getButtonFont() const
{
    QFont font("Microsoft YaHei", 13, QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QFont StyleManager::getSmallFont() const
{
    QFont font("Microsoft YaHei", 12, QFont::Normal);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

QString StyleManager::getMessageBoxStyle() const
{
    return R"(
        QMessageBox {
            background-color: #FFFFFF;
            border: 1px solid #E8E8E8;
            border-radius: 8px;
            min-width: 300px;
        }
        QMessageBox QLabel {
            color: #333333;
            font-size: 14px;
            padding: 20px 24px;
            min-width: 200px;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox QDialogButtonBox {
            button-layout: 2;
            qproperty-centerButtons: true;
        }
        QMessageBox QPushButton {
            height: 36px;
            min-width: 80px;
            padding: 0 20px;
            border: none;
            border-radius: 4px;
            background-color: #4A90D9;
            color: #FFFFFF;
            font-size: 13px;
        }
        QMessageBox QPushButton:hover {
            background-color: #3A7FC8;
        }
        QMessageBox QPushButton:pressed {
            background-color: #2D6CB5;
        }
    )";
}
