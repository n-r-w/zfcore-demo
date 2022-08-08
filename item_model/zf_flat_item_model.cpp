#include "zf_flat_item_model.h"
#include "private/zf_flat_item_model_p.h"
#include "zf_utils.h"
#include "zf_core.h"

#include <QDebug>
#include <QMimeData>

namespace zf
{
static inline QString flatItemModelDataListMimeType()
{
    return QStringLiteral("application/x-zfflatitemmodeldatalist");
}

class _SortModel : public QSortFilterProxyModel
{
public:
    _SortModel(FlatItemModel* m, FlatItemModelSortFunction sort_function)
        : _m(m)
        , _sort_function(sort_function)
    {
        init();
    }
    _SortModel(FlatItemModel* m, const QList<int>& columns, const QList<Qt::SortOrder>& order, const QList<int>& role)
        : _m(m)
        , _columns(columns)
        , _order(order)
        , _role(role)
    {
        init();
    }

    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override
    {
        if (_sort_function)
            return _sort_function(_m, source_left, source_right);

        for (int i = 0; i < _columns.count(); i++) {
            QVariant left = _m->data(source_left.row(), _columns.at(i), _role.at(i), source_left.parent());
            QVariant right = _m->data(source_right.row(), _columns.at(i), _role.at(i), source_right.parent());

            if (Utils::compareVariant(left, right, CompareOperator::Equal, Core::locale(LocaleType::UserInterface), CompareOption::NoOption))
                continue;

            return Utils::compareVariant(left, right, _order.at(i) == Qt::AscendingOrder ? CompareOperator::Less : CompareOperator::More,
                Core::locale(LocaleType::UserInterface), CompareOption::NoOption);
        }
        return false;
    }

    void init()
    {
        setSourceModel(_m); //-V1053
        sort(0); //-V1053
    }

    FlatItemModel* _m = nullptr;
    QList<int> _columns;
    QList<Qt::SortOrder> _order;
    QList<int> _role;
    FlatItemModelSortFunction _sort_function;
};

void FlatItemModel::sort(const QList<int>& columns, const QList<Qt::SortOrder>& order, const QList<int>& role)
{
    Z_CHECK(!columns.isEmpty());
    Z_CHECK(order.isEmpty() || order.count() == columns.count());
    Z_CHECK(role.isEmpty() || role.count() == columns.count());

    QList<Qt::SortOrder> order_prepared;
    QList<int> role_prepared;
    for (int i = 0; i < columns.count(); i++) {
        if (order.isEmpty())
            order_prepared << Qt::AscendingOrder;
        else
            order_prepared << order.at(i);

        if (role.isEmpty())
            role_prepared << Qt::DisplayRole;
        else
            role_prepared << role.at(i);
    }

    FlatItemModel clone;
    Utils::cloneItemModel(this, &clone);

    _SortModel sort_model(&clone, columns, order_prepared, role_prepared);

    setRowCount(0);
    Utils::cloneItemModel(&sort_model, this);
}

void FlatItemModel::sort(FlatItemModelSortFunction sort_function)
{
    FlatItemModel clone;
    Utils::cloneItemModel(this, &clone);

    _SortModel sort_model(&clone, sort_function);

    setRowCount(0);
    Utils::cloneItemModel(&sort_model, this);
}

QModelIndex FlatItemModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    _FlatRows* rows = getRows(parent);
    _FlatIndexData* id = rows->getRow(row)->getData(column);

    return createIndex(row, column, id);
}

QModelIndex FlatItemModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    _FlatIndexData* id = indexData(child);
    Z_CHECK(id->row() != nullptr);
    _FlatRowData* parentInfo = id->row()->parent();

    if (parentInfo) {
        int row = parentInfo->owner()->findRow(parentInfo);
        if (row < 0)
            return QModelIndex();

        _FlatRowData* p_data = parentInfo->owner()->getRow(row);
        _FlatIndexData* i_data = p_data->getData(0);

        return createIndex(row, 0, i_data);
    } else
        return QModelIndex();
}

QLocale::Language FlatItemModel::language() const
{
    return _language;
}

QLocale::Language FlatItemModel::setLanguage(QLocale::Language language)
{
    if (_language == language)
        return _language;

    QLocale::Language old_lang = _language;

    beginResetModel();
    _language = language;
    endResetModel();
    return old_lang;
}

int FlatItemModel::rowCount(const QModelIndex& parent) const
{
    return getRows(parent)->rowCount();
}

int FlatItemModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return _column_count;
}

bool FlatItemModel::hasChildren(const QModelIndex& parent) const
{
    return getRows(parent)->rowCount() > 0;
}

QVariant FlatItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    return Utils::valueToLanguage(indexData(index)->values(role), _language_force ? *_language_force : _language);
}

bool FlatItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return false;

    clearMatchCache();

    if (value.isNull() || !value.isValid() || (value.type() == QVariant::String && value.toString().isEmpty()))
        id->remove(role);
    else
        id->set(value, role, _language_force ? *_language_force : _language);

    emit dataChanged(index, index, {role});

    if (role == Qt::EditRole && value.type() == QVariant::Bool)
        setData(index, value.toBool() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);

    return true;
}

QVariant FlatItemModel::data(int row, int column, int role, const QModelIndex& parent) const
{
    return data(index(row, column, parent), role);
}

QVariant FlatItemModel::data(int row, int column, const QModelIndex& parent) const
{
    return data(row, column, Qt::DisplayRole, parent);
}

bool FlatItemModel::setData(int row, int column, const QVariant& value, int role, const QModelIndex& parent)
{
    return setData(index(row, column, parent), value, role);
}

bool FlatItemModel::setData(int row, int column, const QVariant& value, const QModelIndex& parent)
{
    return setData(row, column, value, Qt::DisplayRole, parent);
}

QString FlatItemModel::text(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::DisplayRole));
}

void FlatItemModel::setText(const QModelIndex& index, const QString& text)
{
    setData(index, text, Qt::DisplayRole);
}

