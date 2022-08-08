#include "zf_model_manager.h"
#include "zf_core.h"
#include "zf_framework.h"
#include "zf_model.h"

#include <QApplication>
#include <QDebug>

namespace zf
{
ModelManager::~ModelManager()
{
    _sync_operation_timeout->stop();
}

QList<ModelPtr> ModelManager::getModelsSyncHelper(const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, const QList<bool>& is_detached_list, int timeout_ms, Error& error)
{
    Z_CHECK_X(!_open_sync_feedback_message_id.isValid(), "recursive sync operation");

    QList<ModelPtr> models;
    error = getModelsAsyncHelper(
        {this}, entity_uid_list, load_options_list, properties_list, all_if_empty_list, is_detached_list, models, _open_sync_feedback_message_id);
    if (error.isError())
        return QList<ModelPtr>();

    if (timeout_ms > 0)
        _sync_operation_timeout->start(timeout_ms);
    _sync_operation_loop.exec();

    if (timeout_ms > 0)
        _sync_operation_timeout->stop();
    _open_sync_feedback_message_id.clear();

    if (!Core::isBootstraped()) {
        // завершение работы приложения
        error = Error::hiddenError("quit");
        return QList<ModelPtr>();
    }

    if (_sync_error.isError()) {
        error = _sync_error;
        _sync_error.clear();
        return QList<ModelPtr>();

    } else {
        return models;
    }
}

ModelPtr ModelManager::createModel(const DatabaseID& database_id, const EntityCode& entity_code, Error& error)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK(entity_code.isValid());
    MessageID feedback_message_id;
    QList<ModelPtr> models;

    error = getModelsAsyncHelper({this}, {Uid::tempEntity(entity_code, database_id)}, QList<LoadOptions> {}, QList<DataPropertySet> {}, QList<bool> {true},
        QList<bool> {false}, models, feedback_message_id);
    return models.isEmpty() ? ModelPtr() : models.first();
}

ModelPtr ModelManager::getModelSync(
    const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties, bool all_if_empty, int timeout_ms, Error& error)
{
    Z_CHECK(entity_uid.isValid() && (entity_uid.type() == UidType::Entity || entity_uid.type() == UidType::UniqueEntity) && !entity_uid.isTemporary());

    QMutexLocker lock(&_mutex);

    auto res = getModelsSyncHelper({entity_uid}, {load_options}, {properties}, {all_if_empty}, QList<bool> {}, timeout_ms, error);
    if (error.isOk())
        return res.first();
    else
        return nullptr;
}

QList<ModelPtr> ModelManager::getModelsSync(const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, int timeout_ms, Error& error)
{
    QMutexLocker lock(&_mutex);
    return getModelsSyncHelper(entity_uid_list, load_options_list, properties_list, all_if_empty_list, {}, timeout_ms, error);
}

Error ModelManager::getModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, QList<ModelPtr>& models, MessageID& feedback_message_id)
{
    QMutexLocker lock(&_mutex);

    feedback_message_id.clear();
    return getModelsAsyncHelper(requester == nullptr ? QList<const I_ObjectExtension*>() : QList<const I_ObjectExtension*>({requester}), entity_uid_list,
        load_options_list, properties_list, all_if_empty_list, {}, models, feedback_message_id);
}

bool ModelManager::loadModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, MessageID& feedback_message_id)
{
    auto msg = MMCommandGetModelsMessage(entity_uid_list, load_options_list, properties_list, all_if_empty_list);
    feedback_message_id = msg.messageId();

    bool res = Core::messageDispatcher()->postMessage(requester, this, msg);
    if (!res)
        feedback_message_id.clear();
    return res;
}

ModelPtr ModelManager::detachModel(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties, bool all_if_empty, Error& error)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK(entity_uid.isValid() && (entity_uid.type() == UidType::Entity || entity_uid.type() == UidType::UniqueEntity) && !entity_uid.isTemporary());

    MessageID feedback_message_id;
    QList<ModelPtr> models;
    error = getModelsAsyncHelper({this}, {entity_uid}, {load_options}, {properties}, {all_if_empty}, {true}, models, feedback_message_id);
    return models.isEmpty() ? ModelPtr() : models.first();
}

