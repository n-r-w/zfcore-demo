#include "zf_main_window.h"
#include "ui_zf_main_window.h"
#include "zf_core.h"
#include "zf_core_messages.h"
#include "zf_framework.h"
#include "zf_html_tools.h"
#include "zf_progress_dialog.h"
#include "zf_window_manager.h"
#include "zf_all_windows_widget.h"
#include "zf_settings_dialog.h"
#include "zf_translation.h"
#include "zf_colors_def.h"

#include <QMessageBox>
#include <QMdiSubWindow>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#define SPLITTER_CONFIG "WINDOWS_SPLITTER"

namespace zf
{
MainWindow::MainWindow(const QString& app_name)
    : QMainWindow()
    , _ui(new Ui::MainWindow)
    , _mdi_area(new MdiArea)
    , _status_message(new QLabel)
    , _object_extension(new ObjectExtension(this))
{
    //    Core::logInfo("main window start load");

    _ui->setupUi(this);

    _ui->statusbar->setHidden(true); // пока не нужен

    _ui->top_line->setFrameShape(QFrame::Shape::StyledPanel);
    _ui->top_line->setMinimumHeight(2);
    _ui->top_line->setStyleSheet(QString("border: none;"
                                         "border-top: 1px solid %1;"
                                         "border-bottom: 1px solid white;")
                                     .arg("#DADADA"));

    // зона MDI
    _mdi_area->setDocumentMode(false);
    _mdi_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _mdi_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _ui->la_mdi->addWidget(_mdi_area);
    Core::fr()->initWindowManager(_mdi_area);

    // рабочие зоны
    _work_zones_tab = new QTabBar;
    _work_zones_tab->setHidden(true);
    _work_zones_tab->setShape(QTabBar::RoundedEast);
    _work_zones_tab->setDocumentMode(true);
    _work_zones_tab->setUsesScrollButtons(true);
    _work_zones_tab->setAutoHide(true);
    _work_zones_tab->setElideMode(Qt::ElideNone);
    _work_zones_tab->setDrawBase(false);
    _work_zones_tab->setSizePolicy(_work_zones_tab->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    _work_zones_tab->setIconSize({Z_PM(PM_LargeIconSize), Z_PM(PM_LargeIconSize)});
    _ui->la_work_zones->addWidget(_work_zones_tab);
    _ui->la_work_zones->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    Utils::prepareTabBar(_work_zones_tab, true, 2);
    connect(_work_zones_tab, &QTabBar::currentChanged, this, &MainWindow::sl_work_zones_tab_changed);

    setContextMenuPolicy(Qt::NoContextMenu);
    configureStatusBar();
    configureMainToolbar();
    configureServiceToolbar();
    configureModuleToolbar();

    qApp->setApplicationName(app_name);
    setWindowTitle(QString("%1 - %2").arg(app_name, Core::connectionInformation()->userInformation().fio()));
    setWindowIcon(qApp->windowIcon());

    connect(Core::windowManager(), &WindowManager::sg_openMdiWindow, this, &MainWindow::sl_openMdiWindow);
    connect(Core::windowManager(), &WindowManager::sg_closeMdiWindow, this, &MainWindow::sl_closeMdiWindow);

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");
    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::SHELL, this, QStringLiteral("sl_message_dispatcher_inbound")));
    Core::fr()->setActive(true);

    _progress_dialog = new ProgressDialog(this);
    _progress_dialog->hide();

    // Устанавливаем интерфейс рабочих зон
    Core::fr()->installWorkZonesInterface(this);

    // Список окон
    _all_win_widget = new AllWindowsWidget(_mdi_area);
    _ui->la_windows_list->addWidget(_all_win_widget);    
    _ui->windows_splitter->setCollapsible(0, false);

    connect(_ui->windows_splitter, &QSplitter::splitterMoved, this, [this]() {
        if (objectExtensionDestroyed())
            return;
        Framework::internalCallbackManager()->addRequest(this, Framework::MAIN_WINDOW_SPLITTER_KEY);
    });

