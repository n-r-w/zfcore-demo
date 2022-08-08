#include "zf_data_hashed.h"
#include <zf_core.h>

namespace zf
{
DataHashed::DataHashed(const QMap<PropertyID, const QAbstractItemModel*>& item_models, ResourceManager* resource_manager)
{
    setResourceManager(resource_manager);

    for (auto it = item_models.constBegin(); it != item_models.constEnd(); ++it) {
        add(it.key(), it.value());
    }
}

DataHashed::DataHashed(const QMap<DataProperty, const QAbstractItemModel*>& item_models,
                       ResourceManager* resource_manager)
{
    setResourceManager(resource_manager);
    for (auto it = item_models.constBegin(); it != item_models.constEnd(); ++it) {
        Z_CHECK(it.key().propertyType() == PropertyType::Dataset);
        add(it.key().id(), it.value());
    }
}

bool DataHashed::contains(const PropertyID& id) const
{
    Z_CHECK(id.isValid());
    return _find_by_columns_hash.contains(id);
}

bool DataHashed::contains(const DataProperty& property) const
{
    return contains(property.id());
}

void DataHashed::remove(const DataProperty& property)
{
    remove(property.id());
}

void DataHashed::remove(const PropertyID& id)
{
    Z_CHECK(id.isValid());
    Z_CHECK(_find_by_columns_hash.contains(id));
    _find_by_columns_hash.remove(id);
}

void DataHashed::add(const PropertyID& id, const QAbstractItemModel* item_model)
{        
    Z_CHECK(!_find_by_columns_hash.contains(id));
    Z_CHECK(id.isValid());
    Z_CHECK_NULL(item_model);

    _find_by_columns_hash[id] = Z_MAKE_SHARED(AutoHashedDataset, item_model);
}

void DataHashed::add(const DataProperty& property, const QAbstractItemModel* item_model)
{
    add(property.id(), item_model);
}

void DataHashed::clearHash(const PropertyID& id)
{
    auto it = _find_by_columns_hash.find(id);
    Z_CHECK(it != _find_by_columns_hash.end());
    it.value()->clear();
}

void DataHashed::clearHash()
{
    for (auto it = _find_by_columns_hash.begin(); it != _find_by_columns_hash.end(); ++it) {
        it.value()->clear();
    }
}

void DataHashed::setResourceManager(ResourceManager* resource_manager)
{
    if (!_resource_manager.isNull())
        disconnect(_resource_manager, &ResourceManager::sg_freeResources, this, &DataHashed::sl_freeResource);

    _resource_manager = resource_manager;
    if (!_resource_manager.isNull())
        connect(_resource_manager, &ResourceManager::sg_freeResources, this, &DataHashed::sl_freeResource);
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QVariantList& values, const DataPropertyList& case_insensitive_columns,
                          const QList<int>& roles) const
{
    return findRowsHelper(columns, values, case_insensitive_columns, roles);
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QStringList& values, const DataPropertyList& case_insensitive_columns,
                          const QList<int>& roles) const
{
    QVariantList vlist;
    for (auto& v : values) {
        vlist << v;
    }
    return findRowsHelper(columns, vlist, case_insensitive_columns, roles);
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QList<int>& values, const DataPropertyList& case_insensitive_columns,
                          const QList<int>& roles) const
{
    QVariantList vlist;
    for (auto v : values) {
        vlist << v;
    }
    return findRowsHelper(columns, vlist, case_insensitive_columns, roles);
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QList<QVariantList>& values,
                          const DataPropertyList& case_insensitive_columns, const QList<int>& roles) const
{
    Rows rows;
    for (int i = 0; i < values.count(); i++) {
        QVariantList vl = values.at(i);
        rows.unite(findRowsHelper(columns, vl, case_insensitive_columns, roles));
    }

    return rows;
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QList<QStringList>& values,
                          const DataPropertyList& case_insensitive_columns, const QList<int>& roles) const
{
    QList<QVariantList> vlist;

    for (int i = 0; i < values.count(); i++) {
        QVariantList vl;
        for (int j = 0; j < values.at(i).count(); j++) {
            vl << values.at(i).at(j);
        }
        vlist << vl;
    }

    return findRows(columns, vlist, case_insensitive_columns, roles);
}

Rows DataHashed::findRows(const DataPropertyList& columns, const QList<QList<int>>& values,
                          const DataPropertyList& case_insensitive_columns, const QList<int>& roles) const
{
    QList<QVariantList> vlist;

    for (int i = 0; i < values.count(); i++) {
        QVariantList vl;
        for (int j = 0; j < values.at(i).count(); j++) {
            vl << values.at(i).at(j);
        }
        vlist << vl;
    }

    return findRows(columns, vlist, case_insensitive_columns, roles);
}

Rows DataHashed::findRows(const DataProperty& column, const QVariant& value, bool case_insensitive, int role) const
{
    return findRows(DataPropertyList({column}), QVariantList({value}), case_insensitive ? DataPropertyList({column}) : DataPropertyList(),
                    QList<int>({role}));
}

Rows DataHashed::findRows(const DataProperty& column, const QVariantList& values, bool case_insensitive, int role) const
{
    QList<QVariantList> vals;
    for (int i = 0; i < values.count(); i++) {
        vals << QVariantList {values.at(i)};
    }

    return findRows(DataPropertyList({column}), vals, case_insensitive ? DataPropertyList({column}) : DataPropertyList(),
                    QList<int>({role}));
}

Rows DataHashed::findRows(const DataProperty& column, const QStringList& values, bool case_insensitive, int role) const
{
    QList<QVariantList> vals;
    for (int i = 0; i < values.count(); i++) {
        vals << QVariantList {values.at(i)};
    }

    return findRows(DataPropertyList({column}), vals, case_insensitive ? DataPropertyList({column}) : DataPropertyList(),
                    QList<int>({role}));
}

Rows DataHashed::findRows(const DataProperty& column, const QList<int>& values, bool case_insensitive, int role) const
{
    QList<QVariantList> vals;
    for (int i = 0; i < values.count(); i++) {
        vals << QVariantList {values.at(i)};
    }

    return findRows(DataPropertyList({column}), vals, case_insensitive ? DataPropertyList({column}) : DataPropertyList(),
                    QList<int>({role}));
}

bool DataHashed::contains(const DataProperty& column, const QVariant& value, bool case_insensitive, int role) const
{
    return !findRows(column, value, case_insensitive, role).isEmpty();
}

void DataHashed::sl_freeResource()
{
    clearHash();
}

Rows DataHashed::findRowsHelper(const DataPropertyList& columns, const QVariantList& values,
                                const DataPropertyList& case_insensitive_columns, const QList<int>& roles) const
{
    Z_CHECK(!columns.empty());
    Z_CHECK(columns.count() == values.count());

    auto dataset = columns.constFirst().dataset();

    if (!_resource_manager.isNull())
        _resource_manager->resourceAdded();

    auto hash = _find_by_columns_hash.value(dataset.id());
    if (hash == nullptr)
        Z_HALT("Property not found: " + QString::number(dataset.id().value()));

    QList<int> columns_pos;
    for (auto& p : columns) {
        Z_CHECK(p.dataset() == dataset);
        columns_pos << dataset.columnPos(p);
    }

    QList<int> case_columns_pos;
    for (auto& p : case_insensitive_columns) {
        case_columns_pos << dataset.columnPos(p);
    }

    return hash->findRows(columns_pos, values, case_columns_pos, roles);
}
} // namespace zf
