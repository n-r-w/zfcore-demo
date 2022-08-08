#pragma once

#include "zf_message.h"
#include "zf_data_container.h"
#include "zf_property_table.h"
#include "zf_connection_information.h"
#include "zf_database_credentials.h"
#include "zf_catalog_info.h"
#include "zf_data_table.h"
#include "zf_data_restriction.h"
#include "zf_db_srv.h"
#include "zf_property_restriction.h"

/* Сообщения для взаимодействия с сервером БД
 * Если сообщение содержит Command, то это команда для сервера
 * Если сообщение содержит Event, то это информация от сервера
 * В качестве ответа может придти указанное в описании класса сообщение или ErrorMessage
 */

namespace zf {
/*! Произвольный запрос к менеджеру БД (MessageType::DBCommandQuery)
Ответ: DBEventQueryFeedbackMessage */
class ZCORESHARED_EXPORT DBCommandQueryMessage : public Message {
public:
    DBCommandQueryMessage();
    DBCommandQueryMessage(const Message& m);    
    explicit DBCommandQueryMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код запроса
        const MessageCode& message_code,
        //! Данные
        const DataContainerList& data);
    explicit DBCommandQueryMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код запроса
        const MessageCode& message_code,
        //! Данные
        const DataContainer& data);

    //! Идентификатор базы данных, к которому относится сообщение
    DatabaseID databaseID() const;
    //! Данные
    DataContainerList data() const;

    Message* clone() const override;

private:
    static Message create(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код запроса
        const MessageCode& message_code,
        //! Данные
        const DataContainerList& data);
};

/*! Получить информацию о подключении к БД (MessageType::DBCommandGetConnectionInformation)
 * Ответ: DBEventConnectionInformationMessage */
class ZCORESHARED_EXPORT DBCommandGetConnectionInformationMessage : public Message
{
public:
    DBCommandGetConnectionInformationMessage();
    DBCommandGetConnectionInformationMessage(const Message& m);    
    explicit DBCommandGetConnectionInformationMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id);

    //! Идентификатор базы данных, к которому относится сообщение
    DatabaseID databaseID() const;

    Message* clone() const override;
};

//! Получить таблицу со свойств сущностей указанного вида из БД (DBCommandGetPropertyTable)
//! Ответ: DBEventPropertyTableMessage
class ZCORESHARED_EXPORT DBCommandGetPropertyTableMessage : public Message {
public:
    DBCommandGetPropertyTableMessage();
    DBCommandGetPropertyTableMessage(const Message& m);    
    //! Запрос конкретных свойств
    explicit DBCommandGetPropertyTableMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код сущности
        const EntityCode& entity_code,
        //! Какие свойства нужны. Если не задано, то все доступные свойства типа PropertyType::Field
        //! Все свойства должны относиться к сущности entity_code
        const DataPropertyList& properties = {},
        //! Ограничения на свойства
        const PropertyRestriction& restriction = {},
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});
    //! Запрос конкретных свойств по их кодам
    explicit DBCommandGetPropertyTableMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код сущности
        const EntityCode& entity_code,
        //! Какие свойства нужны (коды свойств)
        const PropertyIDList& property_codes,
        //! Ограничения на свойства
        const PropertyRestriction& restriction = {},
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});
    //! Запрос свойств по их параметрам
    explicit DBCommandGetPropertyTableMessage(
        //! Идентификатор базы данных
        const DatabaseID& database_id,
        //! Код сущности
        const EntityCode& entity_code,
        //! Свойства с какими параметрами нужны (логическое OR)
        const PropertyOptions& options,
        //! Ограничения на свойства
        const PropertyRestriction& restriction = {},
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});

    //! Идентификатор базы данных, к которому относится сообщение
    DatabaseID databaseID() const;
    //! Код сущности
    EntityCode entityCode() const;
    //! Какие свойства нужны (свойства)
    DataPropertyList properties() const;    
    //! Свойства с какими параметрами нужны (логическое OR)
    PropertyOptions options() const;
    //! Ограничения на свойства
    PropertyRestriction restriction() const;

    //! Произвольные параметры
    QMap<QString, QVariant> parameters() const;

    Message* clone() const override;

