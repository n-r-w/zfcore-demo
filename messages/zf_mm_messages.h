#pragma once

#include "zf_message.h"
#include "zf_defs.h"

namespace zf
{
class Model;
typedef std::shared_ptr<Model> ModelPtr;

/* Команды для работы с менеджером моделей
 * Если сообщение содержит Command, то это команда для менеджера моделей
 * Если сообщение содержит Event, то это информация от менеджера моделей
 * В качестве ответа может придти указанное в описании класса сообщение или ErrorMessage
 */

/*! Вернуть список загруженных моделей (MessageType::MMCommandGetModels)
 * Ответ от менеджера: MMEventModelListMessage
 * Отправка этого сообщения аналогична вызову метода ModelManager::loadModelsAsync */
class ZCORESHARED_EXPORT MMCommandGetModelsMessage : public Message
{
public:
    MMCommandGetModelsMessage();
    MMCommandGetModelsMessage(const Message& m);
    explicit MMCommandGetModelsMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать (если не указано, то все)
        const QList<DataPropertySet>& properties_list = {},
        //! Загружат ли все свойства, если properties_list для данного идентификатора пустой
        const QList<bool>& all_if_empty_list = {});
    explicit MMCommandGetModelsMessage(
        //! Идентификаторы сущностей
        const Uid& entity_uid,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Что загружать (если не указано, то все)
        const DataPropertySet& properties = {},
        //! Загружат ли все свойства, если properties пусто
        bool all_if_empty = true);

    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Параметры загрузки
    QList<LoadOptions> loadOptions() const;
    //! Что загружать
    QList<DataPropertySet> properties() const;
    //! Загружат ли все свойства, если properties для данного идентификатора пустой
    QList<bool> allIfEmptyProperties() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

/*! Команда на удаление моделей (MessageType::MMCommandRemoveModels)
 * Ответ: ConfirmMessage
 * Отправка этого сообщения аналогична вызову метода ModelManager::removeModelsAsync
 * В отличие от DBCommandRemoveEntityMessage сначала создается экземпляр модели и затем для него вызывается model->remove(). Это имеет
 * смысл, если необходимо контролировать логику удаления на клиенте. Если это не нужно, то лучше вызывать DBCommandRemoveEntityMessage, т.к.
 * это гораздо быстрее, особенно при удалении нескольких сущностей.
 */
class ZCORESHARED_EXPORT MMCommandRemoveModelsMessage : public Message
{
public:
    MMCommandRemoveModelsMessage();
    MMCommandRemoveModelsMessage(const Message& m);
    explicit MMCommandRemoveModelsMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Свойства моделей (какие свойства инициализировать при создании моделей)
        const QList<DataPropertySet>& properties = {},
        //! Что загружать (если не указано, то все)
        const QList<DataPropertySet>& properties_list = {},
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {},
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user = {});

    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Свойства моделей
    QList<DataPropertySet> properties() const;
    //! Произвольные параметры
    QList<QMap<QString, QVariant>> parameters() const;
    //! Загружат ли все свойства, если properties для данного идентификатора пустой
    QList<bool> allIfEmptyProperties() const;
    //! Действие пользователя. Количество равно entityUids или пусто (не определено)
    QList<bool> byUser() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Список моделей вида std::shared_ptr<Model> (MessageType::MMEventModelList)
//! Ответ на MMCommandGetModelsMessage
class ZCORESHARED_EXPORT MMEventModelListMessage : public ModelListMessage
{
public:
    MMEventModelListMessage();
    MMEventModelListMessage(const Message& m);    
    MMEventModelListMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Модели
        const QList<ModelPtr>& models);

    using ModelListMessage::models;

    Message* clone() const override { return new MMEventModelListMessage(*this); }
};

//! Данные модели стали невалидными. Сообщения, рассылается моделями в канал CoreChannels::MODEL_INVALIDATE
//! (MessageType::ModelInvalide)
class ZCORESHARED_EXPORT ModelInvalideMessage : public EntityUidListMessage
{
public:
    ModelInvalideMessage();
    ModelInvalideMessage(const Message& m);    
    ModelInvalideMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids);

    Message* clone() const override { return new ModelInvalideMessage(*this); }
};