bool ModelManager::removeModelsAsync(const I_ObjectExtension* requester, const UidList& entity_uids, const QList<DataPropertySet>& properties,
    const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& all_if_empty_list, const QList<bool>& by_user, MessageID& feedback_message_id)
{
    QMutexLocker lock(&_mutex);
    return removeModelsAsyncHelper(requester, entity_uids, properties, parameters, all_if_empty_list, by_user, feedback_message_id);
}

Error ModelManager::removeModelsSync(const UidList& entity_uid_list, const QList<DataPropertySet>& properties_list,
    const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& all_if_empty_list, const QList<bool>& by_user, int timeout_ms)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK_X(!_remove_sync_feedback_message_id.isValid(), "recursive sync operation");

    Z_CHECK(removeModelsAsync(this, entity_uid_list, properties_list, parameters, all_if_empty_list, by_user, _remove_sync_feedback_message_id));

    if (timeout_ms > 0)
        _sync_operation_timeout->start(timeout_ms);
    _sync_operation_loop.exec();

    if (timeout_ms > 0)
        _sync_operation_timeout->stop();
    _remove_sync_feedback_message_id.clear();

    if (!Core::isBootstraped()) {
        // завершение работы приложения
        return Error::hiddenError("quit");
    }

    Error error;
    if (_sync_error.isError()) {
        error = _sync_error;
        _sync_error.clear();
    }

    return error;
}

QList<ModelPtr> ModelManager::openedModels() const
{
    QMutexLocker lock(&_mutex);

    QList<ModelPtr> res;
    auto objects = SharedObjectManager::activeObjects();
    for (auto& o : qAsConst(objects)) {
        ModelPtr m = std::dynamic_pointer_cast<Model>(o);
        Z_CHECK_NULL(m);
        res << m;
    }
    return res;
}

void ModelManager::invalidate(const UidList& model_uids)
{
    if (model_uids.isEmpty()) {
        QMutexLocker lock(&_mutex);
        auto objects = SharedObjectManager::activeObjects();
        for (auto& o : qAsConst(objects)) {
            ModelPtr m = std::dynamic_pointer_cast<Model>(o);
            Z_CHECK_NULL(m);

            if (model_uids.contains(m->entityUid()))
                Core::databaseManager()->entityChanged(CoreUids::MODEL_MANAGER, m->entityUid());
        }
    } else {
        Core::databaseManager()->entityChanged(CoreUids::MODEL_MANAGER, model_uids);
    }
}

int ModelManager::cacheSize(const EntityCode& entity_code) const
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(entity_code.isValid());
    return SharedObjectManager::cacheSize(entity_code.value());
}

void ModelManager::disableCache(const EntityCode& entity_code)
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(entity_code.isValid());
    SharedObjectManager::disableCache(entity_code.value());
}

QObject* ModelManager::createObject(const Uid& uid, bool is_detached, Error& error) const
{
    Z_CHECK(uid.isValid() && (uid.type() == UidType::Entity || (uid.type() == UidType::UniqueEntity)));
    error.clear();

    I_Plugin* plugin = Core::getPlugin(uid.entityCode(), error);
    if (error.isError())
        return nullptr;

    auto data_structure = plugin->dataStructure(uid, error);
    if (data_structure == nullptr)
        return nullptr;

    Model* model = plugin->createModel(uid, is_detached);
    Z_CHECK_X(model != nullptr, QString("Module %1 - model not supported"));
    model->afterCreated();
    return model;
}

void ModelManager::prepareSharedPointerObject(const std::shared_ptr<QObject>& object) const
{
    SharedObjectManager::prepareSharedPointerObject(object);
    auto model = std::dynamic_pointer_cast<Model>(object);
    Z_CHECK_NULL(model);
    if (model->isKeeped() && Utils::isMainThread())
        Core::modelKeeper()->keepModels({model});
}

int ModelManager::extractType(const Uid& uid) const
{
    return uid.entityCode().value();
}

Uid ModelManager::extractUid(QObject* object) const
{
    auto m = dynamic_cast<Model*>(object);
    Z_CHECK_NULL(m);
    return m->entityUid();
}

bool ModelManager::isDetached(QObject* object) const
{
    auto m = dynamic_cast<Model*>(object);
    Z_CHECK_NULL(m);
    return m->isDetached();
}

bool ModelManager::canCloneData(QObject* object) const
{
    Q_UNUSED(object);
    return true;
}

