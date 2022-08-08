#include "zf_hashed_dataset.h"
#include "zf_item_model.h"
#include "zf_core.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QSharedData>

namespace zf
{
HashedDataset::HashedDataset(const QAbstractItemModel* dataset, const QList<int>& keyColumns, const QList<int>& roles)
    : _item_model(dataset)
    , _key_columns(keyColumns)
    , _roles(roles)
{
    init();
}

HashedDataset::HashedDataset(const QAbstractItemModel* dataset, int keyColumn1, int keyColumn2, int keyColumn3,
                             int keyColumn4, int keyColumn5, int keyColumn6, int keyColumn7, int keyColumn8,
                             int keyColumn9)
    : _item_model(dataset)
{
    initKeyColumns(
            keyColumn1, keyColumn2, keyColumn3, keyColumn4, keyColumn5, keyColumn6, keyColumn7, keyColumn8, keyColumn9);
    init();
}

HashedDataset::~HashedDataset()
{
}

const DataContainer* HashedDataset::dataContainer() const
{
    return _data_container;
}

DataProperty HashedDataset::datasetProperty() const
{
    return _dataset_property;
}

bool HashedDataset::isCaseInsensitive(int keyColumn) const
{
    return _case_insensitive.value(keyColumn, false);
}

void HashedDataset::setDataContainer(const DataContainer* data, const DataProperty& dataset_property)
{
    Z_CHECK_NULL(data);

    _data_container = data;
    _dataset_property = dataset_property;

    connectToDataContainer();
}

void HashedDataset::setCaseInsensitive(int keyColumn, bool isCaseInsensitive)
{
    _case_insensitive[keyColumn] = isCaseInsensitive;

    QList<int> ciCols;
    auto keys = _case_insensitive.keys();
    for (int c : qAsConst(keys)) {
        if (_case_insensitive.value(c))
            ciCols << c;
    }

    _unique_key = uniqueKeyGenerate(_key_columns, ciCols, _roles);
    clear();

    prepareCaseInsensitive();
}

void HashedDataset::setCaseInsensitive(const QList<int>& case_insensitive_columns)
{
    _case_insensitive.clear();
    for (int c : qAsConst(case_insensitive_columns)) {
        _case_insensitive[c] = true;
    }

    _unique_key = uniqueKeyGenerate(_key_columns, case_insensitive_columns, _roles);
    clear();

    prepareCaseInsensitive();
}

bool HashedDataset::isAutoUpdate() const
{
    return _is_auto_update;
}

void HashedDataset::setAutoUpdate(bool b)
{
    _is_auto_update = b;
}

QList<int> HashedDataset::keyColumns() const
{
    return _key_columns;
}

QList<int> HashedDataset::roles() const
{
    return _roles;
}

QString HashedDataset::uniqueKey() const
{
    return _unique_key;
}

QString HashedDataset::uniqueKeyGenerate(const QList<int>& columns, const QList<int>& case_insensitive_columns,
                                         const QList<int>& roles)
{
    QString key;
    for (int i = 0; i < columns.count(); i++) {
        key += QString::number(columns.at(i))
               + (case_insensitive_columns.contains(i) ? QStringLiteral("@") : QStringLiteral("$"))
               + Consts::KEY_SEPARATOR + (roles.isEmpty() ? QString() : QString::number(roles.at(i)));
    }
    return key;
}

Rows HashedDataset::findRows(const QVariantList& values) const
{
    Z_CHECK(_customize == nullptr);

    Z_CHECK(values.count() == _key_columns.count());

    if (_item_model->rowCount() == 0)
        return Rows();

    if (!_hash_generated)
        const_cast<HashedDataset*>(this)->updateHash();

    QString key = generateKey(values);
    if (key.isEmpty())
        return Rows();
    else {
        return findRowsByHash(key);
    }
}

Rows HashedDataset::findRows(const QVariant& value) const
{
    QVariantList values;
    values << value;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2) const
{
    QVariantList values;
    values << value1 << value2;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3) const
{
    QVariantList values;
    values << value1 << value2 << value3;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4, const QVariant& value5) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4 << value5;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4, const QVariant& value5, const QVariant& value6) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4 << value5 << value6;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4, const QVariant& value5, const QVariant& value6,
                             const QVariant& value7) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4 << value5 << value6 << value7;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4, const QVariant& value5, const QVariant& value6,
                             const QVariant& value7, const QVariant& value8) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4 << value5 << value6 << value7 << value8;
    return findRows(values);
}

