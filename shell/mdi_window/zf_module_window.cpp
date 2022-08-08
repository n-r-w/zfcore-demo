#include "zf_module_window.h"

#include "ui_zf_module_window.h"
#include "zf_core.h"
#include "zf_framework.h"
#include "zf_work_zones.h"
#include "zf_colors_def.h"

namespace zf
{
ModuleWindow::ModuleWindow(const std::weak_ptr<View>& view_ptr, MdiArea* mdi_area, const ModuleWindowOptions& options)
    : QMdiSubWindow(mdi_area)
    , _ui(new Ui::ModuleWindow)
    , _view(view_ptr)
    , _mdi_area(mdi_area)
    , _options(options)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(!view_ptr.expired());
    Z_CHECK_NULL(mdi_area);

    auto view = view_ptr.lock();

    connect(this, &ModuleWindow::windowStateChanged, this, &ModuleWindow::sl_windowStateChanged);
    connect(Core::windowManager(), &WindowManager::sg_workZoneRemoved, this, &ModuleWindow::sl_workZoneRemoved);
    connect(Core::windowManager(), &WindowManager::sg_currentWorkZoneChanged, this, &ModuleWindow::sl_currentWorkZoneChanged);

    _entity_uid = view->entityUid();

    //    Core::logInfo(QString("mdi window start load: %1").arg(view->entityUid().toPrintable()));

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    setAcceptDrops(true);
    setWindowIcon(view->moduleInfo().icon());

    QWidget* body = new QWidget();
    body->setObjectName(QStringLiteral("zfbody"));
    _ui->setupUi(body);

    // невидимая кнопка для корректного закрытия MDI окна через UI Automaton
    connect(_ui->zfclose, &QToolButton::clicked, this, [&]() { close(); });

    _ui->search_frame->setStyleSheet(QStringLiteral("QFrame {border: 1px solid %1; }").arg(Colors::uiLineColor(false).name()));
    _ui->search_next->setToolTip(ZF_TR(ZFT_NEXT));
    _ui->search_prev->setToolTip(ZF_TR(ZFT_PREV));

    _ui->search_edit->setPlaceholderText(ZF_TR(ZFT_SEARCH));
    _ui->search_edit->installEventFilter(this);

    connect(_ui->search_prev, &QToolButton::clicked, this, [&]() {
        if (this->view()->currentDataset().isValid())
            this->view()->searchTextPrevious(this->view()->currentDataset(), _ui->search_edit->text().trimmed());
    });
    connect(_ui->search_next, &QToolButton::clicked, this, [&]() {
        if (this->view()->currentDataset().isValid())
            this->view()->searchTextNext(this->view()->currentDataset(), _ui->search_edit->text().trimmed());
    });
    connect(view.get(), &View::sg_operationProcessed, this, &ModuleWindow::sl_operationProcessed);

    _ui->body_la->addWidget(view->mainWidget());
    view->mainWidget()->setHidden(false);

    _ui->_UI_ZFT_REMOVED_FROM_DB->setHidden(true);

    layout()->setMargin(0);
    layout()->setSpacing(0);

    Z_CHECK(Core::messageDispatcher()->registerObject(view->entityUid(), this, QStringLiteral("sl_message_dispatcher_inbound")));
    _remove_handle = Core::messageDispatcher()->subscribe(CoreChannels::ENTITY_REMOVED, this, {}, {view->entityUid()});

    connectToView();
    connectToModel();

    setWidget(body);

    sl_entityNameChanged();

    // откладываем отрисовку тулбара, чтобы не было "мельтешения" видимости/доступности
    if (!_toolbar_created) {
        _toolbar_created = true;
        _ui->toolbar_widget->setUpdatesEnabled(false);
        createToolbar();
        // шлем запрос на отображение тулбара
        Framework::internalCallbackManager()->addRequest(this, Framework::MODULE_WINDOW_CREATE_TOOLBAR_KEY);
    }

    // отладочное меню
    systemMenu()->addSeparator();
    QMenu* _debug_menu = new QMenu("debug");
    systemMenu()->addMenu(_debug_menu);

