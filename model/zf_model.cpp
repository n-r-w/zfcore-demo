#include "zf_core.h"
#include "zf_framework.h"
#include "zf_translation.h"

#include <QDebug>

using namespace std::placeholders;

namespace zf
{
Model::Model(const Uid& entity_uid, bool is_detached, const DatabaseObjectOptions& database_options, const ModuleDataOptions& data_options)
    : EntityObject(entity_uid, getDataStructureHelper(entity_uid), nullptr, data_options)
    , _is_detached(is_detached)
    , _options(database_options)
{
    bootstrap();
}

Model::Model(const Uid& entity_uid, bool is_detached, const DataStructurePtr& data_structure, const DatabaseObjectOptions& database_options,
    const ModuleDataOptions& data_options)
    : EntityObject(entity_uid, data_structure, nullptr, data_options)
    , _is_detached(is_detached)
    , _options(database_options)
{
    bootstrap();
}

Model::~Model()
{
    _send_message_loop.quit();
#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    QString thread_info;
    if (!Utils::isMainThread())
        thread_info = QStringLiteral(", no main thread");
    Core::logInfo("model deleted: " + debugName() + thread_info);

#endif
#endif
}

DatabaseObjectOptions Model::databaseOptions() const
{
    return _options;
}

Uid Model::persistentUid() const
{
    return entityUid().isPersistent() ? entityUid() : _persistent_uid;
}

int Model::id() const
{
    Z_CHECK(persistentUid().isValid());
    return persistentUid().id().value();
}

bool Model::isDetached() const
{
    return _is_detached;
}

Error Model::nonCriticalSaveError() const
{
    return _non_critical_save_error;
}

DataContainerPtr Model::originalData() const
{
    return _original_data;
}

bool Model::hasUnsavedData(zf::Error& error, const DataPropertyList& ignored_properties, zf::DataContainer::ChangedBinaryMethod changed_binary_method) const
{
    if (_original_data == nullptr)
        return isTemporary();

    auto ignored_props = structure()
                             ->propertiesByOptions(PropertyType::Field, PropertyOption::DBWriteIgnored)
                             .toSet()
                             .unite(structure()->propertiesByOptions(PropertyType::Dataset, PropertyOption::DBWriteIgnored).toSet())
                             .unite(ignored_properties.toSet());

    return _original_data->hasDiff(data().get(), ignored_props, error, changed_binary_method);
}

bool Model::hasUnsavedData(Error& error, const PropertyIDList& ignored_properties, DataContainer::ChangedBinaryMethod changed_binary_method) const
{
    return hasUnsavedData(error, structure()->toPropertyList(ignored_properties), changed_binary_method);
}

bool Model::hasUnsavedData(const DataPropertyList& ignored_properties, DataContainer::ChangedBinaryMethod changed_binary_method) const
{
    Error error;
    bool res = hasUnsavedData(error, ignored_properties, changed_binary_method);
    Z_CHECK_ERROR(error);
    return res;
}

bool Model::hasUnsavedData(const PropertyIDList& ignored_properties, DataContainer::ChangedBinaryMethod changed_binary_method) const
{
    return hasUnsavedData(structure()->toPropertyList(ignored_properties), changed_binary_method);
}

bool Model::invalidate(const ChangeInfo& info) const
{
    bool res = EntityObject::invalidate(info);

    if (res && !isTemporary()) {
        Core::databaseManager()->entityInvalidated(entityUid(), entityUid());
#ifdef RNIKULENKOV
        Core::logInfo(QString("Entity invalidated %1").arg(entityUid().toPrintable()));
#endif
    }

    return res;
}

Model::CommandExecuteResult Model::reload(const LoadOptions& load_options, const DataPropertySet& properties)
{
    if (isTemporary())
        return CommandExecuteResult::Ignored; // временные вообще нельзя грузить из БД

    if (properties.count()) {
        for (auto& p : qAsConst(properties)) {
            data()->setInvalidate(p, true);
        }
    } else
        data()->setInvalidateAll(true);

    return load(load_options, properties);
}

Model::CommandExecuteResult Model::reload(const PropertyIDSet& properties)
{
    return reload(LoadOptions(), structure()->propertiesToSet(properties));
}

Model::CommandExecuteResult Model::load(const LoadOptions& load_options, const DataPropertySet& properties)
{
    if (isTemporary())
        return CommandExecuteResult::Ignored; // временные вообще нельзя грузить из БД

    // определяем что собственно надо грузить
    auto requested_props = properties.isEmpty() ? structure()->propertiesMain() : properties;

    // валидные свойства грузить не надо
    auto invalidated = data()->whichPropertiesInvalidated(requested_props, true);
    auto not_initialized = data()->whichPropertiesInitialized(requested_props, false);
    requested_props = invalidated.unite(not_initialized);

    DataPropertySet target_props;
    for (auto& p : qAsConst(requested_props))     {
        if (p.options().testFlag(PropertyOption::DBReadIgnored))
            continue;
        target_props << p;
    }

    target_props.unite(extendLoadProperties(target_props));

    if (target_props.isEmpty())
        return CommandExecuteResult::Ignored;

    emit sg_beforeLoad(load_options, target_props);

    auto c_info = Z_MAKE_SHARED(LoadInfo);
    c_info->properties = target_props;
    c_info->options = load_options;
    getLoadParameters(c_info->parameters);

    CommandExecuteResult result = tryExecuteCommand(ModuleCommands::Load, c_info);
    if (result == CommandExecuteResult::Added) {
        startProgress();
    }

    if (result != CommandExecuteResult::Ignored && !_was_load_request) {
        _was_load_request = true;
        emit sg_onFirstLoadRequest();
    }

    return result;
}

Model::CommandExecuteResult Model::load(const PropertyIDSet& properties)
{
    return load(LoadOptions(), structure()->propertiesToSet(properties));
}

Model::CommandExecuteResult Model::load(const DataPropertySet& properties)
{
    return load(LoadOptions(), properties);
}

Error Model::loadSync(const LoadOptions& load_options, const DataPropertySet& properties, int timeout_ms)
{
    load(load_options, properties);

    if (!isLoading())
        return {};

    if (timeout_ms > 0)
        _send_message_timer->start(timeout_ms);
    _send_message_loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    return _sync_error;
}

Error Model::loadSync(const PropertyIDSet& properties, int timeout_ms)
{
    DataPropertySet props;
    for (auto& p : properties) {
        props << property(p);
    }
    return loadSync(LoadOptions(), props, timeout_ms);
}

bool Model::isLoading(bool is_waiting) const
{
    return (isCommandExecuting() && currentCommand() == ModuleCommands::Load) || (is_waiting && isWaitingCommandExecute(ModuleCommands::Load));
}

bool Model::isLoadingComplete() const
{
    if (isLoading())
        return false;

    return _is_loading_complete;
}

bool Model::postLoadMessageCommand(const Command& message_key, const DataPropertySet& properties) const
{
    return postMessageCommand(message_key, Core::databaseManager(), DBCommandGetEntityMessage(entityUid(), properties));
}

bool Model::postLoadMessageCommand(const Command& message_key, const PropertyIDSet& properties) const
{
    DataPropertySet props;
    for (auto& p : properties) {
        props << property(p);
    }
    return postLoadMessageCommand(message_key, props);
}

Model::CommandExecuteResult Model::save(const DataPropertySet& properties, const QVariant& by_user)
{
    _non_critical_save_error.clear();

    if (!isDetached() && !isTemporary() &&
        // модели, созданные вне основного потока не являются разделяемыми и их можно смело менять и сохранять
        Utils::isMainThread(this))
        Z_CHECK(databaseOptions().testFlag(DatabaseObjectOption::AllowSaveNotDatached));

    // определяем что собственно надо сохранять
    auto requested_props = properties.isEmpty() ? data()->initializedProperties() : properties;

    DataPropertySet target_props;
    for (auto& p : qAsConst(requested_props)) {
        if (p.options().testFlag(PropertyOption::DBWriteIgnored))
            continue;
        target_props << p;
    }

    if (isTemporary() && target_props.isEmpty())
        Z_HALT("Попытка сохранить модель без постоянного идентификатора без наличия данных для сохранения");

    if (target_props.isEmpty())
        return CommandExecuteResult::Ignored;

    auto c_info = Z_MAKE_SHARED(SaveInfo);
    c_info->properties = target_props;
    c_info->by_user = by_user;
    getSaveParameters(c_info->parameters);

    CommandExecuteResult result = tryExecuteCommand(ModuleCommands::Save, c_info);
    if (result == CommandExecuteResult::Added)
        startProgress();

    return result;
}

Model::CommandExecuteResult Model::save(const PropertyIDSet& properties, const QVariant& by_user)
{
    return save(structure()->propertiesToSet(properties), by_user);
}

Model::CommandExecuteResult Model::save(const QVariant& by_user)
{
    return save(DataPropertySet {}, by_user);
}

Error Model::saveSync(const DataPropertySet& properties, Uid& persistent_uid, int timeout_ms, const QVariant& by_user)
{
    save(properties, by_user);

    if (!isSaving()) {
        persistent_uid = persistentUid();
        Z_CHECK(persistent_uid.isValid());
        return {};
    }

    if (timeout_ms > 0)
        _send_message_timer->start(timeout_ms);
    _send_message_loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    persistent_uid = persistentUid();
    return _sync_error;
}

Error Model::saveSync(const PropertyIDSet& properties, Uid& persistent_uid, int timeout_ms, const QVariant& by_user)
{
    return saveSync(structure()->propertiesToSet(properties), persistent_uid, timeout_ms, by_user);
}

Error Model::saveSync(Uid& persistent_uid, int timeout_ms, const QVariant& by_user)
{
    return saveSync(DataPropertySet {}, persistent_uid, timeout_ms, by_user);
}

Error Model::saveSync(int timeout_ms, const QVariant& by_user)
{
    Uid persistent_uid;
    return saveSync(persistent_uid, timeout_ms, by_user);
}

bool Model::isSaving(bool is_waiting) const
{
    return (isCommandExecuting() && currentCommand() == ModuleCommands::Save) || (is_waiting && isWaitingCommandExecute(ModuleCommands::Save));
}

bool Model::postSaveMessageCommand(const Command& message_key, const DataPropertySet& properties, const QVariant& by_user) const
{
    DataContainer detached_data = *data();
    detached_data.detach();

    Message message;
    if (by_user.isValid())
        message = zf::DBCommandWriteEntityMessage(entityUid(), detached_data, properties, (originalData() != nullptr ? *originalData() : DataContainer()),
            QMap<QString, QVariant>(), by_user.toBool());
    else
        message = zf::DBCommandWriteEntityMessage(entityUid(), detached_data, properties, (originalData() != nullptr ? *originalData() : DataContainer()));

    return postMessageCommand(message_key, Core::databaseManager(), message);
}

bool Model::postSaveMessageCommand(const Command& message_key, const PropertyIDSet& properties, const QVariant& by_user) const
{
    return postSaveMessageCommand(message_key, structure()->propertiesToSet(properties), by_user);
}

Model::CommandExecuteResult Model::remove(const QVariant& by_user)
{
    if (!isExists())
        return CommandExecuteResult::Ignored;

    auto c_info = Z_MAKE_SHARED(RemoveInfo);
    getRemoveParameters(c_info->parameters);
    c_info->by_user = by_user;

    CommandExecuteResult result = tryExecuteCommand(ModuleCommands::Remove, c_info);

    if (result == CommandExecuteResult::Added)
        startProgress();

    return result;
}

Error Model::removeSync(int timeout_ms, const QVariant& by_user)
{
    remove(by_user);

    if (!isRemoving())
        return {};

    if (timeout_ms > 0)
        _send_message_timer->start(timeout_ms);
    _send_message_loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    return _sync_error;
}

bool Model::isRemoving(bool is_waiting) const
{
    return (isCommandExecuting() && currentCommand() == ModuleCommands::Remove) || (is_waiting && isWaitingCommandExecute(ModuleCommands::Remove));
}

Error Model::executeOperation(const Operation& op)
{
    ProgressHelper ph;
    if (op.options() & OperationOption::ShowProgress) {
        ph.reset(this);
        startProgress();
    }

    emit sg_operationBeforeProcess(op);
    Error error = processOperation(op);
    emit sg_operationProcessed(op, error);
    return error;
}

Error Model::executeOperation(const zf::OperationID& operation_id)
{
    return executeOperation(operation(operation_id));
}

void Model::requestWidgetUpdate(QWidget* widget)
{
    if (!isLoading())
        return;

    _widget_update_requested.insert(QPointer<QWidget>(widget));
}

void Model::requestWidgetUpdate(const QWidgetList& widgets)
{
    for (auto w : widgets) {
        requestWidgetUpdate(w);
    }
}

void Model::copyFrom(const ModuleDataObject* source, bool save_original_data)
{
    EntityObject::copyFrom(source);
    if (isDetached() && save_original_data)
        saveOriginalData();
}

void Model::doCopyFrom(const zf::ModuleDataObject* source)
{
    EntityObject::doCopyFrom(source);
    if (isDetached())
        saveOriginalData();
}

ModelPtr Model::clone(const Uid& new_uid, bool keep_detached) const
{
    Error error;
    zf::ModelPtr m = nullptr;
    if (isDetached() && keep_detached) {
        m = Core::detachModel<Model>(entityUid(), LoadOptions(), PropertyIDSet(), error);
    } else
        m = Core::createModel<Model>(databaseId(), entityCode(), error);

    Z_CHECK_ERROR(error);

    if (new_uid.isValid())
        m->setEntity(new_uid);

    if (isDetached() && keep_detached)
        m->copyFrom(this, false);
    else
        m->copyFrom(this);

    if (isDetached() && keep_detached)
        m->_original_data = _original_data;

    return m;
}

AccessRights Model::directAccessRights() const
{
    return _direct_rights == nullptr ? EntityObject::directAccessRights() : *_direct_rights;
}

AccessRights Model::relationAccessRights() const
{
    return _relation_rights == nullptr ? EntityObject::directAccessRights() : *_relation_rights;
}

QString Model::entityName() const
{
    return EntityObject::entityName();
}

void Model::keep()
{
    Z_CHECK(!_is_constructed);
    if (Utils::isMainThread())
        _is_model_keeped = true;
}

DataPropertySet Model::extendLoadProperties(const DataPropertySet& requested)
{
    Q_UNUSED(requested);
    return {};
}

bool Model::isKeeped() const
{
    return _is_model_keeped;
}

Error Model::processOperation(const Operation& op)
{
    Q_UNUSED(op)
    return Error();
}

Error Model::beforeLoad(const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
    return Error();
}

Error Model::afterLoad(const LoadOptions& load_options, const DataPropertySet& properties, const zf::Error& error)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
    return error;
}

