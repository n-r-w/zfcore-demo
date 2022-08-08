#include "zf_window_manager.h"

#include "mdi_window/zf_module_window.h"
#include "modal_window/zf_modal_window.h"
#include "zf_core.h"
#include "zf_dbg_window.h"
#include "zf_framework.h"
#include "zf_main_window.h"
#include "zf_mdi_area.h"
#include "zf_translation.h"

#include <QDebug>
#include <QVBoxLayout>

namespace zf {
//! Последнее место размещения окна
QPoint WindowManager::_last_place = { -1, -1 };

WindowManager::WindowManager(MdiArea* mdi_area)
    : QObject()
    , _mdi_area(mdi_area)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_mdi_area);
    _mdi_area->viewport()->setObjectName("zfmv"); // криворукие дятлы из qt забыли присвоить ему имя
    _mdi_area->setActivationOrder(MdiArea::ActivationHistoryOrder);
    _mdi_area->setOption(MdiArea::DontMaximizeSubWindowOnActivation);

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");
    connect(Core::fr(), &Framework::sg_operationProcessed, this, &WindowManager::sl_operationProcessed);

    connect(Core::fr(), &Framework::sg_workZoneInserted, this, &WindowManager::sg_workZoneInserted);
    connect(Core::fr(), &Framework::sg_workZoneRemoved, this, &WindowManager::sg_workZoneRemoved);
    connect(Core::fr(), &Framework::sg_currentWorkZoneChanged, this, &WindowManager::sl_currentWorkZoneChanged);
    connect(Core::fr(), &Framework::sg_workZoneEnabledChanged, this, &WindowManager::sg_workZoneEnabledChanged);

    connect(qApp, &QApplication::applicationStateChanged, this, [&](Qt::ApplicationState state) {
        if (state == Qt::ApplicationState::ApplicationActive)
            fixWorkZoneViewFocus();
    });
}

WindowManager::~WindowManager()
{
    for (const auto& v : qAsConst(_work_zone_views)) {
        View::WindowConfiguration config;
        v->saveConfiguration(config);
    }

    delete _object_extension;
}

bool WindowManager::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void WindowManager::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant WindowManager::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool WindowManager::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void WindowManager::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void WindowManager::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void WindowManager::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void WindowManager::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void WindowManager::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

QList<ModuleWindow*> WindowManager::windows() const
{
    QList<ModuleWindow*> res;
    auto windows = _mdi_area->subWindowList();
    for (auto w : windows) {
        auto m_w = qobject_cast<ModuleWindow*>(w);
        if (m_w != nullptr && !m_w->isClosed() && m_w->view() != nullptr) {
            res << m_w;
        }
    }

    return res;
}

QList<ModuleWindow*> WindowManager::windowsCurrentWorkZone() const
{
    QList<ModuleWindow*> res;
    auto ws = windows();
    for (auto w : qAsConst(ws)) {
        QList<int> work_zones = w->workZoneIds();
        if (work_zones.contains(workZones()->currentWorkZoneId()))
            res << w;
    }
    return res;
}

Error WindowManager::openWindow(const Uid& uid, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    ModelPtr model;
    ViewPtr view;
    return openWindow(uid, model, view, options, code, custom_data);
}

Error WindowManager::openWindow(
    const Uid& uid, ModelPtr& model, ViewPtr& view, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK(uid.isValid());
    model.reset();

    Error error = checkAccessRights(uid.entityCode(), AccessFlag::View);
    if (error.isError())
        return error;

    auto windows = findModuleWindows(uid);
    if (!windows.isEmpty()) {
        // активация открытого окна
        openExistWindow(windows.first());
        model = windows.first()->view()->model();
        view = windows.first()->view();
        return Error();
    }

    QList<ModelPtr> models;
    MessageID feedback_id;
    error = Core::fr()->getModels(this, { uid }, {}, {},
        // не грузим свойства - представление запросит их само
        { false }, models, feedback_id);
    if (error.isError())
        return error;

    Z_CHECK(models.count() == 1);

    model = models.first();
    return openWindow(models.first(), view, options, code, custom_data);
}