QIcon FlatItemModel::icon(const QModelIndex& index) const
{
    return qvariant_cast<QIcon>(data(index, Qt::DecorationRole));
}

void FlatItemModel::setIcon(const QModelIndex& index, const QIcon& icon)
{
    setData(index, icon, Qt::DecorationRole);
}

QString FlatItemModel::toolTip(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::ToolTipRole));
}

void FlatItemModel::setToolTip(const QModelIndex& index, const QString& toolTip)
{
    setData(index, toolTip, Qt::ToolTipRole);
}

QString FlatItemModel::statusTip(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::StatusTipRole));
}

void FlatItemModel::setStatusTip(const QModelIndex& index, const QString& statusTip)
{
    setData(index, statusTip, Qt::StatusTipRole);
}

QString FlatItemModel::whatsThis(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::WhatsThisRole));
}

void FlatItemModel::setWhatsThis(const QModelIndex& index, const QString& whatsThis)
{
    setData(index, whatsThis, Qt::WhatsThisRole);
}

QSize FlatItemModel::sizeHint(const QModelIndex& index) const
{
    return qvariant_cast<QSize>(data(index, Qt::SizeHintRole));
}

void FlatItemModel::setSizeHint(const QModelIndex& index, const QSize& sizeHint)
{
    setData(index, sizeHint, Qt::SizeHintRole);
}

QFont FlatItemModel::font(const QModelIndex& index) const
{
    return qvariant_cast<QFont>(data(index, Qt::FontRole));
}

void FlatItemModel::setFont(const QModelIndex& index, const QFont& font)
{
    setData(index, font, Qt::FontRole);
}

Qt::Alignment FlatItemModel::textAlignment(const QModelIndex& index) const
{
    return Qt::Alignment(qvariant_cast<int>(data(index, Qt::TextAlignmentRole)));
}

void FlatItemModel::setTextAlignment(const QModelIndex& index, Qt::Alignment textAlignment)
{
    setData(index, int(textAlignment), Qt::TextAlignmentRole);
}

QBrush FlatItemModel::background(const QModelIndex& index) const
{
    return qvariant_cast<QBrush>(data(index, Qt::BackgroundRole));
}

void FlatItemModel::setBackground(const QModelIndex& index, const QBrush& brush)
{
    setData(index, brush, Qt::BackgroundRole);
}

QBrush FlatItemModel::foreground(const QModelIndex& index) const
{
    return qvariant_cast<QBrush>(data(index, Qt::ForegroundRole));
}

void FlatItemModel::setForeground(const QModelIndex& index, const QBrush& brush)
{
    setData(index, brush, Qt::ForegroundRole);
}

Qt::CheckState FlatItemModel::checkState(const QModelIndex& index) const
{
    return Qt::CheckState(qvariant_cast<int>(data(index, Qt::CheckStateRole)));
}

void FlatItemModel::setCheckState(const QModelIndex& index, Qt::CheckState checkState)
{
    setData(index, checkState, Qt::CheckStateRole);
}

QString FlatItemModel::accessibleText(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::AccessibleTextRole));
}

void FlatItemModel::setAccessibleText(const QModelIndex& index, const QString& accessibleText)
{
    setData(index, accessibleText, Qt::AccessibleTextRole);
}

QString FlatItemModel::accessibleDescription(const QModelIndex& index) const
{
    return qvariant_cast<QString>(data(index, Qt::AccessibleDescriptionRole));
}

void FlatItemModel::setAccessibleDescription(const QModelIndex& index, const QString& accessibleDescription)
{
    setData(index, accessibleDescription, Qt::AccessibleDescriptionRole);
}

bool FlatItemModel::isEnabled(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsEnabled) != 0;
}

void FlatItemModel::setEnabled(const QModelIndex& index, bool enabled)
{
    changeFlags(index, enabled, Qt::ItemIsEnabled);
}

bool FlatItemModel::isEditable(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsEditable) != 0;
}

void FlatItemModel::setEditable(const QModelIndex& index, bool editable)
{
    changeFlags(index, editable, Qt::ItemIsEditable);
}

bool FlatItemModel::isSelectable(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsSelectable) != 0;
}

void FlatItemModel::setSelectable(const QModelIndex& index, bool selectable)
{
    changeFlags(index, selectable, Qt::ItemIsSelectable);
}

bool FlatItemModel::isCheckable(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsUserCheckable) != 0;
}

void FlatItemModel::setCheckable(const QModelIndex& index, bool checkable)
{
    if (checkable && !isCheckable(index)) {
        // make sure there's data for the checkstate role (загадочная логика из QStandardItemModel)
        if (!data(index, Qt::CheckStateRole).isValid())
            setData(index, Qt::Unchecked, Qt::CheckStateRole);
    }
    changeFlags(index, checkable, Qt::ItemIsUserCheckable);
}

bool FlatItemModel::isAutoTristate(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsAutoTristate) != 0;
}

void FlatItemModel::setAutoTristate(const QModelIndex& index, bool tristate)
{
    changeFlags(index, tristate, Qt::ItemIsAutoTristate);
}

bool FlatItemModel::isUserTristate(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsUserTristate) != 0;
}

void FlatItemModel::setUserTristate(const QModelIndex& index, bool tristate)
{
    changeFlags(index, tristate, Qt::ItemIsUserTristate);
}

bool FlatItemModel::isDragEnabled(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsDragEnabled) != 0;
}

void FlatItemModel::setDragEnabled(const QModelIndex& index, bool dragEnabled)
{
    changeFlags(index, dragEnabled, Qt::ItemIsDragEnabled);
}

bool FlatItemModel::isDropEnabled(const QModelIndex& index) const
{
    return (flags(index) & Qt::ItemIsDropEnabled) != 0;
}

void FlatItemModel::setDropEnabled(const QModelIndex& index, bool dropEnabled)
{
    changeFlags(index, dropEnabled, Qt::ItemIsDropEnabled);
}

