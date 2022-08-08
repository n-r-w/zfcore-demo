#include "zf_entity_object.h"
#include "zf_core.h"
#include "zf_framework.h"

#include <QApplication>
#include <QDebug>

using namespace std::placeholders;

namespace zf
{
EntityObject::EntityObject(const Uid& entity_uid, const DataStructurePtr& data_structure, HighlightProcessor* master_highlight,
                           const ModuleDataOptions& options)
    : ModuleDataObject(entity_uid.entityCode(), data_structure, master_highlight, options)
    , _entity_uid(entity_uid)
{
    setObjectName(entity_uid.toPrintable());
    internalCallbackManager()->addRequest(this, Framework::ENTITY_OBJECT_AFTER_CREATED);

    Z_CHECK(Core::messageDispatcher()->registerObject(_entity_uid, this, "sl_message_dispatcher_inbound",
                                                      "sl_message_dispatcher_inbound_advanced"));

    _is_exists = !_entity_uid.isTemporary();

    subscribeToChannels();

    _message_processor = new MessageProcessor(this, internalCallbackManager());

    connect(_message_processor, &MessageProcessor::sg_feedback, this, &EntityObject::sl_message_feedback);
    connect(
        _message_processor, &MessageProcessor::sg_startWaitingFeedback, this, &EntityObject::sl_startWaitingFeedback);
    connect(
        _message_processor, &MessageProcessor::sg_finishWaitingFeedback, this, &EntityObject::sl_finishWaitingFeedback);
    connect(_message_processor, &MessageProcessor::sg_requestAdded, this, &EntityObject::sl_requestAdded);
    connect(_message_processor, &MessageProcessor::sg_requestRemoved, this, &EntityObject::sl_requestRemoved);

    _command_processor = new CommandProcessor(this, internalCallbackManager());
    connect(_command_processor, &CommandProcessor::sg_commandAdded, this, &EntityObject::sl_commandAdded);
    connect(_command_processor, &CommandProcessor::sg_commandRemoved, this, &EntityObject::sl_commandRemoved);

    connect(&_inbound_msg_buffer_timer, &FeedbackTimer::timeout, this, [this]() { processInboundMessages(); });
}

EntityObject::~EntityObject()
{
    _message_processor->objectExtensionDestroy();
    _command_processor->objectExtensionDestroy();
}

Uid EntityObject::entityUid() const
{
    return _entity_uid;
}

bool EntityObject::isTemporary() const
{
    return _entity_uid.isTemporary();
}

bool EntityObject::isPersistent() const
{
    return _entity_uid.isPersistent();
}

bool EntityObject::isExists() const
{
    return _is_exists;
}

DatabaseID EntityObject::databaseId() const
{
    return _entity_uid.database();
}

DatabaseID EntityObject::childDatabaseId(const EntityCode& entity_code) const
{
    return Core::entityCodeDatabase(entityUid(), entity_code);
}

QString EntityObject::entityName() const
{
    QString name;
    if (!_entity_name.isEmpty()) {
        name = _entity_name;

    } else if (nameProperty().isValid()) {
        name = data()->value(nameProperty()).toString().simplified();
        if (!name.isEmpty()) {
            if (!name.contains('-'))
                name = moduleInfo().name() + " - " + name;
            else if (!name.contains(':'))
                name = moduleInfo().name() + ": " + name;
            else
                name = moduleInfo().name() + " " + name;

            if (isTemporary())
                name += " (" + ZF_TR(ZFT_CREATE).toLower() + ")";
        }
    }

    if (name.isEmpty()) {
        name = moduleInfo().name();
        if (isTemporary())
            name += " (" + ZF_TR(ZFT_CREATE).toLower() + ")";
    }

    return name;
}

DataProperty EntityObject::nameProperty() const
{
    if (_name_property == nullptr) {
        auto props = structure()->propertiesByOptions(PropertyType::Field, PropertyOption::FullName);
        if (props.isEmpty())
            props = structure()->propertiesByOptions(PropertyType::Field, PropertyOption::Name);
        if (!props.isEmpty())
            _name_property = std::make_unique<DataProperty>(props.first());
        else
            _name_property = std::make_unique<DataProperty>();
    }

    return *_name_property.get();
}

void EntityObject::setEntityName(const QString& s)
{
    QString name = s.simplified();

    if (name == _entity_name)
        return;

    _entity_name = name;
    onEntityNameChangedHelper();
}

bool EntityObject::isEntityNameRedefined() const
{
    return !_entity_name.isEmpty();
}

AccessRights EntityObject::directAccessRights() const
{
    return Core::connectionInformation()->directPermissions().rights(entityUid());
}

AccessRights EntityObject::relationAccessRights() const
{
    return Core::connectionInformation()->relationPermissions().rights(entityUid());
}

const ProgramFeatures& EntityObject::features()
{
    return Core::connectionInformation()->features();
}

I_Plugin* EntityObject::plugin() const
{
    return Core::getPlugin(entityCode());
}

QString EntityObject::getDebugName() const
{
    return QStringLiteral("%1, %2").arg(metaObject()->className()).arg(entityUid().toPrintable());
}

void EntityObject::setExists(bool b) const
{
    if (_is_exists == b)
        return;

    if (isTemporary())
        Z_CHECK(!b);

    _is_exists = b;
    emit const_cast<EntityObject*>(this)->sg_existsChanged();
}

bool EntityObject::postMessage(const I_ObjectExtension* receiver, const Message& message) const
{
    return Core::messageDispatcher()->postMessage(this, receiver, message);
}

bool EntityObject::postMessageCommand(const Command& message_key, const I_ObjectExtension* receiver, const Message& message,
                                      const std::shared_ptr<void>& custom_data) const
{
    if (_message_processor->objectName().isEmpty())
        _message_processor->setObjectName(metaObject()->className());

    if (_message_processor->property(MessageDispatcher::OWNER_NAME_PROPERTY).toString().isEmpty())
        _message_processor->setProperty(MessageDispatcher::OWNER_NAME_PROPERTY, metaObject()->className());

    return _message_processor->addMessageRequest(message_key, custom_data, receiver, message);
}

void EntityObject::onMessageCommandAdded(const Command& key)
{
    Q_UNUSED(key);
}

void EntityObject::onMessageCommandRemoved(const Command& key)
{
    Q_UNUSED(key);
}

bool EntityObject::isWaitingMessageCommand(const Command& message_key) const
{
    return _message_processor->isWaitingMessageFeedback(message_key);
}

bool EntityObject::isMessageCommandInQueue(const Command& message_key) const
{
    return _message_processor->inQueue(message_key);
}

bool EntityObject::postDatabaseMessageCommand(const Command& message_key, const Message& message,
                                              const std::shared_ptr<void>& custom_data) const
{
    return postMessageCommand(message_key, Core::databaseManager(), message, custom_data);
}

Error EntityObject::onMessageCommandFeedback(const Command& key, const std::shared_ptr<void>& custom_data, const Message& source_message,
                                             const Message& feedback_message)
{
    Q_UNUSED(key)
    Q_UNUSED(custom_data)
    Q_UNUSED(source_message)
    Q_UNUSED(feedback_message)
    return {};
}

void EntityObject::onStartWaitingMessageCommandFeedback(const Command& key)
{
    Q_UNUSED(key)
}

void EntityObject::onFinishWaitingMessageCommandFeedback(const Command& key)
{
    Q_UNUSED(key)
}

void EntityObject::registerError(const Error& error, ObjectActionType type)
{
    Z_CHECK(error.isError());
    emit sg_error(type, error);
}

void EntityObject::addCommandRequest(const Command& command_key, const std::shared_ptr<void>& custom_data) const
{
    startProgress();

    if (_command_processor->objectName().isEmpty())
        _command_processor->setObjectName(objectName());

    if (_command_processor->property(MessageDispatcher::OWNER_NAME_PROPERTY).toString().isEmpty())
        _command_processor->setProperty(MessageDispatcher::OWNER_NAME_PROPERTY, metaObject()->className());

    _command_processor->addCommandRequest(
        command_key, custom_data,
        std::bind(&EntityObject::processCommandHelper, const_cast<EntityObject*>(this), _1, _2));

    emit const_cast<EntityObject*>(this)->sg_commandReguested(command_key, custom_data);
}

int EntityObject::removeCommandRequests(const Command& command_key, const std::shared_ptr<void>& custom_data) const
{
    int count = _command_processor->removeCommandRequests(command_key, custom_data);
    if (count > 0) {
        int c = count;
        while (c > 0) {
            finishProgress();
            c--;
        }

        emit const_cast<EntityObject*>(this)->sg_commandRemoved(command_key, custom_data);
    }
    return count;
}

void EntityObject::processCommand(const Command& command_key, const std::shared_ptr<void>& custom_data)
{
    Q_UNUSED(command_key)
    Q_UNUSED(custom_data)
}

bool EntityObject::finishCommand() const
{
    Command current_command = _command_processor->currentCommand();
    auto current_data = _command_processor->currentData();

    bool res;
    if (_command_processor->finishCommand()) {
        finishProgress();
        res = true;
        emit const_cast<EntityObject*>(this)->sg_commandFinished(current_command, current_data);

    } else {
        res = false;
    }

    return res;
}

void EntityObject::onCommandRequestAdded(const Command& key, const std::shared_ptr<void>& custom_data)
{
    Q_UNUSED(key);
    Q_UNUSED(custom_data);
}

void EntityObject::onCommandRequestRemoved(const Command& key, const std::shared_ptr<void>& custom_data)
{
    Q_UNUSED(key);
    Q_UNUSED(custom_data);
}

bool EntityObject::isWaitingCommandExecute(const Command& command_key) const
{
    return _command_processor->isWaitingCommandExecute(command_key);
}

QList<std::shared_ptr<void>> EntityObject::commandData(const Command& command_key) const
{
    return _command_processor->commandData(command_key);
}

bool EntityObject::isCommandExecuting() const
{
    return _command_processor->isExecuting();
}

Command EntityObject::currentCommand() const
{
    return _command_processor->currentCommand();
}

std::shared_ptr<void> EntityObject::currentData() const
{
    return _command_processor->currentData();
}

Error EntityObject::processInboundMessage(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
    return {};
}

Error EntityObject::processInboundMessageAdvanced(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const Message& message,
                                                  SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender_ptr)
    Q_UNUSED(sender_uid)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
    return {};
}

