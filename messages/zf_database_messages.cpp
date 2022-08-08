#include "zf_database_messages.h"
#include "zf_core.h"

namespace zf
{
//! Права доступа в виде QMap
static UidAccessRights rightsMapHelper(const UidList& uids, const AccessRightsList& rights)
{
    Z_CHECK(uids.count() == rights.count());

    UidAccessRights res;
    for (int i = 0; i < uids.count(); i++)
        res[uids.at(i)] = rights.at(i);

    return res;
}

DBCommandIsEntityExistsMessage::DBCommandIsEntityExistsMessage()
    : EntityUidListMessage()
{
}

DBCommandIsEntityExistsMessage::DBCommandIsEntityExistsMessage(const Message& m)
    : EntityUidListMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandIsEntityExists);
}

DBCommandIsEntityExistsMessage::DBCommandIsEntityExistsMessage(const UidList& entity_uids, const QList<QMap<QString, QVariant>>& parameters)
    : EntityUidListMessage(MessageType::DBCommandIsEntityExists, entity_uids, MessageID(), parameters, QVariant())
{
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());
}

DBCommandIsEntityExistsMessage::DBCommandIsEntityExistsMessage(const Uid& entity_uid, const QMap<QString, QVariant>& parameters)
    : DBCommandIsEntityExistsMessage(UidList {entity_uid}, {parameters})
{
}

Message* DBCommandIsEntityExistsMessage::clone() const
{
    return new DBCommandIsEntityExistsMessage(*this);
}

DBCommandGetEntityMessage::DBCommandGetEntityMessage()
    : Message()
{
}

DBCommandGetEntityMessage::DBCommandGetEntityMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandGetEntity);
}

DBCommandGetEntityMessage::DBCommandGetEntityMessage(const UidList& entity_uids, const QList<DataPropertySet>& properties,
                                                     const QList<QMap<QString, QVariant>>& parameters)
    : Message(
        Message::create(MessageType::DBCommandGetEntity, MessageCode(), MessageID(),
                        QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(properties), QVariant::fromValue(parameters)}))
{
    Z_CHECK(properties.isEmpty() || properties.count() == entity_uids.count());
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());
}

DBCommandGetEntityMessage::DBCommandGetEntityMessage(const Uid& entity_uid, const DataPropertySet& properties,
                                                     const QMap<QString, QVariant>& parameters)
    : DBCommandGetEntityMessage(UidList {entity_uid}, QList<DataPropertySet> {properties}, QList<QMap<QString, QVariant>> {parameters})
{
}

UidList DBCommandGetEntityMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<DataPropertySet> DBCommandGetEntityMessage::properties() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<DataPropertySet>>() : QList<DataPropertySet>();
}

QList<QMap<QString, QVariant>> DBCommandGetEntityMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

Message* DBCommandGetEntityMessage::clone() const
{
    return new DBCommandGetEntityMessage(*this);
}

QString DBCommandGetEntityMessage::dataToText() const
{
    return Utils::uidsPropsToString(entityUids(), properties());
}

int DBCommandGetEntityMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBCommandGetEntityMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBCommandRemoveEntityMessage::DBCommandRemoveEntityMessage()
    : EntityUidListMessage()
{
}

DBCommandRemoveEntityMessage::DBCommandRemoveEntityMessage(const Message& m)
    : EntityUidListMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandRemoveEntity);
}

DBCommandRemoveEntityMessage::DBCommandRemoveEntityMessage(const UidList& entity_uids, const QList<QMap<QString, QVariant>>& parameters,
                                                           const QList<bool>& by_user)
    : EntityUidListMessage(MessageType::DBCommandRemoveEntity, entity_uids, MessageID(), parameters, QVariant::fromValue(by_user))
{    
}

DBCommandRemoveEntityMessage::DBCommandRemoveEntityMessage(const Uid& entity_uid, const QMap<QString, QVariant>& parameters)
    : DBCommandRemoveEntityMessage(UidList {entity_uid}, {parameters})
{
}

DBCommandRemoveEntityMessage::DBCommandRemoveEntityMessage(const Uid& entity_uid, const QMap<QString, QVariant>& parameters, bool by_user)
    : DBCommandRemoveEntityMessage(UidList {entity_uid}, {parameters}, QList<bool> {by_user})
{
}

QList<bool> DBCommandRemoveEntityMessage::byUser() const
{
    return additionalInfo().value<QList<bool>>();
}

Message* DBCommandRemoveEntityMessage::clone() const
{
    return new DBCommandRemoveEntityMessage(*this);
}

DBEventQueryFeedbackMessage::DBEventQueryFeedbackMessage()
    : Message()
{
}

DBEventQueryFeedbackMessage::DBEventQueryFeedbackMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventQueryFeedback);
}


DBEventQueryFeedbackMessage::DBEventQueryFeedbackMessage(
        const MessageID& feedback_message_id, const DataContainerList& data)
    : Message(create(feedback_message_id, data))
{
}

DataContainerList DBEventQueryFeedbackMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DataContainerList>() : DataContainerList();
}

Message* DBEventQueryFeedbackMessage::clone() const
{
    return new DBEventQueryFeedbackMessage(*this);
}

Message DBEventQueryFeedbackMessage::create(const MessageID& feedback_message_id, const DataContainerList& data)
{
    return Message::create(MessageType::DBEventQueryFeedback, MessageCode(), feedback_message_id, QVariantList {QVariant::fromValue(data)});
}

DBEventEntityRelatedMessage::DBEventEntityRelatedMessage()
    : Message()
{
}

DBEventEntityRelatedMessage::DBEventEntityRelatedMessage(const Message& m)
    : Message(m)
{    
}