private:
    //! Количество кодов идентификаторов
    int count_I_MessageContainsUidCodes() const override;
    //! Код идентификатора
    EntityCode value_I_MessageContainsUidCodes(int index) const override;

    static DataPropertyList codeToProperty(
        //! Код сущности
        const EntityCode& entity_code,
        //! Какие свойства нужны (коды свойств)
        const PropertyIDList& property_codes);
};

/*! Получить идентификаторы сущностей указанного вида из БД (MessageType::DBCommandGetEntityList)
Ответ: DBEventEntityListMessage */
class ZCORESHARED_EXPORT DBCommandGetEntityListMessage : public Message {
public:
    DBCommandGetEntityListMessage();
    DBCommandGetEntityListMessage(const Message& m);
    explicit DBCommandGetEntityListMessage(
        //! Код сущности
        const EntityCode& entity_code,
        //! Идентификатор базы данных
        const DatabaseID& database_id = {},
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});

    //! Идентификатор базы данных, к которому относится сообщение
    DatabaseID databaseID() const;
    //! Код сущности
    EntityCode entityCode() const;

    //! Произвольные параметры
    QMap<QString, QVariant> parameters() const;

    //! Сообщение содержит информацию об идентификаторах указанного типа
    bool contains(const EntityCode& entity_code) const override;

    Message* clone() const override;

private:
    //! Количество кодов идентификаторов
    int count_I_MessageContainsUidCodes() const override;
    //! Код идентификатора
    EntityCode value_I_MessageContainsUidCodes(int index) const override;
};

/*! Существуют ли такие сущности в БД (MessageType::DBCommandIsEntityExists)
Ответ: DBEventEntityExistsMessage */
class ZCORESHARED_EXPORT DBCommandIsEntityExistsMessage : public EntityUidListMessage
{
public:
    DBCommandIsEntityExistsMessage();
    DBCommandIsEntityExistsMessage(const Message& m);
    explicit DBCommandIsEntityExistsMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {});
    explicit DBCommandIsEntityExistsMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid,
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});

    Message* clone() const override;
};

/*! Получить сущности из БД (MessageType::DBCommandGetEntity)
Ответ: DBEventEntityLoadedMessage */
class ZCORESHARED_EXPORT DBCommandGetEntityMessage : public Message
{
public:
    DBCommandGetEntityMessage();
    DBCommandGetEntityMessage(const Message& m);
    explicit DBCommandGetEntityMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Атрибуты, которые надо загрузить. Количество properties должно совпадать с properties или быть равно 0
        //! Каждый DataPropertySet может быть пустым (тогда грузятся все атрибуты) или с указанием списка нужных
        //! атрибутов
        const QList<DataPropertySet>& properties = QList<DataPropertySet>(),
        //! Произвольные параметры. Количество равно entity_uids или пусто
        const QList<QMap<QString, QVariant>>& parameters = {});
    explicit DBCommandGetEntityMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить. Количество properties должнос совпадать с properties или быть равно 0
        //! Каждый DataPropertySet может быть пустым (тогда грузятся все атрибуты) или с указанием списка нужных
        //! атрибутов
        const DataPropertySet& properties = DataPropertySet(),
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});

    //! Идентификаторы
    UidList entityUids() const;
    //! Атрибуты
    QList<DataPropertySet> properties() const;

    //! Произвольные параметры
    QList<QMap<QString, QVariant>> parameters() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Записать сущности в БД (MessageType::DBCommandWriteEntity)