void EntityObject::onEntityNameChanged()
{
}

void EntityObject::onConnectionChanged(const ConnectionInformation& info)
{
    Q_UNUSED(info)
}

void EntityObject::onPropertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    ModuleDataObject::onPropertyChanged(p, old_values);

    if (p == nameProperty())
        onEntityNameChangedHelper();
}

void EntityObject::onAllPropertiesUnBlocked()
{
    ModuleDataObject::onAllPropertiesUnBlocked();
    onEntityNameChangedHelper();
}

void EntityObject::onPropertyUnBlocked(const DataProperty& p)
{
    ModuleDataObject::onPropertyUnBlocked(p);
    if (p == nameProperty())
        onEntityNameChangedHelper();
}

void EntityObject::setEntity(const Uid& entity_uid)
{
    if (_entity_uid == entity_uid)
        return;

    Uid old = _entity_uid;
    _entity_uid = entity_uid;

    Z_CHECK(Core::messageDispatcher()->unRegisterObject(this));
    Z_CHECK(Core::messageDispatcher()->registerObject(_entity_uid, this, QStringLiteral("sl_message_dispatcher_inbound")));
    subscribeToChannels();

    emit sg_entityChanged(old, entity_uid);
}

void EntityObject::processCallbackInternal(int key, const QVariant& data)
{
    ModuleDataObject::processCallbackInternal(key, data);

    if (key == Framework::ENTITY_OBJECT_AFTER_CREATED) {
        setObjectName(QString("%1:%2").arg(metaObject()->className()).arg(entityUid().toPrintable()));
    }
}