Error Model::beforeSave(const DataPropertySet& properties)
{
    Q_UNUSED(properties)
    return Error();
}

void Model::afterSave(const zf::DataPropertySet& requested_properties, const zf::DataPropertySet& saved_properties, Error& error)
{
    Q_UNUSED(requested_properties)
    Q_UNUSED(saved_properties)
    Q_UNUSED(error)
}

Error Model::beforeRemove()
{
    return Error();
}

void Model::afterRemove(Error& error)
{
    Q_UNUSED(error)
}

Error Model::onLoadConfiguration()
{
    return {};
}

Error Model::onSaveConfiguration() const
{
    return {};
}

Error Model::onMessageCommandFeedback(
    const zf::Command& key, const std::shared_ptr<void>& custom_data, const zf::Message& source_message, const zf::Message& feedback_message)
{
    Q_UNUSED(source_message)

    Error error;

    if (key == ModuleCommands::Load)
        error = finishLoadHelper(custom_data, feedback_message, Error(), AccessRights(), AccessRights());
    else if (key == ModuleCommands::Save)
        error = finishSaveHelper(custom_data, feedback_message, Error(), Uid(), Error());
    else if (key == ModuleCommands::Remove)
        error = finishRemoveHelper(custom_data, feedback_message, Error());

    return error;
}