DBEventEntityRelatedMessage::DBEventEntityRelatedMessage(MessageType type, const UidList& entity_uids, const QList<bool>& by_user,
                                                         const QVariant& additional_info, const MessageID& feedback_message_id)
    : Message(Message::create(type, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(by_user), additional_info}))
{    
    Z_CHECK(by_user.isEmpty() || by_user.count() == entity_uids.count());
}

UidList DBEventEntityRelatedMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<bool> DBEventEntityRelatedMessage::byUser() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<bool>>() : QList<bool>();
}

QVariant DBEventEntityRelatedMessage::additionalInfo() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QVariant>() : QVariant();
}

QString DBEventEntityRelatedMessage::dataToText() const
{
    return Utils::containerToString(entityUids(), 10);
}

int DBEventEntityRelatedMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBEventEntityRelatedMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBEventEntityChangedMessage::DBEventEntityChangedMessage()
    : DBEventEntityRelatedMessage()
{
}

DBEventEntityChangedMessage::DBEventEntityChangedMessage(const Message& m)
    : DBEventEntityRelatedMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityChanged);
}

DBEventEntityChangedMessage::DBEventEntityChangedMessage(const UidList& entity_uids, const EntityCodeList& entity_codes,
                                                         const QList<bool>& by_user)
    : DBEventEntityRelatedMessage(MessageType::DBEventEntityChanged, entity_uids, by_user, QVariant::fromValue(entity_codes), MessageID())
{
    Z_CHECK(!entity_uids.isEmpty() || !entity_codes.isEmpty());
}

EntityCodeList DBEventEntityChangedMessage::entityCodes() const
{
    return additionalInfo().value<EntityCodeList>();
}

QString DBEventEntityChangedMessage::dataToText() const
{
    QString res = Utils::containerToString(entityUids(), 10);
    auto codes = entityCodes();
    if (!res.isEmpty() && !codes.isEmpty())
        res += "; ";

    for (int i = 0; i < codes.count(); i++) {
        res += codes.at(i).string();
        if (i != codes.count() - 1)
            res += ", ";
    }

    return res;
}

Message* DBEventEntityChangedMessage::clone() const
{
    return new DBEventEntityChangedMessage(*this);
}

int DBEventEntityChangedMessage::count_I_MessageContainsUidCodes() const
{
    return entityCodes().count();
}

EntityCode DBEventEntityChangedMessage::value_I_MessageContainsUidCodes(int index) const
{
    return entityCodes().at(index);
}

DBEventEntityRemovedMessage::DBEventEntityRemovedMessage()
    : DBEventEntityRelatedMessage()
{
}

DBEventEntityRemovedMessage::DBEventEntityRemovedMessage(const Message& m)
    : DBEventEntityRelatedMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityRemoved);
}

DBEventEntityRemovedMessage::DBEventEntityRemovedMessage(const UidList& entity_uids, const QList<bool>& by_user)
    : DBEventEntityRelatedMessage(MessageType::DBEventEntityRemoved, entity_uids, by_user, QVariant(), MessageID())
{    
}

DBEventEntityRemovedMessage::DBEventEntityRemovedMessage(const Uid& entity_uid)
    : DBEventEntityRemovedMessage(UidList {entity_uid})
{
}

Message* DBEventEntityRemovedMessage::clone() const
{
    return new DBEventEntityRemovedMessage(*this);
}

DBEventEntityCreatedMessage::DBEventEntityCreatedMessage()
    : DBEventEntityRelatedMessage()
{
}

DBEventEntityCreatedMessage::DBEventEntityCreatedMessage(const Message& m)
    : DBEventEntityRelatedMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityCreated);
}

DBEventEntityCreatedMessage::DBEventEntityCreatedMessage(const UidList& entity_uids, const QList<bool>& by_user)
    : DBEventEntityRelatedMessage(MessageType::DBEventEntityCreated, entity_uids, by_user, QVariant(), MessageID())
{    
}

Message* DBEventEntityCreatedMessage::clone() const
{
    return new DBEventEntityCreatedMessage(*this);
}

DBEventInformationMessage::DBEventInformationMessage()
    : Message()
{
}

DBEventInformationMessage::DBEventInformationMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventInformation);
}

DBEventInformationMessage::DBEventInformationMessage(const QMap<QString, QVariant>& messages)
    : Message(Message::create(MessageType::DBEventInformation, MessageCode(), MessageID(), QVariantList { QVariant::fromValue(messages) }))
{
}

QMap<QString, QVariant> DBEventInformationMessage::messages() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

Message* DBEventInformationMessage::clone() const
{
    return new DBEventInformationMessage(*this);
}

DBEventConnectionDoneMessage::DBEventConnectionDoneMessage()
    : Message()
{
}

DBEventConnectionDoneMessage::DBEventConnectionDoneMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventConnectionDone);
}

DBEventConnectionDoneMessage::DBEventConnectionDoneMessage(const QMap<QString, QVariant>& data)
    : Message(Message::create(MessageType::DBEventConnectionDone, MessageCode(), MessageID(), QVariantList { QVariant::fromValue(data) }))
{
}

QMap<QString, QVariant> DBEventConnectionDoneMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

Message* DBEventConnectionDoneMessage::clone() const
{
    return new DBEventConnectionDoneMessage(*this);
}

DBEventInitLoadDoneMessage::DBEventInitLoadDoneMessage()
    : Message()
{
}

DBEventInitLoadDoneMessage::DBEventInitLoadDoneMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventInitLoadDone);
}

DBEventInitLoadDoneMessage::DBEventInitLoadDoneMessage(const QMap<QString, QVariant>& data)
    : Message(Message::create(MessageType::DBEventInitLoadDone, MessageCode(), MessageID(), QVariantList { QVariant::fromValue(data) }))
{
}