    QAction* a_model_deb_print = new QAction("Model debPrint");
    connect(a_model_deb_print, &QAction::triggered, this, [&]() {
        if (this->view() != nullptr)
            Core::debPrint(this->view()->model().get());
    });
    _debug_menu->addAction(a_model_deb_print);
    QAction* a_view_deb_print = new QAction("View debPrint");
    connect(a_view_deb_print, &QAction::triggered, this, [&]() {
        if (this->view() != nullptr)
            Core::debPrint(this->view().get());
    });
    _debug_menu->addAction(a_view_deb_print);

    view->setModuleWindowInterface(this);
}

ModuleWindow::~ModuleWindow()
{
    _destroying = true;

    if (Core::isBootstraped() && view() != nullptr) {
        removeToolbars();
        view()->mainWidget()->setHidden(false);
        view()->mainWidget()->setParent(nullptr);
    }
    delete _ui;
    delete _object_extension;

    //    Core::logInfo(QString("mdi window closed: %1").arg(_entity_uid.toPrintable()));
}

bool ModuleWindow::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void ModuleWindow::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant ModuleWindow::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool ModuleWindow::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void ModuleWindow::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void ModuleWindow::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void ModuleWindow::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void ModuleWindow::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void ModuleWindow::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

ViewPtr ModuleWindow::view() const
{
    return _view.expired() ? nullptr : _view.lock();
}

ModelPtr ModuleWindow::model() const
{
    return view() != nullptr ? view()->model() : nullptr;
}

bool ModuleWindow::isClosed() const
{
    return _closed;
}

QList<int> ModuleWindow::workZoneIds() const
{
    return _work_zone_ids;
}

void ModuleWindow::addToWorkZone(int work_zone_id)
{
    Z_CHECK(!_work_zone_ids.contains(work_zone_id));
    _work_zone_ids << work_zone_id;

    if (Core::windowManager()->workZones()->currentWorkZoneId() == work_zone_id && !isVisible())
        show(false);

    emit sg_addedToWorkZone(work_zone_id);
}

void ModuleWindow::removeFromWorkZone(int work_zone_id)
{
    Z_CHECK(_work_zone_ids.removeOne(work_zone_id));

    if (_work_zone_ids.isEmpty() && !isVisible())
        show(false);
    else if (Core::windowManager()->workZones()->currentWorkZoneId() == work_zone_id && isVisible())
        hide();

    emit sg_removedFromWorkZone(work_zone_id);
}

QSize ModuleWindow::sizeHint() const
{
    QSize max_size = mdiSize();
    max_size.setWidth(max_size.width() / 2);
    max_size.setHeight(max_size.height() / 2);

    QSize size = QMdiSubWindow::sizeHint();
    size.setWidth(qMax(mdiSize().width() / 3, size.width()));
    size.setHeight(qMax(mdiSize().height() / 3, size.height()));

    size.setWidth(qMin(max_size.width(), size.width()));
    size.setHeight(qMin(max_size.height(), size.height()));

    return size;
}

QSize ModuleWindow::minimumSizeHint() const
{
    return QMdiSubWindow::minimumSizeHint();
}

void ModuleWindow::closeEvent(QCloseEvent* closeEvent)
{
    if (_work_zone_ids.count() > 1) {
        removeFromWorkZone(Core::windowManager()->workZones()->currentWorkZoneId());
        closeEvent->ignore();
        return;
    }

    QMdiSubWindow::closeEvent(closeEvent);

    if (Core::isBootstraped() && view() != nullptr) {
        view()->mainWidget()->setHidden(true);
        view()->mainWidget()->setParent(nullptr);

        _closed = true;
        emit sg_closed(view()->entityUid());
    }
}

void ModuleWindow::showEvent(QShowEvent* showEvent)
{
    QMdiSubWindow::showEvent(showEvent);

    if (view() != nullptr && !loadConfiguration()) {
        requestResize(QSize(view()->moduleUI()->baseSize().width() > 0 ? view()->moduleUI()->baseSize().width() : width(),
            view()->moduleUI()->baseSize().height() > 0 ? view()->moduleUI()->baseSize().height() : height()));
    }

    checkSearchEnabled();
}

void ModuleWindow::hideEvent(QHideEvent* hideEvent)
{
    saveConfiguration();
    QMdiSubWindow::hideEvent(hideEvent);
}

void ModuleWindow::resizeEvent(QResizeEvent* resizeEvent)
{
    QMdiSubWindow::resizeEvent(resizeEvent);
    requestSaveConfiguration();
}

