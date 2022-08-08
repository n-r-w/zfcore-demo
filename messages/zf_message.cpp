#include "zf_message.h"
#include "zf_core.h"
#include "zf_logging.h"
#include "zf_framework.h"
#include "translation/zf_translation.h"
#include "zf_data_container.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>

//! Версия структуры стрима
static int _MESSAGE_STREAM_VERSION = 2;

namespace zf
{
//! Данные для Message
class Message_SharedData : public QSharedData
{
public:
    Message_SharedData();
    Message_SharedData(const Message_SharedData& d);
    virtual ~Message_SharedData();

    void copyFrom(const Message_SharedData* d)
    {
        type = d->type;
        code = d->code;
        id = d->id;
        data = d->data;
        feedback_message_id = d->feedback_message_id;
    }

    MessageType type = MessageType::Invalid;
    MessageCode code;
    MessageID id;
    MessageID feedback_message_id;
    DataContainerList data;
};

Message_SharedData::Message_SharedData()
{
    Z_DEBUG_NEW("Message_SharedData");
}

Message_SharedData::Message_SharedData(const Message_SharedData& d)
    : QSharedData(d)
{
    copyFrom(&d);
    Z_DEBUG_NEW("Message_SharedData");
}

Message_SharedData::~Message_SharedData()
{
    Z_DEBUG_DELETE("Message_SharedData");
}

Message::Message()
    : _d(new Message_SharedData())
{
}

Message::Message(const Message& m)
    : _d(m._d)
{
}

Message& Message::operator=(const Message& m)
{
    if (this != &m) {
        _d = m._d;
        _contains.reset();
    }
    return *this;
}

Message::Message(MessageType type, const MessageCode& message_code, const MessageID& feedback_message_id, const DataContainerList& data)
    : Message(type, message_code, feedback_message_id, data, MessageID::generate())
{
}

Message::Message(const MessageCode& message_code, const MessageID& feedback_message_id, const DataContainerList& data)
    : Message(MessageType::General, message_code, feedback_message_id, data)
{
}

Message::Message(const EntityCode& entity_code, int message_code, const MessageID& feedback_message_id, const DataContainerList& data)
    : Message(MessageType::General, MessageCode(entity_code, message_code), feedback_message_id, data)
{
}

MessageUidContainsHelper* Message::containsHelper() const
{
    auto self = const_cast<Message*>(this);
    if (self->_contains == nullptr)
        self->_contains = std::make_unique<MessageUidContainsHelper>(self);

    return self->_contains.get();
}

Message::~Message()
{
}

Message* Message::clone() const
{
    return new Message(*this);
}

bool Message::operator==(const Message& m) const
{
    if (this == &m)
        return true;

    return _d->id == m._d->id;
}

bool Message::operator!=(const Message& m) const
{
    return !operator==(m);
}

bool Message::operator<(const Message& m) const
{
    return messageId() < m.messageId();
}

void Message::clear()
{
    *this = Message();
}

bool Message::isValid() const
{
    return _d->type != MessageType::Invalid;
}

bool Message::isEmpty() const
{
    return _d->data.isEmpty();
}

MessageID Message::messageId() const
{
    return _d->id;
}

MessageType Message::messageType() const
{
    return _d->type;
}

QString Message::messageTypeName() const
{
    return Utils::messageTypeName(messageType());
}

MessageID Message::feedbackMessageId() const
{
    return _d->feedback_message_id;
}

QString Message::toPrintable(bool show_type, bool show_id) const
{
    QString text;
    bool not_processed = false;
    switch (messageType()) {
        case MessageType::Invalid:
        case MessageType::PropertyTable:
        case MessageType::Confirm:
        case MessageType::IntTable:
        case MessageType::DBCommandQuery:
        case MessageType::DBCommandGetConnectionInformation:
        case MessageType::DBCommandGetPropertyTable:
        case MessageType::DBCommandGetAccessRights:
        case MessageType::DBEventPropertyTable:
        case MessageType::DBEventQueryFeedback:
        case MessageType::DBEventConnectionInformation:
        case MessageType::DBEventAccessRights:
        case MessageType::MMEventCatalogValue:
        case MessageType::DBCommandGenerateReport:
        case MessageType::DBEventReport:
        case MessageType::DBEventCatalogInfo:
        case MessageType::DBCommandLogin:
        case MessageType::DBEventLogin:
        case MessageType::DBCommandGetDataTable:
        case MessageType::DBEventDataTable:
        case MessageType::DBCommandGetDataTableInfo:
        case MessageType::DBEventDataTableInfo:
        case MessageType::DBCommandReconnect:
        case MessageType::DBEventInformation:
        case MessageType::DBEventConnectionDone:
        case MessageType::DBEventInitLoadDone:
            not_processed = true;
            break;

        case MessageType::Error:
            text = ErrorMessage(*this).error().fullText();
            break;
        case MessageType::General:
            text = messageCode().toPrintable();
            break;

        case MessageType::VariantList:
            text = VariantListMessage(*this).dataToText();
            break;
        case MessageType::IntList:
            text = IntListMessage(*this).dataToText();
            break;
        case MessageType::Progress:
            text = ProgressMessage(*this).dataToText();
            break;
        case MessageType::StringList:
            text = StringListMessage(*this).dataToText();
            break;
        case MessageType::ByteArrayList:
            text = ByteArrayListMessage(*this).dataToText();
            break;
        case MessageType::Bool:
            text = BoolMessage(*this).dataToText();
            break;
        case MessageType::VariantMap:
            text = VariantMapMessage(*this).dataToText();
            break;

        case MessageType::DBCommandIsEntityExists:
            text = DBCommandIsEntityExistsMessage(*this).dataToText();
            break;
        case MessageType::DBCommandGetEntity:
            text = DBCommandGetEntityMessage(*this).dataToText();
            break;
        case MessageType::DBCommandWriteEntity:
            text = DBCommandWriteEntityMessage(*this).dataToText();
            break;
        case MessageType::DBCommandRemoveEntity:
            text = DBCommandRemoveEntityMessage(*this).dataToText();
            break;
        case MessageType::DBCommandGetEntityList:
            text = DBCommandGetEntityListMessage(*this).entityCode().string();
            break;
        case MessageType::DBCommandUpdateEntities:
            text = DBCommandUpdateEntitiesMessage(*this).dataToText();
            break;

        case MessageType::DBEventEntityLoaded:
            text = DBEventEntityLoadedMessage(*this).dataToText();
            break;
        case MessageType::DBEventEntityExists:
            text = DBEventEntityExistsMessage(*this).dataToText();
            break;
        case MessageType::DBEventEntityList:
            text = DBEventEntityListMessage(*this).dataToText();
            break;
        case MessageType::DBEventEntityChanged:
            text = DBEventEntityChangedMessage(*this).dataToText();
            break;
        case MessageType::DBEventEntityRemoved:
            text = DBEventEntityRemovedMessage(*this).dataToText();
            break;
        case MessageType::DBEventEntityCreated:
            text = DBEventEntityCreatedMessage(*this).dataToText();
            break;     
        case MessageType::MMCommandGetModels:
            text = MMCommandGetModelsMessage(*this).dataToText();
            break;
        case MessageType::MMCommandRemoveModels:
            text = MMCommandRemoveModelsMessage(*this).dataToText();
            break;
        case MessageType::MMCommandGetEntityNames:
            text = MMCommandGetEntityNamesMessage(*this).dataToText();
            break;
        case MessageType::MMCommandGetCatalogValue:
            text = MMCommandGetCatalogValueMessage(*this).dataToText();
            break;
        case MessageType::MMEventModelList:
            text = MMEventModelListMessage(*this).dataToText();
            break;
        case MessageType::MMEventEntityNames:
            text = MMEventEntityNamesMessage(*this).dataToText();
            break;
        case MessageType::ModelInvalide:
            text = ModelInvalideMessage(*this).dataToText();
            break;
        case MessageType::DBCommandGetCatalogInfo:
            text = DBCommandGetCatalogInfoMessage(*this).catalogUid().toPrintable();
            break;
    }

    if (text.isEmpty() && !not_processed)
        Z_HALT_INT; // пропущен case

    QStringList res;
    if (show_type)
        res << messageTypeName();
    if (!text.isEmpty())
        res << text;
    if (show_id)
        res << QStringLiteral("id: ") + messageId().toString();

    return res.join("; ");
}

void Message::debPrint() const
{
    Core::debPrint(variant());
}

MessageCode Message::messageCode() const
{
    return _d->code;
}

const DataContainerList& Message::rawData() const
{
    return _d->data;
}

QVariant Message::variant() const
{
    return QVariant::fromValue(*this);
}

Message Message::fromVariant(const QVariant& v)
{
    return v.value<Message>();
}

bool Message::contains(const EntityCode& entity_code) const
{
    return containsHelper()->contains(entity_code);
}

bool Message::contains(const Uid& entity_uid) const
{
    return containsHelper()->contains(entity_uid);
}

bool Message::contains(const EntityCodeList& entity_codes) const
{
    for (auto& c : entity_codes) {
        if (contains(c))
            return true;
    }
    return false;
}

bool Message::contains(const UidList& entity_uids) const
{
    for (const Uid& u : entity_uids) {
        if (contains(u))
            return true;
    }
    return false;
}

int Message::count_I_MessageContainsUid() const
{
    return 0;
}

Uid Message::value_I_MessageContainsUid(int index) const
{
    Q_UNUSED(index)
    Z_HALT_INT;
    return Uid();
}

int Message::count_I_MessageContainsUidCodes() const
{
    return 0;
}

EntityCode Message::value_I_MessageContainsUidCodes(int index) const
{
    Q_UNUSED(index)
    Z_HALT_INT;
    return EntityCode();
}

Message Message::create(MessageType message_type, const MessageCode& message_code, const MessageID& feedback_message_id,
                        const DataContainerList& data)
{
    return Message(message_type, message_code, feedback_message_id, data);
}

Message Message::create(MessageType message_type, const MessageCode& message_code, const MessageID& feedback_message_id,
                        const DataContainer& data)
{
    return create(message_type, message_code, feedback_message_id, DataContainerList {data});
}

Message Message::create(MessageType type, const MessageCode& message_code, const MessageID& feedback_message_id, const QVariantList& data)
{
    return Message(
        type, message_code, feedback_message_id, {DataContainer::createVariantProperties({QVariant::fromValue(data)})});
}

QVariantList Message::variantList() const
{
    return isValid() && !isEmpty() && rawData().at(0).contains(PropertyID::def())
               ? rawData().at(0).value(PropertyID::def()).value<QVariantList>()
               : QVariantList();
}

Message& Message::setMessageId(const MessageID& id)
{
    _d->id = id;
    return *this;
}

QByteArray Message::toByteArray() const
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setVersion(Consts::DATASTREAM_VERSION);
    ds << *this;
    return qCompress(data);
}