void ModelManager::cloneData(QObject* source, QObject* destination) const
{
    auto m_source = dynamic_cast<Model*>(source);
    Z_CHECK_NULL(m_source);
    auto m_destination = dynamic_cast<Model*>(destination);
    Z_CHECK_NULL(m_destination);
    m_destination->copyFrom(m_source);
}

void ModelManager::markAsTemporary(QObject* object) const
{
    auto m = dynamic_cast<Model*>(object);
    Z_CHECK_NULL(m);
    // данные во временной модели не рассматриваем как инвалидные, т.к. ее не надо перезагружать из БД
    m->data()->setInvalidateAll(false);
}

bool ModelManager::isShareBetweenThreads(const Uid& uid) const
{
    return zf::Core::getPlugin(uid.entityCode())->isModelSharedBetweenThreads(uid);
}

void ModelManager::sl_callback(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (objectExtensionDestroyed())
        return;

    if (key == Framework::MODEL_MANAGER_SYNC_OPERATION_CALLBACK_KEY) {
        QMutexLocker lock(&_mutex);
        _sync_operation_loop.quit();
    }
}

void ModelManager::sl_inbound_message(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
}

void ModelManager::sl_inbound_message_advanced(
    const zf::I_ObjectExtension* sender_ptr, const Uid& sender_uid, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(subscribe_handle)
    Z_CHECK(message.isValid());

    //    qDebug() << "ModelManager inbound message from" << sender.toPrintable() << ":" << message.toPrintable();

    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    if (!Core::messageDispatcher()->isObjectRegistered(sender_ptr))
        return;

    if (message.messageType() == MessageType::DBEventEntityCreated) {
        process_DBEventEntityCreated(sender_ptr, sender_uid, DBEventEntityCreatedMessage(message));
        return;

    } else if (message.messageType() == MessageType::DBEventEntityChanged) {
        process_DBEventEntityChanged(sender_ptr, sender_uid, DBEventEntityChangedMessage(message));
        return;

    } else if (message.messageType() == MessageType::DBEventEntityRemoved) {
        process_DBEventEntityRemoved(sender_ptr, sender_uid, DBEventEntityRemovedMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMCommandGetModels) {
        process_MMCommandGetModels(sender_ptr, sender_uid, MMCommandGetModelsMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMEventModelList) {
        process_MMEventModelList(sender_ptr, sender_uid, MMEventModelListMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMCommandRemoveModels) {
        process_MMCommandRemoveModels(sender_ptr, sender_uid, MMCommandRemoveModelsMessage(message));
        return;

    } else if (message.messageType() == MessageType::Error) {
        process_ErrorMessage(sender_ptr, sender_uid, ErrorMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMCommandGetEntityNames) {
        process_MMCommandGetEntityNames(sender_ptr, sender_uid, MMCommandGetEntityNamesMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMEventEntityNames) {
        process_MMEventEntityNames(sender_ptr, sender_uid, MMEventEntityNamesMessage(message));
        return;

    } else if (message.messageType() == MessageType::MMCommandGetCatalogValue) {
        process_MMCommandGetCatalogValue(sender_ptr, sender_uid, MMCommandGetCatalogValueMessage(message));
        return;
    }

    Core::logError(QString("ModelManager::sl_inbound_message. Unknown message. Sender: %1, Type: %2, id: %3, feedback id: %4")
                       .arg(sender_uid.toPrintable(), message.toPrintable(), message.messageId().toString(), message.feedbackMessageId().toString()));
}

void ModelManager::sl_modelFinishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    Model* model = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(model);
    disconnect(model, &Model::sg_finishLoad, this, &ModelManager::sl_modelFinishLoad);

    auto l_info_list = _open_models_async_by_model.values(model);
    Z_CHECK(!l_info_list.isEmpty());

    for (auto& load_info : qAsConst(l_info_list)) {
        Z_CHECK(load_info->not_loaded.remove(model));

        if (error.isEntityNotFoundError()) {
            // информируем об удалении объекта
            Core::databaseManager()->entityRemoved(CoreUids::MODEL_MANAGER, {model->entityUid()});
        } else {
            load_info->error << error;
        }

        if (load_info->not_loaded.isEmpty())
            processFinishLoadAsync(load_info);
    }

    _open_models_async_by_model.remove(model);
}

void ModelManager::sl_modelFinishRemove(const Error& error)
{
    QMutexLocker lock(&_mutex);

    Model* model = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(model);
    disconnect(model, &Model::sg_finishRemove, this, &ModelManager::sl_modelFinishRemove);

    auto r_info_list = _remove_models_async_by_model.values(model);
    Z_CHECK(!r_info_list.isEmpty());

    for (auto& r_info : qAsConst(r_info_list)) {
        r_info->not_removed.remove(model);
        r_info->error << error;

        if (r_info->not_removed.isEmpty())
            processFinishRemoveAsync(r_info);
    }

    _remove_models_async_by_model.remove(model);
}

void ModelManager::sl_catalogFinishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    QMutexLocker lock(&_mutex);

    Model* model = qobject_cast<Model*>(sender());
    auto info = _catalog_value_async_by_model.value(model);
    Z_CHECK_NULL(info);

    QObject::disconnect(info->connection);
    _catalog_value_async.remove(info->feedback_message_id);
    _catalog_value_async_by_model.remove(model);

    if (error.isError()) {
        Core::messageDispatcher()->postMessage(this, info->requester, ErrorMessage(info->feedback_message_id, error));

    } else {
        ModelPtr data_not_ready;
        Core::messageDispatcher()->postMessage(this, info->requester,
            MMEventCatalogValueMessage(info->feedback_message_id,
                Core::fr()->catalogValue(info->catalog_id, info->id, info->property_id, info->language, info->database_id, data_not_ready)));
        Z_CHECK(data_not_ready == nullptr);
    }
}

void ModelManager::process_DBEventEntityCreated(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityCreatedMessage& message)
{
    Q_UNUSED(sender_ptr)
    Q_UNUSED(sender_uid)
    Q_UNUSED(message)
}

void ModelManager::process_DBEventEntityChanged(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityChangedMessage& message)
{
    Q_UNUSED(sender_ptr)
    Q_UNUSED(sender_uid)
    clearCache(message.entityUids());

    auto codes = message.entityCodes();
    for (auto& c : qAsConst(codes)) {
        clearCache(c.value());
    }
}

void ModelManager::process_DBEventEntityRemoved(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityRemovedMessage& message)
{
    Q_UNUSED(sender_ptr);
    Q_UNUSED(sender_uid);

    auto uids = message.entityUids();
    for (auto& uid : qAsConst(uids)) {
        removeObject(uid);
    }
}

void ModelManager::process_MMCommandGetModels(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetModelsMessage& message)
{
    Q_UNUSED(sender_uid);

    MessageID feedback_id = message.messageId();
    QList<ModelPtr> models;
    Error error = getModelsAsyncHelper(
        {sender_ptr}, message.entityUids(), message.loadOptions(), message.properties(), message.allIfEmptyProperties(), {}, models, feedback_id);
    if (error.isError())
        Core::messageDispatcher()->postMessage(this, sender_ptr, ErrorMessage(message.messageId(), error));
}

void ModelManager::process_MMEventModelList(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMEventModelListMessage& message)
{
    Q_UNUSED(sender_ptr);
    Q_UNUSED(sender_uid);

    if (message.feedbackMessageId() == _open_sync_feedback_message_id) {
        _sync_operation_loop.quit();

    } else if (_remove_models_async_by_load.contains(message.feedbackMessageId())) {
        auto r_info = _remove_models_async_by_load.value(message.feedbackMessageId());
        r_info->to_remove = MMEventModelListMessage(message).models();
        bool start = false;
        for (auto& m : r_info->to_remove) {
            Model::CommandExecuteResult r_result;
            if (r_info->by_user.isEmpty()) {
                r_result = m->remove();

            } else {
                int index = r_info->uids.indexOf(m->entityUid());
                Z_CHECK(index >= 0);
                Z_CHECK(r_info->by_user.count() > index);

                if (!r_info->parameters.isEmpty()) {
                    Z_CHECK(r_info->parameters.count() > index);
                    m->setRemoveParameters(r_info->parameters.at(index));
                }

                r_result = m->remove(r_info->by_user.at(index));
            }

            if (r_result == Model::CommandExecuteResult::Added) {
                start = true;
                _remove_models_async_by_model.insert(m.get(), r_info);
                r_info->not_removed << m.get();
                connect(m.get(), &Model::sg_finishRemove, this, &ModelManager::sl_modelFinishRemove);
            }
        }

        _remove_models_async_by_load.remove(message.feedbackMessageId());

        if (!start)
            processFinishRemoveAsync(r_info);
    }
}

void ModelManager::process_MMCommandRemoveModels(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandRemoveModelsMessage& message)
{
    Q_UNUSED(sender_uid);

    MessageID feedback_id = message.messageId();
    if (!removeModelsAsync(
            sender_ptr, message.entityUids(), message.properties(), message.parameters(), message.allIfEmptyProperties(), message.byUser(), feedback_id))
        Core::messageDispatcher()->postMessage(this, sender_ptr, ErrorMessage(message.messageId(), Error("unknown error")));
}

void ModelManager::process_MMCommandGetEntityNames(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetEntityNamesMessage& message)
{
    Q_UNUSED(sender_uid);

    // запрос на получение имен сущностей

    // пересылаем сообщение драйверу БД и ждем ответ
    // Ответ будет MMEventEntityNamesMessage
    // Для обработанных имен результат будет в MMEventEntityNamesMessage::names
    // Если обработать не удалось, то в MMEventEntityNamesMessage::errors будет ошибка ErrorType::DataNotFound
    // Любая другая ошибка - реальная

    // клонируем, но с новым id и пересылаем драйверу
    MMCommandGetEntityNamesMessage db_msg(message.entityUids(), message.nameProperties(), message.language());
    Z_CHECK(Core::postDatabaseMessage(this, db_msg));

    auto info = Z_MAKE_SHARED(EntityNamesAsyncInfoDb);
    info->feedback_message_id = message.messageId();
    info->requester = sender_ptr;
    info->uids = message.entityUids();
    info->properties = message.nameProperties();
    info->language = message.language();

    // ждем ответа от драйвера
    _entity_names_async_db[db_msg.messageId()] = info;
}

void ModelManager::process_ErrorMessage(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const ErrorMessage& message)
{
    Q_UNUSED(sender_ptr);

    if (sender_uid == CoreUids::DATABASE_MANAGER) {
        // это может произойти при ответе драйвера БД на MMCommandGetEntityNamesMessage
        // получен ответ от драйвера БД на запрос имен сущностей
        // ищем оригинальное входящее сообщение
        auto orig_info = _entity_names_async_db.value(message.feedbackMessageId());
        Z_CHECK_NULL(orig_info);
        _entity_names_async_db.remove(message.feedbackMessageId());

        Core::messageDispatcher()->postMessage(this, orig_info->requester, ErrorMessage(orig_info->feedback_message_id, ErrorMessage(message).error()));

    } else if (message.messageId() == _open_sync_feedback_message_id) {
        _sync_error = ErrorMessage(message).error();
        _sync_operation_loop.quit();

    } else if (_remove_models_async_by_load.contains(message.feedbackMessageId())) {
        auto r_info = _remove_models_async_by_load.value(message.feedbackMessageId());
        r_info->error = ErrorMessage(message).error();
        processFinishRemoveAsync(r_info);

        if (message.messageId() == _remove_sync_feedback_message_id)
            _sync_operation_loop.quit();
    }
}

void ModelManager::process_MMEventEntityNames(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMEventEntityNamesMessage& message)
{
    Q_UNUSED(sender_ptr)

    // ответ на запрос имени сущностей

    if (sender_uid == CoreUids::DATABASE_MANAGER) {
        // получен ответ от драйвера БД на запрос имен сущностей
        // ищем оригинальное входящее сообщение
        auto orig_info = _entity_names_async_db.value(message.feedbackMessageId());
        Z_CHECK_NULL(orig_info);
        _entity_names_async_db.remove(message.feedbackMessageId());

        // смотрим что прислал драйвер
        UidList db_uids = message.entityUids();
        QStringList db_names = message.names();
        ErrorList db_errors = message.errors();
        Z_CHECK(db_uids.count() == db_names.count());
        Z_CHECK(db_uids.count() == db_errors.count());
        Z_CHECK(db_uids.count() == orig_info->uids.count());

        // смотрим что надо переслать плагинам, т.к драйвер не смог обработать
        UidList uids_to_plugin;
        DataPropertyList properties_to_plugin;
        // обработанные запросы
        UidList uids_ready;
        QStringList names_ready;
        ErrorList errors_ready;
        for (int i = 0; i < db_uids.count(); i++) {
            if (db_errors.at(i).isUnsupportedError()) {
                // драйвер не смог обработать запрос имени по данному идентификатору, надо получать его обычным способом через плагин
                uids_to_plugin << db_uids.at(i);
                if (!orig_info->properties.isEmpty())
                    properties_to_plugin << orig_info->properties.at(i);

            } else {
                uids_ready << db_uids.at(i);
                names_ready << db_names.at(i);
                errors_ready << db_errors.at(i);
            }
        }

        if (uids_ready.count() == db_uids.count()) {
            // все ответы получены, можно сразу отвечать
            Core::messageDispatcher()->postMessage(
                this, orig_info->requester, MMEventEntityNamesMessage(orig_info->feedback_message_id, uids_ready, names_ready, errors_ready));
            return;
        }

        // группируем запрос по плагинам (типам сущностей)
        QMap<EntityCode, std::shared_ptr<UidList>> grouped_uids;
        QMap<EntityCode, DataProperty> grouped_name_property;
        for (int i = 0; i < uids_to_plugin.count(); i++) {
            Uid uid = uids_to_plugin.at(i);

            Z_CHECK(uid.isValid());
            auto list = grouped_uids.value(uid.entityCode());
            if (list == nullptr) {
                list = Z_MAKE_SHARED(UidList);
                grouped_uids[uid.entityCode()] = list;
            }

            DataProperty name_p;
            if (!properties_to_plugin.isEmpty()) {
                name_p = properties_to_plugin.at(i);
                if (name_p.isValid() && !grouped_name_property.contains(uid.entityCode()))
                    grouped_name_property[uid.entityCode()] = name_p;
            }

            list->append(uid);
        }

        auto info = Z_MAKE_SHARED(EntityNamesAsyncInfo);
        info->feedback_message_id = orig_info->feedback_message_id;
        info->requester = orig_info->requester;
        info->uids = uids_to_plugin;
        info->uids_ready = uids_ready;
        info->names_ready = names_ready;
        info->errors_ready = errors_ready;

        Error error;
        for (auto g = grouped_uids.constBegin(); g != grouped_uids.constEnd(); ++g) {
            auto plugin = Core::getPlugin(g.key(), error);
            Z_CHECK_ERROR(error);
            auto fake_message_id = MessageID::generate();
            plugin->getEntityName(this, fake_message_id, *g.value(), grouped_name_property.value(g.key()), orig_info->language);
            info->requested << fake_message_id;
            _entity_names_async_mm[fake_message_id] = info;
        }

    } else {
        auto info = _entity_names_async_mm.value(message.feedbackMessageId());
        Z_CHECK_NULL(info);
        info->collected << message;

        if (info->requested.count() == info->collected.count()) {
            // получили все ответы
            UidList uids;
            QStringList names;
            ErrorList errors;
            for (auto& c : qAsConst(info->collected)) {
                uids << c.entityUids();
                names << c.names();
                errors << c.errors();
            }

            uids << info->uids_ready;
            names << info->names_ready;
            for (int i = 0; i < info->uids_ready.count(); i++) {
                errors << Error();
            }

            _entity_names_async_mm.remove(message.feedbackMessageId());

            // формируем ответ
            Core::messageDispatcher()->postMessage(this, info->requester, MMEventEntityNamesMessage(info->feedback_message_id, uids, names, errors));
        }
    }
}

void ModelManager::process_MMCommandGetCatalogValue(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetCatalogValueMessage& message)
{
    Q_UNUSED(sender_uid);

    // значение каталога
    auto catalog_model = Core::catalogModel(message.catalogId(), message.databaseId());
    catalog_model->load();
    if (!catalog_model->isLoading()) {
        // можем ответить сразу
        ModelPtr data_not_ready;
        QVariant value
            = Core::fr()->catalogValue(message.catalogId(), message.id(), message.property().id(), message.language(), message.databaseId(), data_not_ready);
        Z_CHECK(data_not_ready == nullptr);
        Core::messageDispatcher()->postMessage(this, sender_ptr, MMEventCatalogValueMessage(message.messageId(), value));

    } else {
        // надо ждать окончания загрузки
        auto info = Z_MAKE_SHARED(CatalogValueAsyncInfo);
        info->feedback_message_id = message.messageId();
        info->requester = sender_ptr;
        info->connection = connect(catalog_model.get(), &Model::sg_finishLoad, this, &ModelManager::sl_catalogFinishLoad);
        info->catalog_id = message.catalogId();
        info->id = message.id();
        info->property_id = message.property().id();
        info->language = message.language();
        info->database_id = message.databaseId();
        _catalog_value_async[info->feedback_message_id] = info;
        _catalog_value_async_by_model[catalog_model.get()] = info;
    }
}

QMap<int, int> ModelManager::convertCacheConfig(const EntityCacheConfig& cache_config)
{
    QMap<int, int> res;
    for (auto i = cache_config.constBegin(); i != cache_config.constEnd(); ++i) {
        res[i.key().value()] = i.value();
    }
    return res;
}

void ModelManager::connectToDatabaseManager()
{
    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::MODEL_MANAGER, this, "sl_inbound_message", "sl_inbound_message_advanced"));

    Z_CHECK(Z_VALID_HANDLE(Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_CHANGED, this)));
    Z_CHECK(Z_VALID_HANDLE(Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_REMOVED, this)));
}

