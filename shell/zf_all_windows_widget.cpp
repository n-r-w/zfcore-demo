#include "zf_all_windows_widget.h"
#include "zf_core.h"
#include "zf_module_window.h"
#include "zf_translation.h"
#include "zf_framework.h"
#include "zf_work_zones.h"
#include "zf_mdi_area.h"
#include "zf_item_model.h"
#include "zf_uid.h"
#include "zf_tree_view.h"
#include "zf_colors_def.h"

#include <QListView>
#include <QApplication>

#define WIN_ROLE (Qt::UserRole + 1)
#define WORKZONE_ROLE (Qt::UserRole + 2)

namespace zf
{
AllWindowsWidget::AllWindowsWidget(MdiArea* mdi_area)
    : QWidget()
    , _windows_model(new ItemModel(0, 1, this))
    , _mdi_area(mdi_area)
{    
    setWindowIcon(QIcon(":/share_icons/windows.svg"));
    setWindowTitle(ZF_TR(ZFT_ALL_WINDOWS));

    connect(Core::windowManager(), &WindowManager::sg_openMdiWindow, this, &AllWindowsWidget::sl_openMdiWindow);
    connect(Core::windowManager(), &WindowManager::sg_closeMdiWindow, this, &AllWindowsWidget::sl_closeMdiWindow);
    connect(_mdi_area, &MdiArea::subWindowActivated, this, &AllWindowsWidget::sl_subWindowActivated);

    setLayout(new QVBoxLayout);
    layout()->setObjectName(QStringLiteral("zfla"));
    layout()->setContentsMargins(0, 0, 0, 0);
    _view = new TreeView(this);
    layout()->addWidget(_view);

    _view->setObjectName(QStringLiteral("zfw"));
    _view->header()->setVisible(false);
    _view->setEditTriggers(QListView::NoEditTriggers);
    _view->setSelectionMode(QAbstractItemView::SingleSelection);
    _view->setTextElideMode(Qt::TextElideMode::ElideRight);
    _view->setModel(_windows_model);
    _view->rootHeaderItem()->append(1)->setResizeMode(QHeaderView::ResizeMode::Stretch);
    _view->header()->setStretchLastSection(true);
    _view->setUniformRowHeights(true);    
    _view->setFrameShape(QFrame::StyledPanel);
    _view->setUseHtml(false);
    _view->setStyleSheet(QStringLiteral("QFrame {"
                                        "border: 1px %1;"
                                        "border-top-style: solid; "
                                        "border-right-style: solid; "
                                        "border-bottom-style: solid; "
                                        "border-left-style: none;"
                                        "}")
                             .arg(Colors::uiLineColor(true).name()));

    connect(_view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &AllWindowsWidget::sl_currentRowChanged);
    connect(_view, &QTreeView::clicked, this, &AllWindowsWidget::sl_clicked);
    connect(Core::windowManager(), &WindowManager::sg_workZoneInserted, this, &AllWindowsWidget::sl_workZoneInserted);
    connect(Core::windowManager(), &WindowManager::sg_workZoneRemoved, this, &AllWindowsWidget::sl_workZoneRemoved);
    connect(Core::windowManager(), &WindowManager::sg_currentWorkZoneChanged, this, &AllWindowsWidget::sl_currentWorkZoneChanged);
    connect(Core::windowManager(), &WindowManager::sg_workZoneEnabledChanged, this, &AllWindowsWidget::sl_workZoneEnabledChanged);

    // начальная загрузка
    initWindows();

    _view->expandAll();
}

AllWindowsWidget::~AllWindowsWidget()
{    
}

void AllWindowsWidget::sl_openMdiWindow(View* view)
{
    auto windows = Core::windowManager()->findModuleWindows(view->entityUid());
    Z_CHECK(!windows.isEmpty());
    ModuleWindow* window = nullptr;
    for (auto w : qAsConst(windows)) {
        if (w->view().get() == view) {
            window = w;
            break;
        }
    }
    Z_CHECK_NULL(window);

    connectToWindow(window);
}

void AllWindowsWidget::sl_closeMdiWindow(View* view)
{
    QModelIndexList indexes = findWindowIndexesAllZones(view);
    _block++;
    for (auto& idx : indexes) {
        _windows_model->removeRow(idx);
    }
    _block--;
}

void AllWindowsWidget::sl_entityNameChanged()
{
    View* v = qobject_cast<View*>(sender());
    Z_CHECK_NULL(v);
    QModelIndexList indexes = findWindowIndexesAllZones(v);
    for (auto& idx : indexes) {
        _windows_model->setData(idx, v->entityName().simplified());
    }
}

void AllWindowsWidget::sl_subWindowActivated(QMdiSubWindow* w)
{
    if (_view == nullptr)
        return;

    ModuleWindow* m_w = qobject_cast<ModuleWindow*>(w);
    if (m_w != nullptr) {
        int wz_current_id = Core::windowManager()->workZones()->currentWorkZoneId();
        QModelIndex idx = findWindowIndex(m_w->view().get(), wz_current_id);
        _block++;
        _view->setCurrentIndex(idx);
        _block--;
    }
}

void AllWindowsWidget::sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous)
    onCurrentChanged(current);
}