Error WindowManager::openWindow(const ModelPtr& model, ViewPtr& view, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK_NULL(model);
    view.reset();

    Error error = checkAccessRights(model->entityCode(), AccessFlag::View);
    if (error.isError())
        return error;

    if (Utils::isModalWindow()) {
        viewWindow(model, error, options, code, custom_data);
        return error;
    }

    auto windows = findModuleWindows(model);
    if (!windows.isEmpty()) {
        // активация открытого окна
        openExistWindow(windows.first());
        view = windows.first()->view();
        return Error();
    }

    view = Core::createView<View>(model, ViewState::MdiWindow, code, error);
    if (error.isError())
        return error;

    view->setCustomData(custom_data);

    connect(view.get(), &View::sg_operationProcessed, this, &WindowManager::sl_operationProcessed);

    auto module_window = new ModuleWindow(view, _mdi_area, options);
    module_window->setObjectName(QStringLiteral("zfmw%1").arg(model->entityCode().value()));
    // исключаем падение счетчика view до 0
    _windows.insert(model->entityUid(), new ViewPtr(view));

    _mdi_area->addSubWindow(module_window);
    module_window->show(true);

    _last_place = module_window->geometry().topLeft();

    emit sg_openMdiWindow(view.get());

    connect(module_window, &ModuleWindow::windowStateChanged, this, &WindowManager::sl_windowStateChanged);
    connect(module_window, &ModuleWindow::sg_closed, this, &WindowManager::sl_closeMdiWindow);
    connect(module_window, &ModuleWindow::sg_entityChanged, this, &WindowManager::sl_entityChanged);
    connect(view.get(), &View::sg_progressEnd, this, &WindowManager::sl_progressEndMdiWindow);

    auto wz = Core::windowManager()->workZones()->currentWorkZone();
    if (wz != nullptr) {
        module_window->addToWorkZone(wz->id());
        emit sg_mdiWindowAddedToWorkZone(view.get());
    }

    return Error();
}

bool WindowManager::editWindow(const ModelPtr& model, Error& error, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK_NULL(model);

    error = checkAccessRights(model->entityCode(),
        // открываем с правами просмотра, т.к. за разрешение на изменения должен отвечать диалог
        AccessFlag::View);
    if (error.isError())
        return false;

    auto view = Core::createView<View>(model, ViewState::EditWindow, code, error);
    if (error.isError())
        return false;

    view->setCustomData(custom_data);

    ObjectExtensionPtr<ModalWindow> dialog = new ModalWindow(view.get(), options);
    int res = dialog->exec();
    error = dialog->operationError();

    return res == QDialogButtonBox::Cancel ? false : true;
}

bool WindowManager::editWindow(const Uid& uid, Error& error, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    ModelPtr model;
    return editWindow(uid, model, error, options, code, custom_data);
}

bool WindowManager::editWindow(
    const Uid& uid, ModelPtr& model, Error& error, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK((uid.isPersistent()));
    model.reset();

    error = checkAccessRights(uid.entityCode(),
        // открываем с правами просмотра, т.к. за разрешение на изменения должен отвечать диалог
        AccessFlag::View);
    if (error.isError())
        return false;

    model = Core::detachModel<Model>(uid, {}, DataPropertySet {}, error);
    if (error.isError())
        return false;

    return editWindow(model, error, options, code, custom_data);
}

bool WindowManager::editNewWindow(const EntityCode& entity_code, Error& error, const ModuleWindowOptions& options, int code, const DatabaseID& database_id,
    const QMap<QString, QVariant>& custom_data)
{
    ModelPtr model;
    return editNewWindow(entity_code, model, error, options, code, database_id, custom_data);
}