Error ModelManager::getModelsAsyncHelper(const QList<const I_ObjectExtension*> requesters, const UidList& entity_uid_list,
    const QList<LoadOptions>& load_options_list, const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list,
    const QList<bool>& is_detached_list, QList<ModelPtr>& models, MessageID& feedback_message_id)
{
    Error error;
    QList<ModelPtr> exists;
    QList<ModelPtr> need_load;
    QSet<Model*> need_load_set;
    MessagePauseHelper mpause;

    if (entity_uid_list.count() > 1 && (entity_uid_list.count() > 100 || Utils::isProcessEventsInterval()))
        mpause.pause();

    for (int i = 0; i < entity_uid_list.count(); i++) {
        if (entity_uid_list.count() > 1)
            Utils::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

        Uid entity_uid = entity_uid_list.at(i);
        Z_CHECK(entity_uid.isEntity());

        LoadOptions load_options = load_options_list.count() > i ? load_options_list.at(i) : LoadOptions();
        DataPropertySet properties = properties_list.count() > i ? properties_list.at(i) : DataPropertySet();
        auto structure = DataStructure::structure(entity_uid);

        bool all_if_empty = all_if_empty_list.count() > i ? all_if_empty_list.at(i) : true;
        bool is_detached = is_detached_list.count() > i ? is_detached_list.at(i) : false;
        bool is_cloned = false;

        if (properties.isEmpty() && all_if_empty)
            properties = structure->propertiesMain();

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
        // тут не должно быть свойств вроде колонок и т.п.
        for (auto& p : qAsConst(properties)) {
            Z_CHECK(p.propertyType() == PropertyType::Dataset || p.propertyType() == PropertyType::Field);
        }
#endif

        bool is_from_cache;
        bool is_new;
        ModelPtr result_model = std::dynamic_pointer_cast<Model>(getObject(entity_uid, is_detached, is_from_cache, is_cloned, is_new, error));
        if (error.isError()) {
            exists.clear();
            models.clear();
            return error;
        }
        Z_CHECK_NULL(result_model);
        if (!is_detached)
            connectToModel(result_model.get());

        DataPropertySet required_props
            = result_model->data()->whichPropertiesInvalidated(properties, true).unite(result_model->data()->whichPropertiesInitialized(properties, false));

        if (!required_props.isEmpty() && !entity_uid.isTemporary()) {
            // надо асинхронно загрузить данные из БД
            result_model->load(load_options, required_props);
            if (result_model->isLoading()) {
                if (!_open_models_async_by_model.contains(result_model.get()))
                    connect(result_model.get(), &Model::sg_finishLoad, this, &ModelManager::sl_modelFinishLoad);
                if (!need_load_set.contains(result_model.get())) {
                    need_load << result_model;
                    need_load_set << result_model.get();
                }

            } else {
                exists << result_model;
            }

        } else
            exists << result_model;

        models << result_model;
    }

    if (!feedback_message_id.isValid())
        feedback_message_id = MessageID::generate();
    auto load_info = Z_MAKE_SHARED(LoadAsyncInfo);
    load_info->requesters = requesters;
    load_info->feedback_message_id = feedback_message_id;

    if (!exists.isEmpty()) {
        load_info->has_data = exists;
    }

    if (!need_load.isEmpty()) {
        load_info->to_load = need_load;

        for (auto& m : load_info->to_load) {
            Z_CHECK(!load_info->not_loaded.contains(m.get()));
            load_info->not_loaded << m.get();
            _open_models_async_by_model.insert(m.get(), load_info);
        }
        _open_models_async[feedback_message_id] = load_info;

    } else {
        // нечего грузить с сервера, все уже загружено
        processFinishLoadAsync(load_info);
    }

    return Error();
}

