#ifndef ZF_MESSAGE_DISPATCHER_H
#define ZF_MESSAGE_DISPATCHER_H

#include "zf_callback.h"
#include "zf_message.h"
#include "zf_message_channel.h"
#include "zf_sequence_generator.h"
#include "zf_object_extension.h"

#include <QEventLoop>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQueue>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#include <QRecursiveMutex>
#else
#include <QMutex>
#endif

class QTimer;

namespace zf
{
/*! Использование диспетчера сообщений
Глобальный объект: Core::messageDispatcher()
1. Зарегистрировать объект в диспетчере через registerObject
2. У объекта должен быть определен слот вида:
    void sl_inbound_message(
        //! Отправитель
        const zf::Uid& sender,
        //! Сообщение
        const zf::Message& message,
        //! Идентификатор подписки на канал (только для рассылок через каналы)
        zf::SubscribeHandle subscribe_handle);
    ВАЖНО: параметры должны выглядеть именно так как написано (не убирать namespace)
3. В слоте обязательно вызывать метод confirmMessageDelivery для подтверждения получения сообщения
4. При необходимости подписаться на каналы через subscribe
5. После этого можно отправлять сообщения через sendMessage/postMessage/postMessageToChannel и принимать их

Дополнительно можно задать необязательный слот вида:
    void sl_inbound_message_advanced(
        //! Указатель но отправителя. ВАЖНО! Нет гарантии что в момент вызова слота это будет валидный указатель
        //! Использовать только после проверки MessageDispatcher::isObjectRegistered
        const zf::I_ObjectExtension* sender_ptr
        //! Отправитель
        const zf::Uid& sender_uid,
        //! Сообщение
        const zf::Message& message,
        //! Идентификатор подписки на канал (только для рассылок через каналы)
        zf::SubscribeHandle subscribe_handle);

*/

//! Диспетчер сообщений
class ZCORESHARED_EXPORT MessageDispatcher : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    //! Имя свойства объектов, регестрируемых в MessageDispatcher
    //! Выводится при отладке сообщений
    static const char* OWNER_NAME_PROPERTY;

    explicit MessageDispatcher();
    ~MessageDispatcher();

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
    //! Начальная инициализация
    void bootstrap();

    //! Подтвердить получение сообщения. Необходимо обязательно вызывать в обработчике
    //! Необходимо для корректной отработки processMessages
    bool confirmMessageDelivery(const Message& message, const I_ObjectExtension* object);

    //! Возвращает истину, если такой объект зарегистрирован в диспетчере
    bool isObjectRegistered(const I_ObjectExtension* object) const;
    //! Возвращает истину, если такой объект c таким идентификатором зарегистрирован в диспетчере
    bool isObjectRegistered(const Uid& id) const;

    //! Идентификатор зарегистрированного объекта
    Uid objectUid(const I_ObjectExtension* object) const;
    //! Список зарегистрированных объектов для данного идентификатора
    QList<const I_ObjectExtension*> objects(const Uid& id) const;

    //! Регистрация объекта в диспетчере
    bool registerObject(
        //! Идентификатор, под которым объект будет зарегистрирован в диспетчере
        const Uid& id,
        //! Объект
        const I_ObjectExtension* object,
        /*! Название слота для получения сообщений. Слот должен иметь вид:
         * (const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle)
         * ВАЖНО: параметры должены выглядеть именно так как написано (не убирать namespace) */
        const QString& slot,
        /*! Необязательный дополнительный слот вида:            
         * (const I_ObjectExtension* sender_ptr, const zf::Uid& sender_uid, const zf::Message& message, zf::SubscribeHandle subscribe_handle)
         * ВАЖНО: параметры должены выглядеть именно так как написано (не убирать namespace) */
        const QString& slot_advanced = QString());
    //! Удаление регистрации объекта в диспетчере
    bool unRegisterObject(const I_ObjectExtension* object);

    //! Асинхронно отправить сообщение объекту по указателю
    bool postMessage(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Получатель
        const I_ObjectExtension* receiver,
        //! Сообщение
        const Message& message);
    //! Асинхронно отправить сообщение объекту по указателю
    bool postMessage(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Список получателей
        const QList<const I_ObjectExtension*>& receivers,
        //! Сообщение
        const Message& message);
    /*! Упрощенный метод асинхронной отправки сообщения
     * Используется если надо просто отправить сообщение с каким-то кодом.
     * Результатом отправки будет идентификатор сформированного сообщения. Если получатель в настоящий момент не зарегистрирован, то
     * MessageID будет не валидным
     * ВАЖНО: код должен быть уникальным в рамках сообщений, которые обрабатывает получатель! */
    MessageID postMessage(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Получатель
        const Uid& receiver,
        //! Код сообщения
        int message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id = MessageID(),
        //! Данные
        const DataContainerList& data = {});

