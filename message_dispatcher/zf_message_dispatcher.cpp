#include "zf_message_dispatcher.h"
#include "zf_core.h"
#include "zf_exception.h"
#include "zf_translation.h"
#include "zf_framework.h"

#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

namespace zf
{
//! Имя свойства объектов, регестрируемых в MessageDispatcher
//! Выводится при отладке сообщений
const char* MessageDispatcher::OWNER_NAME_PROPERTY = "Z_MSD_NAME";

MessageDispatcher::MessageDispatcher()
    : QObject()
    , _object_extension(new ObjectExtension(this))
{    
    connect(&_buffer_callback_timer, &FeedbackTimer::timeout, this, &MessageDispatcher::sl_bufferCallbackTimeout);
}

MessageDispatcher::~MessageDispatcher()
{
    _deleted = true;
    delete _object_extension;
}

bool MessageDispatcher::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void MessageDispatcher::bootstrap()
{
    Z_CHECK(Utils::isMainThread());

    // регистрируем самого себя для эмуляции работы с синхронными сообщениями
    Z_CHECK(registerObject(CoreUids::MESSAGE_DISPATCHER, this, "sl_message_dispatcher_inbound"));
}

bool MessageDispatcher::confirmMessageDelivery(const Message& message, const I_ObjectExtension* object)
{
    Z_CHECK(message.isValid());
    Z_CHECK_NULL(object);

    QMutexLocker lock(&_mutex);
    return _not_delivered.remove(message.messageId(), object) > 0;
}

void MessageDispatcher::waitForExternalEventsStart()
{
    // метод вызывается только из главного потока
    _wait_external_counter++;
}

void MessageDispatcher::waitForExternalEventsFinish()
{
    // метод вызывается только из главного потока
    Z_CHECK(_wait_external_counter > 0);
    _wait_external_counter--;
}

bool MessageDispatcher::isObjectRegistered(const I_ObjectExtension* object) const
{
    Z_CHECK_NULL(object);
    QMutexLocker lock(&_mutex);
    return objectInfo(object) != nullptr;
}

bool MessageDispatcher::isObjectRegistered(const Uid& id) const
{
    QMutexLocker lock(&_mutex);
    return !objectInfo(id).isEmpty();
}

Uid MessageDispatcher::objectUid(const I_ObjectExtension* object) const
{
    QMutexLocker lock(&_mutex);
    auto o_info = objectInfo(object);
    return o_info == nullptr ? Uid() : o_info->id;
}

QList<const I_ObjectExtension*> MessageDispatcher::objects(const Uid& id) const
{
    QMutexLocker lock(&_mutex);

    auto o_info = objectInfo(id);
    QList<const I_ObjectExtension*> res;
    for (auto& o : qAsConst(o_info)) {
        res << const_cast<I_ObjectExtension*>(o->object_prt);
    }

    return res;
}

bool MessageDispatcher::registerObject(const Uid& id, const I_ObjectExtension* object, const QString& slot, const QString& slot_advanced)
{
    Z_CHECK_NULL(object);
    Z_CHECK(!slot.isEmpty());
    Z_CHECK(id.isValid());

    QMutexLocker lock(&_mutex);

    I_ObjectExtension* notc_object = const_cast<I_ObjectExtension*>(object);

    if (isObjectRegistered(object))
        return false;

    auto info = Z_MAKE_SHARED(ObjectInfo);
    info->id = id;    
    info->object_prt = notc_object;
    info->slot = slot.toLatin1();
    info->slot_advanced = slot_advanced.toLatin1();

    _objects_by_id.insert(id, info);
    _objects_by_ptr[notc_object] = info;

    objectExtensionRegisterUseInternal(notc_object);

    return true;
}

bool MessageDispatcher::unRegisterObject(const I_ObjectExtension* object)
{
    if (!Core::isBootstraped())
        return true;

    QMutexLocker lock(&_mutex);

    Z_CHECK_NULL(object);
    I_ObjectExtension* notc_object = const_cast<I_ObjectExtension*>(object);

    if (!isObjectRegistered(object))
        return false;

    unSubscribe(object);

    auto o_info = _objects_by_ptr.value(notc_object);
    Z_CHECK_NULL(o_info);

    Z_CHECK(_objects_by_ptr.remove(notc_object));
    Z_CHECK(_objects_by_id.remove(o_info->id, o_info));

    // удаляем данные по этому объекту из буфера
    for (int i = _buffer.count() - 1; i >= 0; i--) {
        auto b = _buffer.at(i);
        if (b->reciever_info != nullptr && b->reciever_info->object_prt == object)
            _buffer.removeAt(i);
    }

    auto not_delivered_msg = _not_delivered.keys(object);
    for (auto& id : not_delivered_msg)
        _not_delivered.remove(id);

    for (int i = _message_queue.count() - 1; i >= 0; i--) {
        if (_message_queue.at(i)->receiver_info->object_prt == object) {
            _message_queue.removeAt(i);
        }
    }

    objectExtensionUnregisterUseInternal(notc_object);

    return true;
}

bool MessageDispatcher::postMessage(const I_ObjectExtension* sender, const Uid& receiver, const Message& message)
{
    Z_CHECK(message.isValid());

    QMutexLocker lock(&_mutex);

    QList<const I_ObjectExtension*> objs = objects(receiver);
    if (objs.count() == 0)
        return false;

    return postMessage(sender, objs, message);
}

bool MessageDispatcher::postMessage(const I_ObjectExtension* sender, const I_ObjectExtension* receiver, const Message& message)
{
    return postMessage(sender, QList<const I_ObjectExtension*> {receiver}, message);
}

Message MessageDispatcher::sendMessage(const Uid& receiver, const Message& message, int timeout_ms, bool force)
{
    Z_CHECK(message.isValid());

    QMutexLocker lock(&_mutex);

    auto receivers = objects(receiver);
    Z_CHECK(receivers.count() == 1);

    return sendMessage(receivers.at(0), message, timeout_ms, force);
}

QList<Message> MessageDispatcher::sendMessages(const I_ObjectExtension* receiver, const QList<Message>& messages, int timeout_ms,
                                               bool force)
{
    Z_CHECK(!messages.isEmpty());

    _send_message_mutex.lock();
    auto info = _send_message_info.value(QThread::currentThreadId());
    _send_message_mutex.unlock();
    Z_CHECK(info == nullptr); // проверка на зацикливание

    info = Z_MAKE_SHARED(SendMessageInfo);
    _send_message_mutex.lock();
    _send_message_info[QThread::currentThreadId()] = info;
    _send_message_mutex.unlock();

    info->thread_id = QThread::currentThreadId();

    for (auto& message : messages) {
        _send_message_mutex.lock();
        _send_message_info_by_id[message.messageId()] = info;
        _send_message_mutex.unlock();

        info->message_data[message.messageId()] = Message();

        Z_CHECK(postMessage(this, receiver, message));

        if (force)
            QMetaObject::invokeMethod(this, "forceMessage", Qt::QueuedConnection, Q_ARG(zf::MessageID, message.messageId()));
    }

    if (timeout_ms > 0) {
        info->send_message_timer = new QTimer;
        info->send_message_timer->setSingleShot(true);
        auto loop_ptr = &info->send_message_loop;
        connect(
            info->send_message_timer, &QTimer::timeout, this, [loop_ptr]() { loop_ptr->quit(); }, Qt::DirectConnection);
        info->send_message_timer->start(timeout_ms);
    }

    info->send_message_loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    delete info->send_message_timer;

    QList<Message> result;
    for (auto& message : messages) {
        Message feedback_message = info->message_data.value(message.messageId());
        result << feedback_message;
    }

    _send_message_mutex.lock();
    Z_CHECK(_send_message_info.remove(QThread::currentThreadId()));
    for (auto& message : messages) {
        Z_CHECK(_send_message_info_by_id.remove(message.messageId()));
    }
    _send_message_mutex.unlock();

    return result;
}

bool MessageDispatcher::postMessage(const I_ObjectExtension* sender, const QList<const I_ObjectExtension*>& receivers,
                                    const Message& message)
{
    QMutexLocker lock(&_mutex);

    auto info = objectInfo(sender);
    if (info == nullptr)
        return false;

    QList<const I_ObjectExtension*> receivers_prepered;
    for (auto const& r : receivers) {
        if (r == nullptr || !isObjectRegistered(r))
            continue;
        receivers_prepered << r;
    }

    if (receivers_prepered.isEmpty())
        return false;

    putMessageToBuffer(info, MessageChannel(), receivers_prepered, message);
    return true;
}

MessageID MessageDispatcher::postMessage(const I_ObjectExtension* sender, const Uid& receiver, int message_code,
                                         const MessageID& feedback_message_id, const DataContainerList& data)
{
    auto msg = Message(receiver.entityCode(), message_code, feedback_message_id, data);
    bool res = postMessage(sender, receiver, msg);
    return res ? msg.messageId() : MessageID();
}

Message MessageDispatcher::sendMessage(const I_ObjectExtension* receiver, const Message& message, int timeout_ms, bool force)
{
    QList<Message> result = sendMessages(receiver, QList<Message> {message}, timeout_ms, force);
    Z_CHECK(result.count() == 1);
    if (!result.constBegin()->isValid())
        return ErrorMessage(message.messageId(), Error::timeoutError());
    else
        return *result.constBegin();
}

bool MessageDispatcher::isChannelRegistered(const MessageChannel& id) const
{
    QMutexLocker lock(&_mutex);

    return channelInfo(id) != nullptr;
}

bool MessageDispatcher::registerChannel(const MessageChannel& id)
{
    QMutexLocker lock(&_mutex);

    if (isChannelRegistered(id))
        return false;

    auto info = Z_MAKE_SHARED(ChannelInfo);
    info->id = id;
    _channels << info;

    return true;
}

bool MessageDispatcher::hasSubscribers(const MessageChannel& channel) const
{
    QMutexLocker lock(&_mutex);
    if (!isChannelRegistered(channel))
        return false;

    auto c_info = channelInfo(channel);
    Z_CHECK_NULL(c_info);
    return !c_info->subscribe.isEmpty();
}

QList<SubscribeHandle> MessageDispatcher::subscribeHandles(const MessageChannel& channel, const I_ObjectExtension* subscriber) const
{
    QMutexLocker lock(&_mutex);

    Z_CHECK_NULL(subscriber);
    if (!isChannelRegistered(channel))
        return QList<SubscribeHandle>();

    auto c_info = channelInfo(channel);
    Z_CHECK_NULL(c_info);

    auto o_info = objectInfo(subscriber);
    if (o_info == nullptr)
        return QList<SubscribeHandle>();

    auto s_infos = c_info->subscribe_by_object.values(subscriber);
    QList<SubscribeHandle> res;
    for (auto& s : qAsConst(s_infos)) {
        res << s->handle;
    }

    return res;
}

SubscribeHandle MessageDispatcher::subscribe(const MessageChannel& channel, const I_ObjectExtension* subscriber,
                                             const EntityCodeList& contains_types, const QList<Uid>& contains,
                                             const EntityCodeList& sender_types, const QList<Uid>& senders)
{
    auto h = subscribe(QList<MessageChannel> {channel}, subscriber, contains_types, contains, sender_types, senders);
    return h.isEmpty() ? 0 : h.constFirst();
}

QList<SubscribeHandle> MessageDispatcher::subscribe(const QList<MessageChannel>& channels, const I_ObjectExtension* subscriber,
                                                    const EntityCodeList& contains_types, const QList<Uid>& contains,
                                                    const EntityCodeList& sender_types, const QList<Uid>& senders)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK_NULL(subscriber);
    if (!isObjectRegistered(subscriber))
        return QList<SubscribeHandle>();

