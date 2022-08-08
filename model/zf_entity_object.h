#pragma once

#include "zf_command_processor.h"
#include "zf_message.h"
#include "zf_message_processor.h"
#include "zf_module_data_object.h"
#include "zf_connection_information.h"

namespace zf
{
//! Сущность
class ZCORESHARED_EXPORT EntityObject : public ModuleDataObject
{
    Q_OBJECT
public:
    EntityObject(
        //! Уникальный идентификатор сущности
        const Uid& entity_uid,
        //! Структура данных модели
        const DataStructurePtr& data_structure,
        //! Объект, который отвечает за проверку ошибок и генерирует запросы автоматические на проверку. Может быть
        //! равно nullptr, тогда назначается этот объект
        HighlightProcessor* master_highlight,
        //! Параметры ModuleDataObject
        const ModuleDataOptions& options);
    ~EntityObject() override;

    //! Уникальный идентификатор сущности
    //! ВАЖНО!!! Модель zf::Model может быть создана как временная и затем получить постоянный идентификатор persistentUid
    //! после сохранения в БД, но это не отразится на ее entityUid и свойстве isPersistent
    Uid entityUid() const;
    //! Является ли временной (не существующей в БД - с временным идентификатором)
    bool isTemporary() const;
    //! Является ли постоянной (с реальным идентификатором)
    //! ВАЖНО!!! Модель zf::Model может быть создана как временная и затем получить постоянный идентификатор persistentUid
    //! после сохранения в БД, но это не отразится на ее entityUid и данном свойстве
    bool isPersistent() const;

    //! Существует в БД. Свойство относится к наличию этого объекта в БД
    bool isExists() const;

    //! В какой БД находится данная сущность
    DatabaseID databaseId() const;
    //! К какой БД относится дочерний элемент данной сущности
    DatabaseID childDatabaseId(
        //! Код сущности дочернего элемента (например карточка сотрудника для списка сотрудников)
        const EntityCode& entity_code) const;

    //! Имя сущности
    virtual QString entityName() const;
    //! Свойство, отвечающее за имя объекта (используется если не было задано имя через setEntityName)
    DataProperty nameProperty() const;

    //! Задать имя сущности
    void setEntityName(const QString& s);
    //! Было ли задано имя сущности через setEntityName
    bool isEntityNameRedefined() const;

    //! Возможности программы (глобальный параметр)
    static const ProgramFeatures& features();

    //! Плагин
    I_Plugin* plugin() const;

    template <class T> T* plugin() const
    {
        I_Plugin* p = plugin();
        T* converted = dynamic_cast<T*>(p);
        if (converted == nullptr) {
            Z_HALT("Ошибка преобразования типа");
        }
        return converted;
    }

public:
    //! Прямые права доступа к данной сущности
    AccessRights directAccessRights() const override;
    //! Косвенные права доступа к данному типу сущности
    AccessRights relationAccessRights() const override;

protected:
    //! Имя конфигурации конкретной сущности
    QString entityConfigurationCode() const;

    //! Указать существует ли экземпляр сущности в БД
    void setExists(bool b) const;

    //! Отправить произвольное сообщение. Ответ придет в processInboundMessage.
    //! При обработке processInboundMessage проверять на совпадение:
    //! message/параметр processInboundMessage/.feedbackMessageId() == message/параметр этого метода/.messageId()
    bool postMessage(
        //! Получатель
        const I_ObjectExtension* receiver,
        //! Сообщение
        const Message& message) const;

    /*! Отправить сообщение вида key получателю. Ответ поступит в onMessageCommandFeedback
     * Метод упрощает работу с асинхронными сообщениями, т.к. позволяет следить не сообщениями с ответом, а за
     * совпадением ключа. Ответ на сообщение поступит в виртуальный метод EntityObject::onMessageCommandFeedback, в котором
     * надо проверить ключ. Вторым плюсом является, то что можно не дожидаясь ответа на сообщение, снова вызвать
     * postMessageCommand с таким же ключом и не следить за цепочкой вызовов */
    virtual bool postMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Получатель
        const I_ObjectExtension* receiver,
        //! Сообщение
        const Message& message,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data = std::shared_ptr<void>()) const;