zf::Error Model::processInboundMessage(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    auto error = EntityObject::processInboundMessage(sender, message, subscribe_handle);

    if (message.messageType() == MessageType::DBEventEntityChanged && !isProgress()) {
        DBEventEntityChangedMessage msg(message);
        if (msg.contains(entityUid()) || msg.entityCodes().contains(entityCode()))
            invalidate();
    }

    return error;
}

bool Model::isHighlightViewIsCheckRequired() const
{
    return EntityObject::isHighlightViewIsCheckRequired() && (!isExists() || isDetached()) && moduleDataOptions().testFlag(ModuleDataOption::HighlightEnabled);
}

void Model::processCallbackInternal(int key, const QVariant& data)
{
    EntityObject::processCallbackInternal(key, data);

    if (key == Framework::MODEL_FINISH_CUSTOM_LOAD) {
        finishCustomLoadHelper(_custom_error, _custom_load_direct_rights, _custom_load_relation_rights);
        _custom_error.clear();
        _custom_load_direct_rights.clear();
        _custom_load_relation_rights.clear();

    } else if (key == Framework::MODEL_FINISH_CUSTOM_SAVE) {
        finishCustomSaveHelper(_custom_error, _custom_save_persistent_uid, _custom_save_non_critical_error);
        _custom_error.clear();
        _custom_save_persistent_uid.clear();

    } else if (key == Framework::MODEL_FINISH_CUSTOM_REMOVE) {
        finishCustomRemoveHelper(_custom_error);
        _custom_error.clear();
    }
}

bool Model::commandContains(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2)
{
    if (command_key == ModuleCommands::Load)
        return std::static_pointer_cast<LoadInfo>(command_info_1)->contains(std::static_pointer_cast<LoadInfo>(command_info_2).get());

    if (command_key == ModuleCommands::Save)
        return std::static_pointer_cast<SaveInfo>(command_info_1)->contains(std::static_pointer_cast<SaveInfo>(command_info_2).get());

    if (command_key == ModuleCommands::Remove)
        return std::static_pointer_cast<RemoveInfo>(command_info_1)->contains(std::static_pointer_cast<RemoveInfo>(command_info_2).get());

    Z_HALT_INT;
    return false;
}

bool Model::commandEqual(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2)
{
    if (command_key == ModuleCommands::Load)
        return std::static_pointer_cast<LoadInfo>(command_info_1)->equal(std::static_pointer_cast<LoadInfo>(command_info_2).get());

    if (command_key == ModuleCommands::Save)
        return std::static_pointer_cast<SaveInfo>(command_info_1)->equal(std::static_pointer_cast<SaveInfo>(command_info_2).get());

    if (command_key == ModuleCommands::Remove)
        return std::static_pointer_cast<RemoveInfo>(command_info_1)->equal(std::static_pointer_cast<RemoveInfo>(command_info_2).get());

    Z_HALT_INT;
    return false;
}