bool ModelManager::removeModelsAsyncHelper(const I_ObjectExtension* requester, const UidList& entity_uids, const QList<DataPropertySet>& properties,
    //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
    const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& all_if_empty_list, const QList<bool>& by_user, MessageID& feedback_message_id)
{
    Z_CHECK_X(Core::messageDispatcher()->isObjectRegistered(requester), "Object not registered in MessageDispatcher");

    if (!feedback_message_id.isValid())
        feedback_message_id = MessageID::generate();

    MessageID load_feedback_id;
    bool res = loadModels(this, entity_uids, QList<LoadOptions>(), properties, all_if_empty_list, load_feedback_id);
    if (!res)
        return false;

    auto r_info = Z_MAKE_SHARED(RemoveAsyncInfo);
    r_info->feedback_message_id = feedback_message_id;
    r_info->load_feedback_id = load_feedback_id;
    r_info->requester = requester;
    r_info->parameters = parameters;
    r_info->uids = entity_uids;
    r_info->by_user = by_user;
    _remove_models_async[feedback_message_id] = r_info;
    _remove_models_async_by_load[load_feedback_id] = r_info;

    return true;
}

void ModelManager::processFinishLoadAsync(const std::shared_ptr<LoadAsyncInfo>& load_info)
{
    bool is_ok = true;

    if (!load_info->requesters.isEmpty()) {
        Message msg;
        if (load_info->error.isError()) {
            // все загрузилось, но с ошибками
            msg = ErrorMessage(load_info->feedback_message_id, load_info->error);
        } else {
            // все загрузилось без ошибок
            msg = MMEventModelListMessage(load_info->feedback_message_id, load_info->to_load + load_info->has_data);
        }

        // отправляем тому, кто запрашивал
        for (auto r : qAsConst(load_info->requesters)) {
            if (r == nullptr)
                continue;

            is_ok |= Core::messageDispatcher()->postMessage(this, r, msg);
        }
    }

    _open_models_async.remove(load_info->feedback_message_id);

    if (!is_ok)
        _sync_operation_loop.quit();
}