Message Message::fromByteArray(const QByteArray& data, Error& error)
{
    error.clear();

    QByteArray uncompressed = qUncompress(data);

    if (!isSupportedStreamVersion(uncompressed, false)) {
        error = Error(ZF_TR(ZFT_WRONG_PROTOCOL_VERSION));
        return Message();
    }

    QDataStream ds(uncompressed);
    ds.setVersion(Consts::DATASTREAM_VERSION);
    Message message;
    ds >> message;
    if (ds.status() != QDataStream::Ok) {
        error = Error::corruptedDataError("Message::fromByteArray corrupted");
        return Message();
    }
    return message;
}

bool Message::isSupportedStreamVersion(const QByteArray& data, bool compressed)
{
    QDataStream ds(compressed ? qUncompress(data) : data);
    ds.setVersion(Consts::DATASTREAM_VERSION);

    int version;
    ds >> version;
    return version == _MESSAGE_STREAM_VERSION;
}

Message::Message(MessageType message_type, const MessageCode& message_code, const MessageID& feedback_message_id,
                 const DataContainerList& data, const MessageID& message_id)
    : _d(new Message_SharedData())
{
    _d->type = message_type;
    _d->code = message_code;
    _d->id = message_id;
    _d->data = data;
    _d->feedback_message_id = feedback_message_id;
}