QMap<QString, QVariant> DBEventInitLoadDoneMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

Message* DBEventInitLoadDoneMessage::clone() const
{
    return new DBEventInitLoadDoneMessage(*this);
}

DBCommandWriteEntityMessage::DBCommandWriteEntityMessage()
    : Message()
{
}

DBCommandWriteEntityMessage::DBCommandWriteEntityMessage(const Message& m) : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandWriteEntity);
}

DBCommandWriteEntityMessage::DBCommandWriteEntityMessage(const UidList& entity_uids, const DataContainerList& data,
                                                         const QList<DataPropertySet>& properties, const DataContainerList& old_data,
                                                         const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& by_user)
    : Message(Message::create(MessageType::DBCommandWriteEntity, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(data), QVariant::fromValue(properties),
                                            QVariant::fromValue(old_data), QVariant::fromValue(parameters), QVariant::fromValue(by_user)}))
{
    Z_CHECK((entity_uids.count() == data.count()) && (entity_uids.count() == properties.count()));
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());
    Z_CHECK(by_user.isEmpty() || by_user.count() == entity_uids.count());
}

DBCommandWriteEntityMessage::DBCommandWriteEntityMessage(const Uid& entity_uid, const DataContainer& data,
                                                         const DataPropertySet& properties, const DataContainer& old_data,
                                                         const QMap<QString, QVariant>& parameters)
    : DBCommandWriteEntityMessage(UidList {entity_uid}, DataContainerList {data}, QList<DataPropertySet> {properties},
                                  old_data.isValid() ? DataContainerList {old_data} : DataContainerList(), {parameters})
{
}

DBCommandWriteEntityMessage::DBCommandWriteEntityMessage(const Uid& entity_uid, const DataContainer& data,
                                                         const DataPropertySet& properties, const DataContainer& old_data,
                                                         const QMap<QString, QVariant>& parameters, bool by_user)
    : DBCommandWriteEntityMessage(UidList {entity_uid}, DataContainerList {data}, QList<DataPropertySet> {properties},
                                  old_data.isValid() ? DataContainerList {old_data} : DataContainerList(), {parameters},
                                  QList<bool> {by_user})
{
}

UidList DBCommandWriteEntityMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

DataContainerList DBCommandWriteEntityMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<DataContainerList>() : DataContainerList();
}

QList<DataPropertySet> DBCommandWriteEntityMessage::properties() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QList<DataPropertySet>>() : QList<DataPropertySet>();
}

QList<bool> DBCommandWriteEntityMessage::byUser() const
{
    return isValid() && !isEmpty() ? variantList().at(5).value<QList<bool>>() : QList<bool>();
}

DataContainerList DBCommandWriteEntityMessage::oldData() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<DataContainerList>() : DataContainerList();
}

QList<QMap<QString, QVariant>> DBCommandWriteEntityMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

Message* DBCommandWriteEntityMessage::clone() const
{
    return new DBCommandWriteEntityMessage(*this);
}

QString DBCommandWriteEntityMessage::dataToText() const
{
    return Utils::uidsPropsToString(entityUids(), properties());
}

int DBCommandWriteEntityMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBCommandWriteEntityMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBEventEntityLoadedMessage::DBEventEntityLoadedMessage()
    : Message()
{
}

DBEventEntityLoadedMessage::DBEventEntityLoadedMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityLoaded);
}


UidList DBEventEntityLoadedMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

DataContainerList DBEventEntityLoadedMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<DataContainerList>() : DataContainerList();
}

AccessRightsList DBEventEntityLoadedMessage::directRights() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<AccessRightsList>() : AccessRightsList();
}

UidAccessRights DBEventEntityLoadedMessage::directRightsMap() const
{
    return rightsMapHelper(entityUids(), directRights());
}

AccessRightsList DBEventEntityLoadedMessage::relationRights() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<AccessRightsList>() : AccessRightsList();
}

UidAccessRights DBEventEntityLoadedMessage::relationRightsMap() const
{
    return rightsMapHelper(entityUids(), relationRights());
}

Message* DBEventEntityLoadedMessage::clone() const
{
    return new DBEventEntityLoadedMessage(*this);
}

QString DBEventEntityLoadedMessage::dataToText() const
{
    auto uids = entityUids();
    auto conteiners = data();
    QList<DataPropertySet> props;

    for (auto& c : qAsConst(conteiners)) {
        props << c.initializedProperties();
    }
    Z_CHECK(props.count() == uids.count());
    return Utils::uidsPropsToString(uids, props);
}

int DBEventEntityLoadedMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBEventEntityLoadedMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBEventEntityLoadedMessage::DBEventEntityLoadedMessage(const MessageID& feedback_message_id, const UidList& entity_uids,
                                                       const DataContainerList& data, const AccessRightsList& direct_rights,
                                                       const AccessRightsList& relation_rights)
    : Message(Message::create(MessageType::DBEventEntityLoaded, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(data), QVariant::fromValue(direct_rights),
                                            QVariant::fromValue(relation_rights)}))
{
    Z_CHECK(!data.isEmpty());
    Z_CHECK(data.count() == entity_uids.count());
    Z_CHECK(direct_rights.isEmpty() || direct_rights.count() == entity_uids.count());
    Z_CHECK(relation_rights.isEmpty() || relation_rights.count() == entity_uids.count());
}

DBEventEntityExistsMessage::DBEventEntityExistsMessage()
    : Message()
{
}

DBEventEntityExistsMessage::DBEventEntityExistsMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityExists);
}