void ModuleWindow::moveEvent(QMoveEvent* moveEvent)
{
    QMdiSubWindow::moveEvent(moveEvent);
}

void ModuleWindow::keyPressEvent(QKeyEvent* e)
{
    QMdiSubWindow::keyPressEvent(e);

    if (view() != nullptr && e->modifiers().testFlag(Qt::ShiftModifier) && e->modifiers().testFlag(Qt::AltModifier) && e->key() == Qt::Key_F12) {
        // тестовый просмотр содержимого данных
        zf::Utils::debugDataShow(view()->model());
    }
}

void ModuleWindow::focusOutEvent(QFocusEvent* focusOutEvent)
{
    QMdiSubWindow::focusOutEvent(focusOutEvent);

    saveConfiguration();

    if (view() != nullptr) {
        view()->model()->onActivatedInternal(false);
        view()->onActivatedInternal(false);
    }
}

void ModuleWindow::focusInEvent(QFocusEvent* focusInEvent)
{
    QMdiSubWindow::focusInEvent(focusInEvent);

    loadConfiguration();

    if (view() != nullptr) {
        view()->model()->onActivatedInternal(true);
        view()->onActivatedInternal(true);
    }
}

void ModuleWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (view() != nullptr && view()->onDragEnterHelper(this, event))
        return;

    QMdiSubWindow::dragEnterEvent(event);
}

void ModuleWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if (view() != nullptr && view()->onDragMoveHelper(this, event))
        return;

    QMdiSubWindow::dragMoveEvent(event);
}

void ModuleWindow::dropEvent(QDropEvent* event)
{
    if (view() != nullptr && view()->onDropHelper(this, event))
        return;

    QMdiSubWindow::dropEvent(event);
}

void ModuleWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    if (view() != nullptr && view()->onDragLeaveHelper(this, event))
        return;

    QMdiSubWindow::dragLeaveEvent(event);
}

void ModuleWindow::sl_windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
{
    Q_UNUSED(oldState)
    Q_UNUSED(newState)
}

void ModuleWindow::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (_remove_handle == subscribe_handle) {
    }
}

void ModuleWindow::sl_existsChanged()
{
    if (!view()->isExists())
        close();
}

void ModuleWindow::sl_entityNameChanged()
{
    if (view() == nullptr)
        return;

    QString name = view()->entityName();
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    name += " [" + view()->entityUid().toPrintable() + "]";
#endif

    setWindowTitle(name);
}

void ModuleWindow::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::MODULE_WINDOW_CREATE_TOOLBAR_KEY) {
        _ui->toolbar_widget->setUpdatesEnabled(true);

    } else if (key == Framework::MODULE_WINDOW_RESIZE_KEY) {
        resizeHelper();

    } else if (key == Framework::MODULE_WINDOW_SAVE_CONFIG_KEY) {
        saveConfiguration();
    }
}

void ModuleWindow::sl_searchWidgetEdited(const Uid& entity)
{
    Z_CHECK(entity.isValid());
    Z_CHECK(_entity_uid != entity);

    auto old_entity = _entity_uid;
    _entity_uid = entity;

    disconnectFromModel();
    zf::Error error;
    auto model = Core::getModelFull<Model>(entity, error);
    if (error.isError()) {
        Core::error(error);
        return;
    }

    view()->setModel(model);
    connectToModel();

    emit sg_entityChanged(old_entity, entity);
}

void ModuleWindow::sl_workZoneRemoved(int id)
{
    if (!_work_zone_ids.removeOne(id))
        return;

    if (_work_zone_ids.isEmpty() && !isVisible())
        show(false);
}

void ModuleWindow::sl_currentWorkZoneChanged(int, int current_id)
{
    if (_work_zone_ids.contains(current_id)) {
        if (!isVisible())
            show(false);
    } else {
        if (!_work_zone_ids.isEmpty() && isVisible())
            hide();
    }
}

void ModuleWindow::sl_fieldVisibleChanged(const DataProperty& property, QWidget* widget)
{
    Q_UNUSED(widget)

    if (property.propertyType() == PropertyType::Dataset)
        checkSearchEnabled();
}

void ModuleWindow::sl_model_finishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error);
    Q_UNUSED(load_options);
    Q_UNUSED(properties);
}

void ModuleWindow::sl_operationProcessed(const Operation& op, const Error& error)
{
    Q_UNUSED(op);
    Q_UNUSED(error);
}