    if (Core::fr()->configuration()->contains(SPLITTER_CONFIG))
        _ui->windows_splitter->setSizes(Core::fr()->configuration()->value(SPLITTER_CONFIG).value<QList<int>>());
    else
        _ui->windows_splitter->setSizes({2000, 200});
}

MainWindow::~MainWindow()
{
    delete _progress_dialog;

    if (Core::isBootstraped()) {
        Core::fr()->initWindowManager(nullptr);
        Core::fr()->setActive(false);
    }
    delete _ui;
    delete _object_extension;
}

bool MainWindow::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void MainWindow::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant MainWindow::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool MainWindow::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void MainWindow::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void MainWindow::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void MainWindow::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void MainWindow::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void MainWindow::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void MainWindow::addMenu(QMenu* menu)
{
    Z_CHECK_NULL(menu);
    if (menu->parent() == nullptr)
        menu->setParent(_service_menu);

    _service_menu->insertMenu(_service_menu->actions().constFirst(), menu);
}

void MainWindow::addMenu(const QList<QAction*> actions)
{
    for (auto a : actions) {
        Z_CHECK_NULL(a);
        if (a->parent() == nullptr)
            a->setParent(_service_menu);
    }

    _service_menu->insertActions(_service_menu->actions().constFirst(), actions);
}

bool MainWindow::isWorkZoneExists(int id) const
{
    return workZone(id, false) != nullptr;
}

QList<WorkZone*> MainWindow::workZones() const
{
    QList<WorkZone*> res;
    for (const auto &z : qAsConst(_work_zones_ordered)) {
        res << z.get();
    }
    return res;
}

WorkZone* MainWindow::workZone(int id, bool halt_if_not_found) const
{
    auto z = _work_zones.value(id);
    if (z == nullptr && halt_if_not_found)
        Z_HALT_INT;
    return z.get();
}

WorkZone* MainWindow::insertWorkZone(int before_id, int id, const QString& translation_id, const QIcon& icon, QWidget* widget,
                                     const QList<QToolBar*>& toolbars)
{
    Z_CHECK(workZone(id, false) == nullptr);
    auto z = std::shared_ptr<WorkZone>(new WorkZone(id, translation_id, widget, toolbars, icon));
    int pos;
    if (before_id >= 0) {
        auto before_z = _work_zones.value(before_id);
        Z_CHECK_NULL(before_z);
        pos = _work_zones_ordered.indexOf(before_z);
        Z_CHECK(pos >= 0);
    } else {
        pos = _work_zones_ordered.count();
    }

    _work_zones[id] = z;
    _work_zones_ordered.insert(pos, z);

    _work_zones_tab->insertTab(pos, z->icon(), z->name());
    if (_work_zones_ordered.count() == 1)
        currentWorkZoneChangeHelper(pos);

    emit sg_workZoneInserted(id);
    onWorkZonesChanged();

    return z.get();
}

QWidget* MainWindow::replaceWorkZoneWidget(int id, QWidget* w)
{
    auto z = _work_zones.value(id);
    Z_CHECK_NULL(z);
    auto replaced = z->replaceWidget(w);

    if (currentWorkZoneId() == id)
        _mdi_area->setBackgroundWidget(w);

    return replaced;
}

QList<QToolBar*> MainWindow::replaceWorkZoneToolbars(int id, const QList<QToolBar*>& toolbars)
{
    auto z = _work_zones.value(id);
    Z_CHECK_NULL(z);

    auto replaced = z->replaceToolbars(toolbars);
    if (replaced == toolbars)
        return {};

    if (!replaced.isEmpty()) {
        for (auto t : qAsConst(replaced)) {
            removeToolBar(t);
            t->setParent(this);
            t->setHidden(true);
        }
    }

    if (currentWorkZoneId() == id && !toolbars.isEmpty()) {
        for (auto t : qAsConst(replaced)) {
            insertToolBar(_ui->main_toolBar, t);
            t->setVisible(isWorkZoneEnabled(id));
        }
    }

    return replaced;
}

