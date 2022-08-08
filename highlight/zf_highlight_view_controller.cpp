#include "zf_highlight_view_controller.h"

#include "zf_data_widgets.h"
#include "zf_highlight_view.h"
#include "zf_highlight_mapping.h"
#include "zf_view.h"
#include "zf_framework.h"

namespace zf
{
HightlightViewController::HightlightViewController(I_HightlightViewController* interface, HighlightView* highlight_view,
                                                   HighlightMapping* mapping)
    : QObject()
    , _interface(interface)
    , _highlight_view(highlight_view)
    , _mapping(mapping)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_interface);
    Z_CHECK_NULL(_highlight_view);
    Z_CHECK_NULL(_mapping);

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    connect(_interface->hightlightControllerGetWidgets(), &DataWidgets::sg_focusChanged, this, &HightlightViewController::sl_focusChanged);
    connect(_highlight_view, &HighlightView::sg_currentChanged, this, &HightlightViewController::sl_currentChanged);
    connect(_highlight_view, &HighlightView::sg_currentEdited, this, &HightlightViewController::sl_currentEdited);

    connect(_highlight_view->highlightProcessor(), &HighlightProcessor::sg_highlightItemInserted, this,
            &HightlightViewController::sl_itemInserted);
    connect(_highlight_view->highlightProcessor(), &HighlightProcessor::sg_highlightItemRemoved, this,
            &HightlightViewController::sl_itemRemoved);
    connect(_highlight_view->highlightProcessor(), &HighlightProcessor::sg_highlightItemChanged, this,
            &HightlightViewController::sl_itemChanged);
    connect(_highlight_view->highlightProcessor(), &HighlightProcessor::sg_highlightBeginUpdate, this,
            &HightlightViewController::sl_beginUpdate);
    connect(_highlight_view->highlightProcessor(), &HighlightProcessor::sg_highlightEndUpdate, this, &HightlightViewController::sl_endUpdate);
}

HightlightViewController::~HightlightViewController()
{
    delete _object_extension;
}

bool HightlightViewController::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void HightlightViewController::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant HightlightViewController::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool HightlightViewController::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void HightlightViewController::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void HightlightViewController::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void HightlightViewController::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void HightlightViewController::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void HightlightViewController::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void HightlightViewController::focusError(View* form)
{
    Z_CHECK_NULL(form);

    // если уже находится в поле с ошибкой, то ничего не делаем
    DataProperty current_property = _interface->hightlightControllerGetCurrentFocus();
    if (current_property.isValid()
        && highlightModel(true)->contains(current_property, InformationType::Error | InformationType::Critical | InformationType::Fatal))
        return;

    auto items = highlightModel(true)->items(InformationType::Error | InformationType::Critical | InformationType::Fatal);
    if (items.isEmpty())
        return;

    setFocus(items.first().property());
}

void HightlightViewController::focusNextError(View* form, InformationTypes info_type)
{
    Z_CHECK_NULL(form);

    DataProperty current_property = _interface->hightlightControllerGetCurrentFocus();

    DataPropertyList properties = highlightModel(true)->properties(info_type);
    Z_CHECK(!properties.isEmpty());

    DataProperty next_property;

    if (current_property.isValid()) {
        int pos = properties.indexOf(current_property);
        if (pos >= 0) {
            if (pos == properties.count() - 1)
                pos = 0;
            else
                pos++;

            next_property = properties.at(pos);
        }
    }
    if (!next_property.isValid())
        next_property = properties.first();

    if (next_property == current_property)
        return;

    setFocus(next_property);
}

void HightlightViewController::sl_focusChanged(QWidget* previous_widget, const zf::DataProperty& previous_property, QWidget* current_widget,
                                           const zf::DataProperty& current_property)
{
    Q_UNUSED(previous_widget)
    Q_UNUSED(previous_property)

    if (objectExtensionDestroyed())
        return;

    if (current_widget != nullptr && (current_widget == _highlight_view || Utils::hasParent(current_widget, _highlight_view)))
        return; // игнорируем панель ошибок

    if (_block_counter > 0)
        return;

    if (current_property.isValid() && _focus_request != nullptr && _focus_request->isValid()
        && _focus_request->propertyType() == PropertyType::Cell && _focus_request->dataset() == current_property)
        return;

    _focus_request = Z_MAKE_SHARED(DataProperty, current_property);

    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_CONTROLLER_SET_FOCUS);
}

void HightlightViewController::sl_currentChanged(const DataProperty& previous, const DataProperty& current)
{
    Q_UNUSED(previous)
    Q_UNUSED(current)
}

void HightlightViewController::sl_currentEdited(const DataProperty& previous, const DataProperty& current)
{
    Q_UNUSED(previous)

    if (_block_counter > 0)
        return;
    _block_counter++;

    if (current.isValid())
        setFocus(current);

    _block_counter--;
}

void HightlightViewController::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::HIGHLIGHT_CONTROLLER_SYNCHRONIZE) {
        synchronizeHelper();

    } else if (key == Framework::HIGHLIGHT_CONTROLLER_SET_FOCUS) {
        processFocusRequest();
    }
}

void HightlightViewController::sl_itemInserted(const HighlightItem& item)
{
    Q_UNUSED(item)
    synchronize();
}

void HightlightViewController::sl_itemRemoved(const HighlightItem& item)
{
    Q_UNUSED(item)
    synchronize();
}

void HightlightViewController::sl_itemChanged(const HighlightItem& item)
{
    Q_UNUSED(item)
    synchronize();
}

void HightlightViewController::sl_beginUpdate()
{
}

void HightlightViewController::sl_endUpdate()
{
    synchronize();
}

void HightlightViewController::synchronize()
{
    if (objectExtensionDestroyed())
        return;

    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_CONTROLLER_SYNCHRONIZE);
}

void HightlightViewController::synchronizeHelper()
{
    if (_block_counter > 0)
        return;

    _block_counter++;

    auto focused
        = _interface->hightlightControllerGetMapping()->getSourceHighlightProperty(_interface->hightlightControllerGetCurrentFocus());

    if (highlightModel(false)->contains(focused))
        _highlight_view->setCurrentItem(focused);
    else
        _highlight_view->setCurrentItem({});

    _block_counter--;
}

void HightlightViewController::processFocusRequest()
{
    if (_block_counter > 0 || _focus_request == nullptr)
        return;

    _block_counter++;

    zf::DataProperty real_p = _interface->hightlightControllerGetMapping()->getSourceHighlightProperty(*_focus_request);

    if (real_p.isValid() && highlightModel(true)->contains(real_p)) {
        _highlight_view->setCurrentItem(real_p);

    } else if (real_p.isValid() && highlightModel(true)->contains(real_p.dataset())) {
        _highlight_view->setCurrentItem(real_p.dataset());

    } else {
        _highlight_view->setCurrentItem(DataProperty());
    }

    _focus_request.reset();

    _block_counter--;

    synchronize();
}

void HightlightViewController::setFocus(const DataProperty& property)
{
    auto p = _mapping->getDestinationHighlightProperty(property);
    _interface->hightlightControllerBeforeFocusHighlight(p);
    _interface->hightlightControllerSetFocus(p);
    _interface->hightlightControllerAfterFocusHighlight(p);
}

HighlightModel* HightlightViewController::highlightModel(bool execute_check) const
{
    if (execute_check)
        _highlight_view->highlightProcessor()->executeHighlightCheckRequests();
    return const_cast<HighlightModel*>(_highlight_view->highlightProcessor()->highlight());
}

} // namespace zf
