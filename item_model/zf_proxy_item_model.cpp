#include "zf_proxy_item_model.h"
#include "zf_view.h"
#include "zf_core.h"
#include "zf_item_model.h"

namespace zf
{
ProxyItemModel::ProxyItemModel(DataFilter* filter, const DataProperty& dataset_property, QObject* parent)
    : LeafFilterProxyModel(LeafFilterProxyModel::FilterMode, parent)
    , _filter(filter)
    , _dataset_property(dataset_property)
{
    Z_CHECK_NULL(filter);
    Z_CHECK(dataset_property.isValid());
    Z_CHECK(filter->structure()->contains(dataset_property));
    setObjectName(QStringLiteral("%1, %2").arg(_filter->objectName(), dataset_property.toPrintable()));
    setSourceModel(_filter->data()->dataset(dataset_property));
}

ProxyItemModel::~ProxyItemModel()
{
}

void ProxyItemModel::setSourceModel(QAbstractItemModel* m)
{
    LeafFilterProxyModel::setSourceModel(m);
}

DataProperty ProxyItemModel::datasetProperty() const
{
    return _dataset_property;
}

void ProxyItemModel::invalidateFilter()
{
    if (useCache())
        resetCache();
    LeafFilterProxyModel::invalidateFilter();
}

bool ProxyItemModel::insertRows(int row, int count, const QModelIndex& parent)
{
    bool res = LeafFilterProxyModel::insertRows(row, count, parent);

    if (res && count > 0)
        emit sg_rowsInserted(parent, row, row + count - 1);

    return res;
}

bool ProxyItemModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    bool res = LeafFilterProxyModel::insertColumns(column, count, parent);

    if (res && count > 0)
        emit sg_columnsInserted(parent, column, column + count - 1);

    return res;
}

QVariant ProxyItemModel::data(int row, int column, const QModelIndex& parent, int role) const
{
    return index(row, column, parent).data(role);
}

bool ProxyItemModel::filterAcceptsRowItself(
        int source_row, const QModelIndex& source_parent, bool& exclude_hierarchy) const
{
    if (!_filter->filterAcceptsRowHelper(_dataset_property, source_row, source_parent, exclude_hierarchy))
        return false;

    return LeafFilterProxyModel::filterAcceptsRowItself(source_row, source_parent, exclude_hierarchy);
}

RowID ProxyItemModel::getRowID(int source_row, const QModelIndex& source_parent) const
{
    return _filter->source()->datasetRowID(_dataset_property, source_row, source_parent);
}

bool ProxyItemModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    return _filter->lessThanHelper(_dataset_property, source_left, source_right);
}

} // namespace zf
