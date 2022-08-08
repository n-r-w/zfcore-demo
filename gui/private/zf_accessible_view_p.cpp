#include "zf_accessible_view_p.h"
#include "zf_item_selector_p.h"
#include "zf_item_view_p.h"
#include "zf_core.h"
#include "zf_itemview_header_item.h"

#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <private/qtreeview_p.h>
#include <private/qwidget_p.h>

namespace zf
{
QAbstractItemView* AccessibleTable::view() const
{
    return qobject_cast<QAbstractItemView*>(object());
}

int AccessibleTable::logicalIndex(const QModelIndex& index) const
{
    if (!view()->model() || !index.isValid())
        return -1;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (index.row() + hHeader)*(index.model()->columnCount() + vHeader) + (index.column() + vHeader);
}

AccessibleTable::AccessibleTable(QWidget* w)
    : QAccessibleObject(w)
{
    Z_CHECK(view());

    if (qobject_cast<const QTableView*>(view())) {
        m_role = QAccessible::Table;
    } else if (qobject_cast<const QTreeView*>(view())) {
        m_role = QAccessible::Tree;
    } else if (qobject_cast<const QListView*>(view())) {
        m_role = QAccessible::List;
    } else {
        // is this our best guess?
        m_role = QAccessible::Table;
    }
}

bool AccessibleTable::isValid() const
{
    return view() && !qt_widget_private(view())->data.in_destructor;
}

AccessibleTable::~AccessibleTable()
{
    for (QAccessible::Id id : qAsConst(childToId))
        QAccessible::deleteAccessibleInterface(id);
}

QHeaderView* AccessibleTable::horizontalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
    } else if (const QTableView* tv = qobject_cast<const QTableView*>(view())) {
        header = tv->horizontalHeader();

    } else if (const QTreeView* tv = qobject_cast<const QTreeView*>(view())) {
        header = tv->header();
    }
    return header;
}

QHeaderView* AccessibleTable::verticalHeader() const
{
    QHeaderView *header = 0;
    if (false) {
    } else if (const QTableView* tv = qobject_cast<const QTableView*>(view())) {
        header = tv->verticalHeader();
    }
    return header;
}

QAccessibleInterface* AccessibleTable::cellAt(int row, int column) const
{
    if (!view()->model())
        return 0;
    Z_CHECK(role() != QAccessible::Tree);
    QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning() << "QAccessibleTable::cellAt: invalid index: " << index << " for " << view();
        return 0;
    }
    return child(logicalIndex(index));
}

QAccessibleInterface* AccessibleTable::caption() const
{
    return 0;
}

QString AccessibleTable::columnDescription(int column) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(column, Qt::Horizontal).toString();
}

int AccessibleTable::columnCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->columnCount();
}

int AccessibleTable::rowCount() const
{
    if (!view()->model())
        return 0;
    return view()->model()->rowCount();
}

int AccessibleTable::selectedCellCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedIndexes().count();
}

int AccessibleTable::selectedColumnCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedColumns().count();
}

int AccessibleTable::selectedRowCount() const
{
    if (!view()->selectionModel())
        return 0;
    return view()->selectionModel()->selectedRows().count();
}

QString AccessibleTable::rowDescription(int row) const
{
    if (!view()->model())
        return QString();
    return view()->model()->headerData(row, Qt::Vertical).toString();
}

QList<QAccessibleInterface*> AccessibleTable::selectedCells() const
{
    QList<QAccessibleInterface*> cells;
    if (!view()->selectionModel())
        return cells;
    const QModelIndexList selectedIndexes = view()->selectionModel()->selectedIndexes();
    cells.reserve(selectedIndexes.size());
    for (const QModelIndex &index : selectedIndexes)
        cells.append(child(logicalIndex(index)));
    return cells;
}

QList<int> AccessibleTable::selectedColumns() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> columns;
    const QModelIndexList selectedColumns = view()->selectionModel()->selectedColumns();
    columns.reserve(selectedColumns.size());
    for (const QModelIndex &index : selectedColumns)
        columns.append(index.column());

    return columns;
}