void MainWindow::removeWorkZone(int id)
{
    auto z = _work_zones.value(id);
    Z_CHECK_NULL(z);
    bool current_removed = (currentWorkZone() == z.get());

    Z_CHECK(_work_zones.remove(id));
    int old_pos = _work_zones_ordered.indexOf(z);
    Z_CHECK(old_pos >= 0);
    _work_zones_ordered.removeAt(old_pos);

    _work_zones_tab->removeTab(old_pos);

    if (current_removed) {
        int new_pos = -1;
        if (!_work_zones.isEmpty()) {
            if (old_pos == 0)
                new_pos = 0;
            else
                new_pos = old_pos - 1;

            while (new_pos >= 0) {
                if (new_pos >= _work_zones.count()) {
                    new_pos = -1;
                    break;
                }
                if (isWorkZoneEnabled(new_pos))
                    break;

                new_pos--;
            }
        }

        if (new_pos < 0) {
            currentWorkZoneChangeHelper(-1);
            emit sg_workZoneRemoved(id);
            onWorkZonesChanged();
            return;
        }

        setCurrentWorkZone(_work_zones_ordered.at(new_pos)->id());
    }

    emit sg_workZoneRemoved(id);
    onWorkZonesChanged();
}

WorkZone* MainWindow::setWorkZoneEnabled(int id, bool enabled)
{
    if (isWorkZoneEnabled(id) == enabled)
        return currentWorkZone();

    auto z = _work_zones.value(id);
    Z_CHECK_NULL(z);
    int pos = _work_zones_ordered.indexOf(z);
    _work_zones_tab->setTabEnabled(pos, enabled);
    // надо заново установить стили, чтобы подхватилось скрытие/отображение
    Utils::prepareTabBar(_work_zones_tab, true, 2);

    if (currentWorkZoneId() == id && !hasWorkZoneEnabled())
        currentWorkZoneChangeHelper(-1);

    emit sg_workZoneEnabledChanged(id, enabled);
    onWorkZonesChanged();

    return currentWorkZone();
}

bool MainWindow::isWorkZoneEnabled(int id) const
{
    auto z = _work_zones.value(id);
    if (z == nullptr)
        return false;

    int pos = _work_zones_ordered.indexOf(z);
    return _work_zones_tab->isTabEnabled(pos);
}

bool MainWindow::hasWorkZoneEnabled() const
{
    for (auto& z : qAsConst(_work_zones)) {
        if (isWorkZoneEnabled(z->id()))
            return true;
    }
    return false;
}

WorkZone* MainWindow::currentWorkZone() const
{
    return _current_work_zone.get();
}

int MainWindow::currentWorkZoneId() const
{
    return _current_work_zone == nullptr ? -1 : _current_work_zone->id();
}

WorkZone* MainWindow::setCurrentWorkZone(int id)
{
    if (!isWorkZoneEnabled(id))
        return nullptr;

    auto z = _work_zones.value(id);
    Z_CHECK_NULL(z);
    if (z == _current_work_zone)
        return z.get();

    int pos = _work_zones_ordered.indexOf(z);

    _work_zones_tab_changing = true;
    _work_zones_tab->setCurrentIndex(pos);
    _work_zones_tab_changing = false;

    currentWorkZoneChangeHelper(pos);
    return z.get();
}

#ifdef Q_OS_WIN
QWinTaskbarButton* MainWindow::taskbarButton() const
{
    return _taskbarButton;
}
#endif

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    // блокировка действий пользователя при обработке processEvents
    switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::HoverMove:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
            if (Utils::isProcessingEvent()) {
                event->ignore();
                return true;
            }
            break;
        default:
            break;
    }

    return QMainWindow::eventFilter(obj, event);
}