Rows HashedDataset::findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3,
                             const QVariant& value4, const QVariant& value5, const QVariant& value6,
                             const QVariant& value7, const QVariant& value8, const QVariant& value9) const
{
    QVariantList values;
    values << value1 << value2 << value3 << value4 << value5 << value6 << value7 << value8 << value9;
    return findRows(values);
}

Rows HashedDataset::findRowsByHash(const QString& hash_key) const
{
    Z_CHECK(!hash_key.isEmpty());

    if (!_hash_generated)
        const_cast<HashedDataset*>(this)->updateHash();

    QList<HashData*> data = _hash.values(hash_key);
    Rows res;
    for (auto i = data.constBegin(); i != data.constEnd(); ++i) {
        Z_CHECK((*i)->index.isValid());
        res.append((*i)->index);
    }

    return res;
}

int HashedDataset::uniqueRowCount() const
{
    if (!_hash_generated)
        const_cast<HashedDataset*>(this)->updateHash();
    return _unique_values.count();
}

QVariantList HashedDataset::uniqueRowValues(int i) const
{
    if (!_hash_generated)
        const_cast<HashedDataset*>(this)->updateHash();
    Z_CHECK(i >= 0 && i < _unique_values.count());
    return _unique_values.at(i);
}

const QAbstractItemModel* HashedDataset::itemModel() const
{
    return _item_model;
}

int HashedDataset::rowCount(const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    return _item_model->rowCount(parent);
}

int HashedDataset::columnCount(const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    return _item_model->columnCount(parent);
}

QVariant HashedDataset::variant(int row, int column, int role, const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    Z_CHECK(row >= 0 && column >= 0 && _item_model->rowCount(parent) > row
            && _item_model->columnCount(parent) > column);
    return _item_model->index(row, column, parent).data(role);
}

QString HashedDataset::string(int row, int column, int role, const QModelIndex& parent) const
{
    return variant(row, column, role, parent).toString();
}

int HashedDataset::integer(int row, int column, int role, const QModelIndex& parent) const
{
    return variant(row, column, role, parent).toInt();
}

void HashedDataset::sl_itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    clearIfNeedHelper(topLeft, bottomRight, false);
}

void HashedDataset::sl_allPropertiesUnBlocked()
{
    clearHelper(false);
}

void HashedDataset::sl_propertyUnBlocked(const DataProperty& p)
{
    if (p == _dataset_property)
        clearHelper(false);
}

void HashedDataset::connectToDataContainer()
{
    connect(_data_container, &DataContainer::sg_allPropertiesUnBlocked, this,
            &HashedDataset::sl_allPropertiesUnBlocked);
    connect(_data_container, &DataContainer::sg_propertyUnBlocked, this, &HashedDataset::sl_propertyUnBlocked);
}

void HashedDataset::initKeyColumns(int keyColumn1, int keyColumn2, int keyColumn3, int keyColumn4, int keyColumn5,
                                   int keyColumn6, int keyColumn7, int keyColumn8, int keyColumn9)
{
    Z_CHECK(keyColumn1 >= 0);
    _key_columns << keyColumn1;
    if (keyColumn2 >= 0)
        _key_columns << keyColumn2;
    if (keyColumn3 >= 0)
        _key_columns << keyColumn3;
    if (keyColumn4 >= 0)
        _key_columns << keyColumn4;
    if (keyColumn5 >= 0)
        _key_columns << keyColumn5;
    if (keyColumn6 >= 0)
        _key_columns << keyColumn6;
    if (keyColumn7 >= 0)
        _key_columns << keyColumn7;
    if (keyColumn8 >= 0)
        _key_columns << keyColumn8;
    if (keyColumn9 >= 0)
        _key_columns << keyColumn9;
}