QList<int> AccessibleTable::selectedRows() const
{
    if (!view()->selectionModel())
        return QList<int>();
    QList<int> rows;
    const QModelIndexList selectedRows = view()->selectionModel()->selectedRows();
    rows.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows)
        rows.append(index.row());

    return rows;
}

QAccessibleInterface* AccessibleTable::summary() const
{
    return 0;
}

bool AccessibleTable::isColumnSelected(int column) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isColumnSelected(column, QModelIndex());
}

bool AccessibleTable::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    return view()->selectionModel()->isRowSelected(row, QModelIndex());
}

bool AccessibleTable::selectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectRows && columnCount() > 1 )
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}

bool AccessibleTable::selectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;
    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectRows)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if (view()->selectionBehavior() != QAbstractItemView::SelectColumns && rowCount() > 1)
            return false;
        Q_FALLTHROUGH();
    case QAbstractItemView::ContiguousSelection:
        if ((!column || !view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && !view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Columns);
    return true;
}

bool AccessibleTable::unselectRow(int row)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(row, 0, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedRowCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedRowCount() == 1)
            return false;

        if ((!row || view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex())) {
            //If there are rows selected both up the current row and down the current rown,
            //the ones which are down the current row will be deselected
            selection = QItemSelection(index, view()->model()->index(rowCount() - 1, 0, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    return true;
}

bool AccessibleTable::unselectColumn(int column)
{
    if (!view()->model() || !view()->selectionModel())
        return false;

    QModelIndex index = view()->model()->index(0, column, view()->rootIndex());
    if (!index.isValid())
        return false;

    QItemSelection selection(index, index);

    switch (view()->selectionMode()) {
    case QAbstractItemView::SingleSelection:
        //In SingleSelection and ContiguousSelection once an item
        //is selected, there's no way for the user to unselect all items
        if (selectedColumnCount() == 1)
            return false;
        break;
    case QAbstractItemView::ContiguousSelection:
        if (selectedColumnCount() == 1)
            return false;

        if ((!column || view()->selectionModel()->isColumnSelected(column - 1, view()->rootIndex()))
            && view()->selectionModel()->isColumnSelected(column + 1, view()->rootIndex())) {
            //If there are columns selected both at the left of the current row and at the right
            //of the current rown, the ones which are at the right will be deselected
            selection = QItemSelection(index, view()->model()->index(0, columnCount() - 1, view()->rootIndex()));
        }
    default:
        break;
    }

    view()->selectionModel()->select(selection, QItemSelectionModel::Deselect | QItemSelectionModel::Columns);
    return true;
}

QAccessible::Role AccessibleTable::role() const
{
    return m_role;
}

QAccessible::State AccessibleTable::state() const
{
    return QAccessible::State();
}

QAccessibleInterface* AccessibleTable::childAt(int x, int y) const
{
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);
    // FIXME: if indexPosition < 0 in one coordinate, return header

    QModelIndex index = view()->indexAt(indexPosition);
    if (index.isValid()) {
        return child(logicalIndex(index));
    }
    return 0;
}

int AccessibleTable::childCount() const
{
    if (!view()->model())
        return 0;
    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;
    return (view()->model()->rowCount()+hHeader) * (view()->model()->columnCount()+vHeader);
}

int AccessibleTable::indexOfChild(const QAccessibleInterface* iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface *parent = iface->parent();
    if (parent->object() != view())
        return -1;

    Z_CHECK(iface->role() != QAccessible::TreeItem); // should be handled by tree class
    if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
        const AccessibleTableCell* cell = static_cast<const AccessibleTableCell*>(iface);
        return logicalIndex(cell->m_index);
    } else if (iface->role() == QAccessible::ColumnHeader){
        const AccessibleTableHeaderCell* cell = static_cast<const AccessibleTableHeaderCell*>(iface);
        return cell->index + (verticalHeader() ? 1 : 0);
    } else if (iface->role() == QAccessible::RowHeader){
        const AccessibleTableHeaderCell* cell = static_cast<const AccessibleTableHeaderCell*>(iface);
        return (cell->index + 1) * (view()->model()->columnCount() + 1);
    } else if (iface->role() == QAccessible::Pane) {
        return 0; // corner button
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild Fix my children..."
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: we are in denial of our children. this should stop.
    return -1;
}

QString AccessibleTable::text(QAccessible::Text t) const
{
    if (t == QAccessible::Description)
        return view()->accessibleDescription();
    return view()->accessibleName();
}

QRect AccessibleTable::rect() const
{
    if (!view()->isVisible())
        return QRect();
    QPoint pos = view()->mapToGlobal(QPoint(0, 0));
    return QRect(pos.x(), pos.y(), view()->width(), view()->height());
}

QAccessibleInterface* AccessibleTable::parent() const
{
    if (view() && view()->parent()) {
        if (qstrcmp("QComboBoxPrivateContainer", view()->parent()->metaObject()->className()) == 0
            || qstrcmp("zf::ItemSelectorContainer", view()->parent()->metaObject()->className()) == 0) {
            return QAccessible::queryAccessibleInterface(view()->parent()->parent());
        }
        return QAccessible::queryAccessibleInterface(view()->parent());
    }
    return 0;
}

QAccessibleInterface* AccessibleTable::child(int logicalIndex) const
{
    if (!view()->model())
        return 0;

    auto id = childToId.constFind(logicalIndex);
    if (id != childToId.constEnd())
        return QAccessible::accessibleInterface(id.value());

    int vHeader = verticalHeader() ? 1 : 0;
    int hHeader = horizontalHeader() ? 1 : 0;

    int columns = view()->model()->columnCount() + vHeader;

    int row = logicalIndex / columns;
    int column = logicalIndex % columns;

    QAccessibleInterface *iface = 0;

    if (vHeader) {
        if (column == 0) {
            if (hHeader && row == 0) {
                iface = new AccessibleTableCornerButton(view());
            } else {
                iface = new AccessibleTableHeaderCell(view(), row - hHeader, Qt::Vertical);
            }
        }
        --column;
    }
    if (!iface && hHeader) {
        if (row == 0) {
            iface = new AccessibleTableHeaderCell(view(), column, Qt::Horizontal);
        }
        --row;
    }

    if (!iface) {
        QModelIndex index = view()->model()->index(row, column, view()->rootIndex());
        if (Q_UNLIKELY(!index.isValid())) {
            qWarning("QAccessibleTable::child: Invalid index at: %d %d", row, column);
            return 0;
        }
        iface = new AccessibleTableCell(view(), index, cellRole());
    }

    QAccessible::registerAccessibleInterface(iface);
    childToId.insert(logicalIndex, QAccessible::uniqueId(iface));
    return iface;
}

void* AccessibleTable::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableInterface)
       return static_cast<QAccessibleTableInterface*>(this);
   return 0;
}