//! Ответ: ConfirmMessage. Содержит в data() QVariantList со следующими элементами:
//!  1) zf::UidList с идентификаторами записанных сущностей. Порядок и количество совпадает с entity_uids
//!  2) QList<DataPropertySet> с фактическим списком записанных свойств (может быть шире чем список свойств сохранения). Порядок и количество совпадает с entity_uids
//!  3) QList<Error> не критические ошибки. Порядок и количество совпадает с entity_uids
class ZCORESHARED_EXPORT DBCommandWriteEntityMessage : public Message
{
public:
    DBCommandWriteEntityMessage();
    DBCommandWriteEntityMessage(const Message& m);
    explicit DBCommandWriteEntityMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Данные, содержащие атрибуты сущностей (количество должно совпадать с entity_uids)
        const DataContainerList& data,
        //! Какие свойства надо сохранять (количество должно совпадать с entity_uids)
        const QList<DataPropertySet>& properties,
        //! Данные, содержащие атрибуты сущностей до изменения (количество должно совпадать с entity_uids или быть пустым)
        const DataContainerList& old_data = {},
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {},
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});

    explicit DBCommandWriteEntityMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid,
        //! Данные, содержащие атрибуты сущностей
        const DataContainer& data,
        //! Какие свойства надо сохранять
        const DataPropertySet& properties,
        //! Данные, содержащие атрибуты сущностей до изменения
        const DataContainer& old_data = {},
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});

    explicit DBCommandWriteEntityMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid,
        //! Данные, содержащие атрибуты сущностей
        const DataContainer& data,
        //! Какие свойства надо сохранять
        const DataPropertySet& properties,
        //! Данные, содержащие атрибуты сущностей до изменения
        const DataContainer& old_data,
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters,
        //! Действие пользователя
        bool by_user);

    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Данные, содержащие атрибуты сущностей (количество должно совпадать с entity_uids)
    DataContainerList data() const;
    //! Данные, содержащие атрибуты сущностей до изменения (количество должно совпадать с entity_uids или быть пустым)
    DataContainerList oldData() const;
    //! Какие свойства надо сохранять (количество должно совпадать с entity_uids)
    QList<DataPropertySet> properties() const;
    //! Действие пользователя. Количество равно entityUids или нулю
    QList<bool> byUser() const;

    //! Произвольные параметры
    QList<QMap<QString, QVariant>> parameters() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;    
};

//! Удалить сущности из БД (MessageType::DBCommandRemoveEntity)
//! Ответ: ConfirmMessage
class ZCORESHARED_EXPORT DBCommandRemoveEntityMessage : public EntityUidListMessage
{
public:
    DBCommandRemoveEntityMessage();
    DBCommandRemoveEntityMessage(const Message& m);
    explicit DBCommandRemoveEntityMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {},
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});
    explicit DBCommandRemoveEntityMessage(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {});
    explicit DBCommandRemoveEntityMessage(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters,
        //! Действие пользователя
        bool by_user);

    //! Действие пользователя. Количество равно entityUids или нулю
    QList<bool> byUser() const;

    Message* clone() const override;
};

/*! Запрос прав доступа (MessageType::DBCommandGetAccessRights)
 * Передается набор идентификаторов, которые должны быть проверены
Ответ: DBEventAccessRightsMessage */
class ZCORESHARED_EXPORT DBCommandGetAccessRightsMessage : public Message
{
public:
    DBCommandGetAccessRightsMessage();
    DBCommandGetAccessRightsMessage(const Message& m);    
    explicit DBCommandGetAccessRightsMessage(
        /*! Идентификаторы сущностей, для которых нужны права доступа */
        const UidList& entity_uids,
        //! Логин. Если не задано, то для текущего
        const QString& login = QString());

    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Логин. Если не задано, то для текущего
    QString login() const;

    Message* clone() const override;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

/*! Получить информацию о структуре каталога (MessageType::DBCommandGetCatalogInfo)
Ответ: DBEventCatalogInfoMessage */
class ZCORESHARED_EXPORT DBCommandGetCatalogInfoMessage : public Message
{
public:
    DBCommandGetCatalogInfoMessage();
    DBCommandGetCatalogInfoMessage(const Message& m);    
    explicit DBCommandGetCatalogInfoMessage(
        /*! Идентификатор каталога */
        const zf::Uid& catalog_uid);

    //! Идентификатор каталога
    Uid catalogUid() const;

    Message* clone() const override;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

/*! Запрос подключения к БД (MessageType::DBCommandLogin)
Ответ: DBEventLoginMessage */
class ZCORESHARED_EXPORT DBCommandLoginMessage : public Message
{
public:
    DBCommandLoginMessage();
    DBCommandLoginMessage(const Message& m);    
    explicit DBCommandLoginMessage(
        //! База данных
        const DatabaseID& database_id,
        //! Авторизация
        const Credentials& credentials);