bool WindowManager::editNewWindow(const EntityCode& entity_code, ModelPtr& model, Error& error, const ModuleWindowOptions& options, int code,
    const DatabaseID& database_id, const QMap<QString, QVariant>& custom_data)
{
    model = zf::Core::createModel<zf::Model>(database_id.isValid() ? database_id : Core::defaultDatabase(), entity_code, error);
    if (error.isError())
        return false;

    return editWindow(model, error, options, code, custom_data);
}

bool WindowManager::editNewWindowHelper(const ViewPtr& view, const ModuleWindowOptions& options, const QMap<QString, QVariant>& custom_data, Error& error)
{
    Z_CHECK_NULL(view);
    Z_CHECK_NULL(view->model());

    view->setCustomData(custom_data);

    ObjectExtensionPtr<ModalWindow> dialog = new ModalWindow(view.get(), options);
    int res = dialog->exec();
    error = dialog->operationError();

    return res == QDialogButtonBox::Cancel ? false : true;
}

ViewPtr WindowManager::editNewWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)
    Z_CHECK_NULL(model);

    error = checkAccessRights(model->entityCode(),
        // открываем с правами просмотра, т.к. за разрешение на изменения должен отвечать диалог
        AccessFlag::View);
    if (error.isError())
        return nullptr;

    auto view = Core::createView<View>(model, ViewState::EditWindow, code, error);
    if (error.isError())
        return nullptr;

    return view;
}

ModelPtr WindowManager::editNewWindowHelperModelGet(const EntityCode& entity_code, const DatabaseID& database_id, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)

    return zf::Core::createModel<zf::Model>(database_id.isValid() ? database_id : Core::defaultDatabase(), entity_code, error);
}

ModelPtr WindowManager::editWindowHelperModelGet(const Uid& entity, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)
    Z_CHECK((entity.isPersistent()));

    error = checkAccessRights(entity.entityCode(),
        // открываем с правами просмотра, т.к. за разрешение на изменения должен отвечать диалог
        AccessFlag::View);
    if (error.isError())
        return nullptr;

    auto model = Core::detachModel<Model>(entity, {}, DataPropertySet {}, error);
    if (error.isError())
        return nullptr;

    return model;
}

ViewPtr WindowManager::editWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)
    return Core::createView<View>(model, ViewState::EditWindow, code, error);
}

bool WindowManager::editWindowHelper(const ViewPtr& view, const ModuleWindowOptions& options, const QMap<QString, QVariant>& custom_data, Error& error)
{
    Z_CHECK_NULL(view);
    Z_CHECK_NULL(view->model());

    view->setCustomData(custom_data);

    ObjectExtensionPtr<ModalWindow> dialog = new ModalWindow(view.get(), options);
    int res = dialog->exec();
    error = dialog->operationError();

    return res == QDialogButtonBox::Cancel ? false : true;
}

ModelPtr WindowManager::viewWindowHelperModelGet(const Uid& entity, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)
    Z_CHECK((entity.isPersistent()));

    error = checkAccessRights(entity.entityCode(), AccessFlag::View);
    if (error.isError())
        return nullptr;

    auto model = Core::detachModel<Model>(entity, {}, DataPropertySet {}, error);
    if (error.isError())
        return nullptr;

    return model;
}

ViewPtr WindowManager::viewWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error)
{
    Q_UNUSED(options)
    return Core::createView<View>(model, options.testFlag(ModuleWindowOption::ForceEditView)? ViewState::ViewEditWindow : ViewState::ViewWindow,
        code, error);
}

bool WindowManager::viewWindowHelper(const ViewPtr& view, const ModuleWindowOptions& options, const QMap<QString, QVariant>& custom_data, Error& error)
{
    Q_UNUSED(error);
    Z_CHECK_NULL(view);
    Z_CHECK_NULL(view->model());

    view->setCustomData(custom_data);
    view->setReadOnly(true);

    ObjectExtensionPtr<ModalWindow> dialog = new ModalWindow(view.get(), options);
    int res = dialog->exec();

    return res == QDialogButtonBox::Cancel ? false : true;
}

