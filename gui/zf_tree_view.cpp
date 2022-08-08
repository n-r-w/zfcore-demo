#include "zf_tree_view.h"
#include "zf_header_view.h"
#include "zf_item_delegate.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_view.h"
#include "private/zf_tree_view_p.h"
#include "private/zf_item_view_p.h"
#include <private/qtreeview_p.h>

#include <QApplication>
#include <QDebug>
#include <QScrollBar>
#include <QBuffer>

namespace zf
{
TreeView::TreeView(QWidget* parent)
    : QTreeView(parent)
{
    init();
}

TreeView::TreeView(const DataStructurePtr& structure, const DataProperty& dataset_property, QWidget* parent)
    : QTreeView(parent)
    , _dataset_property(dataset_property)
    , _structure(structure)
{
    Z_CHECK_NULL(_structure);
    Z_CHECK(_dataset_property.isValid());
    init();
}

TreeView::TreeView(const View* view, const DataProperty& dataset_property, QWidget* parent)
    : QTreeView(parent)
    , _view(view)
    , _dataset_property(dataset_property)
{
    Z_CHECK_NULL(_view);
    Z_CHECK(_dataset_property.isValid());
    _structure = _view->structure();
    init();
}

TreeView::TreeView(const DataWidgets* widgets, const DataProperty& dataset_property, const HighlightProcessor* highlight, QWidget* parent)
    : QTreeView(parent)
    , _widgets(widgets)
    , _highlight(highlight)
    , _dataset_property(dataset_property)
{
    Z_CHECK_NULL(_widgets);
    Z_CHECK(_dataset_property.isValid());
    _structure = _widgets->structure();
    init();
}

HeaderItem* TreeView::rootHeaderItem() const
{
    return header()->rootItem();
}

HeaderItem* TreeView::headerItem(int id) const
{
    return rootHeaderItem()->item(id);
}

HeaderView* TreeView::horizontalHeader() const
{
    return qobject_cast<HeaderView*>(QTreeView::header());
}

HeaderView* TreeView::header() const
{
    return horizontalHeader();
}

void TreeView::setSortingEnabled(bool enable)
{
    horizontalHeader()->setAllowSorting(enable);
    QTreeView::setSortingEnabled(enable);
}

DatasetConfigOptions TreeView::configMenuOptions() const
{
    return horizontalHeader()->configMenuOptions();
}

void TreeView::setConfigMenuOptions(const DatasetConfigOptions& o)
{
    horizontalHeader()->setConfigMenuOptions(o);
}

void TreeView::setModel(QAbstractItemModel* model)
{
    if (model == this->model())
        return;

    if (this->model() != nullptr) {
        disconnect(this->model(), &QAbstractItemModel::layoutChanged, this, &TreeView::sl_layoutChanged);
        disconnect(this->model(), &QAbstractItemModel::rowsRemoved, this, &TreeView::sl_rowsRemoved);
        disconnect(this->model(), &QAbstractItemModel::rowsInserted, this, &TreeView::sl_rowsInserted);
        disconnect(this->model(), &QAbstractItemModel::rowsMoved, this, &TreeView::sl_rowsMoved);
        disconnect(this->model(), &QAbstractItemModel::modelReset, this, &TreeView::sl_modelReset);
    }

    if (model != nullptr) {
        connect(model, &QAbstractItemModel::layoutChanged, this, &TreeView::sl_layoutChanged);
        connect(model, &QAbstractItemModel::rowsRemoved, this, &TreeView::sl_rowsRemoved);
        connect(model, &QAbstractItemModel::rowsInserted, this, &TreeView::sl_rowsInserted);
        connect(model, &QAbstractItemModel::rowsMoved, this, &TreeView::sl_rowsMoved);
        connect(model, &QAbstractItemModel::modelReset, this, &TreeView::sl_modelReset);
    }

    _checked.clear();
    _all_checked = false;

    QTreeView::setModel(model);
}

bool TreeView::isReloading() const
{
    return _reloading > 0;
}

View* TreeView::view() const
{
    return const_cast<View*>(_view);
}

DataProperty TreeView::datasetProperty() const
{
    return _dataset_property;
}

DataWidgets* TreeView::widgets() const
{
    return const_cast<DataWidgets*>(_widgets);
}

HighlightProcessor* TreeView::highlight() const
{
    return const_cast<HighlightProcessor*>(_highlight);
}

DataStructurePtr TreeView::structure() const
{
    return _structure;
}

void TreeView::setUseHtml(bool b)
{
    if (auto d = qobject_cast<ItemDelegate*>(itemDelegate())) {
        d->setUseHtml(b);
        update();
    } else {
        Z_HALT_INT;
    }
}

bool TreeView::isUseHtml() const
{
    if (auto d = qobject_cast<ItemDelegate*>(itemDelegate()))
        return d->isUseHtml();

    Z_HALT_INT;
    return false;
}

void TreeView::setFormatBottomItems(bool b)
{
    if (auto d = qobject_cast<ItemDelegate*>(itemDelegate())) {
        d->setFormatBottomItems(b);
        update();
    } else {
        Z_HALT_INT;
    }
}

bool TreeView::isFormatBottomItems() const
{
    if (auto d = qobject_cast<ItemDelegate*>(itemDelegate()))
        return d->isFormatBottomItems();

    Z_HALT_INT;
    return false;
}

Error TreeView::serialize(QIODevice* device) const
{
    return Utils::saveHeader(device, rootHeaderItem(), 0);
}

Error TreeView::serialize(QByteArray& ba) const
{
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    return serialize(&buffer);
}

Error TreeView::deserialize(QIODevice* device)
{
    int frozen_count;
    Error error = Utils::loadHeader(device, rootHeaderItem(), frozen_count);
    if (error.isError())
        return error;

    // размер последней секции при условии, что stretchLastSection() == true
    if (header()->stretchLastSection()) {
        int last_visible = rootHeaderItem()->lastVisibleSection();
        if (last_visible >= 0 && rootHeaderItem()->bottomCount() > last_visible)
            rootHeaderItem()->bottomItem(last_visible)->setSectionSize(rootHeaderItem()->defaultSectionSize());
    }

    return Error();
}

Error TreeView::deserialize(const QByteArray& ba)
{
    if (ba.isNull())
        return Error();

    QBuffer buffer;
    buffer.setData(ba);
    buffer.open(QIODevice::ReadOnly);
    return deserialize(&buffer);
}

void TreeView::setBooleanColumn(int logical_index, bool is_boolean)
{
    if (is_boolean) {
        if (!_boolean_columns.contains(logical_index))
            _boolean_columns << logical_index;
    } else {
        _boolean_columns.removeOne(logical_index);
    }
}

bool TreeView::isBooleanColumn(int logical_index) const
{
    return _boolean_columns.contains(logical_index);
}

void TreeView::setNoEitTriggersColumn(int logical_index, bool no_edit_triggers)
{
    if (no_edit_triggers) {
        if (!_no_edit_triggers_columns.contains(logical_index))
            _no_edit_triggers_columns << logical_index;
    } else {
        _no_edit_triggers_columns.removeOne(logical_index);
    }
}

bool TreeView::isNoEditTriggersColumn(int logical_index) const
{
    return _no_edit_triggers_columns.contains(logical_index);
}

void TreeView::setModernCheckBox(bool b)
{
    if (b == _modern_check_box)
        return;

    _modern_check_box = b;
    viewport()->update();
}

bool TreeView::isModernCheckBox() const
{
    return _modern_check_box;
}

void TreeView::setCurrentIndex(const QModelIndex& index)
{
    _saved_index = index;
    QTreeView::setCurrentIndex(index);
}

void TreeView::updateGeometries()
{
    if (!isShowCheckPanel()) {
        QTreeView::updateGeometries();
        return;
    }

    // Метод setViewportMargins предназначен для управления отступами. Мало того, в документации указано что это может быть полезно при
    // создании нестандартных боковиков у таблиц, что нам и надо. Однако криворукие дебилы из Qt плюют на свою же документацию и
    // принудительно устанавливают setViewportMargins внутри QTreeView::updateGeometries. Если попытаться задать его после вызова
    // QTreeView::updateGeometries, то начнется рекурсивный вызов. Поэтому мы вынуждены полностью клонировать данный метод и задавать
    // setViewportMargins самостоятельно.

    if (_geometry_recursion_block)
        return;
    _geometry_recursion_block = true;

    int left_panel_width = TreeCheckBoxPanel::width();

    int height = 0;
    if (!header()->isHidden()) {
        height = qMax(header()->minimumHeight(), header()->sizeHint().height());
        height = qMin(height, header()->maximumHeight());
    }
    setViewportMargins(left_panel_width - 1, height, 0, 0);
    QRect vg = viewport()->geometry();
    QRect geometryRect(vg.left(), vg.top() - height, vg.width(), height);
    header()->setGeometry(geometryRect);
    QMetaObject::invokeMethod(header(), "updateGeometries");
    privatePtr()->updateScrollBars();

    _geometry_recursion_block = false;
    QAbstractItemView::updateGeometries();

    _check_panel->setGeometry(0, 0, left_panel_width, geometry().height());
    _check_panel->update();
}

void TreeView::delegateGetCheckInfo(QAbstractItemView* item_view, const QModelIndex& index, bool& show, bool& checked) const
{
    Q_UNUSED(item_view)
    show = cellCheckColumns(indexLevel(index)).contains(index.column());
    if (!show) {
        checked = false;
        return;
    }
    checked = isCellChecked(index);
}

void TreeView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);

    // отображение куда будет вставлен перетаскиваемый заголовок
    Utils::paintHeaderDragHandle(this, horizontalHeader());
}