QMap<DatabaseID, Message> Message::splitByDatabase(const Message& message)
{
    // обрабатывает команды серверу (commands)

    QMap<DatabaseID, Message> result;

    auto type = message.messageType();

    if (type == MessageType::DBCommandGetPropertyTable)
        return {{DBCommandGetPropertyTableMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandQuery)
        return {{DBCommandQueryMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandGetConnectionInformation)
        return {{DBCommandGetConnectionInformationMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandGetEntityList)
        return {{DBCommandGetEntityListMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandLogin)
        return {{DBCommandLoginMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandGetDataTable)
        return {{DBCommandGetDataTableMessage(message).databaseID(), message}};

    if (type == MessageType::DBCommandIsEntityExists) {
        QMap<DatabaseID, UidList> split;
        auto uids = DBCommandIsEntityExistsMessage(message).entityUidList();
        for (const auto& uid : qAsConst(uids)) {
            auto f = split.value(uid.database());
            f << uid;
            split[uid.database()] = f;
        }

        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = DBCommandIsEntityExistsMessage(it.value()).setMessageId(message.messageId());
        }

    } else if (type == MessageType::DBCommandGetEntity) {
        struct Data
        {
            UidList uids;
            QList<DataPropertySet> properties;
        };
        QMap<DatabaseID, std::shared_ptr<Data>> split;

        auto msg = DBCommandGetEntityMessage(message);
        auto uids = msg.entityUids();
        auto props = msg.properties();

        for (int i = 0; i < uids.count(); i++) {
            auto f = split.value(uids.at(i).database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(Data);
                split[uids.at(i).database()] = f;
            }

            f->uids << uids.at(i);
            f->properties << props.at(i);
        }
        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = DBCommandGetEntityMessage(it.value()->uids, it.value()->properties).setMessageId(message.messageId());
        }

    } else if (type == MessageType::DBCommandWriteEntity) {
        struct Data
        {
            UidList uids;
            DataContainerList containers;
            QList<DataPropertySet> properties;
        };
        QMap<DatabaseID, std::shared_ptr<Data>> split;

        auto msg = DBCommandWriteEntityMessage(message);
        auto uids = msg.entityUids();
        auto containers = msg.data();
        auto props = msg.properties();

        for (int i = 0; i < uids.count(); i++) {
            auto f = split.value(uids.at(i).database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(Data);
                split[uids.at(i).database()] = f;
            }

            f->uids << uids.at(i);
            f->containers << containers.at(i);
            f->properties << props.at(i);
            split[uids.at(i).database()] = f;
        }
        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = DBCommandWriteEntityMessage(it.value()->uids, it.value()->containers, it.value()->properties)
                                   .setMessageId(message.messageId());
        }

    } else if (type == MessageType::DBCommandUpdateEntities) {
        struct Data
        {
            UidList uids;
            QList<QMap<DataProperty, QVariant>> values;
        };
        QMap<DatabaseID, std::shared_ptr<Data>> split;

        auto msg = DBCommandUpdateEntitiesMessage(message);
        auto uids = msg.entityUids();
        auto values = msg.values();

        for (int i = 0; i < uids.count(); i++) {
            auto f = split.value(uids.at(i).database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(Data);
                split[uids.at(i).database()] = f;
            }

            f->uids << uids.at(i);
            f->values << values.at(i);
            split[uids.at(i).database()] = f;
        }
        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()]
                = DBCommandUpdateEntitiesMessage(it.value()->uids, it.value()->values, msg.parameters()).setMessageId(message.messageId());
        }

    } else if (type == MessageType::DBCommandRemoveEntity) {
        QMap<DatabaseID, std::shared_ptr<UidList>> split;
        auto uids = DBCommandRemoveEntityMessage(message).entityUidList();
        for (const auto& uid : qAsConst(uids)) {
            auto f = split.value(uid.database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(UidList);
                split[uid.database()] = f;
            }
            *f << uid;
        }

        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = DBCommandRemoveEntityMessage(*it.value()).setMessageId(message.messageId());
        }

    } else if (type == MessageType::DBCommandGetAccessRights) {
        QMap<DatabaseID, std::shared_ptr<UidList>> split;
        auto msg = DBCommandGetAccessRightsMessage(message);
        auto uids = msg.entityUids();
        for (const auto& uid : qAsConst(uids)) {
            auto f = split.value(uid.database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(UidList);
                split[uid.database()] = f;
            }
            *f << uid;
        }

        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = DBCommandGetAccessRightsMessage(*it.value(), msg.login()).setMessageId(message.messageId());
        }

    } else if (type == MessageType::MMCommandGetModels) {
        struct Data
        {
            UidList uids;
            QList<LoadOptions> load_options;
            QList<DataPropertySet> properties;
        };
        QMap<DatabaseID, std::shared_ptr<Data>> split;

        auto msg = MMCommandGetModelsMessage(message);
        auto uids = msg.entityUids();
        auto load_options = msg.loadOptions();
        auto props = msg.properties();

        for (int i = 0; i < uids.count(); i++) {
            auto f = split.value(uids.at(i).database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(Data);
                split[uids.at(i).database()] = f;
            }

            f->uids << uids.at(i);
            f->load_options << load_options.at(i);
            f->properties << props.at(i);
        }
        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = MMCommandGetModelsMessage(it.value()->uids, it.value()->load_options, it.value()->properties)
                                   .setMessageId(message.messageId());
        }

    } else if (type == MessageType::MMCommandRemoveModels) {
        struct Data
        {
            UidList uids;
            QList<DataPropertySet> properties;
        };
        QMap<DatabaseID, std::shared_ptr<Data>> split;

        auto msg = MMCommandRemoveModelsMessage(message);
        auto uids = msg.entityUids();
        auto props = msg.properties();

        for (int i = 0; i < uids.count(); i++) {
            auto f = split.value(uids.at(i).database());
            if (f == nullptr) {
                f = Z_MAKE_SHARED(Data);
                split[uids.at(i).database()] = f;
            }

            f->uids << uids.at(i);
            f->properties << props.at(i);
        }
        for (auto it = split.constBegin(); it != split.constEnd(); ++it) {
            result[it.key()] = MMCommandRemoveModelsMessage(it.value()->uids, it.value()->properties).setMessageId(message.messageId());
        }
    }

    return result;
}

