#include "zf_entity_name_widget.h"
#include "zf_core.h"
#include "zf_line_edit.h"
#include <QHBoxLayout>

namespace zf
{
class EntityNameWidgetEditor : public LineEdit
{
public:
    EntityNameWidgetEditor(QWidget* parent)
        : LineEdit(parent)
    {
    }
};

class EntityNameWidgetLabel : public QLabel
{
public:
    EntityNameWidgetLabel(QWidget* parent)
        : QLabel(parent)
    {
    }
};

EntityNameWidget::EntityNameWidget(QWidget* parent)
    : QWidget(parent)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::ENTITY_NAME_WIDGET, this, QStringLiteral("sl_message_dispatcher_inbound")));

    _layout = new QHBoxLayout;
    setLayout(_layout);
    _layout->setMargin(0);

    initEditor();
}

EntityNameWidget::~EntityNameWidget()
{
    delete _object_extension;
}

Uid EntityNameWidget::entityUid() const
{
    return _entity_uid;
}

void EntityNameWidget::setEntityUid(const Uid& entity_uid, const PropertyID& name_property)
{
    if (_entity_uid == entity_uid && _name_property == name_property)
        return;

    if (_entity_uid.isValid()) {
        Core::messageDispatcher()->unSubscribe(_subscribe_handle_entity_changed);
        Core::messageDispatcher()->unSubscribe(_subscribe_handle_entity_removed);
        _subscribe_handle_entity_changed = 0;
        _subscribe_handle_entity_removed = 0;
    }

    if (entity_uid.isValid()) {
        _subscribe_handle_entity_changed
            = Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_CHANGED, this, EntityCodeList(), {entity_uid});
        _subscribe_handle_entity_removed
            = Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_REMOVED, this, EntityCodeList(), {entity_uid});
    }

    _entity_uid = entity_uid;
    _name_property = name_property;

    requestUpdateName();
}

QString EntityNameWidget::name() const
{
    return _name;
}

PropertyID EntityNameWidget::nameProperty() const
{
    return _name_property;
}

void EntityNameWidget::setOverrideText(const QString& s)
{
    if (_override_text == s)
        return;

    _override_text = s;
    setEditorText(_override_text.isEmpty() ? _name : _override_text);
}

void EntityNameWidget::setType(Type type)
{
    if (_type == type)
        return;

    _type = type;
    initEditor();
}

EntityNameWidget::Type EntityNameWidget::type() const
{
    return _type;
}

bool EntityNameWidget::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void EntityNameWidget::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender);

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (_message_id.isValid() && message.feedbackMessageId() == _message_id) {
        if (message.messageType() == MessageType::Error) {
            setName(ErrorMessage(message).error().fullText());

        } else {
            MMEventEntityNamesMessage msg(message);
            if (msg.errors().at(0).isError())
                setName(msg.errors().at(0).fullText());
            else
                setName(msg.names().at(0));
        }
    }

    if (Z_VALID_HANDLE(_subscribe_handle_entity_changed) && subscribe_handle == _subscribe_handle_entity_changed) {
        requestUpdateName();
    }

    if (Z_VALID_HANDLE(_subscribe_handle_entity_removed) && subscribe_handle == _subscribe_handle_entity_removed) {
        requestUpdateName();
    }
}

void EntityNameWidget::requestUpdateName()
{
    if (!_entity_uid.isValid()) {
        setName(QString());
        return;
    }

    _message_id = Core::getEntityName(this, _entity_uid, _name_property);
}

void EntityNameWidget::setName(const QString& name)
{
    _name = name;
    setEditorText(_override_text.isEmpty() ? _name : _override_text);
    _message_id.clear();
    emit sg_nameReceived();
}

void EntityNameWidget::initEditor()
{
    if (_type == LineEditType) {
        Z_CHECK(_editor == nullptr);
        if (_label != nullptr) {
            delete _label;
            _label = nullptr;
        }

        _editor = new EntityNameWidgetEditor(this);
        _layout->addWidget(_editor);
        _editor->setReadOnly(true);

    } else {
        Z_CHECK(_label == nullptr);
        if (_editor != nullptr) {
            delete _editor;
            _editor = nullptr;
        }

        _label = new EntityNameWidgetLabel(this);
        _layout->addWidget(_label);
    }
}

void EntityNameWidget::setEditorText(const QString& s)
{
    if (_editor != nullptr)
        _editor->setText(s);
    else if (_label != nullptr)
        _label->setText(s);
    else
        Z_HALT_INT;
}

void EntityNameWidget::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant EntityNameWidget::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool EntityNameWidget::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void EntityNameWidget::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void EntityNameWidget::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void EntityNameWidget::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void EntityNameWidget::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void EntityNameWidget::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

} // namespace zf
