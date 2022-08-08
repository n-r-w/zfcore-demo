#include "zf_database_manager.h"
#include "zf_core.h"
#include "zf_database_messages.h"
#include "zf_framework.h"
#include "zf_model_manager.h"
#include "zf_translation.h"
#include "zf_database_driver_worker.h"
#include "zf_database_driver.h"

#include <QPluginLoader>
#include <QApplication>

#define DB_DRIVER_USE_THREAD true // запускать драйвер в отдельном потоке

namespace zf
{
class DriverThread : public QThread
{
public:
    DriverThread() { setTerminationEnabled(true); }
};

//! Соответствие между командами и ответами сервера в случае успеха
QMap<MessageType, MessageType> DatabaseManager::_command_feedback_mapping = {
    {MessageType::DBCommandQuery, MessageType::DBEventQueryFeedback},
    {MessageType::DBCommandGetPropertyTable, MessageType::DBEventPropertyTable},
    {MessageType::DBCommandIsEntityExists, MessageType::DBEventEntityExists},
    {MessageType::DBCommandGetEntity, MessageType::DBEventEntityLoaded},
    {MessageType::DBCommandWriteEntity, MessageType::Confirm},
    {MessageType::DBCommandRemoveEntity, MessageType::Confirm},
    {MessageType::DBCommandGetEntityList, MessageType::DBEventEntityList},
    {MessageType::DBCommandGetConnectionInformation, MessageType::DBEventConnectionInformation},
    {MessageType::DBCommandGetAccessRights, MessageType::DBEventAccessRights},
    {MessageType::DBCommandGenerateReport, MessageType::DBEventReport},
    {MessageType::DBCommandGetCatalogInfo, MessageType::DBEventCatalogInfo},
    {MessageType::DBCommandGetDataTable, MessageType::DBEventDataTable},
    {MessageType::DBCommandGetDataTableInfo, MessageType::DBEventDataTableInfo},
    {MessageType::DBCommandUpdateEntities, MessageType::Confirm},
    {MessageType::MMCommandGetEntityNames, MessageType::MMEventEntityNames},
};

DatabaseManager::DatabaseManager(int terminate_timeout_ms)
    : QObject()
    , _terminate_timeout_ms(terminate_timeout_ms)
    , _object_extension(new ObjectExtension(this))
{
}

DatabaseManager::~DatabaseManager()
{
    freeResources();
    delete _object_extension;
}

bool DatabaseManager::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void DatabaseManager::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant DatabaseManager::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool DatabaseManager::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void DatabaseManager::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void DatabaseManager::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void DatabaseManager::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void DatabaseManager::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void DatabaseManager::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

DatabaseID DatabaseManager::defaultDatabase() const
{
    return Core::defaultDatabase();
}

DatabaseID DatabaseManager::entityCodeDatabase(const Uid& owner_uid, const EntityCode& entity_code) const
{
    Z_CHECK_NULL(_driver);
    return _driver->entityCodeDatabase(owner_uid, entity_code);
}

void DatabaseManager::bootstrap()
{
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::ENTITY_CHANGED));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::ENTITY_REMOVED));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::ENTITY_CREATED));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::SERVER_INFORMATION));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::CONNECTION_INFORMATION));

    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::DATABASE_MANAGER, this, "sl_message_dispatcher_inbound",
                                                      "sl_message_dispatcher_inbound_advanced"));
}

Error DatabaseManager::registerDatabaseDriver(const DatabaseDriverConfig& config, const QString& library_name, const QString& path)
{
    Z_CHECK_X(_driver_worker == nullptr, "driver already registered");

    Error error;

    QString s_path = QApplication::applicationDirPath() + "/" + path;
    QString lib = QDir(QDir::fromNativeSeparators(s_path)).absoluteFilePath(Utils::libraryName(library_name));
    if (!QFile::exists(lib))
        return Error::moduleError(ZF_TR(ZFT_DATABASE_DRIVER_NOT_FOUND).arg(library_name));

    QPluginLoader loader(lib);
    if (!loader.load()) {
        return Error::databaseDriverError(loader.errorString());
    }

    _driver = dynamic_cast<DatabaseDriver*>(loader.instance());
    if (_driver == nullptr) {
        loader.unload();
        return Error::databaseDriverError(ZF_TR(ZFT_DATABASE_DRIVER_LOAD_ERROR).arg(library_name));
    }
    error = _driver->bootstrap(config);
    if (error.isError())
        return error;

    _driver_worker = _driver->createWorker(error);
    if (error.isError()) {
        loader.unload();
        return error;
    }
    Z_CHECK_NULL(_driver_worker);

    Core::writeToLogStorage("database driver loaded: " + library_name, InformationType::Information);

#if DB_DRIVER_USE_THREAD
    _driver_thread = new DriverThread;
    _driver_worker->moveToThread(_driver_thread);
//    connect(_driver_thread, &QThread::finished, _driver_worker, [this]() { _driver_worker->deleteLater(); });
#endif
    connect(_driver_worker, &DatabaseDriverWorker::sg_feedback, this, &DatabaseManager::sl_feedback);

#if DB_DRIVER_USE_THREAD
    _driver_thread->start();
#endif

    return Error();
}