void WindowManager::fixWorkZoneViewFocus()
{
    if (!hasNotMimimized()) {
        _mdi_area->setActiveSubWindow(nullptr);
        View* view = currentEntityWorkZoneView();
        if (view != nullptr && view->lastFocused() != nullptr)
            view->lastFocused()->setFocus();
    }
}

bool WindowManager::viewWindow(const ModelPtr& model, Error& error, const ModuleWindowOptions& options, int code, const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK_NULL(model);

    error = checkAccessRights(model->entityCode(), AccessFlag::View);
    if (error.isError())
        return false;

    auto view = Core::createView<View>(model, options.testFlag(ModuleWindowOption::ForceEditView)? ViewState::ViewEditWindow : ViewState::ViewWindow,
        code, error);
    if (error.isError())
        return false;

    view->setCustomData(custom_data);
    view->setReadOnly(true);

    ObjectExtensionPtr<ModalWindow> dialog = new ModalWindow(view.get(), options);
    int res = dialog->exec();

    return res == QDialogButtonBox::Cancel ? false : true;
}

bool WindowManager::viewWindow(const Uid& uid, Error& error, const ModuleWindowOptions& options, int code)
{
    Z_CHECK((uid.isPersistent()));

    error = checkAccessRights(uid.entityCode(), AccessFlag::View);
    if (error.isError())
        return false;

    QList<ModelPtr> models;
    MessageID feedback_id;
    error = Core::fr()->getModels(this, { uid }, {}, {},
        // не грузим свойства - представление запросит их само
        { false }, models, feedback_id);
    if (error.isError())
        return false;

    Z_CHECK(models.count() == 1);

    return viewWindow(models.first(), error, options, code);
}

I_WorkZones* WindowManager::workZones() const
{
    Z_CHECK_NULL(Core::fr()->workZones());
    return Core::fr()->workZones();
}

int WindowManager::entityWorkZone(const Uid& uid) const
{
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        if (i.value()->entityUid() == uid)
            return i.key();
    }
    return -1;
}

Uid WindowManager::entityWorkZone(int id) const
{
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        if (i.key() == id && i.value()->entityUid().isValid())
            return i.value()->entityUid();
    }
    return Uid();
}

UidList WindowManager::entityWorkZones() const
{
    UidList res;
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        res << i.value()->entityUid();
    }
    return res;
}

QList<View*> WindowManager::entityWorkZoneViews() const
{
    QList<View*> res;
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        res << i.value().get();
    }
    return res;
}

View* WindowManager::entityWorkZoneView(const Uid& uid) const
{
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        if (i.value()->entityUid() == uid)
            return i.value().get();
    }
    return nullptr;
}

View* WindowManager::entityWorkZoneView(int id) const
{
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        if (i.key() == id && i.value()->entityUid().isValid())
            return i.value().get();
    }
    return nullptr;
}

bool WindowManager::isWorkZoneEnabled(const Uid& uid) const
{
    for (auto i = _work_zone_views.constBegin(); i != _work_zone_views.constEnd(); ++i) {
        if (i.value()->entityUid() == uid)
            return isWorkZoneEnabled(i.key());
    }
    return false;
}

bool WindowManager::isWorkZoneEnabled(int id) const
{
    return workZones()->isWorkZoneEnabled(id);
}

View* WindowManager::currentEntityWorkZoneView() const
{
    return _work_zone_views.value(workZones()->currentWorkZoneId()).get();
}