void AccessibleTable::modelChange(QAccessibleTableModelChangeEvent* event)
{
    // if there is no cache yet, we don't update anything
    if (childToId.isEmpty())
        return;

    switch (event->modelChangeType()) {
    case QAccessibleTableModelChangeEvent::ModelReset:
        for (QAccessible::Id id : qAsConst(childToId))
            QAccessible::deleteAccessibleInterface(id);
        childToId.clear();
        break;

    // rows are inserted: move every row after that
    case QAccessibleTableModelChangeEvent::RowsInserted:
    case QAccessibleTableModelChangeEvent::ColumnsInserted: {
        int newRows = event->lastRow() - event->firstRow() + 1;
        int newColumns = event->lastColumn() - event->firstColumn() + 1;

        ChildCache newCache;
        ChildCache::ConstIterator iter = childToId.constBegin();

        while (iter != childToId.constEnd()) {
            QAccessible::Id id = iter.value();
            QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
            Z_CHECK(iface);
            if (event->modelChangeType() == QAccessibleTableModelChangeEvent::RowsInserted
                && iface->role() == QAccessible::RowHeader) {
                AccessibleTableHeaderCell* cell = static_cast<AccessibleTableHeaderCell*>(iface);
                if (cell->index >= event->firstRow()) {
                    cell->index += newRows;
                }
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::ColumnsInserted
                   && iface->role() == QAccessible::ColumnHeader) {
                AccessibleTableHeaderCell* cell = static_cast<AccessibleTableHeaderCell*>(iface);
                if (cell->index >= event->firstColumn()) {
                    cell->index += newColumns;
                }
            }
            if (indexOfChild(iface) >= 0) {
                newCache.insert(indexOfChild(iface), id);
            } else {
                // ### This should really not happen,
                // but it might if the view has a root index set.
                // This needs to be fixed.
                QAccessible::deleteAccessibleInterface(id);
            }
            ++iter;
        }
        childToId = newCache;
        break;
    }

    case QAccessibleTableModelChangeEvent::ColumnsRemoved:
    case QAccessibleTableModelChangeEvent::RowsRemoved: {
        int deletedColumns = event->lastColumn() - event->firstColumn() + 1;
        int deletedRows = event->lastRow() - event->firstRow() + 1;
        ChildCache newCache;
        ChildCache::ConstIterator iter = childToId.constBegin();
        while (iter != childToId.constEnd()) {
            QAccessible::Id id = iter.value();
            QAccessibleInterface *iface = QAccessible::accessibleInterface(id);
            Z_CHECK(iface);
            if (iface->role() == QAccessible::Cell || iface->role() == QAccessible::ListItem) {
                Z_CHECK(iface->tableCellInterface());
                AccessibleTableCell* cell = static_cast<AccessibleTableCell*>(iface->tableCellInterface());
                // Since it is a QPersistentModelIndex, we only need to check if it is valid
                if (cell->m_index.isValid())
                    newCache.insert(indexOfChild(cell), id);
                else
                    QAccessible::deleteAccessibleInterface(id);
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::RowsRemoved
                       && iface->role() == QAccessible::RowHeader) {
                AccessibleTableHeaderCell* cell = static_cast<AccessibleTableHeaderCell*>(iface);
                if (cell->index < event->firstRow()) {
                    newCache.insert(indexOfChild(cell), id);
                } else if (cell->index > event->lastRow()) {
                    cell->index -= deletedRows;
                    newCache.insert(indexOfChild(cell), id);
                } else {
                    QAccessible::deleteAccessibleInterface(id);
                }
            } else if (event->modelChangeType() == QAccessibleTableModelChangeEvent::ColumnsRemoved
                   && iface->role() == QAccessible::ColumnHeader) {
                AccessibleTableHeaderCell* cell = static_cast<AccessibleTableHeaderCell*>(iface);
                if (cell->index < event->firstColumn()) {
                    newCache.insert(indexOfChild(cell), id);
                } else if (cell->index > event->lastColumn()) {
                    cell->index -= deletedColumns;
                    newCache.insert(indexOfChild(cell), id);
                } else {
                    QAccessible::deleteAccessibleInterface(id);
                }
            }
            ++iter;
        }
        childToId = newCache;
        break;
    }

    case QAccessibleTableModelChangeEvent::DataChanged:
        // nothing to do in this case
        break;
    }
}

