#pragma once

#include <QTableWidget>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>

class DraggableTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit DraggableTableWidget(QWidget* parent = nullptr)
        : QTableWidget(parent)
    {
        // 启用拖拽功能
        setDragEnabled(true);
        setAcceptDrops(true);
        setDragDropOverwriteMode(false);
        setDropIndicatorShown(true);

        // 设置为内部移动模式
        setDragDropMode(QAbstractItemView::InternalMove);

        // 设置选择行为为整行选择
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::SingleSelection);

        // 禁用默认的拖拽移动行为，我们自己处理
        setDefaultDropAction(Qt::MoveAction);
    }

signals:
    void rowsSwapped(int fromRow, int toRow);
    void contextMenuRequested(const QPoint& pos);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override
    {
        // 使用全局坐标转换到 viewport 坐标，确保 itemAt 能正确识别行
        const QPoint viewportPos = viewport()->mapFromGlobal(event->globalPos());
        emit contextMenuRequested(viewportPos);
        event->accept();
    }

    void dropEvent(QDropEvent* event) override
    {
        // 检查是否是内部拖拽
        if (event->source() != this)
        {
            QTableWidget::dropEvent(event);
            return;
        }

        // 获取拖拽的行
        QList<QTableWidgetItem*> items = this->selectedItems();
        if (items.isEmpty())
        {
            event->ignore();
            return;
        }

        int sourceRow = items.first()->row();

        // 计算目标行
        QTableWidgetItem* targetItem = itemAt(event->position().toPoint());
        int targetRow = targetItem ? targetItem->row() : rowCount() - 1;

        // 如果源行和目标行相同，忽略
        if (sourceRow == targetRow)
        {
            event->ignore();
            return;
        }

        // 阻止信号，避免在移动过程中触发 itemChanged 信号
        blockSignals(true);

        // 交换两行的数据
        swapRows(sourceRow, targetRow);

        blockSignals(false);

        // 发出信号通知外部更新配置
        emit rowsSwapped(sourceRow, targetRow);

        // 接受事件
        event->accept();

        // 重新选择原来的行（现在在目标位置）
        selectRow(targetRow);
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        // 只接受内部拖拽
        if (event->source() == this)
        {
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        // 只接受内部拖拽
        if (event->source() == this)
        {
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
    }

private:
    void swapRows(int row1, int row2)
    {
        // 确保行号有效
        if (row1 < 0 || row2 < 0 || row1 >= rowCount() || row2 >= rowCount())
        {
            return;
        }

        // 交换两行的所有单元格数据
        for (int col = 0; col < columnCount(); ++col)
        {
            QTableWidgetItem* item1 = takeItem(row1, col);
            QTableWidgetItem* item2 = takeItem(row2, col);

            // 设置回交换后的位置
            if (item1)
            {
                setItem(row2, col, item1);
            }
            if (item2)
            {
                setItem(row1, col, item2);
            }
        }

        // 更新序号列（第一列）
        updateRowNumbers();
    }

    void updateRowNumbers()
    {
        for (int row = 0; row < rowCount(); ++row)
        {
            QTableWidgetItem* numItem = item(row, 0);
            if (numItem)
            {
                numItem->setText(QString::number(row + 1));
            }
        }
    }
};
