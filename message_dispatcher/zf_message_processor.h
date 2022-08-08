#pragma once

#include <QObject>
#include <QQueue>

#include "zf.h"
#include "zf_callback.h"
#include "zf_message.h"
#include "zf_uid.h"
#include "zf_message_dispatcher.h"

namespace zf
{
/*! Класс для упрощения ожидания ответа на асинхронные сообщения
 * Вместо того, чтобы запоминать id отправляемого сообщения и сравнивать с feedbackId входящих, можно определить набор
 * ключевых значений (видов запросов) ModuleMessages::MessageKey и контролировать ответы по ним. При этом можно в любой
 * момент запросить ответ заново, не дожидаясь его поступления. При этом ответ на старый запрос будет проигнорирован.
 * Объект данного класса автоматически уничтожается вместе с object
 */
class ZCORESHARED_EXPORT MessageProcessor : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    explicit MessageProcessor(const I_ObjectExtension* object,
                              //! Менеджер обратных вызовов
                              CallbackManager* callback_manager);
    ~MessageProcessor() override;

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
    //! Добавить запрос на отправку сообщений
    bool addMessageRequest(
        //! Ключ, по которому отпределяются однотипные запросы. При повторном запросе с
        //! указанным ключом, предыдущий затирается
        const Command& key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data,
        //! Получатель сообщения
        const I_ObjectExtension* receiver,
        //! Сообщение
        const Message& message,
        //! Поставить в очередь и не отправлять до тех пор, пока не будут получены ответы на сообщения с указанными
        //! ключами, если они есть в очереди сообщений
        const QList<Command> queue_keys = {});
    //! Удалить запрос из очереди
    void removeMessageRequest(const Command& key);
    //! Ждем ли ответа на запрос отправки сообщений
    bool isWaitingMessageFeedback(const Command& key) const;
    //! Находится ли запрос в очереди
    bool inQueue(const Command& key) const;

signals:
    //! Вызывается при добавлении запроса. Если такой запрос уже есть и пока не обработан, то не вызыватся
    void sg_startWaitingFeedback(const zf::Command& key);
    //! Вызывается после обработки запроса
    void sg_finishWaitingFeedback(const zf::Command& key);

    //! Поступил ответ
    void sg_feedback(const zf::Command& key, const std::shared_ptr<void>& data, const zf::Message& source_message,
                     const zf::Message& feedback_message);

    //! Запрос добавлен
    void sg_requestAdded(const zf::Command& key);
    //! Запрос удален или выполнен
    void sg_requestRemoved(const zf::Command& key);

public slots:
    //! Для приема сообщений через messageDispatcher
    void sl_inbound_message(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

    //! Обратный вызов от CallbackManager
    void sl_callback(int key, const QVariant& data);

private:
    bool processMessageQueue();

    struct MessageQueueInfo
    {
        Command request_key;
        std::shared_ptr<void> data;
        const I_ObjectExtension* receiver;
        std::unique_ptr<Message> message;
        QList<Command> queue_request_keys;
    };
    QMap<Command, std::shared_ptr<MessageQueueInfo>> _message_queue;

    const I_ObjectExtension* _object;
    Uid _entity_uid;

    QMap<Command, std::shared_ptr<MessageQueueInfo>> _messages;
    QMap<MessageID, Command> _message_id;

    //! Менеджер обратных вызовов
    CallbackManager* _callback_manager;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