bool Model::commandMerge(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2,
    std::shared_ptr<void>& target_command_info)
{
    if (command_key == ModuleCommands::Load) {
        auto info1 = std::static_pointer_cast<LoadInfo>(command_info_1);
        auto info2 = std::static_pointer_cast<LoadInfo>(command_info_2);

        if (info1->options != info2->options)
            return false;

        if (!Utils::equal(info1->parameters, info2->parameters))
            return false;

        auto target_info = Z_MAKE_SHARED(LoadInfo);
        target_info->options = info1->options;
        target_info->parameters = info1->parameters;
        target_info->properties = info1->properties.unite(info2->properties);

        target_command_info = target_info;

        return true;
    }

    if (command_key == ModuleCommands::Save) {
        auto info1 = std::static_pointer_cast<SaveInfo>(command_info_1);
        auto info2 = std::static_pointer_cast<SaveInfo>(command_info_2);

        if (!Utils::equal(info1->parameters, info2->parameters))
            return false;

        auto target_info = Z_MAKE_SHARED(SaveInfo);
        target_info->parameters = info1->parameters;
        target_info->properties = info1->properties.unite(info2->properties);

        target_command_info = target_info;

        return true;
    }

    if (command_key == ModuleCommands::Remove) {
        auto info1 = std::static_pointer_cast<RemoveInfo>(command_info_1);
        auto info2 = std::static_pointer_cast<RemoveInfo>(command_info_2);

        if (!Utils::equal(info1->parameters, info2->parameters))
            return false;

        auto target_info = Z_MAKE_SHARED(SaveInfo);
        target_info->parameters = info1->parameters;

        target_command_info = target_info;

        return true;
    }

    Z_HALT_INT;
    return false;
}

Error Model::onStartCustomLoad(const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
    return Error();
}

void Model::finishCustomLoad(const Error& error, const AccessRights& direct_rights, const AccessRights& relation_rights) const
{
    if (objectExtensionDestroyed())
        return;

    Z_CHECK(_is_custom_db_operation);
    _is_custom_db_operation = false;
    _custom_error = error;
    _custom_load_direct_rights = direct_rights;
    _custom_load_relation_rights = relation_rights;

    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_SAVE));
    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_REMOVE));

    // надо развязать вызовы, чтобы можно было вызывать finishCustomLoad внутри onStartCustomLoad
    internalCallbackManager()->addRequest(this, Framework::MODEL_FINISH_CUSTOM_LOAD);
}

Error Model::onStartCustomSave(const DataPropertySet& properties)
{
    Q_UNUSED(properties)
    return Error();
}

void Model::finishCustomSave(const Error& error, const Uid& persistent_uid, const Error& non_critical_error) const
{
    if (objectExtensionDestroyed())
        return;

    Z_CHECK(_is_custom_db_operation);
    _is_custom_db_operation = false;
    _custom_error = error;
    _custom_save_non_critical_error = non_critical_error;

    if (databaseOptions().testFlag(DatabaseObjectOption::StandardAfterSaveExtension)) {
        Z_CHECK(!persistent_uid.isValid());
        if (error.isOk())
            _custom_save_persistent_uid = persistentUid();

    } else {
        _custom_save_persistent_uid = persistent_uid;
    }

    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_LOAD));
    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_REMOVE));

    // надо развязать вызовы, чтобы можно было вызывать finishCustomSave внутри onStartCustomSave
    internalCallbackManager()->addRequest(this, Framework::MODEL_FINISH_CUSTOM_SAVE);
}

Error Model::onStartCustomRemove()
{
    return Error();
}

void Model::finishCustomRemove(const Error& error) const
{
    if (objectExtensionDestroyed())
        return;

    Z_CHECK(_is_custom_db_operation);
    _is_custom_db_operation = false;
    _custom_error = error;

    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_LOAD));
    Z_CHECK(!internalCallbackManager()->isRequested(this, Framework::MODEL_FINISH_CUSTOM_SAVE));

    // надо развязать вызовы, чтобы можно было вызывать finishCustomRemove внутри onStartCustomRemove
    internalCallbackManager()->addRequest(this, Framework::MODEL_FINISH_CUSTOM_REMOVE);
}

void Model::registerError(const Error& error, ObjectActionType type)
{
    EntityObject::registerError(error, type);
}

void Model::getLoadParameters(QMap<QString, QVariant>& parameters) const
{
    parameters = _load_parameters;
}

void Model::setLoadParameters(const QMap<QString, QVariant>& parameters)
{
    _load_parameters = parameters;
}

void Model::getSaveParameters(QMap<QString, QVariant>& parameters) const
{
    parameters = _save_parameters;
}

void Model::setSaveParameters(const QMap<QString, QVariant>& parameters)
{
    _save_parameters = parameters;
}

void Model::getRemoveParameters(QMap<QString, QVariant>& parameters) const
{
    parameters = _remove_parameters;
}

void Model::setRemoveParameters(const QMap<QString, QVariant>& parameters)
{
    _remove_parameters = parameters;
}

void Model::setSyncError(const Error& error) const
{
    _sync_error = error;
}

void Model::bootstrap()
{
    if (_is_detached)
        Z_CHECK(!isTemporary());

    // проверка криворукости
    if (_options.testFlag(DatabaseObjectOption::CustomLoad))
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::StandardLoadExtension));
    if (_options.testFlag(DatabaseObjectOption::StandardLoadExtension))
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::CustomLoad));

    if (_options.testFlag(DatabaseObjectOption::CustomSave)) {
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::StandardBeforeSaveExtension));
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::StandardAfterSaveExtension));
    }
    if (_options.testFlag(DatabaseObjectOption::StandardBeforeSaveExtension)) {
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::CustomSave));
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::StandardAfterSaveExtension));
    }

    if (_options.testFlag(DatabaseObjectOption::StandardAfterSaveExtension)) {
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::CustomSave));
        Z_CHECK(!_options.testFlag(DatabaseObjectOption::StandardBeforeSaveExtension));
    }

    if (_options.testFlag(DatabaseObjectOption::Dummy))
        Z_CHECK(_options == DatabaseObjectOptions(DatabaseObjectOption::Dummy));

    _send_message_timer = new QTimer(this);

    _send_message_timer->setSingleShot(true);

    // регистрация в internalCallbackManager должны быть в классе - предшественнике
    connect(_send_message_timer, &QTimer::timeout, this, [&]() { _send_message_loop.quit(); });
}

