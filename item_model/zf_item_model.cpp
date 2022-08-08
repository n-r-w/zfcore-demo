#include "zf_item_model.h"
#include "zf_core.h"
#include "zf_utils.h"

#include <QDebug>

namespace zf
{
ItemModel::ItemModel(QObject* parent)
    : FlatItemModel(parent)
{    
}

ItemModel::ItemModel(int rows, int columns, QObject* parent)
    : FlatItemModel(rows, columns, parent)
{
}

std::shared_ptr<ItemModel> ItemModel::fromVariant(const QVariant& v)
{
    return std::static_pointer_cast<ItemModel>(FlatItemModel::fromVariant(v));
}

QVariantList ItemModel::find(const DataProperty& search_column, const DataProperty& target_column, const QVariant& value, int search_role,
                             int target_role) const
{
    Z_CHECK(search_column.isValid());
    Z_CHECK(target_column.isValid());
    QModelIndexList match_res = match(index(0, search_column.pos()), search_role, value);

    QVariantList res;
    for (auto& i : qAsConst(match_res)) {
        res << index(i.row(), target_column.pos()).data(target_role);
    }

    return res;
}

QVariantList ItemModel::find(const EntityCode& entity_code, const PropertyID& search_column, const PropertyID& target_column,
                             const QVariant& value, int search_role, int target_role) const
{
    Z_CHECK(entity_code.isValid());
    return find(DataProperty::property(entity_code, search_column), DataProperty::property(entity_code, target_column), value, search_role,
                target_role);
}

QVariantList ItemModel::find(const Uid& entity_uid, const PropertyID& search_column, const PropertyID& target_column, const QVariant& value,
                             int search_role, int target_role) const
{
    Z_CHECK(entity_uid.isValid());
    return find(DataProperty::property(entity_uid, search_column), DataProperty::property(entity_uid, target_column), value, search_role,
                target_role);
}

QModelIndexList ItemModel::find(const QModelIndex& start, const DataProperty& search_column, const QVariant& value, int search_role) const
{
    Z_CHECK(search_column.isValid());
    QModelIndex start_index = start.isValid() ? index(start.row(), search_column.pos()) : index(0, search_column.pos());
    QModelIndexList match_res = match(start_index, search_role, value);

    return match_res;
}

QModelIndexList ItemModel::find(const QModelIndex& start, const EntityCode& entity_code, const PropertyID& search_column,
                                const QVariant& value, int search_role) const
{
    Z_CHECK(entity_code.isValid());
    return find(start, DataProperty::property(entity_code, search_column), value, search_role);
}

QModelIndexList ItemModel::find(const QModelIndex& start, const Uid& entity_uid, const PropertyID& search_column, const QVariant& value,
                                int search_role) const
{
    Z_CHECK(entity_uid.isValid());
    return find(start, DataProperty::property(entity_uid, search_column), value, search_role);
}

QModelIndexList ItemModel::find(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    return match(start.isValid() ? start : index(0, 0), role, value, hits, flags);
}

} // namespace zf