/*! Команда на получение имен сущностей (MessageType::MMCommandGetEntityNames)
 * Ответ: MMEventEntityNamesMessage или ErrorMessage (если вообще что-то нехорошее) */
class ZCORESHARED_EXPORT MMCommandGetEntityNamesMessage : public Message
{
public:
    MMCommandGetEntityNamesMessage();
    MMCommandGetEntityNamesMessage(const Message& m);    
    explicit MMCommandGetEntityNamesMessage(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Какие свойства отвечают за хранение имени. Количество быть пустым или должно совпадать с entity_uids. Если какое-то свойство не
        //! валидно или список свойств пустой, то ядро будет пытаться получить свойство с именем на основании zf::PropertyOption:FullName, а
        //! затем zf::PropertyOption:Name
        const DataPropertyList& name_properties = {},
        //! Язык. Если QLocale::AnyLanguage, то используется язык интерфейса
        QLocale::Language language = QLocale::AnyLanguage);

    //! Идентификаторы сущностей
    UidList entityUids() const;
    //! Свойства моделей
    DataPropertyList nameProperties() const;
    //! Язык
    QLocale::Language language() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

//! Имена сущностей (MessageType::MMCommandGetEntityNames)
//! Ответ на MMCommandGetEntityNamesMessage
class ZCORESHARED_EXPORT MMEventEntityNamesMessage : public Message
{
public:
    MMEventEntityNamesMessage();
    MMEventEntityNamesMessage(const Message& m);    
    MMEventEntityNamesMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Имена. Количество всегда равно количеству entityUids в MMCommandGetEntityNamesMessage
        //! Если при получении была ошибка, то она содержится в errors
        const QStringList& names,
        //! Ошибки. Количество всегда равно количеству entityUids в MMCommandGetEntityNamesMessage или пусто
        const ErrorList& errors = ErrorList());

    //! Идентификаторы сущностей. Порядок может не совпадать с MMCommandGetEntityNamesMessage
    UidList entityUids() const;
    //! Имена. Количество всегда равно количеству entityUids
    //! Если при получении была ошибка, то она содержится в errors
    QStringList names() const;
    //! Ошибки. Количество всегда равно количеству entityUids в MMCommandGetEntityNamesMessage
    ErrorList errors() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;

private:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;
};

/*! Команда на получение значения из каталога (MessageType::MMCommandGetCatalogValue)
 * Ответ: MMCommandGetEntityNamesMessage или ErrorMessage */
class ZCORESHARED_EXPORT MMCommandGetCatalogValueMessage : public Message
{
public:
    MMCommandGetCatalogValueMessage();
    MMCommandGetCatalogValueMessage(const Message& m);    
    explicit MMCommandGetCatalogValueMessage(
        //! Код сущности каталога
        const EntityCode& catalog_id,
        //! Код строки каталога
        const EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Язык
        QLocale::Language language = QLocale::AnyLanguage,
        //! База данных
        const DatabaseID& database_id = {});

    //! Код сущности каталога
    EntityCode catalogId() const;
    //! Код строки каталога
    EntityID id() const;
    //! Идентификатор свойства каталога для которого надо получить значение
    DataProperty property() const;
    //! База данных
    DatabaseID databaseId() const;
    //! Язык
    QLocale::Language language() const;

    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Значение из каталога (MessageType::MMEventCatalogValue)
//! Ответ на MMCommandGetCatalogValueMessage
class ZCORESHARED_EXPORT MMEventCatalogValueMessage : public Message
{
public:
    MMEventCatalogValueMessage();
    MMEventCatalogValueMessage(const Message& m);    
    MMEventCatalogValueMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Значение
        const QVariant& value);

    //! Значение
    QVariant value() const;

    Message* clone() const override;
};

} // namespace zf
