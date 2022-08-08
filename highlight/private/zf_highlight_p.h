#pragma once

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTableView>
#include "../zf_highlight_view.h"

#define ZF_HIGHLIGHT_PROPERTY_ROLE (Qt::UserRole + 1)

namespace zf
{
//! Делегат для отрисовки ячеек HighlightView
class HighlightDelegate : public QStyledItemDelegate
{
public:
    HighlightDelegate(HighlightView* view);

protected:
    //! Отрисовка содержимого ячейки
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    //! Определение минимального размера ячейки
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    void initTextDocument(const QStyleOptionViewItem& option, const QModelIndex& index, QTextDocument& doc) const;

    //! Цвет рамки, которым отмечается выделение ошибок
    static QColor highlightColorBorder(InformationType type);
    //! Цвет текста, которым отмечается выделение ошибок
    static QColor highlightColorText(InformationType type);

    HighlightView* _view;
};

class HighlightTableView : public QTableView
{
public:
    HighlightTableView(HighlightView* view);

    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    HighlightView* _view;
};

} // namespace zf