void ModelManager::processFinishRemoveAsync(const std::shared_ptr<ModelManager::RemoveAsyncInfo>& remove_info)
{
    Message msg;
    if (remove_info->error.isError()) {
        // все удалилось, но с ошибками
        msg = ErrorMessage(remove_info->feedback_message_id, remove_info->error);
    } else {
        // все удалилось без ошибок
        msg = ConfirmMessage(remove_info->feedback_message_id);
    }

    // отправляем тому, кто запрашивал
    bool is_ok = Core::messageDispatcher()->postMessage(this, remove_info->requester, msg);

    _remove_models_async.remove(remove_info->feedback_message_id);

    if (!is_ok)
        _sync_operation_loop.quit();

    if (remove_info->feedback_message_id == _remove_sync_feedback_message_id)
        _sync_operation_loop.quit();
}

void ModelManager::connectToModel(Model* model)
{
    Q_UNUSED(model)
}

ModelManager::ModelManager(const EntityCacheConfig& cache_config, int default_cache_size, int history_size)
    : SharedObjectManager(convertCacheConfig(cache_config), default_cache_size, history_size)
    , _sync_operation_timeout(new QTimer(this))
{
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::MODEL_INVALIDATE));

    connectToDatabaseManager();

    _sync_operation_timeout->setSingleShot(true);
    connect(_sync_operation_timeout, &QTimer::timeout, this, [&]() {
        if (!objectExtensionDestroyed())
            Framework::internalCallbackManager()->addRequest(this, Framework::MODEL_MANAGER_SYNC_OPERATION_CALLBACK_KEY);
    });

    Framework::internalCallbackManager()->registerObject(this, "sl_callback");
}

} // namespace zf
