#ifndef ZF_DATABASEMANAGER_H
#define ZF_DATABASEMANAGER_H

#include "zf_global.h"
#include "zf_error.h"
#include "zf_uid.h"
#include "zf_message_dispatcher.h"
#include "zf_database_credentials.h"
#include "zf_connection_information.h"
#include "zf_message.h"
#include "zf_object_extension.h"
#include <QThread>

namespace zf
{
class DatabaseDriver;
class DatabaseDriverWorker;
class DatabaseDriverConfig;

//! Обертка вокруг драйвера БД
class ZCORESHARED_EXPORT DatabaseManager : public QObject, public I_ObjectExtension
{
    Q_OBJECT

public:
    DatabaseManager(
        //! Таймаут ожидания завершения работы драйвера
        int terminate_timeout_ms);
    ~DatabaseManager();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    ;
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
    //! База данных по умолчанию (точка входа в систему)
    DatabaseID defaultDatabase() const;
    //! Получить БД, в которой находятся объекты указанного типа
    DatabaseID entityCodeDatabase(
        //! Владелец объекта (идентификатор клиента и т.п.)
        const Uid& owner_uid,
        //! Код сущности
        const EntityCode& entity_code) const;

    //! Синхронно проверить есть ли в БД сущности с указанными идентификаторами
    QList<bool> isEntityExistsSync(const UidList& entity_uid, int timeout_ms, Error& error) const;

    //! Синхронно получить из базы данных полный список идентификаторов сущностей, с указанным кодом
    UidList getEntityUidsSync(const DatabaseID& database_id, const EntityCode& entity_code, Error& error) const;

    //! Разослать уведомление о том что сущность стала невалидной
    void entityInvalidated(const Uid& sender, const UidList& entity_uids);
    void entityInvalidated(const Uid& sender, const Uid& entity_uid);

    //! Разослать уведомление об изменении сущностей
    void entityChanged(const Uid& sender, const UidList& entity_uids,
                       //! Действие пользователя. Количество равно entityUids или нулю
                       const QList<bool>& by_user = {});
    void entityChanged(const Uid& sender, const Uid& entity_uid);
    void entityChanged(const Uid& sender, const Uid& entity_uid,
                       //! Действие пользователя
                       bool by_user);
    //! Разослать уведомление об изменении сущностей по всем загруженным моделям указанного вида
    void entityChanged(const Uid& sender, const EntityCode& entity_code);

    //! Разослать уведомление об удалении сущностей
    void entityRemoved(const Uid& sender, const UidList& entity_uids,
                       //! Действие пользователя. Количество равно entityUids или нулю
                       const QList<bool>& by_user = {});
    void entityRemoved(const Uid& sender, const Uid& entity_uid);
    void entityRemoved(const Uid& sender, const Uid& entity_uid,
                       //! Действие пользователя
                       bool by_user);

    //! Разослать уведомление о создании сущностей
    void entityCreated(const Uid& sender, const UidList& entity_uids,
                       //! Действие пользователя. Количество равно entityUids или нулю
                       const QList<bool>& by_user = {});
    void entityCreated(const Uid& sender, const Uid& entity_uid);
    void entityCreated(const Uid& sender, const Uid& entity_uid,
                       //! Действие пользователя
                       bool by_user);

    //! Блокировка отправки в каналы сообщений, влиящих на автоматическую перезагрузку моделей
    void blockModelsReload();
    //! Разблокировка отправки в каналы сообщений, влиящих на автоматическую перезагрузку моделей
    void unBlockModelsReload();

    //! Зарегистрировать обновление одной сущности при изменении другой
    void registerUpdateLink(const Uid& source_entity, const Uid& target_entity);
    //! Зарегистрировать обновление одной сущности при изменении другой
    void registerUpdateLink(const EntityCode& source_entity_code, const Uid& target_entity);
    //! Зарегистрировать обновление одной сущности при изменении другой
    void registerUpdateLink(const EntityCode& source_entity_code, const EntityCode& target_entity_code);
    //! Зарегистрировать обновление одной сущности при изменении другой
    void registerUpdateLink(const QList<EntityCode>& source_entity_codes, const EntityCode& target_entity_code);

    //! Информация о подключении к БД
    const ConnectionInformation* connectionInformation() const;
    //! Попытка получить кэшированные результаты запроса на права доступа
    bool requestCachedAccessRights(
        //! Список сущностей
        const UidList& entity_uids,
        //! Прямые права
        AccessRightsList& direct_access_rights,
        //! Косвенные права
        AccessRightsList& relation_access_rights,
        //! Логин. Если не задано, то для текущего
        const QString& login = QString()) const;