void AllWindowsWidget::sl_clicked(const QModelIndex& index)
{
    onCurrentChanged(index);
}

void AllWindowsWidget::sl_workZoneInserted(int id)
{
    auto zone = Core::windowManager()->workZones()->workZone(id);
    int pos = Core::windowManager()->workZones()->workZones().indexOf(zone);
    Z_CHECK(pos >= 0);
    _windows_model->insertRow(pos);

    _windows_model->setData(pos, 0, zone->name().simplified());
    _windows_model->setData(pos, 0, 0, WIN_ROLE);
    _windows_model->setData(pos, 0, zone->id(), WORKZONE_ROLE);
    _windows_model->setData(pos, 0, Utils::fontBold(qApp->font()), Qt::FontRole);
    _windows_model->setData(pos, 0, zone->icon(), Qt::DecorationRole);

    QModelIndex index = _windows_model->index(pos, 0);
    _windows_model->setFlags(index, _windows_model->flags(index) | Qt::ItemIsDropEnabled);
}

void AllWindowsWidget::sl_workZoneRemoved(int)
{
    initWindows();
}

void AllWindowsWidget::sl_currentWorkZoneChanged(int, int current_id)
{
    if (_block > 0)
        return;

    _block++;
    QModelIndex index = findWorkZoneIndex(current_id);
    if (index.isValid())
        _view->setCurrentIndex(index);

    _block--;
}

void AllWindowsWidget::sl_workZoneEnabledChanged(int id, bool enabled)
{
    if (_block > 0)
        return;

    _block++;
    auto all = findAllWorkZoneIndexes(id);
    for (auto& i : qAsConst(all)) {
        auto flags = i.flags();
        flags.setFlag(Qt::ItemIsEnabled, enabled);
        _windows_model->setFlags(i, flags);
        _view->setRowHidden(i.row(), i.parent(), !enabled);
    }

    _block--;
}

void AllWindowsWidget::sl_windowAddedToWorkZone(int work_zone_id)
{
    addWindowRow(qobject_cast<ModuleWindow*>(sender()), findWorkZoneIndex(work_zone_id));
}

void AllWindowsWidget::addWindowRow(ModuleWindow* window, const QModelIndex& zone_index)
{
    Z_CHECK_NULL(window);

    _block++;

    int row = _windows_model->appendRow(zone_index);
    QModelIndex index = _windows_model->index(row, 0, zone_index);
    _windows_model->setData(row, 0, window->view()->entityName().simplified(), Qt::EditRole, zone_index);
    _windows_model->setData(row, 0, reinterpret_cast<qintptr>(window), WIN_ROLE, zone_index);
    _windows_model->setData(row, 0, zone_index.data(WORKZONE_ROLE).toInt(), WORKZONE_ROLE, zone_index);
    _windows_model->setData(row, 0, window->view()->moduleInfo().icon(), Qt::DecorationRole, zone_index);
    _windows_model->setFlags(index, _windows_model->flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled);

    if (_view != nullptr)
        _view->setCurrentIndex(index);

    _block--;
}

void AllWindowsWidget::sl_windowRemovedFromWorkZone(int work_zone_id)
{
    auto w = qobject_cast<ModuleWindow*>(sender());

    QModelIndex idx = findWindowIndex(w->view().get(), work_zone_id);
    _block++;
    _windows_model->removeRow(idx.row(), idx.parent());
    if (w->workZoneIds().isEmpty())
        addWindowRow(w, QModelIndex());
    _block--;
}