    QList<SubscribeHandle> result;
    for (const auto& channel : channels) {
        if (!isChannelRegistered(channel)) {
            for (auto& h : result) {
                unSubscribe(h);
            }
            return QList<SubscribeHandle>();
        }

        auto c_info = channelInfo(channel);
        Z_CHECK_NULL(c_info);

        auto o_info = objectInfo(subscriber);
        Z_CHECK_NULL(o_info);

        auto s_info = Z_MAKE_SHARED(SubscribeInfo);

        s_info->channel_info = c_info;
        s_info->object_info = o_info;
        s_info->contains_types = contains_types;
        s_info->contains = contains;
        s_info->sender_types = sender_types;
        s_info->senders = senders;
        s_info->any_message = (contains_types.isEmpty() && contains.isEmpty() && sender_types.isEmpty() && senders.isEmpty());

        s_info->handle = Z_NEW(SubscribeHandle__);
        c_info->subscribe << s_info;
        c_info->subscribe_by_object.insert(subscriber, s_info);

        result << s_info->handle;
        _subscribe_info[s_info->handle] = s_info;
    }

    return result;
}

bool MessageDispatcher::unSubscribe(SubscribeHandle subscribe_handle)
{
    return unSubscribe(QList<SubscribeHandle> {subscribe_handle});
}

