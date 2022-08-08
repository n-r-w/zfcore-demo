#include "zf_leaf_filter_proxy_item_model.h"
#include "zf_utils.h"
#include "zf_core.h"

namespace zf
{
LeafFilterProxyModel::LeafFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

LeafFilterProxyModel::LeafFilterProxyModel(Mode mode, QObject* parent)
    : QSortFilterProxyModel(parent)
    , _mode(mode)
{
}

bool LeafFilterProxyModel::useCache() const
{
    return _use_cache;
}

void LeafFilterProxyModel::setUseCache(bool b)
{
    if (_use_cache == b)
        return;

    _use_cache = b;
    resetCache();
}

void LeafFilterProxyModel::resetCache()
{
    _accepted_cache.clear();
}

QModelIndexList LeafFilterProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    if (!flags.testFlag(Qt::MatchFixedString) || !flags.testFlag(Qt::MatchRecursive) || start.parent().isValid() || start.row() > 0)
        return QSortFilterProxyModel::match(start, role, value, hits, flags);

    auto top_index = Utils::getTopSourceIndex(start);
    auto flat_model = qobject_cast<const FlatItemModel*>(top_index.model());
    if (flat_model == nullptr)
        return QSortFilterProxyModel::match(start, role, value, hits, flags);

    QModelIndexList res = flat_model->match(top_index, role, value, hits, flags);
    for (int i = res.count() - 1; i >= 0; i--) {
        auto proxy_index = Utils::alignIndexToModel(res.at(i), this);
        if (!proxy_index.isValid())
            res.removeAt(i);
        else
            res[i] = proxy_index;
    }
    return res;
}

RowID LeafFilterProxyModel::getRowID(int source_row, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_row)
    Q_UNUSED(source_parent)
    return RowID();
}

bool LeafFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (Utils::isAppHalted())
        return false;

    bool exclude_hierarchy = false;
    if (filterAcceptsRowItselfHelper(source_row, source_parent, exclude_hierarchy)) {
        if (_mode == FilterMode)
            return allAcceptedParents(source_row, source_parent); // все родители тоже должны быть видны
        return true;
    }

    if ( // в режиме фильтра не анализируем детей или родителей если сам узел недоступен
        _mode == FilterMode ||
        // если исключен из иерархии, то нет смысла проверять родителей и детей
        exclude_hierarchy)
        return false;

    // разрешить если один из родителей разрешен
    QModelIndex parent = source_parent;
    while (parent.isValid()) {
        if (filterAcceptsRowItselfHelper(parent.row(), parent.parent(), exclude_hierarchy))
            return true;

        if (exclude_hierarchy)
            return false; // если один из родителей исключен из иерархии, то исключаем и его детей

        parent = parent.parent();
    }

    // разрешить если один из детей разрешен
    return hasAcceptedChildren(source_row, source_parent);
}

bool LeafFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    QVariant left = (source_left.model() ? source_left.model()->data(source_left, sortRole()) : QVariant());
    QVariant right = (source_right.model() ? source_right.model()->data(source_right, sortRole()) : QVariant());

    return Utils::compareVariant(left, right, CompareOperator::Less, Core::locale(LocaleType::UserInterface), CompareOption::NoOption);
}

bool LeafFilterProxyModel::filterAcceptsRowItself(int source_row, const QModelIndex& source_parent, bool& exclude_hierarchy) const
{
    Q_UNUSED(source_row)
    Q_UNUSED(source_parent)
    Q_UNUSED(exclude_hierarchy)
    return true;
}

bool LeafFilterProxyModel::filterAcceptsRowItselfHelper(int source_row, const QModelIndex& source_parent, bool& exclude_hierarchy) const
{
    QPair<int, int> accepted = QPair<int, int>(-1, -1);
    RowID* key = nullptr;
    if (_use_cache) {
        RowID row_key = getRowID(source_row, source_parent);
        Z_CHECK_X(row_key.isValid(), "Invalid getRowKey function");
        key = new RowID(row_key);
        accepted = _accepted_cache.value(*key, QPair<int, int>(-1, -1));
    }

    if (accepted.first < 0) {
        accepted.first = filterAcceptsRowItself(source_row, source_parent, exclude_hierarchy);
        accepted.second = exclude_hierarchy;
        if (accepted.first)
            accepted.first = static_cast<int>(QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent));

        if (_use_cache && key)
            _accepted_cache[*key] = accepted;
    }

    if (key)
        delete key;

    exclude_hierarchy = static_cast<bool>(accepted.second);
    return static_cast<bool>(accepted.first);
}

bool LeafFilterProxyModel::hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex item = sourceModel()->index(source_row, 0, source_parent);
    if (!item.isValid())
        return false;

    //Есть ли дети
    int childCount = item.model()->rowCount(item);
    if (childCount == 0)
        return false;

    for (int i = 0; i < childCount; ++i) {
        bool exclude_hierarchy = false;
        if (filterAcceptsRowItself(i, item, exclude_hierarchy))
            return true;

        if (exclude_hierarchy)
            continue; // дочерний узел исключен из иерархии, рекурсивно дальше не обрабатываем

        // рекурсивный анализ детей
        if (hasAcceptedChildren(i, item))
            return true;
    }

    return false;
}

bool LeafFilterProxyModel::allAcceptedParents(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex item = sourceModel()->index(source_row, 0, source_parent).parent();
    bool exclude_hierarchy = false;
    while (item.isValid()) {
        if (!filterAcceptsRowItselfHelper(item.row(), item.parent(), exclude_hierarchy))
            return false;
        item = item.parent();
    }
    return true;
}

} // namespace zf