// TREE VIEW

QModelIndex AccessibleTree::indexFromLogical(int row, int column) const
{
    if (!isValid() || !view()->model())
        return QModelIndex();

    const QTreeView* treeView = qobject_cast<const QTreeView*>(view());

    if (Q_UNLIKELY(row < 0 || column < 0 || call_d_func(treeView)->viewItems.count() <= row)) {
        qWarning() << "AccessibleTree::indexFromLogical: invalid index: " << row << column << " for " << treeView;
        return QModelIndex();
    }
    QModelIndex modelIndex = call_d_func(treeView)->viewItems.at(row).index;

    if (modelIndex.isValid() && column > 0) {
        modelIndex = view()->model()->index(modelIndex.row(), column, modelIndex.parent());
    }
    return modelIndex;
}

QTreeViewPrivate* AccessibleTree::call_d_func(const QObject* obj)
{
    Z_CHECK_NULL(obj);
    ItemSelectorTreeView* i_tree = qobject_cast<ItemSelectorTreeView*>(const_cast<QObject*>(obj));
    if (i_tree != nullptr)
        return static_cast<QTreeViewPrivate*>(i_tree->privateData());

    zf::TreeView* z_tree = qobject_cast<zf::TreeView*>(const_cast<QObject*>(obj));
    if (z_tree != nullptr)
        return z_tree->privatePtr();

    Z_HALT(obj->metaObject()->className());
    return nullptr;
}