DBEventEntityExistsMessage::DBEventEntityExistsMessage(const MessageID& feedback_message_id, const UidList& entity_uids,
                                                       const QList<bool>& is_exists)
    : Message(Message::create(MessageType::DBEventEntityExists, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(is_exists)}))
{
}

UidList DBEventEntityExistsMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<bool> DBEventEntityExistsMessage::isExists() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<bool>>() : QList<bool>();
}

Message* DBEventEntityExistsMessage::clone() const
{
    return new DBEventEntityExistsMessage(*this);
}

QString DBEventEntityExistsMessage::dataToText() const
{
    return Utils::containerToString(entityUids(), 10);
}

int DBEventEntityExistsMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBEventEntityExistsMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBCommandQueryMessage::DBCommandQueryMessage()
    : Message()
{
}

DBCommandQueryMessage::DBCommandQueryMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandQuery);
}

DBCommandQueryMessage::DBCommandQueryMessage(const DatabaseID& database_id, const MessageCode& message_code, const DataContainerList& data)
    : Message(create(database_id, message_code, data))
{
}

DBCommandQueryMessage::DBCommandQueryMessage(const DatabaseID& database_id, const MessageCode& message_code, const DataContainer& data)
    : DBCommandQueryMessage(database_id, message_code, DataContainerList {data})
{
}

DatabaseID DBCommandQueryMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

DataContainerList DBCommandQueryMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<DataContainerList>() : DataContainerList();
}

Message* DBCommandQueryMessage::clone() const
{
    return new DBCommandQueryMessage(*this);
}

Message DBCommandQueryMessage::create(const DatabaseID& database_id, const MessageCode& message_code, const DataContainerList& data)
{
    return Message::create(MessageType::DBCommandQuery, message_code, MessageID(),
                           QVariantList {QVariant::fromValue(database_id), QVariant::fromValue(data)});
}

DBCommandGetPropertyTableMessage::DBCommandGetPropertyTableMessage()
    : Message()
{
}

DBCommandGetPropertyTableMessage::DBCommandGetPropertyTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandGetPropertyTable);
}

DBCommandGetPropertyTableMessage::DBCommandGetPropertyTableMessage(const DatabaseID& database_id, const EntityCode& entity_code,
                                                                   const DataPropertyList& properties,
                                                                   const PropertyRestriction& restriction,
                                                                   const QMap<QString, QVariant>& parameters)
    : Message(
        Message::create(MessageType::DBCommandGetPropertyTable, MessageCode(), MessageID(),
                        QVariantList {QVariant::fromValue(database_id), QVariant::fromValue(entity_code), QVariant::fromValue(properties),
                                      QVariant(), restriction.variant(), QVariant::fromValue(parameters)}))
{
}

DBCommandGetPropertyTableMessage::DBCommandGetPropertyTableMessage(const DatabaseID& database_id, const EntityCode& entity_code,
                                                                   const PropertyIDList& property_codes,
                                                                   const PropertyRestriction& restriction,
                                                                   const QMap<QString, QVariant>& parameters)
    : Message(Message::create(MessageType::DBCommandGetPropertyTable, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(database_id), QVariant::fromValue(entity_code),
                                            QVariant::fromValue(codeToProperty(entity_code, property_codes)), QVariant(),
                                            restriction.variant(), QVariant::fromValue(parameters)}))
{
}

DBCommandGetPropertyTableMessage::DBCommandGetPropertyTableMessage(const DatabaseID& database_id, const EntityCode& entity_code,
                                                                   const PropertyOptions& options, const PropertyRestriction& restriction,
                                                                   const QMap<QString, QVariant>& parameters)
    : Message(Message::create(MessageType::DBCommandGetPropertyTable, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(database_id), QVariant::fromValue(entity_code), QVariant(),
                                            QVariant::fromValue(options), restriction.variant(), QVariant::fromValue(parameters)}))
{
}

DatabaseID DBCommandGetPropertyTableMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

EntityCode DBCommandGetPropertyTableMessage::entityCode() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<EntityCode>() : EntityCode();
}

DataPropertyList DBCommandGetPropertyTableMessage::properties() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<DataPropertyList>() : DataPropertyList();
}

PropertyOptions DBCommandGetPropertyTableMessage::options() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<PropertyOptions>() : PropertyOptions();
}

PropertyRestriction DBCommandGetPropertyTableMessage::restriction() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<PropertyRestriction>() : PropertyRestriction();
}

QMap<QString, QVariant> DBCommandGetPropertyTableMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(5).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

Message* DBCommandGetPropertyTableMessage::clone() const
{
    return new DBCommandGetPropertyTableMessage(*this);
}

int DBCommandGetPropertyTableMessage::count_I_MessageContainsUidCodes() const
{
    return isValid() ? 1 : 0;
}

EntityCode DBCommandGetPropertyTableMessage::value_I_MessageContainsUidCodes(int index) const
{
    Z_CHECK(index == 0);
    return entityCode();
}

DataPropertyList DBCommandGetPropertyTableMessage::codeToProperty(const EntityCode& entity_code, const PropertyIDList& property_codes)
{
    Z_CHECK(!property_codes.isEmpty());
    DataPropertyList res;
    for (auto& id : property_codes) {
        res << DataProperty::property(entity_code, id);
    }
    return res;
}

ConfirmMessage::ConfirmMessage() : Message()
{
}

ConfirmMessage::ConfirmMessage(const Message& m) : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::Confirm);
}

ConfirmMessage::ConfirmMessage(const MessageID& feedback_message_id, const QVariant& data, bool no_action)
    : Message(Message::create(MessageType::Confirm, MessageCode(), feedback_message_id, QVariantList {data, QVariant(no_action)}))
{
}