void Model::afterCreated()
{
    _is_constructed = true;

#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    QString thread_info;
    if (!Utils::isMainThread())
        thread_info = QStringLiteral(", no main thread");
    Core::logInfo("model created: " + debugName() + thread_info);
#endif
#endif
}

void Model::onEntityNameChangedHelper()
{
    EntityObject::onEntityNameChangedHelper();
}

Error Model::processOperationHelper(const Operation& op)
{
    return executeOperation(op);
}

zf::Model::CommandExecuteResult Model::tryExecuteCommand(const Command& command_key, const std::shared_ptr<void>& custom_data)
{
    CommandExecuteResult result = CommandExecuteResult::Added;
    std::shared_ptr<void> target_сustom_data = custom_data;

    // смотрим нет ли уже такой команды
    if (isCommandExecuting() && currentCommand() == command_key) {
        // операция уже выполняется
        if (!commandContains(command_key, currentData(), custom_data)) {
            //выполняемая команда не содержит новой, надо объединить данные и отправить заново
            std::shared_ptr<void> megred_data;
            // если нельзя объединить - значит неверное использование модели
            Z_CHECK(commandMerge(command_key, currentData(), custom_data, megred_data));
            Z_CHECK_NULL(megred_data);
            // сливаем две команды
            target_сustom_data = megred_data;
            result = CommandExecuteResult::Added;

        } else {
            return CommandExecuteResult::Ignored;
        }
    }

    if (isWaitingCommandExecute(command_key)) {
        // операция ждет выполнения
        auto commands_info = commandData(command_key);
        Z_CHECK(commands_info.count() == 1);
        auto c_info_old_ptr = commands_info.constFirst();

        std::shared_ptr<void> megred_data;
        // если нельзя объеденить - значит неверное использование модели
        Z_CHECK(commandMerge(command_key, c_info_old_ptr, custom_data, megred_data));
        Z_CHECK_NULL(megred_data);
        // удаляем лежащую в очереди команду
        Z_CHECK(removeCommandRequests(command_key, c_info_old_ptr) == 1);
        // сливаем две команды
        target_сustom_data = megred_data;
        result = CommandExecuteResult::Merged;
    }

#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    if (result == CommandExecuteResult::Added) {
        QString thread_info;
        if (!Utils::isMainThread())
            thread_info = QStringLiteral(", no main thread");
        if (command_key == ModuleCommands::Load)
            Core::logInfo(QStringLiteral("model start load: %1%2").arg(debugName(), thread_info));
        else if (command_key == ModuleCommands::Save)
            Core::logInfo(QStringLiteral("model start save: %1%2").arg(debugName(), thread_info));
        else if (command_key == ModuleCommands::Remove)
            Core::logInfo(QStringLiteral("model start remove: %1%2").arg(debugName(), thread_info));
    }
#endif

#endif

    // добавляем команду
    addCommandRequest(command_key, target_сustom_data);

    return result;
}

void Model::startLoadHelper(const std::shared_ptr<zf::Model::LoadInfo>& info) const
{
    // надо очистить идентификаторы строк
    for (const auto& p : qAsConst(info->properties)) {
        if (!p.isDataset() || !data()->isInitialized(p))
            continue;

        clearDatasetRowIDs(p, dataset(p), QModelIndex());
    }
}

void Model::processCommand(const Command& command_key, const std::shared_ptr<void>& custom_data)
{
    /* TODO ВАЖНО!!! Cейчас тут неверная логика обработки ошибок, надо исправить. Сейчас пока не трогаю, чтобы не сломать то что работает.
     * 1. Вызовы beforeLoad, beforeRemove, beforeSave могут вернуть ошибку. При этом операция не начнется и фунция завершения типа finishLoadHelper
     * вызвана не будет, а значит ошибка просто не будет обработана
     * 2. onStartCustomLoad и т.п. могут вернуть ошибку. При этом операция загрузки не начнется. Однако ранее будет вызван сигнал sg_startLoad, но 
     * т.к. isLoading == false, то finishLoadHelper вызван не будет и операция подвиснет в без завершения */

    if (command_key == ModuleCommands::Load) {
        _is_loading_complete = false;

        // принудительно останавливаем отслеживание изменений
        auto tracking = trackingIDs();
        for (const auto& id : qAsConst(tracking)) {
            stopTracking(id);
        }

        data()->blockAllProperties();

        auto load_data = std::reinterpret_pointer_cast<LoadInfo>(custom_data);
        Error error = beforeLoad(load_data->options, load_data->properties);

        if (error.isOk() && databaseOptions().testFlag(DatabaseObjectOption::Dummy)) {
            // не надо грузить данные
            Z_CHECK(!databaseOptions().testFlag(DatabaseObjectOption::CustomLoad));

            emitStartLoad();
            finishLoadHelper(custom_data, Message(), Error(), AccessRights(), AccessRights());

        } else {
            if (error.isOk()) {
                if (databaseOptions().testFlag(DatabaseObjectOption::CustomLoad)) {
                    Z_CHECK(!_is_custom_db_operation);
                    _custom_db_operation_data = custom_data;
                    _is_custom_db_operation = true;

                    emitStartLoad();
                    error = const_cast<Model*>(this)->onStartCustomLoad(load_data->options, load_data->properties);

                } else {
                    postMessageCommand(ModuleCommands::Load, Core::databaseManager(),
                        DBCommandGetEntityMessage(entityUid(), load_data->properties, load_data->parameters), custom_data);
                }
            }

            if (error.isError() && isLoading())
                finishLoadHelper(custom_data, Message(), error, AccessRights(), AccessRights());
            else
                startLoadHelper(load_data);
        }

    } else if (command_key == ModuleCommands::Remove) {
        Error error = beforeRemove();

        auto remove_data = std::reinterpret_pointer_cast<RemoveInfo>(custom_data);

        if (error.isOk()) {
            if (databaseOptions().testFlag(DatabaseObjectOption::CustomRemove)) {
                Z_CHECK(!_is_custom_db_operation);
                _custom_db_operation_data = custom_data;
                _is_custom_db_operation = true;

                emit sg_startRemove();
                error = const_cast<Model*>(this)->onStartCustomRemove();

            } else {
                Message message;
                if (remove_data->by_user.isValid())
                    message = DBCommandRemoveEntityMessage(entityUid(), remove_data->parameters, remove_data->by_user.toBool());
                else
                    message = DBCommandRemoveEntityMessage(entityUid(), remove_data->parameters);

                postMessageCommand(ModuleCommands::Remove, Core::databaseManager(), message, custom_data);
            }
        }

        if (error.isError() && isRemoving())
            finishRemoveHelper(custom_data, Message(), error);

    } else if (command_key == ModuleCommands::Save) {
        // принудительно формируем идентификаторы строк чтобы контейнер ушел на сохранение с ними
        for (const auto& dataset_p : structure()->propertiesByType(PropertyType::Dataset)) {
            forceCalculateRowID(dataset_p);
        }

        auto save_data = std::reinterpret_pointer_cast<SaveInfo>(custom_data);
        Error error = beforeSave(save_data->properties);

        if (error.isOk()) {
            if (databaseOptions().testFlag(DatabaseObjectOption::CustomSave) || databaseOptions().testFlag(DatabaseObjectOption::StandardBeforeSaveExtension)) {
                Z_CHECK(!_is_custom_db_operation);
                _custom_db_operation_data = custom_data;
                _is_custom_db_operation = true;

                emit sg_startSave();
                error = const_cast<Model*>(this)->onStartCustomSave(save_data->properties);

            } else {
                DataContainer detached_data = *data();
                detached_data.detach();

                Message message;
                if (save_data->by_user.isValid())
                    message = DBCommandWriteEntityMessage(entityUid(), detached_data, save_data->properties,
                        (originalData() != nullptr ? *originalData() : DataContainer()), save_data->parameters, save_data->by_user.toBool());
                else
                    message = DBCommandWriteEntityMessage(entityUid(), detached_data, save_data->properties,
                        (originalData() != nullptr ? *originalData() : DataContainer()), save_data->parameters);

                postMessageCommand(ModuleCommands::Save, Core::databaseManager(), message, custom_data);
            }
        }

        if (error.isError() && isSaving())
            finishSaveHelper(custom_data, Message(), error, Uid(), Error());
    }
}