void EntityObject::onEntityNameChangedHelper()
{
    onEntityNameChanged();
    emit sg_entityNameChanged();
}

void EntityObject::sl_message_feedback(const Command& key, const std::shared_ptr<void>& custom_data, const Message& source_message,
                                       const Message& feedback_message)
{
    Error error = onMessageCommandFeedback(key, custom_data, source_message, feedback_message);
    if (error.isError())
        registerError(error);
}

void EntityObject::sl_startWaitingFeedback(const Command& key)
{
    startProgress();
    onStartWaitingMessageCommandFeedback(key);
}

void EntityObject::sl_finishWaitingFeedback(const Command& key)
{
    onFinishWaitingMessageCommandFeedback(key);
    finishProgress();
}

void EntityObject::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    storeInboundMessage(sender, message, subscribe_handle);
}

void EntityObject::sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const Uid& sender_uid,
                                                          const Message& message, SubscribeHandle subscribe_handle)
{
    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (Z_VALID_HANDLE(_subscribe_handle_entity_removed) && subscribe_handle == _subscribe_handle_entity_removed) {
        setExists(false);
    }

    if (Z_VALID_HANDLE(_subscribe_handle_connection_info) && subscribe_handle == _subscribe_handle_connection_info
        && !message.feedbackMessageId().isValid()) {
        // рассылка о смене подключения
        onConnectionChanged(DBEventConnectionInformationMessage(message).information());
    }

    auto error = processInboundMessageAdvanced(sender_ptr, sender_uid, message, subscribe_handle);
    if (error.isError())
        registerError(error);
}