Error WindowManager::setEntityToWorkZone(int work_zone_id, const Uid& uid, const DataPropertyList& properties, int code)
{
    Error error = checkAccessRights(uid.entityCode(), AccessFlag::View);
    if (error.isError())
        return error;

    QList<ModelPtr> models;
    MessageID feedback_id;
    error = Core::fr()->getModels(this, { uid }, {}, { Utils::toSet(properties) },
        // не грузим свойства - представление запросит их само
        { false }, models, feedback_id);
    if (error.isError())
        return error;

    Z_CHECK(models.count() == 1);

    ViewPtr view = Core::createView<View>(models.at(0), ViewState::MdiWindow, code, error);
    if (error.isError())
        return error;

    connect(view.get(), &View::sg_operationProcessed, this, &WindowManager::sl_operationProcessed);

    // Устанавливаем UI модуля в рабочую зону
    QWidget* old_widget = workZones()->replaceWorkZoneWidget(work_zone_id, view->mainWidget());

    // Устанавливаем тулбары модуля в рабочую зону
    QList<QToolBar*> old_toolbars;
    QList<QToolBar*> new_toolbars;
    if (view->mainToolbar() != nullptr)
        new_toolbars << view->mainToolbar();

    new_toolbars << view->additionalToolbars();

    if (!new_toolbars.isEmpty()) {
        for (auto t : qAsConst(new_toolbars)) {
            Utils::prepareToolbar(t, ToolbarType::Main);
        }

        old_toolbars = workZones()->replaceWorkZoneToolbars(work_zone_id, new_toolbars);
    }

    view->loadConfiguration();

    if (_work_zone_views.contains(work_zone_id)) {
        _work_zone_views.value(work_zone_id)->saveConfiguration();
        _work_zone_views.remove(work_zone_id);

    } else {
        if (old_widget != nullptr)
            // виджет был установлен минуя этот метод
            delete old_widget;

        // тулбары были установлен минуя этот метод
        qDeleteAll(old_toolbars);
    }

    _work_zone_views[work_zone_id] = view;

    return Error();
}

Error WindowManager::setEntityToWorkZone(int work_zone_id, const Uid& uid, const QList<PropertyID>& properties, int code)
{
    Z_CHECK(uid.isValid());
    DataPropertyList props;
    for (auto& p : properties) {
        props << DataProperty::property(uid, p);
    }
    return setEntityToWorkZone(work_zone_id, uid, props, code);
}

QWidget* WindowManager::setMdiBackground(QWidget* w)
{
    return _mdi_area->setBackgroundWidget(w);
}

bool WindowManager::askClose()
{
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    return true;
#else // QT_DEBUG
    return Core::ask(ZF_TR(ZFT_ASK_APP_QUIT));
#endif // QT_DEBUG
}

void WindowManager::closeActiveWindow()
{
    _mdi_area->closeActiveSubWindow();
}

void WindowManager::closeAllWindows()
{
    // закрываем только окна приложений
    for (auto w : windows()) {
        if (w->workZoneIds().isEmpty() || (w->workZoneIds().count() == 1 && w->workZoneIds().contains(workZones()->currentWorkZoneId()))) {
            w->close();
        } else if ((w->workZoneIds().count() > 1 && w->workZoneIds().contains(workZones()->currentWorkZoneId()))) {
            w->removeFromWorkZone(workZones()->currentWorkZoneId());
        }
    }
}

void WindowManager::tileWindows()
{
    beforeRearange();
    _mdi_area->tileSubWindows();
    afterRearange();
}

void WindowManager::cascadeWindows()
{
    beforeRearange();
    _mdi_area->cascadeSubWindows();
    afterRearange();
}

void WindowManager::activateNextWindow()
{
    _mdi_area->activateNextSubWindow();
}

void WindowManager::activatePreviousWindow()
{
    _mdi_area->activatePreviousSubWindow();
}

void WindowManager::minimizeWindows()
{
    for (auto w : windows()) {
        if (w->isMinimized() || !w->isVisible())
            continue;

        QList<int> work_zones = w->workZoneIds();
        if (!work_zones.contains(workZones()->currentWorkZoneId()))
            continue;

        w->showMinimized();
    }
    fixWorkZoneViewFocus();
}