#define WINDOW_GEOMETRY_KEY "MAIN_WINDOW_GEOMETRY"
void MainWindow::showEvent(QShowEvent* event)
{
#ifdef Q_OS_WIN
    if (_taskbarButton == nullptr) {
        _taskbarButton = new QWinTaskbarButton(this);
        _taskbarButton->setWindow(windowHandle());
    }
#endif

    QMainWindow::showEvent(event);

    QByteArray saved_geometry = Core::fr()->coreConfiguration()->value(WINDOW_GEOMETRY_KEY).toByteArray();
    if (!saved_geometry.isNull())
        restoreGeometry(saved_geometry);
    else
        showMaximized();
}

void MainWindow::hideEvent(QHideEvent* event)
{
    Core::fr()->coreConfiguration()->setValue(WINDOW_GEOMETRY_KEY, saveGeometry());
    QMainWindow::hideEvent(event);
    emit sg_hidden();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (Core::windowManager()->askClose()) {
        event->accept();
        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

void MainWindow::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    if (message.messageCode() == MessageUpdateStatusBar::MSG_CODE_UPDATE_STATUS_BAR) {
        MessageUpdateStatusBar msg(message);
        _status_message->setText(msg.text());
    }
}

void MainWindow::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::MAIN_WINDOW_SPLITTER_KEY) {
        Core::fr()->configuration()->setValue(SPLITTER_CONFIG, QVariant::fromValue(_ui->windows_splitter->sizes()));
    }
}

void MainWindow::sl_openMdiWindow(View* view)
{
    Q_UNUSED(view)
}

void MainWindow::sl_closeMdiWindow(View* view)
{
    Q_UNUSED(view)
}

void MainWindow::sl_work_zones_tab_changed(int index)
{
    if (_work_zones_tab_changing || index < 0)
        return;

    Z_CHECK(index < _work_zones_ordered.count());
    setCurrentWorkZone(_work_zones_ordered.at(index)->id());
}

void MainWindow::configureStatusBar()
{
    _ui->statusbar->addWidget(_status_message, 1);
    _ui->statusbar->setStyleSheet("QStatusBar::item {border: None;}");
}

void MainWindow::configureMainToolbar()
{    
    _ui->main_toolBar->installEventFilter(this);
    Utils::prepareToolbar(_ui->main_toolBar, ToolbarType::Main);
}