Qt::DropActions FlatItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList FlatItemModel::mimeTypes() const
{
    return QAbstractItemModel::mimeTypes() << flatItemModelDataListMimeType();
}

QMimeData* FlatItemModel::mimeData(const QModelIndexList& indexes) const
{
    Q_UNUSED(indexes)

    // TODO не реализовано
    return nullptr;
}

bool FlatItemModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    Q_UNUSED(data)
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)

    // TODO не реализовано
    return false;
}

bool FlatItemModel::clearData(const QModelIndex& index)
{
    if (!index.isValid())
        return false;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return false;

    QVector<int>* roles = id->clear(true);
    if (!roles->isEmpty()) {
        clearMatchCache();
        emit dataChanged(index, index, *roles);
        Z_DELETE(roles);
    }

    return true;
}

QVariant FlatItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0)
        return QVariant();

    if (orientation == Qt::Vertical) {
        if (section >= rowCount())
            return QVariant();
    } else {
        if (section >= columnCount())
            return QVariant();
    }

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    QMap<int, QVariant>* v = nullptr;

    if (orientation == Qt::Vertical) {
        if (_v_headers.size() > section)
            v = _v_headers.at(section);
    } else {
        if (_h_headers.size() > section)
            v = _h_headers.at(section);
    }

    if (v == nullptr)
        return role == Qt::EditRole ? section + 1 : QVariant();
    else
        return v->value(role);
}

bool FlatItemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (section < 0)
        return false;

    if (orientation == Qt::Vertical) {
        if (section >= rowCount())
            return false;
    } else {
        if (section >= columnCount())
            return false;
    }

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    QMap<int, QVariant>* v = (orientation == Qt::Vertical) ? _v_headers.at(section) : _h_headers.at(section);

    if (!v) {
        typedef QMap<int, QVariant> VarMapInt;
        v = Z_NEW(VarMapInt);
        if (orientation == Qt::Vertical)
            _v_headers[section] = v;
        else
            _h_headers[section] = v;
    }

    v->insert(role, value);
    emit headerDataChanged(orientation, section, section);
    return true;
}

bool FlatItemModel::insertRows(int row, int count, const QModelIndex& parent)
{
    return getRows(parent)->insertRows(row, count);
}

bool FlatItemModel::insertColumns(int column, int count, const QModelIndex& parent)
{
    if (getRows(parent)->insertColumns(column, count)) {
        _column_count += count;
        return true;

    } else
        return false;
}

bool FlatItemModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (getRows(parent)->removeRows(row, count, false)) {
        // очищаем заголовки
        for (int i = qMin(_v_headers.count() - 1, row + count - 1); i >= row; --i) {
            auto val = _v_headers.at(i);
            if (val != nullptr)
                Z_DELETE(val);

            _v_headers.remove(i);
        }

        // скорее всего это не нужно, т.к. падает прокси модель
        // emit headerDataChanged(Qt::Vertical, row, row + count - 1);

        return true;
    } else
        return false;
}

bool FlatItemModel::removeColumns(int column, int count, const QModelIndex& parent)
{
    if (getRows(parent)->removeColumns(column, count)) {
        // очищаем заголовки
        for (int i = qMin(_h_headers.count() - 1, column + count - 1); i >= column; --i) {
            auto val = _h_headers.at(i);
            if (val != nullptr)
                Z_DELETE(val);

            _h_headers.remove(i);
        }
        _column_count -= count;

        // скорее всего это не нужно, т.к. падает прокси модель
        // emit headerDataChanged(Qt::Horizontal, column, column + count - 1);

        return true;

    } else
        return false;
}

bool FlatItemModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
    if (sourceRow < 0 || count < 0 || destinationChild < 0)
        return false;

    if (!allowMove(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild, Qt::Vertical))
        return false;

    if (count == 0 || (sourceParent == destinationParent && sourceRow == destinationChild))
        return true;

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild))
        return false;

    bool res = false;
    if (sourceParent == destinationParent) {
        if (destinationChild < sourceRow)
            res = getRows(sourceParent)->moveRows(sourceRow, count, destinationChild);
        else
            res = getRows(sourceParent)->moveRows(sourceRow, count, destinationChild - count);

        // перемещаем заголовки
        if (res && !sourceParent.isValid()) {
            if (destinationChild < sourceRow) {
                moveHeaders(Qt::Vertical, sourceRow, count, destinationChild);
                emit headerDataChanged(Qt::Vertical, destinationChild, sourceRow + count - 1);

            } else {
                moveHeaders(Qt::Vertical, sourceRow, count, destinationChild - count);
                emit headerDataChanged(Qt::Vertical, sourceRow, destinationChild - 1);
            }
        }

    } else {
        QList<_FlatRowData*> moved_rows = getRows(sourceParent)->takeRows(sourceRow, count);
        if (moved_rows.isEmpty())
            res = false;
        else
            res = getRows(destinationParent)->putRows(moved_rows, destinationChild);
    }

    clearMatchCache();
    endMoveRows();

    return res;
}

bool FlatItemModel::moveColumns(const QModelIndex& sourceParent, int sourceColumn, int count, const QModelIndex& destinationParent, int destinationChild)
{
    if (sourceParent.isValid() || destinationParent.isValid())
        return false; // колонки перемещаются только на верхнем уровне

    if (!allowMove(sourceParent, sourceColumn, sourceColumn + count - 1, destinationParent, destinationChild, Qt::Horizontal))
        return false;

    if (count == 0)
        return true;

    if (!beginMoveColumns(sourceParent, sourceColumn, sourceColumn + count - 1, destinationParent, destinationChild))
        return false;

    bool res;
    if (destinationChild < sourceColumn)
        res = _rows->moveColumns(sourceColumn, count, destinationChild);
    else
        res = _rows->moveColumns(sourceColumn, count, destinationChild - count);

    clearMatchCache();
    endMoveColumns();

    if (res) {
        // перемещаем заголовки
        if (destinationChild < sourceColumn) {
            moveHeaders(Qt::Horizontal, sourceColumn, count, destinationChild);
            emit headerDataChanged(Qt::Horizontal, destinationChild, sourceColumn + count - 1);

        } else {
            moveHeaders(Qt::Horizontal, sourceColumn, count, destinationChild - count);
            emit headerDataChanged(Qt::Horizontal, sourceColumn, destinationChild - 1);
        }
    }

    return res;
}

