#include "zf_data_structure.h"

#include "zf_core.h"
#include "zf_model.h"
#include "zf_translation.h"
#include "zf_framework.h"
#include "services/item_model/item_model_service.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>

namespace zf
{
//! Хэшированные свойства для быстрого поиска через property()
std::unique_ptr<QHash<QString, DataProperty>> DataProperty::_property_hash;
QMutex DataProperty::_property_hash_mutex;
int DataProperty::_metatype = 0;

//! Данные для DataProperty
class DataProperty_SharedData : public QSharedData
{
public:
    DataProperty_SharedData();
    DataProperty_SharedData(DataProperty_SharedData& d)
        : QSharedData(d)
    {
        copyFrom(&d);
    }
    DataProperty_SharedData(const PropertyID& id, PropertyType property_type, DataType data_type, const PropertyOptions& options, const DataProperty& entity,
        const EntityCode& entity_code, const Uid& entity_uid, const DataProperty& dataset, const DataProperty& column_group, const QVariant& default_value,
        const DataProperty& row, const RowID& row_id, const DataProperty& column, const QString& translation_id);
    ~DataProperty_SharedData();

    void copyFrom(const DataProperty_SharedData* d);

    void updateUniqueKey();

    static DataProperty_SharedData* shared_null();

    //! Идентификатор свойства
    PropertyID id;
    //! Тип данных
    DataType data_type = DataType::Undefined;
    //! Параметры
    PropertyOptions options = PropertyOption::NoOptions;
    //! Идентификатор перевода (название свойства)
    QString translation_id;

    //! Значение по умолчанию (для всех, кроме PropertyType::Dataset, PropertyType::Model)
    QVariant default_value;

    //! Вид свойства
    PropertyType property_type = PropertyType::Undefined;

    //! Сущность - владелец (для свойств, связанных с сущностями)
    DataPropertyPtr entity;
    //! Код сущности - владельца (для свойств, связанных с сущностями)
    EntityCode entity_code;

    //! Идентификатор сущности. Для специфических свойств, структура которых зависит от идентификатора
    Uid entity_uid;

    //! Набор данных (для колонок, строк и ячеек наборов данных)
    DataPropertyPtr dataset;

    //! Группа колонок (для колонок, входящих в группу)
    DataPropertyPtr column_group;

    //! Строка (для свойств, связанных с наборами данных)
    DataPropertyPtr row;
    //! Идентификатор строки (для свойств, связанных с наборами данных)
    RowID row_id;

    //! Колонка (для свойств, связанных с наборами данных)
    DataPropertyPtr column;

    //! Колонки набора данных (для PropertyType::Dataset)
    QList<DataProperty> columns;
    //! Колонки набора данных в которых есть ограничения (только для PropertyType::Dataset)
    QList<DataProperty> columns_constraint;
    //! Идентификаторы колонок по порядку в columns (для PropertyType::Dataset)
    PropertyIDList columns_id;

    //! Уникальный ключ (комбинация id, owner_code и т.д.)
    QString unique_key;
    //! Ключ для работы qHash
    uint hash_key = 0;

    //! Откуда должен производится выбор значений
    PropertyLookupPtr lookup;
    //! Ограничения
    QList<PropertyConstraintPtr> constraints;
    //! Есть ли ограничения
    bool has_constraints = false;

    //! Связи с другими свойствами
    QList<PropertyLinkPtr> links;

    //! Идентификатор первой колонки с признаком PropertyOption::Id
    PropertyID column_id_option;

    mutable int natural_index = -1;
};

Q_GLOBAL_STATIC(DataProperty_SharedData, nullResult)
DataProperty_SharedData* DataProperty_SharedData::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0) {
        res->ref.ref(); // чтобы не было удаления самого nullResult
    }
    return res;
}

DataProperty_SharedData::DataProperty_SharedData()
{
    Z_DEBUG_NEW("DataProperty_SharedData");
}

DataProperty_SharedData::DataProperty_SharedData(const PropertyID& id, PropertyType property_type, DataType data_type, const PropertyOptions& options,
    const DataProperty& entity, const EntityCode& entity_code, const Uid& entity_uid, const DataProperty& dataset, const DataProperty& column_group,
    const QVariant& default_value, const DataProperty& row, const RowID& row_id, const DataProperty& column, const QString& translation_id)
    : id(id)
    , data_type(data_type)
    , options(options)
    , translation_id(translation_id)
    , default_value(default_value)
    , property_type(property_type)
    , entity_code(entity_code)
    , entity_uid(entity_uid)
    , row_id(row_id)
{
    Z_CHECK(id.isValid());

    if (DataProperty::isDataTypeRequired(property_type))
        Z_CHECK(data_type != DataType::Undefined);

    this->options = options;

    if (options & PropertyOption::ForceReadOnly) {
        this->options.setFlag(PropertyOption::ReadOnly, true);
    }

    if (options.testFlag(PropertyOption::Id) || options.testFlag(PropertyOption::ParentId)) {
        this->options.setFlag(PropertyOption::Hidden, true);
    }

    if (options & PropertyOption::Hidden) {
        this->options.setFlag(PropertyOption::Filtered, false);
    }

    if (options.testFlag(PropertyOption::Id) && translation_id.isEmpty())
        this->translation_id = TR::ZFT_ID;

    if (entity.isValid())
        this->entity = Z_MAKE_SHARED(DataProperty, entity);
    if (dataset.isValid())
        this->dataset = Z_MAKE_SHARED(DataProperty, dataset);
    if (column_group.isValid())
        this->column_group = Z_MAKE_SHARED(DataProperty, column_group);
    if (row.isValid())
        this->row = Z_MAKE_SHARED(DataProperty, row);
    if (column.isValid())
        this->column = Z_MAKE_SHARED(DataProperty, column);

    updateUniqueKey();

    Z_DEBUG_NEW("DataProperty_SharedData");
}

DataProperty_SharedData::~DataProperty_SharedData()
{
    Z_DEBUG_DELETE("DataProperty_SharedData");
}

void DataProperty_SharedData::copyFrom(const DataProperty_SharedData* d)
{
    id = d->id;
    property_type = d->property_type;
    data_type = d->data_type;
    options = d->options;
    entity = d->entity;
    entity_code = d->entity_code;
    entity_uid = d->entity_uid;
    dataset = d->dataset;
    column_group = d->column_group;
    default_value = d->default_value;
    translation_id = d->translation_id;
    columns = d->columns;
    columns_id = d->columns_id;
    row = d->row;
    row_id = d->row_id;
    column = d->column;
    unique_key = d->unique_key;
    hash_key = d->hash_key;
    lookup = d->lookup;
    constraints = d->constraints;
    has_constraints = d->has_constraints;
    links = d->links;
    column_id_option = d->column_id_option;

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    if (d == shared_null()) {
        translation_id.clear();
    }
#endif
}

void DataProperty_SharedData::updateUniqueKey()
{
    if (property_type == PropertyType::Entity) {
        unique_key = entity_uid.isValid() ? entity_uid.serializeToString() : QString::number(entity_code.value());

    } else if (property_type == PropertyType::Dataset) {
        unique_key = (entity == nullptr ? QString() : entity->uniqueKey()) + Consts::KEY_SEPARATOR + QString::number(static_cast<int>(property_type))
                     + Consts::KEY_SEPARATOR + QString::number(id.value());

    } else if (property_type == PropertyType::Field) {
        unique_key = (entity == nullptr ? QString() : entity->uniqueKey()) + Consts::KEY_SEPARATOR + QString::number(static_cast<int>(property_type))
                     + Consts::KEY_SEPARATOR + QString::number(id.value());

    } else if (property_type == PropertyType::Cell) {
        Z_CHECK(row->isRow() && column->isColumn());
        Z_CHECK(row->dataset() == column->dataset());

        unique_key = row->uniqueKey() + Consts::KEY_SEPARATOR + column->uniqueKey();

    } else if (property_type == PropertyType::RowFull || property_type == PropertyType::RowPartial) {
        Z_CHECK(row_id.isValid());

        if (row != nullptr && column != nullptr)
            Z_CHECK(row->dataset() == column->dataset());

        unique_key = (dataset == nullptr ? QString() : dataset->uniqueKey()) + Consts::KEY_SEPARATOR + row_id.uniqueKey() + Consts::KEY_SEPARATOR
                     + QString::number(static_cast<int>(property_type));

    } else if (property_type == PropertyType::ColumnFull || property_type == PropertyType::ColumnPartial || property_type == PropertyType::ColumnGroup) {
        unique_key = (dataset == nullptr ? QString() : dataset->uniqueKey()) + Consts::KEY_SEPARATOR
                     + (column_group == nullptr ? QString() : column_group->uniqueKey()) + Consts::KEY_SEPARATOR + QString::number(id.value())
                     + Consts::KEY_SEPARATOR + QString::number(static_cast<int>(property_type));

    } else {
        Z_HALT_INT;
    }

    hash_key = ::qHash(unique_key);
}

DataProperty::DataProperty()
    : _d(DataProperty_SharedData::shared_null())
{
}

DataProperty::DataProperty(const DataProperty& p)
    : _d(p._d)
{
}

DataProperty::DataProperty(const PropertyID& id, PropertyType property_type, DataType data_type, const PropertyOptions& options, const DataProperty& entity,
    const EntityCode& entity_code, const Uid& entity_uid, const DataProperty& dataset, const DataProperty& column_group, const QVariant& default_value,
    const DataProperty& row, const RowID& row_id, const DataProperty& column, const QString& translation_id)
    : _d(new DataProperty_SharedData(
        id, property_type, data_type, options, entity, entity_code, entity_uid, dataset, column_group, default_value, row, row_id, column, translation_id))
{
    if (options.testFlag(PropertyOption::Required)) {
        Z_CHECK(property_type == PropertyType::Field || property_type == PropertyType::ColumnFull);
        setConstraintRequired();
    }
}

DataProperty::~DataProperty()
{
}

QVariant DataProperty::toVariant() const
{
    return QVariant::fromValue(*this);
}

DataProperty DataProperty::fromVariant(const QVariant& v)
{
    return v.value<DataProperty>();
}

PropertyIDList DataProperty::toIDList(const DataPropertyList& props)
{
    PropertyIDList res;
    for (auto& p : props) {
        res << p.id();
    }
    return res;
}

PropertyIDList DataProperty::toIDList(const DataPropertySet& props)
{
    return toIDList(props.toList());
}

DataPropertyList DataStructure::fromIDList(const PropertyIDList& props) const
{
    DataPropertyList res;
    for (auto& p : props) {
        res << property(p);
    }
    return res;
}

PropertyIDList DataStructure::fromPropertyList(const DataPropertyList& props)
{
    PropertyIDList res;
    for (auto& p : props) {
        res << p.id();
    }
    return res;
}

DataProperty DataProperty::property(const EntityCode& code, const PropertyID& property_id, bool halt_if_not_found)
{
    QMutexLocker lock(&_property_hash_mutex);

    if (_property_hash == nullptr)
        _property_hash = std::make_unique<QHash<QString, DataProperty>>();

    QString key = QString::number(code.value()) + Consts::KEY_SEPARATOR + QString::number(property_id.value());
    DataProperty prop = _property_hash->value(key);
    if (!prop.isValid()) {
        auto data_struct = DataStructure::structure(code, halt_if_not_found);
        if (data_struct == nullptr) {
            Z_CHECK(!halt_if_not_found);
            return DataProperty();
        }

        prop = data_struct->property(property_id, halt_if_not_found);
        if (!prop.isValid()) {
            Z_CHECK(!halt_if_not_found);
            return DataProperty();
        }
        _property_hash->insert(key, prop);
    }

    return prop;
}

DataProperty DataProperty::property(const Uid& entity_uid, const PropertyID& property_id, bool halt_if_not_found)
{
    auto data_struct = DataStructure::structure(entity_uid, halt_if_not_found);
    if (data_struct == nullptr) {
        Z_CHECK(!halt_if_not_found);
        return DataProperty();
    }

    return data_struct->property(property_id, halt_if_not_found);
}

DataPropertyList DataProperty::propertiesByOptions(const EntityCode& code, PropertyType type, const PropertyOptions& options)
{
    return DataStructure::structure(code)->propertiesByOptions(type, options);
}

DataPropertyList DataProperty::propertiesByOptions(const Uid& entity_uid, PropertyType type, const PropertyOptions& options)
{
    return DataStructure::structure(entity_uid)->propertiesByOptions(type, options);
}

DataPropertyList DataProperty::propertiesByType(const EntityCode& entity_code, PropertyType type)
{
    return DataStructure::structure(entity_code)->propertiesByType(type);
}

DataPropertyList DataProperty::propertiesByType(const Uid& entity_uid, PropertyType type)
{
    return DataStructure::structure(entity_uid)->propertiesByType(type);
}

Uid DataProperty::findPropertyUid(const DataProperty& property, bool halt_if_not_found)
{
    Z_CHECK(property.isValid());
    zf::Error error;
    auto plugin = Core::getPlugin(property.entityCode(), error);
    if (error.isError()) {
        if (halt_if_not_found)
            Z_HALT(error);
        else
            return Uid();
    }

    if (plugin->getModuleInfo().type() == ModuleType::Unique)
        return plugin->getModuleInfo().uid();

    auto created = plugin->createdStructure();
    for (auto it = created.constBegin(); it != created.constEnd(); ++it) {
        if (it.value()->contains(property))
            return it.key();
    }

    if (halt_if_not_found)
        Z_HALT(QString("Property not found %1:%2").arg(property.entityCode().value()).arg(property.id().value()));

    return Uid();
}

DataStructurePtr DataProperty::findStructure(const DataProperty& property, bool halt_if_not_found)
{
    Z_CHECK(property.isValid());
    zf::Error error;
    auto plugin = Core::getPlugin(property.entityCode(), error);
    if (error.isError()) {
        if (halt_if_not_found)
            Z_HALT(error);
        else
            return nullptr;
    }

    if (plugin->getModuleInfo().type() == ModuleType::Unique) {
        auto structure = plugin->dataStructure(Uid(), error);
        if (error.isError()) {
            if (halt_if_not_found)
                Z_HALT(error);
            else
                return nullptr;
        }
        return structure;
    }

    auto created = plugin->createdStructure();
    for (auto it = created.constBegin(); it != created.constEnd(); ++it) {
        if (it.value()->contains(property))
            return it.value();
    }

    if (halt_if_not_found)
        Z_HALT(QString("Property not found %1:%2").arg(property.entityCode().value()).arg(property.id().value()));

    return nullptr;
}