void MainWindow::configureServiceToolbar()
{
    // кнопка "свернуть/развернуть окна"
    _a_min_max_windows = new QAction;
    _a_min_max_windows->setObjectName("zfminmax");
    updateWindowsActions();

    connect(_a_min_max_windows, &QAction::triggered, this, []() {
        if (Core::windowManager()->hasNotMimimized())
            Core::windowManager()->minimizeWindows();
        else
            Core::windowManager()->restoreWindows();
    });
    connect(Core::windowManager(), &WindowManager::sg_windowStateChanged, this, &MainWindow::updateWindowsActions);
    connect(Core::windowManager(), &WindowManager::sg_mdiWindowAddedToWorkZone, this, &MainWindow::updateWindowsActions);
    connect(Core::windowManager(), &WindowManager::sg_closeMdiWindow, this, &MainWindow::updateWindowsActions);
    _ui->service_toolBar->addAction(_a_min_max_windows);

    // меню "окна"
    _windows_menu = new QMenu;
    _windows_menu->setObjectName("zfwindowsmenu");

    _a_close_all = _windows_menu->addAction(QIcon(), ZF_TR(ZFT_CLOSE_ALL_WINDOWS));
    _a_close_all->setObjectName("zfcloseall");
    connect(_a_close_all, &QAction::triggered, [&]() { Core::windowManager()->closeAllWindows(); });

    _a_tile = _windows_menu->addAction(QIcon(), ZF_TR(ZFT_TILE_WINDOWS));
    _a_tile->setObjectName("zftile");
    connect(_a_tile, &QAction::triggered, [&]() { Core::windowManager()->tileWindows(); });

    _a_cascade = _windows_menu->addAction(QIcon(), ZF_TR(ZFT_CASCADE_WINDOWS));
    _a_cascade->setObjectName("zfcascade");
    connect(_a_cascade, &QAction::triggered, [&]() { Core::windowManager()->cascadeWindows(); });

    _a_minimize = _windows_menu->addAction(QIcon(), ZF_TR(ZFT_MINIMIZE_WINDOWS).simplified());
    _a_minimize->setObjectName("zfminimize");
    connect(_a_minimize, &QAction::triggered, [&]() { Core::windowManager()->minimizeWindows(); });

    _a_restore = _windows_menu->addAction(QIcon(), ZF_TR(ZFT_RESTORE_WINDOWS).simplified());
    _a_restore->setObjectName("zfrestore");
    connect(_a_restore, &QAction::triggered, [&]() { Core::windowManager()->restoreWindows(); });

    auto a_windows = Utils::createToolbarMenuAction(Core::toolbarButtonStyle(ToolbarType::Main), ZF_TR(ZFT_WINDOWS),
                                                    QIcon(":/share_icons/windows.svg"), _ui->service_toolBar, nullptr, _windows_menu,
                                                    "zf_windowsa", "zf_windowsb");
    a_windows->setToolTip(ZF_TR(ZFT_ALL_WINDOWS));

    // сервисное меню
    _service_menu = new QMenu;
    _service_menu->setObjectName("zfservicemenu");

    auto a_settings = _service_menu->addAction(QIcon(":/share_icons/blue/setting.svg"), ZF_TR(ZFT_SETTINGS) + "...");
    a_settings->setObjectName("zfsettings");
    connect(a_settings, &QAction::triggered, a_settings, [&]() {
        SettingsDialog dlg;
        dlg.exec();
    });

    auto a_changelog = _service_menu->addAction(QIcon(":/share_icons/changelog.svg"), ZF_TR(ZFT_CHANGELOG) + "...");
    a_changelog->setObjectName("zfchangelog");
    connect(a_changelog, &QAction::triggered, a_changelog, [&]() {
        zf::Core::info(ZF_TR(ZFT_CHANGELOG), zf::Core::changelog().isEmpty()? "нет информации" :  zf::Core::changelog());
    });

    auto a_about = _service_menu->addAction(QIcon(":/share_icons/blue/info.svg"), ZF_TR(ZFT_ABOUT));
    a_about->setObjectName("zfabout");
    connect(a_about, &QAction::triggered, a_about, [&]() {
        QString proxy_name = Core::getProxy().hostName();

        Utils::showAbout(
            this, qApp->applicationName(),
            QString("%1 %2\n"
                    "%3: %4, %5\n"
                    "%6 (%7)\n"
                    "%8: %9\n"
                    "%10: %11")
                .arg(qApp->applicationName(), Core::applicationVersion().text(3))
                .arg(ZF_TR(ZFT_REVISION), Core::buildVersion(), Utils::variantToString(Core::buildTime()))
                .arg(Core::connectionInformation()->userInformation().fio(), Core::connectionInformation()->userInformation().login())
                .arg(ZF_TR(ZFT_HOST), Core::connectionInformation()->host())
                .arg(ZF_TR(ZFT_PROXY_SERVER), proxy_name.isEmpty() ? ZF_TR(ZFT_NO).toLower() : proxy_name));
    });
    _service_menu->addSeparator();

    auto a_quit = _service_menu->addAction(QIcon(), ZF_TR(ZFT_QUIT));
    a_quit->setObjectName("zfquit");
    connect(a_quit, &QAction::triggered, a_quit, [&]() { close(); });

    auto a_service
        = Utils::createToolbarMenuAction(Core::toolbarButtonStyle(ToolbarType::Main), ZF_TR(ZFT_MISC), QIcon(":/share_icons/menu.svg"),
                                         _ui->service_toolBar, nullptr, _service_menu, "zf_servicea", "zf_serviceb");
    a_service->setToolTip(ZF_TR(ZFT_MISC_EXT));

    if (!_ui->service_toolBar->actions().isEmpty()) {
        // сдвигаем кнопки сервисного тулбара вправо
        QWidget* spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _ui->service_toolBar->insertWidget(_ui->service_toolBar->actions().constFirst(), spacer);
    }

    _ui->service_toolBar->installEventFilter(this);
    Utils::prepareToolbar(_ui->service_toolBar, ToolbarType::Main);

    updateWindowsActions();
}

