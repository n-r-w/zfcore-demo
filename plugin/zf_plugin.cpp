#include "zf_plugin.h"
#include "zf_translation.h"
#include "zf_core.h"
#include "zf_view.h"
#include "zf_framework.h"
#include <QApplication>
#include <QMessageBox>
#include <QMainWindow>

namespace zf
{
Plugin::Plugin(const ModuleInfo& module_info)
    : QObject()
    , _module_info(module_info)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(_module_info.isValid());
    Z_CHECK(Core::messageDispatcher()->registerObject(module_info.uid(), this, "sl_message_dispatcher_inbound",
                                                      "sl_message_dispatcher_inbound_advanced"));

    connect(&_inbound_msg_buffer_timer, &FeedbackTimer::timeout, this, [this]() { processInboundMessages(); });
}

Plugin::~Plugin()
{
    delete _object_extension;
}

bool Plugin::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void Plugin::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
    _callback_manager->stop();
}

QVariant Plugin::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool Plugin::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void Plugin::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void Plugin::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void Plugin::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void Plugin::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void Plugin::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

Uid Plugin::pluginUid() const
{
    return _module_info.uid();
}

DataStructurePtr Plugin::dataStructure(const Uid& entity_uid, Error& error) const
{
    QMutexLocker lock(&_mutex);

    DataStructurePtr res;
    if (_module_info.isDataStructureInitialized())
        res = _module_info.dataStructure();
    else
        res = getEntityDataStructure(entity_uid, error);

    if (res != nullptr && !_is_releation_update_initialized.contains(entity_uid)) {
        _is_releation_update_initialized << entity_uid;

        /* отключаем, но пусть останется - вдруг будет надо. это слишком накладно и по факту не нужно, т.к. реагировать надо не любые изменения, а на invalidate, да и то
         * только при нажатии на кнопку "обновить"
        // регистрируем обновление связанных моделей
        DataPropertyList properties = res->propertiesByType(PropertyType::Field);
        properties << res->propertiesByType(PropertyType::ColumnFull);

        for (auto& p : qAsConst(properties)) {
            if (p.lookup() && p.lookup()->listEntity().isValid()) {                
                if (Core::isCatalogUid(p.lookup()->listEntity()))
                    continue; // справочники сами это зарегистрируют

                if (entity_uid.isValid())
                    zf::Core::databaseManager()->registerUpdateLink(entity_uid, p.lookup()->listEntity());
                else
                    zf::Core::databaseManager()->registerUpdateLink(entityCode(), p.lookup()->listEntity());
            }
        }*/
    }

    return res;
}

void Plugin::setSharedDataStructure(const DataStructurePtr& structure)
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(_shared_structure == nullptr);
    _shared_structure = structure;
}

DataStructurePtr Plugin::sharedDataStructure() const
{
    return _shared_structure;
}

void Plugin::setEntityDataStructure(const Uid& entity_uid, const DataStructurePtr& structure)
{
    Z_CHECK(entity_uid.isValid());

    QMutexLocker lock(&_mutex);
    Z_CHECK(!_entity_structure.contains(entity_uid));
    _entity_structure[entity_uid] = structure;
}

QHash<Uid, DataStructurePtr> Plugin::createdStructure() const
{
    return _entity_structure;
}

Operation Plugin::operation(const OperationID& operation_id) const
{
    QMutexLocker lock(&_mutex);

    Operation operation;
    if (!_operation_hash.contains(operation_id)) {
        for (auto& op : _module_info.operations()) {
            if (op.id() != operation_id)
                continue;

            operation = op;
            _operation_hash[operation_id] = op;
            break;
        }
    } else {
        operation = _operation_hash.value(operation_id);
    }

    return operation;
}

OperationList Plugin::operations() const
{
    QMutexLocker lock(&_mutex);
    return _module_info.operations();
}

View* Plugin::createView(const ModelPtr& model, const ViewStates& states, const QVariant& data)
{
    Q_UNUSED(model)
    Q_UNUSED(states)
    Q_UNUSED(data)
    return nullptr;
}

