#include "zf_header_config_dialog.h"
#include "ui_zf_header_config_dialog.h"
#include "zf_core.h"
#include "zf_header_view.h"
#include "zf_item_delegate.h"
#include "zf_itemview_header_item.h"
#include "zf_translation.h"
#include "zf_view.h"
#include "zf_colors_def.h"

#include <QDebug>
#include <QKeyEvent>
#include <QStandardItemModel>

#define COL_NAME 0
#define COL_VISIBLE 1
#define COL_FROZEN 2
#define COL_COUNT (COL_FROZEN + 1)

#define ORDER_ROLE (Qt::UserRole + 1)
#define ITEM_ID_ROLE (Qt::UserRole + 2)
#define ITEM_OUT_OF_LIMIT_ROLE (Qt::UserRole + 3)

namespace zf
{
ItemViewHeaderConfigDialog::ItemViewHeaderConfigDialog()
    : Dialog({QDialogButtonBox::Ok, QDialogButtonBox::Cancel, QDialogButtonBox::RestoreDefaults},
        {ZF_TR(ZFT_APPLY), ZF_TR(ZFT_CANCEL), ZF_TR(ZFT_RESTORE_DEFAULTS)},
        {QDialogButtonBox::AcceptRole, QDialogButtonBox::RejectRole, QDialogButtonBox::ResetRole})
    , _ui(new Ui::ItemViewHeaderConfig)
{
    Core::registerNonParentWidget(this);

    setWindowIcon(QIcon(":/share_icons/grid.svg"));

    //    setBottomLineVisible(false);
    _ui->setupUi(workspace());

    _ui->b_left->setToolTip(ZF_TR(ZFT_COLDLG_UP));
    _ui->b_right->setToolTip(ZF_TR(ZFT_COLDLG_DOWN));
    _ui->b_first->setToolTip(ZF_TR(ZFT_COLDLG_BEGIN));
    _ui->b_last->setToolTip(ZF_TR(ZFT_COLDLG_END));

    connect(_ui->b_left, &QToolButton::clicked, this, &ItemViewHeaderConfigDialog::sl_moveLeft);
    connect(_ui->b_right, &QToolButton::clicked, this, &ItemViewHeaderConfigDialog::sl_moveRight);
    connect(_ui->b_first, &QToolButton::clicked, this, &ItemViewHeaderConfigDialog::sl_moveFirst);
    connect(_ui->b_last, &QToolButton::clicked, this, &ItemViewHeaderConfigDialog::sl_moveLast);

    _view = new TreeView;
    _view->setEditTriggers(QTreeView::NoEditTriggers);

    _view->viewport()->installEventFilter(this);
    _view->setStyleSheet(QStringLiteral("QTreeView {border: 1px %1;"
                                        "border-top-style: none; "
                                        "border-right-style: solid; "
                                        "border-bottom-style: none; "
                                        "border-left-style: solid;}")
                             .arg(Colors::uiLineColor(true).name()));

    _view->setConfigMenuOptions(DatasetConfigOptions());
    _view->horizontalHeader()->setStretchLastSection(false);
    _ui->la_main->insertWidget(0, _view);

    _view->rootHeaderItem()->append(COL_NAME, ZF_TR(ZFT_COLDLG_COLUMNS))->setResizeMode(HeaderView::Stretch);
    _view->rootHeaderItem()
        ->append(COL_VISIBLE, ZF_TR(ZFT_COLDLG_VISABILITY))
        ->setIcon(QIcon(":/share_icons/visible.svg"), true)
        ->setResizeMode(HeaderView::ResizeToContents);
    _view->rootHeaderItem()
        ->append(COL_FROZEN, ZF_TR(ZFT_COLDLG_FIX))
        ->setIcon(QIcon(":/share_icons/column.svg"), true)
        ->setResizeMode(HeaderView::ResizeToContents);
}

ItemViewHeaderConfigDialog::~ItemViewHeaderConfigDialog()
{
    delete _ui;
}

bool ItemViewHeaderConfigDialog::run(int& frozen_group, HeaderView* header_view, bool first_column_movable)
{
    Z_CHECK_NULL(header_view);

    QString dataset_name;
    if (header_view->datasetProperty().hasName())
        dataset_name = header_view->datasetProperty().name();
    else if (header_view->view() != nullptr && header_view->view()->structure()->isSingleDataset()
             && header_view->datasetProperty().entity().hasName())
        dataset_name = header_view->datasetProperty().entity().name();
    if (!dataset_name.isEmpty())
        setWindowTitle(ZF_TR(ZFT_COLDLG_NAME) + ": " + dataset_name);
    else
        setWindowTitle(ZF_TR(ZFT_COLDLG_NAME));

    _header_view = header_view;
    _root = header_view->rootItem();
    _frozen_group = frozen_group;
    _first_column_movable = first_column_movable;

    bool res = (exec() == QDialogButtonBox::Ok);
    if (res)
        frozen_group = _frozen_group;
    return res;
}

void ItemViewHeaderConfigDialog::adjustDialogSize()
{
    resize(800, 600);
}

void ItemViewHeaderConfigDialog::doLoad()
{
    _model = createModel();

    _proxy_model = new QSortFilterProxyModel;
    _proxy_model->setSourceModel(_model);
    _proxy_model->setSortRole(ORDER_ROLE);
    _proxy_model->sort(COL_NAME, Qt::AscendingOrder);

    _view->setModel(_proxy_model);
    _view->expandAll();

    _view->rootHeaderItem()->item(COL_FROZEN)->setHidden(_frozen_group < 0);

    connect(_model, &QStandardItemModel::dataChanged, this, &ItemViewHeaderConfigDialog::sl_dataChanged);
    connect(_view->selectionModel(), &QItemSelectionModel::currentChanged, this, [&]() { updateEnabled(); });

    updateEnabled();
}

void ItemViewHeaderConfigDialog::doClear()
{
    delete _proxy_model;
    _proxy_model = nullptr;
    delete _model;
    _model = nullptr;
}

Error ItemViewHeaderConfigDialog::onApply()
{
    _root->beginUpdate();
    applyOrder(QModelIndex());
    applyVisible(QModelIndex());
    _root->endUpdate();

    _frozen_group = 0;
    for (int i = 0; i < _proxy_model->rowCount(); i++) {
        QModelIndex idx = _proxy_model->index(i, 0);
        if (isColumnFrozen(idx) && isColumnVisible(idx))
            _frozen_group++;
    }

    return Error();
}

bool ItemViewHeaderConfigDialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (button == QDialogButtonBox::RestoreDefaults) {
        _block_data_change = true;
        reloadOrder(QModelIndex());
        clearFrozen(QModelIndex());
        allVisible(QModelIndex());
        _block_data_change = false;

        updateEnabled();
    }