Message Message::concatByDatabase(const QList<Message>& messages)
{
    // обрабатывает ответы сервера (events)

    if (messages.isEmpty())
        return Message();

    if (messages.at(0).messageType() == MessageType::DBEventQueryFeedback) {
        DataContainerList data;
        for (auto& m : messages) {
            data << DBEventQueryFeedbackMessage(m).data();
        }
        return DBEventQueryFeedbackMessage(messages.at(0).feedbackMessageId(), data);
    }

    if (messages.at(0).messageType() == MessageType::DBEventEntityLoaded) {
        UidList uids;
        DataContainerList data;
        AccessRightsList direct_rights;
        AccessRightsList relation_rights;
        for (auto& m : messages) {
            auto msg = DBEventEntityLoadedMessage(m);
            uids << msg.entityUids();
            data << msg.data();
            direct_rights << msg.directRights();
            relation_rights << msg.relationRights();
        }
        return DBEventEntityLoadedMessage(messages.at(0).feedbackMessageId(), uids, data, direct_rights, relation_rights);
    }

    if (messages.at(0).messageType() == MessageType::DBEventEntityExists) {
        UidList uids;
        QList<bool> exists;
        for (auto& m : messages) {
            auto msg = DBEventEntityExistsMessage(m);
            uids << msg.entityUids();
            exists << msg.isExists();
        }
        return DBEventEntityExistsMessage(messages.at(0).feedbackMessageId(), uids, exists);
    }

    if (messages.at(0).messageType() == MessageType::DBEventEntityChanged) {
        UidList uids;
        EntityCodeList codes;
        for (auto& m : messages) {
            auto msg = DBEventEntityChangedMessage(m);
            uids << msg.entityUids();
            codes << msg.entityCodes();
        }
        return DBEventEntityChangedMessage(uids, codes);
    }

    if (messages.at(0).messageType() == MessageType::DBEventEntityRemoved) {
        UidList uids;
        for (auto& m : messages) {
            auto msg = DBEventEntityRemovedMessage(m);
            uids << msg.entityUids();
        }
        return DBEventEntityRemovedMessage(uids);
    }

    if (messages.at(0).messageType() == MessageType::DBEventEntityCreated) {
        UidList uids;
        for (auto& m : messages) {
            auto msg = DBEventEntityCreatedMessage(m);
            uids << msg.entityUids();
        }
        return DBEventEntityCreatedMessage(uids);
    }

    if (messages.at(0).messageType() == MessageType::DBEventAccessRights) {
        UidList uids;
        AccessRightsList direct_rights;
        AccessRightsList relation_rights;
        for (auto& m : messages) {
            auto msg = DBEventAccessRightsMessage(m);
            uids << msg.entityUids();
            direct_rights << msg.directRights();
            relation_rights << msg.relationRights();
        }
        return DBEventAccessRightsMessage(messages.at(0).feedbackMessageId(), uids, direct_rights, relation_rights);
    }

    if (messages.at(0).messageType() == MessageType::MMEventModelList) {
        QList<ModelPtr> models;
        for (auto& m : messages) {
            auto msg = MMEventModelListMessage(m);
            models << msg.models();
        }
        return MMEventModelListMessage(messages.at(0).feedbackMessageId(), models);
    }

    Z_CHECK(messages.count() == 1);
    return messages.at(0);
}