static void _update_invalidate_helper(const DataContainerPtr& data, const DataPropertySet& properties)
{
    for (auto& p : qAsConst(properties)) {
        if (!data->isInitialized(p))
            data->initValue(p);
        data->setInvalidate(p, false);
    }
}

Error Model::finishLoadHelper(const std::shared_ptr<void>& custom_data, const Message& message, const Error& error, const AccessRights& custom_direct_rights,
    const AccessRights& custom_relation_rights) const
{
    Z_CHECK(isLoading());

    auto info = std::reinterpret_pointer_cast<LoadInfo>(custom_data);
    Z_CHECK_NULL(info);

    Error error_f;
    AccessRights direct_rights;
    AccessRights relation_rights;
    bool access_changed = false;

    if (message.isValid()) {
        Z_CHECK_ERROR(error);
        if (message.messageType() == MessageType::DBEventEntityLoaded) {
            DBEventEntityLoadedMessage m(message);
            auto uids = m.entityUids();
            Z_CHECK(uids.count() == 1 && uids.first() == entityUid());
            auto m_data = m.data();
            Z_CHECK(!m_data.isEmpty());
            auto& model_data = m_data.constFirst();

            if (model_data.isValid()) {
                // копируем только те, что были загружены
                data()->copyFrom(&model_data, info->properties.intersect(model_data.initializedProperties()), true, CloneContainerDatasetMode::MoveContent);

                // надо пометить все полученные свойства как валидные, т.к. не факт что они были отмечены так в драйвере
                _update_invalidate_helper(data(), info->properties);

                // это надо сделать после установки состояния валидности
                if (!_options.testFlag(DatabaseObjectOption::StandardLoadExtension) && isDetached())
                    saveOriginalData();

            } else {
                // надо пометить все полученные свойства как валидные, т.к. не факт что они были отмечены так в драйвере
                _update_invalidate_helper(data(), info->properties);
            }

            direct_rights = m.directRights().constFirst();
            relation_rights = m.relationRights().constFirst();
            access_changed = true;

        } else if (message.messageType() == MessageType::Error) {
            // надо пометить запрошенные свойства как валидные, иначе они будут запрашиваться до бесконечности
            _update_invalidate_helper(data(), info->properties);
            error_f = ErrorMessage(message).error();

        } else
            Z_HALT_INT;

    } else if (error.isError()) {
        // надо пометить запрошенные свойства как валидные, иначе они будут запрашиваться до бесконечности
        _update_invalidate_helper(data(), info->properties);
        error_f = error;

    } else {
        // нет входящего сообщения, значит это вызов CustomLoad или Dummy
        if (_options.testFlag(DatabaseObjectOption::CustomLoad) || _options.testFlag(DatabaseObjectOption::StandardLoadExtension)) {
            // считаем что программист не дебил и при вызове finishCustomLoad данные в модель уже загружены
            _update_invalidate_helper(data(), info->properties);

            if (isDetached())
                saveOriginalData();

            direct_rights = custom_direct_rights;
            relation_rights = custom_relation_rights;
            access_changed = true;

        } else if (_options.testFlag(DatabaseObjectOption::Dummy)) {
            // ничего не делаем
        } else
            Z_HALT_INT;
    }

    if (!_is_standard_load_extension && error_f.isOk() && _options.testFlag(DatabaseObjectOption::StandardLoadExtension)) {
        // асинхронное расширение стандартной загрузки
        Z_CHECK(!_is_custom_db_operation);
        _is_standard_load_extension = true;
        _custom_db_operation_data = custom_data;
        _is_custom_db_operation = true;
        error_f = const_cast<Model*>(this)->onStartCustomLoad(info->options, info->properties);
        if (error.isOk())
            return {}; // выходим и ждем когда программист вызовет finishCustomLoad
    }

    error_f = const_cast<Model*>(this)->afterLoad(info->options, info->properties, error_f);

    Z_CHECK(currentCommand() == ModuleCommands::Load);
    finishCommand();

    _custom_db_operation_data.reset();
    _is_custom_db_operation = false;
    _is_standard_load_extension = false;
    setSyncError(error_f);

    _send_message_loop.quit();

    data()->unBlockAllProperties();

    if (access_changed)
        setAccessRights(direct_rights, relation_rights);

    if (error_f.isOk())
        const_cast<Model*>(this)->onLoadConfiguration();

    // если в процессе загрузки начали еще одну, то не вызываем до окончания
    if (!isLoading()) {
        _is_start_load_emmited = false;
        emit const_cast<Model*>(this)->sg_finishLoad(error_f, info->options, info->properties);
        // загрузка завершена
        _is_loading_complete = true;
    }

    updateRequestedWidgets();

    finishProgress();

#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    QString thread_info;
    if (!Utils::isMainThread())
        thread_info = QStringLiteral(", no main thread");
    Core::logInfo(QStringLiteral("model finish load: %1%2%3")
                      .arg(debugName())
                      .arg(error_f.isError() ? QStringLiteral(" (error: %1)").arg(error_f.fullText()) : QString())
                      .arg(thread_info));
#endif
#endif

    return error_f;
}