DataPropertyList DataProperty::datasetColumnsByOptions(const EntityCode& entity_code, const PropertyID& dataset_property_id, const PropertyOptions& options)
{
    return DataStructure::structure(entity_code)->datasetColumnsByOptions(dataset_property_id, options);
}

DataPropertyList DataProperty::datasetColumnsByOptions(const Uid& entity_uid, const PropertyID& dataset_property_id, const PropertyOptions& options)
{
    return DataStructure::structure(entity_uid)->datasetColumnsByOptions(dataset_property_id, options);
}

DataProperty DataProperty::independentFieldProperty(
    const PropertyID& id, DataType data_type, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(data_type != DataType::Table && data_type != DataType::Tree);

    return DataProperty(id, PropertyType::Field, data_type, options, DataProperty::independentEntityProperty(), EntityCode(), Uid(), DataProperty(),
        DataProperty(), default_value, DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::independentDatasetProperty(const PropertyID& id, DataType data_type, const QString& translation_id, const PropertyOptions& options)
{
    Z_CHECK(data_type == DataType::Table || data_type == DataType::Tree);

    return DataProperty(id, PropertyType::Dataset, data_type, options, DataProperty::independentEntityProperty(), EntityCode(), Uid(), DataProperty(),
        DataProperty(), {}, DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::independentEntityProperty()
{
    return DataProperty(PropertyID(Consts::INDEPENDENT_ENTITY_CODE), PropertyType::Entity, DataType::Undefined, PropertyOptions(), DataProperty(),
        EntityCode(Consts::INDEPENDENT_ENTITY_CODE), Uid(), DataProperty(), DataProperty(), QVariant(), DataProperty(), RowID(), DataProperty(), QString());
}

DataProperty DataProperty::entityProperty(const EntityCode& entity_code, const QString& translation_id, const PropertyOptions& options)
{
    return DataProperty(PropertyID(entity_code.value()), PropertyType::Entity, DataType::Undefined, options, DataProperty(), entity_code, Uid(), DataProperty(),
        DataProperty(), QVariant(), DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::entityProperty(const Uid& entity_uid, const QString& translation_id, const PropertyOptions& options)
{
    Z_CHECK(entity_uid.isValid());
    return DataProperty(PropertyID(entity_uid.entityCode().value()), PropertyType::Entity, DataType::Undefined, options, DataProperty(),
        entity_uid.entityCode(), entity_uid, DataProperty(), DataProperty(), QVariant(), DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::fieldProperty(const DataProperty& entity, const PropertyID& id, DataType data_type, const QString& translation_id,
    const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(entity._d->property_type == PropertyType::Entity && data_type != DataType::Table && data_type != DataType::Tree);

    return DataProperty(id, PropertyType::Field, data_type, options, entity, entity.entityCode(), entity.entityUid(), DataProperty(), DataProperty(),
        default_value, DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::datasetProperty(
    const DataProperty& entity, const PropertyID& id, DataType data_type, const QString& translation_id, const PropertyOptions& options)
{
    Z_CHECK(entity._d->property_type == PropertyType::Entity && (data_type == DataType::Table || data_type == DataType::Tree));

    return DataProperty(id, PropertyType::Dataset, data_type, options, entity, entity.entityCode(), entity.entityUid(), DataProperty(), DataProperty(),
        QVariant(), DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::cellProperty(
    const DataProperty& row, const DataProperty& column, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(row.isRow() && column.isColumn() && row.dataset() == column.dataset());

    return DataProperty(PropertyID(Consts::MINUMUM_PROPERTY_ID), PropertyType::Cell, column.dataType(), options, column.dataset().entity(),
        column.dataset().entityCode(), column.dataset().entityUid(), column.dataset(), DataProperty(), default_value, row, row.rowId(), column, translation_id);
}

DataProperty DataProperty::rowProperty(
    const DataProperty& dataset, const RowID& row_id, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(dataset._d->property_type == PropertyType::Dataset && row_id.isValid());

    return DataProperty(PropertyID(Consts::MINUMUM_PROPERTY_ID), PropertyType::RowFull, DataType::Undefined, options, dataset.entity(), dataset.entityCode(),
        dataset.entityUid(), dataset, DataProperty(), default_value, DataProperty(), row_id, DataProperty(), translation_id);
}

DataProperty DataProperty::columnProperty(const DataProperty& dataset, const PropertyID& id, DataType data_type, const QString& translation_id,
    const QVariant& default_value, const PropertyOptions& options, const DataProperty& column_group)
{
    Z_CHECK(dataset._d->property_type == PropertyType::Dataset);
    if (column_group.isValid())
        Z_CHECK(column_group.dataset() == dataset);

    return DataProperty(id, PropertyType::ColumnFull, data_type, options, dataset.entity(), dataset.entityCode(), dataset.entityUid(), dataset, column_group,
        default_value, DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty DataProperty::columnGroupProperty(const DataProperty& dataset, const PropertyID& id)
{
    Z_CHECK(dataset._d->property_type == PropertyType::Dataset);

    return DataProperty(id, PropertyType::ColumnGroup, DataType::Undefined, PropertyOptions(), dataset.entity(), dataset.entityCode(), dataset.entityUid(),
        dataset, DataProperty(), QVariant(), DataProperty(), RowID(), DataProperty(), QString());
}

DataProperty DataProperty::rowPartialProperty(
    const DataProperty& dataset, const RowID& row_id, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(dataset._d->property_type == PropertyType::Dataset && row_id.isValid());

    return DataProperty(PropertyID(Consts::MINUMUM_PROPERTY_ID), PropertyType::RowPartial, DataType::Undefined, options, dataset.entity(), dataset.entityCode(),
        dataset.entityUid(), dataset, DataProperty(), default_value, DataProperty(), row_id, DataProperty(), translation_id);
}

DataProperty DataProperty::columnPartialProperty(const DataProperty& dataset, const PropertyID& id, DataType data_type, const QString& translation_id,
    const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(dataset._d->property_type == PropertyType::Dataset);

    return DataProperty(id, PropertyType::ColumnPartial, data_type, options, dataset.entity(), dataset.entityCode(), dataset.entityUid(), dataset,
        DataProperty(), default_value, DataProperty(), RowID(), DataProperty(), translation_id);
}

DataProperty& DataProperty::operator=(const DataProperty& p)
{
    if (this != &p) {
        detachHelper();
        _d = p._d;
    }

    return *this;
}

bool DataProperty::operator==(const DataProperty& p) const
{
    return _d == p._d || _d->unique_key == p._d->unique_key;
}

bool DataProperty::operator!=(const DataProperty& p) const
{
    return !operator==(p);
}

bool DataProperty::operator<(const DataProperty& p) const
{
    return _d->unique_key < p._d->unique_key;
}

bool DataProperty::isValid() const
{
    return _d->property_type != PropertyType::Undefined;
}

bool DataProperty::canHaveValue() const
{
    return isValid() && (_d->property_type == PropertyType::Field || _d->property_type == PropertyType::Cell);
}

bool DataProperty::isEntityIndependent() const
{
    return isValid() && _d->entity_code == Consts::INDEPENDENT_ENTITY_CODE;
}

bool DataProperty::isEntity() const
{
    return isValid() && (_d->property_type == PropertyType::Entity);
}

bool DataProperty::isField() const
{
    return isValid() && (_d->property_type == PropertyType::Field);
}

bool DataProperty::isDataset() const
{
    return isValid() && (_d->property_type == PropertyType::Dataset);
}

bool DataProperty::isCell() const
{
    return isValid() && (_d->property_type == PropertyType::Cell);
}

bool DataProperty::isRow() const
{
    return isValid() && (_d->property_type == PropertyType::RowFull || _d->property_type == PropertyType::RowPartial);
}

bool DataProperty::isColumn() const
{
    return isValid() && (_d->property_type == PropertyType::ColumnFull || _d->property_type == PropertyType::ColumnPartial);
}

bool DataProperty::isDatasetPart() const
{
    return isColumn() || isRow() || _d->property_type == PropertyType::Cell;
}

PropertyID DataProperty::id() const
{
    return _d->id;
}

PropertyType DataProperty::propertyType() const
{
    return _d->property_type;
}

DataType DataProperty::dataType() const
{
    return _d->data_type;
}

PropertyOptions DataProperty::options() const
{
    return _d->options;
}

EntityCode DataProperty::entityCode() const
{
    return _d->entity_code;
}

DataProperty DataProperty::entity() const
{
    if (_d->property_type == PropertyType::Entity)
        return *this;
    return _d->entity == nullptr ? DataProperty() : *_d->entity;
}

Uid DataProperty::entityUid() const
{
    return _d->entity_uid;
}

DataStructurePtr DataProperty::structure(bool halt_if_not_found) const
{
    if (halt_if_not_found) {
        Z_CHECK(!isEntityIndependent());
        Z_CHECK(entityCode().isValid());
    }

    if (entityUid().isValid())
        return DataStructure::structure(entityUid());
    if (entityCode().isValid())
        return DataStructure::structure(entityCode());
    return nullptr;
}

QString DataProperty::toPrintable() const
{
    if (!isValid())
        return QStringLiteral("invalid");

    QString res;
    if (entityUid().isValid())
        res = QStringLiteral("%1:%2").arg(entityUid().toPrintable()).arg(id().value());
    if (entity().isValid())
        res = QStringLiteral("%1:%2").arg(entityCode().value()).arg(id().value());

    if (res.isEmpty())
        return QString::number(id().value());

    if (rowId().isValid())
        return res + QStringLiteral("; ") + rowId().toPrintable();

    return res;
}

QString DataProperty::uniqueKey() const
{
    return _d->unique_key;
}

QVariant DataProperty::defaultValue() const
{
    return _d->default_value;
}

DataProperty DataProperty::dataset() const
{
    if (_d->property_type == PropertyType::Dataset)
        return *this;
    return _d->dataset == nullptr ? DataProperty() : *_d->dataset;
}

DataProperty DataProperty::columnGroup() const
{
    if (_d->property_type == PropertyType::ColumnGroup)
        return *this;
    return _d->column_group == nullptr ? DataProperty() : *_d->column_group;
}

DataProperty DataProperty::row() const
{
    if (_d->property_type == PropertyType::RowFull || _d->property_type == PropertyType::RowPartial)
        return *this;
    return _d->row == nullptr ? DataProperty() : *_d->row;
}

const RowID& DataProperty::rowId() const
{
    return _d->row_id;
}

DataProperty DataProperty::column() const
{
    if (_d->property_type == PropertyType::ColumnFull || _d->property_type == PropertyType::ColumnPartial)
        return *this;
    return _d->column == nullptr ? DataProperty() : *_d->column;
}

void DataProperty::setColumns(const DataPropertyList& columns)
{
    detachHelper();

    Z_CHECK(_d->property_type == PropertyType::Dataset || _d->property_type == PropertyType::ColumnGroup);
    Z_CHECK(!columns.isEmpty());

    _d->columns_id.clear();
    _d->column_id_option.clear();

    bool has_filter = false;
    for (auto& p : columns) {
        Z_CHECK(p._d->property_type == PropertyType::ColumnFull);
        Z_CHECK(p.dataset() == *this);
        Z_CHECK(!_d->columns_id.contains(p.id()));
        _d->columns_id << p.id();

        if (p.options().testFlag(PropertyOption::Filtered))
            has_filter = true;

        if (!_d->column_id_option.isValid() && p.options().testFlag(PropertyOption::Id))
            _d->column_id_option = p.id();
    }

    _d->options.setFlag(PropertyOption::Filtered, has_filter);
    _d->columns = columns;
}

DataProperty DataProperty::addColumn(const DataProperty& column)
{
    setColumns(DataPropertyList(columns()) << column);
    return column;
}

DataProperty& DataProperty::operator<<(const DataProperty& column)
{
    addColumn(column);
    return *this;
}

DataProperty DataProperty::addColumn(
    const PropertyID& id, DataType data_type, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    Z_CHECK(_d->property_type == PropertyType::Dataset || _d->property_type == PropertyType::ColumnGroup);
    return addColumn(DataProperty::columnProperty(*this, id, data_type, translation_id, default_value, options));
}

QString DataProperty::translationID() const
{
    return _d->translation_id;
}

QString DataProperty::name(bool show_translation_warning) const
{
    if (_d->translation_id.isEmpty() && _d->property_type == PropertyType::Cell)
        return column().name(show_translation_warning);

    if (_d->translation_id.isEmpty())
        return QString::number(id().value());

    return options().testFlag(PropertyOption::NoTranslate) ? _d->translation_id : Translator::translate(_d->translation_id, show_translation_warning);
}

bool DataProperty::hasName() const
{
    return !_d->translation_id.isEmpty();
}

const DataPropertyList& DataProperty::columns() const
{
    Z_CHECK(_d->property_type == PropertyType::Dataset || _d->property_type == PropertyType::ColumnGroup);
    return _d->columns;
}

const DataPropertyList& DataProperty::columnsConstraint() const
{
    Z_CHECK(_d->property_type == PropertyType::Dataset || _d->property_type == PropertyType::ColumnGroup);
    return _d->columns_constraint;
}

int DataProperty::columnPos(const DataProperty& column_property) const
{
    return columnPos(column_property.id());
}

int DataProperty::columnPos(const PropertyID& column_property_id) const
{
    Z_CHECK(_d->property_type == PropertyType::Dataset || _d->property_type == PropertyType::ColumnGroup);
    if (_columns_pos == nullptr) {
        int max_id = 0;
        for (int i = 0; i < _d->columns_id.count(); i++) {
            max_id = qMax(max_id, _d->columns_id.at(i).value());
        }

        _columns_pos = std::make_unique<QVector<int>>(max_id + 1, -1);
        for (int i = 0; i < _d->columns_id.count(); i++) {
            _columns_pos->operator[](_d->columns_id.at(i).value()) = i;
        }
    }
    Z_CHECK_X(column_property_id.value() < _columns_pos->count(), QString("column %1 not found in %2").arg(column_property_id.value()).arg(id().value()));
    return _columns_pos->at(column_property_id.value());
}

QList<int> DataProperty::columnPos(PropertyOption option) const
{
    if (option == PropertyOption::NoOptions)
        return {};

    QList<int> res;
    for (int pos = 0; pos < _d->columns.count(); pos++) {
        if ((_d->columns.at(pos).options() & option) > 0)
            res << pos;
    }

    return res;
}

int DataProperty::pos() const
{
    Z_CHECK(_d->property_type == PropertyType::ColumnFull);
    return dataset().columnPos(*this);
}

int DataProperty::columnCount() const
{
    return columns().count();
}

void DataProperty::setTranslationID(const QString& translation_id)
{
    Z_CHECK(isValid());
    detachHelper();
    _d->translation_id = translation_id;
}

DataPropertyList DataProperty::columnsByOptions(const PropertyOptions& options) const
{
    Z_CHECK(_d->property_type == PropertyType::Dataset);

    DataPropertyList res;
    for (auto& c : columns()) {
        if ((c.options() & options) > 0)
            res << c;
    }

    return res;
}

DataProperty DataProperty::idColumnProperty() const
{
    if (!_d->column_id_option.isValid())
        return DataProperty();

    for (auto& c : columns()) {
        if (c == _d->column_id_option)
            return c;
    }
    return DataProperty();
}

const PropertyID& DataProperty::idColumnPropertyId() const
{
    return _d->column_id_option;
}

DataProperty DataProperty::nameColumn() const
{
    auto c = columnsByOptions(PropertyOption::Name);
    Z_CHECK(c.count() <= 1);
    return c.isEmpty() ? DataProperty() : c.at(0);
}

DataProperty DataProperty::fullNameColumn() const
{
    auto c = columnsByOptions(PropertyOption::FullName);
    Z_CHECK(c.count() <= 1);
    return c.isEmpty() ? DataProperty() : c.at(0);
}

bool DataProperty::contains(const DataProperty& property) const
{
    if (_d->property_type == property._d->property_type)
        return *this == property;

    if (_d->property_type == PropertyType::Field || _d->property_type == PropertyType::Cell)
        return false; // если совпадает, то проверка сработала раньше

    if (property._d->property_type == PropertyType::Field) {
        if (_d->property_type == PropertyType::Entity)
            return property.entity() == *this;
        return false;
    }

    if (property._d->property_type == PropertyType::Cell) {
        if (_d->property_type == PropertyType::Dataset)
            return property.dataset() == *this;
        if (_d->property_type == PropertyType::Entity)
            return property.entity() == *this;
        return false;
    }

    if (property._d->property_type == PropertyType::RowFull || property._d->property_type == PropertyType::RowPartial
        || property._d->property_type == PropertyType::ColumnFull || property._d->property_type == PropertyType::ColumnPartial) {
        // строки и колонки могут входить в набор Dataset или Entity
        if (_d->property_type == PropertyType::Dataset)
            return property.dataset() == *this;
        else if (_d->property_type == PropertyType::Entity)
            return property.entity() == *this;
        return false;
    }

    if (property._d->property_type == PropertyType::Dataset) {
        if (_d->property_type == PropertyType::Entity)
            return property.entity() == *this;
    }

    return false;
}

Error DataProperty::convertValue(const QVariant& value, QVariant& result) const
{
    Error error;
    result.clear();

    if (value.isNull())
        return Error();

    return convertValueHelper(value, result);
}

void DataProperty::clear()
{
    detachHelper();
    *this = DataProperty();
}

PropertyLookupPtr DataProperty::lookup() const
{
    return _d->lookup;
}

DataProperty& DataProperty::setLookupList(const QVariantList& list_values, const QStringList& list_names, bool editable)
{
    Z_CHECK(isValid());
    detachHelper();

    // make_shared нельзя использовать для приватного конструктора
    _d->lookup = std::shared_ptr<PropertyLookup>(new PropertyLookup(this, LookupType::List, Uid(), PropertyID(), PropertyID(), -1, PropertyID(), -1,
        PropertyID(), -1, list_values, list_names, editable, Uid(), QString(), PropertyIDList(), QVariant(), PropertyIDMultiMap(), QStringList()));
    return *this;
}

DataProperty& DataProperty::setLookupEntity(const EntityCode& entity_code, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    int display_column_role, const PropertyID& data_column_id, int data_column_role, const PropertyID& dataset_key_column_id, int dataset_key_column_role)
{
    return setLookupEntity(Uid::uniqueEntity(entity_code), dataset_property_id, display_column_id, display_column_role, data_column_id, data_column_role,
        dataset_key_column_id, dataset_key_column_role);
}

DataProperty& DataProperty::setLookupEntity(const Uid& entity, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    int display_column_role, const PropertyID& data_column_id, int data_column_role, const PropertyID& dataset_key_column_id, int dataset_key_column_role)
{
    Z_CHECK(isValid());
    detachHelper();

    // make_shared нельзя использовать для приватного конструктора
    _d->lookup = std::shared_ptr<PropertyLookup>(new PropertyLookup(this, LookupType::Dataset, entity, dataset_property_id, display_column_id,
        display_column_role, data_column_id, data_column_role, dataset_key_column_id, dataset_key_column_role, QVariantList(), QStringList(), false, Uid(),
        QString(), PropertyIDList(), QVariant(), PropertyIDMultiMap(), QStringList()));
    return *this;
}

DataProperty& DataProperty::setLookupEntity(const EntityCode& entity_code, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    const PropertyID& data_column_id, const PropertyID& dataset_key_column_id)
{
    return setLookupEntity(
        entity_code, dataset_property_id, display_column_id, Qt::DisplayRole, data_column_id, Qt::DisplayRole, dataset_key_column_id, Qt::DisplayRole);
}

DataProperty& DataProperty::setLookupEntity(const Uid& entity, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    const PropertyID& data_column_id, const PropertyID& dataset_key_column_id)
{
    return setLookupEntity(
        entity, dataset_property_id, display_column_id, Qt::DisplayRole, data_column_id, Qt::DisplayRole, dataset_key_column_id, Qt::DisplayRole);
}

DataProperty& DataProperty::setLookupEntity(const EntityCode& entity_code, const PropertyID& dataset_property_id)
{
    return setLookupEntity(entity_code, dataset_property_id, PropertyID(), Qt::DisplayRole, PropertyID(), Qt::DisplayRole);
}

DataProperty& DataProperty::setLookupEntity(const Uid& entity, const PropertyID& dataset_property_id)
{
    return setLookupEntity(entity, dataset_property_id, PropertyID(), Qt::DisplayRole, PropertyID(), Qt::DisplayRole);
}

DataProperty& DataProperty::setLookupDynamic(bool editable)
{
    Z_CHECK(isValid());
    detachHelper();

    // make_shared нельзя использовать для приватного конструктора
    _d->lookup = std::shared_ptr<PropertyLookup>(new PropertyLookup(this, LookupType::Dataset, Uid(), PropertyID(), PropertyID(), -1, PropertyID(), -1,
        PropertyID(), -1, QVariantList(), QStringList(), editable, Uid(), QString(), PropertyIDList(), QVariant(), PropertyIDMultiMap(), QStringList()));
    return *this;
}

DataProperty& DataProperty::setLookupRequest(const Uid& request_service_uid, const QString& request_type, const PropertyID& request_service_value_property,
    const PropertyIDList& parent_key_properties, const PropertyIDMultiMap& attributes, const QStringList& attributes_to_clear,
    const PropertyIDList& request_id_source_properties, const QVariant& options)
{
    Z_CHECK(isValid());
    Z_CHECK(request_service_uid.isValid());

    detachHelper();

    // make_shared нельзя использовать для приватного конструктора
    _d->lookup
        = std::shared_ptr<PropertyLookup>(new PropertyLookup(this, LookupType::Request, Uid(), PropertyID(), PropertyID(), -1, PropertyID(), -1, PropertyID(),
            -1, QVariantList(), QStringList(), false, request_service_uid, request_type, parent_key_properties, options, attributes, attributes_to_clear));

    if (!request_id_source_properties.isEmpty())
        addLinkDataSourcePriority(request_id_source_properties);

    if (request_service_value_property.isValid())
        addLinkLookupFreeText(request_service_value_property);

    return *this;
}

DataProperty& DataProperty::setLookupRequest(const Uid& request_service_uid, const QString& request_type, const PropertyID& request_service_value_property,
    const PropertyIDMultiMap& attributes, const QStringList& attributes_to_clear, const PropertyIDList& request_id_source_properties, const QVariant& options)
{
    return setLookupRequest(request_service_uid, request_type, request_service_value_property, PropertyIDList(), attributes, attributes_to_clear,
        request_id_source_properties, options);
}

DataProperty& DataProperty::setLookupRequest(const EntityCode& request_service_entity_code, const QString& request_type,
    const PropertyID& request_service_value_property, const PropertyIDList& parent_key_properties, const PropertyIDMultiMap& attributes,
    const QStringList& attributes_to_clear, const PropertyIDList& request_id_source_properties, const QVariant& options)
{
    return setLookupRequest(Uid::uniqueEntity(request_service_entity_code), request_type, request_service_value_property, parent_key_properties, attributes,
        attributes_to_clear, request_id_source_properties, options);
}

DataProperty& DataProperty::setLookupRequest(const EntityCode& request_service_entity_code, const QString& request_type,
    const PropertyID& request_service_value_property, const PropertyIDMultiMap& attributes, const QStringList& attributes_to_clear,
    const PropertyIDList& request_id_source_properties, const QVariant& options)
{
    return setLookupRequest(request_service_entity_code, request_type, request_service_value_property, PropertyIDList(), attributes, attributes_to_clear,
        request_id_source_properties, options);
}

DataProperty& DataProperty::setModelRequest(const Uid& lookup_entity_uid, const PropertyID& dataset_property_id)
{
    return setModelRequest(lookup_entity_uid, dataset_property_id, PropertyID(), Qt::DisplayRole, PropertyID(), Qt::DisplayRole, PropertyID());
}

DataProperty& DataProperty::setModelRequest(const EntityCode& lookup_entity_code, const PropertyID& dataset_property_id)
{
    return setModelRequest(Uid::uniqueEntity(lookup_entity_code), dataset_property_id);
}

DataProperty& DataProperty::setModelRequest(const Uid& lookup_entity_uid, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    int display_column_role, const PropertyID& data_column_id, int data_column_role, const PropertyID& request_service_value_property)
{
    Z_CHECK(isValid());
    Z_CHECK(lookup_entity_uid.isValid());

    detachHelper();

    ModelServiceLookupRequestOptions options;
    options.lookup_entity_uid = lookup_entity_uid;
    options.dataset_property_id = dataset_property_id;
    options.display_column_id = display_column_id;
    options.display_column_role = display_column_role > 0 ? display_column_role : Qt::DisplayRole;
    options.data_column_id = data_column_id;
    options.data_column_role = data_column_role > 0 ? data_column_role : Qt::DisplayRole;

    // make_shared нельзя использовать для приватного конструктора
    _d->lookup = std::shared_ptr<PropertyLookup>(
        new PropertyLookup(this, LookupType::Request, Uid(), PropertyID(), PropertyID(), -1, PropertyID(), -1, PropertyID(), -1, QVariantList(), QStringList(),
            false, CoreUids::MODEL_SERVICE, QString(), PropertyIDList(), QVariant::fromValue(options), PropertyIDMultiMap(), QStringList()));

    if (request_service_value_property.isValid())
        addLinkLookupFreeText(request_service_value_property);

    return *this;
}

DataProperty& DataProperty::setModelRequest(const EntityCode& lookup_entity_code, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    int display_column_role, const PropertyID& data_column_id, int data_column_role, const PropertyID& request_service_value_property)
{
    return setModelRequest(Uid::uniqueEntity(lookup_entity_code), dataset_property_id, display_column_id, display_column_role, data_column_id, data_column_role,
        request_service_value_property);
}

DataProperty& DataProperty::setModelRequest(const EntityCode& lookup_entity_code, const PropertyID& dataset_property_id, const PropertyID& display_column_id)
{
    return setModelRequest(Uid::uniqueEntity(lookup_entity_code), dataset_property_id, display_column_id, -1, PropertyID(), -1, PropertyID());
}

DataProperty& DataProperty::setModelRequest(const Uid& lookup_entity_uid, const PropertyID& dataset_property_id, const PropertyID& display_column_id)
{
    return setModelRequest(lookup_entity_uid, dataset_property_id, display_column_id, -1, PropertyID(), -1, PropertyID());
}

DataProperty& DataProperty::setModelRequest(const Uid& lookup_entity_uid, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    const PropertyID& data_column_id, const PropertyID& request_service_value_property)
{
    return setModelRequest(
        lookup_entity_uid, dataset_property_id, display_column_id, Qt::DisplayRole, data_column_id, Qt::DisplayRole, request_service_value_property);
}

DataProperty& DataProperty::setModelRequest(const EntityCode& lookup_entity_code, const PropertyID& dataset_property_id, const PropertyID& display_column_id,
    const PropertyID& data_column_id, const PropertyID& request_service_value_property)
{
    return setModelRequest(
        lookup_entity_code, dataset_property_id, display_column_id, Qt::DisplayRole, data_column_id, Qt::DisplayRole, request_service_value_property);
}

QList<PropertyConstraintPtr> DataProperty::constraints() const
{
    return _d->constraints;
}

DataProperty& DataProperty::setConstraintMaxTextLength(int max_length, const QString& message, InformationType highlight_type,
    const PropertyID& show_higlight_property, ConstraintOptions options, const HighlightOptions& highlight_options)
{
    Z_CHECK(isValid());
    addConstraintHelper(std::shared_ptr<PropertyConstraint>(new PropertyConstraint(
        ConditionType::MaxTextLength, message, highlight_type, this->options(), max_length, nullptr, options, show_higlight_property, highlight_options)));
    return *this;
}

DataProperty& DataProperty::setConstraintRegexp(const QString& regexp, const QString& message, InformationType highlight_type,
    const PropertyID& show_higlight_property, ConstraintOptions options, const HighlightOptions& highlight_options)
{
    Z_CHECK(isValid());
    addConstraintHelper(std::shared_ptr<PropertyConstraint>(new PropertyConstraint(
        ConditionType::MaxTextLength, message, highlight_type, this->options(), regexp, nullptr, options, show_higlight_property, highlight_options)));
    return *this;
}

DataProperty& DataProperty::setConstraintValidator(const QValidator* validator, const QString& message, InformationType highlight_type,
    const PropertyID& show_higlight_property, ConstraintOptions options, const HighlightOptions& highlight_options)
{
    Z_CHECK(isValid());
    Z_CHECK(validator != nullptr && validator->parent() != nullptr);

    addConstraintHelper(std::shared_ptr<PropertyConstraint>(new PropertyConstraint(ConditionType::MaxTextLength, message, highlight_type, this->options(),
        QVariant(), reinterpret_cast<void*>(const_cast<QValidator*>(validator)), options, show_higlight_property, highlight_options)));
    return *this;
}

DataProperty& DataProperty::setConstraintRequired(const QString& message, InformationType highlight_type, const PropertyID& show_higlight_property,
    ConstraintOptions options, const HighlightOptions& highlight_options)
{
    Z_CHECK(isValid());
#if 0 // defined(RNIKULENKOV)
    Q_UNUSED(message)
    Q_UNUSED(highlight_type)
    Q_UNUSED(options)
#else
    // make_shared нельзя использовать для приватного конструктора
    addConstraintHelper(std::shared_ptr<PropertyConstraint>(new PropertyConstraint(
        ConditionType::Required, message, highlight_type, this->options(), QVariant(), nullptr, options, show_higlight_property, highlight_options)));
#endif
    return *this;
}

DataProperty& DataProperty::setConstraintRequired(
    InformationType highlight_type, const PropertyID& show_higlight_property, ConstraintOptions options, const HighlightOptions& highlight_options)
{
    return setConstraintRequired(QString(), highlight_type, show_higlight_property, options, highlight_options);
}

bool DataProperty::hasConstraints(ConditionTypes types) const
{
    if (types == ConditionTypes()) {
        if (isDatasetPart())
            return dataset().hasConstraints();
        return _d->has_constraints;

    } else {
        for (auto c = _d->constraints.constBegin(); c != _d->constraints.constEnd(); ++c) {
            if (types.testFlag((*c)->type()))
                return true;
        }
        return false;
    }
}

DataProperty& DataProperty::addLinkLookupFreeText(const PropertyID& property_id)
{
    addLinkHelper(std::shared_ptr<PropertyLink>(new PropertyLink(id(), property_id)));
    return *this;
}

DataProperty& DataProperty::addLinkDataSource(const PropertyID& key_property_id, Uid source_entity, const PropertyID& source_column_id)
{
    addLinkHelper(std::shared_ptr<PropertyLink>(new PropertyLink(id(), key_property_id, source_entity, source_column_id)));
    return *this;
}

DataProperty& DataProperty::addLinkDataSource(const PropertyID& key_property_id, const EntityCode& source_entity_code, const PropertyID& source_column_id)
{
    return addLinkDataSource(key_property_id, Uid::uniqueEntity(source_entity_code), source_column_id);
}

DataProperty& DataProperty::addLinkDataSourceCatalog(const PropertyID& key_property_id, const EntityCode& catalog_code, const PropertyID& source_column_id)
{
    return addLinkDataSource(key_property_id, Core::catalogUid(catalog_code), source_column_id);
}

DataProperty& DataProperty::addLinkDataSourcePriority(const PropertyIDList& properties)
{
    addLinkHelper(std::shared_ptr<PropertyLink>(new PropertyLink(properties)));
    return *this;
}

const QList<PropertyLinkPtr>& DataProperty::links() const
{
    return _d->links;
}

QList<PropertyLinkPtr> DataProperty::links(PropertyLinkType type) const
{
    Z_CHECK(type != PropertyLinkType::Undefined);
    QList<PropertyLinkPtr> res;
    for (auto& l : qAsConst(_d->links)) {
        if (l->type() == type)
            res << l;
    }
    return res;
}

bool DataProperty::hasLinks() const
{
    return !_d->links.isEmpty();
}

bool DataProperty::hasLinks(PropertyLinkType type) const
{
    for (auto& l : qAsConst(_d->links)) {
        if (l->type() == type)
            return true;
    }
    return false;
}

bool DataProperty::isDataTypeRequired(PropertyType property_type)
{
    return (property_type != PropertyType::Entity && property_type != PropertyType::Dataset && property_type != PropertyType::Cell
            && property_type != PropertyType::RowFull && property_type != PropertyType::RowPartial);
}

uint DataProperty::hashKey() const
{
    return _d->hash_key;
}

int DataProperty::naturalIndex() const
{
    return _d->natural_index;
}

int DataProperty::metaType()
{
    return _metatype;
}

void DataProperty::addConstraintHelper(const PropertyConstraintPtr& c)
{
    Z_CHECK(isValid());
    Z_CHECK_NULL(c);
    Z_CHECK(c->isValid());
    detachHelper();

    for (int i = 0; i < _d->constraints.count(); i++) {
        if (_d->constraints.at(i)->type() == c->type() && _d->constraints.at(i)->highlightType() == c->highlightType()) {
            _d->constraints.removeAt(i);
            break;
        }
    }

    _d->constraints << c;

    _d->has_constraints = true;
    if (_d->property_type == PropertyType::ColumnFull) {
        // обновляем информацию о наличии ограничений для набора даннных и сущности
        Z_CHECK_NULL(_d->dataset);
        _d->dataset->_d->has_constraints = true;
        _d->dataset->_d->columns_constraint << *this;
    }

    Z_CHECK_NULL(_d->entity);
    _d->entity->detachHelper();
    _d->entity->_d->has_constraints = true;
}

void DataProperty::addLinkHelper(const PropertyLinkPtr& link)
{
    Z_CHECK(isValid());
    Z_CHECK_NULL(link);
    Z_CHECK(link->isValid());

    // защита от дурака (циклическое обновление)
    Z_CHECK(id() != link->linkedPropertyId());

    detachHelper();

    for (auto& l : qAsConst(_d->links)) {
        if (link->mainPropertyId().isValid())
            Z_CHECK_X(
                l->mainPropertyId() != link->mainPropertyId() || l->type() != PropertyLinkType::DataSource || l->type() != PropertyLinkType::DataSourcePriority,
                "Property link duplication");
        else
            Z_CHECK_X(l->type() != PropertyLinkType::DataSource, "Property link duplication");
    }

    _d->links << link;
}

Error DataProperty::convertValueHelper(const QVariant& value, QVariant& result) const
{
    Error error = Utils::convertValue(value, _d->data_type, nullptr, ValueFormat::Internal, Core::fr()->c_locale(), result);
    if (error.isError())
        return Error(QStringLiteral("Property %1(%2): %3").arg(entityCode().value()).arg(id().value()).arg(error.fullText()));
    return error;
}

void DataProperty::setDataset(const DataProperty& dataset)
{
    _d->dataset = Z_MAKE_SHARED(DataProperty, dataset);
    _d->updateUniqueKey();
}

void DataProperty::setColumnGroup(const DataProperty& column_group)
{
    _d->column_group = Z_MAKE_SHARED(DataProperty, column_group);
    _d->updateUniqueKey();
}

void DataProperty::detachHelper()
{
    if (_d == DataProperty_SharedData::shared_null())
        _d.detach();
}

QJsonObject DataProperty::toJson() const
{
    QJsonObject json;
    json["id"] = static_cast<int>(_d->id.value());
    json["property_type"] = static_cast<int>(_d->property_type);
    json["data_type"] = static_cast<int>(_d->data_type);

    if (_d->property_type == PropertyType::Dataset) {
        QJsonArray cols;
        for (auto& c : columns()) {
            cols.append(c.toJson());
        }

        json["columns"] = cols;
    }

    return json;
}

DataProperty DataProperty::fromJson(const QJsonObject& json, const DataProperty& parent, Error& error)
{
    Z_CHECK(parent.isValid());

    int id;
    PropertyType property_type = PropertyType::Undefined;
    DataType data_type = DataType::Undefined;
    error = fromJsonHelper(json, id, property_type, data_type);
    if (error.isError())
        return DataProperty();

    if (parent._d->property_type == PropertyType::Entity) {
        if (property_type != PropertyType::Field && property_type != PropertyType::Dataset) {
            error = Error::jsonError({"property", "property_type", "PropertyType::Field"});
            return DataProperty();
        }

        if (property_type == PropertyType::Field)
            return fieldProperty(parent, PropertyID(id), data_type);

        Z_CHECK(property_type == PropertyType::Dataset); //-V547

        auto dataset = datasetProperty(parent, PropertyID(id), data_type);

        auto columns = json.value("columns").toArray();
        if (columns.isEmpty()) {
            error = Error::jsonError({"property", "columns"});
            return DataProperty();
        }

        for (const auto& c : columns) {
            auto col_obj = c.toObject();
            if (col_obj.isEmpty()) {
                error = Error::jsonError({"property", "columns"});
                return DataProperty();
            }

            auto column = fromJson(col_obj, dataset, error);
            if (error.isError())
                return DataProperty();

            dataset.addColumn(column);
        }

        return dataset;
    }

    if (parent._d->property_type == PropertyType::Dataset) {
        if (property_type != PropertyType::ColumnFull) {
            error = Error::jsonError({"property", "property_type", "PropertyType::Dataset"});
            return DataProperty();
        }
        return columnProperty(parent, PropertyID(id), data_type);
    }

    Z_HALT_INT;
    return DataProperty();
}

DataProperty DataProperty::fromJson(const QJsonObject& json, Error& error)
{
    int id;
    PropertyType property_type = PropertyType::Undefined;
    DataType data_type = DataType::Undefined;
    error = fromJsonHelper(json, id, property_type, data_type);
    if (error.isError())
        return DataProperty();

    if (property_type == PropertyType::Field)
        return independentFieldProperty(PropertyID(id), data_type);

    if (property_type == PropertyType::Dataset) {
        DataProperty dataset = independentDatasetProperty(PropertyID(id), data_type);

        auto columns = json.value("columns").toArray();
        if (columns.isEmpty()) {
            error = Error::jsonError({"property", "columns"});
            return DataProperty();
        }

        for (const auto& c : columns) {
            auto col_obj = c.toObject();
            if (col_obj.isEmpty()) {
                error = Error::jsonError({"property", "columns"});
                return DataProperty();
            }

            auto column = fromJson(col_obj, dataset, error);
            if (error.isError())
                return DataProperty();

            dataset.addColumn(column);
        }

        return dataset;
    }

    error = Error::jsonError({"property", "property_type", "PropertyType::Field"});
    return DataProperty();
}

Error DataProperty::fromJsonHelper(const QJsonObject& json, int& id, PropertyType& property_type, DataType& data_type)
{
    auto v = json.value("id");
    if (v.isNull() || v.isUndefined())
        return Error::jsonError({"property", "id"});
    id = static_cast<int>(v.toInt());
    if (id < Consts::MINUMUM_PROPERTY_ID)
        return Error(QString("Wrong property ID: %1").arg(id));

    v = json.value("property_type");
    if (v.isNull() || v.isUndefined())
        return Error::jsonError({"property", "property_type"});
    property_type = static_cast<PropertyType>(v.toInt());

    v = json.value("data_type");
    if (v.isNull() || v.isUndefined())
        return Error::jsonError({"property", "data_type"});
    data_type = static_cast<DataType>(v.toInt());

    return Error();
}

DataStructure::DataStructure()
    : QObject()
{
    Z_DEBUG_NEW("DataStructure");
}

DataStructure::DataStructure(const DataStructure& m)
    : QObject()
    , _id(m._id)
    , _entity(m._entity)
    , _structure_version(m._structure_version)
    , _properties(m._properties)
    , _properties_list_main(m._properties_list_main)
    , _properties_by_type(m._properties_by_type)
    , _single_dataset_id(m._single_dataset_id)
{
    if (m._properties_list_expanded != nullptr)
        _properties_list_expanded = std::make_unique<DataPropertySet>(*m._properties_list_expanded);

    for (auto& p : m._same_properties) {
        setSameProperties(p);
    }

    Z_DEBUG_NEW("DataStructure");
}

DataStructurePtr DataStructure::structure(const EntityCode& code, bool halt_if_not_found)
{
    Error error;
    auto plugin = Core::getPlugin(code, error);

    DataStructurePtr str;
    if (error.isOk())
        str = plugin->dataStructure(Uid(), error);

    if (error.isError()) {
        if (!halt_if_not_found)
            return nullptr;
        Z_HALT(error);
    }

    return str;
}

DataStructurePtr DataStructure::structure(const Uid& entity_uid, bool halt_if_not_found)
{
    Error error;
    auto plugin = Core::getPlugin(entity_uid.entityCode(), error);

    DataStructurePtr str;
    if (error.isOk())
        str = plugin->dataStructure(entity_uid, error);

    if (error.isError()) {
        if (!halt_if_not_found)
            return nullptr;
        Z_HALT(error);
    }

    return str;
}

DataStructurePtr DataStructure::structure(const DataProperty& entity_property, bool halt_if_not_found)
{
    Z_CHECK(entity_property.propertyType() == PropertyType::Entity);
    if (entity_property.entityUid().isValid())
        return structure(entity_property.entityUid(), halt_if_not_found);

    return structure(entity_property.entityCode(), halt_if_not_found);
}

PropertyIDList DataStructure::toIDList(const DataPropertyList& props)
{
    return DataProperty::toIDList(props);
}

PropertyIDList DataStructure::toIDList(const DataPropertySet& props)
{
    return DataProperty::toIDList(props);
}

DataPropertyList DataStructure::toPropertyList(const PropertyIDList& props) const
{
    DataPropertyList res;
    for (auto& p : props) {
        res << property(p);
    }
    return res;
}

DataPropertyList DataStructure::toPropertyList(const PropertyIDSet& props) const
{
    return toPropertyList(props.toList());
}

DataStructurePtr DataStructure::independentStructure(const PropertyID& id, int structure_version, const DataPropertyList& properties)
{
    Z_CHECK(id.isValid());
    return DataStructurePtr(new DataStructure(id, DataProperty::independentEntityProperty(), structure_version, properties));
}

DataProperty DataStructure::independentEntityProperty()
{
    return DataProperty::independentEntityProperty();
}

DataStructurePtr DataStructure::entityStructure(const DataProperty& entity_property, int structure_version, const DataPropertyList& properties)
{
    Z_CHECK(entity_property.isValid());
    return DataStructurePtr(new DataStructure(PropertyID(entity_property.entityCode().value()), entity_property, structure_version, properties));
}

DataStructurePtr DataStructure::entityStructure(const EntityCode& entity_code, int structure_version)
{
    return entityStructure(DataProperty::entityProperty(entity_code), structure_version);
}

DataStructurePtr DataStructure::toIndependent(DataStructurePtr entity_structure)
{
    Z_CHECK_NULL(entity_structure);
    Z_CHECK(entity_structure->isValid());
    Z_CHECK(!entity_structure->isEntityIndependent());

    auto res = DataStructure::independentStructure(entity_structure->id(), entity_structure->structureVersion());

    for (auto& p : entity_structure->propertiesMain()) {
        if (p.propertyType() == zf::PropertyType::Field) {
            res->addProperty(zf::DataProperty::independentFieldProperty(p.id(), p.dataType(), p.translationID(), p.defaultValue(), p.options()));
        } else if (p.propertyType() == zf::PropertyType::Dataset) {
            zf::DataProperty dataset = zf::DataProperty::independentDatasetProperty(p.id(), p.dataType(), p.translationID(), p.options());
            for (auto& d_c : p.columns()) {
                DataProperty column;

                if (d_c.propertyType() == PropertyType::ColumnFull) {
                    column = dataset.addColumn(zf::DataProperty::columnProperty(dataset, d_c.id(), d_c.dataType(), d_c.translationID(), {}, d_c.options()));

                } else if (d_c.propertyType() == zf::PropertyType::ColumnGroup) {
                    column = zf::DataProperty::columnGroupProperty(d_c.dataset(), d_c.id());
                    for (auto& f : d_c.columns()) {
                        auto c
                            = column.addColumn(zf::DataProperty::columnProperty(f.dataset(), f.id(), f.dataType(), f.translationID(), {}, f.options(), column));
                        auto lookup = f.lookup();
                        if (lookup != nullptr) {
                            if (lookup->type() == LookupType::List) {
                                c.setLookupList(lookup->listValues(), lookup->listNames(), lookup->isEditable());

                            } else if (lookup->type() == LookupType::Dataset) {
                                if (lookup->listEntity().isValid())
                                    c.setLookupEntity(lookup->listEntity(), lookup->datasetPropertyID(), lookup->displayColumnID(), lookup->displayColumnRole(),
                                        lookup->dataColumnID(), lookup->dataColumnRole());
                                else
                                    c.setLookupDynamic(lookup->isEditable());
                            } else {
                                Z_HALT_INT;
                            }
                        }
                    }
                } else
                    Z_HALT_INT;

                auto lookup = d_c.lookup();
                if (lookup != nullptr) {
                    if (lookup->type() == LookupType::List) {
                        column.setLookupList(lookup->listValues(), lookup->listNames(), lookup->isEditable());

                    } else if (lookup->type() == LookupType::Dataset) {
                        if (lookup->listEntity().isValid())
                            column.setLookupEntity(lookup->listEntity(), lookup->datasetPropertyID(), lookup->displayColumnID(), lookup->displayColumnRole(),
                                lookup->dataColumnID(), lookup->dataColumnRole());
                        else
                            column.setLookupDynamic(lookup->isEditable());
                    } else {
                        Z_HALT_INT;
                    }
                }
            }
            res->addProperty(dataset);
        }
    }

    return res;
}

DataStructurePtr DataStructure::toEntityBased(DataStructurePtr independent_structure, const DataProperty& entity)
{
    Z_CHECK_NULL(independent_structure);
    Z_CHECK(independent_structure->isValid());
    Z_CHECK(independent_structure->isEntityIndependent());

    auto res = zf::DataStructure::entityStructure(entity, independent_structure->structureVersion());

    for (auto& p : independent_structure->propertiesMain()) {
        if (p.propertyType() == zf::PropertyType::Field) {
            res->addProperty(zf::DataProperty::fieldProperty(entity, p.id(), p.dataType(), {}, {}, p.options()));

        } else if (p.propertyType() == zf::PropertyType::Dataset) {
            zf::DataProperty dataset = zf::DataProperty::datasetProperty(entity, p.id(), p.dataType(), {}, p.options());
            for (auto& d_c : p.columns()) {
                DataProperty column;

                if (d_c.propertyType() == PropertyType::ColumnFull) {
                    column = dataset.addColumn(zf::DataProperty::columnProperty(dataset, d_c.id(), d_c.dataType(), d_c.translationID(), {}, d_c.options()));

                } else if (d_c.propertyType() == zf::PropertyType::ColumnGroup) {
                    column = zf::DataProperty::columnGroupProperty(d_c.dataset(), d_c.id());
                    for (auto& f : p.columns()) {
                        auto c
                            = column.addColumn(zf::DataProperty::columnProperty(f.dataset(), f.id(), f.dataType(), f.translationID(), {}, f.options(), column));
                        auto lookup = f.lookup();
                        if (lookup != nullptr) {
                            if (lookup->type() == LookupType::List) {
                                c.setLookupList(lookup->listValues(), lookup->listNames(), lookup->isEditable());

                            } else if (lookup->type() == LookupType::Dataset) {
                                if (lookup->listEntity().isValid())
                                    c.setLookupEntity(lookup->listEntity(), lookup->datasetPropertyID(), lookup->displayColumnID(), lookup->displayColumnRole(),
                                        lookup->dataColumnID(), lookup->dataColumnRole());
                                else
                                    c.setLookupDynamic(lookup->isEditable());
                            } else {
                                Z_HALT_INT;
                            }
                        }
                    }
                } else
                    Z_HALT_INT;

                auto lookup = d_c.lookup();
                if (lookup != nullptr) {
                    if (lookup->type() == LookupType::List) {
                        column.setLookupList(lookup->listValues(), lookup->listNames(), lookup->isEditable());

                    } else if (lookup->type() == LookupType::Dataset) {
                        if (lookup->listEntity().isValid())
                            column.setLookupEntity(lookup->listEntity(), lookup->datasetPropertyID(), lookup->displayColumnID(), lookup->displayColumnRole(),
                                lookup->dataColumnID(), lookup->dataColumnRole());
                        else
                            column.setLookupDynamic(lookup->isEditable());
                    } else {
                        Z_HALT_INT;
                    }
                }
            }
            res->addProperty(dataset);
        }
    }

    return res;
}

DataStructure::~DataStructure()
{
    _properties_by_type.clear();
    _properties_list_main.clear();
    _properties_list_expanded.reset();
    _properties.clear();

    Z_DEBUG_DELETE("DataStructure");
}

int DataStructure::structureVersion() const
{
    return _structure_version;
}

bool DataStructure::isValid() const
{
    return id().isValid();
}

bool DataStructure::isEntityIndependent() const
{
    return !_entity.isValid() || _entity.isEntityIndependent();
}

PropertyID DataStructure::id() const
{
    return _entity.isValid() ? _entity.id() : _id;
}

EntityCode DataStructure::entityCode() const
{
    return _entity.entityCode();
}

DataProperty DataStructure::entity() const
{
    return _entity;
}

void DataStructure::expandProperties(const DataPropertyList& properties, DataPropertyList& expanded, PropertyID& max_id)
{
    expanded.append(properties);

    max_id.clear();
    for (auto& p : properties) {
        if (max_id < p.id())
            max_id = p.id();

        if (p.propertyType() == PropertyType::Dataset) {
            PropertyID max_column_id;
            expandProperties(p.columns(), expanded, max_column_id);
            if (max_id < max_column_id)
                max_id = max_column_id;
        }
    }
}

const DataPropertySet& DataStructure::expanded() const
{
    if (_properties_list_expanded == nullptr) {
        // отложенная инициализация, т.к колонки могут быть добавлены после добавления набора данных в структуру
        _properties_list_expanded = std::make_unique<DataPropertySet>();

        DataPropertyList properties;
        for (auto& p : qAsConst(_properties)) {
            if (p == nullptr)
                continue;
            properties << *p;
        }

        DataPropertyList expanded;
        PropertyID max_id;
        expandProperties(properties, expanded, max_id);
        if (max_id.value() >= _properties.count()) {
            // добавились колонки наборов данных
            _properties.resize(max_id.value() + 1);
        }

        for (auto& p : expanded) {
            _properties_list_expanded->insert(p);
            if (_properties.at(p.id().value()) == nullptr)
                _properties[p.id().value()] = Z_MAKE_SHARED(DataProperty, p);
        }
    }
    return *_properties_list_expanded.get();
}

void DataStructure::setProperties(const DataPropertyList& properties)
{
    int max_id = -1;
    for (auto& p : qAsConst(properties)) {
        Z_CHECK(p.isValid());
        Z_CHECK(p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Dataset);
        max_id = qMax(max_id, p.id().value());
    }

    _properties_by_type.clear();
    _properties.clear();
    _properties_list_main = Utils::toSet(properties);
    _properties_list_expanded.reset(); // инициализация откладывается до вызова expanded()
    _single_dataset_id.clear();

    _properties.resize(0);
    _properties.resize(max_id + 1);
    bool single_dataset_init = false;
    Uid entity_uid;
    PropertyIDSet duplication_test;
    for (auto& p : properties) {
        Z_CHECK_X(!duplication_test.contains(p.id()), QString("Property duplication. Structure id: %1, property id: %2").arg(_id.value()).arg(p.id().value()));
        duplication_test << p.id();

        if (entity_uid.isValid()) {
            Z_CHECK_X(entity_uid == p.entityUid(), QString("Invalid entity %1 != %2").arg(entity_uid.toPrintable()).arg(p.entityUid().toPrintable()));

        } else {
            entity_uid = p.entityUid();
        }

        _properties[p.id().value()] = Z_MAKE_SHARED(DataProperty, p);

        if (p.propertyType() == PropertyType::Dataset) {
            for (auto& col_p : p.columns()) {
                Z_CHECK_X(!duplication_test.contains(col_p.id()),
                    QString("Property duplication. Structure id: %1, property id: %2").arg(_id.value()).arg(col_p.id().value()));
                duplication_test << col_p.id();
            }

            if (single_dataset_init) {
                _single_dataset_id.clear();
            } else {
                _single_dataset_id = p.id();
                single_dataset_init = true;
            }
        }
    }
}

DataProperty DataStructure::addProperty(const DataProperty& property)
{
    Z_CHECK(property.isValid());
    property._d->natural_index = _properties_list_main.count();
    setProperties(_properties_list_main.values() + DataPropertyList({property}));
    return property;
}

DataStructure& DataStructure::operator<<(const DataProperty& property)
{
    addProperty(property);
    return *this;
}

DataStructure& DataStructure::operator=(const DataPropertySet& properties)
{
    setProperties(properties.values());
    return *this;
}

DataPropertySet DataStructure::propertiesToSet(const PropertyIDList& properties) const
{
    DataPropertySet res;
    for (auto& p : properties) {
        res << property(p);
    }
    return res;
}

DataPropertySet DataStructure::propertiesToSet(const PropertyIDSet& properties) const
{
    DataPropertySet res;
    for (auto& p : properties) {
        res << property(p);
    }
    return res;
}

DataPropertySet DataStructure::propertiesToSet(const PropertyID& property) const
{
    return DataPropertySet {this->property(property)};
}

DataStructure& DataStructure::operator=(const DataPropertyList& properties)
{
    setProperties(properties);
    return *this;
}

DataProperty DataStructure::addEntityProperty(const EntityCode& entity_code, const QString& translation_id, const PropertyOptions& options)
{
    return addProperty(DataProperty::entityProperty(entity_code, translation_id, options));
}

DataProperty DataStructure::addEntityProperty(const Uid& entity_uid, const QString& translation_id, const PropertyOptions& options)
{
    return addProperty(DataProperty::entityProperty(entity_uid, translation_id, options));
}

DataProperty DataStructure::addFieldProperty(const DataProperty& entity, const PropertyID& id, DataType data_type, const QString& translation_id,
    const QVariant& default_value, const PropertyOptions& options)
{
    return addProperty(DataProperty::fieldProperty(entity, id, data_type, translation_id, default_value, options));
}

DataProperty DataStructure::addDatasetProperty(
    const DataProperty& entity, const PropertyID& id, DataType data_type, const QString& translation_id, const PropertyOptions& options)
{
    return addProperty(DataProperty::datasetProperty(entity, id, data_type, translation_id, options));
}

DataProperty DataStructure::addIndependentFieldProperty(
    const PropertyID& id, DataType data_type, const QString& translation_id, const QVariant& default_value, const PropertyOptions& options)
{
    return addProperty(DataProperty::independentFieldProperty(id, data_type, translation_id, default_value, options));
}

DataProperty DataStructure::addIndependentDatasetProperty(
    const PropertyID& id, DataType data_type, const QString& translation_id, const PropertyOptions& options)
{
    return addProperty(DataProperty::independentDatasetProperty(id, data_type, translation_id, options));
}

DataStructure::DataStructure(const PropertyID& id, const DataProperty& entity, int structure_version, const DataPropertyList& properties)
    : QObject()
    , _id(id)
    , _entity(entity)
    , _structure_version(structure_version)
{
    if (entity.isValid())
        _id = PropertyID(entity.entityCode().value());

    setProperties(properties);
}

bool DataStructure::contains(const PropertyID& property_id) const
{
    expanded(); // инициализация
    if (!property_id.isValid() || _properties.count() <= property_id.value())
        return false;
    return _properties.at(property_id.value()) != nullptr;
}

bool DataStructure::contains(const DataProperty& p) const
{
    return contains(p.id());
}

DataProperty DataStructure::property(const PropertyID& property_id, bool halt_if_not_found) const
{
    Z_CHECK(property_id.isValid());
    expanded(); // инициализация

    if (_properties.count() <= property_id.value()) {
        if (halt_if_not_found)
            Z_HALT(QString("Property %1 not found in DataStructure %2").arg(property_id.value()).arg(id().value()));
        else
            return DataProperty();
    }

    auto res = _properties.at(property_id.value());

    if (halt_if_not_found && _properties.at(property_id.value()) == nullptr)
        Z_HALT(QString("Property %1 not found in DataStructure %2").arg(property_id.value()).arg(id().value()));

    return res == nullptr ? DataProperty() : *res;
}

PropertyOptions DataStructure::propertyOptions(const PropertyID& property_id, bool halt_if_not_found) const
{
    return property(property_id, halt_if_not_found).options();
}

DataProperty DataStructure::propertyRow(const DataProperty& dataset, const RowID& row_id)
{
    return DataProperty::rowProperty(dataset, row_id);
}

DataProperty DataStructure::propertyColumn(const DataProperty& dataset, int column, bool halt_if_not_found)
{
    Z_CHECK(column >= 0);
    if (dataset.columns().count() <= column) {
        if (halt_if_not_found)
            Z_HALT(QString("Колонка с номером %1 не найдена в наборе данных %2:%3").arg(column).arg(dataset.entityCode().value()).arg(dataset.id().value()));
        else
            return DataProperty();
    }
    return dataset.columns().at(column);
}

DataProperty DataStructure::propertyCell(const DataProperty& dataset, const RowID& row_id, int column)
{
    return DataProperty::cellProperty(propertyRow(dataset, row_id), propertyColumn(dataset, column));
}

DataProperty DataStructure::propertyCell(const RowID& row_id, const DataProperty& column_property)
{
    return DataProperty::cellProperty(propertyRow(column_property.dataset(), row_id), column_property);
}

DataPropertyList DataStructure::propertiesByOptions(PropertyType type, const PropertyOptions& options) const
{
    Z_CHECK(type != PropertyType::Undefined);
    Z_CHECK(options != PropertyOptions());

    if (type == PropertyType::Entity) {
        if ((_entity.options() & options) > 0)
            return DataPropertyList {_entity};
        return {};
    }

    DataPropertyList res;
    for (auto& p : qAsConst(expanded())) {
        if (p.propertyType() == type && (p.options() & options) > 0)
            res << p;
    }
    return res;
}

DataPropertyList DataStructure::propertiesByLinkType(PropertyType ptype, PropertyLinkType ltype) const
{
    Z_CHECK(ptype == PropertyType::Field || ptype == PropertyType::ColumnFull);
    Z_CHECK(ltype != PropertyLinkType::Undefined);

    DataPropertyList res;
    for (auto& p : qAsConst(expanded())) {
        if (p.propertyType() != ptype)
            continue;
        auto links = p.links();
        for (const auto& l : qAsConst(links)) {
            if (l->type() == ltype)
                res << p;
        }
    }
    return res;
}

DataPropertyList DataStructure::propertiesByLookupRequest(const Uid& uid) const
{
    Z_CHECK(uid.isValid());

    DataPropertyList res;
    for (auto& p : qAsConst(expanded())) {
        if (p.lookup() != nullptr && p.lookup()->type() == LookupType::Request && p.lookup()->requestServiceUid() == uid)
            res << p;
    }
    return res;
}

DataPropertyList DataStructure::propertiesByLookupRequest(const EntityCode& uid) const
{
    return propertiesByLookupRequest(Uid::uniqueEntity(uid));
}

DataProperty DataStructure::nameProperty() const
{
    auto props = propertiesByOptions(PropertyType::Field, PropertyOption::FullName);
    if (!props.isEmpty())
        return props.constFirst();

    props = propertiesByOptions(PropertyType::Field, PropertyOption::Name);
    if (!props.isEmpty())
        return props.constFirst();

    return DataProperty();
}

DataProperty DataStructure::nameColumn(const DataProperty& dataset) const
{
    auto props = datasetColumnsByOptions(dataset, PropertyOption::FullName);
    if (!props.isEmpty())
        return props.constFirst();

    props = datasetColumnsByOptions(dataset, PropertyOption::Name);
    if (!props.isEmpty())
        return props.constFirst();

    return DataProperty();
}

DataProperty DataStructure::nameColumn(const PropertyID& dataset_property_id) const
{
    return nameColumn(property(dataset_property_id));
}

bool DataStructure::hasConstraints() const
{
    for (auto& p : properties()) {
        if (!p.constraints().isEmpty())
            return true;
    }
    return false;
}

DataPropertyList DataStructure::datasetColumnsByOptions(const DataProperty& dataset, const PropertyOptions& options) const
{
    Z_CHECK(dataset.propertyType() == PropertyType::Dataset);
    DataPropertyList res;
    for (auto& p : propertiesByOptions(PropertyType::ColumnFull, options)) {
        if (p.dataset() == dataset)
            res << p;
    }
    return res;
}

DataPropertyList DataStructure::datasetColumnsByOptions(const PropertyID& dataset_property_id, const PropertyOptions& options) const
{
    return datasetColumnsByOptions(property(dataset_property_id), options);
}

int DataStructure::column(const DataProperty& column_property) const
{
    Z_CHECK(column_property.entity() == entity());
    return column_property.pos();
}

int DataStructure::column(const PropertyID& column_property_id) const
{
    return property(column_property_id).pos();
}

bool DataStructure::isSingleDataset() const
{
    return _single_dataset_id.isValid();
}

PropertyID DataStructure::singleDatasetId() const
{
    Z_CHECK_X(_single_dataset_id.isValid(), QString("single dataset error %1").arg(id().value()));
    return _single_dataset_id;
}

DataProperty DataStructure::singleDataset() const
{
    return property(singleDatasetId());
}

PropertyID DataStructure::maxPropertyId() const
{
    Z_CHECK(isValid());
    expanded(); // инициализация
    return
        // Т.к. свойства начинаются с 1, то _properties всегда будет иметь пустой элемент с индексом 0
        _properties.count() <= 1 ? PropertyID() : PropertyID(_properties.count() - 1);
}

QVariant DataStructure::variant() const
{
    return QVariant::fromValue(*this);
}

const DataPropertySet& DataStructure::properties() const
{
    return expanded();
}

const DataPropertySet& DataStructure::propertiesMain() const
{
    return _properties_list_main;
}

DataPropertyList DataStructure::propertiesSorted() const
{
    DataPropertyList res = properties().values();
    std::sort(res.begin(), res.end(), [](const DataProperty& p1, const DataProperty& p2) -> bool { return p1.id() < p2.id(); });
    return res;
}

DataPropertyList DataStructure::propertiesMainSorted() const
{
    DataPropertyList res = propertiesMain().values();
    std::sort(res.begin(), res.end(), [](const DataProperty& p1, const DataProperty& p2) -> bool { return p1.id() < p2.id(); });
    return res;
}

const DataPropertyList& DataStructure::propertiesByType(PropertyType type) const
{
    Z_CHECK(type != PropertyType::Undefined);
    auto pt = _properties_by_type.constFind(type);
    if (pt == _properties_by_type.constEnd()) {
        DataPropertyList res;
        if (type == PropertyType::Entity) {
            res = DataPropertyList {_entity};

        } else {
            for (auto& p : qAsConst(expanded())) {
                if (p.propertyType() == type)
                    res << p;
            }

            _properties_by_type[type] = res;
        }

        pt = _properties_by_type.constFind(type);
        Z_CHECK(pt != _properties_by_type.constEnd());
    }

    return pt.value();
}

DataStructure DataStructure::fromVariant(const QVariant& v)
{
    return v.value<DataStructure>();
}

void DataStructure::getLookupInfo(const DataProperty& property, DataProperty& dataset, int& display_column, int& data_column) const
{
    Z_CHECK_NULL(property.lookup());
    dataset = this->property(property.lookup()->datasetPropertyID());
    display_column = dataset.columnPos(property.lookup()->displayColumnID());
    data_column = dataset.columnPos(property.lookup()->dataColumnID());
}

QJsonObject DataStructure::toJson() const
{
    QJsonObject json;

    json["id"] = id().value();
    if (!isEntityIndependent())
        json["entity_code"] = _entity.entityCode().value();
    json["structure_version"] = _structure_version;

    QJsonArray properties;
    for (auto& p : _properties_list_main) {
        if (p.propertyType() == PropertyType::Dataset)
            continue;

        properties.append(p.toJson());
    }
    for (auto& p : _properties_list_main) {
        if (p.propertyType() != PropertyType::Dataset)
            continue;

        properties.append(p.toJson());
    }

    json["properties"] = properties;

    return json;
}

DataStructurePtr DataStructure::fromJson(const QJsonObject& source, Error& error)
{
    int id = 1;
    if (source.contains("id")) {
        auto v = source.value("id");
        if (v.isNull() || v.isUndefined()) {
            error = Error::jsonError({"DataStructure", "id"});
            return nullptr;
        }
        id = v.toInt();
        if (id <= 0) {
            error = Error::jsonError({"DataStructure", "id"});
            return nullptr;
        }
    }

    int entity_code = 0;
    int structure_version = 1;
    if (source.contains("entity_code")) {
        auto v = source.value("entity_code");
        entity_code = -1;
        if (!v.isUndefined()) {
            entity_code = v.toInt(-1);
            if (entity_code <= 0) {
                error = Error::jsonError({"DataStructure", "entity_code"});
                return nullptr;
            }
        }

        v = source.value("structure_version");
        if (v.isNull() || v.isUndefined()) {
            error = Error::jsonError({"DataStructure", "structure_version"});
            return nullptr;
        }
        structure_version = static_cast<int>(v.toInt(-1));
        if (structure_version < 0) {
            error = Error::jsonError({"DataStructure", "structure_version"});
            return nullptr;
        }
    }

    if (entity_code > 0) {
        // ищем зарегистрированную структуру
        auto structure = DataStructure::structure(EntityCode(entity_code), false);
        if (structure == nullptr) {
            error = Error::jsonError({"DataStructure", QStringLiteral("unknown entity: %1").arg(entity_code)});
            return nullptr;
        }
        if (structure->structureVersion() != structure_version) {
            error = Error::jsonError(
                {"DataStructure", QStringLiteral("version mismatch: json(%1) real(%2)").arg(structure_version).arg(structure->structureVersion())});
            return nullptr;
        }
        return structure;
    }

    auto v = source.value("properties");
    if (v.isNull() || v.isUndefined() || !v.isArray()) {
        error = Error::jsonError({"DataStructure", "properties"});
        return nullptr;
    }

    DataPropertyList properties;
    auto json_properties = v.toArray();
    for (const auto& p : json_properties) {
        auto property = DataProperty::fromJson(p.toObject(), error);
        if (properties.contains(property)) {
            error = Error(QString("DataStructure, property diplication: %1").arg(property.id().value()));
            return nullptr;
        }
        properties << DataProperty::fromJson(p.toObject(), error);
        if (error.isError())
            return nullptr;
    }

    return independentStructure(PropertyID(id), structure_version, properties);
}

void DataStructure::setSameProperties(const PropertyIDList& properties)
{
    auto props = std::make_shared<PropertyIDSet>();

    for (auto& p : properties) {
        Z_CHECK(!props->contains(p));
        props->insert(p);
        _same_properties_hash.insert(p, props);
    }

    _same_properties << properties;
}

void DataStructure::setSameProperties(const DataPropertyList& properties)
{
    PropertyIDList list;
    for (auto& p : properties) {
        Z_CHECK(contains(p));
        list << p.id();
    }
    setSameProperties(list);
}

QList<std::shared_ptr<PropertyIDSet>> DataStructure::getSameProperties(const PropertyID& p) const
{
    return _same_properties_hash.values(p);
}

QList<std::shared_ptr<PropertyIDSet>> DataStructure::getSameProperties(const DataProperty& p) const
{
    return getSameProperties(p.id());
}

DataProperty DataStructure::samePropertiesMaster(const DataProperty& property) const
{
    QMutexLocker lock(&_same_props_master_mutex);

    // выбираем первый, у которого нет PropertyOption::DBWriteIgnored
    auto res = _same_props_master_info.constFind(property.id());
    if (res != _same_props_master_info.constEnd())
        return *res;

    DataProperty control;
    QList<std::shared_ptr<PropertyIDSet>> same_props = getSameProperties(property);
    if (!same_props.isEmpty()) {
        for (const std::shared_ptr<PropertyIDSet>& same_group : qAsConst(same_props)) {
            if (control.isValid())
                break;
            for (const PropertyID& prop : qAsConst(*same_group)) {
                auto p = this->property(prop);
                if (p.options().testFlag(PropertyOption::DBWriteIgnored))
                    continue;

                control = p;
                break;
            }
        }
    }
    _same_props_master_info[property.id()] = control;
    return control;
}

DataProperty DataStructure::samePropertiesMaster(const PropertyID& p) const
{
    return samePropertiesMaster(property(p));
}

const QList<PropertyIDList>& DataStructure::sameProperties() const
{
    return _same_properties;
}

const QSet<DataProperty>& DataStructure::constraintsShowHiglightProperties() const
{
    if (_constraints_show_higlight_properties == nullptr) {
        _constraints_show_higlight_properties = std::make_unique<QSet<DataProperty>>();
        for (auto& p_a : properties()) {
            for (auto& c : p_a.constraints()) {
                if (c->showHiglightProperty().isValid())
                    *_constraints_show_higlight_properties << property(c->showHiglightProperty());
            }
        }
    }

    return *_constraints_show_higlight_properties;
}

void DataStructure::invalidateLookupModels() const
{
    DataPropertyList properties = propertiesByType(PropertyType::Field);
    properties << propertiesByType(PropertyType::ColumnFull);

    for (auto& p : qAsConst(properties)) {
        if (p.lookup()) {
            if (p.lookup()->listEntity().isValid()) {
                auto entity_uid = p.lookup()->listEntity();
                if (Core::isCatalogUid(entity_uid) && !Core::catalogsInterface()->isCatalogUpdatable(entity_uid))
                    continue;
                if (Core::catalogsInterface()->isCatalogFake(entity_uid))
                    continue;

                Core::databaseManager()->entityChanged(CoreUids::CORE, entity_uid);
            }

            if (p.lookup()->requestServiceUid() == CoreUids::MODEL_SERVICE)
                Core::modelManager()->invalidate({p.lookup()->modelServiceOptions().lookup_entity_uid});
        }
    }
}

bool PropertyLookup::isValid() const
{
    return _type != LookupType::Undefined;
}

PropertyID PropertyLookup::masterPropertyID() const
{
    return _master_property_id;
}

LookupType PropertyLookup::type() const
{
    return _type;
}

Uid PropertyLookup::listEntity() const
{
    return _list_entity;
}

PropertyID PropertyLookup::datasetPropertyID() const
{
    Z_CHECK(_type == LookupType::Dataset && _list_entity.isValid());
    if (!_dataset_property_id.isValid()) {
        // надо определить автоматически
        _dataset_property_id = DataStructure::structure(_list_entity)->singleDatasetId();
    }

    return _dataset_property_id;
}

DataProperty PropertyLookup::datasetProperty() const
{
    return DataProperty::property(_list_entity, datasetPropertyID());
}

PropertyID PropertyLookup::displayColumnID() const
{
    Z_CHECK(_type == LookupType::Dataset && _list_entity.isValid() && datasetPropertyID().isValid());

    if (!_display_column_id.isValid()) {
        // надо определить автоматически
        auto props = DataProperty::datasetColumnsByOptions(_list_entity, _dataset_property_id, PropertyOption::FullName);
        if (props.isEmpty())
            props = DataProperty::datasetColumnsByOptions(_list_entity, _dataset_property_id, PropertyOption::Name);

        if (props.isEmpty()) {
            // если есть всего одно видимое свойство, то будем считать его как PropertyOption::Name
            auto ds_prop = DataStructure::structure(_list_entity)->property(_dataset_property_id);
            DataProperty prop_found;
            for (const auto& c : ds_prop.columns()) {
                if (c.options().testFlag(PropertyOption::Hidden) || c.options().testFlag(PropertyOption::Id))
                    continue;
                if (prop_found.isValid()) {
                    prop_found.clear();
                    break;
                }
                prop_found = c;
            }
            if (prop_found.isValid())
                props << prop_found;
        }
        Z_CHECK_X(!props.isEmpty(), QString("Для набора данных %1::%2 не найдено свойство с параметром PropertyOption::Name")
                                        .arg(_list_entity.toPrintable())
                                        .arg(_dataset_property_id.value()));
        _display_column_id = props.first().id();
    }

    return _display_column_id;
}

DataProperty PropertyLookup::displayColumn() const
{
    return DataProperty::property(_list_entity, displayColumnID());
}

int PropertyLookup::displayColumnRole() const
{
    return _display_column_role;
}

PropertyID PropertyLookup::dataColumnID() const
{
    Z_CHECK(_type == LookupType::Dataset && _list_entity.isValid() && datasetPropertyID().isValid());

    if (!_data_column_id.isValid()) {
        // надо определить автоматически
        DataPropertyList column = DataStructure::structure(_list_entity)->datasetColumnsByOptions(_dataset_property_id, PropertyOption::Id);
        Z_CHECK_X(!column.isEmpty(), QString("Для набора данных %1::%2 не найдено свойство с параметром PropertyOption::Id")
                                         .arg(_list_entity.toPrintable())
                                         .arg(_dataset_property_id.value()));

        _data_column_id = column.constFirst().id();
    }

    return _data_column_id;
}

DataProperty PropertyLookup::dataColumn() const
{
    return DataProperty::property(_list_entity, dataColumnID());
}

int PropertyLookup::dataColumnRole() const
{
    return _data_column_role;
}

PropertyID PropertyLookup::datasetKeyColumnID() const
{
    return _dataset_key_column_id;
}

int PropertyLookup::datasetKeyColumnRole() const
{
    return _dataset_key_column_id_role;
}

const QVariantList& PropertyLookup::listValues() const
{
    return _list_values;
}

const QStringList& PropertyLookup::listNames() const
{
    return _list_names;
}

QString PropertyLookup::listName(const QVariant& id_value) const
{
    int index = _list_values.indexOf(id_value);
    if (index < 0) {
        if (!id_value.isValid() || id_value.isNull())
            return QString();

        return QString(ZF_TR(ZFT_LOOKUP_LIST_NOT_FOUND).arg(id_value.toString()));
    }
    return _list_names.at(index);
}

bool PropertyLookup::isEditable() const
{
    return _editable;
}

Uid PropertyLookup::requestServiceUid() const
{
    return _request_service_uid;
}

QString PropertyLookup::requestType() const
{
    return _request_type;
}

I_ExternalRequest* PropertyLookup::requestService() const
{
    if (_request_service == nullptr) {
        Z_CHECK(_request_service_uid.isValid());
        _request_service = Utils::getInterface<I_ExternalRequest>(_request_service_uid);
    }
    Z_CHECK_NULL(_request_service);
    return _request_service;
}

ModelServiceLookupRequestOptions PropertyLookup::modelServiceOptions() const
{
    Z_CHECK(_request_service_uid == CoreUids::MODEL_SERVICE);
    return _data.value<ModelServiceLookupRequestOptions>();
}

const PropertyIDList& PropertyLookup::parentKeyProperties() const
{
    return _parent_key_properties;
}

const PropertyIDMultiMap& PropertyLookup::attributes() const
{
    return _attributes;
}

const QStringList& PropertyLookup::attributesToClear() const
{
    return _attributes_to_clear;
}

QVariant PropertyLookup::data() const
{
    return _data;
}

PropertyLookup::PropertyLookup()
{
}

PropertyLookup::PropertyLookup(DataProperty* master_property, LookupType type, const Uid& list_entity, const PropertyID& dataset_property_id,
    const PropertyID& display_column_id, int display_column_role, const PropertyID& data_column_id, int data_column_role,
    const PropertyID& dataset_key_column_id, int dataset_key_column_id_role, const QVariantList& list_values, const QStringList& list_names, bool editable,
    const Uid& request_service_uid, const QString& request_type, const PropertyIDList& parent_key_properties, const QVariant& data,
    const PropertyIDMultiMap& attributes, const QStringList& attributes_to_clear)
    : _type(type)
    , _list_entity(list_entity)
    , _dataset_property_id(dataset_property_id)
    , _display_column_id(display_column_id)
    , _display_column_role(display_column_role)
    , _data_column_id(data_column_id)
    , _data_column_role(data_column_role)
    , _dataset_key_column_id(dataset_key_column_id)
    , _dataset_key_column_id_role(dataset_key_column_id_role)
    , _list_values(list_values)
    , _editable(editable)
    , _request_service_uid(request_service_uid)
    , _request_type(request_type)
    , _parent_key_properties(parent_key_properties)
    , _attributes(attributes)
    , _attributes_to_clear(attributes_to_clear)
    , _data(data)
{
    Z_CHECK_NULL(master_property);
    _master_entity = master_property->entityCode();
    _master_property_id = master_property->id();

    for (QString a : _attributes_to_clear) {
        Z_CHECK(_attributes.contains(a));
    }

    if (master_property->options().testFlag(PropertyOption::SimpleDataset) && !_dataset_key_column_id.isValid()) {
        // надо определить автоматически
        DataPropertyList column = master_property->columnsByOptions(PropertyOption::SimpleDatasetId);
        Z_CHECK_X(!column.isEmpty(), QString("Для набора данных %1::%2 не найдено свойство с параметром PropertyOption::SimpleDatasetId")
                                         .arg(_master_entity.value())
                                         .arg(_master_property_id.value()));
        DataPropertyList id_column = master_property->columnsByOptions(PropertyOption::Id);
        if (!id_column.isEmpty()) {
            Z_CHECK_X(id_column != column, QString("Для набора данных %1::%2 свойство PropertyOption::SimpleDatasetId совпадает со "
                                                   "свойством PropertyOption::Id")
                                               .arg(_master_entity.value())
                                               .arg(_master_property_id.value()));
        }

        _dataset_key_column_id = column.constFirst().id();
    }

    if (list_names.isEmpty()) {
        for (QVariant& v : _list_values) {
            _list_names << v.toString();
        }
    } else {
        Z_CHECK(list_values.count() == list_names.count());
        for (auto& n : qAsConst(list_names)) {
            _list_names << translate(n);
        }
    }

    if (!_parent_key_properties.isEmpty()) {
        Z_CHECK(_request_service_uid.isValid());
    }

    init();
}

void PropertyLookup::init()
{
    if (!_list_entity.isValid()) {
        // Пустой код допустим только для ItemModel (в таком случае модель запрашивается через интерфейс
        // I_DataWidgetsLookupModels). Либо при использовании List
        Z_CHECK(_type == LookupType::Dataset || _type == LookupType::List || _type == LookupType::Request);
    }

    if (_type == LookupType::Dataset)
        Z_CHECK(_list_values.isEmpty());

    if (!_list_values.isEmpty())
        Z_CHECK(_type == LookupType::List);

    if (_type == LookupType::Request) {
        Z_CHECK(_request_service_uid.isEntity());
    }
}

PropertyConstraint::PropertyConstraint()
{
}

PropertyConstraint::PropertyConstraint(const PropertyConstraint& p)
    : _type(p._type)
    , _message(p._message)
    , _highlight_type(p._highlight_type)
    , _options(p._options)
    , _add_data(p._add_data)
    , _c_options(p._c_options)
    , _show_higlight_property(p._show_higlight_property)
    , _highlight_options(p._highlight_options)
{
}

PropertyConstraint& PropertyConstraint::operator=(const PropertyConstraint& p)
{
    _type = p._type;
    _message = p._message;
    _highlight_type = p._highlight_type;
    _options = p._options;
    _add_data = p._add_data;
    _c_options = p._c_options;
    _show_higlight_property = p._show_higlight_property;
    _highlight_options = p._highlight_options;
    return *this;
}

bool PropertyConstraint::isValid() const
{
    return _type != ConditionType::Undefined;
}

ConditionType PropertyConstraint::type() const
{
    return _type;
}
QVariant PropertyConstraint::addData() const
{
    return _add_data;
}

void* PropertyConstraint::addPtr() const
{
    return _add_ptr;
}

ConstraintOptions PropertyConstraint::options() const
{
    return _c_options;
}

PropertyID PropertyConstraint::showHiglightProperty() const
{
    return _show_higlight_property;
}

QRegularExpression PropertyConstraint::regularExpression() const
{
    return _regexp;
}

QValidator* PropertyConstraint::validator() const
{
    return _type == ConditionType::Validator ? static_cast<QValidator*>(_add_ptr) : nullptr;
}

QString PropertyConstraint::message() const
{
    if (_message.isEmpty())
        return QString();

    return _options.testFlag(PropertyOption::NoTranslate) ? _message : translate(_message);
}

InformationType PropertyConstraint::highlightType() const
{
    return _highlight_type;
}

HighlightOptions PropertyConstraint::highlightOptions() const
{
    return _highlight_options;
}

PropertyConstraint::PropertyConstraint(ConditionType type, const QString& message, InformationType highlight_type, const PropertyOptions& options,
    QVariant add_data, void* add_ptr, ConstraintOptions c_options, const PropertyID& show_higlight_property, const HighlightOptions& highlight_options)
    : _type(type)
    , _message(message)
    , _highlight_type(highlight_type)
    , _options(options)
    , _add_data(add_data)
    , _add_ptr(add_ptr)
    , _c_options(c_options)
    , _show_higlight_property(show_higlight_property)
    , _highlight_options(highlight_options)
{
    Z_CHECK(_type != ConditionType::Undefined);
    Z_CHECK(_highlight_type != InformationType::Invalid);

    if (_type == ConditionType::RegExp) {
        _regexp = QRegularExpression(_add_data.toString());
        Z_CHECK_X(_regexp.isValid(), QString("%1, %2").arg(_regexp.pattern(), _regexp.errorString()));
    }

    if (_type == ConditionType::Validator)
        Z_CHECK_NULL(_add_ptr);
}

PropertyLink::PropertyLink()
{
}

PropertyLink::PropertyLink(const PropertyLink& l)
    : _type(l._type)
    , _main_property_id(l._main_property_id)
    , _linked_property_id(l._linked_property_id)
    , _data_source_entity(l._data_source_entity)
    , _data_source_column_id(l._data_source_column_id)
    , _data_source_priority(l._data_source_priority)
{
}

PropertyLink& PropertyLink::operator=(const PropertyLink& l)
{
    _type = l._type;
    _main_property_id = l._main_property_id;
    _linked_property_id = l._linked_property_id;
    _data_source_entity = l._data_source_entity;
    _data_source_column_id = l._data_source_column_id;
    _data_source_priority = l._data_source_priority;
    return *this;
}

bool PropertyLink::operator==(const PropertyLink& l) const
{
    return _type == l._type && _main_property_id == l._main_property_id && _linked_property_id == l._linked_property_id
           && _data_source_entity == l._data_source_entity && _data_source_column_id == l._data_source_column_id
           && _data_source_priority == l._data_source_priority;
}

bool PropertyLink::isValid() const
{
    return _type != PropertyLinkType::Undefined;
}

PropertyID PropertyLink::mainPropertyId() const
{
    return _main_property_id;
}

PropertyID PropertyLink::linkedPropertyId() const
{
    return _linked_property_id;
}

Uid PropertyLink::dataSourceEntity() const
{
    return _data_source_entity;
}

PropertyID PropertyLink::dataSourceColumnId() const
{
    return _data_source_column_id;
}

const PropertyIDList& PropertyLink::dataSourcePriority() const
{
    return _data_source_priority;
}

PropertyLinkType PropertyLink::type() const
{
    return _type;
}

PropertyLink::PropertyLink(const PropertyID& main_property_id, const PropertyID& linked_property_id)
    : _type(PropertyLinkType::LookupFreeText)
    , _main_property_id(main_property_id)
    , _linked_property_id(linked_property_id)

{
    Z_CHECK(_main_property_id.isValid());
    Z_CHECK(_linked_property_id.isValid());
}

PropertyLink::PropertyLink(
    const PropertyID& main_property_id, const PropertyID& linked_property_id, const Uid& data_source_entity, const PropertyID& data_source_column_id)
    : _type(PropertyLinkType::DataSource)
    , _main_property_id(main_property_id)
    , _linked_property_id(linked_property_id)
    , _data_source_entity(data_source_entity)
    , _data_source_column_id(data_source_column_id)
{
    Z_CHECK(_main_property_id.isValid());
    Z_CHECK(_linked_property_id.isValid());
    Z_CHECK(_data_source_entity.isValid());
    Z_CHECK(_data_source_column_id.isValid());
}

PropertyLink::PropertyLink(const PropertyIDList& sources)
    : _type(PropertyLinkType::DataSourcePriority)
    , _data_source_priority(sources)
{
    Z_CHECK(!_data_source_priority.isEmpty());
}

} // namespace zf

//! Версия данных стрима DataProperty
static int _DataPropertyStreamVersion = 11;
QDataStream& operator<<(QDataStream& out, const zf::DataProperty& obj)
{
    using namespace zf;

    out << _DataPropertyStreamVersion;
    out << obj.isValid();
    if (!obj.isValid())
        return out;

    toStreamInt(out, obj.propertyType());

    // надо найти и сохранить идентификатор сущности для которой было создано данное свойство
    // в противном случае при восстановлении свойства из стрима, мы не сможем по коду восстановить свойство для динамически создаваемых структур,
    // которые зависят не только от кода сущности, но и от id объекта
    out << obj.entityUid();

    out << obj.id();
    out << obj.isEntityIndependent();
    if (!obj.isEntityIndependent()) {
        out << obj.entityCode();
        return out;
    }

    toStreamInt(out, obj.dataType());
    out << obj.options();
    out << obj.defaultValue();
    out << (obj._d->dataset == nullptr || obj.isColumn() // борьба с бесконечной рекурсией
                ? zf::DataProperty()
                : obj.dataset());
    out << (obj._d->column_group == nullptr || obj.propertyType() == PropertyType::ColumnGroup // борьба с бесконечной рекурсией
                ? zf::DataProperty()
                : obj.columnGroup());
    out << (obj._d->row == nullptr ? zf::DataProperty() : obj.row());
    out << obj.rowId();
    out << (obj._d->column == nullptr ? zf::DataProperty() : obj.column());
    out << (obj.propertyType() == PropertyType::Dataset ? obj.columns() : DataPropertyList());

    out << obj.translationID();

    out << (obj._d->lookup == nullptr ? PropertyLookup() : *obj._d->lookup);

    out << obj._d->links.count();
    for (const auto& l : qAsConst(obj._d->links)) {
        out << *l;
    }

    out << obj._d->column_id_option;

    return out;
}

QDataStream& operator>>(QDataStream& in, zf::DataProperty& obj)
{
    using namespace zf;

    int version;
    in >> version;
    if (version != _DataPropertyStreamVersion) {
        obj.clear();
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    bool valid;
    in >> valid;
    if (!valid) {
        obj.clear();
        return in;
    }

    PropertyType property_type;
    fromStreamInt(in, property_type);

    Uid entity_uid;
    in >> entity_uid;

    int id;
    in >> id;

    bool independent;
    in >> independent;
    if (!independent) {
        int entity_code;
        in >> entity_code;
        if (in.status() != QDataStream::Ok || entity_code == Consts::INVALID_ENTITY_CODE || id < 0) {
            obj.clear();
            if (in.status() == QDataStream::Ok)
                in.setStatus(QDataStream::ReadCorruptData);

        } else {
            if (property_type == PropertyType::Entity) {
                auto structure = DataStructure::structure(EntityCode(entity_code), false);
                if (structure == nullptr) {
                    Core::logError(QStringLiteral("Stream error: property %1").arg(entity_code));
                    obj.clear();
                    in.setStatus(QDataStream::ReadCorruptData);

                } else {
                    obj = structure->entity();
                }

            } else {
                if (!entity_uid.isValid()) {
                    obj = DataProperty::property(EntityCode(entity_code), PropertyID(id), false);
                    if (!obj.isValid()) {
                        Core::logError(QStringLiteral("Stream error: property %1:%2 not found").arg(entity_code).arg(id));
                        obj.clear();
                        in.setStatus(QDataStream::ReadCorruptData);
                    }

                } else {
                    obj = DataProperty::property(entity_uid, PropertyID(id), false);
                    if (!obj.isValid()) {
                        Core::logError(QStringLiteral("Stream error: property %1:%2 not found").arg(entity_code).arg(id));
                        obj.clear();
                        in.setStatus(QDataStream::ReadCorruptData);
                    }
                }
            }
        }

        return in;
    }

    DataType data_type;
    PropertyOptions options;
    DataProperty dataset;
    DataProperty column_group;
    QVariant default_value;
    DataProperty row;
    RowID row_id;
    DataProperty column;
    DataPropertyList columns;
    QString translation_id;

    fromStreamInt(in, data_type);

    in >> options;
    in >> default_value;
    in >> dataset;
    in >> column_group;
    in >> row;
    in >> row_id;
    in >> column;
    in >> columns;

    in >> translation_id;

    PropertyLookup lookup;
    in >> lookup;

    if (in.status() != QDataStream::Ok || id < 0) {
        obj.clear();
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
    } else {
        obj = DataProperty(PropertyID(id), property_type, data_type, options, DataProperty(), EntityCode(), Uid(), dataset, column_group, default_value, row,
            row_id, column, translation_id);

        if (lookup.isValid())
            obj._d->lookup = Z_MAKE_SHARED(PropertyLookup, lookup);

        if (!columns.isEmpty()) {
            for (auto& p : columns) {
                // это свойство не инициализировалось из-за бесконечной рекурсии
                if (obj.propertyType() == PropertyType::Dataset)
                    p.setDataset(obj);
                else if (obj.propertyType() == PropertyType::ColumnGroup)
                    p.setColumnGroup(obj);
                else
                    Z_HALT_INT;
            }

            obj.setColumns(columns);
        }
    }

    int link_count;
    in >> link_count;
    if (in.status() != QDataStream::Ok) {
        obj.clear();

    } else {
        for (int i = 0; i < link_count; i++) {
            PropertyLink link;
            in >> link;
            if (link.isValid())
                obj._d->links << std::shared_ptr<PropertyLink>(new PropertyLink(link));
        }
    }

    in >> obj._d->column_id_option;

    if (in.status() != QDataStream::Ok)
        obj.clear();

    return in;
}

QDataStream& operator<<(QDataStream& out, const zf::PropertyConstraint& obj)
{
    toStreamInt(out, obj._type);
    out << obj._message;
    toStreamInt(out, obj._highlight_type);
    out << obj._options;
    out << obj._add_data;
    out << obj._c_options;
    out << obj._show_higlight_property;
    out << obj._highlight_options;
    return out;
}

QDataStream& operator>>(QDataStream& in, zf::PropertyConstraint& obj)
{
    fromStreamInt(in, obj._type);
    in >> obj._message;
    fromStreamInt(in, obj._highlight_type);
    in >> obj._options;
    in >> obj._add_data;
    in >> obj._c_options;
    in >> obj._show_higlight_property;
    in >> obj._highlight_options;
    return in;
}

QDataStream& operator<<(QDataStream& out, const zf::PropertyLookup& obj)
{
    toStreamInt(out, obj._type);
    return out << obj._master_entity << obj._master_property_id << obj._list_entity << obj._dataset_property_id << obj._display_column_id
               << obj._display_column_role << obj._data_column_id << obj._data_column_role << obj._list_values << obj._list_names << obj._editable
               << obj._request_service_uid << obj._parent_key_properties << obj._request_type << obj._dataset_key_column_id << obj._data << obj._attributes
               << obj._attributes_to_clear;
}

QDataStream& operator>>(QDataStream& in, zf::PropertyLookup& obj)
{
    obj._request_service = nullptr;

    fromStreamInt(in, obj._type);

    return in >> obj._master_entity >> obj._master_property_id >> obj._list_entity >> obj._dataset_property_id >> obj._display_column_id
           >> obj._display_column_role >> obj._data_column_id >> obj._data_column_role >> obj._list_values >> obj._list_names >> obj._editable
           >> obj._request_service_uid >> obj._parent_key_properties >> obj._request_type >> obj._dataset_key_column_id >> obj._data >> obj._attributes
           >> obj._attributes_to_clear;
}

QDataStream& operator<<(QDataStream& out, const zf::DataStructure& obj)
{
    out << obj._id << obj._entity << obj._structure_version << obj._properties << obj._properties_list_main << obj.expanded() << obj._single_dataset_id;

    out << obj._same_properties.count();
    for (const zf::PropertyIDList& p : obj._same_properties) {
        out << p;
    }

    return out;
}

QDataStream& operator>>(QDataStream& in, zf::DataStructure& obj)
{
    obj._properties_list_expanded = std::make_unique<zf::DataPropertySet>();

    in >> obj._id >> obj._entity >> obj._structure_version >> obj._properties >> obj._properties_list_main >> *obj._properties_list_expanded.get()
        >> obj._single_dataset_id;

    int count;
    in >> count;

    for (int i = 0; i < count; i++) {
        zf::PropertyIDList p;
        in >> p;
        obj.setSameProperties(p);
    }

    return in;
}

QDataStream& operator<<(QDataStream& out, const zf::DataPropertyPtr& obj)
{
    if (obj == nullptr)
        return out << zf::DataProperty();
    return out << *obj;
}

QDataStream& operator>>(QDataStream& in, zf::DataPropertyPtr& obj)
{
    if (obj == nullptr)
        obj = Z_MAKE_SHARED(zf::DataProperty);

    return in >> *obj;
}

QDataStream& operator<<(QDataStream& out, const zf::DataStructurePtr& obj)
{
    if (obj == nullptr)
        return out << zf::DataStructure();
    return out << *obj;
}

QDataStream& operator>>(QDataStream& in, zf::DataStructurePtr& obj)
{
    if (obj == nullptr)
        obj = Z_MAKE_SHARED(zf::DataStructure);

    return in >> *obj;
}

QDataStream& operator<<(QDataStream& out, const zf::PropertyLink& obj)
{
    return out << static_cast<int>(obj._type) << obj._main_property_id << obj._linked_property_id << obj._data_source_entity << obj._data_source_column_id
               << obj._data_source_priority;
}
QDataStream& operator>>(QDataStream& in, zf::PropertyLink& obj)
{
    int type;
    in >> type;
    obj._type = static_cast<zf::PropertyLinkType>(type);
    return in >> obj._main_property_id >> obj._linked_property_id >> obj._data_source_entity >> obj._data_source_column_id >> obj._data_source_priority;
}

QDebug operator<<(QDebug debug, const zf::DataProperty& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    using namespace zf;

    if (!c.isValid()) {
        debug << "invalid";

    } else {
        debug << "\n";
        Core::beginDebugOutput();
        debug << Core::debugIndent() << "id: " << c.id() << '\n';
        debug << Core::debugIndent() << "entityCode: " << c.entityCode() << '\n';
        debug << Core::debugIndent() << "propertyType: " << c.propertyType() << '\n';
        debug << Core::debugIndent() << "dataType: " << c.dataType() << '\n';
        debug << Core::debugIndent() << "options: " << c.options() << '\n';
        debug << Core::debugIndent() << "translationID: " << c.translationID() << '\n';
        debug << Core::debugIndent() << "name: " << c.name().simplified() << '\n';
        debug << Core::debugIndent() << "defaultValue: " << Utils::variantToString(c.defaultValue());

        if (c.propertyType() == PropertyType::Dataset) {
            auto columns = c.columns();
            if (!columns.isEmpty()) {
                debug << '\n';
                debug << Core::debugIndent() << "columns:" << '\n';

                Core::beginDebugOutput();
                for (int i = 0; i < columns.count(); i++) {
                    debug << Core::debugIndent() << QString("column(%1)").arg(columns.at(i).name().simplified()) << columns.at(i);
                    if (i != columns.count() - 1)
                        debug << '\n';
                }
                Core::endDebugOutput();
            }
        }
        Core::endDebugOutput();
    }

    return debug;
}

QDebug operator<<(QDebug debug, const zf::DataStructure* c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    using namespace zf;

    if (c == nullptr) {
        debug << "nullptr";

    } else if (!c->isValid()) {
        debug << "invalid";

    } else {
        debug << '\n';
        Core::beginDebugOutput();
        debug << Core::debugIndent() << "version: " << c->structureVersion() << '\n';
        debug << Core::debugIndent() << "entityIndependent: " << c->isEntityIndependent() << '\n';
        debug << Core::debugIndent() << "id: " << c->id() << '\n';
        debug << Core::debugIndent() << "entityCode: " << c->entityCode() << '\n';
        debug << Core::debugIndent() << "entity: " << c->entity();

        auto fields = c->propertiesByType(PropertyType::Field);
        std::sort(fields.begin(), fields.end(), [](const DataProperty& p1, const DataProperty& p2) -> bool { return p1.naturalIndex() < p2.naturalIndex(); });
        if (!fields.isEmpty()) {
            debug << '\n';
            debug << Core::debugIndent() << "fields:" << '\n';
            Core::beginDebugOutput();
            for (int i = 0; i < fields.count(); i++) {
                debug << Core::debugIndent() << QString("field(%1)").arg(fields.at(i).name().simplified()) << fields.at(i);
                if (i != fields.count() - 1)
                    debug << '\n';
            }
            Core::endDebugOutput();
        }

        auto datasets = c->propertiesByType(PropertyType::Dataset);
        if (!datasets.isEmpty()) {
            debug << '\n';
            debug << Core::debugIndent() << "datasets:" << '\n';
            Core::beginDebugOutput();
            for (int i = 0; i < datasets.count(); i++) {
                debug << Core::debugIndent() << QString("%1:").arg(datasets.at(i).name().simplified()) << datasets.at(i);
                if (i != datasets.count() - 1)
                    debug << '\n';
            }
            Core::endDebugOutput();
        }

        Core::endDebugOutput();
    }

    return debug;
}

QDebug operator<<(QDebug debug, const zf::DataStructurePtr c)
{
    return debug << c.get();
}

QDebug operator<<(QDebug debug, const zf::DataStructure& c)
{
    return debug << &c;
}