QStyleOptionViewItem TreeView::viewOptions() const
{
    auto opt = QTreeView::viewOptions();
    // убираем выделение в узле (QPalette::Highlight используется в QCommonStyle::drawPrimitive - PE_PanelItemViewRow)
    opt.palette.setBrush(QPalette::Highlight, QBrush(QColor(0, 0, 0, 0)));
    return opt;
}

bool TreeView::event(QEvent* event)
{
    bool res = QTreeView::event(event);

    if (event->type() == QEvent::DynamicPropertyChange) {
        QDynamicPropertyChangeEvent* e = static_cast<QDynamicPropertyChangeEvent*>(event);
        if (e->propertyName() == QStringLiteral("sortingEnabled") && header() != nullptr) {
            header()->setAllowSorting(isSortingEnabled());
        }
    }

    return res;
}

QModelIndex TreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    // обработка клавиш left,right, которая вместо нормальной навигации делает expand/collapse
    QModelIndex idx = currentIndex();
    QModelIndex next_idx = idx;

    if (modifiers == Qt::NoModifier && (cursorAction == MoveLeft || cursorAction == MoveRight) && idx.isValid()) {
        if (isFirstColumnSpanned(idx.row(), idx.parent()))
            return QTreeView::moveCursor(cursorAction, modifiers);

        auto root = rootHeaderItem();
        auto visual_headers = root->allBottomVisual(Qt::AscendingOrder, true);
        if (visual_headers.count() > 1) {
            int visual_pos = root->logicalToVisual(idx.column());
            if (cursorAction == MoveLeft)
                visual_pos--;
            else
                visual_pos++;

            if (visual_pos >= 0 && visual_pos < visual_headers.count()) {
                next_idx = model()->index(idx.row(), root->visualToLogical(visual_pos), idx.parent());
                Z_CHECK(next_idx.isValid());
            }
        }

        return next_idx;
    }

    return QTreeView::moveCursor(cursorAction, modifiers);
}