    return Dialog::onButtonClick(button);
}

bool ItemViewHeaderConfigDialog::eventFilter(QObject* obj, QEvent* e)
{
    if (_view != nullptr && obj == _view->viewport()) {
        if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            QModelIndex index = _proxy_model->mapToSource(_view->indexAt(me->pos()));
            if (index.column() == COL_VISIBLE || index.column() == COL_FROZEN) {
                _model->setData(index, _model->data(index, Qt::CheckStateRole).toInt() == Qt::Checked ? Qt::Unchecked : Qt::Checked,
                                Qt::CheckStateRole);
            }
        }
    }

    return Dialog::eventFilter(obj, e);
}

void ItemViewHeaderConfigDialog::sl_moveLeft()
{
    auto info = getMoveLeftData();
    if (info.second == nullptr) {
        updateEnabled();
        return;
    }

    int current_order = info.first->data(ORDER_ROLE).toInt();
    int prev_order = info.second->data(ORDER_ROLE).toInt();

    info.first->setData(prev_order, ORDER_ROLE);
    info.second->setData(current_order, ORDER_ROLE);

    if (info.first->parent() == nullptr)
        updateFrozen(info.first->index(), true);

    updateEnabled();
}

void ItemViewHeaderConfigDialog::sl_moveRight()
{
    auto info = getMoveRightData();
    if (info.second == nullptr) {
        updateEnabled();
        return;
    }

    int current_order = info.first->data(ORDER_ROLE).toInt();
    int next_order = info.second->data(ORDER_ROLE).toInt();

    info.first->setData(next_order, ORDER_ROLE);
    info.second->setData(current_order, ORDER_ROLE);

    if (info.first->parent() == nullptr)
        updateFrozen(info.first->index(), true);

    updateEnabled();
}

void ItemViewHeaderConfigDialog::sl_moveFirst()
{
    while (_ui->b_left->isEnabled()) {
        sl_moveLeft();
    }
}

void ItemViewHeaderConfigDialog::sl_moveLast()
{
    while (_ui->b_right->isEnabled()) {
        sl_moveRight();
    }
}