void EntityObject::sl_requestAdded(const Command& key)
{
    onMessageCommandAdded(key);
}

void EntityObject::sl_requestRemoved(const Command& key)
{
    onMessageCommandRemoved(key);
}

void EntityObject::sl_commandAdded(const Command& key, const std::shared_ptr<void>& custom_data)
{
    onCommandRequestAdded(key, custom_data);
}

void EntityObject::sl_commandRemoved(const Command& key, const std::shared_ptr<void>& custom_data)
{
    onCommandRequestRemoved(key, custom_data);
}

void EntityObject::processCommandHelper(const Command& command_key, const std::shared_ptr<void>& custom_data)
{    
    processCommand(command_key, custom_data);
    emit sg_commandExecuting(command_key, custom_data);
}

void EntityObject::subscribeToChannels()
{
    // подписка на информацию об изменении сущностей
    if (!_entity_uid.isTemporary()) {
        _subscribe_handle_entity_changed
            = Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_CHANGED, this, {entityCode()}, {_entity_uid});
        Z_CHECK(Z_VALID_HANDLE(_subscribe_handle_entity_changed));

        _subscribe_handle_entity_removed = Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_REMOVED, this, {}, {_entity_uid});
        Z_CHECK(Z_VALID_HANDLE(_subscribe_handle_entity_removed));
    }
    // подписка на информацию о подключении к БД
    _subscribe_handle_connection_info = Core::messageDispatcher()->subscribe(CoreChannels::CONNECTION_INFORMATION, this);
    Z_CHECK(Z_VALID_HANDLE(_subscribe_handle_connection_info));
}

void EntityObject::storeInboundMessage(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    auto i = std::make_shared<InboundMessageInfo>();
    i->sender = sender;
    i->message = message;
    i->subscribe_handle = subscribe_handle;
    _inbound_msg_buffer.enqueue(i);

    if (!_inbound_msg_buffer_processing)
        _inbound_msg_buffer_timer.start();
}

void EntityObject::processInboundMessages()
{
    Z_CHECK(!_inbound_msg_buffer_processing);
    _inbound_msg_buffer_processing = true;
    auto buffer = _inbound_msg_buffer;
    _inbound_msg_buffer.clear();
    while (!buffer.isEmpty()) {
        auto i = buffer.dequeue();
        auto error = processInboundMessage(i->sender, i->message, i->subscribe_handle);
        if (error.isError())
            registerError(error);
    }
    _inbound_msg_buffer_processing = false;
    if (!_inbound_msg_buffer.isEmpty())
        _inbound_msg_buffer_timer.start();
}

QString zf::EntityObject::entityConfigurationCode() const
{
    return QStringLiteral("id_%1").arg(entityUid().id().value());
}

} // namespace zf
