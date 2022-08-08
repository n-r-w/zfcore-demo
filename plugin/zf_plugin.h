#ifndef ZF_PLUGIN_H
#define ZF_PLUGIN_H

#include "zf_callback.h"
#include "zf_core.h"
#include "zf_operation.h"

namespace zf
{
class ZCORESHARED_EXPORT Plugin : public QObject, public I_Plugin, public I_ObjectExtension
{
    Q_OBJECT
    Q_INTERFACES(zf::I_Plugin)
public:
    Plugin(const ModuleInfo& module_info);
    ~Plugin() override;

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
    //! Уникальный идентификатор плагина
    Uid pluginUid() const;

    //! Структура данных модуля
    DataStructurePtr dataStructure(
        //! Для какого экземпляра сущности запрашивается структура.
        //! Если модуль поддерживает общую структуру для всех
        //! сущностей, то можно указать пустой Uid()
        const Uid& entity_uid,
        //! Ошибка
        Error& error) const final;

    //! Задать разделяемую структуру: cтруктура данных сущности с полями, общими для всех экземпляров сущности
    //! Используется для модулей, которые не передают структуру в конструкторе плагина
    void setSharedDataStructure(const DataStructurePtr& structure);
    //! Разделяемая структура данных
    DataStructurePtr sharedDataStructure() const;

    //! Задать структуру для конкретной сущности
    //! Используется для модулей, которые не передают структуру в конструкторе плагина
    void setEntityDataStructure(const Uid& entity_uid, const DataStructurePtr& structure);

    //! Созданные ранее структуры
    QHash<Uid, DataStructurePtr> createdStructure() const final;

    //! Вернуть полный список поддерживаемых операций
    OperationList operations() const final;
    //! Найти операцию по ее коду
    Operation operation(const OperationID& operation_id) const final;

    //! Общая информация о модуле
    const ModuleInfo& getModuleInfo() const final;
    //! Код сущности
    EntityCode entityCode() const final;

    //! Создать новое view для модели
    View* createView(
        //! Модель
        const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Произвольные данные. Для возможности создания разных представлений по запросу
        const QVariant& data) override;    
    //! Создать новую модель
    Model* createModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Является ли модель отсоединенной. Такие модели предназначены для редактирования и не доступны для
        //! получения через менеджер моделей
        bool is_detached) override;

    //! Асинхронное получение имени сущностей.
    //! В ответ необходимо послать сообщение MMEventEntityNamesMessage для sender.
    //! По умолчанию возвращает имя модуля для уникальных сущностей и грузит модель с данными для получения именя для остальных (медленно)
    //! Если имеется возможность получить расшифровку имени другими быстрыми способами (например через справочники) то надо переопределить
    //! этот метод и использовать методы: registerEntityNamesRequest и collectEntityNames
    void getEntityName(
        //! Кому отправить ответное сообщение MMEventEntityNamesMessage
        const I_ObjectExtension* requester,
        //! Идентификатор сообщения, который надо передать в ответное MMEventEntityNamesMessage
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей (все относятся к данному плагину)
        const UidList& entity_uids,
        //! Какое свойство отвечают за хранение имени. Если какое свойство не валидно , то получить свойство с именем на основании
        //! zf::PropertyOption:FullName, а затем zf::PropertyOption:Name
        const DataProperty& name_property,
        //! Язык. Если QLocale::AnyLanguage, то используется язык интерфейса
        QLocale::Language language) override;

    //! Менеджер обратных вызовов
    CallbackManager* callbackManager() const;

    //! Параметры плагина
    QMap<QString, QVariant> options() const;

    //! Каталог, где можно хранить разные данные
    QString moduleDataLocation() const override;

protected:
    //! Модель с данным идентификатором можно использовать в разных потоках одновременно
    bool isModelSharedBetweenThreads(const Uid& entity_uid) const override;

    //! Вызывается ядром после загрузки всех модулей
    void afterLoadModules() override;
    //! Начальная загрузка данных из БД (если надо). Вызывается ядром один раз после инициализации плагина либо программистом при
    //! необходимости перегрузить данные из БД по какому то событию
    Error loadDataFromDatabase() override;

    //! Вызывается ядром при необходимости загрузить конфигурацию модуля. Каталог для хранения файлов: moduleDataLocation
    virtual Error onLoadConfiguration() override;
    //! Вызывается ядром при необходимости сохранить конфигурацию модуля. Каталог для хранения файлов: moduleDataLocation
    virtual Error onSaveConfiguration() const override;

    //! Вызывается ядром перед завершением работы
    void onShutdown() override;

    //! Запрос на подтверждение операции. Вызывается автоматически перед processOperation для
    //! OperationOption::Confirmation
    bool confirmOperation(const Operation& operation) override;
    //! Обработчик операции OperationScope::Module. Вызывается автоматически при активации экшена операции
    Error processOperation(const Operation& operation) override;