bool MessageDispatcher::unSubscribe(const QList<SubscribeHandle>& subscribe_handles)
{
    QMutexLocker lock(&_mutex);

    bool result = true;
    for (auto h : subscribe_handles) {
        auto s_info = _subscribe_info.value(h);
        if (s_info == nullptr) {
            result = false;
            continue;
        }
        Z_CHECK(!s_info->channel_info.expired());
        auto c_info = s_info->channel_info.lock();
        Z_CHECK(c_info->subscribe.removeOne(s_info));
        Z_CHECK(c_info->subscribe_by_object.remove(s_info->object_info->object_prt, s_info) == 1);

        for (int i = _buffer.count() - 1; i >= 0; i--) {
            if (_buffer.at(i)->subcribe_handle == h) {
                _buffer.removeAt(i);
            }
        }

        Z_CHECK(_subscribe_info.remove(h));
        Z_DELETE(h);
    }

    return result;
}

void MessageDispatcher::unSubscribe(const I_ObjectExtension* subscriber)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK_NULL(subscriber);
    for (auto& c : qAsConst(_channels)) {
        auto subs = c->subscribe_by_object.values(subscriber);
        for (auto& s : qAsConst(subs)) {
            unSubscribe(s->handle);
        }
    }
}

bool MessageDispatcher::postMessageToChannel(
    const MessageChannel& channel, const Uid& sender, const Message& message)
{
    return postMessageToChannel(QList<MessageChannel> {channel}, sender, message);
}

