#pragma once

#include <QObject>
#include "zf_shared_object_manager.h"
#include "zf_defs.h"
#include "zf_message.h"
#include "zf_mm_messages.h"
#include "zf_database_messages.h"

namespace zf
{
class Model;
class ModelFactory;

//! Параметры кэширования сущностей. Ключ - тип сущности, значение - размер кэша
typedef QMap<EntityCode, int> EntityCacheConfig;

//! Создание и кэширование моделей
class ZCORESHARED_EXPORT ModelManager : public SharedObjectManager
{
    Q_OBJECT
public:
    ModelManager(
        //! Параметры кэширования. Ключ - тип сущности, значение - размер кэша
        const EntityCacheConfig& cache_config,
        //! Размер кэша по умолчанию
        int default_cache_size,
        //! Размер буфера последних операций
        int history_size);
    ~ModelManager() override;

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * ModelListMessage или ErrorMessage с id=feedback_message_id */
    Error getModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Модели (свойства пока не загружены)
        QList<ModelPtr>& models,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id */
    bool loadModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Создать экземпляр модели с указанным идентификатором и загрузить ее данные из БД
    ModelPtr getModelSync(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties,
        //! Загружать все свойства, если properties_list пустой
        bool all_if_empty, int timeout_ms, Error& error);
    //! Синхронно создать экземпляры моделей с указанными идентификаторами и загрузить их данные из БД
    QList<ModelPtr> getModelsSync(
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Сколько ждать результата. 0 - бесконечно
        int timeout_ms,
        //! Ошибка
        Error& error);

    /*! Асинхронно создать новый экземпляр модели с указанным идентификатором. Модель не загружается из БД, а просто
     * создается в памяти без инициализации свойств */
    ModelPtr createModel(const DatabaseID& database_id, const EntityCode& entity_code, Error& error);
    /*! Асинхронно создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
     * вызывающему */
    ModelPtr detachModel(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties,
        //! Загружать все свойства, если properties_list пустой
        bool all_if_empty, Error& error);