QMap<int, QVariant> FlatItemModel::itemData(const QModelIndex& index) const
{
    if (!index.isValid())
        return QMap<int, QVariant>();

    return indexData(index)->toMap(_language_force ? *_language_force : _language);
}

bool FlatItemModel::setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles)
{
    if (!index.isValid())
        return false;

    clearMatchCache();
    indexData(index)->fromMap(roles, _language_force ? *_language_force : _language);
    return true;
}

Qt::ItemFlags FlatItemModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return _flags;

    _FlatIndexData* d = indexData(index);
    return d->hasFlags() ? d->flags() : _flags;
}

void FlatItemModel::setFlags(const QModelIndex& index, Qt::ItemFlags flags)
{
    if (!index.isValid()) {
        clearMatchCache();
        setDefaultFlags(flags);
        return;
    }

    _FlatIndexData* d = indexData(index);

    if (d->hasFlags() && d->flags() == flags)
        return;

    d->setFlags(flags);

    clearMatchCache();

    emit dataChanged(index, index, {Qt::UserRole - 1});
}

void FlatItemModel::setDefaultFlags(Qt::ItemFlags flags)
{
    _flags = flags;
}

Qt::ItemFlags FlatItemModel::defaultFlags() const
{
    return _flags;
}

void FlatItemModel::setRowCount(int n, const QModelIndex& parent)
{
    if (!parent.isValid()) {
        for (int i = n; i < _v_headers.size(); i++) {
            if (_v_headers.at(i) != nullptr)
                Z_DELETE(_v_headers.at(i));
        }

        _v_headers.resize(n);
    }

    _FlatRows* rows = getRows(parent);
    rows->setRowCount(n);
}

void FlatItemModel::setColumnCount(int n, const QModelIndex& parent)
{
    Q_UNUSED(parent)
    for (int i = n; i < _h_headers.size(); i++) {
        if (_h_headers.at(i) != nullptr)
            Z_DELETE(_h_headers.at(i));
    }

    _h_headers.resize(n);

    _column_count = n;
    _rows->setColumnCount(n);
}

void FlatItemModel::appendRows(int count, const QModelIndex& parent)
{
    insertRows(rowCount(parent), count, parent);
}

void FlatItemModel::appendColumns(int count, const QModelIndex& parent)
{
    Q_UNUSED(parent)
    insertColumns(columnCount(parent), count);
}

int FlatItemModel::appendRow(const QModelIndex& parent)
{
    appendRows(1, parent);
    return rowCount(parent) - 1;
}

int FlatItemModel::appendColumn(const QModelIndex& parent)
{
    appendColumns(1);
    return columnCount(parent) - 1;
}

bool FlatItemModel::removeRow(const QModelIndex& index)
{
    return removeRow(index.row(), index.parent());
}

bool FlatItemModel::removeRow(int row, const QModelIndex& parent)
{
    return removeRows(row, 1, parent);
}

bool FlatItemModel::removeColumn(int column, const QModelIndex& parent)
{
    Q_UNUSED(parent)
    return removeColumns(column, 1);
}

bool FlatItemModel::moveRows(int sourceRow, int count, int destinationRow)
{
    return moveRows(QModelIndex(), sourceRow, count, QModelIndex(), destinationRow);
}

bool FlatItemModel::moveColumns(int sourceColumn, int count, int destinationColumn)
{
    return moveColumns(QModelIndex(), sourceColumn, count, QModelIndex(), destinationColumn);
}

bool FlatItemModel::getRows(int row, int count, QList<FlatRow*>& rows, const QModelIndex& parent)
{
    if (row < 0 || count < 0 || row + count > rowCount(parent))
        return false;

    _FlatRows* r = getRows(parent);

    rows.clear();
    rows.reserve(r->rowCount());

    for (int i = row; i < row + count; i++) {
        rows << Z_NEW(FlatRow, r->getRow(i), nullptr);
    }

    return true;
}

bool FlatItemModel::takeRows(int row, int count, QList<FlatRow*>& rows, const QModelIndex& parent)
{
    bool res = getRows(row, count, rows, parent);
    if (!res)
        return false;

    res = removeRows(row, count, parent);
    if (!res) {
        Utils::zDeleteAllCustom(rows);
        rows.clear();
    }

    return res;
}

bool FlatItemModel::insertRows(int row, const QList<FlatRow*>& rows, const QModelIndex& parent)
{
    if (!insertRows(row, rows.count(), parent))
        return false;

    int col_count = columnCount();
    for (int i = 0; i < rows.count(); i++) {
        FlatRow* r_data = rows.at(i);
        Z_CHECK(r_data->columnCount() == col_count);
        // с конца, чтобы сразу была выделена память
        for (int j = col_count - 1; j >= 0; j--) {
            setItemData(index(row + i, j, parent), r_data->itemData(j, _language_force ? *_language_force : _language));
        }
    }

    return true;
}

bool FlatItemModel::appendRows(const QList<FlatRow*>& rows, const QModelIndex& parent)
{
    return insertRows(rowCount(parent), rows, parent);
}

bool FlatItemModel::isChanged(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return false;

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return false;

    return id->isChanged(role);
}

void FlatItemModel::resetChanged(const QModelIndex& index, int role)
{
    if (!index.isValid())
        return;

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return;

    id->resetChanged(role);
}

void FlatItemModel::resetChanged()
{
    _rows->resetChanged();
}

void FlatItemModel::alloc()
{
    _rows->alloc();
}