bool MessageDispatcher::postMessageToChannel(
    const QList<MessageChannel>& channels, const Uid& sender, const Message& message)
{
    QMutexLocker lock(&_mutex);

    Z_CHECK(message.isValid());

    auto info = objectInfo(sender);
    if (info.isEmpty())
        return false;

    bool result = true;
    for (const auto& channel : channels) {
        if (!isChannelRegistered(channel) || blockedChanelMessageTypes(channel).contains(message.messageType())) {
            result = false;
            continue;
        }

        auto c_info = channelInfo(channel);
        Z_CHECK_NULL(c_info);

        putMessageToBuffer(info.at(0), c_info->id, {}, message);
    }
    return result;
}

bool MessageDispatcher::postMessageToChannel(const MessageChannel& channel, const Message& message)
{
    return postMessageToChannel(channel, CoreUids::CORE, message);
}

bool MessageDispatcher::postMessageToChannel(const QList<MessageChannel>& channels, const Message& message)
{
    return postMessageToChannel(channels, CoreUids::CORE, message);
}

void MessageDispatcher::stop()
{
    QMutexLocker lock(&_mutex);

    _stop_count++;
    if (_stop_count == 1) {        
        _buffer_callback_timer.stop();        
    }
}

void MessageDispatcher::start()
{
    QMutexLocker lock(&_mutex);

    if (_stop_count == 0) {
        Core::logError("MessageDispatcher::start counter error");
        return;
    }

    _stop_count--;
    if (_stop_count == 0) {
        if (!_buffer.isEmpty())
            _buffer_callback_timer.start();        
    }
}

bool MessageDispatcher::isStarted() const
{
    QMutexLocker lock(&_mutex);
    return _stop_count > 0;
}

void MessageDispatcher::setEnabledReceivers(const UidList& entity_uids)
{
    QMutexLocker lock(&_mutex);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    auto set = QSet(entity_uids.begin(), entity_uids.end());
#else
    auto set = entity_uids.toSet();
#endif

    if (_enabled_receivers == set)
        return;

    Z_CHECK(set.isEmpty() || _enabled_receivers.isEmpty());

    _enabled_receivers = set;

    if (!_buffer.isEmpty())
        _buffer_callback_timer.start();    
}