QAccessibleInterface* AccessibleTree::childAt(int x, int y) const
{
    if (!view()->model())
        return 0;
    QPoint viewportOffset = view()->viewport()->mapTo(view(), QPoint(0,0));
    QPoint indexPosition = view()->mapFromGlobal(QPoint(x, y) - viewportOffset);

    QModelIndex index = view()->indexAt(indexPosition);
    if (!index.isValid())
        return 0;

    const QTreeView* treeView = qobject_cast<const QTreeView*>(view());

    int row = call_d_func(treeView)->viewIndex(index) + (horizontalHeader() ? 1 : 0);
    int column = index.column();

    int i = row * view()->model()->columnCount() + column;
    return child(i);
}

int AccessibleTree::childCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Z_CHECK(treeView);
    if (!view()->model())
        return 0;

    int hHeader = horizontalHeader() ? 1 : 0;
    return (call_d_func(treeView)->viewItems.count() + hHeader) * view()->model()->columnCount();
}

QAccessibleInterface* AccessibleTree::child(int logicalIndex) const
{
    if (logicalIndex < 0 || !view()->model() || !view()->model()->columnCount())
        return 0;

    QAccessibleInterface *iface = 0;
    int index = logicalIndex;

    if (horizontalHeader()) {
        if (index < view()->model()->columnCount()) {
            iface = new AccessibleTableHeaderCell(view(), index, Qt::Horizontal);
        } else {
            index -= view()->model()->columnCount();
        }
    }

    if (!iface) {
        int row = index / view()->model()->columnCount();
        int column = index % view()->model()->columnCount();
        QModelIndex modelIndex = indexFromLogical(row, column);
        if (!modelIndex.isValid())
            return 0;
        iface = new AccessibleTableCell(view(), modelIndex, cellRole());
    }
    QAccessible::registerAccessibleInterface(iface);
    // ### FIXME: get interfaces from the cache instead of re-creating them
    return iface;
}

int AccessibleTree::rowCount() const
{
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Z_CHECK(treeView);
    return call_d_func(treeView)->viewItems.count();
}

int AccessibleTree::indexOfChild(const QAccessibleInterface* iface) const
{
    if (!view()->model())
        return -1;
    QAccessibleInterface *parent = iface->parent();
    if (parent->object() != view())
        return -1;

    if (iface->role() == QAccessible::TreeItem) {
        const AccessibleTableCell* cell = static_cast<const AccessibleTableCell*>(iface);
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
        Z_CHECK(treeView);
        int row = call_d_func(treeView)->viewIndex(cell->m_index) + (horizontalHeader() ? 1 : 0);
        int column = cell->m_index.column();

        int index = row * view()->model()->columnCount() + column;
        return index;
    } else if (iface->role() == QAccessible::ColumnHeader){
        const AccessibleTableHeaderCell* cell = static_cast<const AccessibleTableHeaderCell*>(iface);
        return cell->index;
    } else {
        qWarning() << "WARNING QAccessibleTable::indexOfChild invalid child"
                   << iface->role() << iface->text(QAccessible::Name);
    }
    // FIXME: add scrollbars and don't just ignore them
    return -1;
}

QAccessibleInterface* AccessibleTree::cellAt(int row, int column) const
{
    QModelIndex index = indexFromLogical(row, column);
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning("Requested invalid tree cell: %d %d", row, column);
        return 0;
    }
    const QTreeView *treeView = qobject_cast<const QTreeView*>(view());
    Z_CHECK(treeView);
    int logicalIndex = call_d_func(treeView)->accessibleTable2Index(index);

    return child(logicalIndex); // FIXME ### new AccessibleTableCell(view(), index, cellRole());
}

QString AccessibleTree::rowDescription(int) const
{
    return QString(); // no headers for rows in trees
}

bool AccessibleTree::isRowSelected(int row) const
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);
    return view()->selectionModel()->isRowSelected(index.row(), index.parent());
}

