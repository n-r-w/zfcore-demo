#include "zf_mm_messages.h"
#include "zf_core.h"

namespace zf
{
MMEventModelListMessage::MMEventModelListMessage()
    : ModelListMessage()
{
}

MMEventModelListMessage::MMEventModelListMessage(const Message& m)
    : ModelListMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMEventModelList));
}

MMEventModelListMessage::MMEventModelListMessage(const MessageID& feedback_message_id, const QList<ModelPtr>& models)
    : ModelListMessage(MessageType::MMEventModelList, feedback_message_id, models)
{
}

MMCommandGetModelsMessage::MMCommandGetModelsMessage()
    : Message()
{
}

MMCommandGetModelsMessage::MMCommandGetModelsMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMCommandGetModels));
}

MMCommandGetModelsMessage::MMCommandGetModelsMessage(const UidList& entity_uids, const QList<LoadOptions>& load_options_list,
                                                     const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list)
    : Message(Message::create(MessageType::MMCommandGetModels, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(load_options_list),
                                            QVariant::fromValue(properties_list), QVariant::fromValue(all_if_empty_list)}))
{
}

MMCommandGetModelsMessage::MMCommandGetModelsMessage(const Uid& entity_uid, const LoadOptions& load_options,
                                                     const DataPropertySet& properties, bool all_if_empty)
    : MMCommandGetModelsMessage(UidList {entity_uid}, QList<LoadOptions> {load_options}, QList<DataPropertySet> {properties},
                                QList<bool> {all_if_empty})
{
}

UidList MMCommandGetModelsMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<LoadOptions> MMCommandGetModelsMessage::loadOptions() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<LoadOptions>>() : QList<LoadOptions>();
}

QList<DataPropertySet> MMCommandGetModelsMessage::properties() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QList<DataPropertySet>>() : QList<DataPropertySet>();
}

QList<bool> MMCommandGetModelsMessage::allIfEmptyProperties() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<QList<bool>>() : QList<bool>();
}

Message* MMCommandGetModelsMessage::clone() const
{
    return new MMCommandGetModelsMessage(*this);
}

QString MMCommandGetModelsMessage::dataToText() const
{
    return Utils::uidsPropsToString(entityUids(), properties());
}

int MMCommandGetModelsMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid MMCommandGetModelsMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

ModelInvalideMessage::ModelInvalideMessage()
    : EntityUidListMessage()
{
}

ModelInvalideMessage::ModelInvalideMessage(const Message& m)
    : EntityUidListMessage(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::ModelInvalide));
}

ModelInvalideMessage::ModelInvalideMessage(const UidList& entity_uids)
    : EntityUidListMessage(MessageType::ModelInvalide, entity_uids, MessageID(), QList<QMap<QString, QVariant>>(), QVariant())
{
}

MMCommandRemoveModelsMessage::MMCommandRemoveModelsMessage()
    : Message()
{
}

MMCommandRemoveModelsMessage::MMCommandRemoveModelsMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMCommandRemoveModels));
}

MMCommandRemoveModelsMessage::MMCommandRemoveModelsMessage(const UidList& entity_uids, const QList<DataPropertySet>& properties,
                                                           const QList<DataPropertySet>& properties_list,
                                                           const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& by_user)
    : Message(
        Message::create(MessageType::MMCommandRemoveModels, MessageCode(), MessageID(),
                        QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(properties),
                                      QVariant::fromValue(properties_list), QVariant::fromValue(by_user), QVariant::fromValue(parameters)}))
{
    Z_CHECK(properties.isEmpty() || properties.count() == entity_uids.count());
    Z_CHECK(properties_list.isEmpty() || properties_list.count() == entity_uids.count());
    Z_CHECK(by_user.isEmpty() || by_user.count() == entity_uids.count());
    Z_CHECK(parameters.isEmpty() || parameters.count() == entity_uids.count());
}

UidList MMCommandRemoveModelsMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QList<DataPropertySet> MMCommandRemoveModelsMessage::properties() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QList<DataPropertySet>>() : QList<DataPropertySet>();
}

QList<QMap<QString, QVariant>> MMCommandRemoveModelsMessage::parameters() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<QList<QMap<QString, QVariant>>>() : QList<QMap<QString, QVariant>>();
}

QList<bool> MMCommandRemoveModelsMessage::allIfEmptyProperties() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QList<bool>>() : QList<bool>();
}

QList<bool> MMCommandRemoveModelsMessage::byUser() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<QList<bool>>() : QList<bool>();
}

Message* MMCommandRemoveModelsMessage::clone() const
{
    return new MMCommandRemoveModelsMessage(*this);
}

QString MMCommandRemoveModelsMessage::dataToText() const
{
    return Utils::uidsPropsToString(entityUids(), properties());
}

int MMCommandRemoveModelsMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid MMCommandRemoveModelsMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

MMCommandGetEntityNamesMessage::MMCommandGetEntityNamesMessage()
    : Message()
{
}

MMCommandGetEntityNamesMessage::MMCommandGetEntityNamesMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMCommandGetEntityNames));
}

MMCommandGetEntityNamesMessage::MMCommandGetEntityNamesMessage(const UidList& entity_uids, const DataPropertyList& name_properties,
                                                               QLocale::Language language)
    : Message(Message::create(
        MessageType::MMCommandGetEntityNames, MessageCode(), MessageID(),
        QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(name_properties), QVariant::fromValue(language)}))
{
    Z_CHECK(!entity_uids.isEmpty());
    Z_CHECK(name_properties.isEmpty() || name_properties.count() == entity_uids.count());
}

UidList MMCommandGetEntityNamesMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

DataPropertyList MMCommandGetEntityNamesMessage::nameProperties() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<DataPropertyList>() : DataPropertyList();
}

QLocale::Language MMCommandGetEntityNamesMessage::language() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<QLocale::Language>() : QLocale::AnyLanguage;
}

Message* MMCommandGetEntityNamesMessage::clone() const
{
    return new MMCommandGetEntityNamesMessage(*this);
}

QString MMCommandGetEntityNamesMessage::dataToText() const
{
    return Utils::containerToString(entityUids(), 10);
}

int MMCommandGetEntityNamesMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid MMCommandGetEntityNamesMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

MMEventEntityNamesMessage::MMEventEntityNamesMessage()
    : Message()
{
}

MMEventEntityNamesMessage::MMEventEntityNamesMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMEventEntityNames));
}

MMEventEntityNamesMessage::MMEventEntityNamesMessage(const MessageID& feedback_message_id, const UidList& entity_uids,
                                                     const QStringList& names, const ErrorList& errors)
    : Message(Message::create(MessageType::MMEventEntityNames, MessageCode(), feedback_message_id,
                              QVariantList {QVariant::fromValue(entity_uids), QVariant::fromValue(names), QVariant::fromValue(errors)}))
{
    Z_CHECK(!names.isEmpty() && (errors.isEmpty() || names.count() == errors.count()));
}

UidList MMEventEntityNamesMessage::entityUids() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<UidList>() : UidList();
}

QStringList MMEventEntityNamesMessage::names() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<QStringList>() : QStringList();
}

ErrorList MMEventEntityNamesMessage::errors() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<ErrorList>() : ErrorList();
}

Message* MMEventEntityNamesMessage::clone() const
{
    return new MMEventEntityNamesMessage(*this);
}

QString MMEventEntityNamesMessage::dataToText() const
{
    return Utils::containerToString(entityUids());
}

int MMEventEntityNamesMessage::count_I_MessageContainsUid() const
{
    return entityUids().count();
}

Uid MMEventEntityNamesMessage::value_I_MessageContainsUid(int index) const
{
    return entityUids().at(index);
}

MMCommandGetCatalogValueMessage::MMCommandGetCatalogValueMessage()
    : Message()
{
}

MMCommandGetCatalogValueMessage::MMCommandGetCatalogValueMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMCommandGetCatalogValue));
}

MMCommandGetCatalogValueMessage::MMCommandGetCatalogValueMessage(const EntityCode& catalog_id, const EntityID& id,
                                                                 const PropertyID& property_id, QLocale::Language language,
                                                                 const DatabaseID& database_id)
    : Message(Message::create(MessageType::MMCommandGetCatalogValue, MessageCode(), MessageID(),
                              QVariantList {QVariant::fromValue(catalog_id), QVariant::fromValue(id),
                                            QVariant::fromValue(Core::catalogProperty(catalog_id, property_id, database_id)),
                                            QVariant::fromValue(database_id), QVariant::fromValue(language)}))
{
}

EntityCode MMCommandGetCatalogValueMessage::catalogId() const
{
    return isValid() && !isEmpty() ? variantList().at(0).value<EntityCode>() : EntityCode();
}

EntityID MMCommandGetCatalogValueMessage::id() const
{
    return isValid() && !isEmpty() ? variantList().at(1).value<EntityID>() : EntityID();
}

DataProperty MMCommandGetCatalogValueMessage::property() const
{
    return isValid() && !isEmpty() ? variantList().at(2).value<DataProperty>() : DataProperty();
}

DatabaseID MMCommandGetCatalogValueMessage::databaseId() const
{
    return isValid() && !isEmpty() ? variantList().at(3).value<DatabaseID>() : DatabaseID();
}

QLocale::Language MMCommandGetCatalogValueMessage::language() const
{
    return isValid() && !isEmpty() ? variantList().at(4).value<QLocale::Language>() : QLocale::AnyLanguage;
}

Message* MMCommandGetCatalogValueMessage::clone() const
{
    return new MMCommandGetCatalogValueMessage(*this);
}

QString MMCommandGetCatalogValueMessage::dataToText() const
{
    return catalogId().string();
}

MMEventCatalogValueMessage::MMEventCatalogValueMessage()
    : Message()
{
}

MMEventCatalogValueMessage::MMEventCatalogValueMessage(const Message& m)
    : Message(m)
{
    Z_CHECK(!isValid() || !m.isValid() || (m.messageType() == MessageType::MMEventCatalogValue));
}

MMEventCatalogValueMessage::MMEventCatalogValueMessage(const MessageID& feedback_message_id, const QVariant& value)
    : Message(Message::create(MessageType::MMEventCatalogValue, MessageCode(), feedback_message_id, QVariantList {value}))
{
}

QVariant MMEventCatalogValueMessage::value() const
{
    return isValid() && !isEmpty() ? variantList().at(0) : QVariant();
}

Message* MMEventCatalogValueMessage::clone() const
{
    return new MMEventCatalogValueMessage(*this);
}

} // namespace zf