    //! Асинхронно отправить сообщение объекту по его идетификатору
    //! Сообщение будет отправлено всем объектам с идентификатором receiver
    bool postMessage(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Получатель
        const Uid& receiver,
        //! Сообщение
        const Message& message);

    /*! Синхронно отправить сообщение объекту и получить ответ. Если ответ не получен, то результат isValid() == false
     * ВАЖНО: получатель примет сообщение, отправленное от MessageDispatcher - CoreUids::MESSAGE_DISPATCHER(), т.к.
     * внутри происходит симуляция синхронной отправки и ответ ждет MessageDispatcher, который затем возвращает его из
     * этой функции */
    Message sendMessage(
        //! Получатель
        const I_ObjectExtension* receiver,
        //! Сообщение
        const Message& message,
        //! Время ожидания ответа. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Игнорировать блокировку обработки сообщений
        bool force = false);
    /*! Синхронно отправить сообщение объекту и получить ответ. Если ответ не получен, то результат isValid() == false
     * ВАЖНО: получатель примет сообщение, отправленное от MessageDispatcher - CoreUids::MESSAGE_DISPATCHER(), т.к.
     * внутри происходит симуляция синхронной отправки и ответ ждет MessageDispatcher, который затем возвращает его из
     * этой функции */
    Message sendMessage(
        //! Получатель
        const Uid& receiver,
        //! Сообщение
        const Message& message,
        //! Время ожидания ответа. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Игнорировать блокировку обработки сообщений
        bool force = false);

    //! Синхронно отправить несколько сообщений
    QList<Message> sendMessages(
        //! Получатель
        const I_ObjectExtension* receiver,
        //! Сообщение
        const QList<Message>& message,
        //! Время ожидания ответа. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Игнорировать блокировку обработки сообщений
        bool force = false);

    //! Возвращает истину, если такой канал зарегистрирован в диспетчере
    bool isChannelRegistered(const MessageChannel& id) const;
    //! Регистрирует канал. Возвращает ложь, если такой канал уже зарегистрирован
    bool registerChannel(const MessageChannel& id);
    //! Имеет ли канал подписчиков
    bool hasSubscribers(const MessageChannel& id) const;

    //! Список идентификаторов подписки на канал для объекта
    QList<SubscribeHandle> subscribeHandles(const MessageChannel& channel, const I_ObjectExtension* subscriber) const;

    //! Подписать объект на события канала. Возвращает уникальный идентификатор подписки или 0 при ошибке
    SubscribeHandle subscribe(
        //! Канал
        const MessageChannel& channel,
        //! Подписчик
        const I_ObjectExtension* subscriber,
        //! Получать сообщения в данном канале только если они содержат информацию о сущностях указанного типа
        const EntityCodeList& contains_types = EntityCodeList(),
        //! Получать сообщения в данном канале только если они содержат информацию об указанных сущностях
        const QList<Uid>& contains = QList<Uid>(),
        //! Получать сообщения в данном канале только от отправителей указанного типа
        const EntityCodeList& sender_types = EntityCodeList(),
        //! Получать сообщения в данном канале только от указанных отправителей
        const QList<Uid>& senders = QList<Uid>());
    //! Подписать объект на события нескольких каналов. Возвращает список уникальных идентификаторов подписки
    //! При ошибке - возвращает пустой список
    QList<SubscribeHandle> subscribe(
        //! Список каналов
        const QList<MessageChannel>& channels,
        //! Подписчик
        const I_ObjectExtension* subscriber,
        //! Получать сообщения в данном канале только если они содержат информацию о сущностях указанного типа
        const EntityCodeList& contains_types = EntityCodeList(),
        //! Получать сообщения в данном канале только если они содержат информацию об указанных сущностях
        const QList<Uid>& contains = QList<Uid>(),
        //! Получать сообщения в данном канале только от отправителей указанного типа
        const EntityCodeList& sender_types = EntityCodeList(),
        //! Получать сообщения в данном канале только от указанных отправителей
        const QList<Uid>& senders = QList<Uid>());