zf::Error Model::finishSaveHelper(
    const std::shared_ptr<void>& custom_data, const Message& message, const Error& error, const Uid& persistent_uid, const Error& non_critical_error) const
{
    Z_CHECK(isSaving());

    auto info = std::reinterpret_pointer_cast<SaveInfo>(custom_data);
    Z_CHECK_NULL(info);

    DataPropertySet saved_properties;
    Error error_f;
    bool no_action = false;
    if (message.isValid()) {
        Z_CHECK_ERROR(error);
        Z_CHECK(!persistent_uid.isValid());

        if (message.messageType() == MessageType::Error) {
            _persistent_uid.clear();
            error_f = ErrorMessage(message).error();

        } else if (message.messageType() == MessageType::Confirm) {
            auto c_message = ConfirmMessage(message);
            auto v_data = c_message.data().value<QVariantList>();
            // данные должны быть списком из 3 элементов: список ключей, набор записанных свойств, список некритичных ошибок
            Z_CHECK(v_data.count() == 3);
            no_action = c_message.noAction();

            if (isTemporary()) {
                auto uids = v_data.at(0).value<UidList>();
                Z_CHECK(uids.count() == 1);
                _persistent_uid = uids.at(0);
                Z_CHECK(_persistent_uid.isPersistent());
            }

            saved_properties = v_data.at(1).value<DataPropertySet>();
            if (saved_properties.isEmpty())
                saved_properties = info->properties;

            // формируется в RequestProcessor::execute_DBCommandWriteEntity при условии что
            // сервер при добавлении нового объекта возвращает его id и при этом еще и ошибку
            ErrorList non_critical_errors = v_data.at(2).value<ErrorList>();
            Z_CHECK(non_critical_errors.count() == 1);
            _non_critical_save_error = non_critical_errors.at(0);

        } else
            Z_HALT_INT;

    } else if (error.isError()) {
        if (isTemporary())
            _persistent_uid.clear();
        error_f = error;

    } else {
        // нет входящего сообщения, значит это вызов CustomSave
        Z_CHECK(_options.testFlag(DatabaseObjectOption::CustomSave) || databaseOptions().testFlag(DatabaseObjectOption::StandardAfterSaveExtension));

        if (!persistentUid().isValid() && persistent_uid.isValid())
            _persistent_uid = persistent_uid;
    }

    if (!_is_standard_after_save_extension && error_f.isOk() && _options.testFlag(DatabaseObjectOption::StandardAfterSaveExtension)) {
        // асинхронное расширение стандартного сохранения
        Z_CHECK(!_is_custom_db_operation);
        _is_standard_after_save_extension = true;
        _custom_db_operation_data = custom_data;
        _is_custom_db_operation = true;
        error_f = const_cast<Model*>(this)->onStartCustomSave(info->properties);
        if (error.isOk())
            return {}; // выходим и ждем когда программист вызовет finishCustomSave
    }

    Z_CHECK(currentCommand() == ModuleCommands::Save);
    finishCommand();

    if (_non_critical_save_error.isOk() && non_critical_error.isError())
        _non_critical_save_error = non_critical_error;

    _custom_db_operation_data.reset();
    _is_custom_db_operation = false;
    _is_standard_after_save_extension = false;
    setSyncError(error_f.setNonCriticalError(_non_critical_save_error));

    if (!isSaving()) // если в процессе сохранения начали еще одно (но вообще такого быть не должно), то не вызываем до окончания
        emit const_cast<Model*>(this)->sg_finishSave(error_f, info->properties, saved_properties, _persistent_uid);

    _send_message_loop.quit();

    if (error_f.isOk() && databaseOptions().testFlag(DatabaseObjectOption::DataChangeBroadcasting) && !no_action) {
        // сохранение выполнено. необходимо уведомить об этом
        if (isTemporary())
            Core::databaseManager()->entityCreated(entityUid(), {persistentUid()});
        else
            Core::databaseManager()->entityChanged(entityUid(), {persistentUid()});
    }

    const_cast<Model*>(this)->afterSave(info->properties, saved_properties, error_f);

    finishProgress();

#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    QString thread_info;
    if (!Utils::isMainThread())
        thread_info = QStringLiteral(", no main thread");
    Core::logInfo(QStringLiteral("model finish save: %1%2%3")
                      .arg(debugName())
                      .arg(error_f.isError() ? QStringLiteral(" (error: %1)").arg(error_f.fullText()) : QString())
                      .arg(thread_info));
#endif
#endif

    return error_f;
}

zf::Error Model::finishRemoveHelper(const std::shared_ptr<void>& custom_data, const Message& message, const Error& error) const
{
    Q_UNUSED(custom_data)

    Z_CHECK(isRemoving());

    Error error_f;
    if (message.isValid()) {
        Z_CHECK_ERROR(error);
        if (message.messageType() == MessageType::Confirm) {
            // ничего не делаем
        } else if (message.messageType() == MessageType::Error) {
            error_f = ErrorMessage(message).error();
        }

    } else if (error.isError()) {
        error_f = error;
    }

    Z_CHECK(currentCommand() == ModuleCommands::Remove);
    finishCommand();

    _custom_db_operation_data.reset();
    _is_custom_db_operation = false;
    setSyncError(error_f);

    if (!isRemoving()) // если в процессе удаления начали еще одно (вообще такого быть не должно), то не вызываем до окончания
        emit const_cast<Model*>(this)->sg_finishRemove(error_f);

    _send_message_loop.quit();

    if (error_f.isOk())
        setExists(false);

    const_cast<Model*>(this)->afterRemove(error_f);

    finishProgress();

#if DEBUG_BUILD()
#if defined(RNIKULENKOV) || defined(ZAKHAROV)
    QString thread_info;
    if (!Utils::isMainThread())
        thread_info = QStringLiteral(", no main thread");
    Core::logInfo(QStringLiteral("model finish remove: %1%2%3")
                      .arg(debugName())
                      .arg(error_f.isError() ? QStringLiteral(" (error: %1)").arg(error_f.fullText()) : QString())
                      .arg(thread_info));
#endif
#endif

    return error_f;
}