void WindowManager::restoreWindows()
{
    for (auto w : windows()) {
        if (!w->isMinimized() || !w->isVisible())
            continue;

        QList<int> work_zones = w->workZoneIds();
        if (!work_zones.contains(workZones()->currentWorkZoneId()))
            continue;

        w->showNormal();
    }
}

bool WindowManager::hasNotMimimized() const
{
    for (auto w : windows()) {
        QList<int> work_zones = w->workZoneIds();
        if (!w->isMinimized() && w->isVisible() && work_zones.contains(workZones()->currentWorkZoneId()))
            return true;
    }
    return false;
}

bool WindowManager::hasMimimized() const
{
    for (auto w : windows()) {
        QList<int> work_zones = w->workZoneIds();
        if (w->isMinimized() && w->isVisible() && work_zones.contains(workZones()->currentWorkZoneId()))
            return true;
    }
    return false;
}

void WindowManager::openDebugWindow()
{
    auto debug = new DebugWindow();
    _mdi_area->addSubWindow(debug);
    debug->show();
}

QPoint WindowManager::lastPlace()
{
    return _last_place;
}

void WindowManager::sl_windowStateChanged(Qt::WindowStates old_state, Qt::WindowStates new_state)
{
    // окно иногда "уезжает" под фоновый виджет, поэтому надо опустить фоновый виджет вниз
    if (_mdi_area->backgroundWidget() != nullptr)
        _mdi_area->backgroundWidget()->lower();

    auto w = qobject_cast<ModuleWindow*>(sender());
    if (w == nullptr)
        return; // это может быть если сигнал среагировал на удаление окна и вызвался из деструктора

    emit sg_windowStateChanged(w, w->view().get(), old_state, new_state);
}

void WindowManager::sl_closeMdiWindow(const Uid& entity_uid)
{
    ModuleWindow* window = qobject_cast<ModuleWindow*>(sender());
    Z_CHECK_NULL(window);

    Z_CHECK(_windows.contains(entity_uid));
    emit sg_closeMdiWindow(window->view().get());

    // переносим указатель в _remove_requests
    auto view_ptr = findView(entity_uid, window->view().get());
    Z_CHECK_NULL(view_ptr);

    requestRemove(*view_ptr);

    if (windows().isEmpty())
        _last_place = { -1, -1 };
}

void WindowManager::sl_progressEndMdiWindow()
{
    if (!Core::isBootstraped() || objectExtensionDestroyed())
        return;

    View* view = qobject_cast<View*>(sender());

    if (_remove_requests.contains(view->entityUid()))
        Framework::internalCallbackManager()->addRequest(this, Framework::WINDOW_MANAGER_DELETE_LATER_KEY);
}

void WindowManager::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::WINDOW_MANAGER_DELETE_LATER_KEY) {
        processRemove();
    }
}

void WindowManager::sl_operationProcessed(const Operation& operation, const Error& error)
{
    Q_UNUSED(operation)

    if (error.isError())
        Core::error(error);
}

void WindowManager::sl_entityChanged(const Uid& old_entity, const Uid& new_entity)
{
    ModuleWindow* window = qobject_cast<ModuleWindow*>(sender());
    Z_CHECK_NULL(window);

    auto view = findView(old_entity, window->view().get());
    Z_CHECK_NULL(view);

    Z_CHECK(_windows.remove(old_entity, view) == 1);
    _windows.insert(new_entity, view);
}

void WindowManager::sl_currentWorkZoneChanged(int previous_id, int current_id)
{
    auto view = _work_zone_views.value(previous_id);
    if (view != nullptr) {
        view->model()->onActivatedInternal(false);
        view->onActivatedInternal(false);
    }

    view = _work_zone_views.value(current_id);
    if (view != nullptr) {
        view->model()->onActivatedInternal(true);
        view->onActivatedInternal(true);
    }

    emit sg_currentWorkZoneChanged(previous_id, current_id);

    fixWorkZoneViewFocus();
}