void DatabaseManager::shutdown()
{
    freeResources();
}

void DatabaseManager::setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials)
{
    Z_CHECK_X(_driver_worker != nullptr, "driver not registered");
    _driver_worker->setDatabaseCredentials(database_id, credentials);
}

Error DatabaseManager::databaseInitialized()
{
    Z_CHECK_X(_driver_worker != nullptr, "driver not registered");
    Z_CHECK(!_database_initialized);

    auto codes = Core::fr()->getAllModules();
    Error error;
    for (auto& c : qAsConst(codes)) {
        auto plugin = Core::getPlugin(c);
        Z_CHECK_NULL(plugin);
        Error err = plugin->loadDataFromDatabase();
        if (err.isError())
            error << Error(QString("Module %1 error: %2").arg(c.value()).arg(err.fullText()));
    }

    _database_initialized = true;
    return error;
}

DatabaseDriverWorker* DatabaseManager::worker() const
{
    Z_CHECK_NULL(_driver_worker);
    return _driver_worker;
}

const QMap<MessageType, MessageType>& DatabaseManager::commandFeedbackMapping()
{
    return _command_feedback_mapping;
}

Message DatabaseManager::fastAnswer(const I_ObjectExtension* sender, const Message& message)
{
    Q_UNUSED(sender)
    if (message.messageType() == MessageType::DBCommandGetAccessRights) {
        auto msg = DBCommandGetAccessRightsMessage(message);

        AccessRightsList direct_rights;
        AccessRightsList relation_rights;
        if (requestCachedAccessRights(msg.entityUids(), direct_rights, relation_rights, msg.login()))
            return DBEventAccessRightsMessage(message.messageId(), msg.entityUids(), direct_rights, relation_rights);

        return Message();
    }

    return Message();
}

QList<bool> DatabaseManager::isEntityExistsSync(const UidList& entity_uid, int timeout_ms, Error& error) const
{
    error.clear();

    Message feedback = Core::messageDispatcher()->sendMessage(
        CoreUids::DATABASE_MANAGER, DBCommandIsEntityExistsMessage(entity_uid), timeout_ms);

    if (feedback.messageType() == MessageType::Error) {
        error = ErrorMessage(feedback).error();
        return QList<bool>();
    }

    return DBEventEntityExistsMessage(feedback).isExists();
}

UidList DatabaseManager::getEntityUidsSync(const DatabaseID& database_id, const EntityCode& entity_code, Error& error) const
{
    zf::Message feedback = zf::Core::messageDispatcher()->sendMessage(CoreUids::DATABASE_MANAGER,
                                                                      zf::DBCommandGetEntityListMessage(entity_code, database_id));
    Z_CHECK(feedback.isValid());

    if (feedback.messageType() == zf::MessageType::Error) {
        error = ErrorMessage(feedback).error();
        return UidList();
    }

    error.clear();
    return DBEventEntityListMessage(feedback).entityUidList();
}

void DatabaseManager::entityInvalidated(const Uid& sender, const UidList& entity_uids)
{
    if (!entity_uids.isEmpty()) {
        Core::messageDispatcher()->postMessageToChannel(CoreChannels::MODEL_INVALIDATE, sender, ModelInvalideMessage(entity_uids));
        //        processUpdateLink(entity_uids);
    }
}

void DatabaseManager::entityInvalidated(const Uid& sender, const Uid& entity_uid)
{
    entityInvalidated(sender, UidList {entity_uid});
}

void DatabaseManager::entityChanged(const Uid& sender, const UidList& entity_uids, const QList<bool>& by_user)
{
    if (!entity_uids.isEmpty()) {
        Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_CHANGED, sender,
                                                        DBEventEntityChangedMessage(entity_uids, {}, by_user));
        processUpdateLink(entity_uids, {});
    }
}

