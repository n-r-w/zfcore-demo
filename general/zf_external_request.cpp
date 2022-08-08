#include "zf_external_request.h"
#include "zf_core.h"
#include "zf_core_uids.h"

namespace zf
{
ExternalRequestMessage::ExternalRequestMessage(const Message& m)
    : zf::Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == zf::MessageType::General));
}
ExternalRequestMessage::ExternalRequestMessage(const ExternalRequestMessage& m)
    : zf::Message(m)
{
}

ExternalRequestMessage::ExternalRequestMessage(const MessageID& feedback_id, const zf::MessageCode& service_message_code,
                                               const QList<QPair<Uid, QVariant>>& result, const QList<QMap<QString, QVariant>>& attributes)
    : zf::Message(Message::create(zf::MessageType::General, service_message_code, feedback_id,
                                  QVariantList {QVariant::fromValue(result), QVariant::fromValue(attributes)}))
{
}

QList<QPair<Uid, QVariant>> ExternalRequestMessage::result() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QList<QPair<Uid, QVariant>>>() : QList<QPair<Uid, QVariant>>();
}

QList<QMap<QString, QVariant>> ExternalRequestMessage::attributes() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

MessageID ExternalRequester::requestLookup(I_ExternalRequest* request_interface, const Uid& key, const QString& request_type)
{
    Z_CHECK_NULL(request_interface);
    auto msg_id = request_interface->requestLookup(this, key, request_type);
    _requests << msg_id;
    return msg_id;
}

MessageID ExternalRequester::requestLookup(I_ExternalRequest* request_interface, const QString& text, const UidList& parent_keys,
                                           const QString& request_type)
{
    Z_CHECK_NULL(request_interface);
    auto msg_id = request_interface->requestLookup(this, text, parent_keys, request_type);
    _requests << msg_id;
    return msg_id;
}

void ExternalRequester::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (!_requests.contains(message.feedbackMessageId()))
        return;

    _requests.remove(message.feedbackMessageId());

    Error error;
    QList<QPair<zf::Uid, QVariant>> result;
    QList<QMap<QString, QVariant>> attributes;
    if (message.messageType() == MessageType::Error) {
        error = ErrorMessage(message).error();

    } else {
        auto m = ExternalRequestMessage(message);
        result = m.result();
        attributes = m.attributes();
    }

    emit sg_feedback(message.feedbackMessageId(), result, attributes, error);
}

ExternalRequester::ExternalRequester(QObject* parent)
    : QObject(parent)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::EXTERNAL_REQUESTER, this, QStringLiteral("sl_message_dispatcher_inbound")));
}

ExternalRequester::~ExternalRequester()
{
    delete _object_extension;
}

bool ExternalRequester::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void ExternalRequester::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant ExternalRequester::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool ExternalRequester::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void ExternalRequester::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void ExternalRequester::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void ExternalRequester::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void ExternalRequester::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void ExternalRequester::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

}; // namespace zf