Model* Plugin::createModel(const Uid& entity_uid, bool is_detached)
{
    Q_UNUSED(entity_uid)
    Q_UNUSED(is_detached)
    return nullptr;
}

void Plugin::getEntityName(const I_ObjectExtension* requester, const MessageID& feedback_message_id, const UidList& entity_uids,
                           const DataProperty& name_property, QLocale::Language language)
{
    Error error;

    // определяем свойство, содержащее имя
    DataProperty n_p;
    if (!name_property.isValid()) {
        auto ds = dataStructure(Uid(), error);
        Z_CHECK_ERROR(error);

        // есть ли поле с именем
        auto name_options = ds->propertiesByOptions(PropertyType::Field, PropertyOption::FullName);
        if (name_options.isEmpty())
            name_options = ds->propertiesByOptions(PropertyType::Field, PropertyOption::Name);

        if (name_options.isEmpty()) {
            // непонятно откуда брать имя
        } else {
            n_p = name_options.constFirst();
        }

    } else {
        n_p = name_property;
    }
    Z_CHECK(!n_p.isValid() || n_p.entityCode() == _module_info.code());

    registerEntityNamesRequest(requester, feedback_message_id, entity_uids, n_p, language);

    for (auto& u : entity_uids) {
        Z_CHECK(u.entityCode() == _module_info.code());

        if (u.isUniqueEntity()) {
            // это уникальная сущность - имя определяем сразу по имени модуля
            collectEntityNames(feedback_message_id, u, _module_info.name(), Error());
            continue;
        }

        if (!n_p.isValid()) {
            // непонятно откуда брать имя
            collectEntityNames(feedback_message_id, u, _module_info.name() + " (id: " + u.id().string() + ")", Error());
            continue;
        }

        Uid catalog_uid = Core::catalogEntityUid(u.entityCode(), u.database());
        if (catalog_uid.isValid()) {
            // можно быстро получить расшифровку запросом к каталогу
            auto catalog_id = Core::catalogId(catalog_uid);

            auto catalog = Core::catalogModel(catalog_id, u.database());
            catalog->load({}, {});
            if (!catalog->isLoading()) {
                // можем получить имя сразу
                QString name = Core::fr()->catalogItemName(catalog_uid, u.id(), language);
                if (name.isEmpty())
                    collectEntityNames(feedback_message_id, u, u.toPrintable(), Error());
                else
                    collectEntityNames(feedback_message_id, u, name, Error());

            } else {
                registerEntityNamesRequestLoadingModel(feedback_message_id, catalog, u);
            }
            continue;
        }

        auto model = Core::getModel<Model>(u, {}, {n_p}, error);
        if (error.isError()) {
            collectEntityNames(feedback_message_id, u, QString(), error);
            continue;
        }

        model->load({}, {n_p});
        if (model->isLoading()) {
            registerEntityNamesRequestLoadingModel(feedback_message_id, model, u);
        } else {
            // можем получить имя сразу
            collectEntityNames(feedback_message_id, model->entityUid(), model->value(n_p, language).toString(), Error());
        }
    }
}

void Plugin::registerEntityNamesRequest(const I_ObjectExtension* requester, const MessageID& feedback_message_id,
                                        const UidList& entity_uids, const DataProperty& name_property, QLocale::Language language)
{
    Z_CHECK(Core::messageDispatcher()->isObjectRegistered(requester) && feedback_message_id.isValid() && !entity_uids.isEmpty());

    auto info = Z_MAKE_SHARED(EntityNamesAsyncInfo);
    info->feedback_message_id = feedback_message_id;
    info->requester = requester;
    info->language = language;
    info->to_receive = entity_uids;
    info->name_property = name_property;
    info->not_received = Utils::toSet(entity_uids);
    _entity_names_async[feedback_message_id] = info;

    objectExtensionRegisterUseInternal(const_cast<I_ObjectExtension*>(requester));
}