void HashedDataset::init()
{
    Z_CHECK_NULL(_item_model);
    Z_CHECK(!_key_columns.isEmpty());

    if (_roles.isEmpty()) {
        for (int i = 0; i < _key_columns.count(); i++)
            _roles << Qt::DisplayRole;
    } else {
        Z_CHECK(_roles.count() == _key_columns.count());
    }

    _unique_key = uniqueKeyGenerate(_key_columns, QList<int>(), _roles);

    prepareCaseInsensitive();

    connect(_item_model, &QAbstractItemModel::dataChanged, this, &HashedDataset::sl_itemDataChanged);
    connect(_item_model, &QAbstractItemModel::rowsInserted, this, [&]() { clearHelper(false); });
    connect(_item_model, &QAbstractItemModel::rowsRemoved, this, [&]() { clearHelper(false); });
    connect(_item_model, &QAbstractItemModel::modelReset, this, [&]() { clearHelper(false); });
    connect(_item_model, &QAbstractItemModel::rowsMoved, this, [&]() { clearHelper(false); });
    connect(_item_model, &QAbstractItemModel::columnsMoved, this, [&]() { clearHelper(false); });
}

void HashedDataset::clearHelper(bool force)
{
    if (!force && !_is_auto_update)
        return;

    if (!_hash_generated)
        return;

    _hash_generated = false;
    qDeleteAll(_hash);
    _hash.clear();
    _unique_values.clear();
}

void HashedDataset::clear()
{
    clearHelper(true);
}

void HashedDataset::clearIfNeed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    clearIfNeedHelper(topLeft, bottomRight, true);
}

void HashedDataset::setCustomization(const I_HashedDatasetCutomize* ci, const QString& key)
{
    if (_customize == ci && _customize_key == key)
        return;

    Z_CHECK(!key.isEmpty());

    _customize = ci;
    _customize_key = key;
    clear();
}

void HashedDataset::clearIfNeedHelper(const QModelIndex& topLeft, const QModelIndex& bottomRight, bool force)
{
    if (!force && !_is_auto_update)
        return;

    if (!_hash_generated)
        return;

    // для заблокированных наборов данных очищаем всегда
    if (_data_container == nullptr || !_data_container->isPropertyBlocked(_dataset_property)) {
        bool needUpdate = false;
        for (int i = topLeft.column(); i <= bottomRight.column(); i++) {
            if (!_key_columns.contains(i))
                continue;

            needUpdate = true;
            break;
        }

        if (!needUpdate)
            return;
    }

    clear();
}

void HashedDataset::updateHash()
{
    clear();    
    _hash.reserve(_item_model->rowCount());
    updateHashHelper(QModelIndex());
    _hash_generated = true;
}

void HashedDataset::updateHashHelper(const QModelIndex& parent)
{
    for (int row = 0; row < _item_model->rowCount(parent); row++) {
        updateRowHash(row, parent);
        updateHashHelper(_item_model->index(row, 0, parent));
    }
}

void HashedDataset::updateRowHash(int row, const QModelIndex& parent)
{
    QString key = generateRowHashKey(row, parent);
    if (!key.isEmpty()) {
        if (!_hash.contains(key)) {
            QVariantList values;
            for (int i = 0; i < _key_columns.count(); i++) {
                int col = _key_columns.at(i);
                values << _item_model->index(row, col, parent).data(_roles.at(i));
            }
            _unique_values.append(values);
        }

        _hash.insert(key, new HashData(_item_model->index(row, 0, parent)));
    }
}

QString HashedDataset::generateRowHashKey(int row, const QModelIndex& parent) const
{
    Z_CHECK(row >= 0 && row < _item_model->rowCount(parent));

    QVariantList values;
    for (int i = 0; i < _key_columns.count(); i++) {
        int col = _key_columns.at(i);
        Z_CHECK_X(col < _item_model->columnCount(),
                  utf("Колонка: %1, Всего: %2").arg(col).arg(_item_model->columnCount()));
        values << _item_model->index(row, col, parent).data(_roles.at(i));
    }

    return _customize ? _customize->hashedDatasetkeyValuesToUniqueString(_customize_key, row, parent, values)
                      : generateKey(values);
}