QVariant ConfirmMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0) : QVariant();
}

UidList ConfirmMessage::uidList() const
{
    auto list = data().toList();
    Z_CHECK(list.count() == 3);
    return list.at(0).value<UidList>();
}

QList<DataPropertySet> ConfirmMessage::writedProperties() const
{
    auto list = data().toList();
    Z_CHECK(list.count() == 3);
    return list.at(1).value<QList<DataPropertySet>>();
}

bool ConfirmMessage::noAction() const
{
    return isValid() && !isEmpty() ? variantList().at(1).toBool() : false;
}

Message* ConfirmMessage::clone() const
{
    return new ConfirmMessage(*this);
}

DBCommandGetEntityListMessage::DBCommandGetEntityListMessage()
    : Message()
{
}

DBCommandGetEntityListMessage::DBCommandGetEntityListMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandGetEntityList);
}

DBCommandGetEntityListMessage::DBCommandGetEntityListMessage(const EntityCode& entity_code, const DatabaseID& database_id,
                                                             const QMap<QString, QVariant>& parameters)
    : Message(Message::create(MessageType::DBCommandGetEntityList, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(database_id.isValid() ? database_id : Core::defaultDatabase()),
                                            QVariant::fromValue(entity_code), QVariant::fromValue(parameters)}))
{
}

DatabaseID DBCommandGetEntityListMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

EntityCode DBCommandGetEntityListMessage::entityCode() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<EntityCode>() : EntityCode();
}

QMap<QString, QVariant> DBCommandGetEntityListMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QMap<QString, QVariant>>() : QMap<QString, QVariant>();
}

bool DBCommandGetEntityListMessage::contains(const EntityCode& entity_code) const
{
    return this->entityCode() == entity_code;
}

Message* DBCommandGetEntityListMessage::clone() const
{
    return new DBCommandGetEntityListMessage(*this);
}

int DBCommandGetEntityListMessage::count_I_MessageContainsUidCodes() const
{
    return isValid() ? 1 : 0;
}

EntityCode DBCommandGetEntityListMessage::value_I_MessageContainsUidCodes(int index) const
{
    Z_CHECK(index == 0);
    return entityCode();
}

DBEventEntityListMessage::DBEventEntityListMessage()
    : EntityUidListMessage()
{
}

DBEventEntityListMessage::DBEventEntityListMessage(const Message& m)
    : EntityUidListMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventEntityList);
}

DBEventEntityListMessage::DBEventEntityListMessage(const MessageID& feedback_message_id, const UidList& entity_uids)
    : EntityUidListMessage(MessageType::DBEventEntityList, entity_uids, feedback_message_id, QList<QMap<QString, QVariant>>(), QVariant())
{
}

DBEventEntityListMessage::DBEventEntityListMessage(const MessageID& feedback_message_id, const Uid& entity_uid)
    : EntityUidListMessage(MessageType::DBEventEntityList, UidList {entity_uid}, feedback_message_id, QList<QMap<QString, QVariant>>(),
                           QVariant())
{
}

Message* DBEventEntityListMessage::clone() const
{
    return new DBEventEntityListMessage(*this);
}

DBEventPropertyTableMessage::DBEventPropertyTableMessage()
{
}

DBEventPropertyTableMessage::DBEventPropertyTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventPropertyTable);
}

DBEventPropertyTableMessage::DBEventPropertyTableMessage(const MessageID& feedback_message_id, const EntityCode& entity_code,
                                                         const PropertyTable& data)
    : Message(Message::create(MessageType::DBEventPropertyTable, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_code), QVariant::fromValue(data)}))
{    
}

EntityCode DBEventPropertyTableMessage::entityCode() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<EntityCode>() : EntityCode();
}

PropertyTable DBEventPropertyTableMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<PropertyTable>() : PropertyTable();
}

Message* DBEventPropertyTableMessage::clone() const
{
    return new DBEventPropertyTableMessage(*this);
}

int DBEventPropertyTableMessage::count_I_MessageContainsUidCodes() const
{
    return isValid() ? 1 : 0;
}

EntityCode DBEventPropertyTableMessage::value_I_MessageContainsUidCodes(int index) const
{
    Z_CHECK(index == 0);
    return entityCode();
}

DBCommandGetConnectionInformationMessage::DBCommandGetConnectionInformationMessage()
    : Message()
{
}

DBCommandGetConnectionInformationMessage::DBCommandGetConnectionInformationMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandGetConnectionInformation);
}

DBCommandGetConnectionInformationMessage::DBCommandGetConnectionInformationMessage(const DatabaseID& database_id)
    : Message(Message::create(MessageType::DBCommandGetConnectionInformation, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(database_id)}))
{
}

DatabaseID DBCommandGetConnectionInformationMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

Message* DBCommandGetConnectionInformationMessage::clone() const
{
    return new DBCommandGetConnectionInformationMessage(*this);
}

DBEventConnectionInformationMessage::DBEventConnectionInformationMessage()
    : Message()
{
}

DBEventConnectionInformationMessage::DBEventConnectionInformationMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventConnectionInformation);
}

DBEventConnectionInformationMessage::DBEventConnectionInformationMessage(const MessageID& feedback_message_id,
                                                                         const ConnectionInformation& data)
    : Message(Message::create(MessageType::DBEventConnectionInformation, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(data), QVariant()}))
{
}

DBEventConnectionInformationMessage::DBEventConnectionInformationMessage(const MessageID& feedback_message_id, const Error& error)
    : Message(Message::create(MessageType::DBEventConnectionInformation, MessageCode(), feedback_message_id,
                              QVariantList {QVariant(), error.variant()}))
{
}