bool TreeView::viewportEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::ToolTip: {
            if (!toolTip().isEmpty())
                // вызываем метод пропуская QTreeView, т.к. он подавляет тултипы самого виджета в пользу делегата
                return QAbstractScrollArea::viewportEvent(event);
            break;
        }
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove: {
            // обновления ячеек при переходе с одной на другую
            QHoverEvent* e = static_cast<QHoverEvent*>(event);
            QModelIndex index = indexAt(e->pos());

            if (index != _hover_index) {
                if (_hover_index.isValid()) {
                    update(_hover_index);
                }

                _hover_index = index;
            }

            if (index.isValid())
                update(index);

            break;
        }
        default:
            break;
    }

    return QTreeView::viewportEvent(event);
}

bool TreeView::eventFilter(QObject* object, QEvent* event)
{
    return QTreeView::eventFilter(object, event);
}

void TreeView::mousePressEvent(QMouseEvent* e)
{
    QModelIndex idx = indexAt(e->pos());

    if (idx.isValid()) {
        // было ли нажатие на чекбокс внутри ячейки
        ItemDelegate* delegate = qobject_cast<ItemDelegate*>(itemDelegate());
        if (delegate != nullptr) {
            if (cellCheckColumns(indexLevel(idx)).value(idx.column())) {
                QRect check_rect = delegate->checkBoxRect(idx, false);
                if (check_rect.contains(e->pos())) {
                    setCellChecked(idx, !isCellChecked(idx));
                }
            }
        }
    }

    QTreeView::mousePressEvent(e);
}