void HashedDataset::prepareCaseInsensitive()
{
    _case_insensitive_prepared.clear();
    for (int i = 0; i < _key_columns.count(); i++) {
        _case_insensitive_prepared << isCaseInsensitive(_key_columns.at(i));
    }
}

QString HashedDataset::generateKey(const QVariantList& values) const
{
    return Utils::generateUniqueString(values, _case_insensitive_prepared);
}

AutoHashedDataset::AutoHashedDataset(const QAbstractItemModel* dataset, bool take_ownership)
    : _item_model(dataset)
    , _take_ownership(take_ownership)
{
    Z_CHECK_NULL(_item_model);
}

AutoHashedDataset::~AutoHashedDataset()
{
    qDeleteAll(_find_by_columns_hash);
    _find_by_columns_hash.clear();

    if (_take_ownership)
        delete _item_model;
}

int AutoHashedDataset::rowCount(const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    return _item_model->rowCount(parent);
}

int AutoHashedDataset::columnCount(const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    return _item_model->columnCount(parent);
}

void AutoHashedDataset::setColumnNames(const QStringList& names)
{
    Z_CHECK_NULL(_item_model);
    Z_CHECK(names.count() == _item_model->columnCount());
    for (int i = 0; i < names.count(); i++)
        _column_names[names.at(i).toLower()] = i;
}

int AutoHashedDataset::col(const QString& column_name) const
{
    int c = _column_names.value(column_name.toLower(), -1);
    Z_CHECK_X(c >= 0, utf("Колонка %1 не найдена").arg(column_name));
    return c;
}

QVariant AutoHashedDataset::variant(int row, int column, int role, const QModelIndex& parent) const
{
    Z_CHECK_NULL(_item_model);
    Z_CHECK(row >= 0 && column >= 0 && _item_model->rowCount(parent) > row
            && _item_model->columnCount(parent) > column);
    return _item_model->index(row, column, parent).data(role);
}

QVariant AutoHashedDataset::variant(int row, const QString& column_name, int role, const QModelIndex& parent) const
{
    return variant(row, col(column_name), role, parent);
}

QString AutoHashedDataset::string(int row, int column, int role, const QModelIndex& parent) const
{
    return variant(row, column, role, parent).toString();
}

QString AutoHashedDataset::string(int row, const QString& column_name, int role, const QModelIndex& parent) const
{
    return variant(row, column_name, role, parent).toString();
}

int AutoHashedDataset::integer(int row, int column, int role, const QModelIndex& parent) const
{
    return variant(row, column, role, parent).toInt();
}

int AutoHashedDataset::integer(int row, const QString& column_name, int role, const QModelIndex& parent) const
{
    return variant(row, column_name, role, parent).toInt();
}

void AutoHashedDataset::clear()
{
    for (HashedDataset* m : qAsConst(_find_by_columns_hash)) {
        m->clear();
    }
}

void AutoHashedDataset::clearIfNeed(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    for (HashedDataset* m : qAsConst(_find_by_columns_hash)) {
        m->clearIfNeed(topLeft, bottomRight);
    }
}

void AutoHashedDataset::setDataContainer(const DataContainer* data, const DataProperty& dataset_property)
{
    Z_CHECK(data && data->contains(dataset_property));

    _data_container = data;
    _dataset_property = dataset_property;
}

Rows AutoHashedDataset::findRowsHelper(const QList<int>& columns, const QVariantList& values,
                                       const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    Z_CHECK(!columns.empty());
    Z_CHECK(columns.count() == values.count());

    return getHashedDataset(columns, case_insensitive_columns, roles)->findRows(values);
}

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QVariantList& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    return findRowsHelper(columns, values, case_insensitive_columns, roles);
}

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QStringList& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    QVariantList vlist;
    for (auto& v : qAsConst(values)) {
        vlist << v;
    }
    return findRowsHelper(columns, vlist, case_insensitive_columns, roles);
}

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QList<int>& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    QVariantList vlist;
    for (int v : qAsConst(values)) {
        vlist << v;
    }
    return findRowsHelper(columns, vlist, case_insensitive_columns, roles);
}