ConnectionInformation DBEventConnectionInformationMessage::information() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<ConnectionInformation>() : ConnectionInformation();
}

Error DBEventConnectionInformationMessage::error() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<Error>() : Error();
}

Message* DBEventConnectionInformationMessage::clone() const
{
    return new DBEventConnectionInformationMessage(*this);
}

DBCommandGetAccessRightsMessage::DBCommandGetAccessRightsMessage()
    : Message()
{
}

DBCommandGetAccessRightsMessage::DBCommandGetAccessRightsMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandGetAccessRights);
}

DBCommandGetAccessRightsMessage::DBCommandGetAccessRightsMessage(const UidList& entity_uids, const QString& login)
    : Message(Message::create(MessageType::DBCommandGetAccessRights, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(login)}))
{
}

UidList DBCommandGetAccessRightsMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QString DBCommandGetAccessRightsMessage::login() const
{
    return isValid() && !isEmpty() ? variantList().at(1).toString() : QString();
}

Message* DBCommandGetAccessRightsMessage::clone() const
{
    return new DBCommandGetAccessRightsMessage(*this);
}

int DBCommandGetAccessRightsMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBCommandGetAccessRightsMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBEventAccessRightsMessage::DBEventAccessRightsMessage()
    : Message()
{
}

DBEventAccessRightsMessage::DBEventAccessRightsMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBEventAccessRights);
}


UidList DBEventAccessRightsMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

AccessRightsList DBEventAccessRightsMessage::directRights() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<AccessRightsList>() : AccessRightsList();
}

UidAccessRights DBEventAccessRightsMessage::directRightsMap() const
{
    return rightsMapHelper(entityUids(), directRights());
}

AccessRightsList DBEventAccessRightsMessage::relationRights() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<AccessRightsList>() : AccessRightsList();
}

UidAccessRights DBEventAccessRightsMessage::relationRightsMap() const
{
    return rightsMapHelper(entityUids(), relationRights());
}

Message* DBEventAccessRightsMessage::clone() const
{
    return new DBEventAccessRightsMessage(*this);
}

int DBEventAccessRightsMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBEventAccessRightsMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

DBEventAccessRightsMessage::DBEventAccessRightsMessage(const MessageID& feedback_message_id, const UidList& uids,
                                                       const AccessRightsList& direct_rights, const AccessRightsList& relation_rights)
    : Message(
        Message::create(MessageType::DBEventAccessRights, MessageCode(), feedback_message_id,
                        QVariantList {QVariant::fromValue(uids), QVariant::fromValue(direct_rights), QVariant::fromValue(relation_rights)}))
{
}

DBCommandGenerateReportMessage::DBCommandGenerateReportMessage()
    : Message()
{
}

DBCommandGenerateReportMessage::DBCommandGenerateReportMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBCommandGenerateReport));
}

DBCommandGenerateReportMessage::DBCommandGenerateReportMessage(const QString& template_id, const DataContainer& data,
                                                               const QMap<DataProperty, QString>& field_names, bool auto_map,
                                                               QLocale::Language language,
                                                               const QMap<DataProperty, QLocale::Language>& field_languages)
    : Message(Message::create(MessageType::DBCommandGenerateReport, MessageCode(), MessageID(),
                              QVariantList {template_id, data.variant(), QVariant::fromValue(field_names), auto_map, language,
                                            QVariant::fromValue(field_languages)}))
{
}

QString DBCommandGenerateReportMessage::templateId() const
{
    return isValid() && !isEmpty() ? variantList().at(0).toString() : QString();
}

DataContainer DBCommandGenerateReportMessage::data() const
{
    return isValid() && !isEmpty() ? DataContainer::fromVariant(variantList().at(1)) : DataContainer();
}

QMap<DataProperty, QString> DBCommandGenerateReportMessage::fieldNames() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QMap<DataProperty, QString>>() : QMap<DataProperty, QString>();
}

bool DBCommandGenerateReportMessage::isAutoMap() const
{
    return isValid() && !isEmpty() ? variantList().at(3).toBool() : false;
}

QLocale::Language DBCommandGenerateReportMessage::language() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<QLocale::Language>() : QLocale::AnyLanguage;
}

QMap<DataProperty, QLocale::Language> DBCommandGenerateReportMessage::fieldLanguages() const
{
    return isValid() && !isEmpty() ? variantList().at(5).value<QMap<DataProperty, QLocale::Language>>()
                                   : QMap<DataProperty, QLocale::Language>();
}

Message* DBCommandGenerateReportMessage::clone() const
{
    return new DBCommandGenerateReportMessage(*this);
}

DBEventReportMessage::DBEventReportMessage()
    : Message()
{
}

DBEventReportMessage::DBEventReportMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBEventReport));
}

DBEventReportMessage::DBEventReportMessage(const MessageID& feedback_message_id, QByteArrayPtr data)
    : Message(Message::create(MessageType::DBEventReport, MessageCode(), feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{    
}

Message* DBEventReportMessage::clone() const
{
    return new DBEventReportMessage(*this);
}

QByteArrayPtr DBEventReportMessage::data() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<QByteArrayPtr>() : QByteArrayPtr();
}

DBCommandGetCatalogInfoMessage::DBCommandGetCatalogInfoMessage()
    : Message()
{
}

DBCommandGetCatalogInfoMessage::DBCommandGetCatalogInfoMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBCommandGetCatalogInfo));
}

DBCommandGetCatalogInfoMessage::DBCommandGetCatalogInfoMessage(const Uid& catalog_uid)
    : Message(Message::create(MessageType::DBCommandGetCatalogInfo, MessageCode(), MessageID(), QVariantList {catalog_uid.variant()}))
{
}