void FlatItemModel::moveData(FlatItemModel* source)
{
    Z_CHECK_NULL(source);

    beginResetModel();

    _column_count = source->_column_count;
    _rows = source->_rows;
    _rows->setItemModel(this);
    _h_headers = source->_h_headers;
    _v_headers = source->_v_headers;
    _flags = source->_flags;
    _language = source->_language;
    _language_force = source->_language_force;

    source->_clear_on_destructor = false;
    delete source;

    clearMatchCache();

    endResetModel();
}

QModelIndexList FlatItemModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
    if (!flags.testFlag(Qt::MatchFixedString) || !flags.testFlag(Qt::MatchRecursive) || start.parent().isValid() || start.row() > 0)
        return QAbstractItemModel::match(start, role, value, hits, flags);

    QString key = QString::number(start.column()) + Consts::KEY_SEPARATOR + QString::number(role) + Consts::KEY_SEPARATOR
                  + QString::number(flags & Qt::MatchCaseSensitive);
    auto it = _match_cache.find(key);
    std::shared_ptr<QMultiHash<QString, QModelIndex>> cache;
    if (it == _match_cache.end()) {
        cache = std::make_shared<QMultiHash<QString, QModelIndex>>();
        _match_cache[key] = cache;
        fillMatchCache(cache, start.column(), role, QModelIndex());

    } else {
        cache = it.value();
    }

    QModelIndexList res = cache->values(value.toString());
    while (res.count() > hits) {
        res.removeLast();
    }

    return res;
}

QVariant FlatItemModel::dataHelper(const QModelIndex& index, int role, QLocale::Language language) const
{
    auto old_lang = _language_force;
    _language_force = Z_NEW(QLocale::Language, language);

    QVariant value = data(index, role);

    Z_DELETE(_language_force);
    if (old_lang)
        _language_force = old_lang;
    else
        _language_force = nullptr;

    return value;
}

LanguageMap FlatItemModel::dataHelperLanguageMap(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return {};

    return id->values(role);
}

QMap<int, LanguageMap> FlatItemModel::dataHelperLanguageRoleMap(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    _FlatIndexData* id = indexData(index);
    if (!id)
        return {};

    return id->toMap();
}

bool FlatItemModel::setDataHelper(const QModelIndex& index, const QVariant& value, int role, QLocale::Language language)
{
    auto old_lang = _language_force;
    _language_force = Z_NEW(QLocale::Language, language);

    bool res = setData(index, value, role);

    Z_DELETE(_language_force);
    if (old_lang)
        _language_force = old_lang;
    else
        _language_force = nullptr;

    return res;
}

bool FlatItemModel::setDataHelperLanguageMap(const QModelIndex& index, const LanguageMap& value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return false;

    LanguageMap* old_value = nullptr;
    if (_options.testFlag(FlatItemModelOption::GenerateDataChangedExtendedSignal))
        old_value = new LanguageMap(id->values(role));

    if (value.isEmpty())
        id->remove(role);
    else
        id->set(value, role);

    clearMatchCache();
    emit dataChanged(index, index, {role});

    if (role == Qt::EditRole) {
        bool changed = false;
        for (auto i = value.constBegin(); i != value.constEnd(); ++i) {
            if (i.value().type() == QVariant::Bool) {
                id->set(i.value().toBool() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole, i.key());
                changed = true;
            }
        }
        if (changed) {
            clearMatchCache();
            emit dataChanged(index, index, {Qt::CheckStateRole});
        }
    }

    if (old_value) {
        emit sg_dataChangedExtended(index, value, *old_value, role);
        delete old_value;
    }

    return true;
}

bool FlatItemModel::setDataHelperLanguageRoleMap(const QModelIndex& index, const QMap<int, LanguageMap>& value)
{
    if (!index.isValid())
        return false;

    _FlatIndexData* id = indexData(index);
    if (!id)
        return false;

    QVector roles_chaged = Utils::toSet(id->roles()).unite(Utils::toSet(value.keys())).values().toVector();

    if (value.isEmpty())
        id->clear(false);
    else
        id->fromMap(value);

    clearMatchCache();
    emit dataChanged(index, index, roles_chaged);
    return true;
}

QVariant FlatItemModel::toVariant() const
{
    return QVariant::fromValue(_FlatItemModel_data(const_cast<FlatItemModel*>(this)));
}

std::shared_ptr<FlatItemModel> FlatItemModel::fromVariant(const QVariant& v)
{
    if (!isVariant(v))
        return nullptr;

    return v.value<_FlatItemModel_data>()._d->_model;
}

bool FlatItemModel::isVariant(const QVariant& v)
{
    return (v.isValid() && v.userType() == _FlatItemModel_data::_meta_type_number);
}

int FlatItemModel::metaType()
{
    return _FlatItemModel_data::_meta_type_number;
}

void FlatItemModel::setInternalName(const QString& name)
{
    _rows->setInternalName(name);
}

QString FlatItemModel::internalName() const
{
    return _rows->internalName();
}

void FlatItemModel::beginResetModel()
{
    if (++_reset_counter == 1)
        QAbstractItemModel::beginResetModel();
}

void FlatItemModel::endResetModel()
{
    Z_CHECK(--_reset_counter >= 0);
    if (_reset_counter == 0)
        QAbstractItemModel::endResetModel();
}

void FlatItemModel::changeFlags(const QModelIndex& index, bool enable, Qt::ItemFlags f)
{
    Qt::ItemFlags flags = this->flags(index);
    if (enable)
        flags |= f;
    else
        flags &= ~f;
    setFlags(index, flags);
}

QModelIndex FlatItemModel::indexFromData(int row, int column, _FlatIndexData* data) const
{
    return data == nullptr ? QModelIndex() : createIndex(row, column, data);
}

_FlatRows* FlatItemModel::getRows(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        _FlatIndexData* d = indexData(parent);
        Z_CHECK(d->row() != nullptr);
        return d->row()->children();
    } else
        return _rows;
}