void DatabaseManager::entityChanged(const Uid& sender, const Uid& entity_uid)
{
    entityChanged(sender, UidList {entity_uid});
}

void DatabaseManager::entityChanged(const Uid& sender, const Uid& entity_uid, bool by_user)
{
    entityChanged(sender, UidList {entity_uid}, {by_user});
}

void DatabaseManager::entityChanged(const Uid& sender, const EntityCode& entity_code)
{
    Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_CHANGED, sender, DBEventEntityChangedMessage({}, {entity_code}));
    processUpdateLink({}, {entity_code});
}

void DatabaseManager::entityRemoved(const Uid& sender, const UidList& entity_uids, const QList<bool>& by_user)
{
    if (!entity_uids.isEmpty()) {
        Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_REMOVED, sender,
                                                        DBEventEntityRemovedMessage(entity_uids, by_user));
        processUpdateLink(entity_uids, {});
    }
}

void DatabaseManager::entityRemoved(const Uid& sender, const Uid& entity_uid)
{
    entityRemoved(sender, UidList {entity_uid});
}

void DatabaseManager::entityRemoved(const Uid& sender, const Uid& entity_uid, bool by_user)
{
    entityRemoved(sender, UidList {entity_uid}, {by_user});
}

void DatabaseManager::entityCreated(const Uid& sender, const UidList& entity_uids, const QList<bool>& by_user)
{
    if (!entity_uids.isEmpty()) {
        Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_CREATED, sender,
                                                        DBEventEntityCreatedMessage(entity_uids, by_user));
        processUpdateLink(entity_uids, {});
    }
}

void DatabaseManager::entityCreated(const Uid& sender, const Uid& entity_uid)
{
    entityCreated(sender, UidList {entity_uid});
}

void DatabaseManager::entityCreated(const Uid& sender, const Uid& entity_uid, bool by_user)
{
    entityCreated(sender, UidList {entity_uid}, {by_user});
}

void DatabaseManager::blockModelsReload()
{
    Core::messageDispatcher()->blockChannelMessageType(CoreChannels::ENTITY_CHANGED, MessageType::DBEventEntityChanged);
    Core::messageDispatcher()->blockChannelMessageType(CoreChannels::ENTITY_REMOVED, MessageType::DBEventEntityRemoved);
    Core::messageDispatcher()->blockChannelMessageType(CoreChannels::ENTITY_CREATED, MessageType::DBEventEntityCreated);
}

void DatabaseManager::unBlockModelsReload()
{
    Core::messageDispatcher()->unBlockChannelMessageType(CoreChannels::ENTITY_CHANGED, MessageType::DBEventEntityChanged);
    Core::messageDispatcher()->unBlockChannelMessageType(CoreChannels::ENTITY_REMOVED, MessageType::DBEventEntityRemoved);
    int counter = Core::messageDispatcher()->unBlockChannelMessageType(CoreChannels::ENTITY_CREATED, MessageType::DBEventEntityCreated);

    if (counter == 0) {
        for (auto& m : Core::fr()->openedModels()) {
            m->invalidate();
        }
    }
}

void DatabaseManager::processUpdateLink(const UidList& entity, const EntityCodeList& codes)
{
    QMutexLocker locked(&_links_mutex);
    QSet<EntityCode> links_code;
    QSet<Uid> links_uids;

    QList<UpdateLinkInfo> links;
    for (auto& c : codes) {
        links << _update_links.values(UpdateLinkInfo(c));
    }

    for (auto& u : entity) {
        links << _update_links.values(UpdateLinkInfo(u.entityCode()));
        links << _update_links.values(UpdateLinkInfo(u));
    }

    for (auto& l : qAsConst(links)) {
        if (l.entity.isValid()) {
            if (links_code.contains(l.entity.entityCode()) || links_uids.contains(l.entity))
                continue;

            links_uids << l.entity;
        } else {
            if (links_code.contains(l.entity_code))
                continue;

            links_code << l.entity_code;

            // удаляем идентификаторы, т.к. будет обновление всех моделей такого типа
            QSet<Uid> links_uids_copy = links_uids;
            for (auto& u : qAsConst(links_uids)) {
                if (u.entityCode() == l.entity_code)
                    links_uids_copy.remove(u);
            }
            links_uids = links_uids_copy;
        }
    }

    for (auto& c : qAsConst(links_code)) {
        if (_links_code_process_check.contains(c)) {
            Core::logError(QString("registerUpdateLink - circular link detected. Entity code: %1").arg(c.value()));
            continue;
        }
        _links_code_process_check << c;
        entityChanged(CoreUids::DATABASE_MANAGER, c);
        _links_code_process_check.remove(c);
    }

    for (auto& u : qAsConst(links_uids)) {
        if (_links_uids_process_check.contains(u)) {
            Core::logError(QString("registerUpdateLink - circular link detected. Entity: %1").arg(u.toPrintable()));
            continue;
        }
        _links_uids_process_check << u;
        entityChanged(CoreUids::DATABASE_MANAGER, u);
        _links_uids_process_check.remove(u);
    }
}