void TreeView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton) {
        // блокируем генерацию даблклика по любой кнопке кроме левой
        e->ignore();
        return;
    }

    QModelIndex idx = indexAt(e->pos());

    // игнорируем нажатия в область чекбокса
    if (idx.isValid()) {
        ItemDelegate* delegate = qobject_cast<ItemDelegate*>(itemDelegate());
        if (delegate != nullptr) {
            if (delegate->checkBoxRect(idx, true).contains(e->pos())) {
                e->ignore();
                return;
            }
        }
    }

    if (isBooleanColumn(idx.column())) {
        if (_dataset_property.options().testFlag(PropertyOption::ReadOnly)) {
            e->ignore();
            return;
        }
        if (_dataset_property.columns().count() > idx.column()) {
            auto property = _dataset_property.columns().at(idx.column());
            if (property.options().testFlag(PropertyOption::ReadOnly)) {
                e->ignore();
                return;
            }
        }

        if (changeBooleanState(idx, this)) {
            e->ignore();
            return;
        }
    }

    QTreeView::mouseDoubleClickEvent(e);
}

void TreeView::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);
    _check_panel->update();
}

void TreeView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space && e->modifiers() == Qt::NoModifier && currentIndex().isValid()) {
        if (isBooleanColumn(currentIndex().column())) {
            if (changeBooleanState(currentIndex(), this)) {
                e->ignore();
                return;
            }
        }
    }

    QTreeView::keyPressEvent(e);
}

bool TreeView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    if (isNoEditTriggersColumn(index.column()))
        return false;

    return QTreeView::edit(index, trigger, event);
}

void TreeView::selectColumn(int column)
{
    if (selectionBehavior() == QTreeView::SelectRows || (selectionMode() == QTreeView::SingleSelection && selectionBehavior() == QTreeView::SelectItems))
        return;

    if (column < 0 || column >= model()->columnCount())
        return;

    QItemSelection selection;
    selectColumnHelper(selection, column, QModelIndex());

    selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Columns);
}

void TreeView::sl_configMenuRequested(const QPoint& pos, const DatasetConfigOptions& options)
{
    int frozen_group = -1;
    Utils::headerConfig(frozen_group, horizontalHeader(), pos, options, false, _view, _dataset_property);
}

void TreeView::sl_beforeLoadDataFromRootHeader()
{
    _saved_index = currentIndex();
    _reloading++;
}

void TreeView::sl_afterLoadDataFromRootHeader()
{
    //    if (_saved_index.isValid())
    //        setCurrentIndex(_saved_index);
    _reloading--;
}

void TreeView::sl_columnsDragging(int from_begin, int from_end, int to, int to_hidden, bool left, bool allow)
{
    Q_UNUSED(from_begin)
    Q_UNUSED(from_end)
    Q_UNUSED(to)
    Q_UNUSED(to_hidden)
    Q_UNUSED(left)
    Q_UNUSED(allow)

    viewport()->update();
}

void TreeView::sl_columnsDragFinished()
{
    viewport()->update();
}

void TreeView::sl_columnResized(int column, int oldWidth, int newWidth)
{
    Q_UNUSED(column)
    Q_UNUSED(oldWidth)
    Q_UNUSED(newWidth)

    if (!_column_resize_timer->isActive())
        _column_resize_timer->start();
}

void TreeView::sl_expanded(const QModelIndex& index)
{
    Q_UNUSED(index)
    if (isShowCheckPanel())
        _check_panel->update();
}

void TreeView::sl_collapsed(const QModelIndex& index)
{
    Q_UNUSED(index)
    if (isShowCheckPanel())
        _check_panel->update();
}

void TreeView::sl_layoutChanged()
{
    _check_panel->update();
}

void TreeView::sl_rowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);
    _check_panel->update();
    emit sg_checkedRowsChanged();
}

void TreeView::sl_rowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);
    _check_panel->update();
}

void TreeView::sl_rowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    Q_UNUSED(destination);
    Q_UNUSED(row);

    _checked.clear();
    _check_panel->update();
}

void TreeView::sl_modelReset()
{
    _checked.clear();
    _check_panel->update();
    emit sg_checkedRowsChanged();
}