    //! Инициализация. Вызывать после инициализации ядра
    void bootstrap();

    //! Установить драйвер БД
    Error registerDatabaseDriver(
        //! Конфигурация
        const DatabaseDriverConfig& config,
        //! Название библиотеки драйвера без указания расширения и префикса lib (для Linux)
        const QString& library_name,
        //! Путь относительно основной папки приложения
        const QString& path = QString());

    //! Завершить работу драйвера
    void shutdown();

    //! Задать параметры соединения с БД
    void setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials);

    //! Вызывать после того, как будет готова работа с базой данных (установлен драйвер, логин и пароль)
    Error databaseInitialized();

    //! Драйвер БД - обработчик
    DatabaseDriverWorker* worker() const;

private slots:
    //! Получены сообщения от сервера БД
    void sl_feedback(const QList<zf::Message>& feedback);
    //! Входящие сообщения для DatabaseManager
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    void sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid,
                                                const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    void freeResources();

    //! Соответствие между командами и ответами сервера в случае успеха
    static const QMap<MessageType, MessageType>& commandFeedbackMapping();

    //! Можно ли ответить без запроса к драйверу
    Message fastAnswer(const I_ObjectExtension* sender, const Message& message);

    //! Отправка ответа на асинхронный запрос
    void sendFeedback(const MessageID& feedback_id, const Message& message);

    //! Обновить сущности, зарегистрированные через registerUpdateLink
    void processUpdateLink(const UidList& entity, const EntityCodeList& codes);

    //! обновление одной сущности при изменении другой - информация
    struct UpdateLinkInfo
    {
        UpdateLinkInfo() {}
        UpdateLinkInfo(const EntityCode& c)
            : entity_code(c)
        {
        }
        UpdateLinkInfo(const Uid& e)
            : entity(e)
        {
        }
        EntityCode entity_code;
        Uid entity;

        bool operator==(const UpdateLinkInfo& u) const { return entity_code == u.entity_code && entity == u.entity; }
        bool operator<(const UpdateLinkInfo& u) const
        {
            return entity_code == u.entity_code ? entity < u.entity : entity_code < u.entity_code;
        }
    };
    //! обновление одной сущности при изменении другой
    QMultiHash<UpdateLinkInfo, UpdateLinkInfo> _update_links;
    //! обновление одной сущности при изменении другой - проверка на зацикливание
    QSet<EntityCode> _links_code_process_check;
    //! обновление одной сущности при изменении другой - проверка на зацикливание
    QSet<Uid> _links_uids_process_check;
    mutable QRecursiveMutex _links_mutex;

    //! Драйвер БД
    DatabaseDriver* _driver = nullptr;
    //! Драйвер БД - обработчик
    QPointer<DatabaseDriverWorker> _driver_worker;
    //! БД была инициализщирована
    bool _database_initialized = false;
    //! Удален
    bool _destroyed = false;
    //! Поток, в котором работает драйвер БД
    QThread* _driver_thread = nullptr;

    //! Информация о подключении к БД
    ConnectionInformation _connection_information;

    //! Информация о тех, кто запрашивал ответ от сервера. Ключ - id сообщения, Значение - идентификатор сущности и
    //! сообщение
    QHash<MessageID, QPair<const I_ObjectExtension*, Message>> _waiting_feedback_info;
    //! Соответствие между командами и ответами сервера в случае успеха
    static QMap<MessageType, MessageType> _command_feedback_mapping;

    //! Кэш прав доступа
    struct _AccessRights
    {
        AccessRights direct;
        AccessRights relation;
    };
    //! Ключ верхнего QCache - Credentials::login,
    QCache<Uid, _AccessRights>* accessRightsCache(const QString& login) const;
    mutable std::unique_ptr<QCache<QString, QCache<Uid, _AccessRights>>> _access_rights_cache;
    const int _access_rights_cache_size_L1 = 1000;
    const int _access_rights_cache_size_L2 = 1000;

    //! Таймаут ожидания завершения работы драйвера
    int _terminate_timeout_ms;

    friend uint qHash(const DatabaseManager::UpdateLinkInfo& u);

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

inline uint qHash(const DatabaseManager::UpdateLinkInfo& u)
{
    return ::qHash(QStringLiteral("%1_%2").arg(u.entity_code.value()).arg(u.entity.hashValue()));
}

} // namespace zf

#endif // ZF_DATABASEMANAGER_H