_FlatIndexData* FlatItemModel::indexData(const QModelIndex& index)
{
    return static_cast<_FlatIndexData*>(index.internalPointer());
}

void FlatItemModel::moveHeaders(Qt::Orientation orientation, int source, int count, int destination)
{
    int max_count = (orientation == Qt::Vertical ? rowCount() : columnCount());

    Z_CHECK(destination >= 0 && destination + count <= max_count);
    Z_CHECK(source >= 0 && source + count <= max_count);
    Z_CHECK(destination != source);

    QVector<QMap<int, QVariant>*>* headers = (orientation == Qt::Vertical ? &_v_headers : &_h_headers);

    if (source + count <= headers->size() && destination + count <= headers->size())
        Utils::moveVector<QMap<int, QVariant>*>(*headers, source, count, destination);
}

bool FlatItemModel::allowMove(
    const QModelIndex& srcParent, int start, int end, const QModelIndex& destinationParent, int destinationStart, Qt::Orientation orientation)
{
    if (start < 0 || destinationStart < 0 || end < start)
        return false;

    if (destinationParent == srcParent) {
        if (destinationStart >= start && destinationStart <= end + 1)
            return false;

        if (destinationStart + end - start > (orientation == Qt::Vertical ? rowCount(destinationParent) : columnCount(destinationParent)))
            return false;

        if (end > (orientation == Qt::Vertical ? rowCount(srcParent) : columnCount(srcParent)))
            return false;
    }

    return true;
}

void FlatItemModel::fillMatchCache(const std::shared_ptr<QMultiHash<QString, QModelIndex>>& cache, int column, int role, const QModelIndex& parent) const
{
    int row_count = rowCount(parent);
    for (int row = 0; row < row_count; row++) {
        QModelIndex root = index(row, 0, parent);
        cache->insert(index(row, column, parent).data(role).toString(), root);
        fillMatchCache(cache, column, role, root);
    }
}

void FlatItemModel::clearMatchCache()
{
    _match_cache.clear();
}

FlatItemModel::FlatItemModel(QObject* parent)
    : QAbstractItemModel(parent)
    , _rows(Z_NEW(_FlatRows, this, nullptr))
    , _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled)
{
    _rows->setColumnCount(_column_count);
}

FlatItemModel::FlatItemModel(int rows, int columns, QObject* parent)
    : FlatItemModel(parent)
{
    setColumnCount(columns);
    setRowCount(rows);
}

FlatItemModel::~FlatItemModel()
{
    if (_clear_on_destructor) {
        Z_DELETE(_rows);
        Utils::zDeleteAllCustom(_h_headers);
        Utils::zDeleteAllCustom(_v_headers);
    }
}

FlatItemModelOptions FlatItemModel::options() const
{
    return _options;
}

void FlatItemModel::setOptions(const FlatItemModelOptions& options)
{
    _options = options;
}

FlatRow::FlatRow(int column_count)
    : _data(Z_NEW(_FlatRowData, column_count, nullptr))
{
}

FlatRow::FlatRow(const FlatRow& r)
{
    copyFromEx(r);
}

FlatRow::~FlatRow()
{
    Utils::zDeleteAllCustom(_children);
}

FlatRow& FlatRow::operator=(const FlatRow& r)
{
    copyFromEx(r);
    return *this;
}

FlatRow* FlatRow::parent() const
{
    return _parent;
}

int FlatRow::pos() const
{
    return _parent == nullptr ? -1 : _parent->indexOf(const_cast<FlatRow*>(this));
}

QVariant FlatRow::data(int column, int role, QLocale::Language language) const
{
    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    return Utils::valueToLanguage(_data->getData(column)->values(role), language);
}

bool FlatRow::setData(int column, const QVariant& value, int role, QLocale::Language language)
{
    if (column < 0 || column >= columnCount())
        return false;

    if (role == Qt::DisplayRole)
        role = Qt::EditRole;

    _data->getData(column)->set(value, role, language);

    return true;
}

QString FlatRow::text(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::DisplayRole, language));
}

void FlatRow::setText(int column, const QString& text, QLocale::Language language)
{
    setData(column, text, Qt::DisplayRole, language);
}

QIcon FlatRow::icon(int column, QLocale::Language language) const
{
    return qvariant_cast<QIcon>(data(column, Qt::DecorationRole, language));
}

void FlatRow::setIcon(int column, const QIcon& icon, QLocale::Language language)
{
    setData(column, icon, Qt::DecorationRole, language);
}

QString FlatRow::toolTip(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::ToolTipRole, language));
}

void FlatRow::setToolTip(int column, const QString& toolTip, QLocale::Language language)
{
    setData(column, toolTip, Qt::ToolTipRole, language);
}

QString FlatRow::statusTip(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::StatusTipRole, language));
}

void FlatRow::setStatusTip(int column, const QString& statusTip, QLocale::Language language)
{
    setData(column, statusTip, Qt::StatusTipRole, language);
}

QString FlatRow::whatsThis(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::WhatsThisRole, language));
}

void FlatRow::setWhatsThis(int column, const QString& whatsThis, QLocale::Language language)
{
    setData(column, whatsThis, Qt::WhatsThisRole, language);
}

QSize FlatRow::sizeHint(int column) const
{
    return qvariant_cast<QSize>(data(column, Qt::SizeHintRole, QLocale::AnyLanguage));
}

void FlatRow::setSizeHint(int column, const QSize& sizeHint)
{
    setData(column, sizeHint, Qt::SizeHintRole, QLocale::AnyLanguage);
}

QFont FlatRow::font(int column) const
{
    return qvariant_cast<QFont>(data(column, Qt::FontRole, QLocale::AnyLanguage));
}

void FlatRow::setFont(int column, const QFont& font)
{
    setData(column, font, Qt::FontRole, QLocale::AnyLanguage);
}

Qt::Alignment FlatRow::textAlignment(int column) const
{
    return Qt::Alignment(qvariant_cast<int>(data(column, Qt::TextAlignmentRole, QLocale::AnyLanguage)));
}