Uid DBCommandGetCatalogInfoMessage::catalogUid() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<Uid>() : Uid();
}

Message* DBCommandGetCatalogInfoMessage::clone() const
{
    return new DBCommandGetCatalogInfoMessage(*this);
}

int DBCommandGetCatalogInfoMessage::count_I_MessageContainsUid() const
{
    return isValid() ? 1 : 0;
}

Uid DBCommandGetCatalogInfoMessage::value_I_MessageContainsUid(int index) const
{
    Z_CHECK(index == 0);
    return catalogUid();
}

DBEventCatalogInfoMessage::DBEventCatalogInfoMessage()
    : Message()
{
}

DBEventCatalogInfoMessage::DBEventCatalogInfoMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBEventCatalogInfo));
}

DBEventCatalogInfoMessage::DBEventCatalogInfoMessage(const MessageID& feedback_message_id, const CatalogInfo& info)
    : Message(
        Message::create(MessageType::DBEventCatalogInfo, MessageCode(), feedback_message_id, QVariantList {QVariant::fromValue(info)}))
{
}

CatalogInfo DBEventCatalogInfoMessage::info() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<CatalogInfo>() : CatalogInfo();
}

Message* DBEventCatalogInfoMessage::clone() const
{
    return new DBEventCatalogInfoMessage(*this);
}

DBCommandLoginMessage::DBCommandLoginMessage()
    : Message()
{
}

DBCommandLoginMessage::DBCommandLoginMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBCommandLogin));
}

DBCommandLoginMessage::DBCommandLoginMessage(const DatabaseID& database_id, const Credentials& credentials)
    : Message(Message::create(MessageType::DBCommandLogin, MessageCode(), {},
                              QVariantList {QVariant::fromValue(database_id), QVariant::fromValue(credentials)}))
{
}

DatabaseID DBCommandLoginMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

Credentials DBCommandLoginMessage::credentials() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<Credentials>() : Credentials();
}

Message* DBCommandLoginMessage::clone() const
{
    return new DBCommandLoginMessage(*this);
}

DBEventLoginMessage::DBEventLoginMessage()
    : Message()
{
}

DBEventLoginMessage::DBEventLoginMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBEventLogin));
}

DBEventLoginMessage::DBEventLoginMessage(const MessageID& feedback_message_id, const ConnectionInformation& connection_info)
    : Message(
        Message::create(MessageType::DBEventLogin, MessageCode(), feedback_message_id, QVariantList {QVariant::fromValue(connection_info)}))
{
}

ConnectionInformation DBEventLoginMessage::connectionInfo() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<ConnectionInformation>() : ConnectionInformation();
}

Message* DBEventLoginMessage::clone() const
{
    return new DBEventLoginMessage(*this);
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage()
    : Message()
{
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBCommandGetDataTable));
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const TableID& table_id, DataRestrictionPtr restriction, const FieldIdList& mask)
    : DBCommandGetDataTableMessage(Core::defaultDatabase(), table_id, restriction, FieldID(), {}, mask)
{    
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const FieldID& field_id, const QList<int>& keys, const FieldIdList& mask)
    : DBCommandGetDataTableMessage(Core::defaultDatabase(), field_id.table(), nullptr, field_id, keys, mask)
{
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const DatabaseID& database_id, const FieldID& field_id, const QList<int>& keys,
                                                           const FieldIdList& mask)
    : DBCommandGetDataTableMessage(database_id, field_id.table(), nullptr, field_id, keys, mask)
{
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const DatabaseID& database_id, const TableID& table_id,
                                                           DataRestrictionPtr restriction, const FieldIdList& mask)
    : DBCommandGetDataTableMessage(database_id, table_id, restriction, FieldID(), {}, mask)
{
}

DBCommandGetDataTableMessage::DBCommandGetDataTableMessage(const DatabaseID& database_id, const TableID& table_id,
                                                           DataRestrictionPtr restriction, const FieldID& field_id, const QList<int>& keys,
                                                           const FieldIdList& mask)
    : Message(Message::create(MessageType::DBCommandGetDataTable, MessageCode(), {},
                              QVariantList {database_id.variant(), table_id.variant(), QVariant::fromValue(mask),
                                            QVariant::fromValue(restriction), field_id.variant(), QVariant::fromValue(keys)}))
{
    Z_CHECK(database_id.isValid());
    Z_CHECK(table_id.isValid());
}

DatabaseID DBCommandGetDataTableMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DatabaseID>() : DatabaseID();
}

TableID DBCommandGetDataTableMessage::tableID() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<TableID>() : TableID();
}

FieldIdList DBCommandGetDataTableMessage::mask() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<FieldIdList>() : FieldIdList();
}

DataRestrictionPtr DBCommandGetDataTableMessage::restriction() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<DataRestrictionPtr>() : nullptr;
}

FieldID DBCommandGetDataTableMessage::fieldID() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<FieldID>() : FieldID();
}

QList<int> DBCommandGetDataTableMessage::keys() const
{
    return isValid() && !isEmpty() ? variantList().at(5).value<QList<int>>() : QList<int>();
}

Message* DBCommandGetDataTableMessage::clone() const
{
    return new DBCommandGetDataTableMessage(*this);
}

DBEventDataTableMessage::DBEventDataTableMessage()
    : Message()
{
}

DBEventDataTableMessage::DBEventDataTableMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::DBEventDataTable));
}

DBEventDataTableMessage::DBEventDataTableMessage(const MessageID& feedback_message_id, DataTablePtr data)
    : Message(Message::create(MessageType::DBEventDataTable, MessageCode(), feedback_message_id, QVariantList {QVariant::fromValue(data)}))
{
}