    //! Отписать объект от события канала
    bool unSubscribe(SubscribeHandle subscribe_handle);
    //! Отписать объект от событий нескольких каналов
    bool unSubscribe(const QList<SubscribeHandle>& subscribe_handles);
    //! Отписать объект от всех каналов
    void unSubscribe(const I_ObjectExtension* subscriber);

    //! Поместить сообщение в канал. Его получат все, кто был подписан на данный канал
    bool postMessageToChannel(const MessageChannel& channel, const Uid& sender, const Message& message);
    //! Поместить сообщение в несколько каналов. Их получат все, кто был подписан на эти каналы
    bool postMessageToChannel(const QList<MessageChannel>& channels, const Uid& sender, const Message& message);
    //! Поместить сообщение в канал. Его получат все, кто был подписан на данный канал
    bool postMessageToChannel(const MessageChannel& channel, const Message& message);
    //! Поместить сообщение в несколько каналов. Их получат все, кто был подписан на эти каналы
    bool postMessageToChannel(const QList<MessageChannel>& channels, const Message& message);

    //! Заблокировать отправку сообщений
    void stop();
    //! Разблокировать отправку сообщений
    void start();
    //! Заблокирована ли отправка сообщений
    bool isStarted() const;

    //! Задать разрешенных получателей. Для других сообщения в буфере обрабатываться не будут
    void setEnabledReceivers(const UidList& entity_uids);

    //! Заблокировать отправку сообщений определенного типа в канал
    void blockChannelMessageType(const MessageChannel& channel_id, MessageType t);
    //! Разблокировать отправку сообщений определенного типа в канал. Возвращает счетчик блокировки
    int unBlockChannelMessageType(const MessageChannel& channel_id, MessageType t);
    //! Виды сообщений заблокированных для указанного канала
    QList<MessageType> blockedChanelMessageTypes(const MessageChannel& channel_id) const;

private slots:
    //! Колбек обработки буфера
    void sl_bufferCallbackTimeout();

    //! Входящие сообщения для MessageDispatcher
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    //! Принудительно обработать сообщение из очереди
    Q_SLOT void forceMessage(const zf::MessageID& message_id);

    //! Вызывать, если идет ожидание синхронного ответа от внешнего объекта (например драйвера БД)
    //! Допустимы вложенные вызовы. Необходимо для корректной отработки processMessages, который без этого не узнает что
    //! ему надо ждать ответа внешнего объекта.
    void waitForExternalEventsStart();
    //! Вызывать, когда закончилось ожидание синхронного ответа от внешнего объекта (например драйвера БД)
    void waitForExternalEventsFinish();

    struct ObjectInfo;
    struct ChannelInfo;
    struct BufferInfo;
    struct MessageQueueInfo;
    struct SubscribeInfo;
    struct SendMessageInfo;

    typedef std::shared_ptr<ObjectInfo> ObjectInfoPtr;    
    typedef std::shared_ptr<MessageQueueInfo> MessageQueueInfoPtr;
    typedef std::shared_ptr<ChannelInfo> ChannelInfoPtr;
    typedef std::shared_ptr<SubscribeInfo> SubscribeInfoPtr;
    typedef std::shared_ptr<BufferInfo> BufferInfoPtr;
    typedef std::shared_ptr<SendMessageInfo> SendMessageInfoPtr;

    //! Положить сообщение в буфер
    void putMessageToBuffer(
        //! Отправитель
        const ObjectInfoPtr& sender,
        //! Канал. Заполнен либо канал, либо получатель
        const MessageChannel& channel,
        //! Получатель
        const QList<const I_ObjectExtension*>& receivers, const Message& message);

    //! Положить сообщение в буфер (вызов через QMetaObject::invokeMethod)
    Q_SLOT void putMessageToBuffer_thread(
        //! Отправитель
        const zf::I_ObjectExtension* sender,
        //! Канал. Заполнен либо канал, либо получатель
        const zf::MessageChannel& channel,
        //! Получатель
        const QList<const zf::I_ObjectExtension*>& receivers, const zf::MessagePtr& message);

    struct MessageQueueInfo
    {
        ObjectInfoPtr sender;
        ObjectInfoPtr receiver_info;

        MessagePtr message;
        SubscribeHandle subscribe_handle;
    };
    QQueue<MessageQueueInfoPtr> _message_queue;