void AllWindowsWidget::initWindows()
{
    _block++;

    Core::windowManager()->setNoActiveWindow();
    _windows_model->setRowCount(0);

    QMap<int, int> zones;

    // рабочие зоны
    for (WorkZone* w : Core::windowManager()->workZones()->workZones()) {
        int row = _windows_model->appendRow();
        _windows_model->setData(row, 0, w->name().simplified());
        _windows_model->setData(row, 0, 0, WIN_ROLE);
        _windows_model->setData(row, 0, w->id(), WORKZONE_ROLE);
        _windows_model->setData(row, 0, Utils::fontBold(qApp->font()), Qt::FontRole);
        _windows_model->setData(row, 0, w->icon(), Qt::DecorationRole);

        QModelIndex index = _windows_model->index(row, 0);
        _windows_model->setFlags(index, _windows_model->flags(index) | Qt::ItemIsDropEnabled);

        zones[w->id()] = row;
    }

    // окна
    for (ModuleWindow* w : Core::windowManager()->windows()) {
        QModelIndexList wz_indexes;
        if (w->workZoneIds().isEmpty()) {
            wz_indexes << QModelIndex();
        } else {
            for (int wz_id : w->workZoneIds()) {
                if (zones.contains(wz_id))
                    wz_indexes << _windows_model->index(zones.value(wz_id), 0);
            }
        }

        for (const QModelIndex& wz_index : qAsConst(wz_indexes)) {
            addWindowRow(w, wz_index);
        }

        connectToWindow(w);
    }

    ModuleWindow* window = qobject_cast<ModuleWindow*>(_mdi_area->activeSubWindow());
    QModelIndex row_idx;
    if (window != nullptr) {
        row_idx = findWindowIndex(window->view().get(), Core::windowManager()->workZones()->currentWorkZoneId());
        if (row_idx.isValid())
            _view->setCurrentIndex(row_idx);
    }

    if (!row_idx.isValid()) {
        row_idx = findWorkZoneIndex(Core::windowManager()->workZones()->currentWorkZoneId());
        if (row_idx.isValid())
            _view->setCurrentIndex(row_idx);
    }
    _block--;
}

ModuleWindow* AllWindowsWidget::windowFromIndex(const QModelIndex& index)
{
    return reinterpret_cast<ModuleWindow*>(index.data(WIN_ROLE).value<qintptr>());
}

void AllWindowsWidget::onCurrentChanged(const QModelIndex& current)
{
    if (_block > 0 || !current.isValid() || !Core::isActive())
        return;

    int wz_id = current.data(WORKZONE_ROLE).toInt();
    if (wz_id >= 0 && !Core::windowManager()->workZones()->isWorkZoneEnabled(wz_id))
        return; // не активную рабочую зону игнорируем

    if (wz_id >= 0)
        Core::windowManager()->workZones()->setCurrentWorkZone(wz_id);

    auto window = windowFromIndex(current);
    if (window == nullptr) {
        // это рабочая зона
        Core::windowManager()->setNoActiveWindow();
    } else {
        // это окно
        Core::windowManager()->openExistWindow(window);
    }
}

void AllWindowsWidget::connectToWindow(ModuleWindow* w)
{
    Z_CHECK_NULL(w);
    connect(w->view().get(), &View::sg_entityNameChanged, this, &AllWindowsWidget::sl_entityNameChanged);
    connect(w, &ModuleWindow::sg_addedToWorkZone, this, &AllWindowsWidget::sl_windowAddedToWorkZone);
    connect(w, &ModuleWindow::sg_removedFromWorkZone, this, &AllWindowsWidget::sl_windowRemovedFromWorkZone);
}

QModelIndexList AllWindowsWidget::findAllWorkZoneIndexes(int id) const
{
    QModelIndexList res;
    Utils::getAllIndexes(_windows_model, res);

    for (int i = res.count() - 1; i >= 0; i--) {
        if (res.at(i).data(WORKZONE_ROLE).toInt() != id)
            res.removeAt(i);
    }
    return res;
}

QModelIndex AllWindowsWidget::findWindowIndex(View* view, int work_zone_id) const
{
    Z_CHECK_NULL(view);    
    QModelIndexList indexes;
    Utils::getAllIndexes(_windows_model, indexes);
    for (auto& idx : qAsConst(indexes)) {
        ModuleWindow* w = windowFromIndex(idx);
        if (w == nullptr)
            continue; // это рабочая зона

        if (w->view().get() == view && idx.data(WORKZONE_ROLE).toInt() == work_zone_id)
            return idx;
    }
    return QModelIndex();
}

QModelIndexList AllWindowsWidget::findWindowIndexesAllZones(View* view) const
{
    Z_CHECK_NULL(view);
    QModelIndexList res;
    QModelIndexList indexes;
    Utils::getAllIndexes(_windows_model, indexes);
    for (auto& idx : qAsConst(indexes)) {
        ModuleWindow* w = windowFromIndex(idx);
        if (w == nullptr)
            continue; // это рабочая зона

        if (w->view().get() == view)
            res << idx;
    }
    return res;
}

QModelIndex AllWindowsWidget::findWorkZoneIndex(int id) const
{
    auto all = findAllWorkZoneIndexes(id);
    for (auto& i : qAsConst(all)) {
        if (windowFromIndex(i) != nullptr)
            continue; // это окно
        return i;
    }
    return QModelIndex();
}

int AllWindowsWidget::borderSize() const
{
    return 1;
}

} // namespace zf