VariantListMessage::VariantListMessage()
    : Message()
{
}

VariantListMessage::VariantListMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::VariantList);
}

VariantListMessage::VariantListMessage(const QString& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : VariantListMessage(QVariantList {data}, message_code, feedback_message_id)
{
}

VariantListMessage::VariantListMessage(const char* data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : VariantListMessage(QString::fromUtf8(data), message_code, feedback_message_id)
{
}

VariantListMessage::VariantListMessage(const QVariantList& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::VariantList, message_code, feedback_message_id, data))
{
}

VariantListMessage::VariantListMessage(const QVariant& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : VariantListMessage(QVariantList {data}, message_code, feedback_message_id)
{
}

QVariantList VariantListMessage::data() const
{
    return variantList();
}

QString VariantListMessage::dataToText() const
{
    if (!isValid() || messageType() != MessageType::VariantList)
        return QString();

    QString text;
    auto data = variantList();
    int count = qMin(10, data.count());
    for (int i = 0; i < qMin(10, count); i++) {
        if (i > 0)
            text += ", ";
        text += Utils::variantToString(data.at(i));
    }
    if (count < data.count()) {
        text += QStringLiteral("...(%1)").arg(data.count());
    }

    return text;
}

Message* VariantListMessage::clone() const
{
    return new VariantListMessage(*this);
}

IntListMessage::IntListMessage()
    : Message()
{
}

IntListMessage::IntListMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::IntList);
}

IntListMessage::IntListMessage(const QList<int>& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::IntList, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

QList<int> IntListMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QList<int>>() : QList<int>();
}

Message* IntListMessage::clone() const
{
    return new IntListMessage(*this);
}

QString IntListMessage::dataToText() const
{
    return Utils::containerToString(data());
}

StringListMessage::StringListMessage()
    : Message()
{
}

StringListMessage::StringListMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::StringList);
}

StringListMessage::StringListMessage(const QStringList& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::StringList, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

QStringList StringListMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QStringList>() : QStringList();
}

Message* StringListMessage::clone() const
{
    return new StringListMessage(*this);
}

QString StringListMessage::dataToText() const
{
    return Utils::containerToString(data(), 10);
}

VariantMapMessage::VariantMapMessage()
    : Message()
{
}

VariantMapMessage::VariantMapMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::VariantMap);
}

VariantMapMessage::VariantMapMessage(const QMap<QString, QVariant>& data, const MessageCode& message_code,
                                     const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::VariantMap, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

QMap<QString, QVariant> VariantMapMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

Message* VariantMapMessage::clone() const
{
    return new StringListMessage(*this);
}

QString VariantMapMessage::dataToText() const
{
    return "VariantMapMessage";
}

ByteArrayListMessage::ByteArrayListMessage()
    : Message()
{
}

ByteArrayListMessage::ByteArrayListMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::ByteArrayList);
}

ByteArrayListMessage::ByteArrayListMessage(const QList<QByteArray>& data, const MessageCode& message_code,
                                           const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::ByteArrayList, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

QList<QByteArray> ByteArrayListMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QList<QByteArray>>() : QList<QByteArray>();
}

Message* ByteArrayListMessage::clone() const
{
    return new ByteArrayListMessage(*this);
}

QString ByteArrayListMessage::dataToText() const
{
    return QStringLiteral("ByteArray count %1").arg(data().count());
}