    //! База данных
    DatabaseID databaseID() const;
    //! Авторизация
    Credentials credentials() const;

    Message* clone() const override;
};

//! Отклик сервера на запрос о структуре каталога (MessageType::DBEventLogin)
//! Ответ на команду DBCommandLoginMessage
class ZCORESHARED_EXPORT DBEventLoginMessage : public Message
{
public:
    DBEventLoginMessage();
    DBEventLoginMessage(const Message& m);    
    explicit DBEventLoginMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Информация о подключении
        const ConnectionInformation& connection_info);

    //! Информация о подключении
    ConnectionInformation connectionInfo() const;

    Message* clone() const override;
};

//! Отклик сервера на запрос о структуре каталога (MessageType::DBEventCatalogInfo)
//! Ответ на команду DBCommandGetCatalogInfoMessage
class ZCORESHARED_EXPORT DBEventCatalogInfoMessage : public Message
{
public:
    DBEventCatalogInfoMessage();
    DBEventCatalogInfoMessage(const Message& m);    
    explicit DBEventCatalogInfoMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Информация
        const CatalogInfo& info);

    CatalogInfo info() const;

    Message* clone() const override;
};

//! Отклик сервера на произвольный запрос (MessageType::DBEventQueryFeedback)
//! Ответ на команду DBCommandQueryMessage
class ZCORESHARED_EXPORT DBEventQueryFeedbackMessage : public Message
{
public:
    DBEventQueryFeedbackMessage();
    DBEventQueryFeedbackMessage(const Message& m);    
    //! Команда выполнена успешно
    explicit DBEventQueryFeedbackMessage(
            //! id сообщения, на который дан ответ
            const MessageID& feedback_message_id,
            //! Данные (структура зависит от конкретной команды)
            const DataContainerList& data);

    //! Данные
    DataContainerList data() const;

    Message* clone() const override;

private:
    //! Команда не выполнена
    static Message create(
        //! id сообщения, на который дан ответ
        const MessageID& message_id,
        //! Данные (структура зависит от конкретной команды)
        const DataContainerList& data);
};

/*! Информация от сервера о подключении к БД
 * (MessageType::DBEventConnectionInformation)
 * Может быть как ответом на команду DBCommandGetConnectionInformationMessage, так и сообщением, рассылаемым в канал
 * CoreChannels::CONNECTION_INFORMATION (тогда feedback_message_id не задан
 * Если information не валидно, то значит подключение было потеряно */
class ZCORESHARED_EXPORT DBEventConnectionInformationMessage : public Message
{
public:
    DBEventConnectionInformationMessage();
    DBEventConnectionInformationMessage(const Message& m);    
    //! Команда выполнена успешно
    explicit DBEventConnectionInformationMessage(
        //! id сообщения, на который дан ответ (может быть не задано)
        const MessageID& feedback_message_id,
        //! Информация о подключении к БД
        const ConnectionInformation& data);
    //! Ошибка подключения
    explicit DBEventConnectionInformationMessage(
        //! id сообщения, на который дан ответ (может быть не задано)
        const MessageID& feedback_message_id,
        //! Информация о подключении к БД
        const Error& error);

    //! Информация о подключении к БД. Если не валидно, то значит подключение было потеряно
    ConnectionInformation information() const;
    //! Ошибка подключения
    Error error() const;

    Message* clone() const override;
};

//! Таблица свойств сущностей указанного вида (MessageType::DBEventPropertyTable)
//! Ответ на команду DBCommandGetPropertyTableMessage
class ZCORESHARED_EXPORT DBEventPropertyTableMessage : public Message
{
public:
    DBEventPropertyTableMessage();
    DBEventPropertyTableMessage(const Message& m);    
    explicit DBEventPropertyTableMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Код сущности
        const EntityCode& entity_code,
        //! Данные
        const PropertyTable& data);

    //! Код сущности
    EntityCode entityCode() const;
    //! Данные
    PropertyTable data() const;

    Message* clone() const override;

