#pragma once

#include "zf.h"
#include "zf_highlight_model.h"
#include <QWidget>

namespace zf
{
//! Отвечает за отрисовку состояния ошибок и предупупреждений на произвольных виджетах
class ZCORESHARED_EXPORT WidgetHighlighter : public QObject
{
    Q_OBJECT
public:
    WidgetHighlighter(QObject* parent = nullptr);

    //! Добавить состояние для виджета
    void addHighlight(QWidget* widget, const HighlightItem& item);
    //! Удалить состояние виджета
    void removeHighlight(QWidget* widget,
        //! По умолчанию - удалить все
        const HighlightItem& item = HighlightItem());

    //! Список состояний для виджета
    QList<HighlightItem> highlightList(QWidget* widget) const;
    //! Активное состояние виджета (с наивысшим приоритетом)
    HighlightItem highlight(QWidget* widget) const;

    //! Удалить состояния со всех виджетов
    void clear();

private slots:
    //! Виджет был удален
    void sl_destroyed();
    //! Виджет был удален
    void sl_linketDestroyed();

private:
    //! Информация о связанном виджете
    struct LinkedInfo
    {
        QWidget* base_widget = nullptr;
        QWidget* linked_widget = nullptr;
        //! Состояние ошибки надо отрисовывать на обеих виджетах
        bool is_both = false;
    };

    //! Активное состояние виджета (с наивысшим приоритетом)
    HighlightItem calcHighlight(QWidget* widget) const;
    //! Обновить отображение состояния виджета
    void updateVisualization(QWidget* widget, const std::shared_ptr<LinkedInfo>& linked);

    QHash<QObject*, std::shared_ptr<QList<HighlightItem>>> _highlight;
    QHash<QObject*, HighlightItem> _current_highlight;

    //! Получить связанный виджет
    std::shared_ptr<LinkedInfo> linkedWidget(QWidget* w) const;

    //! Ключ - виджет на котором отображено состояние, значение связанный с ним виджет
    //! Например QPlainEdit - viewPort()
    QHash<QObject*, std::shared_ptr<LinkedInfo>> _linked_widgets;
};
} // namespace zf
