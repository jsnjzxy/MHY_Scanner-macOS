#include "LoginTab.h"

#include <QKeyEvent>

LoginTab::LoginTab(QWidget* parent)
    : QWidget(parent)
{
}

LoginTab::~LoginTab()
{
}

void LoginTab::keyPressEvent(QKeyEvent* event)
{
    // ESC 键由父窗口处理
    if (event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}