void DatabaseManager::registerUpdateLink(const Uid& source_entity, const Uid& target_entity)
{
    QMutexLocker locked(&_links_mutex);
    Z_CHECK(source_entity.isValid() && target_entity.isValid());

    UpdateLinkInfo source(source_entity);
    UpdateLinkInfo dest(target_entity);

    if (_update_links.contains(source, dest))
        return;

    _update_links.insert(source, dest);
}

void DatabaseManager::registerUpdateLink(const EntityCode& source_entity_code, const Uid& target_entity)
{
    QMutexLocker locked(&_links_mutex);
    Z_CHECK(source_entity_code.isValid() && target_entity.isValid());

    UpdateLinkInfo source(source_entity_code);
    UpdateLinkInfo dest(target_entity);

    if (_update_links.contains(source, dest))
        return;

    _update_links.insert(source, dest);
}

void DatabaseManager::registerUpdateLink(const EntityCode& source_entity_code, const EntityCode& target_entity_code)
{
    QMutexLocker locked(&_links_mutex);
    Z_CHECK(source_entity_code.isValid() && target_entity_code.isValid());

    UpdateLinkInfo source(source_entity_code);
    UpdateLinkInfo dest(target_entity_code);

    if (_update_links.contains(source, dest))
        return;

    _update_links.insert(source, dest);
}

void DatabaseManager::registerUpdateLink(const QList<EntityCode>& source_entity_codes, const EntityCode& target_entity_code)
{
    for (auto& e : source_entity_codes) {
        registerUpdateLink(e, target_entity_code);
    }
}

const ConnectionInformation* DatabaseManager::connectionInformation() const
{
    return &_connection_information;
}

bool DatabaseManager::requestCachedAccessRights(const UidList& entity_uids, AccessRightsList& direct_access_rights,
                                                AccessRightsList& relation_access_rights, const QString& login) const
{
    direct_access_rights.clear();
    relation_access_rights.clear();

    AccessRightsList d_rights;
    AccessRightsList r_rights;
    for (auto& uid : entity_uids) {
        // возможно информация была получена при подключении и не надо вообще проверять кэш
        auto direct_it = _connection_information.directPermissions().entityRights().find(uid);
        if (direct_it != _connection_information.directPermissions().entityRights().end()) {
            auto relation_it = _connection_information.relationPermissions().entityRights().find(uid);
            if (relation_it != _connection_information.relationPermissions().entityRights().end()) {
                d_rights << direct_it.value();
                r_rights << relation_it.value();
                continue;
            }
        }

        // ищем к кэше
        auto ar = accessRightsCache(login)->object(uid);
        if (ar == nullptr)
            return false;

        d_rights << ar->direct;
        r_rights << ar->relation;
    }

    direct_access_rights = d_rights;
    relation_access_rights = r_rights;
    return true;
}