void FlatRow::setTextAlignment(int column, Qt::Alignment textAlignment)
{
    setData(column, int(textAlignment), Qt::TextAlignmentRole, QLocale::AnyLanguage);
}

QBrush FlatRow::background(int column) const
{
    return qvariant_cast<QBrush>(data(column, Qt::BackgroundRole, QLocale::AnyLanguage));
}

void FlatRow::setBackground(int column, const QBrush& brush)
{
    setData(column, brush, Qt::BackgroundRole, QLocale::AnyLanguage);
}

QBrush FlatRow::foreground(int column) const
{
    return qvariant_cast<QBrush>(data(column, Qt::ForegroundRole, QLocale::AnyLanguage));
}

void FlatRow::setForeground(int column, const QBrush& brush)
{
    setData(column, brush, Qt::ForegroundRole, QLocale::AnyLanguage);
}

Qt::CheckState FlatRow::checkState(int column) const
{
    return Qt::CheckState(qvariant_cast<int>(data(column, Qt::CheckStateRole, QLocale::AnyLanguage)));
}

void FlatRow::setCheckState(int column, Qt::CheckState checkState)
{
    setData(column, checkState, Qt::CheckStateRole, QLocale::AnyLanguage);
}

QString FlatRow::accessibleText(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::AccessibleTextRole, language));
}

void FlatRow::setAccessibleText(int column, const QString& accessibleText, QLocale::Language language)
{
    setData(column, accessibleText, Qt::AccessibleTextRole, language);
}

QString FlatRow::accessibleDescription(int column, QLocale::Language language) const
{
    return qvariant_cast<QString>(data(column, Qt::AccessibleDescriptionRole, language));
}

void FlatRow::setAccessibleDescription(int column, const QString& accessibleDescription, QLocale::Language language)
{
    setData(column, accessibleDescription, Qt::AccessibleDescriptionRole, language);
}

bool FlatRow::isEnabled(int column) const
{
    return (flags(column) & Qt::ItemIsEnabled) != 0;
}

void FlatRow::setEnabled(int column, bool enabled)
{
    changeFlags(column, enabled, Qt::ItemIsEnabled);
}

bool FlatRow::isEditable(int column) const
{
    return (flags(column) & Qt::ItemIsEditable) != 0;
}

void FlatRow::setEditable(int column, bool editable)
{
    changeFlags(column, editable, Qt::ItemIsEditable);
}

bool FlatRow::isSelectable(int column) const
{
    return (flags(column) & Qt::ItemIsSelectable) != 0;
}

void FlatRow::setSelectable(int column, bool selectable)
{
    changeFlags(column, selectable, Qt::ItemIsSelectable);
}

bool FlatRow::isCheckable(int column) const
{
    return (flags(column) & Qt::ItemIsUserCheckable) != 0;
}

void FlatRow::setCheckable(int column, bool checkable)
{
    if (checkable && !isCheckable(column)) {
        // make sure there's data for the checkstate role (загадочная логика из QStandardItemModel)
        if (!data(column, Qt::CheckStateRole, QLocale::AnyLanguage).isValid())
            setData(column, Qt::Unchecked, Qt::CheckStateRole, QLocale::AnyLanguage);
    }
    changeFlags(column, checkable, Qt::ItemIsUserCheckable);
}

bool FlatRow::isAutoTristate(int column) const
{
    return (flags(column) & Qt::ItemIsAutoTristate) != 0;
}

void FlatRow::setAutoTristate(int column, bool tristate)
{
    changeFlags(column, tristate, Qt::ItemIsAutoTristate);
}

bool FlatRow::isUserTristate(int column) const
{
    return (flags(column) & Qt::ItemIsUserTristate) != 0;
}

void FlatRow::setUserTristate(int column, bool tristate)
{
    changeFlags(column, tristate, Qt::ItemIsUserTristate);
}

bool FlatRow::isDragEnabled(int column) const
{
    return (flags(column) & Qt::ItemIsDragEnabled) != 0;
}

void FlatRow::setDragEnabled(int column, bool dragEnabled)
{
    changeFlags(column, dragEnabled, Qt::ItemIsDragEnabled);
}

bool FlatRow::isDropEnabled(int column) const
{
    return (flags(column) & Qt::ItemIsDropEnabled) != 0;
}

void FlatRow::setDropEnabled(int column, bool dropEnabled)
{
    changeFlags(column, dropEnabled, Qt::ItemIsDropEnabled);
}

bool FlatRow::clearData(int column)
{
    if (column < 0 || column >= columnCount())
        return false;

    _data->getData(column)->clear(false);

    return true;
}

int FlatRow::rowCount() const
{
    return _children.count();
}

int FlatRow::columnCount() const
{
    return _data->columnCount();
}

bool FlatRow::hasChildren() const
{
    return rowCount() > 0;
}

const QList<FlatRow*>& FlatRow::children() const
{
    return _children;
}

FlatRow* FlatRow::child(int row) const
{
    Z_CHECK(row >= 0 && row < _children.count());
    return _children.at(row);
}

int FlatRow::indexOf(FlatRow* row) const
{
    Z_CHECK(row != nullptr);
    return _children.indexOf(row);
}

void FlatRow::setRowCount(int n)
{
    Z_CHECK(n >= 0);

    if (_children.count() > n) {
        for (int i = n; i < _children.count(); i++) {
            Z_DELETE(_children.at(i));
        }
        _children.erase(_children.begin() + n, _children.end());

    } else {
        _children.reserve(n);
        for (int i = _children.count(); i < n; i++) {
            _children << Z_NEW(FlatRow, columnCount(), this);
        }
    }
}

bool FlatRow::insertRows(int row, int count)
{
    if (row < 0 || count < 0 || row > rowCount())
        return false;

    if (count == 0)
        return true;

    if (row == rowCount()) {
        setRowCount(row + count);
        return true;
    }

    _children.reserve(rowCount() + count);
    for (int i = 0; i < count; i++) {
        _children.insert(row + i, Z_NEW(FlatRow, columnCount(), this));
    }

    return true;
}