Rows AutoHashedDataset::findRows(const QStringList& columns, const QVariantList& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    Z_CHECK_X(!_column_names.isEmpty(), utf("Имена колонок не заданы"));
    QList<int> cols;
    for (auto& c : qAsConst(columns)) {
        cols << col(c);
    }
    return findRows(cols, values, case_insensitive_columns, roles);
}

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QList<QVariantList>& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    Rows rows;
    for (int i = 0; i < values.count(); i++) {
        QVariantList vl = values.at(i);
        rows.unite(findRowsHelper(columns, vl, case_insensitive_columns, roles));
    }

    return rows;
}

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QList<QStringList>& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
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

Rows AutoHashedDataset::findRows(const QList<int>& columns, const QList<QList<int>>& values,
                                 const QList<int>& case_insensitive_columns, const QList<int>& roles) const
{
    QList<QVariantList> vlist;

    for (int i = 0; i < values.count(); i++) {
        QVariantList vl;
        for (int j = 0; j < values.at(i).count(); j++) {
            vl << values.at(i).at(i);
        }
        vlist << vl;
    }

    return findRows(columns, vlist, case_insensitive_columns, roles);
}

Rows AutoHashedDataset::findRows(int column, const QVariant& value, bool caseInsensitive, int role) const
{
    return findRows(column, QVariantList({value}), caseInsensitive, role);
}

Rows AutoHashedDataset::findRows(int column, const QVariantList& value, bool caseInsensitive, int role) const
{
    QList<QVariantList> vl;
    for (int i = 0; i < value.count(); i++) {
        vl << QVariantList({value.at(i)});
    }

    return findRows(
            QList<int>({column}), vl, caseInsensitive ? QList<int>({column}) : QList<int>(), QList<int>({role}));
}

Rows AutoHashedDataset::findRows(int column, const QStringList& values, bool caseInsensitive, int role) const
{
    QList<QStringList> vl;
    for (int i = 0; i < values.count(); i++) {
        vl << QStringList({values.at(i)});
    }

    return findRows(
            QList<int>({column}), vl, caseInsensitive ? QList<int>({column}) : QList<int>(), QList<int>({role}));
}

Rows AutoHashedDataset::findRows(int column, const QList<int>& values, bool caseInsensitive, int role) const
{
    QList<QList<int>> vl;
    for (int i = 0; i < values.count(); i++) {
        vl << QList<int>({values.at(i)});
    }

    return findRows(
            QList<int>({column}), vl, caseInsensitive ? QList<int>({column}) : QList<int>(), QList<int>({role}));
}

Rows AutoHashedDataset::findRows(const QString& column_name, const QVariant& value, bool caseInsensitive,
                                 int role) const
{
    return findRows(col(column_name), value, caseInsensitive, role);
}

int AutoHashedDataset::uniqueRowCount(const QList<int>& columns, const QList<int>& case_insensitive_columns,
                                      const QList<int>& roles) const
{
    return getHashedDataset(columns, case_insensitive_columns, roles)->uniqueRowCount();
}

QVariantList AutoHashedDataset::uniqueRowValues(int i, const QList<int>& columns,
                                                const QList<int>& case_insensitive_columns,
                                                const QList<int>& roles) const
{
    return getHashedDataset(columns, case_insensitive_columns, roles)->uniqueRowValues(i);
}

HashedDataset* AutoHashedDataset::getHashedDataset(const QList<int>& columns,
                                                   const QList<int>& case_insensitive_columns,
                                                   const QList<int>& roles) const
{
    Z_CHECK(!columns.empty());

    QString key = HashedDataset::uniqueKeyGenerate(columns, case_insensitive_columns, roles);
    HashedDataset* ds = _find_by_columns_hash.value(key, nullptr);
    if (!ds) {
        ds = new HashedDataset(_item_model, columns, roles);
        if (_data_container)
            ds->setDataContainer(_data_container, _dataset_property);
        ds->setCaseInsensitive(case_insensitive_columns);
        _find_by_columns_hash[key] = ds;
    }

    return ds;
}

} // namespace zf
