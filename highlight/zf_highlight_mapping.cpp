#include "zf_highlight_mapping.h"
#include "zf_core.h"

namespace zf
{
void HighlightMapping::setHighlightWidgetMapping(const DataProperty& property, QWidget* w)
{
    Z_CHECK(_widgets->structure()->propertiesMain().contains(property));
    Z_CHECK_NULL(w);
    auto old = _highlight_widget_mapping.key(w);
    if (old.isValid())
        _highlight_property_mapping.remove(old);

    _highlight_widget_mapping[property] = w;

    QSet<QWidget*> check;
    for (auto i = _highlight_widget_mapping.cbegin(); i != _highlight_widget_mapping.cend(); ++i) {
        Z_CHECK(!check.contains(i.value()));
        check << i.value();
    }

    emit sg_mappingChanged(property);
}

void HighlightMapping::setHighlightWidgetMapping(const PropertyID& property_id, QWidget* w)
{
    setHighlightWidgetMapping(_widgets->structure()->property(property_id), w);
}

void HighlightMapping::setHighlightPropertyMapping(const DataProperty& source_property, const DataProperty& dest_property)
{        
    // удаляем старое
    bool changed = false;
    auto dst = _highlight_property_mapping.keys(source_property);
    for (auto& d : qAsConst(dst)) {
        _highlight_property_mapping.remove(d);
        _highlight_widget_mapping.remove(d);
        changed = true;
    }

    if (_highlight_property_mapping.remove(dest_property))
        changed = true;

    _highlight_widget_mapping.remove(dest_property);

    _highlight_property_mapping[dest_property] = source_property;
    setHighlightWidgetMapping(source_property, _widgets->field(dest_property));

    DataPropertySet check;
    for (auto i = _highlight_property_mapping.cbegin(); i != _highlight_property_mapping.cend(); ++i) {
        Z_CHECK(!check.contains(i.value()));
        check << i.value();
    }

    if (changed)
        emit sg_mappingChanged(source_property);

}

void HighlightMapping::setHighlightPropertyMapping(const PropertyID& source_property_id, const PropertyID& dest_property_id)
{
    setHighlightPropertyMapping(_widgets->structure()->property(source_property_id), _widgets->structure()->property(dest_property_id));
}

HighlightMapping::HighlightMapping(DataWidgets* widgets, QObject* parent)
    : QObject(parent)
    , _widgets(widgets)
{
    Z_CHECK_NULL(_widgets);
}

DataWidgets* HighlightMapping::widgets() const
{
    return _widgets;
}

QWidget* HighlightMapping::getHighlightWidget(const DataProperty& property) const
{
    Z_CHECK(property.isValid());
    initHighlightWidgetMappingLazy();

    if (property.isCell() || property.isColumn() || property.isRow())
        return nullptr;

    auto res = _highlight_widget_mapping.constFind(property);
    if (res != _highlight_widget_mapping.constEnd()) {
        if (res->isNull())
            return nullptr;

        return res->data();
    }

    return _widgets->object<QWidget>(property, false);
}

DataProperty HighlightMapping::getHighlightProperty(QWidget* w) const
{
    Z_CHECK_NULL(w);
    initHighlightWidgetMappingLazy();
    for (auto i = _highlight_widget_mapping.constBegin(); i != _highlight_widget_mapping.constEnd(); ++i) {
        if (i->data() == w)
            return i.key();
    }

    return _widgets->widgetProperty(w);
}

void HighlightMapping::initHighlightWidgetMappingLazy() const
{
    if (_highlight_widget_mapping_initialized)
        return;

    HighlightMapping* c = const_cast<HighlightMapping*>(this);

    c->_highlight_widget_mapping_initialized = true;
    for (auto& p : _widgets->structure()->propertiesByType(PropertyType::Field)) {
        auto constraints = p.constraints();
        for (auto& constr : qAsConst(constraints)) {
            auto shp = constr->showHiglightProperty();
            if (!shp.isValid() || shp == p)
                continue;

            c->setHighlightPropertyMapping(p.id(), shp);
        }
    }
}

DataProperty zf::HighlightMapping::getSourceHighlightProperty(const DataProperty& dest_property) const
{
    auto i = _highlight_property_mapping.constFind(dest_property);
    return i != _highlight_property_mapping.constEnd() ? i.value() : dest_property;
}

DataProperty HighlightMapping::getDestinationHighlightProperty(const DataProperty& source_property) const
{
    auto p = _highlight_property_mapping.key(source_property);
    return p.isValid() ? p : source_property;
}

} // namespace zf