BoolMessage::BoolMessage()
    : Message()
{
}

BoolMessage::BoolMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::Bool);
}

BoolMessage::BoolMessage(bool value, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::Bool, message_code, feedback_message_id, QVariantList {QVariant::fromValue(value)}))
{
}

bool BoolMessage::value() const
{
    return isValid() && !isEmpty() ? variantList().at(0).toBool() : false;
}

Message* BoolMessage::clone() const
{
    return new BoolMessage(*this);
}

QString BoolMessage::dataToText() const
{
    return value() ? QStringLiteral("TRUE") : QStringLiteral("FALSE");
}

PropertyTableMessage::PropertyTableMessage()
    : Message()
{
}

PropertyTableMessage::PropertyTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::PropertyTable);
}

PropertyTableMessage::PropertyTableMessage(const PropertyTable& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::PropertyTable, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

PropertyTable PropertyTableMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<PropertyTable>() : PropertyTable();
}

Message* PropertyTableMessage::clone() const
{
    return new PropertyTableMessage(*this);
}

IntTableMessage::IntTableMessage()
    : Message()
{
}

IntTableMessage::IntTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || messageType() == MessageType::IntTable);
}

IntTableMessage::IntTableMessage(const IntTable& data, const MessageCode& message_code, const MessageID& feedback_message_id)
    : Message(Message::create(MessageType::IntTable, message_code, feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

IntTable IntTableMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<IntTable>() : IntTable();
}

MessageUidContainsHelper::MessageUidContainsHelper(I_MessageContainsUid* mi) : _mi(mi)
{
}

bool MessageUidContainsHelper::contains(const EntityCode& entity_code) const
{
    int res = _contains_code.value(entity_code, -1);
    if (res < 0) {
        res = 0;

        int count = _mi->count_I_MessageContainsUidCodes();
        for (int i = 0; i < count; i++) {
            EntityCode code = _mi->value_I_MessageContainsUidCodes(i);
            if (code == entity_code) {
                res = 1;
                _contains_code[entity_code] = res;
                break;
            }
        }

        count = _mi->count_I_MessageContainsUid();
        for (int i = 0; i < count; i++) {
            Uid uid = _mi->value_I_MessageContainsUid(i);
            _contains_uid[uid] = 1;
            if (uid.entityCode() == entity_code) {
                res = 1;
                _contains_code[entity_code] = res;
                break;
            }
        }
    }

    return res > 0;
}

bool MessageUidContainsHelper::contains(const Uid& entity_uid) const
{
    bool found = false;
    if (_contains_uid.isEmpty()) {
        int count = _mi->count_I_MessageContainsUid();
        if (count == 0)
            return false;

        for (int i = 0; i < count; i++) {
            Uid uid = _mi->value_I_MessageContainsUid(i);
            _contains_code[uid.entityCode()] = 1;
            _contains_uid[uid] = 1;

            if (uid == entity_uid)
                found = true;
        }

        return found;
    }

    return _contains_uid.contains(entity_uid);
}

ModelListMessage::ModelListMessage()
    : Message()
{
}

ModelListMessage::ModelListMessage(const Message& m)
    : Message(m)
{    
}

ModelListMessage::ModelListMessage(MessageType type, const MessageID& feedback_id, const QList<ModelPtr>& models)
    : Message(Message::create(type, MessageCode(), feedback_id, QVariantList {QVariant::fromValue(models)}))
{
}

QList<ModelPtr> ModelListMessage::models() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QList<ModelPtr>>() : QList<ModelPtr>();
}

int ModelListMessage::count_I_MessageContainsUid() const
{
    return models().count();
}

Uid ModelListMessage::value_I_MessageContainsUid(int index) const
{
    return models().at(index)->entityUid();
}

Message* ModelListMessage::clone() const
{
    return new ModelListMessage(*this);
}

QString ModelListMessage::dataToText() const
{
    auto data = models();
    UidList uids;
    for (auto& m : qAsConst(data)) {
        if (m != nullptr)
            uids << m->entityUid();
    }

    return Utils::containerToString(uids, 10);
}

ErrorMessage::ErrorMessage()
    : Message()
{
}

ErrorMessage::ErrorMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::Error);
}

ErrorMessage::ErrorMessage(const MessageID& feedback_message_id, const Error& error)
    : Message(Message::create(MessageType::Error, MessageCode(), feedback_message_id, QVariantList {error.variant()}))
{
}

ErrorMessage::ErrorMessage(const MessageID& message_id, const MessageID& feedback_message_id, const Error& error)
    : ErrorMessage(feedback_message_id, error)
{
    setMessageId(message_id);
}

Error ErrorMessage::error() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<Error>() : Error();
}

QString ErrorMessage::dataToText() const
{
    return error().fullText();
}

EntityUidListMessage::EntityUidListMessage()
    : Message()
{
}

EntityUidListMessage::EntityUidListMessage(const Message& m)
    : Message(m)
{
}