void MessageDispatcher::blockChannelMessageType(const MessageChannel& channel_id, MessageType t)
{
    Z_CHECK(t != MessageType::Invalid);
    QMutexLocker lock(&_mutex);

    auto info = channelInfo(channel_id);
    Z_CHECK_NULL(info);
    auto blocked = info->blocked_types.constFind(t);
    info->blocked_types[t] = (blocked == info->blocked_types.constEnd() ? 1 : blocked.value() + 1);
}

int MessageDispatcher::unBlockChannelMessageType(const MessageChannel& channel_id, MessageType t)
{
    Z_CHECK(t != MessageType::Invalid);
    QMutexLocker lock(&_mutex);

    auto info = channelInfo(channel_id);
    Z_CHECK_NULL(info);
    auto blocked = info->blocked_types.constFind(t);
    Z_CHECK(blocked != info->blocked_types.constEnd());
    int counter = blocked.value() - 1;
    if (counter > 0)
        info->blocked_types[t] = counter;
    else
        info->blocked_types.remove(t);

    return counter;
}

QList<MessageType> MessageDispatcher::blockedChanelMessageTypes(const MessageChannel& channel_id) const
{
    QMutexLocker lock(&_mutex);

    auto info = channelInfo(channel_id);
    if (info == nullptr)
        return {};

    return info->blocked_types.keys();
}

void MessageDispatcher::postDebugMessage(const QString& sender, const QString& receiver, const MessagePtr& message,
                                         SubscribeHandle subscribe_handle)
{
    Z_CHECK_NULL(message);

    if (!hasSubscribers(CoreChannels::MESSAGE_DEBUG))
        return;

    QVariantList debug_text;
    debug_text << sender;
    debug_text << receiver;

    debug_text << message->toPrintable(true, false);
    debug_text << message->messageId().toString();
    debug_text << message->feedbackMessageId().toString();

    debug_text << (reinterpret_cast<qintptr>(subscribe_handle) > 0 ? QVariant(reinterpret_cast<qintptr>(subscribe_handle)) : QVariant());

    postMessageToChannel(CoreChannels::MESSAGE_DEBUG, CoreUids::MESSAGE_DISPATCHER, VariantListMessage(debug_text));
}

void MessageDispatcher::sl_bufferCallbackTimeout()
{
    QMutexLocker lock(&_mutex);

    if (_stop_count > 0)
        return;

    processBuffer(QSet<MessageID>());
}

void MessageDispatcher::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)
    Z_CHECK(message.isValid());

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    _send_message_mutex.lock();
    auto send_message_info = _send_message_info_by_id.value(message.feedbackMessageId());
    _send_message_mutex.unlock();

    if (send_message_info != nullptr) {
        send_message_info->message_data[message.feedbackMessageId()] = message;
        send_message_info->received++;
        if (send_message_info->received == send_message_info->message_data.count())
            send_message_info->send_message_loop.quit();
    }
}

void MessageDispatcher::forceMessage(const MessageID& message_id)
{
    processBuffer(QSet<MessageID>({message_id}));
}

QList<std::shared_ptr<MessageDispatcher::ObjectInfo>> MessageDispatcher::objectInfo(const Uid& id) const
{
    return _objects_by_id.values(id);
}

std::shared_ptr<MessageDispatcher::ObjectInfo> MessageDispatcher::objectInfo(const I_ObjectExtension* object) const
{
    Z_CHECK_NULL(object);
    return _objects_by_ptr.value(const_cast<I_ObjectExtension*>(object));
}

std::shared_ptr<MessageDispatcher::ChannelInfo> MessageDispatcher::channelInfo(MessageChannel id) const
{
    for (auto c : _channels) {
        if (c->id == id)
            return c;
    }

    return std::shared_ptr<MessageDispatcher::ChannelInfo>();
}