    //! Обработчик входящих сообщений от диспетчера сообщений
    virtual zf::Error processInboundMessage(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Обработчик входящих сообщений от диспетчера сообщений.
    virtual zf::Error processInboundMessageAdvanced(
        //! Указатель на отправителя. МОЖЕТ БЫТЬ НЕ ВАЛИДНЫМ! Валидность можно проверить через Core::messageDispatcher()->isObjectRegistered
        const I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

protected:
    //! Запретить кэширование для данного типа сущностей
    void disableCache();

    //! Зарегистрировать запрос на получение имен сущностей. Вызывается в getEntityName
    void registerEntityNamesRequest(
        //! Кому отправить ответное сообщение MMEventEntityNamesMessage
        const I_ObjectExtension* requester,
        //! Идентификатор сообщения, который надо передать в ответное MMEventEntityNamesMessage
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей (все относятся к данному плагину)
        const UidList& entity_uids,
        //! Свойство в котором хранится имя
        const DataProperty& name_property,
        //! Язык
        QLocale::Language language);
    //! Регистрация модели, для которой надо ждать окончания загрузки при получении имен сущностей. Возвращает истину, если такая модель не
    //! была зарегистрирована для данногос сообщения
    bool registerEntityNamesRequestLoadingModel(
        //! Идентификатор сообщения, который надо передать в ответное MMEventEntityNamesMessage
        const MessageID& feedback_message_id,
        //! Указатель на модель
        const ModelPtr& model,
        //! Имя какого идентификатора ищем (в большинстве случаев совпадает с model->entityUid. исключение для поиска по каталогам)
        const Uid& uid);
    //! Накопление имен сообщений для ответа на getEntityName. Если получены все ответы, то автоматически отправляется ответноек сообщение
    bool collectEntityNames(const MessageID& feedback_message_id, const Uid& uid, const QString& name, const Error& error);

    //! Окончание загрузки модели для получения имени сущности
    virtual void onEntityNamesRequestModelLoaded(
        //! Модель
        const Model* model,
        //! Идентификатор сообщения, который надо передать в ответное MMEventEntityNamesMessage
        const MessageID& feedback_message_id,
        //! Ошибка (если была)
        const zf::Error& error);

    //! Обработчик колбек-менеджера
    virtual void onCallback(int key, const QVariant& data);

private slots:
    //! Входящие сообщения от диспетчера сообщений
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    void sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid,
                                                const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Окончание загрузки модели для получения имени сущности
    void sl_entityNamesRequestModelLoaded(const zf::Error& error, const zf::LoadOptions& load_options,
                                          const zf::DataPropertySet& properties);
    //! Обработчик колбек-менеджера
    void sl_callback(int key, const QVariant& data);

private:
    //! Создать структуру данных для сущности
    DataStructurePtr getEntityDataStructure(const Uid& entity_uid,
                                            //! Ошибка
                                            Error& error) const;

    ModuleInfo _module_info;
    mutable QHash<OperationID, Operation> _operation_hash;

    //! Менеджер обратных вызовов
    mutable ObjectExtensionPtr<CallbackManager> _callback_manager;

    //! Созданные структуры данных
    QHash<Uid, DataStructurePtr> _entity_structure;
    //! Общая структура данных
    DataStructurePtr _shared_structure;
    mutable Z_RECURSIVE_MUTEX _mutex;

    //! Информация о запросе на асинхронное получение имен моделей
    struct EntityNamesAsyncInfo
    {
        //! feedback id ответного сообщения
        MessageID feedback_message_id;
        //! Кто запрашивал
        const I_ObjectExtension* requester;
        //! Свойство в котором хранится имя
        DataProperty name_property;
        //! Язык
        QLocale::Language language;
        //! Какие имена надо получить
        UidList to_receive;
        //! Что осталось получить
        UidSet not_received;
        //! Имена и ошибки
        QMap<Uid, QPair<QString, Error>> results;

        //! Модели в процессе загрузки, которые требуются для получения имени. Сохранение указателя, чтобы не было удаления объекта
        QMap<ModelPtr, QMetaObject::Connection> loading_models;
        //! Какие идентификаторы относятся к загружаемым моделям
        QMap<const Model*, Uid> loadind_model_uids;
    };
    //! Информация о запросе на асинхронное получение имен моделей
    QHash<MessageID, std::shared_ptr<EntityNamesAsyncInfo>> _entity_names_async;
    //! Модели в процессе загрузки, которые требуются для получения имени
    QMap<Model*, std::shared_ptr<EntityNamesAsyncInfo>> _entity_names_async_loading_models;

    mutable QSet<Uid> _is_releation_update_initialized;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    struct InboundMessageInfo
    {
        Uid sender;
        Message message;
        zf::SubscribeHandle subscribe_handle;
    };
    QQueue<std::shared_ptr<InboundMessageInfo>> _inbound_msg_buffer;
    FeedbackTimer _inbound_msg_buffer_timer;
    bool _inbound_msg_buffer_processing = false;
    void storeInboundMessage(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle);
    void processInboundMessages();

    friend class Framework;
};

} // namespace zf

#endif // ZF_PLUGIN_H