EntityUidListMessage::EntityUidListMessage(MessageType type, const UidList& entity_uids, const MessageID& feedback_message_id,
                                           const QList<QMap<QString, QVariant>>& parameters, const QVariant& additional_info)
    : Message(Message::create(type, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(parameters), additional_info}))

{
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());
}

Message* EntityUidListMessage::clone() const
{
    return new EntityUidListMessage(*this);
}

QString EntityUidListMessage::dataToText() const
{
    return Utils::containerToString(entityUidList(), 10);
}

UidList EntityUidListMessage::entityUidList() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<QMap<QString, QVariant>> EntityUidListMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

QVariant EntityUidListMessage::additionalInfo() const
{
    return isValid() && !isEmpty() ? variantList().at(2) : QVariant();
}

int EntityUidListMessage::count_I_MessageContainsUid() const
{
    return entityUidList().count();
}

Uid EntityUidListMessage::value_I_MessageContainsUid(int index) const
{
    return entityUidList().at(index);
}

ProgressMessage::ProgressMessage()
    : Message()
{
}

ProgressMessage::ProgressMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::Progress);
}

ProgressMessage::ProgressMessage(int percent, const QString& info, const MessageID& reply_message_id)
    : Message(Message::create(MessageType::Progress, MessageCode(), MessageID(),
                              QVariantList {QVariant(percent), QVariant(info), QVariant(reply_message_id.id())}))
{
}

int ProgressMessage::percent() const
{
    return isValid() && !isEmpty() ? variantList().at(0).toInt() : 0;
}

QString ProgressMessage::info() const
{
    return isValid() && !isEmpty() ? variantList().at(1).toString() : QString();
}

MessageID ProgressMessage::replyMessageID() const
{
    return isValid() && !isEmpty() ? MessageID(variantList().at(2).toInt()) : MessageID();
}

Message* ProgressMessage::clone() const
{
    return new ProgressMessage(*this);
}

QString ProgressMessage::dataToText() const
{
    return QStringLiteral("Percent: %1. Info: %2").arg(percent()).arg(info());
}

BroadcastMessage::BroadcastMessage(const EntityCode& entityCode, int id, const QMap<QString, QVariant>& data)
    : zf::Message(Message::create(zf::MessageType::General, MessageCode(entityCode, id), {}, QVariantList {QVariant::fromValue(data)}))
{
    Z_CHECK(entityCode.isValid());
}

BroadcastMessage::BroadcastMessage(const MessageCode& messageCode, const QMap<QString, QVariant>& data)
    : BroadcastMessage(messageCode.entity(), messageCode.code(), data)
{
}

BroadcastMessage::BroadcastMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == zf::MessageType::General && m.messageCode() == messageCode()));
}

BroadcastMessage::BroadcastMessage(const BroadcastMessage& m)
    : zf::Message(m)
{
}

EntityCode BroadcastMessage::entityCode() const
{
    return messageCode().entity();
}

int BroadcastMessage::id() const
{
    return messageCode().code();
}

QMap<QString, QVariant> BroadcastMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QMap<QString, QVariant>>() : QMap<QString, QVariant> {};
}

#if Z_LOCAL_MESAGE_ID

MessageID::MessageID()
{
}
MessageID::MessageID(quint64 id)
    : Handle<quint64>(id)
{
}
MessageID::MessageID(const Handle<quint64>& h)
    : Handle<quint64>(h)
{
}

MessageCode::MessageCode()
{
    _key.key_64 = 0;
}

MessageCode::MessageCode(const MessageCode& c)
    : _key(c._key)
{
}

MessageCode::MessageCode(int message_code)
{
    Z_CHECK(message_code > 0);
    _key.key_64 = 0;
    _key.key_split.message_code = message_code;
}

MessageCode::MessageCode(const EntityCode& entity_code, int message_code)
{
    Z_CHECK(entity_code.isValid());
    Z_CHECK(message_code > 0);
    _key.key_split.entity_code = entity_code.value();
    _key.key_split.message_code = message_code;
}

bool MessageCode::isValid() const
{
    return _key.key_64 > 0;
}

bool MessageCode::isEntityBased() const
{
    return _key.key_split.entity_code >= Consts::MIN_ENTITY_CODE;
}

EntityCode MessageCode::entity() const
{
    return _key.key_split.entity_code >= Consts::MIN_ENTITY_CODE ? EntityCode(_key.key_split.entity_code) : EntityCode();
}

int MessageCode::code() const
{
    return _key.key_split.message_code;
}

bool MessageCode::operator==(const MessageCode& c) const
{
    return _key.key_64 == c._key.key_64;
}

MessageCode& MessageCode::operator=(const MessageCode& c)
{
    _key.key_64 = c._key.key_64;
    return *this;
}

QString MessageCode::toPrintable() const
{
    return QStringLiteral("%1:%2").arg(entity().value()).arg(code());
}

#else

MessageID::MessageID()
{
}

MessageID::MessageID(const MessageID& m)
    : QString(m)
{
}

MessageID& MessageID::operator=(const MessageID& m)
{
    QString::operator=(m);
    return *this;
}

bool MessageID::operator==(const MessageID& m) const
{
    return cast() == m.cast();
}