void MessageDispatcher::processBuffer(const QSet<MessageID>& force_messages)
{
    if (_stop_count > 0 && force_messages.isEmpty())
        return;

    MessagePauseHelper ph;
    ph.pause();

    int count = 0;
    int not_enabled_count = 0;
    while (!_buffer.isEmpty() && count < Framework::MESSAGE_DISPATCHER_ONE_STEP) {
        // здесь нельзя делать processEvents!
        if (!_enabled_receivers.isEmpty()) {
            if (!_enabled_receivers.contains(_buffer.first()->reciever_info->id)) {
                not_enabled_count++;
                continue;
            }
        }

        auto b_info = _buffer.dequeue();        
        if (force_messages.isEmpty() || force_messages.contains(b_info->message->messageId()))
            enqueueMessage(b_info->sender_info, b_info->reciever_info, b_info->message, b_info->subcribe_handle);

        count++;
    }

    if (_buffer.count() > not_enabled_count)
        _buffer_callback_timer.start();

    processMessageQueue();
}

QString MessageDispatcher::getObjectDescription(const Uid& uid, const I_ObjectExtension* obj)
{
    QString class_name;
    QString owner_name;
    if (obj != nullptr) {
        auto qobj = dynamic_cast<const QObject*>(obj);
        Z_CHECK_NULL(qobj);
        class_name = qobj->metaObject()->className();
        owner_name = qobj->property(OWNER_NAME_PROPERTY).toString();
    } else {
        owner_name = QStringLiteral("deleted");
    }

    QString info = uid.toPrintable();

    if (!class_name.isEmpty())
        info += QStringLiteral(", ") + class_name;
    if (!owner_name.isEmpty())
        info += QStringLiteral(", ") + owner_name;

    return info;
}

void MessageDispatcher::putMessageToBuffer(const ObjectInfoPtr& sender, const MessageChannel& channel,
                                           const QList<const I_ObjectExtension*>& receivers, const Message& message)
{
    Z_CHECK_NULL(sender);

    if (!Utils::isMainThread()) {
        auto ptr = MessagePtr(message.clone());
        Z_CHECK(QMetaObject::invokeMethod(this, "putMessageToBuffer_thread", Qt::QueuedConnection,
                                          Q_ARG(const zf::I_ObjectExtension*, sender->object_prt), Q_ARG(zf::MessageChannel, channel),
                                          Q_ARG(QList<const zf::I_ObjectExtension*>, receivers), Q_ARG(zf::MessagePtr, ptr)));
        return;
    }

    QList<ObjectInfoPtr> receiver_info_list;
    QList<SubscribeHandle> handles;

    ChannelInfoPtr c_info;
    if (channel.isValid()) {
        c_info = channelInfo(channel);
        Z_CHECK_NULL(c_info);
        for (auto& s_info : c_info->subscribe) {
            if (!s_info->any_message) {
                bool has_entity_types = !s_info->contains_types.isEmpty() && message.contains(s_info->contains_types);
                bool has_entity_uids = has_entity_types || (!s_info->contains.isEmpty() && message.contains(s_info->contains));

                // есть требования по наличию типа или идентификатора, но они не найдены
                if (!has_entity_types && !has_entity_uids && (!s_info->contains_types.isEmpty() || !s_info->contains.isEmpty()))
                    continue;

                bool has_sender_types = !s_info->sender_types.isEmpty() && s_info->sender_types.contains(sender->id.entityCode());
                bool has_sender_uids = has_sender_types || (!s_info->senders.isEmpty() && s_info->senders.contains(sender->id));

                // есть требования по наличию типа или идентификатора, но они не найдены
                if (!has_sender_types && !has_sender_uids && (!s_info->sender_types.isEmpty() || !s_info->senders.isEmpty()))
                    continue;
            }

            receiver_info_list << s_info->object_info;
            handles << s_info->handle;
        }

    } else {
        for (auto o : receivers) {
            auto p = objectInfo(o);
            Z_CHECK_NULL(p);
            receiver_info_list << p;
        }
    }

    for (int i = 0; i < receiver_info_list.count(); i++) {
        auto receiver_info = receiver_info_list.at(i);

        auto b_info = Z_MAKE_SHARED(BufferInfo);
        b_info->reciever_info = receiver_info;

        b_info->channel_info = c_info;
        if (c_info != nullptr)
            b_info->subcribe_handle = handles.at(i);

        b_info->sender_info = sender;
        b_info->message = MessagePtr(message.clone());

        _buffer.enqueue(b_info);
    }

    if (!receiver_info_list.isEmpty())
        _buffer_callback_timer.start();
}