    //! Асинхронно удалить модели из базы данных
    bool removeModelsAsync(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы моделей
        const UidList& entity_uids,
        //! Атрибуты, которые надо загрузить
        //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
        const QList<DataPropertySet>& properties,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Синхронно удалить модели из базы данных
    Error removeModelsSync(const UidList& entity_uid_list,
        //! Атрибуты, которые надо загрузить
        //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
        const QList<DataPropertySet>& properties_list,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms);

    //! Получить список открытых моделей
    QList<ModelPtr> openedModels() const;

    //! Сделать указанные модели невалидными. Если не задано, то все
    void invalidate(const UidList& model_uids = {});

    //! Размер кэша для сущности
    int cacheSize(const EntityCode& entity_code) const;
    //! Запретить кэширование для данного типа сущностей. Вызывать в конструкторе плагина модуля
    void disableCache(const EntityCode& entity_code);

protected:
    //! Создать новый объект
    QObject* createObject(const Uid& uid,
        //! Является ли объект отсоединенным. Такие объекты предназначены для редактирования и не доступны для
        //! получения через менеджер
        bool is_detached, Error& error) const override;
    //! Обработка объекта после создания shared_ptr
    void prepareSharedPointerObject(const std::shared_ptr<QObject>& object) const override;
    //! Извлечь тип из идентификатора
    int extractType(const Uid& uid) const override;
    //! Извлечь идентификатор из объекта
    Uid extractUid(QObject* object) const override;
    //! Является ли объект отсоединенным (например, взятым на редактирование)
    bool isDetached(QObject* object) const override;
    //! Можно ли копировать данные из объекта
    bool canCloneData(QObject* object) const override;
    //! Копирование данные из одного объекта в другой
    void cloneData(QObject* source, QObject* destination) const override;
    //! Пометить объект как временный (если надо)
    void markAsTemporary(QObject* object) const override;
    //! Можно ли разделять объект между потоками
    bool isShareBetweenThreads(const Uid& uid) const override;

private slots:
    //! Обратный вызов
    void sl_callback(int key, const QVariant& data);

    //! Для приема сообщений через messageDispatcher
    void sl_inbound_message(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    void sl_inbound_message_advanced(
        const zf::I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

    //! Завершена загрузка модели
    void sl_modelFinishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);
    //! Завершено удаление модели
    void sl_modelFinishRemove(const zf::Error& error);

    //! Окончание загрузки каталога при MMCommandGetCatalogValue
    void sl_catalogFinishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    // обработчики входящих сообщений
private:
    void process_DBEventEntityCreated(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityCreatedMessage& message);
    void process_DBEventEntityChanged(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityChangedMessage& message);
    void process_DBEventEntityRemoved(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const DBEventEntityRemovedMessage& message);
    void process_MMCommandGetModels(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetModelsMessage& message);
    void process_MMEventModelList(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMEventModelListMessage& message);
    void process_MMCommandRemoveModels(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandRemoveModelsMessage& message);
    void process_MMCommandGetEntityNames(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetEntityNamesMessage& message);
    void process_ErrorMessage(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const ErrorMessage& message);
    void process_MMEventEntityNames(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMEventEntityNamesMessage& message);
    void process_MMCommandGetCatalogValue(const I_ObjectExtension* sender_ptr, const Uid& sender_uid, const MMCommandGetCatalogValueMessage& message);

private:
    static QMap<int, int> convertCacheConfig(
        //! Параметры кэширования. Ключ - тип сущности, значение - размер кэша
        const EntityCacheConfig& cache_config);

    //! Подключение к сигналам DatabaseManager
    void connectToDatabaseManager();

    //! Запросить создание экземпляров моделей - синхронно
    QList<ModelPtr> getModelsSyncHelper(const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
        const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, const QList<bool>& is_detached_list, int timeout_ms, Error& error);
    //! Запросить создание экземпляров моделей - асинхронно
    Error getModelsAsyncHelper(const QList<const I_ObjectExtension*> requesters, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
        const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, const QList<bool>& is_detached_list, QList<ModelPtr>& models,
        MessageID& feedback_message_id);

    //! Асинхронно удалить модели из базы данных
    bool removeModelsAsyncHelper(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы моделей
        const UidList& entity_uids,
        //! Атрибуты, которые надо загрузить
        //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
        const QList<DataPropertySet>& properties,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Подключиться к сигналам модели
    void connectToModel(Model* model);

    struct LoadAsyncInfo
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! Кто запрашивал
        QList<const I_ObjectExtension*> requesters;
        //! Что загружается или уже загрузилось
        QList<ModelPtr> to_load;
        //! Что осталось загрузить
        QSet<Model*> not_loaded;
        //! Уже есть, загружать не надо
        QList<ModelPtr> has_data;
        //! Произошла ошибка
        Error error;
    };
    //! Завершение процесса асинхронной загрузки
    void processFinishLoadAsync(const std::shared_ptr<LoadAsyncInfo>& load_info);
    //! Модели для отправки в качестве ответа на запрос загрузки моделей. Ключ - id сообщения
    QHash<MessageID, std::shared_ptr<LoadAsyncInfo>> _open_models_async;
    QMultiHash<Model*, std::shared_ptr<LoadAsyncInfo>> _open_models_async_by_model;

    struct RemoveAsyncInfo
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! feedback id ответа на загрузку моделей
        MessageID load_feedback_id;
        //! Кто запрашивал
        const I_ObjectExtension* requester;
        //! Идентификаторы
        QList<Uid> uids;
        //! Дополнительные параметры
        QList<QMap<QString, QVariant>> parameters;
        //! Удаление инициировал пользователь
        QList<bool> by_user;
        //! Что надо удалить или уже удалилось
        QList<ModelPtr> to_remove;
        //! Что осталось удалить
        QSet<Model*> not_removed;
        //! Произошла ошибка
        Error error;
    };
    //! Завершение процесса асинхронного удаления
    void processFinishRemoveAsync(const std::shared_ptr<RemoveAsyncInfo>& remove_info);

    //! Информация о запросе на удаление моделей. Ключ - feedback id ответного сообщения
    QHash<MessageID, std::shared_ptr<RemoveAsyncInfo>> _remove_models_async;
    //! Информация о запросе на удаление моделей. Ключ - feedback id ответа на загрузку моделей
    QHash<MessageID, std::shared_ptr<RemoveAsyncInfo>> _remove_models_async_by_load;
    QMultiHash<Model*, std::shared_ptr<RemoveAsyncInfo>> _remove_models_async_by_model;

    //! Информация о запросе на асинхронное получение имен моделей (плагины)
    struct EntityNamesAsyncInfo
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! Кто запрашивал
        const I_ObjectExtension* requester;
        //! Идентификаторы для которых нужны имена
        UidList uids;
        //! Информация о запросе на асинхронное получение имен моделей: Все запросы
        QSet<MessageID> requested;
        //! Информация о запросе на асинхронное получение имен моделей: Накопленные ответы
        QList<MMEventEntityNamesMessage> collected;

        //! Идентификаторы для которых имена получены из драйвера БД
        UidList uids_ready;
        //! Информация о запросе на асинхронное получение имен моделей: ответы драйвера БД
        QStringList names_ready;
        //! Информация о запросе на асинхронное получение имен моделей: ошибки драйвера БД
        ErrorList errors_ready;
    };
    //! Информация о запросе на асинхронное получение имен моделей, которая поступает менеджеру моделей
    QHash<MessageID, std::shared_ptr<EntityNamesAsyncInfo>> _entity_names_async_mm;

