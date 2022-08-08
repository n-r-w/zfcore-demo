#include "zf_message_processor.h"
#include "zf_core.h"
#include "zf_database_messages.h"
#include "zf_framework.h"

namespace zf
{
MessageProcessor::MessageProcessor(const I_ObjectExtension* object, CallbackManager* callback_manager)
    : QObject()
    , _object(object)
    , _entity_uid(Core::messageDispatcher()->objectUid(object))
    , _callback_manager(callback_manager)
    , _object_extension(new ObjectExtension(this))

{
    Z_CHECK_NULL(object);
    Z_CHECK(_entity_uid.isValid());
    Z_CHECK_NULL(callback_manager);

    Z_CHECK(Core::messageDispatcher()->registerObject(_entity_uid, this, QStringLiteral("sl_inbound_message")));
    callback_manager->registerObject(this, QStringLiteral("sl_callback"));
}

MessageProcessor::~MessageProcessor()
{
    delete _object_extension;
}

bool MessageProcessor::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

bool MessageProcessor::addMessageRequest(const Command& request_key, const std::shared_ptr<void>& custom_data,
                                         const I_ObjectExtension* receiver, const Message& message, const QList<Command> queue_request_keys)
{
    Z_CHECK(message.isValid());
    Z_CHECK(!queue_request_keys.contains(request_key));

    if (!Core::messageDispatcher()->isObjectRegistered(receiver))
        return false;

    if (!isWaitingMessageFeedback(request_key))
        emit sg_startWaitingFeedback(request_key);

    auto q_info = Z_MAKE_SHARED(MessageQueueInfo);
    q_info->request_key = request_key;
    q_info->data = custom_data;
    q_info->receiver = receiver;
    q_info->message = std::unique_ptr<Message>(message.clone());
    q_info->queue_request_keys = queue_request_keys;

    bool is_new = !_message_queue.contains(request_key);
    _message_queue[request_key] = q_info;

    _callback_manager->addRequest(this, Framework::MESSAGE_PROCESSOR_CALLBACK_KEY);

    if (is_new)
        emit sg_requestAdded(request_key);

    return true;
}

void MessageProcessor::removeMessageRequest(const Command& key)
{
    if (_message_queue.contains(key)) {
        _message_queue.remove(key);
        _messages.remove(key);
        _message_id.remove(_message_id.key(key));

        emit sg_requestRemoved(key);
    }
}

bool MessageProcessor::isWaitingMessageFeedback(const Command& key) const
{
    return _message_queue.contains(key) || _messages.contains(key);
}

bool MessageProcessor::inQueue(const Command& request_key) const
{
    return _message_queue.contains(request_key);
}

void MessageProcessor::sl_inbound_message(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)

    if (!Core::isBootstraped() || _object_extension->objectExtensionDestroyed())
        return;

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (!_message_id.contains(message.feedbackMessageId()))
        return;

    Command request_key = _message_id.value(message.feedbackMessageId());

    auto info = _messages.value(request_key);
    Z_CHECK(info != nullptr);

    _message_id.remove(message.feedbackMessageId());
    _messages.remove(request_key);

    emit sg_feedback(request_key, info->data, *info->message.get(), message);

    _callback_manager->addRequest(this, Framework::MESSAGE_PROCESSOR_CALLBACK_KEY);

    emit sg_finishWaitingFeedback(request_key);

    emit sg_requestRemoved(request_key);
}

void MessageProcessor::sl_callback(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (!Core::isBootstraped() || _object_extension->objectExtensionDestroyed())
        return;

    if (key == Framework::MESSAGE_PROCESSOR_CALLBACK_KEY)
        processMessageQueue();
}

bool MessageProcessor::processMessageQueue()
{
    if (!Core::isBootstraped())
        return true;

    QList<std::shared_ptr<MessageQueueInfo>> to_process;

    for (auto& q : _message_queue) {
        // определяем можно ли обрабатывать элемент очереди
        bool can_process = true;
        for (const auto &request_key : qAsConst(q->queue_request_keys)) {
            if (!_message_queue.contains(request_key))
                continue;
            can_process = false;
            break;
        }

        if (can_process)
            to_process << q;
    }

    bool ok = true;
    for (auto& q : to_process) {
        _message_queue.remove(q->request_key);

        ok = ok && Core::messageDispatcher()->postMessage(this, q->receiver, *q->message.get());

        _messages[q->request_key] = q;

        MessageID msg_id = _message_id.key(q->request_key);
        if (msg_id.isValid())
            _message_id.remove(msg_id);

        _message_id[q->message.get()->messageId()] = q->request_key;
    }

    return ok;
}

void MessageProcessor::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant MessageProcessor::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool MessageProcessor::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void MessageProcessor::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void MessageProcessor::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void MessageProcessor::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void MessageProcessor::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void MessageProcessor::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

} // namespace zf