private:
    //! Количество кодов идентификаторов
    int count_I_MessageContainsUidCodes() const override;
    //! Код идентификатора
    EntityCode value_I_MessageContainsUidCodes(int index) const override;
};

//! Что-то произошло с сущностью. Базовый класс для соответсвующих сообщений
class ZCORESHARED_EXPORT DBEventEntityRelatedMessage : public Message
{
public:
    DBEventEntityRelatedMessage();
    DBEventEntityRelatedMessage(const Message& m);
    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Действие пользователя. Количество равно entityUids или нулю
    QList<bool> byUser() const;
    //! Дополнительная информация
    QVariant additionalInfo() const;

    //! Перевести данные в текст
    QString dataToText() const;

protected:
    explicit DBEventEntityRelatedMessage(
        //! Тип сообщения
        MessageType type,
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user,
        //! Дополнительная информация
        const QVariant& additional_info,
        //! На какое сообщение идет ответ
        const MessageID& feedback_message_id);

    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Изменилась сущность (MessageType::DBEventEntityChanged)
class ZCORESHARED_EXPORT DBEventEntityChangedMessage : public DBEventEntityRelatedMessage
{
public:
    DBEventEntityChangedMessage();
    DBEventEntityChangedMessage(const Message& m);
    explicit DBEventEntityChangedMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Коды сущностей
        const EntityCodeList& entity_codes,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});

    //! Коды сущностей
    EntityCodeList entityCodes() const;

    //! Перевести данные в текст
    QString dataToText() const;

    Message* clone() const override;

private:    
    //! Количество кодов идентификаторов
    int count_I_MessageContainsUidCodes() const override;
    //! Код идентификатора
    EntityCode value_I_MessageContainsUidCodes(int index) const override;
};

//! Сущность удалена (MessageType::DBEventEntityRemoved)
class ZCORESHARED_EXPORT DBEventEntityRemovedMessage : public DBEventEntityRelatedMessage
{
public:
    DBEventEntityRemovedMessage();
    DBEventEntityRemovedMessage(const Message& m);
    explicit DBEventEntityRemovedMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});
    explicit DBEventEntityRemovedMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid);   

    Message* clone() const override;
};

//! Сущность создана (MessageType::DBEventEntityCreated)
class ZCORESHARED_EXPORT DBEventEntityCreatedMessage : public DBEventEntityRelatedMessage
{
public:
    DBEventEntityCreatedMessage();
    DBEventEntityCreatedMessage(const Message& m);
    explicit DBEventEntityCreatedMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});

    Message* clone() const override;
};

//! Список идентификаторов сущностей указанного вида, полученных из БД (MessageType::DBEventEntityList)
//! Ответ на команду DBCommandGetEntityListMessage
class ZCORESHARED_EXPORT DBEventEntityListMessage : public EntityUidListMessage
{
public:
    DBEventEntityListMessage();
    DBEventEntityListMessage(const Message& m);    
    explicit DBEventEntityListMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей
        const UidList& entity_uids);
    explicit DBEventEntityListMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей
        const Uid& entity_uid);

    Message* clone() const override;
};

//! Загружены сущности (MessageType::DBEventEntityLoaded)
//! Ответ на команду DBCommandGetEntityMessage
class ZCORESHARED_EXPORT DBEventEntityLoadedMessage : public Message
{
public:
    DBEventEntityLoadedMessage();
    DBEventEntityLoadedMessage(const Message& m);
    explicit DBEventEntityLoadedMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Данные, содержащие атрибуты сущностей (количество равно entity_uids)
        const DataContainerList& data,
        //! Прямые права доступа. Количество равно нулю или совпадает с entity_uids
        const AccessRightsList& direct_rights,
        //! Косвенные права доступа (на связаные объекты). Количество равно нулю или совпадает с entity_uids
        const AccessRightsList& relation_rights);

    //! Идентификаторы
    UidList entityUids() const;
    //! Данные
    DataContainerList data() const;

    //! Прямые права доступа по порядку идентификаторов (количество совпадает с entityUids)
    AccessRightsList directRights() const;
    //! Прямые права доступа в виде QMap
    UidAccessRights directRightsMap() const;

    //! Косвенные права доступа по порядку идентификаторов (количество совпадает с entityUids)
    AccessRightsList relationRights() const;
    //! Косвенные права доступа в виде QMap
    UidAccessRights relationRightsMap() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Информация о том, существуют ли такие сущности (MessageType::DBEventEntityExists)