QTreeViewPrivate* TreeView::privatePtr() const
{
    return reinterpret_cast<QTreeViewPrivate*>(d_ptr.data());
}

void TreeView::init()
{
    _check_panel = new TreeCheckBoxPanel(this);
    _check_panel->setHidden(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(SingleSelection);
    setSelectionBehavior(SelectRows);
    setTextElideMode(Qt::ElideNone);
    setExpandsOnDoubleClick(false);
    setEditTriggers(DoubleClicked | EditKeyPressed | AnyKeyPressed);

    setHeader(new HeaderView(_view, _dataset_property, Qt::Horizontal, this));
    if (isSortingEnabled())
        horizontalHeader()->setAllowSorting(true);

    connect(this, &TreeView::expanded, this, &TreeView::sl_expanded);
    connect(this, &TreeView::collapsed, this, &TreeView::sl_collapsed);

    connect(horizontalHeader(), &HeaderView::sg_columnsDragging, this, &TreeView::sl_columnsDragging);
    connect(horizontalHeader(), &HeaderView::sg_columnsDragFinished, this, &TreeView::sl_columnsDragFinished);
    connect(horizontalHeader(), &HeaderView::sg_configMenuRequested, this, &TreeView::sl_configMenuRequested);
    connect(horizontalHeader(), &HeaderView::sg_beforeLoadDataFromRootHeader, this, &TreeView::sl_beforeLoadDataFromRootHeader);
    connect(horizontalHeader(), &HeaderView::sg_afterLoadDataFromRootHeader, this, &TreeView::sl_afterLoadDataFromRootHeader);

    // иначе не будет обновляться отрисовка фильтра и т.п. глюки
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [&]() { horizontalHeader()->viewport()->update(); });

    if (view() != nullptr)
        setItemDelegate(new ItemDelegate(view(), datasetProperty(), nullptr, this, this));
    else if (widgets() != nullptr)
        setItemDelegate(new ItemDelegate(widgets(), datasetProperty(), highlight(), nullptr, this, this));
    else
        setItemDelegate(new ItemDelegate(this, nullptr, nullptr, this, structure(), datasetProperty(), this));

    // т.к. заголовок меняет высоту при смене ширины колонки, то надо обновлять геометрию таблицы
    connect(horizontalHeader(), &QHeaderView::sectionResized, this, &TreeView::sl_columnResized);
    _column_resize_timer = new QTimer(this);
    _column_resize_timer->setSingleShot(true);
    _column_resize_timer->setInterval(0);
    connect(_column_resize_timer, &QTimer::timeout, this, [&]() { updateGeometries(); });
}

void TreeView::selectColumnHelper(QItemSelection& selection, int column, const QModelIndex& parent)
{
    QModelIndex index = model()->index(0, column, parent);
    if (!index.isValid())
        return;

    selection.merge(QItemSelection(index, index), QItemSelectionModel::Select | QItemSelectionModel::Columns);

    for (int row = 0; row < model()->rowCount(parent); row++) {
        selectColumnHelper(selection, column, model()->index(row, 0, parent));
    }
}

int TreeView::indexLevel(const QModelIndex& index)
{
    int level = 0;
    QModelIndex parent = index.parent();
    while (parent.isValid()) {
        parent = parent.parent();
        level++;
    }
    return level;
}

std::shared_ptr<QMap<int, bool>> TreeView::cellCheckColumnsHelper(int level) const
{
    auto columns = _cell_сheck_сolumns.value(-1);
    if (columns != nullptr)
        return columns;

    columns = _cell_сheck_сolumns.value(level);
    if (columns != nullptr)
        return columns;

    return nullptr;
}

QModelIndex TreeView::sourceIndex(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    QModelIndex idx = Utils::getTopSourceIndex(index);
    if (model() != nullptr)
        Z_CHECK(idx.model() == Utils::getTopSourceModel(model()));
    return idx;
}

void TreeView::showCheckPanel(bool show)
{
    if (isShowCheckPanel() == show)
        return;

    _check_panel->setHidden(!show);
    updateGeometries();

    emit sg_checkPanelVisibleChanged();
}

bool TreeView::isShowCheckPanel() const
{
    return !_check_panel->isHidden();
}

bool TreeView::hasCheckedRows() const
{
    return isAllRowsChecked() || !_checked.isEmpty();
}