void DatabaseManager::sl_feedback(const QList<Message>& feedback)
{
    if (_destroyed)
        return;

    for (auto& m : feedback) {
        Z_CHECK(m.isValid());

        MessageID feedback_id = m.feedbackMessageId();

        if (m.messageType() == MessageType::DBEventEntityLoaded) {
        } else if (m.messageType() == MessageType::DBEventEntityExists) {
        } else if (m.messageType() == MessageType::DBEventQueryFeedback) {
        } else if (m.messageType() == MessageType::DBEventPropertyTable) {
        } else if (m.messageType() == MessageType::Confirm) {
        } else if (m.messageType() == MessageType::DBEventEntityList) {
        } else if (m.messageType() == MessageType::DBEventReport) {
        } else if (m.messageType() == MessageType::DBEventCatalogInfo) {
        } else if (m.messageType() == MessageType::DBEventDataTable) {
        } else if (m.messageType() == MessageType::DBEventDataTableInfo) {
        } else if (m.messageType() == MessageType::IntTable) {
        } else if (m.messageType() == MessageType::Progress) {
        } else if (m.messageType() == MessageType::Error) {
            if (!feedback_id.isValid())
                Core::postError(ErrorMessage(m).error());

        } else if (m.messageType() == MessageType::DBEventEntityChanged) {
            Z_CHECK(Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_CHANGED, CoreUids::DATABASE_MANAGER, m));
        } else if (m.messageType() == MessageType::DBEventEntityRemoved) {
            Z_CHECK(Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_REMOVED, CoreUids::DATABASE_MANAGER, m));
        } else if (m.messageType() == MessageType::DBEventEntityCreated) {
            Z_CHECK(Core::messageDispatcher()->postMessageToChannel(CoreChannels::ENTITY_CREATED, CoreUids::DATABASE_MANAGER, m));
        } else if (m.messageType() == MessageType::DBEventInformation || m.messageType() == MessageType::DBEventConnectionDone || m.messageType() == MessageType::DBEventInitLoadDone) {
            Z_CHECK(Core::messageDispatcher()->postMessageToChannel(CoreChannels::SERVER_INFORMATION, CoreUids::DATABASE_MANAGER, m));
        } else if (m.messageType() == MessageType::DBEventConnectionInformation) {
            if (!feedback_id.isValid()) {
                // это рассылка о смене подключения
                auto msg = DBEventConnectionInformationMessage(m);
                _connection_information = msg.information();

                if (_connection_information.isValid())
                    Core::logInfo(QString("connected: %1, %2")
                                      .arg(_connection_information.host(), _connection_information.userInformation().login()));
                else
                    Core::logError("Connection error: " + msg.error().fullText());

                // рассылаем массовое уведомление
                if (msg.error().isOk())
                    msg = DBEventConnectionInformationMessage({}, msg.information());
                else
                    msg = DBEventConnectionInformationMessage({}, msg.error());

                Z_CHECK(
                    Core::messageDispatcher()->postMessageToChannel(CoreChannels::CONNECTION_INFORMATION, CoreUids::DATABASE_MANAGER, msg));
            }
        } else if (m.messageType() == MessageType::DBEventAccessRights) {
            // запоминаем в кэше
            DBEventAccessRightsMessage msg(m);
            auto uids = msg.entityUids();
            auto direct_rights = msg.directRights();
            auto relation_rights = msg.relationRights();
            Z_CHECK(uids.count() == direct_rights.count());
            Z_CHECK(uids.count() == relation_rights.count());
            QString login = DBCommandGetAccessRightsMessage(_waiting_feedback_info.value(msg.feedbackMessageId()).second).login();

            for (int i = 0; i < uids.count(); i++) {
                accessRightsCache(login)->insert(uids.at(i), new _AccessRights {direct_rights.at(i), relation_rights.at(i)});
            }
        } else {
            //            Core::logError(QString("DatabaseManager::sl_feedback. Invalid driver message. Type: %1")
            //                               .arg(Utils::messageTypeName(m.messageType(), false)));
        }

        if (feedback_id.isValid()) {
            //            qDebug() << "DatabaseManager message from driver:" << m.toPrintable() << "feeback id:" <<
            //            feedback_id;
            sendFeedback(feedback_id, m);
        } else {
            //            qDebug() << "DatabaseManager message from driver:" << m.toPrintable() << "no feeback id";
        }
    }
}

void DatabaseManager::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
}