bool Plugin::collectEntityNames(const MessageID& feedback_message_id, const Uid& uid, const QString& name, const Error& error)
{
    auto info = _entity_names_async.value(feedback_message_id);
    Z_CHECK_NULL(info);
    Z_CHECK(info->not_received.remove(uid));
    info->results[uid] = QPair<QString, Error>(name, error);

    if (info->not_received.isEmpty()) {
        // получены все результаты
        UidList uids;
        QStringList names;
        ErrorList errors;
        for (auto& u : info->to_receive) {
            QPair<QString, Error> result = info->results.value(u);
            Z_CHECK(!result.first.isEmpty() || result.second.isError());
            uids << u;
            names << result.first;
            errors << result.second;
        }
        // отписываемся от сигналов окончания загрузки
        for (auto con = info->loading_models.constBegin(); con != info->loading_models.constEnd(); ++con) {
            QObject::disconnect(con.value());
            _entity_names_async_loading_models.remove(con.key().get());
        }

        Core::messageDispatcher()->postMessage(this, info->requester,
                                               MMEventEntityNamesMessage(info->feedback_message_id, uids, names, errors));
        _entity_names_async.remove(feedback_message_id);
        objectExtensionUnregisterUseInternal(const_cast<I_ObjectExtension*>(info->requester));
        return true;

    } else {
        return false;
    }
}

void Plugin::onEntityNamesRequestModelLoaded(const Model* model, const MessageID& feedback_message_id, const Error& error)
{
    Z_CHECK_NULL(model);
    auto info = _entity_names_async.value(feedback_message_id);
    Z_CHECK_NULL(info);

    if (error.isError()) {
        collectEntityNames(feedback_message_id, model->entityUid(), {}, error);
        return;
    }

    Uid uid;
    QString name;
    if (Core::isCatalogUid(model->entityUid())) {
        uid = info->loadind_model_uids.value(model);
        Z_CHECK(uid.isValid());
        name = Core::fr()->catalogItemName(model->entityUid(), uid.id(), info->language);

    } else {
        uid = model->entityUid();
        name = model->value(info->name_property, info->language).toString();
    }

    if (name.isEmpty())
        name = model->entityUid().toPrintable();

    collectEntityNames(feedback_message_id, uid, name, {});
}

void Plugin::onCallback(int key, const QVariant& data)
{
    Q_UNUSED(key);
    Q_UNUSED(data);
}

void Plugin::onShutdown()
{
}

bool Plugin::registerEntityNamesRequestLoadingModel(const MessageID& feedback_message_id, const ModelPtr& model, const Uid& uid)
{
    Z_CHECK_NULL(model);
    Z_CHECK(model->isLoading());
    Z_CHECK(uid.isValid());

    auto info = _entity_names_async.value(feedback_message_id);
    Z_CHECK_NULL(info);
    Z_CHECK(info->name_property.isValid());

    if (info->loading_models.contains(model))
        return false;
    // запоминаем подключение
    info->loading_models[model] = connect(model.get(), &Model::sg_finishLoad, this, &Plugin::sl_entityNamesRequestModelLoaded);
    info->loadind_model_uids[model.get()] = uid;
    _entity_names_async_loading_models[model.get()] = info;
    return true;
}

void Plugin::sl_entityNamesRequestModelLoaded(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    Model* model = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(model);
    auto info = _entity_names_async_loading_models.value(model);
    Z_CHECK_NULL(info);
    onEntityNamesRequestModelLoaded(model, info->feedback_message_id, error);
}

void Plugin::sl_callback(int key, const QVariant& data)
{
    if (objectExtensionDestroyed())
        return;

    onCallback(key, data);
}

void Plugin::afterLoadModules()
{
}

Error Plugin::loadDataFromDatabase()
{
    return {};
}

Error Plugin::onLoadConfiguration()
{
    return {};
}

Error Plugin::onSaveConfiguration() const
{
    return {};
}