bool AccessibleTree::selectRow(int row)
{
    if (!view()->selectionModel())
        return false;
    QModelIndex index = indexFromLogical(row);

    if (!index.isValid() || view()->selectionBehavior() == QAbstractItemView::SelectColumns)
        return false;

    switch (view()->selectionMode()) {
    case QAbstractItemView::NoSelection:
        return false;
    case QAbstractItemView::SingleSelection:
        if ((view()->selectionBehavior() != QAbstractItemView::SelectRows) && (columnCount() > 1))
            return false;
        view()->clearSelection();
        break;
    case QAbstractItemView::ContiguousSelection:
        if ((!row || !view()->selectionModel()->isRowSelected(row - 1, view()->rootIndex()))
            && !view()->selectionModel()->isRowSelected(row + 1, view()->rootIndex()))
            view()->clearSelection();
        break;
    default:
        break;
    }

    view()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    return true;
}


// TABLE CELL
AccessibleTableCell::AccessibleTableCell(QAbstractItemView* view_, const QModelIndex& index_, QAccessible::Role role_)
    : /* QAccessibleSimpleEditableTextInterface(this), */ view(view_)
    , m_index(index_)
    , m_role(role_)
{
    if (Q_UNLIKELY(!index_.isValid()))
        qWarning() << "AccessibleTableCell::AccessibleTableCell with invalid index: " << index_;
}

void* AccessibleTableCell::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TableCellInterface)
        return static_cast<QAccessibleTableCellInterface*>(this);
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

int AccessibleTableCell::columnExtent() const
{
    return 1;
}
int AccessibleTableCell::rowExtent() const
{
    return 1;
}

QList<QAccessibleInterface*> AccessibleTableCell::rowHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (verticalHeader()) {
        // FIXME
        headerCell.append(new AccessibleTableHeaderCell(view, m_index.row(), Qt::Vertical));
    }
    return headerCell;
}

QList<QAccessibleInterface*> AccessibleTableCell::columnHeaderCells() const
{
    QList<QAccessibleInterface*> headerCell;
    if (horizontalHeader()) {
        // FIXME
        headerCell.append(new AccessibleTableHeaderCell(view, m_index.column(), Qt::Horizontal));
    }
    return headerCell;
}

QHeaderView* AccessibleTableCell::horizontalHeader() const
{
    QHeaderView *header = 0;

    if (false) {
    } else if (const QTableView *tv = qobject_cast<const QTableView*>(view)) {
        header = tv->horizontalHeader();

    } else if (const QTreeView *tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
    }

    return header;
}

QHeaderView* AccessibleTableCell::verticalHeader() const
{
    QHeaderView *header = 0;

    if (const QTableView *tv = qobject_cast<const QTableView*>(view))
        header = tv->verticalHeader();

    return header;
}

int AccessibleTableCell::columnIndex() const
{
    if (!isValid())
        return -1;
    return m_index.column();
}

int AccessibleTableCell::rowIndex() const
{
    if (!isValid())
        return -1;

    if (role() == QAccessible::TreeItem) {
       const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
       Z_CHECK(treeView);
       int row = AccessibleTree::call_d_func(treeView)->viewIndex(m_index);
       return row;
    }

    return m_index.row();
}

bool AccessibleTableCell::isSelected() const
{
    if (!isValid())
        return false;
    return view->selectionModel()->isSelected(m_index);
}

QStringList AccessibleTableCell::actionNames() const
{
    QStringList names;
    names << toggleAction();
    return names;
}

void AccessibleTableCell::doAction(const QString& actionName)
{
    if (actionName == toggleAction()) {
        if (isSelected())
            unselectCell();
        else
            selectCell();
    }
}

QStringList AccessibleTableCell::keyBindingsForAction(const QString&) const
{
    return QStringList();
}

void AccessibleTableCell::selectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;
    Z_CHECK(table());
    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
    case QAbstractItemView::SelectItems:
        break;
    case QAbstractItemView::SelectColumns:
        if (cellTable)
            cellTable->selectColumn(m_index.column());
        return;
    case QAbstractItemView::SelectRows:
        if (cellTable)
            cellTable->selectRow(m_index.row());
        return;
    }

    if (selectionMode == QAbstractItemView::SingleSelection) {
        view->clearSelection();
    }

    view->selectionModel()->select(m_index, QItemSelectionModel::Select);
}