//! Ответ на команду DBCommandIsEntityExistsMessage
class ZCORESHARED_EXPORT DBEventEntityExistsMessage : public Message
{
public:
    DBEventEntityExistsMessage();
    DBEventEntityExistsMessage(const Message& m);    
    explicit DBEventEntityExistsMessage(
            //! id сообщения, на который дан ответ
            const MessageID& feedback_message_id,
            //! Идентификаторы сущностей
            const UidList& entity_uids,
            //! Существуют ли сущности (количество равно entity_uids)
            const QList<bool>& is_exists);

    //! Идентификаторы
    UidList entityUids() const;
    //! Существует ли
    QList<bool> isExists() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Информация запрос о правах доступа к сущностям (MessageType::DBEventAccessRights)
//! Ответ на команду DBCommandGetAccessRightsMessage
class ZCORESHARED_EXPORT DBEventAccessRightsMessage : public Message
{
public:
    DBEventAccessRightsMessage();
    DBEventAccessRightsMessage(const Message& m);    
    explicit DBEventAccessRightsMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей (количество uids, direct_rights, relation_rights должно совпадать)
        const UidList& uids,
        //! Прямые права доступа
        const AccessRightsList& direct_rights,
        //! Косвенные права доступа (на связаные объекты)
        const AccessRightsList& relation_rights);

    //! Идентификаторы (количество совпадает с accessRights)
    UidList entityUids() const;

    //! Прямые права доступа по порядку идентификаторов (количество совпадает с entityUids)
    AccessRightsList directRights() const;
    //! Прямые права доступа в виде QMap
    UidAccessRights directRightsMap() const;

    //! Косвенные права доступа по порядку идентификаторов (количество совпадает с entityUids)
    AccessRightsList relationRights() const;
    //! Косвенные права доступа в виде QMap
    UidAccessRights relationRightsMap() const;

    Message* clone() const override;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Некое информационное текстовое сообщение от сервера (MessageType::DBEventInformation)
//! Автоматически рассылается в канал CoreChannels::SERVER_INFORMATION
class ZCORESHARED_EXPORT DBEventInformationMessage : public Message
{
public:
    DBEventInformationMessage();
    DBEventInformationMessage(const Message& m);
    explicit DBEventInformationMessage(
        //! Произвольный мап
        const QMap<QString, QVariant>& messages);

    //! Произвольный мап
    QMap<QString, QVariant> messages() const;

    Message* clone() const override;
};

//! Драйвер БД информирует что соединение с сервером выполнено (MessageType::DBEventConnectionDone)
//! Автоматически рассылается в канал CoreChannels::SERVER_INFORMATION
class ZCORESHARED_EXPORT DBEventConnectionDoneMessage : public Message {
public:
    DBEventConnectionDoneMessage();
    DBEventConnectionDoneMessage(const Message& m);
    explicit DBEventConnectionDoneMessage(
        //! Произвольный мап
        const QMap<QString, QVariant>& data);

    //! Произвольный мап
    QMap<QString, QVariant> data() const;

    Message* clone() const override;
};

//! Драйвер БД информирует что закончена начальная инициализация (подгрузка справочников и т.п.) (MessageType::DBEventInitLoadDone)
//! Автоматически рассылается в канал CoreChannels::SERVER_INFORMATION
class ZCORESHARED_EXPORT DBEventInitLoadDoneMessage : public Message {
public:
    DBEventInitLoadDoneMessage();
    DBEventInitLoadDoneMessage(const Message& m);
    explicit DBEventInitLoadDoneMessage(
        //! Произвольный мап
        const QMap<QString, QVariant>& data);

    //! Произвольный мап
    QMap<QString, QVariant> data() const;