    //! Информация о запросе на асинхронное получение имен моделей (драйвер БД)
    struct EntityNamesAsyncInfoDb
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! Кто запрашивал
        const I_ObjectExtension* requester;
        //! Идентификаторы сущностей
        UidList uids;
        //! Свойства моделей
        DataPropertyList properties;
        //! Язык
        QLocale::Language language = QLocale::Language::AnyLanguage;
    };
    //! Информация о запросе на асинхронное получение имен от драйвера БД
    //! Ключ - отправленное драйверу сообщение MMCommandGetEntityNamesMessage, значение - оригинальное сообщение MMCommandGetEntityNamesMessage
    QHash<MessageID, std::shared_ptr<EntityNamesAsyncInfoDb>> _entity_names_async_db;

    //! Информация о запросе на асинхронное получение значения каталога
    struct CatalogValueAsyncInfo
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! Кто запрашивал
        const I_ObjectExtension* requester;
        //! Коннект к модели справочника
        QMetaObject::Connection connection;

        //! Код сущности каталога
        EntityCode catalog_id;
        //! Код строки каталога
        EntityID id;
        //! Идентификатор свойства каталога для которого надо получить значение
        PropertyID property_id;
        //! Язык
        QLocale::Language language;
        //! База данных
        zf::DatabaseID database_id;
    };
    //! Информация о запросе на асинхронное получение значения каталога
    QHash<MessageID, std::shared_ptr<CatalogValueAsyncInfo>> _catalog_value_async;
    QHash<const Model*, std::shared_ptr<CatalogValueAsyncInfo>> _catalog_value_async_by_model;

    //! event loop для ожидания ответа синхронных операций
    QEventLoop _sync_operation_loop;
    QTimer* _sync_operation_timeout = nullptr;
    Error _sync_error;

    //! Ожидание ответа на синхронный запрос открытия модели
    MessageID _open_sync_feedback_message_id;
    //! Ожидание ответа на синхронный запрос удаления модели
    MessageID _remove_sync_feedback_message_id;

    mutable QRecursiveMutex _mutex;
};

} // namespace zf