void ItemViewHeaderConfigDialog::sl_dataChanged(
    const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Z_CHECK(topLeft == bottomRight);

    if (_block_data_change)
        return;

    if (roles.contains(Qt::CheckStateRole)) {
        Qt::CheckState check = static_cast<Qt::CheckState>(topLeft.data(Qt::CheckStateRole).toInt());
        HeaderItem* item = _root->item(_model->index(topLeft.row(), 0, topLeft.parent()).data(ITEM_ID_ROLE).toInt());

        QSet<HeaderItem*> items;
        QList<HeaderItem*> all_parent = item->allParent();
        QList<HeaderItem*> all_children = item->allChildren();

        if (topLeft.column() == COL_VISIBLE) {
            // переключаем видимость для группы узлов
            items = Utils::toSet(all_children);
            if (check == Qt::Checked)
                items.unite(Utils::toSet(all_parent));

            _block_data_change = true;
            for (auto h : qAsConst(items)) {
                if (h->isPermanentHidded())
                    continue;

                QStandardItem* model_item = _model_items.value(h);
                Z_CHECK(model_item != nullptr);

                if (check == Qt::Unchecked)
                    setColumnVisible(model_item->index(), false);
            }
            _block_data_change = false;

        } else if (topLeft.column() == COL_FROZEN) {
            // переключаем фиксацию для группы узлов

            items.unite(Utils::toSet(all_children));
            items.unite(Utils::toSet(all_parent));
            for (auto h : qAsConst(all_parent)) {
                auto all_c = h->allChildren();
                items.unite(Utils::toSet(all_c));
            }
            items.remove(item);

            for (auto h : qAsConst(items)) {
                if (h->isPermanentHidded())
                    continue;

                QStandardItem* model_item = _model_items.value(h);
                Z_CHECK_NULL(model_item);

                setColumnFrozen(model_item->index(), check == Qt::Checked);
            }

            if (!topLeft.parent().isValid())
                updateFrozen(topLeft, false);
        }
    }

    updateEnabled();
}

QPair<QStandardItem*, QStandardItem*> ItemViewHeaderConfigDialog::getMoveLeftData() const
{
    QModelIndex current_index = _view->currentIndex();
    if (!current_index.isValid())
        return QPair<QStandardItem*, QStandardItem*>(nullptr, nullptr);
    current_index = _proxy_model->index(current_index.row(), 0, current_index.parent());

    QStandardItem* current_item = _model->itemFromIndex(_proxy_model->mapToSource(current_index));
    Z_CHECK(current_item != nullptr);

    QStandardItem* prev_item = _model->itemFromIndex(
        _proxy_model->mapToSource(_proxy_model->index(current_index.row() - 1, 0, current_index.parent())));

    if (!_first_column_movable && prev_item != nullptr) {
        HeaderItem* item = headerItem(prev_item->index());
        if (item->topParent()->pos() == 0 || item->pos() == 0)
            prev_item = nullptr;
    }

    if (prev_item == nullptr)
        QPair<QStandardItem*, QStandardItem*>(nullptr, nullptr);

    return QPair<QStandardItem*, QStandardItem*>(current_item, prev_item);
}

QPair<QStandardItem*, QStandardItem*> ItemViewHeaderConfigDialog::getMoveRightData() const
{
    QModelIndex current_index = _view->currentIndex();
    if (!current_index.isValid())
        return QPair<QStandardItem*, QStandardItem*>(nullptr, nullptr);
    current_index = _proxy_model->index(current_index.row(), 0, current_index.parent());

    QStandardItem* current_item = _model->itemFromIndex(_proxy_model->mapToSource(current_index));
    Z_CHECK(current_item != nullptr);

    QStandardItem* next_item = _model->itemFromIndex(
        _proxy_model->mapToSource(_proxy_model->index(current_index.row() + 1, 0, current_index.parent())));

    if (!_first_column_movable) {
        HeaderItem* item = headerItem(current_item->index());
        if (item->topParent()->pos() == 0 || item->pos() == 0)
            next_item = nullptr;
    }

    if (next_item == nullptr)
        return QPair<QStandardItem*, QStandardItem*>(nullptr, nullptr);

    return QPair<QStandardItem*, QStandardItem*>(current_item, next_item);
}

HeaderItem* ItemViewHeaderConfigDialog::headerItem(const QModelIndex& index) const
{
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    int id = _model->index(idx.row(), COL_NAME, idx.parent()).data(ITEM_ID_ROLE).toInt();
    return _root->item(id);
}

QModelIndex ItemViewHeaderConfigDialog::headerIndex(HeaderItem* item) const
{
    auto idx = _model->match(_model->index(0, 0), ITEM_ID_ROLE, item->id(), 1, Qt::MatchExactly | Qt::MatchRecursive);
    if (idx.isEmpty())
        return QModelIndex();

    Z_CHECK(idx.count() == 1);
    return idx.first();
}