bool ModuleWindow::loadConfiguration()
{
    bool resized = false;
    if (Core::isActive() && view() != nullptr) {
        View::WindowConfiguration config;
        view()->loadConfiguration(config);
        if (!_size_initialized && config.size.isValid()) {
            if ((windowState() & (Qt::WindowMaximized | Qt::WindowMinimized)) == 0) {
                requestResize(config.size);
            }
            resized = true;
        }
    }

    _size_initialized = true;
    return resized;
}

void ModuleWindow::saveConfiguration()
{
    if (Core::isActive() && view() != nullptr) {
        View::WindowConfiguration config;
        if (windowState() & Qt::WindowMinimized) {
        } else if (windowState() & Qt::WindowMaximized) {
        } else {
            config.size = size();
        }

        view()->saveConfiguration(config);

        _size_initialized = true;
    }
}

void ModuleWindow::requestSaveConfiguration()
{
    if (!objectExtensionDestroyed())
        Framework::internalCallbackManager()->addRequest(this, Framework::MODULE_WINDOW_SAVE_CONFIG_KEY);
}

void ModuleWindow::connectToView()
{
    connect(view().get(), &View::sg_toolbarAdded, this, [&]() { createToolbar(); });
    connect(view().get(), &View::sg_toolbarRemoved, this, [&]() { createToolbar(); });

    if (searchDataset().isValid()) {
        connect(view().get(), &View::sg_activeControlChanged, this, [&]() { checkSearchEnabled(); });
    }

    connect(view().get(), &View::sg_entityNameChanged, this, &ModuleWindow::sl_entityNameChanged);
    connect(view().get(), &View::destroyed, this, [&]() { objectExtensionDestroy(); });
    connect(view().get(), &View::sg_searchWidgetEdited, this, &ModuleWindow::sl_searchWidgetEdited);
    connect(view().get(), &View::sg_fieldVisibleChanged, this, &ModuleWindow::sl_fieldVisibleChanged);
}

void ModuleWindow::connectToModel()
{
    connect(view()->model().get(), &Model::sg_existsChanged, this, &ModuleWindow::sl_existsChanged);
    //    connect(view()->model().get(), &Model::sg_entityNameChanged, this, &ModuleWindow::sl_entityNameChanged);
    connect(view()->model().get(), &Model::sg_finishLoad, this, &ModuleWindow::sl_model_finishLoad);
}

void ModuleWindow::disconnectFromModel()
{
    disconnect(view()->model().get(), &Model::sg_existsChanged, this, &ModuleWindow::sl_existsChanged);
    //    disconnect(view()->model().get(), &Model::sg_entityNameChanged, this, &ModuleWindow::sl_entityNameChanged);
    disconnect(view()->model().get(), &Model::sg_finishLoad, this, &ModuleWindow::sl_model_finishLoad);
}

void ModuleWindow::createToolbar()
{
    if (_destroying || view() == nullptr || objectExtensionDestroyed())
        return;

    removeToolbars();

    bool has_toolbars = !view()->isToolbarsHidden() && view()->configureToolbarsLayout(_ui->toolbar_la);

    _ui->search_edit->setMaximumWidth(Utils::stringWidth(QString(10, 'O')));

    if (has_toolbars || needSearch())
        Framework::internalCallbackManager()->addRequest(this, Framework::MODULE_WINDOW_RESIZE_KEY, false);
}

void ModuleWindow::removeToolbars()
{
    while (_ui->toolbar_la->count() > 0) {
        QLayoutItem* item = _ui->toolbar_la->takeAt(0);
        if (item->widget() != nullptr) {
            QToolBar* toolbar = qobject_cast<QToolBar*>(item->widget());
            if (toolbar != nullptr)
                toolbar->setParent(nullptr);
            else
                delete item->widget();

        } else if (item->layout() != nullptr) {
            delete item->layout();

        } else if (item->spacerItem() != nullptr) {
            delete item->spacerItem();
        }
        delete item;
    }
}

void ModuleWindow::checkSearchEnabled()
{
    if (view() == nullptr || !needSearch()) {
        _ui->search_widget->setHidden(true);
        return;
    }

    auto ds_view = view()->object<QAbstractItemView>(searchDataset());
    _ui->search_widget->setEnabled(Utils::isVisible(ds_view));
    _ui->search_widget->setVisible(ds_view->isVisible());
}