void MessageDispatcher::putMessageToBuffer_thread(const I_ObjectExtension* sender, const MessageChannel& channel,
                                                  const QList<const I_ObjectExtension*>& receivers, const MessagePtr& message)
{
    QMutexLocker lock(&_mutex);

    auto info = objectInfo(sender);
    if (info == nullptr)
        return;

    QList<const I_ObjectExtension*> receivers_prepared;
    for (auto r : receivers) {
        if (isObjectRegistered(r))
            receivers_prepared << r;
    }

    putMessageToBuffer(info, channel, receivers_prepared, *message);
}

void MessageDispatcher::enqueueMessage(const ObjectInfoPtr& sender, std::shared_ptr<MessageDispatcher::ObjectInfo>& receiver_info,
                                       const MessagePtr& message, zf::SubscribeHandle subscribe_handle)
{
    auto item = Z_MAKE_SHARED(MessageQueueInfo);
    item->sender = sender;    
    item->receiver_info = receiver_info;
    item->message = message;
    item->subscribe_handle = subscribe_handle;
    _message_queue.enqueue(item);
}

void MessageDispatcher::processMessageQueue()
{
    while (!_message_queue.isEmpty()) {
        auto item = _message_queue.dequeue();
        postMessageHelper(item->sender, item->receiver_info, item->message, item->subscribe_handle);
    }
}

void MessageDispatcher::postMessageHelper(const ObjectInfoPtr& sender, ObjectInfoPtr& receiver_info, const MessagePtr& message,
                                          zf::SubscribeHandle subscribe_handle)
{
    Z_CHECK_NULL(sender);
    Z_CHECK_NULL(receiver_info);

    if (message->messageType() != MessageType::General && receiver_info->id != CoreUids::SHELL_DEBUG
        && isChannelRegistered(CoreChannels::MESSAGE_DEBUG)) {
        postDebugMessage(getObjectDescription(sender->id, sender->object_prt),
                         getObjectDescription(receiver_info->id, receiver_info->object_prt), message, subscribe_handle);
    }

    _not_delivered.insert(message->messageId(), receiver_info->object_prt);

    auto qobj = dynamic_cast<QObject*>(receiver_info->object_prt);
    Z_CHECK_NULL(qobj);

    if (!QMetaObject::invokeMethod(qobj, receiver_info->slot.constData(), Qt::QueuedConnection, Q_ARG(zf::Uid, sender->id),
                                   Q_ARG(zf::Message, *message.get()), Q_ARG(zf::SubscribeHandle, subscribe_handle))) {
        Z_HALT(QString("Message inbound slot %1 not found. Object: %2, uid: %3")
                   .arg(receiver_info->slot.constData())
                   .arg(qobj->objectName().isEmpty() ? QString(qobj->metaObject()->className())
                                                     : QString(qobj->metaObject()->className()) + ":" + qobj->objectName())
                   .arg(receiver_info->id.toPrintable()));
    }

    if (!receiver_info->slot_advanced.isEmpty()) {
        if (!QMetaObject::invokeMethod(qobj, receiver_info->slot_advanced.constData(), Qt::QueuedConnection,
                                       Q_ARG(const zf::I_ObjectExtension*, sender->object_prt), Q_ARG(zf::Uid, sender->id),
                                       Q_ARG(zf::Message, *message.get()), Q_ARG(zf::SubscribeHandle, subscribe_handle))) {
            Z_HALT(QString("Message inbound slot %1 not found. Object: %2, uid: %3")
                       .arg(receiver_info->slot_advanced.constData())
                       .arg(qobj->objectName().isEmpty() ? QString(qobj->metaObject()->className())
                                                         : QString(qobj->metaObject()->className()) + ":" + qobj->objectName())
                       .arg(receiver_info->id.toPrintable()));
        }
    }
}

MessageDispatcher::SubscribeInfo::~SubscribeInfo()
{
}

void MessageDispatcher::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant MessageDispatcher::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool MessageDispatcher::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void MessageDispatcher::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void MessageDispatcher::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void MessageDispatcher::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    QMutexLocker lock(&_mutex);

    _object_extension->objectExtensionDeleteInfoExternal(i);

    if (_deleted || !Core::isBootstraped())
        return;

    Z_CHECK(unRegisterObject(i));
}

void MessageDispatcher::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void MessageDispatcher::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

} // namespace zf
