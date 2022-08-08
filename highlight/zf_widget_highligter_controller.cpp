#include "zf_widget_highligter_controller.h"
#include "zf_data_widgets.h"
#include "zf_widget_highlighter.h"
#include "zf_highlight_mapping.h"
#include "zf_core_messages.h"
#include "zf_framework.h"
#include <QToolTip>

namespace zf
{
WidgetHighligterController::WidgetHighligterController(HighlightProcessor* processor, DataWidgets* widgets, WidgetHighlighter* highlighter,
                                                       HighlightMapping* mapping)
    : QObject()
    , _highlight_processor(processor)
    , _widgets(widgets)
    , _widget_highlighter(highlighter)
    , _mapping(mapping)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_highlight_processor);
    Z_CHECK_NULL(_widgets);
    Z_CHECK_NULL(_widget_highlighter);
    Z_CHECK_NULL(_mapping);

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemInserted, this,
            &WidgetHighligterController::sl_highlight_itemInserted);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemRemoved, this,
            &WidgetHighligterController::sl_highlight_itemRemoved);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemChanged, this,
            &WidgetHighligterController::sl_highlight_itemChanged);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightBeginUpdate, this,
            &WidgetHighligterController::sl_highlight_beginUpdate);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightEndUpdate, this, &WidgetHighligterController::sl_highlight_endUpdate);

    connect(_mapping, &HighlightMapping::sg_mappingChanged, this, &WidgetHighligterController::sl_mappingChanged);

    // для ItemSelector при изменении состояния readOnly по разному обрабатывается проверка на корректность lookup значения в методе View::getHighlight
    for (auto& p : _widgets->structure()->propertiesMain()) {
        ItemSelector* w = qobject_cast<ItemSelector*>(_mapping->getHighlightWidget(p));
        if (w == nullptr)
            continue;

        connect(w, &ItemSelector::sg_readOnlyChanged, this, &WidgetHighligterController::sl_itemSelectorReadOnlyChanged);
    }
}

WidgetHighligterController::~WidgetHighligterController()
{
    delete _object_extension;
}

void WidgetHighligterController::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant WidgetHighligterController::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool WidgetHighligterController::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void WidgetHighligterController::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void WidgetHighligterController::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void WidgetHighligterController::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void WidgetHighligterController::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void WidgetHighligterController::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

DataWidgets* WidgetHighligterController::widgets() const
{
    return _widgets;
}

bool WidgetHighligterController::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void WidgetHighligterController::sl_highlight_itemInserted(const HighlightItem& item)
{
    if (objectExtensionDestroyed())
        return;

    DataProperty p = item.property();
    if (p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Dataset) {
        QWidget* w = _mapping->getHighlightWidget(p);
        if (w == nullptr)
            return;

        _widget_highlighter->addHighlight(w, item);
    }

    _items_to_update << item;
    Framework::internalCallbackManager()->addRequest(this, Framework::WIDGET_HIGHLIGHT_CONTROLLER_UPDATE_HIGHLIGHT_KEY);

    updateStatusBarErrorInfo();
}

void WidgetHighligterController::sl_highlight_itemRemoved(const HighlightItem& item)
{
    if (objectExtensionDestroyed())
        return;

    DataProperty p = item.property();
    if (p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Dataset) {
        QWidget* w = _mapping->getHighlightWidget(p);
        if (w == nullptr)
            return;

        _widget_highlighter->removeHighlight(w, item);
    }

    _items_to_update << item;
    Framework::internalCallbackManager()->addRequest(this, Framework::WIDGET_HIGHLIGHT_CONTROLLER_UPDATE_HIGHLIGHT_KEY);

    updateStatusBarErrorInfo();
}

void WidgetHighligterController::sl_highlight_itemChanged(const HighlightItem& item)
{
    sl_highlight_itemInserted(item);
}

void WidgetHighligterController::sl_highlight_beginUpdate()
{
}

void WidgetHighligterController::sl_highlight_endUpdate()
{
    loadAllHighlights();    
}

void WidgetHighligterController::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data);
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::WIDGET_HIGHLIGHT_CONTROLLER_UPDATE_HIGHLIGHT_KEY) {
        for (auto& i : qAsConst(_items_to_update)) {
            updateWidgetHighlight(i);
        }
        _items_to_update.clear();
    }
}

void WidgetHighligterController::sl_mappingChanged(const DataProperty& source_property)
{
    _highlight_processor->registerHighlightCheck(source_property);
}

void WidgetHighligterController::sl_itemSelectorReadOnlyChanged()
{
    auto w = qobject_cast<ItemSelector*>(sender());
    auto p = _mapping->getHighlightProperty(w);
    if (p.isValid())
        _highlight_processor->registerHighlightCheck(p);
}

void WidgetHighligterController::updateWidgetHighlight(const HighlightItem& item)
{
    if (item.property().propertyType() == PropertyType::Cell) {
        QAbstractItemView* w = _widgets->object<QAbstractItemView>(item.property().dataset(), false);
        if (w != nullptr && w->isVisible()) {
            QModelIndex idx = _widgets->data()->cellIndex(item.property());
            if (idx.isValid()) {
                idx = Utils::alignIndexToModel(idx, w->model());
                if (idx.isValid()) {
                    w->update(idx);
                }
            }
        }

    } else if (item.property().isDatasetPart() || item.property().propertyType() == PropertyType::Dataset) {
        if (!item.property().dataset().options().testFlag(PropertyOption::SimpleDataset)) {
            QAbstractItemView* w = _widgets->object<QAbstractItemView>(item.property().dataset(), false);
            if (w != nullptr && w->isVisible()) {
                w->update();
            }
        }
    }

    if (item.property().propertyType() == PropertyType::Field || item.property().propertyType() == PropertyType::Dataset) {
        // обновляем тултип с ошибкой
        QStringList errors;
        QStringList warnings;
        auto items = _highlight_processor->highlight()->items(item.property());
        for (auto& i : items) {
            if (i.type() == InformationType::Error || i.type() == InformationType::Critical || i.type() == InformationType::Fatal)
                errors << i.text();
            else if (i.type() == InformationType::Warning)
                warnings << i.text();
        }

        QString text = QStringList(errors + warnings).join("<br>");
        QWidget* w = _widgets->object<QWidget>(item.property());
        w->setToolTip(text.simplified());

        if (text.isEmpty() && qApp->widgetAt(QCursor::pos()) == w)
            QToolTip::hideText();
    }
}

void WidgetHighligterController::updateStatusBarErrorInfo()
{
    MessageUpdateStatusBar::updateStatusBarErrorInfo(_widgets, _highlight_processor);
}

void WidgetHighligterController::loadAllHighlights()
{
    _widget_highlighter->clear();
    for (int i = 0; i < _highlight_processor->highlight()->count(); i++) {
        HighlightItem item = _highlight_processor->highlight()->item(i);
        QWidget* w = _mapping->getHighlightWidget(item.property());
        if (w == nullptr)
            continue;

        _widget_highlighter->addHighlight(w, item);
    }
    updateStatusBarErrorInfo();
}

} // namespace zf
