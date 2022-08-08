#pragma once

#include <QObject>
#include <QMutex>

#include "zf_database_messages.h"
#include "zf_object_extension.h"

namespace zf
{
class DatabaseDriver;

//! Базовый класс для обработчика команд драйвера БД
class ZCORESHARED_EXPORT DatabaseDriverWorker : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    DatabaseDriverWorker(DatabaseDriver* driver);
    ~DatabaseDriverWorker();

    //! Завершение работы
    void shutdown();
    //! В процессе завершения работы
    bool isShutdowning() const;

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;

    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Драйвер
    DatabaseDriver* driver() const;
    //! Задать параметры соединения с БД
    void setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials);

    //! Результаты логина к БД - информация
    ConnectionInformation loginInfo() const;

    void deleteLater();

protected:
    //! Параметры соединения с БД
    const Credentials& getCredentials(const DatabaseID& database_id) const;

    //! Отправить ответ
    void sendFeedback(const QList<Message>& feedback);
    //! Отправить ответ
    void sendFeedback(const Message& feedback);

    //! Проверка подключения к БД и подключение при необходимости
    Error checkLogin(const DatabaseID& database_id,
                     //! ID сообщения на которое надо дать ответ об изменении подключения
                     const MessageID& feedback_id = MessageID(),
                     //! Переподключиться даже при наличии успешного подключения
                     bool force_relogin = false,
                     //! Ответное сообщение об изменении подключения
                     Message* feedback_message = nullptr);

    //! Задать результаты логина к БД
    void setLoginInfo(const ConnectionInformation& i);

protected:       

    //! Обработка команды
    virtual Message executeCommand(const zf::Uid& sender_uid, const Message& m);

    //! Асинхронный запрос шаблона отчета
    virtual Message execute_DBCommandGenerateReport(const zf::Uid& sender_uid, const DBCommandGenerateReportMessage& msg);
    //! Проверка на существование сущностей в БД
    virtual Message execute_DBCommandIsEntityExists(const zf::Uid& sender_uid, const DBCommandIsEntityExistsMessage& msg);
    //! Запрос на получение сущностей
    virtual Message execute_DBCommandGetEntity(const zf::Uid& sender_uid, const DBCommandGetEntityMessage& msg);
    //! Запись сущностей в БД
    virtual Message execute_DBCommandWriteEntity(const zf::Uid& sender_uid, const DBCommandWriteEntityMessage& msg);
    //! Массосове изменение данных
    virtual Message execute_DBCommandUpdateEntities(const zf::Uid& sender_uid, const DBCommandUpdateEntitiesMessage& msg);
    //! Удаление сущностей из БД
    virtual Message execute_DBCommandRemoveEntity(const zf::Uid& sender_uid, const DBCommandRemoveEntityMessage& msg);
    //! Получение списка ключей сущности указанного типа
    virtual Message execute_DBCommandGetEntityList(const zf::Uid& sender_uid, const DBCommandGetEntityListMessage& msg);
    //! Получение таблицы свойств
    virtual Message execute_DBCommandGetPropertyTable(const zf::Uid& sender_uid, const DBCommandGetPropertyTableMessage& msg);
    //! Запрос прав доступа
    virtual Message execute_DBCommandGetAccessRights(const zf::Uid& sender_uid, const DBCommandGetAccessRightsMessage& msg);
    //! Получение информации о справочниках
    virtual Message execute_DBCommandGetCatalogInfo(const zf::Uid& sender_uid, const DBCommandGetCatalogInfoMessage& msg);

    //! Закрыть текущее соединение с сервером БД
    virtual void closeConnection() = 0;
    //! Подключиться к БД
    virtual Error doLogin(
        //! База данных
        const DatabaseID& database_id,
        //! Логин и пароль
        const Credentials& credentials,
        //! Информация о подключении
        ConnectionInformation& connection_info)
        = 0;

    //! Вызывается при запросе отложенного удаления
    virtual void onDelete();

    //! Входящие сообщения
    virtual void onMessageDispatcherInbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

signals:
    //! Получены сообщения от сервера БД
    void sg_feedback(const QList<zf::Message>& feedback);

private slots:
    //! Выполнить команды на сервере БД
    void sl_executeCommands(
        //! Для каждой команды задается набор информационных пакетов (структура определяется видом команды)
        const QList<zf::Message>& commands,
        //! Для каждой команды информация об отправителе. Количество совпадает с commands
        const QList<zf::Uid>& sender_uids);
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    //! Обработка команды на подключение к БД
    Message execute_DBCommandGetConnectionInformation(const zf::Uid& sender_uid, const DBCommandGetConnectionInformationMessage& msg);

    //! Выполнить подключение к серверу БД
    Error loginHelper(const DatabaseID& database_id,
                      //! ID сообщения на которое надо дать ответ об изменении подключения
                      const MessageID& feedback_id,
                      //! Ответное сообщение об изменении подключения
                      Message* feedback_message);

    //! Результаты логина к БД - ошибка
    const Error& loginError() const;
    //! Задать результаты логина к БД в процессе обработки doLogin
    void setLoginError(const Error& error);

    //! Драйвер
    DatabaseDriver* _driver;
    mutable QMutex _mutex;

    //! Данные по авторизации
    QHash<DatabaseID, Credentials> _credentials;
    Credentials _empty_credentials;

    //! Результаты логина к БД - информация
    ConnectionInformation _login_info;
    //! Результаты логина к БД - ошибка
    Error _login_error;

    bool _deleting = false;
    QAtomicInt _shutdowning = 0;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};


} // namespace zf
