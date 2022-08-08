#include "zf_widget_replacer_mn.h"
#include "zf_core.h"

namespace zf
{
WidgetReplacerManual::WidgetReplacerManual(QWidget* source, QWidget* target, const QString& new_name, bool copy_properties)
    : _source(source)
    , _target(target)
    , _new_name(new_name)
    , _copy_properties(copy_properties)
{
    Z_CHECK_NULL(source);
    Z_CHECK_NULL(target);
    Z_CHECK(!new_name.isEmpty());
}

void WidgetReplacerManual::replaceWidget(QWidget* source, QWidget* target, const QString& new_name, bool copy_properties)
{
    Z_CHECK_NULL(source);
    Z_CHECK_NULL(target);
    Z_CHECK_X(source->parentWidget() != nullptr, "Widget not placed on form");

    WidgetReplacerManual* r = new WidgetReplacerManual(source, target, new_name, copy_properties);
    auto new_widget = r->replace(source->parentWidget());
    Z_CHECK(new_widget.count() == 1 && new_widget.constFirst() == target);
    delete r;
}

bool WidgetReplacerManual::isNeedReplace(const QWidget* widget, QString& tag, QString& new_name)
{
    if (widget == _source) {
        tag.clear();
        new_name = _new_name;
        return true;
    }
    return false;
}

void WidgetReplacerManual::createReplacement(const QString& tag, const QString& new_name, const QWidget* widget, bool& is_hide,
                                             QWidget*& replacement, QWidget*& replacement_container,
                                             QHash<QString, QSet<QString>>*& properties, DataProperty& data_property)
{
    Q_UNUSED(tag)
    Q_UNUSED(new_name)
    Q_UNUSED(widget)
    Q_UNUSED(is_hide)
    Q_UNUSED(replacement_container)    
    Q_UNUSED(data_property)

    replacement = _target;
    if (!_copy_properties)
        properties = new QHash<QString, QSet<QString>>();
}

} // namespace zf