void AccessibleTableCell::unselectCell()
{
    if (!isValid())
        return;
    QAbstractItemView::SelectionMode selectionMode = view->selectionMode();
    if (selectionMode == QAbstractItemView::NoSelection)
        return;

    QAccessibleTableInterface *cellTable = table()->tableInterface();

    switch (view->selectionBehavior()) {
    case QAbstractItemView::SelectItems:
        break;
    case QAbstractItemView::SelectColumns:
        if (cellTable)
            cellTable->unselectColumn(m_index.column());
        return;
    case QAbstractItemView::SelectRows:
        if (cellTable)
            cellTable->unselectRow(m_index.row());
        return;
    }

    //If the mode is not MultiSelection or ExtendedSelection and only
    //one cell is selected it cannot be unselected by the user
    if ((selectionMode != QAbstractItemView::MultiSelection)
        && (selectionMode != QAbstractItemView::ExtendedSelection)
        && (view->selectionModel()->selectedIndexes().count() <= 1))
        return;

    view->selectionModel()->select(m_index, QItemSelectionModel::Deselect);
}

QAccessibleInterface* AccessibleTableCell::table() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessible::Role AccessibleTableCell::role() const
{
    return m_role;
}

QAccessible::State AccessibleTableCell::state() const
{
    QAccessible::State st;
    if (!isValid())
        return st;

    QRect globalRect = view->rect();
    globalRect.translate(view->mapToGlobal(QPoint(0,0)));
    if (!globalRect.intersects(rect()))
        st.invisible = true;

    if (view->selectionModel()->isSelected(m_index))
        st.selected = true;
    if (view->selectionModel()->currentIndex() == m_index)
        st.focused = true;
    if (m_index.model()->data(m_index, Qt::CheckStateRole).toInt() == Qt::Checked)
        st.checked = true;

    Qt::ItemFlags flags = m_index.flags();
    if (flags & Qt::ItemIsSelectable) {
        st.selectable = true;
        st.focusable = true;
        if (view->selectionMode() == QAbstractItemView::MultiSelection)
            st.multiSelectable = true;
        if (view->selectionMode() == QAbstractItemView::ExtendedSelection)
            st.extSelectable = true;
    }

    if (m_role == QAccessible::TreeItem) {
        const QTreeView *treeView = qobject_cast<const QTreeView*>(view);
        if (treeView->model()->hasChildren(m_index))
            st.expandable = true;
        if (treeView->isExpanded(m_index))
            st.expanded = true;
    }

    return st;
}

QRect AccessibleTableCell::rect() const
{
    QRect r;
    if (!isValid())
        return r;
    r = view->visualRect(m_index);

    if (!r.isNull()) {
        r.translate(view->viewport()->mapTo(view, QPoint(0,0)));
        r.translate(view->mapToGlobal(QPoint(0, 0)));
    }
    return r;
}

QString AccessibleTableCell::text(QAccessible::Text t) const
{
    QString value;
    if (!isValid())
        return value;
    QAbstractItemModel *model = view->model();
    switch (t) {
    case QAccessible::Name:
        value = model->data(m_index, Qt::AccessibleTextRole).toString();
        if (value.isEmpty())
            value = model->data(m_index, Qt::DisplayRole).toString();
        break;
    case QAccessible::Description:
        value = model->data(m_index, Qt::AccessibleDescriptionRole).toString();
        break;
    default:
        break;
    }
    return value;
}

void AccessibleTableCell::setText(QAccessible::Text /*t*/, const QString& text)
{
    if (!isValid() || !(m_index.flags() & Qt::ItemIsEditable))
        return;
    view->model()->setData(m_index, text);
}

bool AccessibleTableCell::isValid() const
{
    return view && !qt_widget_private(view)->data.in_destructor
            && view->model() && m_index.isValid();
}

QAccessibleInterface* AccessibleTableCell::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface* AccessibleTableCell::child(int) const
{
    return 0;
}

AccessibleTableHeaderCell::AccessibleTableHeaderCell(QAbstractItemView* view_, int index_, Qt::Orientation orientation_)
    : view(view_)
    , index(index_)
    , orientation(orientation_)
{
    Z_CHECK(index_ >= 0);
}

