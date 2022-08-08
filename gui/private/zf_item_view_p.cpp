#include "zf_core.h"
#include "zf_item_view_p.h"
#include "zf_item_delegate.h"

#include <QPaintEvent>
#include <QPainter>

namespace zf
{
TableViewCorner::TableViewCorner(TableViewBase* table_view)
    : QWidget(table_view)
    , _table_view(table_view)
{
}

void TableViewCorner::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    if (_table_view->verticalHeader() != nullptr && _table_view->verticalHeader()->isVisible()) {
        QPainter painter(this);

        painter.setBrush(palette().brush(QPalette::Button));
        painter.fillRect(rect().adjusted(1, 1, 0, 0), palette().brush(QPalette::Button));

        painter.setPen(palette().color(QPalette::Mid));
        painter.drawLine(rect().right(), rect().top(), rect().right(), rect().bottom());
        painter.drawLine(rect().left(), rect().bottom(), rect().right(), rect().bottom());
    }
}

FrozenTableView::FrozenTableView(TableViewBase* base)
    : TableViewBase(base)
    , _base(base)
{
    init();
}

FrozenTableView::FrozenTableView(TableViewBase* base, View* view, const DataProperty& dataset_property, QWidget* parent)
    : TableViewBase(view, dataset_property, parent)
    , _base(base)
{
    init();
}

FrozenTableView::~FrozenTableView()
{
}

HeaderItem* FrozenTableView::rootHeaderItem(Qt::Orientation orientation) const
{
    return _base->rootHeaderItem(orientation);
}

int FrozenTableView::frozenGroupCount() const
{
    return _base->frozenGroupCount();
}

void FrozenTableView::setFrozenGroupCount(int count)
{
    _base->setFrozenGroupCount(count);
}

int FrozenTableView::sizeHintForRow(int row) const
{
    return _base->sizeHintForRow(row);
}

bool FrozenTableView::isAutoShrink() const
{
    return _base->isAutoShrink();
}

int FrozenTableView::shrinkMinimumRowCount() const
{
    return _base->shrinkMinimumRowCount();
}

int FrozenTableView::shrinkMaximumRowCount() const
{
    return _base->shrinkMaximumRowCount();
}

bool FrozenTableView::isAutoResizeRowsHeight() const
{
    return _base->isAutoResizeRowsHeight();
}

bool FrozenTableView::isBooleanColumn(int logical_index) const
{
    return _base->isBooleanColumn(logical_index);
}

bool FrozenTableView::isNoEditTriggersColumn(int logical_index) const
{
    return _base->isNoEditTriggersColumn(logical_index);
}

bool FrozenTableView::isModernCheckBox() const
{
    return _base->isModernCheckBox();
}

void FrozenTableView::setCellCheckColumn(int logical_index, bool visible, bool enabled, int level)
{
    _base->setCellCheckColumn(logical_index, visible, enabled, level);
}

QMap<int, bool> FrozenTableView::cellCheckColumns(int level) const
{
    return _base->cellCheckColumns(level);
}

bool FrozenTableView::isCellChecked(const QModelIndex& index) const
{
    return _base->isCellChecked(index);
}

void FrozenTableView::setCellChecked(const QModelIndex& index, bool checked)
{
    _base->setCellChecked(index, checked);
}

QModelIndexList FrozenTableView::checkedCells() const
{
    return _base->checkedCells();
}

void FrozenTableView::clearCheckedCells()
{
    _base->clearCheckedCells();
}

TableViewBase* FrozenTableView::base() const
{
    return _base;
}

int FrozenTableView::horizontalHeaderHeight() const
{
    return _base->horizontalHeaderHeight();
}

void FrozenTableView::onColumnResized(int column, int oldWidth, int newWidth)
{
    if (_request_resize)
        return;
    _request_resize = true;
    TableViewBase::onColumnResized(column, oldWidth, newWidth);
    _base->onColumnResized(column, oldWidth, newWidth);
    _request_resize = false;
}

void FrozenTableView::onColumnsDragging(int from_begin, int from_end, int to, int to_hidden, bool left, bool allow)
{
    TableViewBase::onColumnsDragging(from_begin, from_end, to, to_hidden, left, allow);
}

void FrozenTableView::onColumnsDragFinished()
{
    TableViewBase::onColumnsDragFinished();
}

void FrozenTableView::paintEvent(QPaintEvent* event)
{
    TableViewBase::paintEvent(event);
}

void FrozenTableView::init()
{
    Z_CHECK(_base != nullptr);
    setFrameShape(QFrame::NoFrame);

    if (view() == nullptr)
        setItemDelegate(new ItemDelegate(this, _base, nullptr, _base, structure(), datasetProperty(), this));
    else
        setItemDelegate(new ItemDelegate(view(), datasetProperty(), _base, _base, this));

    // т.к. сигнал изначально определен в интерфейсе и у него нет Q_OBJECT, синтаксис Qt5 не подходит
    connect(_base, SIGNAL(sg_checkedCellChanged(QModelIndex, bool)), this, SIGNAL(sg_checkedCellChanged(QModelIndex, bool)));

    connect(verticalHeader(), &QHeaderView::sectionResized, this, [&]() { requestResizeRowsToContents(); });

    requestResizeRowsToContents();

    horizontalHeader()->setAutoSearchEnabled(false);
}

bool changeBooleanState(const QModelIndex& idx, QAbstractItemView* view)
{
    Z_CHECK_NULL(view);

    if (idx.isValid() && view->model() != nullptr && view->editTriggers().testFlag(QAbstractItemView::DoubleClicked)
        && idx.flags().testFlag(Qt::ItemIsEditable)) {
        // надо переключить состояние boolean колонки
        bool current_state;
        bool has_check_state = false;
        QVariant v = idx.data(Qt::CheckStateRole);
        if (v.isValid() && !v.isNull()) {
            current_state = (v.toInt() == Qt::CheckState::Checked);
            has_check_state = true;

        } else {
            v = idx.data(Qt::DisplayRole);
            if (v.isNull() || !v.isValid())
                current_state = false;
            else if (v.type() == QVariant::String && v.toString().trimmed().toLower() == QStringLiteral("true"))
                current_state = true;
            else
                current_state = (v.toInt() > 0);
        }

        current_state = !current_state;

        view->model()->setData(idx, current_state, Qt::EditRole);
        if (has_check_state)
            view->model()->setData(idx, current_state ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);

        return true;
    }

    return false;
}

} // namespace zf
