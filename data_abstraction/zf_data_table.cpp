#include "zf_data_table.h"
#include <zf_core.h>

namespace zf
{
//! Обертка вокруг DataTable без копирования данных
class DataTableItemModel : public QAbstractItemModel
{
public:
    DataTableItemModel(const DataTable* dt)
        : _dt(dt)
    {
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        quintptr ptr = _dt->getCellPointer(row, _dt->fieldFromColumn(column));
        Z_CHECK(ptr > 0);

        return createIndex(row, column, reinterpret_cast<void*>(ptr));
    }
    QModelIndex parent(const QModelIndex&) const override { return QModelIndex(); }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : _dt->rowCount(); }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : _dt->columnCount(); }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if ((role != Qt::DisplayRole && role != Qt::EditRole) || !index.isValid())
            return QVariant();

        return index.parent().isValid() ? QVariant() : _dt->getVariant(index.row(), _dt->fieldFromColumn(index.column()));
    }
    bool setData(const QModelIndex&, const QVariant&, int) override
    {
        Z_HALT("not supported");
        return false;
    }

private:
    const DataTable* _dt;
};

DataTable::~DataTable()
{
    if (_item_model != nullptr)
        delete _item_model;
}

void DataTable::checkRow(int row) const
{
    Z_CHECK_X(row >= 0 && row < rowCount(), QString("DataTable '%1' invalid row '%2'. Row count '%3'").arg(_info).arg(row).arg(rowCount()));
}

QString DataTable::info() const
{
    return _info;
}

ItemModel* DataTable::createItemModel(QObject* parent) const
{
    ItemModel* m = new ItemModel(rowCount(), columnCount(), parent);

    for (int row = 0; row < rowCount(); row++) {
        for (int col = 0; col < columnCount(); col++) {
            m->setData(row, col, getVariant(row, fieldFromColumn(col)));
        }
    }

    return m;
}

QAbstractItemModel* DataTable::itemModel() const
{
    if (_item_model.isNull()) {
        Z_CHECK_X(!_item_model_initialized, "DataTable::itemModel была удалена за пределами DataTable");
        _item_model_initialized = true;
        _item_model = new DataTableItemModel(this);
    }

    return _item_model;
}

int DataTable::keyColumn() const
{
    return _key_column;
}

QList<int> DataTable::getIntList(int row, const FieldIdList& columns) const
{
    Z_CHECK(!columns.isEmpty());
    checkValid();
    checkRow(row);

    QList<int> res;
    for (auto& c : columns)
        res << getInt(row, c);

    return res;
}

QStringList DataTable::getStringList(int row, const FieldIdList& columns) const
{
    Z_CHECK(!columns.isEmpty());
    checkValid();
    checkRow(row);

    QStringList res;
    for (auto& c : columns)
        res << getString(row, c);

    return res;
}

DataTable::DataTable(int key_column, const QString& info)
    : _key_column(key_column < 0 ? 0 : key_column)
    , _info(info)
{
}

QList<double> zf::DataTable::getDoubleList(int row, const FieldIdList& columns) const
{
    Z_CHECK(!columns.isEmpty());
    checkValid();
    checkRow(row);

    QList<double> res;
    for (auto& c : columns)
        res << getDouble(row, c);

    return res;
}

QVariantList zf::DataTable::getVariantList(int row, const zf::FieldIdList& columns) const
{
    Z_CHECK(!columns.isEmpty());
    checkValid();
    checkRow(row);

    QVariantList res;
    for (auto& c : columns)
        res << getVariant(row, c);

    return res;
}

QList<int> DataTable::getFullColumnInt(const FieldID& column) const
{
    checkValid();
    checkColumn(column);

    QList<int> res;
    for (int row = 0; row < rowCount(); row++) {
        res << getInt(row, column);
    }
    return res;
}

QStringList DataTable::getFullColumnString(const FieldID& column) const
{
    checkValid();
    checkColumn(column);

    QStringList res;
    for (int row = 0; row < rowCount(); row++) {
        res << getString(row, column);
    }
    return res;
}

QVariantList DataTable::getFullColumnVariant(const FieldID& column) const
{
    checkValid();
    checkColumn(column);

    QVariantList res;
    for (int row = 0; row < rowCount(); row++) {
        res << getVariant(row, column);
    }
    return res;
}

QList<int> DataTable::findIntRows(const FieldID& column, int key, int row_from) const
{
    QList<int> res;

    while (true) {
        row_from = findInt(column, key, row_from);
        if (row_from < 0)
            break;
        res << row_from;

        if (row_from >= rowCount() - 1)
            break;

        row_from++;
    };

    return res;
}

QList<int> DataTable::findEmptyIDRows(const FieldID& column, int row_from) const
{
    QList<int> res;

    while (true) {
        row_from = findEmptyID(column, row_from);
        if (row_from < 0)
            break;
        res << row_from;

        if (row_from >= rowCount() - 1)
            break;

        row_from++;
    };

    return res;
}

QVariantList DataTable::findIntValuesVariant(const FieldID& column, int key, const FieldID& result_column) const
{
    QVariantList res;

    int row = 0;
    while (true) {
        row = findInt(column, key, row);
        if (row < 0)
            break;
        res << getVariant(row, result_column);

        if (row >= rowCount() - 1)
            break;

        row++;
    };

    return res;
}

QList<int> DataTable::findIntValuesInt(const FieldID& column, int key, const FieldID& result_column) const
{
    return Utils::toIntList(findIntValuesVariant(column, key, result_column));
}

QStringList DataTable::findIntValuesString(const FieldID& column, int key, const FieldID& result_column) const
{
    return Utils::toStringList(findIntValuesVariant(column, key, result_column));
}

} // namespace zf