QAccessible::Role AccessibleTableHeaderCell::role() const
{
    if (orientation == Qt::Horizontal)
        return QAccessible::ColumnHeader;
    return QAccessible::RowHeader;
}

QAccessible::State AccessibleTableHeaderCell::state() const
{
    QAccessible::State s;
    if (QHeaderView *h = headerView()) {
        s.invisible = !h->testAttribute(Qt::WA_WState_Visible);
        s.disabled = !h->isEnabled();
    }
    return s;
}

QRect AccessibleTableHeaderCell::rect() const
{
    QHeaderView *header = 0;
    if (false) {
    } else if (const QTableView* tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }

    } else if (const QTreeView* tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
    }
    if (!header)
        return QRect();
    QPoint zero = header->mapToGlobal(QPoint(0, 0));
    int sectionSize = header->sectionSize(index);
    int sectionPos = header->sectionPosition(index);
    return orientation == Qt::Horizontal
            ? QRect(zero.x() + sectionPos, zero.y(), sectionSize, header->height())
            : QRect(zero.x(), zero.y() + sectionPos, header->width(), sectionSize);
}

QString AccessibleTableHeaderCell::text(QAccessible::Text t) const
{    
    QAbstractItemModel *model = view->model();
    QString value;
    switch (t) {
        case QAccessible::Name: {
            HeaderItem* root = nullptr;
            if (auto v = qobject_cast<zf::TableView*>(view))
                root = v->horizontalRootHeaderItem();
            if (auto v = qobject_cast<zf::FrozenTableView*>(view))
                root = v->rootHeaderItem(Qt::Horizontal);
            else if (auto v = qobject_cast<zf::TreeView*>(view))
                root = v->rootHeaderItem();

            if (root != nullptr) {
                if (index >= 0 && index < root->bottomCount()) {
                    auto item = root->bottomItem(index);
                    QStringList text;
                    while (item != nullptr && !item->isRoot()) {
                        text.prepend(item->label(true).simplified());
                        item = item->parent();
                    }
                    value = text.join(";");
                }
            }
            if (value.isEmpty()) {
                value = model->headerData(index, orientation, Qt::AccessibleTextRole).toString();
                if (value.isEmpty())
                    value = model->headerData(index, orientation, Qt::DisplayRole).toString();
            }

            break;
        }
        case QAccessible::Description:
            value = model->headerData(index, orientation, Qt::AccessibleDescriptionRole).toString();
            break;
        default:
            break;
    }
    return value;
}

void AccessibleTableHeaderCell::setText(QAccessible::Text, const QString&)
{
    return;
}

bool AccessibleTableHeaderCell::isValid() const
{
    return view && !qt_widget_private(view)->data.in_destructor
            && view->model() && (index >= 0)
            && ((orientation == Qt::Horizontal) ? (index < view->model()->columnCount()) : (index < view->model()->rowCount()));
}

QAccessibleInterface* AccessibleTableHeaderCell::parent() const
{
    return QAccessible::queryAccessibleInterface(view);
}

QAccessibleInterface* AccessibleTableHeaderCell::child(int) const
{
    return 0;
}

QHeaderView* AccessibleTableHeaderCell::headerView() const
{
    QHeaderView *header = 0;
    if (false) {
    } else if (const QTableView* tv = qobject_cast<const QTableView*>(view)) {
        if (orientation == Qt::Horizontal) {
            header = tv->horizontalHeader();
        } else {
            header = tv->verticalHeader();
        }

    } else if (const QTreeView* tv = qobject_cast<const QTreeView*>(view)) {
        header = tv->header();
    }
    return header;
}

QAccessibleInterface* AccessibleTable::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("zf::ItemSelectorListView") || class_name == QLatin1String("zf::TableView"))
        return new AccessibleTable(qobject_cast<QWidget*>(object));

    return nullptr;
}

QAccessibleInterface* AccessibleTree::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("zf::ItemSelectorTreeView") || class_name == QLatin1String("zf::TreeView"))
        return new AccessibleTree(qobject_cast<QWidget*>(object));

    return nullptr;
}

} // namespace zf