    Message* clone() const override;
};

//! Ответ от сервера с подтверждением выполнения. Для команд, не требующих дополнительной информации
//! (MessageType::Confirm) Ответ на: DBCommandRemoveEntityMessage, DBCommandWriteEntityMessage
class ZCORESHARED_EXPORT ConfirmMessage : public Message
{
public:
    ConfirmMessage();
    ConfirmMessage(const Message& m);    
    explicit ConfirmMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Произвольные данные (зависят от сообщения на который дается ответ)
        const QVariant& data = {},
        //! Истина, если по факту никаких действий выполнено не было (например не надо ничего сохранять т.к. данные не поменялись)
        bool no_action = false);

    //! Произвольные данные (зависят от сообщения на который дается ответ)
    QVariant data() const;

    //! Метод предназначен для упрощения доступа к информации о созданных идентификаторах при ответе на DBCommandWriteEntityMessage
    //! Если это не ответ на DBCommandWriteEntityMessage, то будет ошибка
    UidList uidList() const;
    //! Метод предназначен для упрощения доступа к информации о фактическом списке записанных свойств при ответе на DBCommandWriteEntityMessage
    //! Порядок совпадает с uidList
    //! Если это не ответ на DBCommandWriteEntityMessage, то будет ошибка
    QList<DataPropertySet> writedProperties() const;

    //! Истина, если по факту никаких действий выполнено не было (например не надо ничего сохранять т.к. данные не поменялись)
    bool noAction() const;

    Message* clone() const override;
};

//! Запрос генерации отчета (MessageType:DBCommandGenerateReport)
class ZCORESHARED_EXPORT DBCommandGenerateReportMessage : public Message
{
public:
    DBCommandGenerateReportMessage();

    DBCommandGenerateReportMessage(const Message& m);    
    explicit DBCommandGenerateReportMessage(
        //! id шаблона
        const QString& template_id,
        //! Данные для генерации отчета
        const DataContainer& data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне
        const QMap<DataProperty, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<DataProperty, QLocale::Language>& field_languages = {});

    QString templateId() const;
    //! Данные для генерации отчета
    DataContainer data() const;
    //! Соответствие между полями данных и текстовыми метками в шаблоне
    QMap<DataProperty, QString> fieldNames() const;
    /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
     * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
    bool isAutoMap() const;
    //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
    QLocale::Language language() const;
    //! Язык для конкретных полей данных
    QMap<DataProperty, QLocale::Language> fieldLanguages() const;

    //! Создать копию сообщения
    Message* clone() const override;
};

//! Готовый отчет в ответ на DBCommandGenerateReportMessage (MessageType:DBEventReport)
class ZCORESHARED_EXPORT DBEventReportMessage : public Message
{
public:
    DBEventReportMessage();
    DBEventReportMessage(const Message& m);    
    explicit DBEventReportMessage(
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id,
        //! Содержимое отчета
        QByteArrayPtr data);

    //! Содержимое отчета
    QByteArrayPtr data() const;

    //! Создать копию сообщения
    Message* clone() const override;
};

//! Запрос выборки из таблицы сервера. Ответ: DBEventDataTableMessage
class ZCORESHARED_EXPORT DBCommandGetDataTableMessage : public Message
{
public:
    DBCommandGetDataTableMessage();
    DBCommandGetDataTableMessage(const Message& m);

    explicit DBCommandGetDataTableMessage(
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});
    explicit DBCommandGetDataTableMessage(
        //! Код БД
        const DatabaseID& database_id,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});

    explicit DBCommandGetDataTableMessage(
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Набор ключей
        const QList<int>& keys,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});
    explicit DBCommandGetDataTableMessage(
        //! Код БД
        const DatabaseID& database_id,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Набор ключей
        const QList<int>& keys,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});

    //! Код БД
    DatabaseID databaseID() const;
    //! Серверный код таблицы
    TableID tableID() const;
    //! Маска. Если не пусто, то только эти колонки
    FieldIdList mask() const;

    //! Ограничения
    DataRestrictionPtr restriction() const;
    //! Код колонки для поиска
    FieldID fieldID() const;
    //! Набор ключей
    QList<int> keys() const;

    //! Создать копию сообщения
    Message* clone() const override;

private:
    DBCommandGetDataTableMessage(
        //! Код БД
        const DatabaseID& database_id,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Набор ключей
        const QList<int>& keys,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask);
};

