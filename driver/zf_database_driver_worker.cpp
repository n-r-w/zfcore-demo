#include "zf_database_driver_worker.h"
#include "zf_core.h"

namespace zf
{
DatabaseDriverWorker::DatabaseDriverWorker(DatabaseDriver* driver)
    : _driver(driver)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_driver);

    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::DRIVER_WORKER, this, "sl_message_dispatcher_inbound"));
}

DatabaseDriverWorker::~DatabaseDriverWorker()
{
    delete _object_extension;
}

void DatabaseDriverWorker::shutdown()
{
    _shutdowning = 1;
    closeConnection();
}

bool DatabaseDriverWorker::isShutdowning() const
{
    return _shutdowning == 1;
}

bool DatabaseDriverWorker::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void DatabaseDriverWorker::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant DatabaseDriverWorker::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool DatabaseDriverWorker::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void DatabaseDriverWorker::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void DatabaseDriverWorker::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void DatabaseDriverWorker::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void DatabaseDriverWorker::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void DatabaseDriverWorker::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

DatabaseDriver* DatabaseDriverWorker::driver() const
{
    return _driver;
}

void DatabaseDriverWorker::setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials)
{
    QMutexLocker locker(&_mutex);
    _credentials[database_id] = credentials;
}

ConnectionInformation DatabaseDriverWorker::loginInfo() const
{
    return _login_info;
}

void DatabaseDriverWorker::deleteLater()
{
    _deleting = true;
    QObject::deleteLater();
    onDelete();
}

const Credentials& DatabaseDriverWorker::getCredentials(const DatabaseID& database_id) const
{
    QMutexLocker locker(&_mutex);
    auto it = _credentials.find(database_id);
    return it == _credentials.end() ? _empty_credentials : (it.value());
}

void DatabaseDriverWorker::sendFeedback(const QList<Message>& feedback)
{
    if (_deleting)
        return;

    emit sg_feedback(feedback);
}

void DatabaseDriverWorker::sendFeedback(const Message& feedback)
{
    sendFeedback(QList<Message> {feedback});
}

void DatabaseDriverWorker::setLoginInfo(const ConnectionInformation& i)
{
    _login_info = i;
}

const Error& DatabaseDriverWorker::loginError() const
{
    return _login_error;
}

void DatabaseDriverWorker::setLoginError(const Error& error)
{
    _login_error = error;
}

void DatabaseDriverWorker::sl_executeCommands(const QList<Message>& commands, const QList<zf::Uid>& sender_uids)
{
    if (_deleting)
        return;

    Z_CHECK(commands.count() == sender_uids.count());

    QList<Message> feedback;
    for (int i = 0; i < commands.count(); i++) {
        auto& m = commands.at(i);
        auto& sender_uid = sender_uids.at(i);
        Z_CHECK(m.isValid());
        Z_CHECK(sender_uid.isValid());

        Message fb;

        if (m.messageType() == MessageType::DBCommandGenerateReport)
            fb = execute_DBCommandGenerateReport(sender_uid, m);
        else if (m.messageType() == MessageType::DBCommandGetConnectionInformation)
            fb = execute_DBCommandGetConnectionInformation(sender_uid, m);
        else
            fb = executeCommand(sender_uid, m);

        if (fb.isValid())
            feedback << fb;
    }

    sendFeedback(feedback);
}

void DatabaseDriverWorker::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    onMessageDispatcherInbound(sender, message, subscribe_handle);
}