DataTablePtr DBEventDataTableMessage::result() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<DataTablePtr>() : nullptr;
}

Message* DBEventDataTableMessage::clone() const
{
    return new DBEventDataTableMessage(*this);
}

DBCommandGetDataTableInfoMessage::DBCommandGetDataTableInfoMessage()
    : Message()
{
}

DBCommandGetDataTableInfoMessage::DBCommandGetDataTableInfoMessage(const zf::Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == zf::MessageType::DBCommandGetDataTableInfo);
}

DBCommandGetDataTableInfoMessage::DBCommandGetDataTableInfoMessage(const zf::DatabaseID& database_id, const zf::TableID& table_id)
    : Message(Message::create(zf::MessageType::DBCommandGetDataTableInfo, MessageCode(), zf::MessageID(),
                              QVariantList {QVariant::fromValue(table_id), QVariant::fromValue(database_id)}))
{
}

zf::TableID DBCommandGetDataTableInfoMessage::tableID() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<zf::TableID>() : zf::TableID();
}

zf::DatabaseID DBCommandGetDataTableInfoMessage::databaseID() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<zf::DatabaseID>() : zf::DatabaseID();
}

zf::Message* DBCommandGetDataTableInfoMessage::clone() const
{
    return new DBCommandGetDataTableInfoMessage(*this);
}

DBEventTableInfoMessage::DBEventTableInfoMessage()
    : Message()
{
}

DBEventTableInfoMessage::DBEventTableInfoMessage(const zf::Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == zf::MessageType::DBEventDataTableInfo);
}

DBEventTableInfoMessage::DBEventTableInfoMessage(const zf::MessageID& feedback_id, const SrvTableInfoPtr& info)
    : Message(Message::create(zf::MessageType::DBEventDataTableInfo, MessageCode(), feedback_id, QVariantList {QVariant::fromValue(info)}))
{
}

SrvTableInfoPtr DBEventTableInfoMessage::info() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<SrvTableInfoPtr>() : nullptr;
}

zf::Message* DBEventTableInfoMessage::clone() const
{
    return new DBEventTableInfoMessage(*this);
}

DBCommandUpdateEntitiesMessage::DBCommandUpdateEntitiesMessage()
    : Message()
{
    
}

DBCommandUpdateEntitiesMessage::DBCommandUpdateEntitiesMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandUpdateEntities);
}

DBCommandUpdateEntitiesMessage::DBCommandUpdateEntitiesMessage(const UidList& entity_uids,
                                                               const QList<QMap<DataProperty, QVariant>>& values,
                                                               const QList<QMap<QString, QVariant>>& parameters)
    : Message(
        Message::create(MessageType::DBCommandUpdateEntities, MessageCode(), MessageID(),
                        QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(values), QVariant::fromValue(parameters)}))
{
    Z_CHECK(entity_uids.count() == values.count());
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());

    for (int i = 0; i < entity_uids.count(); i++) {
        Z_CHECK(entity_uids.at(i).isPersistent());
        for (auto p = values.at(i).constBegin(); p != values.at(i).constEnd(); ++p) {
            Z_CHECK(entity_uids.at(i).isValid());
            Z_CHECK(entity_uids.at(i).entityCode() == p.key().entityCode());
        }
    }
}

DBCommandUpdateEntitiesMessage::DBCommandUpdateEntitiesMessage(const UidList& entity_uids, const QList<QMap<PropertyID, QVariant>>& values,
                                                               const QList<QMap<QString, QVariant>>& parameters)
    : Message(create(entity_uids, values, parameters))
{    
}

UidList DBCommandUpdateEntitiesMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<QMap<DataProperty, QVariant>> DBCommandUpdateEntitiesMessage::values() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<QMap<DataProperty, QVariant>>>()
                                   : QList<QMap<DataProperty, QVariant>>();
}

QList<QMap<QString, QVariant>> DBCommandUpdateEntitiesMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

QString DBCommandUpdateEntitiesMessage::dataToText() const
{
    return Utils::containerToString(entityUids(), 10);
}

Message* DBCommandUpdateEntitiesMessage::clone() const
{
    return new DBCommandUpdateEntitiesMessage(*this);
}

int DBCommandUpdateEntitiesMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid DBCommandUpdateEntitiesMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

Message DBCommandUpdateEntitiesMessage::create(const UidList& entity_uids, const QList<QMap<PropertyID, QVariant>>& values,
                                               const QList<QMap<QString, QVariant>>& parameters)
{
    Z_CHECK(entity_uids.count() == values.count());
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());

    QList<QMap<DataProperty, QVariant>> vals;
    for (int i = 0; i < entity_uids.count(); i++) {
        auto structure = DataStructure::structure(entity_uids.at(i));
        QMap<DataProperty, QVariant> data;
        for (auto p = values.at(i).constBegin(); p != values.at(i).constEnd(); ++p) {
            data[structure->property(p.key())] = p.value();
        }
        vals << data;
    }

    return Message::create(MessageType::DBCommandUpdateEntities, MessageCode(), MessageID(),
                           QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(vals), QVariant::fromValue(parameters)});
}

DBCommandReconnectMessage::DBCommandReconnectMessage(bool is_valid)
    : Message(create(is_valid))
{
}

DBCommandReconnectMessage::DBCommandReconnectMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || m.messageType() == MessageType::DBCommandReconnect);
}

Message* DBCommandReconnectMessage::clone() const
{
    return new DBCommandReconnectMessage(*this);
}

Message DBCommandReconnectMessage::create(bool is_valid)
{
    if (is_valid)
        return Message::create(MessageType::DBCommandReconnect, MessageCode(), MessageID(), QVariantList {QVariant(is_valid)});

    return Message();
}

} // namespace zf