void DatabaseManager::sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const Uid& sender_uid,
                                                             const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(subscribe_handle)

    if (_destroyed)
        return;

    Z_CHECK(sender_ptr != nullptr);
    Z_CHECK(message.isValid());
    Z_CHECK(Utils::isMainThread());

    if (_driver_worker == nullptr) {
        Core::messageDispatcher()->postMessage(this, sender_ptr, ErrorMessage(message.messageId(), Error("database driver not installed")));
        return;
    }

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (message.messageType() != MessageType::General // сообщения общего вида всегда пересылаем
        && message.messageType() != MessageType::DBCommandReconnect
        && commandFeedbackMapping().value(message.messageType(), MessageType::Invalid) == MessageType::Invalid) {
        Core::logError(
            QString("DatabaseManager::sl_message_dispatcher_inbound. Invalid inbound message. Sender %1, type: %2").arg(sender_uid.toPrintable()).arg(Utils::messageTypeName(message.messageType())));
    }

    // проверяем можно ли ответить без запроса к драйверу
    Message fast_answer = fastAnswer(sender_ptr, message);
    if (fast_answer.isValid()) {
        Core::messageDispatcher()->postMessage(this, sender_ptr, fast_answer);
        return;
    }

    // просто пересылаем все сообщения драйверу и ждем ответ
    // запоминаем сообщение, на которое надо ждать ответ от сервера
    _waiting_feedback_info[message.messageId()] = QPair<const I_ObjectExtension*, Message> {sender_ptr, message};

    Core::messageDispatcher()->waitForExternalEventsStart();

    Z_CHECK(QMetaObject::invokeMethod(_driver_worker, "sl_executeCommands", Qt::AutoConnection,
                                      Q_ARG(QList<zf::Message>, QList<zf::Message> {message}),
                                      Q_ARG(QList<zf::Uid>, QList<zf::Uid> {sender_uid})));

    //    qDebug() << "DatabaseManager message from" << sender.toPrintable() << message.toPrintable()
    //             << "forwarded to driver";
}

void DatabaseManager::freeResources()
{
    _destroyed = true;

    if (_driver_worker != nullptr)
        _driver_worker->shutdown();

    if (_driver != nullptr)
        _driver->shutdown();

    if (_driver_thread != nullptr && _driver_thread->isRunning()) {
        _driver_thread->requestInterruption();
        _driver_thread->quit();

        if (_terminate_timeout_ms > 0) {
            if (!_driver_thread->wait(_terminate_timeout_ms)) {
                Core::logError("Driver terminated by timeout");
                _driver_thread->terminate();
                _driver_thread->wait();
            }
        } else
            _driver_thread->wait();

        delete _driver_thread;
        _driver_thread = nullptr;

        if (_driver_worker != nullptr) {
            _driver_worker->objectExtensionDestroy();
            _driver_worker = nullptr;
        }
    }
}

void DatabaseManager::sendFeedback(const MessageID& feedback_id, const Message& message)
{
    // отправляем тому, кто запрашивал
    QPair<const I_ObjectExtension*, Message> sender_info
        = _waiting_feedback_info.value(feedback_id, QPair<const I_ObjectExtension*, Message>(nullptr, Message()));

    if (sender_info.first == nullptr) {
        Core::logError(QString("DatabaseManager::sl_feedback. Unknown sender. Type: %1, ID: %2 %3")
                           .arg(message.toPrintable(), feedback_id.toString())
                           .arg(message.messageType() == MessageType::Error ? ": " + ErrorMessage(message).error().fullText() : QString()));

    } else {
        _waiting_feedback_info.remove(feedback_id);
        Core::messageDispatcher()->waitForExternalEventsFinish();

        if (message.messageType() != MessageType::Error && sender_info.second.messageType() != MessageType::General
            && !commandFeedbackMapping().keys(message.messageType()).contains(sender_info.second.messageType())) {
            // нарушена логика взаимодействия с сервером
            Core::logError(QString("DatabaseManager::sl_feedback. Wrong server feedback. Command type: %1, Feedback type: %2")
                               .arg(Utils::messageTypeName(sender_info.second.messageType()))
                               .arg(Utils::messageTypeName(message.messageType())));
        }

        //        qDebug() << "DatabaseManager forward message" << message.toPrintable() << "to requester"
        //                 << sender_info.first.toPrintable();
        Core::messageDispatcher()->postMessage(this, sender_info.first, message);

        if (message.messageType() == MessageType::Confirm) {
            if (sender_info.second.messageType() == MessageType::DBCommandRemoveEntity) {
                // информируем об удалении сущностей
                auto uids = DBCommandRemoveEntityMessage(sender_info.second).entityUidList();
                for (auto& u : qAsConst(uids)) {
                    entityRemoved(CoreUids::DATABASE_MANAGER, {u});
                }
            }
        }
    }
}

QCache<Uid, DatabaseManager::_AccessRights>* DatabaseManager::accessRightsCache(const QString& login) const
{
    if (_access_rights_cache == nullptr)
        _access_rights_cache = std::make_unique<QCache<QString, QCache<Uid, _AccessRights>>>(_access_rights_cache_size_L1);

    auto ar = _access_rights_cache->object(login);
    if (ar == nullptr) {
        ar = new QCache<Uid, _AccessRights>(_access_rights_cache_size_L2);
        Z_CHECK(_access_rights_cache->insert(login, ar));
    }

    return ar;
}

} // namespace zf