bool TreeView::isRowChecked(const QModelIndex& index) const
{
    if (_all_checked)
        return true;

    return _checked.contains(sourceIndex(index));
}

void TreeView::checkRow(const QModelIndex& index, bool checked)
{
    if (isRowChecked(index) == checked)
        return;

    QModelIndex source_index = sourceIndex(index);

    if (checked) {
        if (_all_checked)
            return;

        _checked << source_index;
    } else {
        if (_all_checked) {
            Q_ASSERT(_checked.isEmpty());
            QModelIndexList all_indexes;
            Utils::getAllIndexes(Utils::getTopSourceModel(model()), all_indexes);

            for (auto idx : qAsConst(all_indexes)) {
                if (source_index == idx)
                    continue;
                _checked << idx;
            }
            _all_checked = false;

        } else {
            _checked.remove(source_index);
        }
    }

    _check_panel->update();
    emit sg_checkedRowsChanged();
}

QSet<QModelIndex> TreeView::checkedRows() const
{
    QSet<QModelIndex> res;
    if (isAllRowsChecked() && _checked.isEmpty()) {
        QModelIndexList all_indexes;
        Utils::getAllIndexes(model(), all_indexes);

        for (auto& i : qAsConst(all_indexes)) {
            res << Utils::getTopSourceIndex(i);
        }

    } else {
        for (auto& idx : qAsConst(_checked)) {
            res << idx;
        }
    }

    return res;
}

bool TreeView::isAllRowsChecked() const
{
    return _all_checked;
}

void TreeView::checkAllRows(bool checked)
{
    if (_all_checked && checked)
        return;

    _checked.clear();
    _all_checked = checked;

    _check_panel->update();
    emit sg_checkedRowsChanged();
}

QMap<int, bool> TreeView::cellCheckColumns(int level) const
{
    auto columns = cellCheckColumnsHelper(level);
    if (columns != nullptr)
        return *columns;

    return {};
}

void TreeView::setCellCheckColumn(int logical_index, bool visible, bool enabled, int level)
{
    Z_CHECK(level >= -1);
    auto columns = _cell_сheck_сolumns.value(level);
    if (columns == nullptr) {
        columns = std::make_shared<QMap<int, bool>>();
        _cell_сheck_сolumns[level] = columns;
    }

    if (visible) {
        if (columns->contains(logical_index))
            return;
        columns->insert(logical_index, enabled);
    } else {
        if (!columns->contains(logical_index))
            return;
        columns->remove(logical_index);
        if (columns->isEmpty())
            _cell_сheck_сolumns.remove(level);
    }

    viewport()->update();
}

bool TreeView::isCellChecked(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    QModelIndex source_index = sourceIndex(index);

    for (int i = _cell_checked.count() - 1; i >= 0; i--) {
        if (!_cell_checked.at(i).isValid()) {
            _cell_checked.removeAt(i);
            continue;
        }

        if (_cell_checked.at(i) == source_index)
            return true;
    }

    return false;
}

void TreeView::setCellChecked(const QModelIndex& index, bool checked)
{
    Z_CHECK(index.isValid());
    QModelIndex source_index = sourceIndex(index);
    QModelIndex view_index = Utils::alignIndexToModel(index, model());

    for (int i = _cell_checked.count() - 1; i >= 0; i--) {
        if (!_cell_checked.at(i).isValid()) {
            _cell_checked.removeAt(i);
            continue;
        }

        if (_cell_checked.at(i) != source_index)
            continue;

        if (checked)
            return;

        _cell_checked.removeAt(i);
        emit sg_checkedCellChanged(source_index, false);
        if (view_index.isValid())
            update(view_index);
        return;
    }

    if (checked) {
        _cell_checked << source_index;
        emit sg_checkedCellChanged(source_index, true);
        if (view_index.isValid())
            update(view_index);
    }
}

QModelIndexList TreeView::checkedCells() const
{
    QModelIndexList res;
    for (int i = _cell_checked.count() - 1; i >= 0; i--) {
        if (!_cell_checked.at(i).isValid()) {
            _cell_checked.removeAt(i);
            continue;
        }

        res << _cell_checked.at(i);
    }

    return res;
}

void TreeView::clearCheckedCells()
{
    for (auto& c : qAsConst(_cell_checked)) {
        setCellChecked(c, false);
    }
}
} // namespace zf