Message DatabaseDriverWorker::executeCommand(const Uid& sender_uid, const Message& m)
{
    Message feedback;

    if (m.messageType() == MessageType::DBCommandIsEntityExists) {
        feedback = execute_DBCommandIsEntityExists(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandGetEntity) {
        feedback = execute_DBCommandGetEntity(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandWriteEntity) {
        feedback = execute_DBCommandWriteEntity(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandUpdateEntities) {
        feedback = execute_DBCommandUpdateEntities(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandRemoveEntity) {
        feedback = execute_DBCommandRemoveEntity(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandGetEntityList) {
        feedback = execute_DBCommandGetEntityList(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandGetPropertyTable) {
        feedback = execute_DBCommandGetPropertyTable(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandGetAccessRights) {
        feedback = execute_DBCommandGetAccessRights(sender_uid, m);

    } else if (m.messageType() == MessageType::DBCommandGetCatalogInfo) {
        feedback = execute_DBCommandGetCatalogInfo(sender_uid, m);

    } else {
        Z_HALT(QString("Ошибочное сообщение для драйвера БД: %1").arg(m.messageTypeName()));
    }

    return feedback;
}

Message DatabaseDriverWorker::execute_DBCommandGenerateReport(const zf::Uid& sender_uid, const DBCommandGenerateReportMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandIsEntityExists(const zf::Uid& sender_uid, const DBCommandIsEntityExistsMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandGetEntity(const zf::Uid& sender_uid, const DBCommandGetEntityMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandWriteEntity(const zf::Uid& sender_uid, const DBCommandWriteEntityMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandUpdateEntities(const zf::Uid& sender_uid, const DBCommandUpdateEntitiesMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandRemoveEntity(const zf::Uid& sender_uid, const DBCommandRemoveEntityMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandGetEntityList(const zf::Uid& sender_uid, const DBCommandGetEntityListMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandGetPropertyTable(const zf::Uid& sender_uid, const DBCommandGetPropertyTableMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandGetAccessRights(const zf::Uid& sender_uid, const DBCommandGetAccessRightsMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

Message DatabaseDriverWorker::execute_DBCommandGetCatalogInfo(const zf::Uid& sender_uid, const DBCommandGetCatalogInfoMessage& msg)
{
    Q_UNUSED(msg)
    Q_UNUSED(sender_uid)
    return Message();
}

void DatabaseDriverWorker::onDelete()
{
}

void DatabaseDriverWorker::onMessageDispatcherInbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender);
    Q_UNUSED(message);
    Q_UNUSED(subscribe_handle);
}

Error DatabaseDriverWorker::checkLogin(const DatabaseID& database_id, const MessageID& feedback_id, bool force_relogin,
                                       Message* feedback_message)
{
    if (loginError().isError()) {
        // подключение уже было и прошло с ошибкой
        Z_CHECK(!loginInfo().isValid());
        if (force_relogin)
            loginHelper(database_id, feedback_id, feedback_message);

    } else if (!loginInfo().isValid() || force_relogin) {
        // еще не подключались или надо переподключиться
        loginHelper(database_id, feedback_id, feedback_message);
    }

    if (feedback_message != nullptr && !feedback_message->isValid()) {
        Z_CHECK(feedback_id.isValid());
        // надо ответить сообщением, т.к. есть запрос на обратную связь
        *feedback_message = DBEventConnectionInformationMessage(feedback_id, loginInfo());
    }

    return loginError();
}

Error DatabaseDriverWorker::loginHelper(const DatabaseID& database_id, const MessageID& feedback_id, Message* feedback_message)
{
    if (loginInfo().isValid()) {
        Z_CHECK(loginError().isOk());
        // сбрасываем текущее подключение
        closeConnection();
    }

    setLoginInfo(ConnectionInformation());
    setLoginError(Error());

    auto credentials = this->getCredentials(database_id);

    if (!credentials.isValid()) {
        setLoginError(Error("Credentials not initialized"));

    } else {
        // Подключение к БД чтобы получить данные о пользователе, правах доступа и т.д.
        // Сдесь же надо сохранить коннект в том виде, в котором будет идти работа с сервером
        Core::logInfo("start database connection");

        ConnectionInformation cinfo;
        setLoginError(doLogin(database_id, credentials, cinfo));
        if (loginError().isOk()) {
            Z_CHECK(cinfo.isValid());
            setLoginInfo(cinfo);
        }
    }

    if (feedback_id.isValid()) {
        // информируем об изменении подключения в ответ на запрос
        Z_CHECK_NULL(feedback_message);
        if (loginError().isOk())
            *feedback_message = DBEventConnectionInformationMessage(feedback_id, loginInfo());
        else
            *feedback_message = DBEventConnectionInformationMessage(feedback_id, loginError());

        // информируем всех об изменении подключения
        DBEventConnectionInformationMessage broadcast_msg;
        if (loginError().isError())
            broadcast_msg = DBEventConnectionInformationMessage(MessageID(), loginError());
        else
            broadcast_msg = DBEventConnectionInformationMessage(MessageID(), loginInfo());

        sendFeedback(broadcast_msg);
    }

    return loginError();
}

Message DatabaseDriverWorker::execute_DBCommandGetConnectionInformation(const Uid& sender_uid,
                                                                        const DBCommandGetConnectionInformationMessage& msg)
{
    Q_UNUSED(sender_uid)

    Message feedback;
    checkLogin(msg.databaseID(), msg.messageId(), false, &feedback);
    return feedback;
}

} // namespace zf