void Model::finishCustomLoadHelper(const Error& error, const AccessRights& direct_rights, const AccessRights& relation_rights)
{
    Error err = error; // чтобы внутри не модифицировалась error и не было коллизии при возврате
    finishLoadHelper(_custom_db_operation_data, Message(), err, direct_rights, relation_rights);
}

void Model::finishCustomSaveHelper(const Error& error, const Uid& persistent_uid, const Error& non_critical_error)
{
    if (error.isError()) {
        Z_CHECK(!persistent_uid.isValid());
        Error err = error + non_critical_error;
        finishSaveHelper(_custom_db_operation_data, Message(), err, Uid(), Error());

    } else {
        if (databaseOptions().testFlag(DatabaseObjectOption::StandardBeforeSaveExtension)) {
            Z_CHECK(!persistent_uid.isValid());

            auto save_data = std::reinterpret_pointer_cast<SaveInfo>(_custom_db_operation_data);
            DataContainer detached_data = *data();
            detached_data.detach();
            postMessageCommand(ModuleCommands::Save, Core::databaseManager(),
                DBCommandWriteEntityMessage(
                    entityUid(), detached_data, save_data->properties, (originalData() != nullptr ? *originalData() : DataContainer()), save_data->parameters),
                _custom_db_operation_data);

            _custom_db_operation_data.reset();
            _is_custom_db_operation = false;

        } else {
            if (!persistentUid().isValid()) {
                Z_CHECK(persistent_uid.isValid());

            } else {
                if (persistent_uid.isValid())
                    Z_CHECK(persistentUid() == persistent_uid); // на всякий случай
            }

            finishSaveHelper(_custom_db_operation_data, Message(), Error(), persistent_uid, non_critical_error);
        }
    }
}

void Model::finishCustomRemoveHelper(const Error& error) const
{
    finishRemoveHelper(_custom_db_operation_data, Message(), error);
}

void Model::saveOriginalData() const
{
    Z_CHECK(isDetached());

    // принудительно задаем rowID чтобы они скопировались в _original_data
    for (const auto& dataset_p : structure()->propertiesByType(PropertyType::Dataset)) {
        forceCalculateRowID(dataset_p);
    }

    // сбрасываем признак изменения
    data()->resetChanged();

    // Сохраняем оригинальные данные до изменения
    _original_data = Z_MAKE_SHARED(DataContainer, structure());
    _original_data->copyFrom(data());
}

void Model::updateRequestedWidgets() const
{
    auto data = _widget_update_requested;
    for (auto& p : data) {
        if (p.isNull())
            continue;

        if (auto w = qobject_cast<QAbstractScrollArea*>(p))
            w->viewport()->update();

        if (auto w = qobject_cast<QTreeView*>(p))
            w->header()->viewport()->update();

        if (auto w = qobject_cast<QTableView*>(p))
            w->horizontalHeader()->viewport()->update();

        p->update();
    }
    _widget_update_requested.clear();
}

void Model::setAccessRights(const AccessRights& direct_rights, const AccessRights& relation_rights) const
{
    if ((_direct_rights != nullptr && *_direct_rights == direct_rights) || (_relation_rights != nullptr && *_relation_rights == relation_rights))
        return;

    _direct_rights = std::make_unique<AccessRights>(direct_rights);
    _relation_rights = std::make_unique<AccessRights>(relation_rights);

    emit const_cast<Model*>(this)->sg_accessRightsChanged(*_direct_rights, *_relation_rights);
}

void Model::clearDatasetRowIDs(const DataProperty& dataset_property, zf::ItemModel* dataset, const QModelIndex& parent) const
{
    int row_count = dataset->rowCount(parent);
    for (int row = 0; row < row_count; row++) {
        data()->clearDatasetRowID(dataset_property, row, parent);
        clearDatasetRowIDs(dataset_property, dataset, dataset->index(row, 0, parent));
    }
}

void Model::emitStartLoad()
{
    if (_is_start_load_emmited)
        return;

    _is_start_load_emmited = true;

    emit sg_startLoad();
}

DataStructurePtr Model::getDataStructureHelper(const Uid& entity_uid)
{
    Error error;
    auto str = Core::getPlugin(entity_uid.entityCode())->dataStructure(entity_uid, error);
    if (error.isError())
        Z_HALT(error);

    return str;
}

void Model::onStartWaitingMessageCommandFeedback(const zf::Command& key)
{
    if (key == ModuleCommands::Save) {
        emit sg_startSave();
    } else if (key == ModuleCommands::Load) {
        emitStartLoad();
    } else if (key == ModuleCommands::Remove) {
        emit sg_startRemove();
    }
}

void Model::onFinishWaitingMessageCommandFeedback(const zf::Command& key)
{
    Q_UNUSED(key)
}

bool Model::LoadInfo::contains(const Model::LoadInfo* i) const
{
    return (i->options == options && properties.contains(i->properties) && Utils::contains(parameters, i->parameters));
}

bool Model::LoadInfo::equal(const Model::LoadInfo* i) const
{
    return contains(i) && i->contains(this);
}

bool Model::SaveInfo::contains(const Model::SaveInfo* i) const
{
    return (properties.contains(i->properties) && Utils::contains(parameters, i->parameters));
}

bool Model::SaveInfo::equal(const Model::SaveInfo* i) const
{
    return contains(i) && i->contains(this);
}

bool Model::RemoveInfo::contains(const Model::RemoveInfo* i) const
{
    return (Utils::contains(parameters, i->parameters));
}

bool Model::RemoveInfo::equal(const Model::RemoveInfo* i) const
{
    return contains(i) && i->contains(this);
}

} // namespace zf
