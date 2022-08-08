#include "zf_widget_highlighter.h"
#include "zf_core.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QGraphicsEffect>
#include <QPainter>
#include <QScrollArea>

#include "zf_core.h"
#include "zf_core_messages.h"
#include "zf_message_dispatcher.h"
#include "zf_graphics_effects.h"
#include "zf_plain_text_edit.h"

namespace zf
{
WidgetHighlighter::WidgetHighlighter(QObject* parent)
    : QObject(parent)
{
}

void WidgetHighlighter::addHighlight(QWidget* widget, const HighlightItem& item)
{
    Z_CHECK(widget != nullptr);
    Z_CHECK(item.isValid());

    auto data = _highlight.value(widget, nullptr);
    if (data == nullptr) {
        data = Z_MAKE_SHARED(QList<HighlightItem>);
        _highlight[widget] = data;
    }
    if (data->contains(item))
        return;

    data->append(item);
    if (!_highlight.contains(widget))
        connect(widget, &QWidget::destroyed, this, &WidgetHighlighter::sl_destroyed);
    auto linked = linkedWidget(widget);
    if (linked != nullptr && !_linked_widgets.contains(widget)) {
        _linked_widgets[widget] = linked;
        connect(linked->linked_widget, &QWidget::destroyed, this, &WidgetHighlighter::sl_linketDestroyed);
    }

    updateVisualization(widget, linked);
}

void WidgetHighlighter::removeHighlight(QWidget* widget, const HighlightItem& item)
{
    Z_CHECK(widget != nullptr);

    bool need_remove = false;
    if (!item.isValid()) {
        need_remove = true;

    } else {
        auto data = _highlight.value(widget, nullptr);
        if (data == nullptr)
            return;
        if (!data->removeOne(item))
            return;

        if (data->isEmpty()) {
            need_remove = true;

        } else
            _highlight[widget] = data;
    }

    auto linked = linkedWidget(widget);
    if (need_remove) {
        _highlight.remove(widget);
        disconnect(widget, &QWidget::destroyed, this, &WidgetHighlighter::sl_destroyed);

        if (linked != nullptr) {
            _linked_widgets.remove(widget);
            disconnect(linked->linked_widget, &QWidget::destroyed, this, &WidgetHighlighter::sl_linketDestroyed);
        }
    }

    updateVisualization(widget, linked);
}

QList<HighlightItem> WidgetHighlighter::highlightList(QWidget* widget) const
{
    Z_CHECK(widget != nullptr);
    auto res = _highlight.value(widget, nullptr);
    return res == nullptr ? QList<HighlightItem>() : *res;
}

HighlightItem WidgetHighlighter::highlight(QWidget* widget) const
{
    return _current_highlight.value(widget);
}

void WidgetHighlighter::clear()
{
    auto all = _highlight.keys();
    for (QObject* w : all) {
        removeHighlight(qobject_cast<QWidget*>(w));
    }
}

HighlightItem WidgetHighlighter::calcHighlight(QWidget* widget) const
{
    Z_CHECK(widget != nullptr);
    auto list = highlightList(widget);
    if (list.isEmpty())
        return HighlightItem();

    std::sort(list.begin(), list.end(),
        [](const HighlightItem& h1, const HighlightItem& h2) -> bool { return h1.type() < h2.type(); });

    return list.last();
}

void WidgetHighlighter::sl_destroyed()
{
    QObject* obj = sender();
    _highlight.remove(obj);
    _current_highlight.remove(obj);
}

void WidgetHighlighter::sl_linketDestroyed()
{
    QObject* obj = sender();

    for (auto i = _linked_widgets.constBegin(); i != _linked_widgets.constEnd(); ++i) {
        if (i.value()->linked_widget != obj)
            continue;

        _linked_widgets.remove(i.key());
        break;
    }
}

void WidgetHighlighter::updateVisualization(QWidget* widget, const std::shared_ptr<LinkedInfo>& linked)
{
    Z_CHECK(widget != nullptr);

    HighlightItem old_type = highlight(widget);
    HighlightItem new_type = calcHighlight(widget);

    if (!new_type.isValid())
        _current_highlight.remove(widget);
    else
        _current_highlight[widget] = new_type;

    if (linked != nullptr) {
        if (old_type.isValid()) {
            // очищаем старую визуализацию
            linked->linked_widget->setGraphicsEffect(nullptr);
            if (linked->is_both)
                linked->base_widget->setGraphicsEffect(nullptr);
        }

        if (new_type.isValid()) {
            // активируем новую визуализацию
            linked->linked_widget->setGraphicsEffect(new HighlightBorderEffect(linked->linked_widget, new_type.type()));
            if (linked->is_both)
                linked->base_widget->setGraphicsEffect(new HighlightBorderEffect(linked->base_widget, new_type.type()));
        }
    } else {
        if (old_type.isValid()) {
            // очищаем старую визуализацию
            widget->setGraphicsEffect(nullptr);
        }

        if (new_type.isValid()) {
            // активируем новую визуализацию
            widget->setGraphicsEffect(new HighlightBorderEffect(widget, new_type.type()));
        }
    }
}

std::shared_ptr<WidgetHighlighter::LinkedInfo> WidgetHighlighter::linkedWidget(QWidget* w) const
{
    auto info = Z_MAKE_SHARED(LinkedInfo);
    info->base_widget = w;
    info->linked_widget = w;

    if (auto ed = qobject_cast<PlainTextEdit*>(w)) {
        info->linked_widget = ed->viewport();
        info->is_both = true;
        return info;
    }

    if (auto sa = qobject_cast<QAbstractScrollArea*>(w)) {
        info->linked_widget = sa->viewport();
        info->is_both = false;
        return info;
    }

    return nullptr;
}

} // namespace zf