void ModuleWindow::resizeHelper()
{
    // борьба с багом QMdiSubWindow, которое не расширяет свой размер несмотря на изменение minimumSizeHint
    QSize size_hint = minimumSize().isEmpty() ? minimumSizeHint() : minimumSize();
    size_hint.setWidth(qMax(size_hint.width(), size().width()));
    size_hint.setHeight(qMax(size_hint.height(), size().height()));
    size_hint.setWidth(qMin(size_hint.width(), mdiArea()->geometry().width()));
    size_hint.setHeight(qMin(size_hint.height(), mdiArea()->geometry().height()));

    if (size_hint != size())
        resize(size_hint);
}

void ModuleWindow::requestResize(const QSize& new_size)
{
    if (_size_initialized || objectExtensionDestroyed())
        return;

    QSize size_hint = minimumSize().isEmpty() ? minimumSizeHint() : minimumSize();
    size_hint.setWidth(qMax(size_hint.width(), new_size.width()));
    size_hint.setHeight(qMax(size_hint.height(), new_size.height()));
    size_hint.setWidth(qMin(size_hint.width(), mdiSize().width()));
    size_hint.setHeight(qMin(size_hint.height(), mdiSize().height()));

    resize(size_hint);

    placeWindow();

    Framework::internalCallbackManager()->addRequest(this, Framework::MODULE_WINDOW_RESIZE_KEY, false);
}

void ModuleWindow::placeWindow()
{
    const int shift = qMax(mdiArea()->width() / 70, 10);
    QPoint place;

    if (WindowManager::lastPlace().x() < 0)
        place = QPoint(0, 0);
    else
        place = {WindowManager::lastPlace().x() + shift, WindowManager::lastPlace().y() + shift};

    if (place.x() + width() > mdiSize().width() || place.y() + height() > mdiSize().height())
        place = QPoint(0, 0);

    move(place);
}

bool ModuleWindow::needSearch() const
{
    return view() != nullptr && searchDataset().isValid();
}

DataProperty ModuleWindow::searchDataset() const
{
    auto datasets = view()->structure()->propertiesByType(PropertyType::Dataset);
    for (int i = datasets.count() - 1; i >= 0; i--) {
        if (datasets.at(i).options().testFlag(PropertyOption::SimpleDataset))
            datasets.removeAt(i);
    }

    if (datasets.count() != 1)
        return DataProperty();

    QWidget* ds_widget = qobject_cast<QAbstractItemView*>(view()->widgets()->field(datasets.constFirst(), false));
    if (ds_widget == nullptr)
        return DataProperty();

    return datasets.constFirst();
}

void ModuleWindow::show(bool raise)
{
    if (_mdi_area->activeSubWindow() != nullptr && (_mdi_area->activeSubWindow()->windowState() & Qt::WindowMaximized)) {
        showMaximized();
        return;
    }

    if (raise) {
        showNormal();

    } else {
        if (windowState().testFlag(Qt::WindowState::WindowMinimized))
            showMinimized();
        else
            showNormal();
    }
}

void ModuleWindow::setVisible(bool b)
{
    bool old = isVisible();
    QMdiSubWindow::setVisible(b);

    if (old != b) {
        // Борьба с багом QMdiArea при котором скрытие/отображение окна не приводит к обновлению скролбаров
        QCoreApplication::postEvent(_mdi_area->viewport(), new QResizeEvent(_mdi_area->viewport()->size(), _mdi_area->viewport()->size()));
    }
}

ModuleWindowOptions ModuleWindow::options() const
{
    return _options;
}

bool ModuleWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == _ui->search_edit && event->type() == QEvent::KeyPress) {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        if (e->modifiers() == Qt::NoModifier) {
            if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
                _ui->search_next->click();
            } else if (e->key() == Qt::Key_Down || e->key() == Qt::Key_Up) {
                if (view()->currentDataset().isValid()) {
                    qApp->sendEvent(view()->object<QObject>(view()->currentDataset()), event);
                }
            }
        }
    }

    return QMdiSubWindow::eventFilter(object, event);
}

QSize ModuleWindow::mdiSize() const
{
    QSize s = mdiArea()->size();
    int scroll_size = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    s.setWidth(s.width() - scroll_size);
    s.setHeight(s.height() - scroll_size);
    return s;
}

} // namespace zf