    //! Запрос на выполнение команды добавлен
    virtual void onMessageCommandAdded(const zf::Command& key);
    //! Запрос на выполнение команды удален или выполнен
    virtual void onMessageCommandRemoved(const zf::Command& key);

    //! Ждем ли ответа на запрос отправки сообщений
    bool isWaitingMessageCommand(const Command& message_key) const;
    //! Находится ли запрос в очереди
    bool isMessageCommandInQueue(const Command& message_key) const;
    //! Отправка сообщения менеджеру БД. Метод аналогичен postMessageCommand
    bool postDatabaseMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Сообщение
        const Message& message,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data = std::shared_ptr<void>()) const;

    //! Вызывается при получении ответа на postMessageCommand
    virtual Error onMessageCommandFeedback(const Command& message_key, const std::shared_ptr<void>& custom_data,
                                           const Message& source_message, const Message& feedback_message);

    //! Вызывается при добавлении сообщения через EntityObject::postMessageCommand
    virtual void onStartWaitingMessageCommandFeedback(const Command& message_key);
    //! Вызывается после обработки запроса через EntityObject::onMessageCommandFeedback
    virtual void onFinishWaitingMessageCommandFeedback(const Command& message_key);

    //! Вызывается при необходимости сгенерировать сигнал об ошибке
    virtual void registerError(const Error& error, ObjectActionType type = ObjectActionType::Undefined);

    //! Имя объекта для отладки
    QString getDebugName() const override;

protected:
    /*! Добавить запрос на выполнение команды  Ответ поступит в виртуальный метод EntityObject::processCommand
     * Необходимо для реализации произвольных асинхронных обработчиков в модели. Выполнение команд выстраивается в
     * очередь. Таким образом можно передать модели несколько команд и они будут выполнены последовательно в
     * processCommand. Переход к следующей команде начинается после вызова программистом метода
     * EntityObject::finishCommand */
    virtual void addCommandRequest(
        //! Ключ команды (ModuleCommands::CommandKey)
        const Command& command_key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data = std::shared_ptr<void>()) const;

    //! Удаляет все команды с данным ключом. Возвращает количество удаленных
    //! Не действует на команду, выполняемую в настоящий момент
    virtual int removeCommandRequests(
        //! Ключ команды ModuleCommands::CommandKey
        const Command& command_key,
        //! Если не nullptr, то удаляет только команды с парой command_key/custom_data
        const std::shared_ptr<void>& custom_data = std::shared_ptr<void>()) const;

    //! Обработка команды
    virtual void processCommand(const Command& command_key, const std::shared_ptr<void>& custom_data);
    //! Закончить выполнение текущей команды. Выполнение команды начинается с вызова onProcessCommand и заканчивается
    //! этим методом. Только после этого пойдет выполнение следующей команды
    bool finishCommand() const;

    //! Команда на отправку сообщения добавлена
    virtual void onCommandRequestAdded(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);
    //! Команда на отправку сообщения удалена или завершилась
    virtual void onCommandRequestRemoved(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);

    //! Ждем ли выполнение команды
    bool isWaitingCommandExecute(const Command& command_key) const;
    //! Данные для команды при условии что она находится в очереди на выполнение. Если нет, то ошибка
    QList<std::shared_ptr<void>> commandData(const Command& command_key) const;

    //! Выполняется ли сейчас команда
    bool isCommandExecuting() const;
    //! Текущая выполнямая команда (ключ) (если нет, то 0)
    Command currentCommand() const;
    //! Текущие данные
    std::shared_ptr<void> currentData() const;