    //! Добавить сообщение в очередь на отправку
    void enqueueMessage(const ObjectInfoPtr& sender, ObjectInfoPtr& receiver_info, const MessagePtr& message,
                        SubscribeHandle subscribe_handle);
    //! Обработать очередь сообщений
    void processMessageQueue();
    //! Отправить сообщение получателю
    void postMessageHelper(const ObjectInfoPtr& sender, ObjectInfoPtr& receiver_info, const MessagePtr& message,
                           SubscribeHandle subscribe_handle);

    //! Отправить отладочное сообщение в окно debug
    void postDebugMessage(const QString& sender, const QString& receiver, const MessagePtr& message, SubscribeHandle subscribe_handle);

    QList<ObjectInfoPtr> objectInfo(const Uid& id) const;
    ObjectInfoPtr objectInfo(const I_ObjectExtension* object) const;
    ChannelInfoPtr channelInfo(MessageChannel id) const;
    //! Обработка буфера
    void processBuffer(const QSet<MessageID>& force_messages);
    //! Информация об объекте
    static QString getObjectDescription(const Uid& uid, const I_ObjectExtension* obj);

    //! хранение данных по зарегистрированным объектам
    struct ObjectInfo
    {
        Uid id;
        I_ObjectExtension* object_prt = nullptr;
        QByteArray slot;
        QByteArray slot_advanced;
    };
    QMultiHash<Uid, ObjectInfoPtr> _objects_by_id;
    QHash<I_ObjectExtension*, ObjectInfoPtr> _objects_by_ptr;

    //! Информация о подписке на канал
    struct SubscribeInfo
    {
        ~SubscribeInfo();

        std::weak_ptr<ChannelInfo> channel_info;
        ObjectInfoPtr object_info;
        EntityCodeList contains_types;
        QList<Uid> contains;
        EntityCodeList sender_types;
        QList<Uid> senders;
        bool any_message = false;
        //! Уникальный id подписки
        SubscribeHandle handle = nullptr;
    };
    //! Ключ - handle подписки
    QHash<SubscribeHandle, SubscribeInfoPtr> _subscribe_info;

    //! хранение данных по зарегистрированным каналам
    struct ChannelInfo
    {        
        MessageChannel id;
        //! Список подписок на канал
        QList<SubscribeInfoPtr> subscribe;
        //! Подписки по объекту
        QMultiHash<const I_ObjectExtension*, SubscribeInfoPtr> subscribe_by_object;
        //! Виды заблокированных сообщений и счетчик блокировок
        QMap<MessageType, int> blocked_types;
    };
    QList<ChannelInfoPtr> _channels;

    //! Буфер сообщений
    struct BufferInfo
    {
        ObjectInfoPtr reciever_info;
        ChannelInfoPtr channel_info;
        SubscribeHandle subcribe_handle;
        ObjectInfoPtr sender_info;
        MessagePtr message;
    };
    //! Буфер, в который попадают сообщения на отправку
    QQueue<BufferInfoPtr> _buffer;

    //! Данные для синхронной отправки сообщений
    struct SendMessageInfo
    {
        Qt::HANDLE thread_id = nullptr;
        //! event loop для ожидания ответа синхронных сообщений sendMessage
        QEventLoop send_message_loop;
        //! Таймер для ожидания ответа синхронных сообщений sendMessage
        QTimer* send_message_timer = nullptr;
        //! Код callback для ожидания ответа синхронных сообщений sendMessage
        const int send_message_callback_key = 2;
        //! Идентификатор отправленного сообщения sendMessage и ответ на него
        QMap<MessageID, Message> message_data;
        //! Сколько ответов получено
        int received = 0;
    };
    QMutex _send_message_mutex;
    //! Ключ - id потока
    QHash<Qt::HANDLE, SendMessageInfoPtr> _send_message_info;
    //! Ключ - id сообщения
    QHash<MessageID, SendMessageInfoPtr> _send_message_info_by_id;

    //! Недоставленные сообщения ключ - id сообщения
    QMultiHash<MessageID, const I_ObjectExtension*> _not_delivered;

    //! Счетчик ожидания ответа от внешнего источника
    int _wait_external_counter = 0;

    //! В процессе удаления
    bool _deleted = false;

    //! Разрешенные получатели
    QSet<Uid> _enabled_receivers;

    //! Счетчик блокировки отправки сообщений
    int _stop_count = 0;

    FeedbackTimer _buffer_callback_timer;

    mutable Z_RECURSIVE_MUTEX _mutex;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    friend class DatabaseManager;
};

} // namespace zf

#endif // ZF_MESSAGE_DISPATCHER_H