bool MessageID::operator!=(const MessageID& m) const
{
    return cast() != m.cast();
}

bool MessageID::operator<(const MessageID& m) const
{
    return cast() < m.cast();
}

bool MessageID::isValid() const
{
    return !isEmpty();
}

QString MessageID::toString() const
{
    return cast();
}

void MessageID::clear()
{
    QString::clear();
}

uint MessageID::hashValue() const
{
    return ::qHash(cast());
}

MessageID MessageID::generate()
{
    return MessageID(Utils::generateUniqueStringDefault());
}

QString& MessageID::cast()
{
    return *this;
}

const QString& MessageID::cast() const
{
    return *this;
}

MessageID::MessageID(const QString& s)
    : QString(s)
{
}

QString MessageCode::toPrintable() const
{
    return toString();
}

#endif

} // namespace zf

QDataStream& operator<<(QDataStream& out, const zf::Message& obj)
{
    out << _MESSAGE_STREAM_VERSION;

    toStreamInt(out, obj._d->type);
    out << obj._d->code << obj._d->id << obj._d->feedback_message_id;

    out << obj._d->data.count();

    for (auto& d : obj._d->data) {
        out << d.isValid();
        if (!d.isValid())
            continue;

        out << *d.structure();
        d.toStream(out);
    }

    return out;
}

QDataStream& operator>>(QDataStream& in, zf::Message& obj)
{
    using namespace zf;

    obj.clear();

    int version;
    in >> version;

    if (version != _MESSAGE_STREAM_VERSION) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    fromStreamInt(in, obj._d->type);
    in >> obj._d->code >> obj._d->id >> obj._d->feedback_message_id;

    int count;
    in >> count;
    if (in.status() != QDataStream::Ok || count < 0) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);

        return in;
    }

    for (int i = 0; i < count; i++) {
        bool valid;
        in >> valid;
        if (!valid) {
            obj._d->data << DataContainer();
            continue;
        }

        DataStructurePtr structure;
        in >> structure;
        if (in.status() != QDataStream::Ok || structure == nullptr) {
            if (in.status() == QDataStream::Ok)
                in.setStatus(QDataStream::ReadCorruptData);

            obj.clear();
            return in;
        }

        Error error;
        auto container = DataContainer::fromStream(in, structure, error);
        if (error.isError())
            Core::logError(error);

        if (in.status() != QDataStream::Ok || error.isError()) {
            if (in.status() == QDataStream::Ok)
                in.setStatus(QDataStream::ReadCorruptData);

            obj.clear();
            return in;
        }

        obj._d->data << *container;
    }

    if (in.status() != QDataStream::Ok)
        obj.clear();
    return in;
}

QDebug operator<<(QDebug debug, const zf::Message& c)
{
    using namespace zf;

    QDebugStateSaver saver(debug);
    debug.nospace().noquote();

    debug << QStringLiteral("%1(%2)").arg(c.messageTypeName()).arg(c.messageId().toString());
    Core::beginDebugOutput();
    debug << "\n";
    debug << Core::debugIndent() << "messageId: " << c.messageId() << "\n";
    debug << Core::debugIndent() << "messageType: " << c.messageType() << "\n";
    debug << Core::debugIndent() << "messageTypeName: " << c.messageTypeName() << "\n";
    debug << Core::debugIndent() << "feedbackMessageId: " << c.feedbackMessageId() << "\n";
    debug << Core::debugIndent() << "messageCode: " << c.messageCode() << "\n";

    debug << Core::debugIndent() << "containers:";
    if (!c.rawData().isEmpty()) {
        debug << "\n";
        for (int i = 0; i < c.rawData().count(); i++) {
            Core::beginDebugOutput();
            debug << Core::debugIndent() << QStringLiteral("container %1:").arg(i + 1);
            debug << c.rawData().at(i);
            Core::endDebugOutput();
        }
    }

    Core::endDebugOutput();

    return debug;
}

#if !Z_LOCAL_MESAGE_ID
QDebug operator<<(QDebug debug, const zf::MessageID& c)
{
    QDebugStateSaver saver(debug);
    return debug.nospace().noquote() << c.cast();
}

QDataStream& operator<<(QDataStream& out, const zf::MessageID& obj)
{
    return out << obj.cast();
}

QDataStream& operator>>(QDataStream& in, zf::MessageID& obj)
{
    return in >> obj.cast();
}
#endif

QDataStream& operator<<(QDataStream& out, const zf::MessageCode& obj)
{
    return out << obj._key.key_64;
}

QDataStream& operator>>(QDataStream& in, zf::MessageCode& obj)
{
    return in >> obj._key.key_64;
}

QDebug operator<<(QDebug debug, const zf::MessageCode& c)
{
    QDebugStateSaver saver(debug);

    if (!c.isValid())
        return debug.nospace().noquote() << "invalid";
    if (c.isEntityBased())
        return debug.nospace().noquote() << QString("%1(%2)").arg(c.entity().string()).arg(c.code());

    return debug.nospace().noquote() << c.code();
}