protected:
    //! Обработчик входящих сообщений от диспетчера сообщений
    virtual zf::Error processInboundMessage(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Обработчик входящих сообщений от диспетчера сообщений.
    virtual zf::Error processInboundMessageAdvanced(
        //! Указатель на отправителя. МОЖЕТ БЫТЬ НЕ ВАЛИДНЫМ! Валидность можно проверить через Core::messageDispatcher()->isObjectRegistered
        const I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

    //! Изменилось имя сущности
    virtual void onEntityNameChanged();
    //! Сменилась информация о подключении к БД
    virtual void onConnectionChanged(const ConnectionInformation& info);

    //! Изменилось значение свойства
    void onPropertyChanged(const DataProperty& p,
                           //! Старые значение (работает только для полей)
                           const LanguageMap& old_values) override;
    //! Все свойства были разблокированы
    void onAllPropertiesUnBlocked() override;
    //! Свойство были разаблокировано
    void onPropertyUnBlocked(const DataProperty& p) override;

    //! Смена сущности
    void setEntity(const Uid& entity_uid);

    //! Обработчик менеджера обратных вызовов
    void processCallbackInternal(int key, const QVariant& data) override;

    //! Изменилось имя сущности
    virtual void onEntityNameChangedHelper();

signals:
    //! Изменился признак существования объекта в БД
    void sg_existsChanged();
    //! Изменилось имя сущности
    void sg_entityNameChanged();
    //! Изменился идентификатор
    void sg_entityChanged(const zf::Uid& old_entity, const zf::Uid& new_entity);

    //! Запущена команда
    void sg_commandReguested(const zf::Command& command_key, const std::shared_ptr<void>& custom_data);
    //! Команда удалена
    void sg_commandRemoved(const zf::Command& command_key, const std::shared_ptr<void>& custom_data);
    //! Команда начала выполняться
    void sg_commandExecuting(const zf::Command& command_key, const std::shared_ptr<void>& custom_data);
    //! Команда завершена
    void sg_commandFinished(const zf::Command& command_key, const std::shared_ptr<void>& custom_data);

    //! Вызывается когда происходит ошибка внутри всяких обработчиков сообщений и т.п.
    void sg_error(zf::ObjectActionType type, const zf::Error& error);

private slots:
    //! Поступил ответ от MessageProcessor
    void sl_message_feedback(const zf::Command& key, const std::shared_ptr<void>& custom_data, const zf::Message& source_message,
                             const zf::Message& feedback_message);
    //! Вызывается при добавлении запроса MessageProcessor
    void sl_startWaitingFeedback(const zf::Command& key);
    //! Вызывается после обработки запроса MessageProcessor
    void sl_finishWaitingFeedback(const zf::Command& key);
    //! Входящие сообщения от диспетчера
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    void sl_message_dispatcher_inbound_advanced(const zf::I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid,
                                                const zf::Message& message, zf::SubscribeHandle subscribe_handle);

    //! Запрос добавлен
    void sl_requestAdded(const zf::Command& key);
    //! Запрос удален или выполнен
    void sl_requestRemoved(const zf::Command& key);
    //! Команда добавлена
    void sl_commandAdded(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);
    //! Команда удалена или завершилась
    void sl_commandRemoved(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);

private:
    //! Обработка команды
    void processCommandHelper(const Command& command_key, const std::shared_ptr<void>& custom_data);
    //! Подписка на каналы
    void subscribeToChannels();

    //! Обработка ожидание ответа на асинхронные сообщения
    MessageProcessor* _message_processor = nullptr;
    //! Работа с очередями команд
    CommandProcessor* _command_processor = nullptr;

    //! Уникальный идентификатор сущности
    Uid _entity_uid;
    //! Существует в БД
    mutable bool _is_exists = false;

    //! Имя сущности
    QString _entity_name;

    //! Свойство, отвечающее за имя объекта
    mutable std::unique_ptr<DataProperty> _name_property;

    //! Подписка на канал ENTITY_CHANGED
    SubscribeHandle _subscribe_handle_entity_changed;
    //! Подписка на канал ENTITY_REMOVED
    SubscribeHandle _subscribe_handle_entity_removed;
    //! Подписка на канал CONNECTION_INFORMATION
    SubscribeHandle _subscribe_handle_connection_info;

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
};

} // namespace zf