void MainWindow::configureModuleToolbar()
{
    if (Core::fr()->isOperationMenuInstalled()) {
        QToolBar* toolbar = Core::operationMenuManager()->toolbar();        
        insertToolBar(_ui->service_toolBar, toolbar);
        Utils::prepareToolbar(toolbar, ToolbarType::Main);

        for (const auto& operation : Core::operationMenuManager()->operations()) {
            QToolButton* button = Core::operationMenuManager()->button(operation);
            if (button != nullptr)
                button->installEventFilter(this);
        }
    }
}

void MainWindow::currentWorkZoneChangeHelper(int pos)
{
    auto prev = _current_work_zone;
    int prev_id = (_current_work_zone == nullptr ? -1 : _current_work_zone->id());

    if (prev != nullptr) {
        for (auto t : prev->toolbars()) {
            removeToolBar(t);
            t->setParent(this);
            t->setHidden(true);
        }
    }

    if (pos < 0) {
        _mdi_area->setBackgroundWidget(nullptr);
        if (_current_work_zone != nullptr) {
            for (auto t : _current_work_zone->toolbars()) {
                removeToolBar(t);
                t->setParent(this);
                t->setHidden(true);
            }
        }

        _current_work_zone.reset();
        emit sg_currentWorkZoneChanged(prev_id, -1);
        return;
    }

    Z_CHECK(pos < _work_zones_ordered.count());
    auto z = _work_zones_ordered.at(pos);
    _current_work_zone = z;

    _mdi_area->setBackgroundWidget(z->widget());

    for (auto t : z->toolbars()) {
        insertToolBar(_ui->main_toolBar, t);
        t->setVisible(isWorkZoneEnabled(z->id()));
    }

    emit sg_currentWorkZoneChanged(prev_id, z->id());

    updateWindowsActions();
}

void MainWindow::onWorkZonesChanged()
{
    int count = 0;
    for (auto& z : qAsConst(_work_zones)) {
        int pos = _work_zones_ordered.indexOf(z);
        if (_work_zones_tab->isTabEnabled(pos))
            count++;
    }
    _work_zones_tab->setVisible(count > 1);
}

void MainWindow::updateWindowsActions()
{
    Z_CHECK_NULL(_a_min_max_windows);
    bool has_windows = !Core::windowManager()->windowsCurrentWorkZone().isEmpty();

    if (!has_windows || Core::windowManager()->hasNotMimimized()) {
        _a_min_max_windows->setIconText(ZF_TR(ZFT_WORKPLACE));
        _a_min_max_windows->setIcon(QIcon(":/share_icons/window_minimize.svg"));

    } else {
        _a_min_max_windows->setIconText(ZF_TR(ZFT_WORKPLACE));
        _a_min_max_windows->setIcon(QIcon(":/share_icons/window_restore.svg"));
    }

    _a_min_max_windows->setEnabled(has_windows);

    if (_windows_menu != nullptr) {        
        _a_tile->setEnabled(Core::windowManager()->hasNotMimimized());
        _a_cascade->setEnabled(Core::windowManager()->hasNotMimimized());
        _a_minimize->setEnabled(Core::windowManager()->hasNotMimimized());
        _a_restore->setEnabled(Core::windowManager()->hasMimimized());
        _windows_menu->menuAction()->setEnabled(has_windows);
    }
}

} // namespace zf