//! Результат запроса на выборку из таблицы сервера (в ответ на DBCommandGetDataTableMessage)
class ZCORESHARED_EXPORT DBEventDataTableMessage : public Message
{
public:
    DBEventDataTableMessage();
    DBEventDataTableMessage(const Message& m);    
    DBEventDataTableMessage(
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id,
        //! Данные
        DataTablePtr data);

    //! Данные
    DataTablePtr result() const;

    //! Создать копию сообщения
    Message* clone() const override;
};

//! Сообщение драйверу с запросом информации о таблице
class ZCORESHARED_EXPORT DBCommandGetDataTableInfoMessage : public Message
{
public:
    DBCommandGetDataTableInfoMessage();
    DBCommandGetDataTableInfoMessage(const Message& m);    
    explicit DBCommandGetDataTableInfoMessage(
        //! База данных
        const zf::DatabaseID& database_id,
        //! Серверный идентификатор таблицы
        const zf::TableID& table_id);

    //! Серверный идентификатор таблицы
    zf::TableID tableID() const;
    //! База данных
    zf::DatabaseID databaseID() const;

    Message* clone() const override;
};

//! Информация о таблице
struct SrvTableInfo
{
    //! Код таблицы
    TableID id;
    //! Структура данных
    zf::DataStructurePtr structure;
    //! Можно ли редактировать
    bool editable = false;

    struct SrvFieldInfo
    {
        //! Код поля
        zf::FieldID field_id;
        //! Описание
        QString description;
    };
    //! Ключ - колонка
    QMap<DataProperty, SrvFieldInfo> fields;
};
typedef std::shared_ptr<SrvTableInfo> SrvTableInfoPtr;

//! Ответ драйвера на запрос информации о таблице
class ZCORESHARED_EXPORT DBEventTableInfoMessage : public Message
{
public:
    DBEventTableInfoMessage();
    DBEventTableInfoMessage(const Message& m);    
    explicit DBEventTableInfoMessage(const zf::MessageID& feedback_id,
                                     //! Информация о таблице
                                     const SrvTableInfoPtr& info);

    //! Информация о таблице
    SrvTableInfoPtr info() const;

    Message* clone() const override;
};

/*! Массовое обновление данных (MessageType::DBCommandUpdateEntities)
Ответ: ConfirmMessage или ErrorMessage */
class ZCORESHARED_EXPORT DBCommandUpdateEntitiesMessage : public Message
{
public:
    DBCommandUpdateEntitiesMessage();
    DBCommandUpdateEntitiesMessage(const Message& m);
    explicit DBCommandUpdateEntitiesMessage(
        //! Список объектов для обновления
        const UidList& entity_uids,
        //! Для каждого из entity_uids набор свойств и новых значений
        //! Таблицы не поддерживаются
        const QList<QMap<DataProperty, QVariant>>& values,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {});
    explicit DBCommandUpdateEntitiesMessage(
        //! Список объектов для обновления
        const UidList& entity_uids,
        //! Для каждого из entity_uids набор свойств и новых значений
        //! Таблицы не поддерживаются
        const QList<QMap<PropertyID, QVariant>>& values,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {});

    //! Идентификаторы
    UidList entityUids() const;
    //! Значения
    QList<QMap<DataProperty, QVariant>> values() const;
    //! Произвольные параметры
    QList<QMap<QString, QVariant>> parameters() const;

    //! Перевести данные в текст
    QString dataToText() const;

    Message* clone() const override;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;

    static Message create(
        //! Список объектов для обновления
        const UidList& entity_uids,
        //! Для каждого из entity_uids набор свойств и новых значений
        //! Таблицы не поддерживаются
        const QList<QMap<PropertyID, QVariant>>& values,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters);
};

//! Переподключиться к серверу
class ZCORESHARED_EXPORT DBCommandReconnectMessage : public Message
{
public:
    DBCommandReconnectMessage(bool is_valid = false);
    DBCommandReconnectMessage(const Message& m);

    Message* clone() const override;

private:
    Message create(bool is_valid);
};

} // namespace zf

Q_DECLARE_METATYPE(zf::SrvTableInfoPtr)