void WindowManager::openExistWindow(ModuleWindow* w)
{
    Z_CHECK_NULL(w);
    Z_CHECK(_mdi_area->subWindowList().contains(w));

    if (workZones()->currentWorkZone() != nullptr) {
        if (!w->workZoneIds().contains(workZones()->currentWorkZoneId()))
            w->addToWorkZone(workZones()->currentWorkZoneId());
    }

    w->show(true);
    _mdi_area->setActiveSubWindow(w);
}

QList<ModuleWindow*> WindowManager::findModuleWindows(const Uid& entity_uid) const
{
    QList<ModuleWindow*> res;

    auto windows = _mdi_area->subWindowList();
    for (auto w : windows) {
        auto m_w = qobject_cast<ModuleWindow*>(w);
        if (m_w != nullptr && !m_w->isClosed() && m_w->view() != nullptr) {
            if ((m_w->view()->entityUid().isTemporary() && entity_uid.isTemporary()) || m_w->view()->entityUid() == entity_uid)
                res << m_w;
        }
    }
    return res;
}

QList<ModuleWindow*> WindowManager::findModuleWindows(const ModelPtr& model) const
{
    Z_CHECK_NULL(model);
    QList<ModuleWindow*> res = findModuleWindows(model->entityUid());
    for (int i = res.count() - 1; i >= 0; i++) {
        if (res.at(i)->view()->model() != model)
            res.removeAt(i);
    }

    return res;
}

ModuleWindow* WindowManager::activeWindow() const
{
    return qobject_cast<ModuleWindow*>(_mdi_area->activeSubWindow());
}

void WindowManager::setNoActiveWindow()
{
    _mdi_area->setActiveSubWindow(nullptr);
}

void WindowManager::invalidateVisibleWindows()
{
    if (currentEntityWorkZoneView() != nullptr)
        currentEntityWorkZoneView()->invalidateUsingModels();

    auto windows = windowsCurrentWorkZone();
    for (auto w : qAsConst(windows)) {
        if (w->view() == nullptr)
            continue;

        w->view()->invalidateUsingModels();
    }
}

void WindowManager::requestRemove(ViewPtr view)
{
    if (objectExtensionDestroyed())
        return;

    _remove_requests.insert(view->entityUid(), new ViewPtr(view));

    Uid uid = view->entityUid();
    ViewPtr* view_saved = findView(uid, view.get());
    Z_CHECK_NULL(view_saved);
    _windows.remove(uid, view_saved);
    delete view_saved;

    if (!view->isProgress())
        Framework::internalCallbackManager()->addRequest(this, Framework::WINDOW_MANAGER_DELETE_LATER_KEY);
}

void WindowManager::processRemove()
{
    auto copy = _remove_requests;
    for (auto i = copy.constBegin(); i != copy.constEnd(); ++i) {
        ViewPtr* view = i.value();
        if ((*view)->isProgress())
            continue;

        _remove_requests.remove(i.key(), i.value());
        delete view;
    }
}

Error WindowManager::checkAccessRights(const EntityCode& entity_code, const AccessRights& rights) const
{
    if (!Core::connectionInformation()->directPermissions().hasRight(entity_code, rights))
        return Error(ZF_TR(ZFT_NO_ACCESS_RIGHTS).arg(Core::getModuleInfo(entity_code).name()));
    return Error();
}

ViewPtr* WindowManager::findView(const Uid& uid, View* view) const
{
    for (auto it = _windows.constBegin(); it != _windows.constEnd(); ++it) {
        if (it.key() == uid && it.value()->get() == view) {
            return it.value();
        }
    }
    return nullptr;
}

void WindowManager::beforeRearange()
{
}

void WindowManager::afterRearange()
{
}

} // namespace zf