int ItemViewHeaderConfigDialog::columnOrder(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    return _model->index(idx.row(), COL_NAME, idx.parent()).data(ORDER_ROLE).toInt();
}

void ItemViewHeaderConfigDialog::setColumnVisible(const QModelIndex& index, bool b)
{
    Z_CHECK(index.isValid());
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    _model->setData(
        _model->index(idx.row(), COL_VISIBLE, idx.parent()), b ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
}

bool ItemViewHeaderConfigDialog::canHideColumn(const QModelIndex& index) const
{
    HeaderItem* item = _root->item(_model->index(index.row(), 0, index.parent()).data(ITEM_ID_ROLE).toInt());

    // проверка на попытку скрытия единственной видимой колонки
    auto all_bottom = item->root()->allBottom();
    auto item_bottom = item->allBottom();
    bool allow = false;
    for (auto h : all_bottom) {
        if (item_bottom.contains(h))
            continue;

        QModelIndex idx = headerIndex(h);
        if (isColumnVisible(idx)) {
            allow = true;
            break;
        }
    }
    if (!allow)
        return false;

    if (!_first_column_movable && item->topParent()->pos() > 0) {
        // проверка на попытку скрыть первый видимый
        QList<HeaderItem*> group_items = item->topParent()->allBottom();
        for (auto h : group_items) {
            QModelIndex idx = headerIndex(h);
            if (!idx.isValid() || !isColumnVisible(idx) || idx == index)
                continue;
            return true;
        }
        return false;
    }
    return true;
}

bool ItemViewHeaderConfigDialog::isColumnFrozen(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    return _model->index(idx.row(), COL_FROZEN, idx.parent()).data(Qt::CheckStateRole).toInt() == Qt::Checked;
}

void ItemViewHeaderConfigDialog::setColumnFrozen(const QModelIndex& index, bool b)
{
    Z_CHECK(index.isValid());
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    _model->setData(
        _model->index(idx.row(), COL_FROZEN, idx.parent()), b ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
}

bool ItemViewHeaderConfigDialog::isColumnVisible(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    QModelIndex idx = (index.model() == _proxy_model ? _proxy_model->mapToSource(index) : index);
    return _model->index(idx.row(), COL_VISIBLE, idx.parent()).data(Qt::CheckStateRole).toInt() == Qt::Checked;
}

void ItemViewHeaderConfigDialog::updateEnabled()
{
    _ui->b_left->setEnabled(getMoveLeftData().second != nullptr);
    _ui->b_first->setEnabled(_ui->b_left->isEnabled());
    _ui->b_right->setEnabled(getMoveRightData().second != nullptr);
    _ui->b_last->setEnabled(_ui->b_right->isEnabled());

    bool allow_ok = _model->rowCount() == 0;
    for (int i = 0; i < _model->rowCount(); i++) {
        if (_model->item(i, COL_VISIBLE)->data(Qt::CheckStateRole).toInt() == Qt::Checked) {
            allow_ok = true;
            break;
        }
    }

    setOkButtonEnabled(allow_ok);
}

void ItemViewHeaderConfigDialog::updateFrozen(const QModelIndex& index, bool moved)
{
    Z_CHECK(!index.parent().isValid());
    QModelIndex idx = index.parent().model() == _proxy_model ? _proxy_model->mapToSource(index) : index;

    int order = columnOrder(idx);
    bool frozen = isColumnFrozen(idx);

    // действия при переключении фиксации
    if (frozen) {
        if (!moved) {
            // делаем фиксированными все левее данного узла
            for (int i = 0; i < _model->rowCount(); i++) {
                QModelIndex index = _model->index(i, COL_NAME);
                int c_order = columnOrder(index);
                if (c_order < order && !isColumnFrozen(index)) {
                    setColumnFrozen(index, true);
                }
            }
        }

    } else {
        // все узлы справа должны стать не фиксированными
        for (int i = 0; i < _model->rowCount(); i++) {
            QModelIndex index = _model->index(i, COL_NAME);
            int c_order = columnOrder(index);
            if (c_order > order && isColumnFrozen(index)) {
                setColumnFrozen(index, false);
            }
        }
    }

    if (moved && frozen) {
        // если после перемещения фиксированной есть предыдущие не фиксированные узлы, то надо сбрасывать
        bool remove_frozen = false;
        for (int i = 0; i < _model->rowCount(); i++) {
            QModelIndex index = _model->index(i, COL_NAME);
            int c_order = columnOrder(index);
            if (c_order < order && !isColumnFrozen(index)) {
                remove_frozen = true;
                break;
            }
        }

        if (remove_frozen) {
            // сбрасываем у всех справа
            for (int i = 0; i < _model->rowCount(); i++) {
                QModelIndex index = _model->index(i, COL_NAME);
                int c_order = columnOrder(index);
                if (c_order >= order && isColumnFrozen(index)) {
                    setColumnFrozen(index, false);
                }
            }
        }
    }
}

void ItemViewHeaderConfigDialog::clearFrozen(const QModelIndex& index)
{
    for (int i = 0; i < _proxy_model->rowCount(index); i++) {
        QModelIndex idx = _proxy_model->index(i, 0, index);
        setColumnFrozen(idx, false);
        clearFrozen(idx);
    }
}

void ItemViewHeaderConfigDialog::allVisible(const QModelIndex& index)
{
    for (int i = 0; i < _proxy_model->rowCount(index); i++) {
        QModelIndex idx = _proxy_model->index(i, 0, index);
        setColumnVisible(idx, true);
        allVisible(idx);
    }
}

void ItemViewHeaderConfigDialog::applyOrder(const QModelIndex& index)
{
    for (int i = 0; i < _proxy_model->rowCount(index); i++) {
        QModelIndex idx = _proxy_model->index(i, 0, index);
        HeaderItem* item = headerItem(idx);
        HeaderItem* move_to_item = item->parent()->childrenVisual(Qt::AscendingOrder).at(i);
        item->move(move_to_item->id(), true);

        applyOrder(idx);
    }
}

void ItemViewHeaderConfigDialog::applyVisible(const QModelIndex& index)
{
    for (int i = 0; i < _proxy_model->rowCount(index); i++) {
        QModelIndex idx = _proxy_model->index(i, 0, index);
        headerItem(idx)->setHidden(!isColumnVisible(idx));
        applyVisible(idx);
    }
}

QStandardItemModel* ItemViewHeaderConfigDialog::createModel()
{
    Z_CHECK(_header_view != nullptr);
    Z_CHECK(_root != nullptr);

    QStandardItemModel* model = new QStandardItemModel(0, COL_COUNT);
    createModelHelper(model, _header_view, _root, model->invisibleRootItem());
    return model;
}

void ItemViewHeaderConfigDialog::createModelHelper(
    QStandardItemModel* model, HeaderView* view, HeaderItem* item, QStandardItem* model_item)
{    
    if (model_item != model->invisibleRootItem()) {
        bool out_limit = item->sectionFrom(false, true) >= view->limitSectionCount();
        bool frozen = _frozen_group > 0 && item->topParent()->visualPos(true) < _frozen_group;

        _model_items[item] = model_item;
        model_item->setData(item->id(), ITEM_ID_ROLE);
        model_item->setText(item->label(true).isEmpty() ? QString::number(item->id()) : item->label(true));
        model_item->setIcon(item->icon());
        model_item->setData(item->sectionFrom(false), ORDER_ROLE);
        model_item->setData(out_limit, ITEM_OUT_OF_LIMIT_ROLE);
        model_item->setEditable(false);

        QStandardItem* parent = model_item->parent();
        if (parent == nullptr)
            parent = model->invisibleRootItem();

        QStandardItem* m_item = parent->child(model_item->row(), COL_VISIBLE);
        m_item->setData(item->isHidden() ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);

        if (!_first_column_movable && item->topParent()->pos() == 0 && item->pos() == 0)
            m_item->setEnabled(false);

        m_item->setCheckable(false);
        m_item->setEditable(false);
        m_item->setToolTip(ZF_TR(ZFT_COLDLG_VISABILITY_TOOLTIP));

        m_item = parent->child(model_item->row(), COL_FROZEN);
        m_item->setData(frozen ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        m_item->setCheckable(false);
        m_item->setEditable(false);
        m_item->setToolTip(ZF_TR(ZFT_COLDLG_FIX_TOOLTIP));
    }

    for (auto h : item->children()) {
        if (h->isPermanentHidded())
            continue;

        QList<QStandardItem*> model_items;
        for (int i = 0; i < COL_COUNT; i++) {
            model_items << new QStandardItem;
        }

        model_item->appendRow(model_items);
        createModelHelper(model, view, h, model_items.first());
    }
}

void ItemViewHeaderConfigDialog::reloadOrder(const QModelIndex& index)
{
    for (int i = 0; i < _model->rowCount(index); i++) {
        QModelIndex idx = _model->index(i, 0, index);
        _model->setData(idx, headerItem(idx)->sectionFrom(true), ORDER_ROLE);
        reloadOrder(idx);
    }
}

} // namespace zf