bool FlatRow::appendRows(int count)
{
    return insertRows(rowCount(), count);
}

bool FlatRow::appendRow()
{
    return appendRows(1);
}

bool FlatRow::removeRows(int row, int count)
{
    if (row < 0 || count < 0 || row + count > rowCount())
        return false;

    if (count == 0)
        return true;

    for (int i = row; i < row + count; i++) {
        Z_DELETE(_children.at(i));
    }
    _children.erase(_children.begin() + row, _children.begin() + row + count);

    return true;
}

bool FlatRow::removeRow(int row)
{
    return removeRows(row, 1);
}

bool FlatRow::moveRows(int source, int count, int destination)
{
    if (source < 0 || count < 0 || source + count > rowCount() || destination < 0 || destination + count > rowCount())
        return false;

    if (source == destination || count == 0)
        return true;

    Utils::moveList<FlatRow*>(_children, source, count, destination);
    return true;
}

void FlatRow::setColumnCount(int n)
{
    _data->setColumnCount(n);

    for (auto i = _children.constBegin(); i != _children.constEnd(); ++i) {
        (*i)->setColumnCount(n);
    }
}

bool FlatRow::insertColumns(int column, int count)
{
    if (column < 0 || count < 0 || column > columnCount())
        return false;

    if (count == 0)
        return true;

    _data->insertColumns(column, count, false);
    for (auto i = _children.constBegin(); i != _children.constEnd(); ++i) {
        (*i)->insertColumns(column, count);
    }

    return true;
}

bool FlatRow::appendColumns(int count)
{
    return insertColumns(columnCount(), count);
}

bool FlatRow::appendColumn()
{
    return appendColumns(1);
}

bool FlatRow::removeColumns(int column, int count)
{
    if (column < 0 || count < 0 || column + count > columnCount())
        return false;

    if (count == 0)
        return true;

    _data->removeColumns(column, count, false);
    for (auto i = _children.constBegin(); i != _children.constEnd(); ++i) {
        (*i)->removeColumns(column, count);
    }

    return true;
}

bool FlatRow::removeColumn(int column)
{
    return removeColumns(column, 1);
}

bool FlatRow::moveColumns(int source, int count, int destination)
{
    if (source < 0 || count < 0 || source + count > columnCount() || destination < 0 || destination + count > columnCount())
        return false;

    if (count == 0 || source == destination)
        return true;

    Z_CHECK(_data->moveColumns(source, count, destination));
    for (auto i = _children.constBegin(); i != _children.constEnd(); ++i) {
        Z_CHECK((*i)->moveColumns(source, count, destination));
    }

    return true;
}

bool FlatRow::copyRows(int row, int count, QList<FlatRow*>& rows)
{
    if (row < 0 || count < 0 || row + count > rowCount())
        return false;

    rows.clear();

    if (count == 0)
        return true;

    rows.reserve(count);
    for (auto i = _children.constBegin() + row; i != _children.constBegin() + row + count; ++i) {
        rows << Z_NEW(FlatRow, **i);
    }

    return true;
}

bool FlatRow::takeRows(int row, int count, QList<FlatRow*>& rows)
{
    if (row < 0 || count < 0 || row + count > rowCount())
        return false;

    rows.clear();

    if (count == 0)
        return true;

    rows.reserve(count);
    for (auto i = _children.constBegin() + row; i != _children.constBegin() + row + count; ++i) {
        rows << *i;
        (*i)->_parent = nullptr;
    }

    _children.erase(_children.begin() + row, _children.begin() + row + count);

    return true;
}

bool FlatRow::insertRows(int row, const QList<FlatRow*>& rows)
{
    if (row < 0 || row > rowCount())
        return false;

    if (rows.isEmpty())
        return true;

    Utils::insertList<FlatRow*>(_children, row, rows);

    return true;
}

bool FlatRow::appendRows(const QList<FlatRow*>& rows)
{
    return insertRows(rowCount(), rows);
}

QMap<int, QVariant> FlatRow::itemData(int column, QLocale::Language language) const
{
    return _data->getData(column)->toMap(language);
}

void FlatRow::setItemData(int column, const QMap<int, QVariant>& roles, QLocale::Language language)
{
    _data->getData(column)->fromMap(roles, language);
}

Qt::ItemFlags FlatRow::flags(int column) const
{
    return _data->getData(column)->flags();
}

void FlatRow::setFlags(int column, Qt::ItemFlags flags)
{
    _data->getData(column)->setFlags(flags);
}

FlatRow::FlatRow(_FlatRowData* data, FlatRow* parent)
{
    copyFrom(data, parent);
}

FlatRow::FlatRow(int column_count, FlatRow* parent)
    : FlatRow(column_count)
{
    _parent = parent;
}

void FlatRow::copyFrom(_FlatRowData* data, FlatRow* parent)
{
    Z_CHECK_NULL(data);

    _parent = parent;
    _data = data->clone(nullptr, false);

    if (data->hasChildren()) {
        _children.reserve(data->children()->rowCount());
        for (int i = 0; i < data->children()->rowCount(); i++) {
            _children << Z_NEW(FlatRow, data->children()->getRow(i), this);
        }
    }
}

void FlatRow::copyFromEx(const FlatRow& r)
{
    copyFrom(r._data, r._parent);

    if (r.hasChildren()) {
        _children.reserve(r._children.size());
        for (auto i = r._children.constBegin(); i != r._children.constEnd(); ++i) {
            _children << Z_NEW(FlatRow, **i);
        }
    }
}

void FlatRow::changeFlags(int column, bool enable, Qt::ItemFlags f)
{
    Qt::ItemFlags flags = this->flags(column);
    if (enable)
        flags |= f;
    else
        flags &= ~f;
    setFlags(column, flags);
}

} // namespace zf