CallbackManager* Plugin::callbackManager() const
{
    if (_callback_manager == nullptr) {
        _callback_manager = new CallbackManager(const_cast<Plugin*>(this));
        _callback_manager->registerObject(this, "sl_callback");
    }
    return _callback_manager.get();
}

QMap<QString, QVariant> Plugin::options() const
{
    return Core::fr()->pluginsOptions().value(_module_info.code());
}

QString Plugin::moduleDataLocation() const
{
    QString s = Utils::moduleDataLocation(entityCode());
    return Utils::location(s + QStringLiteral("/data"), s);
}

bool Plugin::isModelSharedBetweenThreads(const Uid& entity_uid) const
{
    Q_UNUSED(entity_uid);
    return false;
}

bool Plugin::confirmOperation(const Operation& operation)
{
    Q_UNUSED(operation)
    return QMessageBox::question(Utils::getMainWindow(), ZF_TR(ZFT_QUESTION),
                   ZF_TR(ZFT_EXECUTE_QUESTION).arg(operation.name()), QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::Yes;
}

Error Plugin::processOperation(const Operation& operation)
{
    Q_UNUSED(operation)
    return Error();
}

zf::Error Plugin::processInboundMessage(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
    return {};
}

Error Plugin::processInboundMessageAdvanced(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const Message& message,
                                            SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender_ptr)
    Q_UNUSED(sender_uid)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
    return {};
}

void Plugin::disableCache()
{
    zf::Core::fr()->modelManager()->disableCache(entityCode());
}

void Plugin::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    if (!Core::isActive(true))
        return;

    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    storeInboundMessage(sender, message, subscribe_handle);
}

void Plugin::sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const Uid& sender_uid, const Message& message,
                                                    SubscribeHandle subscribe_handle)
{
    if (!Core::isActive(true))
        return;

    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    processInboundMessageAdvanced(sender_ptr, sender_uid, message, subscribe_handle);
}

DataStructurePtr Plugin::getEntityDataStructure(const Uid& entity_uid, Error& error) const
{
    DataStructurePtr structure;

    if (!entity_uid.isValid()) {
        if (_shared_structure != nullptr) {
            structure = _shared_structure;

        } else {
            error = Error(QString("Для сущности %1 не задана общая структура данных").arg(_module_info.code().value()));
            return nullptr;
        }

    } else {
        structure = _entity_structure.value(entity_uid);
    }

    if (structure == nullptr) {
        if (entity_uid.isValid() && Core::isCatalogUid(entity_uid))
            error = Error(QString("Для справочника %1 в драйвере не задана структура данных").arg(Core::catalogId(entity_uid).string()));
        else
            error = Error(QString("Для %1 структура не задана как в плагине, так и через setSharedDataStructure/setEntityDataStructure")
                              .arg(entity_uid.isValid() ? entity_uid.toPrintable() : _module_info.code().string()));
    }

    return structure;
}

void Plugin::storeInboundMessage(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    auto i = std::make_shared<InboundMessageInfo>();
    i->sender = sender;
    i->message = message;
    i->subscribe_handle = subscribe_handle;
    _inbound_msg_buffer.enqueue(i);

    if (!_inbound_msg_buffer_processing)
        _inbound_msg_buffer_timer.start();
}

void Plugin::processInboundMessages()
{
    Z_CHECK(!_inbound_msg_buffer_processing);
    _inbound_msg_buffer_processing = true;
    auto buffer = _inbound_msg_buffer;
    _inbound_msg_buffer.clear();
    while (!buffer.isEmpty()) {
        auto i = buffer.dequeue();
        auto error = processInboundMessage(i->sender, i->message, i->subscribe_handle);
        if (error.isError())
            Core::error(error);
    }
    _inbound_msg_buffer_processing = false;
    if (!_inbound_msg_buffer.isEmpty())
        _inbound_msg_buffer_timer.start();
}

const ModuleInfo& Plugin::getModuleInfo() const
{
    return _module_info;
}

EntityCode Plugin::entityCode() const
{
    return _module_info.code();
}

} // namespace zf
