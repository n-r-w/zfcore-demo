#include "zf_view.h"
#include "zf_child_objects_container.h"
#include "zf_core_messages.h"
#include "zf_data_widgets.h"
#include "zf_framework.h"
#include "zf_highlight_mapping.h"
#include "zf_highlight_model.h"
#include "zf_image_list.h"
#include "zf_itemview_header_item.h"
#include "zf_modal_window.h"
#include "zf_objects_finder.h"
#include "zf_picture_selector.h"
#include "zf_proxy_item_model.h"
#include "zf_translation.h"
#include "zf_translator.h"
#include "zf_ui_size.h"
#include "zf_widget_highlighter.h"
#include "zf_widget_highligter_controller.h"
#include "zf_widget_replacer_ds.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QEvent>
#include <QFormLayout>
#include <QGraphicsColorizeEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMovie>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusTipEvent>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBox>
#include <QToolButton>
#include <QToolTip>
#include <QUiLoader>
#include <QVBoxLayout>

#define CONFIG_VERSION 0
#define CONFIG_KEY_VERSION 0

namespace zf {
//! Имя свойства для mainWidget, указывающего на его View
const char* View::MAIN_WIDGET_VIEW_PRT_NAME = "zf_view_ptr";
//! Имя mainWidget
const char* View::MAIN_WIDGET_OBJECT_NAME = "zfm";
//! Код, под которым сохраняется конфигурация представления
const QString View::VIEW_CONFIG_KEY = "ZF_VIEW_CONFIG";
//! Время последнего изменения конфигурации представления для модуля в целом
std::unique_ptr<QHash<EntityCode, QDateTime>> View::_global_config_time;

View::View(const ModelPtr& model, const ViewStates& states, const std::shared_ptr<WidgetReplacer>& widget_replacer, const DataFilterPtr& filter,
    const ModuleDataOptions& data_options, bool detached_highlight, const ViewOptions& view_options)
    : EntityObject(model->entityUid(), model->structure(),
        // используем управление ошибками из модели
        detached_highlight ? nullptr : model->highlightProcessor(), data_options)
    , _model_proxy_mode(true)
    , _is_detached_highlight(detached_highlight)
    , _view_options(view_options)

{
    setStates(states);
    bootstrap();
    setModel(model, widget_replacer, filter);
    finishConstruction();
}

View::View(const ModelPtr& model, const ViewStates& states, const DataStructurePtr& data_structure,
    const QMap<DataProperty, DataProperty>& property_proxy_mapping, const std::shared_ptr<WidgetReplacer>& widget_replacer, const DataFilterPtr& filter,
    const ModuleDataOptions& data_options, const ViewOptions& view_options)
    : EntityObject(model->entityUid(), data_structure,
        // структура данных отличается от модели - используем свое управление ошибками
        nullptr, data_options)
    , _model(model)
    , _property_proxy_mapping(property_proxy_mapping)
    , _model_proxy_mode(false)
    , _widget_replacer(widget_replacer)
    , _is_detached_highlight(true)
    , _view_options(view_options)
{
    setStates(states);
    bootstrap();
    setModel(model, widget_replacer, filter);
    finishConstruction();
}

View::~View()
{
    _is_destructing = true;

    if (!_search_widget.isNull() && _search_widget->parent() == nullptr)
        delete _search_widget;

    if (!_main_widget.isNull())
        Utils::safeDelete(_main_widget);

    if (!_body.isNull())
        Utils::safeDelete(_body);

    delete _objects_finder;

    //    Core::logInfo("view deleted: " + entityUid().toPrintable());
}

void View::setModel(const ModelPtr& model, const std::shared_ptr<WidgetReplacer>& widget_replacer, const DataFilterPtr& filter)
{
    Z_CHECK_NULL(model);

    if (model == _model)
        return;

    Z_CHECK(model->entityCode() == entityCode());

    auto old_model = _model;

    if (!_visible_status)
        _data_update_required = true;

    auto dataset_props = structure()->propertiesByType(PropertyType::Dataset);

    if (!_is_constructing) {
        beforeModelChange(_model, model);
        emit sg_beforeModelChange(_model, model);
    }

    if (_model_proxy_mode) {
        data()->setProxyContainer(model->data());

    } else {
        if (!_property_proxy_mapping.isEmpty())
            data()->setProxyContainer(model->data(), _property_proxy_mapping);
    }

    bool filter_changed = false;

    if (_filter != nullptr) {
        filter_changed = true;
        for (auto& p : qAsConst(dataset_props)) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            disconnect(proxyDataset(p), &ProxyItemModel::rowsInserted, this, &View::sl_proxy_rowsInserted);
            disconnect(proxyDataset(p), &ProxyItemModel::modelReset, this, &View::sl_proxy_modelReset);
        }

        disconnect(_filter.get(), &DataFilter::sg_refiltered, this, &View::sl_refiltered);
    }

    if (filter == nullptr) {
        _filter = Z_MAKE_SHARED(DataFilter, model.get(), this, this);

    } else {
        Z_CHECK(filter->source()->entityCode() == entityCode());
        _filter = filter;
    }
    connect(_filter.get(), &DataFilter::sg_refiltered, this, &View::sl_refiltered);

    if (_widgets != nullptr)
        _widgets->setSource(this, _filter.get());

    if (widget_replacer != nullptr) {
        _widget_replacer = widget_replacer;
        _widget_replacer_ds = nullptr;
    }

    if (_model != nullptr) {
        unFollowProgress(_model.get());

        disconnect(_model.get(), &Model::sg_operationProcessed, this, &View::sg_operationProcessed);
        disconnect(_model.get(), &Model::sg_startLoad, this, &View::sl_model_startLoad);
        disconnect(_model.get(), &Model::sg_beforeLoad, this, &View::sl_model_beforeLoad);
        disconnect(_model.get(), &Model::sg_startSave, this, &View::sl_model_startSave);
        disconnect(_model.get(), &Model::sg_startRemove, this, &View::sl_model_startRemove);
        disconnect(_model.get(), &Model::sg_finishLoad, this, &View::sl_model_finishLoad);
        disconnect(_model.get(), &Model::sg_finishSave, this, &View::sl_model_finishSave);
        disconnect(_model.get(), &Model::sg_finishRemove, this, &View::sl_model_finishRemove);
        disconnect(_model.get(), &Model::sg_existsChanged, this, &View::sl_existsChanged);
        disconnect(_model.get(), &Model::sg_error, this, &View::sl_error);
        disconnect(_model.get(), &Model::sg_entityNameChanged, this, &View::sl_modelEntityNameChanged);
        disconnect(_model.get(), &Model::sg_onFirstLoadRequest, this, &View::sl_onModelFirstLoadRequest);

        disconnectFromProperties();

        if (moduleDataOptions().testFlag(ModuleDataOption::HighlightEnabled) && !_is_detached_highlight)
            _model->highlightProcessor()->removeExternalProcessing(this);
    }

    if (_model != nullptr && masterHighlightProcessor() == _model->highlightProcessor()) {
        // надо заменить головной процессор ошибок, т.к. он брался из заменяемой модели
        installMasterHighlightProcessor(model->highlightProcessor());
    }

    _model = model;

    connect(_model.get(), &Model::sg_operationProcessed, this, &View::sg_operationProcessed);
    connect(_model.get(), &Model::sg_startLoad, this, &View::sl_model_startLoad);
    connect(_model.get(), &Model::sg_beforeLoad, this, &View::sl_model_beforeLoad);
    connect(_model.get(), &Model::sg_startSave, this, &View::sl_model_startSave);
    connect(_model.get(), &Model::sg_startRemove, this, &View::sl_model_startRemove);
    connect(_model.get(), &Model::sg_finishLoad, this, &View::sl_model_finishLoad);
    connect(_model.get(), &Model::sg_finishSave, this, &View::sl_model_finishSave);
    connect(_model.get(), &Model::sg_finishRemove, this, &View::sl_model_finishRemove);
    connect(_model.get(), &Model::sg_existsChanged, this, &View::sl_existsChanged);
    connect(_model.get(), &Model::sg_error, this, &View::sl_error);
    connect(_model.get(), &Model::sg_entityNameChanged, this, &View::sl_modelEntityNameChanged);
    connect(_model.get(), &Model::sg_onFirstLoadRequest, this, &View::sl_onModelFirstLoadRequest);

    if (moduleDataOptions().testFlag(ModuleDataOption::HighlightEnabled) && !_is_detached_highlight
        && !_model->highlightProcessor()->isExternalProcessingInstalled(this))
        _model->highlightProcessor()->installExternalProcessing(this);

    data()->setRowIdGenerator(_model.get());

    initializeProperties();
    followProgress(_model.get(), false);

    if (filter_changed) {
        // фильтр был заменен, значит у таблиц сбросились модели
        for (auto& p : qAsConst(dataset_props)) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            object<QAbstractItemView>(p)->setModel(proxyDataset(p));

            connect(proxyDataset(p), &ProxyItemModel::rowsInserted, this, &View::sl_proxy_rowsInserted);
            connect(proxyDataset(p), &ProxyItemModel::modelReset, this, &View::sl_proxy_modelReset);

            autoSelectFirstRow(p);
        }
    }

    if (!_is_constructing) {
        afterModelChange(model);
        emit sg_afterModelChange(old_model, model);

        if (_is_read_only)
            setHightlightCheckRequired(false);
    }

    setEntity(model->entityUid());

    if (filter_changed)
        onEntityNameChangedHelper();

    if (!_is_constructing) {
        widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model), true);
        requestUpdateView();
        updateSearchWidget();
        requestProcessVisibleChange();
        registerHighlightCheck();
        connectToProperties();
    }

    if (_model->isLoading())
        onStartLoading();

    if (!_is_constructing && old_model != nullptr) {
        // произошла замена модели. надо инициировать принудительный вызов dataChanged для модели, иначе будет непонятно что данные изменились
        model->data()->allDataChanged();
    }
}

ModelPtr View::model() const
{
    return _model;
}

Uid View::persistentUid() const
{
    return model()->persistentUid();
}

int View::id() const
{
    return model()->id();
}

bool View::isDetached() const
{
    return model()->isDetached();
}

ViewOptions View::options() const
{
    return _view_options;
}

ModuleWindowOptions View::moduleWindowOptions() const
{
    if (_i_modal_window)
        return _i_modal_window->options();
    if (_i_module_window)
        return _i_module_window->options();
    return nullptr;
}

QWidget* View::mainWidget() const
{
    return _main_widget;
}

QWidget* View::moduleUI() const
{
    return _body;
}

ObjectHierarchy View::uiHierarchy() const
{
    return ObjectHierarchy(moduleUI());
}

QWidget* View::lastFocused() const
{
    return _last_focused;
}

QString View::entityName() const
{
    if (isEntityNameRedefined())
        return EntityObject::entityName();

    if (model()->isEntityNameRedefined())
        return model()->entityName();

    return EntityObject::entityName();
}

QToolBar* View::mainToolbar() const
{
    return operationMenuManager()->toolbar();
}

QList<QToolBar*> View::additionalToolbars() const
{
    QList<QToolBar*> res;
    for (int i = _additional_toolbars.count() - 1; i >= 0; i--) {
        if (_additional_toolbars.at(i).isNull()) {
            _additional_toolbars.removeAt(i);
            continue;
        }
        res << _additional_toolbars.at(i);
    }
    return res;
}

QList<QToolBar*> View::toolbarsWithActions(bool visible_only) const
{
    if (visible_only)
        updateAccessRightsHelper();

    QList<QToolBar*> res;
    if (mainToolbar() != nullptr) {
        if (visible_only) {
            for (auto a : mainToolbar()->actions()) {
                if (!a->isVisible())
                    continue;

                res << mainToolbar();
                break;
            }

        } else if (!mainToolbar()->actions().isEmpty())
            res << mainToolbar();
    }

    for (auto t : additionalToolbars()) {
        if (visible_only) {
            for (auto a : t->actions()) {
                if (!a->isVisible())
                    continue;

                res << t;
                break;
            }

        } else {
            if (!t->actions().isEmpty())
                res << t;
        }
    }
    return res;
}

bool View::configureToolbarsLayout(QBoxLayout* layout)
{
    Z_CHECK_NULL(layout);
    QList<QToolBar*> toolbars = toolbarsWithActions(true);
    for (auto t : toolbars) {
        if (layout->count() > 0) {
            QFrame* line;
            line = new QFrame();
            line->setObjectName(QStringLiteral("zf_toolbar_line_%1").arg(entityCode().value()));
            line->setFrameShape(QFrame::VLine);
            line->setFrameShadow(QFrame::Raised);
            layout->addWidget(line);
        }

        layout->addWidget(t);
    }

    return !toolbars.isEmpty();
}

bool View::hasToolbars(bool visible_only) const
{
    return !toolbarsWithActions(visible_only).isEmpty();
}

bool View::isToolbarsHidden() const
{
    return _is_toolbars_hidden;
}

void View::setToolbarsHidden(bool b)
{
    _is_toolbars_hidden = b;
}

QWidget* View::widget(const PropertyID& property_id) const
{
    return object<QWidget>(property_id);
}

QWidget* View::widget(const DataProperty& property_id) const
{
    return object<QWidget>(property_id);
}

QWidget* View::widget(const QString& name) const
{
    return object<QWidget>(name);
}

DataProperty View::widgetProperty(const QWidget* widget) const
{
    return widgets()->widgetProperty(widget);
}

QList<View*> View::parentViews() const
{
    QList<View*> res;
    for (auto& v : qAsConst(_parent_views)) {
        Z_CHECK(!v->view.isNull());
        res << v->view;
    }
    return res;
}

View* View::topParentView() const
{
    for (auto& v : qAsConst(_parent_views)) {
        if (v->view->_parent_views.isEmpty())
            return v->view;
    }
    Z_CHECK(_parent_views.isEmpty());
    return nullptr;
}

Dialog* View::parentDialog() const
{
    return _parent_dialog;
}

DataPropertySet View::visibleProperties() const
{
    DataPropertySet res;
    for (auto& p : structure()->propertiesMain()) {
        QWidget* w = widgets()->field(p, false);
        if (!w)
            continue;

        if (!zf::Utils::isVisible(w)) {
            if (isPartialLoad(p) ||
                // если виджет вообще не был добавлен на форму (нет парента), то считаем его всегда видимым. иначе не начнется загрузка связанных с ним данных
                w->parentWidget() != nullptr)
                continue;
        }

        res << p;
    }

    return res;
}

bool View::isWidgetVisible(const QString& name, bool real_visible) const
{
    if (real_visible)
        return zf::Utils::isVisible(widget(name));
    else
        return !widget(name)->isHidden();
}

void View::setWidgetVisible(const QString& name, bool visible)
{
    setWidgetHidden(name, !visible);
}

void View::setWidgetHidden(const QString& name, bool hidden)
{
    widget(name)->setHidden(hidden);
}

void View::setPartialLoadProperties(const PropertyIDList& properties)
{
    setPartialLoadProperties(structure()->fromIDList(properties));
}

void View::setPartialLoadProperties(const DataPropertyList& properties)
{
    auto props = properties.toSet();
    if (props == _partial_load_properties)
        return;

    _partial_load_properties = props;
    requestProcessVisibleChange();
}

DataPropertySet View::partialLoadProperties() const
{
    return _partial_load_properties;
}

bool View::isPartialLoad(const DataProperty& p) const
{
    return _partial_load_properties.contains(p);
}

bool View::isPartialLoad(const PropertyID& p) const
{
    return isPartialLoad(property(p));
}

bool View::isReadOnly() const
{
    if (_is_read_only || !hasDirectAccessRight(AccessFlag::Modify))
        return true;

    for (auto& v : qAsConst(_parent_views)) {
        Z_CHECK(!v->view.isNull());
        if (v->view->isReadOnly())
            return true;
    }

    return false;
}

void View::setReadOnly(bool b)
{
    if (_is_read_only == b)
        return;

    _is_read_only = b;

    onReadOnlyChangedHelper();

    if (_is_read_only)
        setHightlightCheckRequired(false);
}

void View::setBlockControls(bool b)
{
    if (b == _is_block_controls)
        return;

    _is_block_controls = b;
    requestUpdateView();
}

bool View::isBlockControls() const
{
    return _is_block_controls;
}

AccessRights View::directAccessRights() const
{
    if (model() == nullptr)
        return AccessRights();

    AccessRights r = model()->directAccessRights();
    if (_is_read_only) {
        r = r.set(AccessFlag::Modify, false);
    }
    return r;
}

AccessRights View::relationAccessRights() const
{
    if (model() == nullptr)
        return AccessRights();

    return model()->relationAccessRights();
}

bool View::isMdiWindow() const
{
    return _states.testFlag(ViewState::MdiWindow);
}

bool View::isEditWindow() const
{
    return _states.testFlag(ViewState::EditWindow);
}

bool View::isViewWindow() const
{
    return _states.testFlag(ViewState::ViewWindow) || _states.testFlag(ViewState::ViewEditWindow);
}

bool View::isEmbeddedWindow() const
{
    return !_parent_views.isEmpty() || !_parent_dialog.isNull();
}

bool View::isDialogWindow() const
{
    return !_parent_dialog.isNull();
}

bool View::isNotInWindow() const
{
    return mainWidget()->parent() == nullptr;
}

ViewStates View::states() const
{
    return _states;
}

DataProperty View::editingDataset() const
{
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto iv = object<QAbstractItemView>(p);
        if (iv->currentIndex().isValid() && iv->indexWidget(iv->currentIndex()) != nullptr)
            return p;
    }
    return DataProperty();
}

void View::blockUi(bool force)
{
    if (!_is_full_created) {
        // иначе будут проблемы с обращением к виджетам до момента завершения инициализации представления
        _block_ui_counter_before_create++;
        return;
    }

    _block_ui_counter++;
    // qDebug() << "<<<<<<<< blockUi" << _block_ui_counter;
    if (_block_ui_counter > 1)
        return;

    // если представление встроено в другое, то блокировку не показываем
    if (!isParentViewUiBlocked())
        showBlockUi(force);

    widgets()->beginUpdate(DataWidgets::modelStatusToUpdateReason(model()));
    emit sg_uiBlocked();
}

void View::unBlockUi()
{
    if (_body.isNull() || _main_widget.isNull() || _is_destructing)
        return;

    if (!_is_full_created) {
        Z_CHECK(_block_ui_counter_before_create > 0);
        _block_ui_counter_before_create--;
        return;
    }

    if (_block_ui_counter == 0) {
        Z_HALT("View::unBlockUi counter error");
        return;
    }
    _block_ui_counter--;
    // qDebug() << "<<<<<<<< unBlockUi" << _block_ui_counter;
    if (_block_ui_counter > 0)
        return;

    // если представление встроено в другое, то блокировку не показываем
    if (!isParentViewUiBlocked())
        hideBlockUi();

    widgets()->endUpdate(DataWidgets::modelStatusToUpdateReason(model()));
    emit sg_uiUnBlocked();
}

bool View::isBlockedUi() const
{
    return _block_ui_counter > 0;
}

Error View::updateEmbeddedView(const EntityCode& entity_code, const DataProperty& key_property, const QVariant& view_param, const QString& source_object_name,
    const QString& target_layout_name, ModelPtr& model, ViewPtr& view, const UpdateEmbeddedViewOptions& options)
{
    int key = toInt(key_property);
    zf::Uid uid;
    if (key > 0)
        uid = zf::Uid::entity(entity_code, zf::EntityID(key));

    zf::Error error;
    if (model == nullptr || (!model->isTemporary() && !uid.isValid()) || uid != model->entityUid()) {
        if (uid.isValid()) {
            model = zf::Core::getModel<Model>(uid, error);

        } else if (model == nullptr || !model->isTemporary()) {
            model = zf::Core::createModel<Model>(entity_code, error);
        }

        if (error.isError())
            return error;

        if (view == nullptr) {
            view = zf::Core::createView<zf::View>(model, zf::ViewState::ViewWindow, view_param, error);

            view->setReadOnly(true);
            view->setHightlightCheckRequired(false);

            QObject* source_object = source_object_name.isEmpty() ? moduleUI() : view->object<QObject>(source_object_name);
            QLayout* target_layout = object<QVBoxLayout>(target_layout_name);

            if (QWidget* widget = qobject_cast<QWidget*>(source_object)) {
                if (options.testFlag(UpdateEmbeddedViewOption::LayoutInsertToBegin)) {
                    auto box_layout = qobject_cast<QBoxLayout*>(target_layout);
                    Z_CHECK_NULL(box_layout);
                    box_layout->insertWidget(0, widget);

                } else {
                    target_layout->addWidget(widget);
                }

            } else if (QLayout* layout = qobject_cast<QLayout*>(source_object)) {
                QBoxLayout* box_layout = qobject_cast<QBoxLayout*>(target_layout);
                Z_CHECK_NULL(box_layout);
                if (options.testFlag(UpdateEmbeddedViewOption::LayoutInsertToBegin))
                    box_layout->insertLayout(0, layout);
                else
                    box_layout->addLayout(layout);

            } else {
                Z_HALT_INT;
            }
        }
    }

    Z_CHECK_NULL(view);
    if (view->model() != model)
        view->setModel(model);

    return zf::Error();
}

QAction* View::operationAction(const OperationID& operation_id) const
{
    return operationAction(operation(operation_id));
}

QAction* View::operationAction(const Operation& operation) const
{
    return operationMenuManager()->operationAction(operation, true);
}

bool View::isOperationActionExists(const OperationID& operation_id) const
{
    return isOperationActionExists(operation(operation_id));
}

bool View::isOperationActionExists(const Operation& operation) const
{
    return operationMenuManager()->operationAction(operation, false) != nullptr;
}

void View::setOperationActionText(const OperationID& operation_id, const QString& text)
{
    setOperationActionText(operation(operation_id), text);
}

void View::setOperationActionText(const Operation& operation, const QString& text)
{
    auto a = operationAction(operation);
    a->setIconText(text);
    auto st = text.simplified();
    a->setText(st);
    a->setToolTip(st);
    a->setStatusTip(st);
}

QAction* View::nodeAction(const QString& accesible_id) const
{
    return operationMenuManager()->nodeAction(accesible_id, true);
}

bool View::isNodeExists(const QString& accesible_id) const
{
    return operationMenuManager()->nodeAction(accesible_id, false) != nullptr;
}

const OperationMenu* View::operationMenu() const
{
    return operationMenuManager()->operationMenu();
}

OperationList View::modalWindowOperations() const
{
    return _modal_window_operations;
}

QPushButton* View::modalWindowOperationButton(const OperationID& operation_id) const
{
    if (!_modal_window_operations.isEmpty()) {
        if (_modal_window_buttons.isEmpty()) {
            for (auto& op : qAsConst(_modal_window_operations)) {
                QPushButton* button = new QPushButton;
                button->setText((op.icon().isNull() ? "" : " ") + op.name());
                button->setIcon(op.icon());
                connect(button, &QPushButton::clicked, this, [op, this]() {
                    if (_i_modal_window == nullptr) {
                        Core::logError("modalWindowOperationButton action without interface installed");
                        return;
                    }

                    if (!_i_modal_window->beforeModalWindowOperationExecuted(op))
                        return;

                    auto error = const_cast<View*>(this)->executeOperation(op, false);
                    _i_modal_window->afterModalWindowOperationExecuted(op, error);

                    if (error.isError())
                        Core::error(error);
                });

                _modal_window_buttons[op.id()] = button;
            }
        }
    }

    Z_CHECK(_modal_window_buttons.contains(operation_id));
    return _modal_window_buttons.value(operation_id);
}

Error View::executeOperation(const Operation& op, bool emit_signal)
{
    // Надо зафиксировать изменения, которые есть в представлении, но не получены моделью
    Utils::fixUnsavedData();

    auto action = operationMenuManager()->operationAction(op, false);
    if (action != nullptr && (!action->isVisible() || !Utils::isActionEnabled(action)))
        return Error();

    if (_execute_operation_handler != nullptr) {
        Error error;
        bool processed = _execute_operation_handler->executeOperationHandler(this, op, error);
        if (error.isError())
            return error;
        if (processed)
            return Error();
    }

    if (op.options() & OperationOption::Confirmation) {
        // требуется подтверждение
        if (!confirmOperation(op))
            return Error();
    }

    ProgressHelper ph;
    if (op.options() & OperationOption::ShowProgress) {
        ph.reset(this);
        startProgress();
    }

    Error error;
    if (!processOperation(op, error)) {
        if (error.isOk()) {
            // sg_operationProcessed сгенерирует модель
            error = model()->processOperationHelper(op);
        } else {
            if (emit_signal)
                emit sg_operationProcessed(op, error);
        }
    } else {
        if (emit_signal)
            emit sg_operationProcessed(op, error);
    }

    fixFocusedWidget();

    return error;
}

Error View::executeOperation(const OperationID& operation_id)
{
    return executeOperation(operation(operation_id));
}

void View::setOperationHandler(I_ViewOperationHandler* handler)
{
    _execute_operation_handler = handler;
}

void View::cloneAction(const QAction* source, QAction* dest)
{
    Z_CHECK_NULL(source);
    Z_CHECK_NULL(dest);

    dest->setText(source->text());
    dest->setIcon(source->icon());
    dest->setToolTip(source->toolTip());
    dest->setStatusTip(source->statusTip());
}

void View::cloneAction(const OperationID& operation_id, const PropertyID& dataset_property_id, ObjectActionType type)
{
    cloneAction(operationAction(operation_id), widgets()->datasetAction(dataset_property_id, type));
}

QAction* View::datasetAction(const DataProperty& property, ObjectActionType type) const
{
    return widgets()->datasetAction(property, type);
}

QAction* View::datasetAction(const PropertyID& property_id, ObjectActionType type) const
{
    return widgets()->datasetAction(property_id, type);
}

QToolButton* View::datasetActionButton(const DataProperty& property, ObjectActionType type) const
{
    return widgets()->datasetActionButton(property, type);
}

QToolButton* View::datasetActionButton(const PropertyID& property_id, ObjectActionType type) const
{
    return widgets()->datasetActionButton(property_id, type);
}

void View::createDatasetActionMenu(const DataProperty& property, ObjectActionType type, const QList<QAction*>& actions)
{
    widgets()->createDatasetActionMenu(property, type, actions);
}

void View::createDatasetActionMenu(const PropertyID& property_id, ObjectActionType type, const QList<QAction*>& actions)
{
    widgets()->createDatasetActionMenu(property_id, type, actions);
}

bool View::sanitizeLookup(const DataProperty& property)
{
    if (!property.isField() || isNull(property))
        return false;

    if (auto w = object<zf::ItemSelector>(property, true, false)) {
        if (w->hasFilteredValue(value(property)))
            return false;

        clearValue(property);
        return true;
    }

    return false;
}

bool View::sanitizeLookup(const PropertyID& property_id)
{
    return sanitizeLookup(property(property_id));
}

DataFilter* View::filter() const
{
    return _filter.get();
}

ComplexCondition* View::conditionFilter(const DataProperty& dataset) const
{
    auto filter = _condition_filters.value(dataset);
    if (filter == nullptr) {
        Z_CHECK(dataset.propertyType() == PropertyType::Dataset);
        Z_CHECK(structure()->contains(dataset));

        filter = Z_MAKE_SHARED(ComplexCondition);
        _condition_filters[dataset] = filter;
    }

    return filter.get();
}

bool View::hasConditionFilter(const DataProperty& dataset) const
{
    Z_CHECK(dataset.isValid());
    return dataset.options().testFlag(PropertyOption::Filtered);
}

ProxyItemModel* View::proxyDataset(const DataProperty& dataset_property) const
{
    return _filter->proxyDataset(dataset_property);
}

ProxyItemModel* View::proxyDataset(const PropertyID& property_id) const
{
    return proxyDataset(property(property_id));
}

QModelIndex View::sourceIndex(const QModelIndex& index) const
{
    return _filter->sourceIndex(index);
}

QModelIndex View::proxyIndex(const QModelIndex& index) const
{
    return _filter->proxyIndex(index);
}

QModelIndex View::cellIndex(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent) const
{
    return data()->cellIndex(dataset_property, row, column, parent);
}

QModelIndex View::cellIndex(const PropertyID& dataset_property_id, int row, int column, const QModelIndex& parent) const
{
    return data()->cellIndex(dataset_property_id, row, column, parent);
}

QModelIndex View::cellIndex(int row, const DataProperty& column_property, const QModelIndex& parent) const
{
    return data()->cellIndex(row, column_property, parent);
}

QModelIndex View::cellIndex(int row, const PropertyID& column_property_id, const QModelIndex& parent) const
{
    return data()->cellIndex(row, column_property_id, parent);
}

QModelIndex View::cellIndex(const DataProperty& cell_property) const
{
    if (!cell_property.isValid())
        return QModelIndex();

    Z_CHECK(cell_property.propertyType() == PropertyType::Cell);
    QModelIndex idx = findDatasetRowID(cell_property.dataset(), cell_property.rowId());
    if (!idx.isValid())
        return QModelIndex();

    return cellIndex(idx.row(), cell_property.column(), idx.parent());
}

DataProperty View::indexColumn(const QModelIndex& index) const
{
    return widgets()->indexColumn(index);
}

bool View::setFocus(const DataProperty& p)
{
    return Utils::setFocus(p, highligtMapping());
}

bool View::setFocus(const PropertyID& p)
{
    return setFocus(property(p));
}

DataProperty View::currentFocus() const
{
    DataProperty current_property;
    QWidget* focused = moduleUI()->focusWidget();
    if (focused != nullptr) {
        current_property = widgetProperty(focused);
        if (current_property.propertyType() == PropertyType::Dataset && !current_property.options().testFlag(PropertyOption::SimpleDataset)
            && widgets()->currentIndex(current_property).isValid())
            current_property = propertyCell(widgets()->currentIndex(current_property));
    }

    return current_property;
}

QModelIndex View::currentActiveIndex() const
{
    auto p = currentDataset();
    return p.isValid() ? currentIndex(p) : QModelIndex();
}

DataProperty View::currentActiveColumn() const
{
    auto index = currentActiveIndex();
    if (!index.isValid())
        return {};

    return indexColumn(index);
}

QModelIndex View::currentIndex(const DataProperty& dataset_property) const
{
    return widgets()->currentIndex(dataset_property);
}

QModelIndex View::currentIndex(const PropertyID& dataset_property_id) const
{
    return widgets()->currentIndex(dataset_property_id);
}

QModelIndex View::currentIndex() const
{
    return widgets()->currentIndex();
}

int View::currentRow(const DataProperty& dataset_property) const
{
    return widgets()->currentRow(dataset_property);
}

int View::currentRow(const PropertyID& dataset_property_id) const
{
    return widgets()->currentRow(dataset_property_id);
}

int View::currentRow() const
{
    return widgets()->currentRow();
}

QModelIndexList View::selectedIndexes(const DataProperty& dataset_property) const
{
    return widgets()->selectedIndexes(dataset_property);
}

QModelIndexList View::selectedIndexes(const PropertyID& dataset_property_id) const
{
    return widgets()->selectedIndexes(dataset_property_id);
}

QModelIndexList View::selectedIndexes() const
{
    return widgets()->selectedIndexes();
}

DataProperty View::currentColumn(const DataProperty& dataset_property) const
{
    return widgets()->currentColumn(dataset_property);
}

DataProperty View::currentColumn(const PropertyID& dataset_property_id) const
{
    return widgets()->currentColumn(dataset_property_id);
}

DataProperty View::currentColumn() const
{
    return widgets()->currentColumn();
}

bool View::setCurrentIndex(const DataProperty& dataset_property, const QModelIndex& index, bool current_column) const
{
    return widgets()->setCurrentIndex(dataset_property, index, current_column);
}

bool View::setCurrentIndex(const PropertyID& dataset_property_id, const QModelIndex& index, bool current_column) const
{
    return widgets()->setCurrentIndex(dataset_property_id, index, current_column);
}

bool View::setCurrentIndex(const QModelIndex& index, bool current_column) const
{
    return widgets()->setCurrentIndex(index, current_column);
}

bool View::setCurrentIndex(const RowID& row, const DataProperty& column_property)
{
    Z_CHECK(column_property.isValid());

    if (!row.isValid())
        return setCurrentIndex(column_property.dataset(), QModelIndex());

    QModelIndex index = findDatasetRowID(column_property.dataset(), row);
    if (!index.isValid())
        return false;

    return setCurrentIndex(column_property.dataset(), index);
}

bool View::setCurrentIndex(const RowID& row, const PropertyID& column_property_id)
{
    return setCurrentIndex(row, property(column_property_id));
}

Rows View::selectedRows(const DataProperty& dataset_property) const
{
    QModelIndexList selected = selectionModel(dataset_property)->selectedRows();
    if (selected.isEmpty()) {
        QModelIndex current = currentIndex(dataset_property);
        if (current.isValid())
            selected << current;
    }
    for (auto& i : selected) {
        i = sourceIndex(i);
        Z_CHECK(i.isValid());
        i = i.model()->index(i.row(), 0, i.parent());
    }
    return Rows(selected).sort();
}

QItemSelectionModel* View::selectionModel(const DataProperty& dataset_property) const
{
    return object<QAbstractItemView>(dataset_property)->selectionModel();
}

int View::firstVisibleColumn(const DataProperty& dataset_property) const
{
    return horizontalRootHeaderItem(dataset_property)->firstVisibleSection();
}

int View::lastVisibleColumn(const DataProperty& dataset_property) const
{
    return horizontalRootHeaderItem(dataset_property)->lastVisibleSection();
}

DataProperty View::lastActiveDataset() const
{
    return _last_active_dataset;
}

void View::blockShowErrors()
{
    _block_show_errors++;
}

void View::unBlockShowErrors()
{
    _block_show_errors--;
    Z_CHECK(_block_show_errors >= 0);
}

HeaderItem* View::rootHeaderItem(const DataProperty& dataset_property, Qt::Orientation orientation) const
{
    return widgets()->rootHeaderItem(dataset_property, orientation);
}

HeaderItem* View::horizontalRootHeaderItem(const DataProperty& dataset_property) const
{
    return widgets()->horizontalRootHeaderItem(dataset_property);
}

HeaderItem* View::horizontalRootHeaderItem(const PropertyID& dataset_property_id) const
{
    return widgets()->horizontalRootHeaderItem(dataset_property_id);
}

HeaderItem* View::verticalRootHeaderItem(const DataProperty& dataset_property) const
{
    return widgets()->verticalRootHeaderItem(dataset_property);
}

HeaderItem* View::verticalRootHeaderItem(const PropertyID& dataset_property_id) const
{
    return widgets()->verticalRootHeaderItem(dataset_property_id);
}

HeaderItem* View::horizontalHeaderItem(const DataProperty& dataset_property, int section_id) const
{
    return widgets()->horizontalHeaderItem(dataset_property, section_id);
}

HeaderItem* View::horizontalHeaderItem(const PropertyID& dataset_property_id, int section_id) const
{
    return widgets()->horizontalHeaderItem(dataset_property_id, section_id);
}

HeaderItem* View::horizontalHeaderItem(const DataProperty& section_id) const
{
    return widgets()->horizontalHeaderItem(section_id);
}

HeaderItem* View::horizontalHeaderItem(const PropertyID& section_id) const
{
    return widgets()->horizontalHeaderItem(section_id);
}

HeaderItem* View::verticalHeaderItem(const DataProperty& dataset_property, int section_id) const
{
    return widgets()->verticalHeaderItem(dataset_property, section_id);
}

HeaderItem* View::verticalHeaderItem(const PropertyID& dataset_property_id, int section_id) const
{
    return widgets()->verticalHeaderItem(dataset_property_id, section_id);
}

HeaderView* View::horizontalHeaderView(const DataProperty& dataset_property) const
{
    return widgets()->horizontalHeaderView(dataset_property);
}

HeaderView* View::horizontalHeaderView(const PropertyID& dataset_property_id) const
{
    return widgets()->horizontalHeaderView(dataset_property_id);
}

void View::setSectionSizePx(const DataProperty& dataset_property, int section_id, int size)
{
    widgets()->setSectionSizePx(dataset_property, section_id, size);
}

void View::setSectionSizePx(const PropertyID& dataset_property_id, int section_id, int size)
{
    widgets()->setSectionSizePx(dataset_property_id, section_id, size);
}

void View::setSectionSizePx(const DataProperty& column_property, int size)
{
    widgets()->setSectionSizePx(column_property, size);
}

void View::setSectionSizeChar(const DataProperty& dataset_property, int section_id, int size)
{
    widgets()->setSectionSizeChar(dataset_property, section_id, size);
}

void View::setSectionSizeChar(const PropertyID& dataset_property_id, int section_id, int size)
{
    widgets()->setSectionSizeChar(dataset_property_id, section_id, size);
}

void View::setSectionSizeChar(const DataProperty& column_property, int size)
{
    widgets()->setSectionSizeChar(column_property, size);
}

int View::sectionSizePx(const DataProperty& dataset_property, int section_id) const
{
    return widgets()->sectionSizePx(dataset_property, section_id);
}

int View::sectionSizePx(const PropertyID& dataset_property_id, int section_id) const
{
    return widgets()->sectionSizePx(dataset_property_id, section_id);
}

int View::sectionSizePx(const DataProperty& column_property) const
{
    return widgets()->sectionSizePx(column_property);
}

void View::initDatasetHorizontalHeader(const DataProperty& dataset_property)
{
    widgets()->initDatasetHorizontalHeader(dataset_property);
}

void View::initDatasetHorizontalHeaders()
{
    widgets()->initDatasetHorizontalHeaders();
}

void View::resetSectionSize(const PropertyID& dataset_property_id, int section_id)
{
    widgets()->resetSectionSize(dataset_property_id, section_id);
}

void View::resetSectionSize(const DataProperty& column_property)
{
    widgets()->resetSectionSize(column_property);
}

DataProperty View::currentDataset() const
{
    if (QApplication::focusWidget() != nullptr) {
        DataProperty p = widgetProperty(QApplication::focusWidget());
        if (p.propertyType() == PropertyType::Dataset)
            return p;
    }

    auto datasets = structure()->propertiesByType(PropertyType::Dataset);
    for (int i = datasets.count() - 1; i >= 0; i--) {
        if (datasets.at(i).options().testFlag(PropertyOption::SimpleDataset))
            datasets.removeAt(i);
    }

    if (datasets.isEmpty())
        return DataProperty();

    if (datasets.count() == 1) {
        auto view = object<QAbstractItemView>(datasets.first());
        if (Utils::isVisible(view))
            return datasets.first();
        else
            return DataProperty();

    } else {
        // Ищем все видимые наборы данных
        QList<QAbstractItemView*> visible_list;
        for (auto& p : datasets) {
            auto view = object<QAbstractItemView>(p);
            if (Utils::isVisible(view))
                visible_list << view;
        }

        // Если всего один видимый, то он всегда активный
        if (visible_list.count() == 1) {
            return widgetProperty(visible_list.first());

        } else {
            // Если больше одного видимого, то активен имеющий фокус ввода
            if (_last_active_dataset.isValid())
                return Utils::isVisible(object<QAbstractItemView>(_last_active_dataset)) ? _last_active_dataset : DataProperty();
        }

        return DataProperty();
    }
}

QStyleOptionViewItem View::getDatasetItemViewOptions(const DataProperty& dataset) const
{
    if (dataset.dataType() == DataType::Table)
        return object<TableView>(dataset)->viewOptions();
    else if (dataset.dataType() == DataType::Tree)
        return object<TreeView>(dataset)->viewOptions();
    else
        Z_HALT_INT;

    return QStyleOptionViewItem();
}

bool View::editCell(const PropertyID& dataset_property_id)
{
    return editCell(property(dataset_property_id));
}

bool View::editCell(const DataProperty& dataset)
{
    auto view = object<QAbstractItemView>(dataset);
    QModelIndex index = view->currentIndex();
    if (index.isValid()) {
        view->edit(index);
        return true;
    }

    return false;
}

void View::showDatasetCheckPanel(const DataProperty& dataset_property, bool show)
{
    object<TableView>(dataset_property)->showCheckRowPanel(show);
}

void View::showDatasetCheckPanel(const PropertyID& dataset_property_id, bool show)
{
    showDatasetCheckPanel(property(dataset_property_id), show);
}

bool View::isShowDatasetCheckPanel(const DataProperty& dataset_property) const
{
    return object<TableView>(dataset_property)->isShowCheckRowPanel();
}

bool View::isShowDatasetCheckPanel(const PropertyID& dataset_property_id) const
{
    return isShowDatasetCheckPanel(property(dataset_property_id));
}

bool View::hasCheckedDatasetRows(const DataProperty& dataset_property) const
{
    return object<TableView>(dataset_property)->hasCheckedRows();
}

bool View::hasCheckedDatasetRows(const PropertyID& dataset_property_id) const
{
    return hasCheckedDatasetRows(property(dataset_property_id));
}

bool View::isDatasetRowChecked(const DataProperty& dataset_property, int row) const
{
    return object<TableView>(dataset_property)->isRowChecked(row);
}

bool View::isDatasetRowChecked(const PropertyID& dataset_property_id, int row) const
{
    return isDatasetRowChecked(property(dataset_property_id), row);
}

void View::checkDatasetRow(const DataProperty& dataset_property, int row, bool checked)
{
    object<TableView>(dataset_property)->checkRow(row, checked);
}

void View::checkDatasetRow(const PropertyID& dataset_property_id, int row, bool checked)
{
    checkDatasetRow(property(dataset_property_id), row, checked);
}

QSet<int> View::checkedDatasetRows(const DataProperty& dataset_property) const
{
    return object<TableView>(dataset_property)->checkedRows();
}

QSet<int> View::checkedDatasetRows(const PropertyID& dataset_property_id) const
{
    return checkedDatasetRows(property(dataset_property_id));
}

int View::checkedDatasetRowsCount(const DataProperty& dataset_property) const
{
    return object<TableView>(dataset_property)->checkedRowsCount();
}

int View::checkedDatasetRowsCount(const PropertyID& dataset_property_id) const
{
    return checkedDatasetRowsCount(property(dataset_property_id));
}

void View::clearAllDatasetCheckRows(const DataProperty& dataset_property)
{
    object<TableView>(dataset_property)->clearAllCheckRows();
}

void View::clearAllDatasetCheckRows(const PropertyID& dataset_property_id)
{
    clearAllDatasetCheckRows(property(dataset_property_id));
}

bool View::isAllDatasetRowsChecked(const DataProperty& dataset_property) const
{
    return object<TableView>(dataset_property)->isAllRowsChecked();
}

bool View::isAllDatasetRowsChecked(const PropertyID& dataset_property_id) const
{
    return isAllDatasetRowsChecked(property(dataset_property_id));
}

void View::checkAllDatasetRows(const PropertyID& dataset_property_id, bool checked)
{
    checkAllDatasetRows(property(dataset_property_id), checked);
}

int View::datasetFrozenGroupCount(const DataProperty& dataset_property) const
{
    return widgets()->datasetFrozenGroupCount(dataset_property);
}

int View::datasetFrozenGroupCount(const PropertyID& dataset_property_id) const
{
    return widgets()->datasetFrozenGroupCount(dataset_property_id);
}

void View::setDatasetFrozenGroupCount(const DataProperty& dataset_property, int count)
{
    widgets()->setDatasetFrozenGroupCount(dataset_property, count);
}

void View::setDatasetFrozenGroupCount(const PropertyID& dataset_property_id, int count)
{
    widgets()->setDatasetFrozenGroupCount(dataset_property_id, count);
}

void View::invalidateUsingModels() const
{
    if (_invalidating_using_models)
        return;

    _invalidating_using_models = true;

    EntityObject::invalidateUsingModels();

    // встроенные представления
    for (auto v : embedViews()) {
        v->invalidateUsingModels();
    }

    _invalidating_using_models = false;
}

QVariantList View::getColumnValues(const DataProperty& column, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, QModelIndexList(), Qt::DisplayRole, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const PropertyID& column, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, QModelIndexList(), Qt::DisplayRole, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const DataProperty& column, const QList<RowID>& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const DataProperty& column, const QModelIndexList& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const DataProperty& column, const QList<int>& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const PropertyID& column, const QList<RowID>& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const PropertyID& column, const QModelIndexList& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const PropertyID& column, const QList<int>& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

QVariantList View::getColumnValues(const PropertyID& column, const Rows& rows, int role, bool use_filter, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, use_filter ? filter() : nullptr, valid_only);
}

void View::checkAllDatasetRows(const DataProperty& dataset_property, bool checked)
{
    object<TableView>(dataset_property)->checkAllRows(checked);
}

OperationMenu View::createOperationMenu() const
{
    return OperationMenu();
}

void View::onReadOnlyChanged()
{
}

void View::onParentViewChanged(View* view)
{
    Q_UNUSED(view)
}

void View::onParentDialogChanged(Dialog* dialog)
{
    Q_UNUSED(dialog)
}

bool View::isUiCreated() const
{
    return _is_full_created;
}

void View::createUI()
{
}

void View::configureWidgetScheme(WidgetScheme* scheme) const
{
    Q_UNUSED(scheme)
}

void View::onWidgetCreated(QWidget* w, const DataProperty& p)
{
    Q_UNUSED(w)
    Q_UNUSED(p)
}

Error View::getDatasetCellVisibleValue(int row, const DataProperty& column, const QModelIndex& parent, const QVariant& original_value,
    const VisibleValueOptions& options, QString& value, QList<ModelPtr>& model_data_not_ready) const
{
    return getDatasetCellVisibleValue(
        object<QAbstractItemView>(column.dataset()), column, cellIndex(row, column, parent), original_value, options, value, model_data_not_ready);
}

Error View::getDatasetCellVisibleValue(QAbstractItemView* item_view, const DataProperty& column, const QModelIndex& source_index,
    const QVariant& original_value, const VisibleValueOptions& options, QString& value, QList<ModelPtr>& model_data_not_ready)
{
    Z_CHECK_NULL(item_view);
    Error error;

    QVariant converted;
    if (column.isValid())
        Utils::convertValue(
            original_value, column.dataType(), Core::locale(LocaleType::Universal), ValueFormat::Edit, Core::locale(LocaleType::UserInterface), converted);
    else
        converted = original_value;

    auto lookup = column.lookup();
    if (lookup == nullptr) {
        if (column.dataType() == DataType::Bool) {
            if (options.testFlag(VisibleValueOption::Export)) {
                if (original_value.type() == QVariant::Bool) {
                    value = original_value.toBool() ? ZF_TR(ZFT_YES).toLower() : QString();

                } else {
                    if (source_index.data(Qt::CheckStateRole).isValid()) {
                        if (source_index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                            value = ZF_TR(ZFT_YES).toLower();
                        else if (source_index.data(Qt::CheckStateRole).toInt() == Qt::PartiallyChecked)
                            value = ZF_TR(ZFT_PARTIAL).toLower();
                        else
                            value.clear();

                    } else {
                        value.clear();
                    }
                }

            } else {
                value.clear();
            }

        } else {
            value = Utils::variantToString(converted);
        }
        return error;
    }

    QVariant result;

    if (lookup->type() == LookupType::List) {
        result = lookup->listName(converted);

    } else if (lookup->type() == LookupType::Dataset) {
        // запоминаем lookup модель, чтобы не грузилась постоянно из БД
        if (lookup->listEntity().isValid())
            Core::modelKeeper()->keepModels(lookup->listEntity(), { lookup->datasetProperty() });

        auto free_text_link = column.links(PropertyLinkType::LookupFreeText);
        if (!free_text_link.isEmpty() && (!converted.isValid() || (converted.type() == QVariant::String && converted.toString().isEmpty()))) {
            // надо показывать значение из колонки free_text
            result = source_index.model()
                         ->index(source_index.row(), column.dataset().columnPos(free_text_link.constFirst()->linkedPropertyId()), source_index.parent())
                         .data(Qt::DisplayRole);

        } else {
            // поиск значения в lookup модели
            ModelPtr source_model;
            if (!Core::getEntityValue(lookup, converted, result, source_model, error, { item_view })) {
                if (error.isError()) {
                    result = error.fullText();
                } else {
                    result = ZF_TR(ZFT_LOADING).toLower();
                    model_data_not_ready << source_model;
                }
            }
        }

    } else if (lookup->type() == LookupType::Request) {
        Z_CHECK(source_index.isValid());
        auto links = column.links(PropertyLinkType::LookupFreeText);
        Z_CHECK(links.count() == 1);
        result = source_index.model()
                     ->index(source_index.row(), column.dataset().columnPos(links.at(0)->linkedPropertyId()), source_index.parent())
                     .data(Qt::DisplayRole);

    } else
        Z_HALT_INT;

    value = Utils::variantToString(result);
    return error;
}

void View::getDatasetCellProperties(int row, const DataProperty& column, const QModelIndex& parent, QStyle::State state, QFont& font, QBrush& background,
    QPalette& palette, QIcon& icon, Qt::Alignment& alignment) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    Q_UNUSED(state);
    Q_UNUSED(background);
    Q_UNUSED(palette);
    Q_UNUSED(icon);
    Q_UNUSED(font);
    Q_UNUSED(alignment);
}

bool View::isDatasetCellReadOnly(int row, const DataProperty& column, const QModelIndex& parent) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    return false;
}

bool View::processDatasetKeyPress(const DataProperty& p, QAbstractItemView* view, QKeyEvent* e)
{
    Q_UNUSED(p)
    Q_UNUSED(view)
    Q_UNUSED(e)
    return false;
}

void View::getDynamicLookupModel(const DataProperty& property, ModelPtr& model, DataFilterPtr& filter, PropertyID& lookup_column_display,
    PropertyID& lookup_column_id, int& lookup_role_display, int& lookup_role_id) const
{
    Q_UNUSED(property)
    Q_UNUSED(model)
    Q_UNUSED(filter)
    Q_UNUSED(lookup_column_display)
    Q_UNUSED(lookup_column_id)
    Q_UNUSED(lookup_role_display)
    Q_UNUSED(lookup_role_id)
}

DataWidgets* View::widgets() const
{
    if (_widgets == nullptr) {
        View* c_this = const_cast<View*>(this);
        _widgets = new DataWidgets(c_this, _filter.get(), c_this);
        connect(_widgets.get(), &DataWidgets::sg_datasetActionTriggered, this, &View::sl_datasetActionActivated);

        _highligt_mapping = new HighlightMapping(_widgets.get(), c_this);

        Z_CHECK(_widget_highligter_controller == nullptr);
        _widget_highligter_controller = new WidgetHighligterController(c_this->highlightProcessor(), _widgets.get(), _widget_highlighter, _highligt_mapping);
    }

    return _widgets.get();
}

const QList<View*>& View::embedViews() const
{
    return _embed_views;
}

bool View::isPropertyReadOnly(const DataProperty& p) const
{
    auto w = object<QWidget>(p, false);
    if (w != nullptr && _ignore_read_only_widgets.contains(w))
        return false;

    auto info = propertyInfo(p);
    if (info->read_only != Answer::Undefined)
        return info->read_only == Answer::Yes;

    return p.options().testFlag(PropertyOption::ReadOnly);
}

bool View::isPropertyReadOnly(const PropertyID& p) const
{
    return isPropertyReadOnly(property(p));
}

void View::setPropertyReadOnly(const DataProperty& p, bool read_only)
{
    auto info = propertyInfo(p);
    info->read_only = read_only ? Answer::Yes : Answer::No;
    if (p.isDataset() && !p.options().testFlag(PropertyOption::SimpleDataset))
        requestUpdateAccessRights();

    requestUpdatePropertyStatus(p);
}

void View::setLayoutWidgetsReadOnly(QLayout* layout, bool read_only, const QSet<QWidget*>& excluded_widgets, const QSet<QLayout*>& excluded_layouts)
{
    Z_CHECK_NULL(layout);
    Z_CHECK(!excluded_widgets.contains(nullptr));
    Z_CHECK(!excluded_layouts.contains(nullptr));

    const QObjectList& objList = layout->children();

    for (int i = 0; i < objList.size(); i++) {
        QLayout* la = qobject_cast<QLayout*>(objList.at(i));
        if (!la || excluded_layouts.contains(la))
            continue;

        setLayoutWidgetsReadOnly(la, read_only, excluded_widgets, excluded_layouts);
    }

    for (int i = 0; i < layout->count(); i++) {
        if (layout->itemAt(i) && !layout->itemAt(i)->isEmpty() && layout->itemAt(i)->widget()) {
            if (excluded_widgets.contains(layout->itemAt(i)->widget()))
                continue;
            auto w = layout->itemAt(i)->widget();

            // надо исключить изменение доступности тулбаров
            bool ignore = false;
            for (auto& ds : structure()->propertiesByType(PropertyType::Dataset)) {
                if (ds.options().testFlag(PropertyOption::SimpleDataset))
                    continue;
                if (w == widgets()->datasetToolbarWidget(ds) || w == widgets()->datasetToolbar(ds)) {
                    ignore = true;
                    break;
                }
            }
            if (ignore)
                continue;

            auto prop = widgetProperty(w);
            if (prop.isValid())
                setPropertyReadOnly(prop, read_only);
            else
                setWidgetReadOnly(w, read_only);
        }
    }
}

void View::setPropertyReadOnly(const PropertyID& p, bool read_only)
{
    setPropertyReadOnly(property(p), read_only);
}

void View::setWidgetIgnoreReadOnly(QWidget* w)
{
    Z_CHECK_NULL(w);
    _ignore_read_only_widgets << w;
}

void View::requestUpdatePropertyStatus(const DataProperty& p) const
{
    if (objectExtensionDestroyed())
        return;

    if ((p.isField() || p.isDataset()) && !internalCallbackManager()->isRequested(this, Framework::VIEW_REQUEST_UPDATE_ALL_PROPERTY_STATUS)
        && !internalCallbackManager()->isRequested(this, Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS)) {
        _properties_to_update << p;
        internalCallbackManager()->addRequest(this, Framework::VIEW_UPDATE_PROPERTY_STATUS_KEY);
    }
}

void View::requestUpdatePropertyStatus(const PropertyID& p) const
{
    requestUpdatePropertyStatus(property(p));
}

void View::requestUpdateAllPropertiesStatus() const
{
    if (objectExtensionDestroyed())
        return;

    if (!internalCallbackManager()->isRequested(this, Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS)) {
        _properties_to_update.clear();
        internalCallbackManager()->addRequest(this, Framework::VIEW_REQUEST_UPDATE_ALL_PROPERTY_STATUS);
    }
}

void View::setVisibleStatus(bool b)
{
    if (_visible_status == b)
        return;

    _visible_status = b;

    if (!b && internalCallbackManager()->isRequested(this, Framework::VIEW_INVALIDATED_KEY))
        _data_update_required = true;

    if (b && _data_update_required) {
        if (!objectExtensionDestroyed())
            internalCallbackManager()->addRequest(this, Framework::VIEW_INVALIDATED_KEY);
        _data_update_required = false;
    }
}

bool View::isPropertyVisible(const DataProperty& p, bool real_visible) const
{
    return !isPropertyHidden(p, real_visible);
}

bool View::isPropertyVisible(const PropertyID& p, bool real_visible) const
{
    return !isPropertyHidden(p, real_visible);
}

bool View::isPropertyHidden(const DataProperty& p, bool real_hidden) const
{
    if (real_hidden)
        return !zf::Utils::isVisible(widget(p));

    return widget(p)->isHidden();
}

bool View::isPropertyHidden(const PropertyID& p, bool real_hidden) const
{
    return isPropertyHidden(property(p), real_hidden);
}

void View::setPropertyVisible(const DataProperty& p, bool visible)
{
    setPropertyHidden(p, !visible);
}

void View::setPropertyVisible(const PropertyID& p, bool visible)
{
    setPropertyHidden(p, !visible);
}

void View::setPropertyHidden(const DataProperty& p, bool hidden)
{
    QWidget* w;
    if (p.isDataset())
        w = widgets()->datasetGroup(p);
    else
        w = widget(p);

    // не показываем тех, у кого нет родителя, иначе они будут болтаться на экране в виде отдельного окна
    if (w->parentWidget() != nullptr || hidden)
        w->setHidden(hidden);
}

void View::setPropertyHidden(const PropertyID& p, bool hidden)
{
    setPropertyHidden(property(p), hidden);
}

ItemSelector* View::createSearchWidget(const Uid& entity, const PropertyID& id_column_property_id, const PropertyID& display_column_property_id,
    const QString& placeholder_translation_id, int id_colum_role, int display_colum_role)
{
    auto display_column_property = DataProperty::property(entity.entityCode(), display_column_property_id);
    auto id_column_property = DataProperty::property(entity.entityCode(), id_column_property_id);
    Z_CHECK(display_column_property.propertyType() == PropertyType::ColumnFull);
    Z_CHECK(id_column_property.propertyType() == PropertyType::ColumnFull);
    Z_CHECK(display_column_property.dataset() == id_column_property.dataset());

    Error error;
    auto model = Core::getModel<Model>(entity, {}, { display_column_property.dataset() }, error);
    if (error.isError())
        Z_HALT(error);

    return createSearchWidget(model, id_column_property_id, display_column_property_id, placeholder_translation_id, id_colum_role, display_colum_role, nullptr);
}

ItemSelector* View::createSearchWidget(const ModelPtr& model, const PropertyID& id_column_property_id, const PropertyID& display_column_property_id,
    const QString& placeholder_translation_id, int id_colum_role, int display_colum_role, const DataFilterPtr& data_filter)
{
    if (_search_widget != nullptr) {
        delete _search_widget;
        _search_widget = nullptr;
    }

    Z_CHECK_NULL(model);
    auto display_column_property = model->structure()->property(display_column_property_id);
    Z_CHECK(display_column_property.propertyType() == PropertyType::ColumnFull);
    auto id_column_property = model->structure()->property(id_column_property_id);
    Z_CHECK(id_column_property.propertyType() == PropertyType::ColumnFull);

    _search_widget = new ItemSelector(model, display_column_property.dataset(), data_filter);
    _search_widget->setObjectName(QStringLiteral("zfsw"));
    _search_widget->setDisplayColumn(display_column_property.pos());
    _search_widget->setDisplayRole(display_colum_role);
    _search_widget->setIdColumn(id_column_property.pos());
    _search_widget->setIdRole(id_colum_role);
    _search_widget->setEraseEnabled(false);

    if (placeholder_translation_id.isEmpty())
        _search_widget->setPlaceholderText(translate(TR::ZFT_SEARCH_WIDGET_PLACEHOLDER));
    else
        _search_widget->setPlaceholderText(translate(placeholder_translation_id));

    if (_widget_replacer_ds != nullptr)
        _widget_replacer_ds->setEntitySearchWidget(_search_widget);

    updateSearchWidget();

    connect(_search_widget, qOverload<const QModelIndex&, bool>(&ItemSelector::sg_edited), this, &View::sl_searchWidgetEdited);

    return _search_widget;
}

ItemSelector* View::searchWidget() const
{
    return _search_widget;
}

void View::replaceWidgets()
{
    // поисковый виджет используем только в MDI окнах просмотра
    QWidget* s_w = (states() & ViewState::MdiWindow ? searchWidget() : nullptr);

    if (_widget_replacer == nullptr) {
        auto wr = Z_MAKE_SHARED(WidgetReplacerDataStructure, widgets(), s_w);
        _widget_replacer_ds = wr.get();
        _widget_replacer = wr;
    }

    if (_widget_replacer_ds != nullptr)
        _widget_replacer_ds->setEntitySearchWidget(s_w);

    _widget_replacer->replace(_body);
}

void View::initWidgetStyle(QWidget* w, const DataProperty& p)
{
    Q_UNUSED(w)
    Q_UNUSED(p)
}

void View::requestUpdateView() const
{
    requestUpdateAccessRights();
    requestUpdateWidgetStatuses();
}

void View::onUpdateAccessRights()
{
    QList<ObjectActionType> types
        = { ObjectActionType::Create, ObjectActionType::CreateLevelDown, ObjectActionType::Remove, ObjectActionType::Modify, ObjectActionType::View };
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        for (auto t : qAsConst(types)) {
            bool visible = true;
            bool enable = true;
            checkDatasetActionProperties(p, t, enable, visible);
            QAction* a = widgets()->datasetAction(p, t);
            a->setEnabled(enable);
            a->setVisible(visible);
        }
    }
}

void View::bootstrap()
{
    //    Core::logInfo(QString("view start load: %1").arg(entityUid().toPrintable()));

    connect(highlightProcessor(), &HighlightProcessor::sg_highlightItemInserted, this, &View::sl_highlight_itemInserted);
    connect(highlightProcessor(), &HighlightProcessor::sg_highlightItemRemoved, this, &View::sl_highlight_itemRemoved);
    connect(highlightProcessor(), &HighlightProcessor::sg_highlightItemChanged, this, &View::sl_highlight_itemChanged);
    connect(highlightProcessor(), &HighlightProcessor::sg_highlightBeginUpdate, this, &View::sl_highlight_beginUpdate);
    connect(highlightProcessor(), &HighlightProcessor::sg_highlightEndUpdate, this, &View::sl_highlight_endUpdate);

    _show_block_ui_timer = new QTimer(this);
    _show_block_ui_timer->setSingleShot(true);
    _show_block_ui_timer->setInterval(50);
    connect(_show_block_ui_timer, &QTimer::timeout, this, &View::sl_blockUiTimeout);

    _search_timer = new QTimer(this);
    _search_timer->setSingleShot(true);
    _search_timer->setInterval(Consts::USER_INPUT_TIMEOUT_MS);
    connect(_search_timer, &QTimer::timeout, this, &View::sl_searchTimeout);

    _dataset_current_cell_changed_timer = new QTimer(this);
    _dataset_current_cell_changed_timer->setSingleShot(true);
    _dataset_current_cell_changed_timer->setInterval(Consts::USER_INPUT_TIMEOUT_MS);
    connect(_dataset_current_cell_changed_timer, &QTimer::timeout, this, &View::sl_datasetCurrentCellChangedTimeout);

    _dataset_current_row_changed_timer = new QTimer(this);
    _dataset_current_row_changed_timer->setSingleShot(true);
    _dataset_current_row_changed_timer->setInterval(Consts::USER_INPUT_TIMEOUT_MS);
    connect(_dataset_current_row_changed_timer, &QTimer::timeout, this, &View::sl_datasetCurrentRowChangedTimeout);

    _dataset_current_column_changed_timer = new QTimer(this);
    _dataset_current_column_changed_timer->setSingleShot(true);
    _dataset_current_column_changed_timer->setInterval(Consts::USER_INPUT_TIMEOUT_MS);
    connect(_dataset_current_column_changed_timer, &QTimer::timeout, this, &View::sl_datasetCurrentColumnChangedTimeout);

    _main_widget = new QWidget;
    _main_widget->setUpdatesEnabled(false);
    _main_widget->setObjectName(MAIN_WIDGET_OBJECT_NAME);
    _main_widget->setProperty(MAIN_WIDGET_VIEW_PRT_NAME, qintptr(this));
    ChildObjectsContainer::markIgnored(_main_widget);

    _body = new QWidget;
    _body->setObjectName(QStringLiteral("zfb"));

    _widget_highlighter = new WidgetHighlighter(this);

    QVBoxLayout* main_la = new QVBoxLayout;
    main_la->setObjectName(QStringLiteral("zfla"));
    main_la->setMargin(0);
    _main_widget->setLayout(main_la);
    main_la->addWidget(_body);
    ChildObjectsContainer::markIgnored(main_la);

#ifndef QT_DEBUG
    _block_ui_top_widget = new QWidget(_main_widget);
    _block_ui_top_widget->setObjectName(QStringLiteral("zfbui"));
    _block_ui_top_widget->setHidden(true);
    ChildObjectsContainer::markIgnored(_block_ui_top_widget);

    _wait_movie_label = Utils::createWaitingMovie(_block_ui_top_widget);
    _wait_movie_label->setHidden(true);
    ChildObjectsContainer::markIgnored(_wait_movie_label);
#endif

    // надо подписаться на изменения в lookup для таблиц, чтобы обновить отображение
    for (auto& c : structure()->propertiesByType(PropertyType::ColumnFull)) {
        if (c.lookup() == nullptr || c.lookup()->type() != LookupType::Dataset)
            continue;

        _lookup_subscribe << Core::messageDispatcher()->subscribe(zf::CoreChannels::ENTITY_CHANGED, this, { c.lookup()->listEntity().entityCode() });
        _lookup_subscribe << Core::messageDispatcher()->subscribe(zf::CoreChannels::ENTITY_REMOVED, this, { c.lookup()->listEntity().entityCode() });
    }

    _body->installEventFilter(this);
    _main_widget->installEventFilter(this);

    _objects_finder = new ObjectsFinder(entityCode().string(), { moduleUI() });
    connect(_objects_finder, &ObjectsFinder::sg_objectAdded, this, &View::sl_containerObjectAdded);
    connect(_objects_finder, &ObjectsFinder::sg_objectRemoved, this, &View::sl_containerObjectRemoved);

    connect(this, &QObject::objectNameChanged, this, [&]() {
        if (_widgets != nullptr)
            _widgets->setObjectName(objectName());
    });
}

void View::finishConstruction()
{
    _is_constructing = false;
}

void View::disconnectFromProperties()
{
    for (auto& p : structure()->properties()) {
        if (p.propertyType() == PropertyType::Dataset) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            auto v = widgets()->field<QAbstractItemView>(p);

            disconnect(v, &QAbstractItemView::clicked, this, &View::sl_datasetClicked);
            disconnect(v, &QAbstractItemView::doubleClicked, this, &View::sl_datasetDoubleClicked);
            disconnect(v, &QAbstractItemView::activated, this, &View::sl_datasetActivated);

            if (v->selectionModel() != nullptr) {
                disconnect(v->selectionModel(), &QItemSelectionModel::selectionChanged, this, &View::sl_datasetSelectionChanged);
                disconnect(v->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::sl_datasetCurrentChanged);
                disconnect(v->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &View::sl_datasetCurrentRowChanged);
                disconnect(v->selectionModel(), &QItemSelectionModel::currentColumnChanged, this, &View::sl_datasetCurrentColumnChanged);
            }

            disconnect(v, &QAbstractItemView::customContextMenuRequested, this, &View::sl_datasetContextMenuRequested);

            TableView* tv = qobject_cast<TableView*>(v);
            if (tv != nullptr) {
                disconnect(tv, &TableView::sg_checkPanelVisibleChanged, this, &View::sl_checkPanelVisibleChanged);
                disconnect(tv, &TableView::sg_checkedRowsChanged, this, &View::sl_checkedRowsChanged);
            }
        }
    }
}

void View::connectToProperties()
{
    for (auto& p : structure()->properties()) {
        if (p.propertyType() == PropertyType::Dataset) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            connect(proxyDataset(p), &ProxyItemModel::rowsInserted, this, &View::sl_proxy_rowsInserted);
            connect(proxyDataset(p), &ProxyItemModel::modelReset, this, &View::sl_proxy_modelReset);

            auto v = widgets()->field<QAbstractItemView>(p);

            Z_CHECK_NULL(v->model());

            connect(v, &QAbstractItemView::clicked, this, &View::sl_datasetClicked);
            connect(v, &QAbstractItemView::doubleClicked, this, &View::sl_datasetDoubleClicked);
            connect(v, &QAbstractItemView::activated, this, &View::sl_datasetActivated);

            Z_CHECK_NULL(v->selectionModel());
            connect(v->selectionModel(), &QItemSelectionModel::selectionChanged, this, &View::sl_datasetSelectionChanged);
            connect(v->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::sl_datasetCurrentChanged);
            connect(v->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &View::sl_datasetCurrentRowChanged);
            connect(v->selectionModel(), &QItemSelectionModel::currentColumnChanged, this, &View::sl_datasetCurrentColumnChanged);

            connect(v, &QAbstractItemView::customContextMenuRequested, this, &View::sl_datasetContextMenuRequested);

            TableView* tv = qobject_cast<TableView*>(v);
            if (tv != nullptr) {
                connect(tv, &TableView::sg_checkPanelVisibleChanged, this, &View::sl_checkPanelVisibleChanged);
                connect(tv, &TableView::sg_checkedRowsChanged, this, &View::sl_checkedRowsChanged);
            }
        }
    }
}

void View::initializeProperties()
{
}

void View::sl_existsChanged()
{
    setExists(model()->isExists());
}

void View::sl_modelEntityNameChanged()
{
    onEntityNameChangedHelper();
}

void View::sl_additionalToolbarDestroyed(QObject* obj)
{
    Q_UNUSED(obj) // объект из _additional_toolbars удалится автоматически при вызове метода additionalToolbars
    emit sg_toolbarRemoved();
}

void View::sl_operationActionActivated(const Operation& op)
{
    executeOperation(op);
}

void View::sl_expandedChanged()
{
    requestUpdateView();
    requestProcessVisibleChange();
    emit sg_activeControlChanged();
}

void View::sl_currentTabChanged(int index)
{
    Q_UNUSED(index)
    requestUpdateAccessRights();
    requestProcessVisibleChange();
    emit sg_activeControlChanged();
}

void View::sl_splitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    requestUpdateAccessRights();
    requestProcessVisibleChange();
    emit sg_activeControlChanged();
}

void View::sl_model_beforeLoad(const LoadOptions& load_options, const DataPropertySet& properties)
{
    onModelBeforeLoad(load_options, properties);
}

void View::sl_model_startLoad()
{
    if (Utils::isAppHalted())
        return;

    saveReloadState();
    onModelStartLoad();
    requestUpdateView();

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));

    onStartLoading();
}

void View::sl_model_startSave()
{
    onModelStartSave();

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));
}

void View::sl_model_startRemove()
{
    onModelStartRemove();

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));
}

void View::sl_blockUiTimeout()
{
    showBlockUiHelper(false);
}

void View::sl_searchTimeout()
{
    bool has_text = !_last_search.isEmpty();
    _last_search.clear();

    if (has_text)
        emit sg_autoSearchTextChanged(_last_search);
    emit sg_autoSearchStatusChanged(false);
}

void View::sl_datasetCurrentCellChangedTimeout()
{
    if (_dataset_current_cell_changed.isValid())
        onDatasetCurrentCellChangedTimeout(_dataset_current_cell_changed, currentIndex(_dataset_current_cell_changed));
}

void View::sl_datasetCurrentRowChangedTimeout()
{
    if (Utils::isAppHalted())
        return;

    for (auto& p : qAsConst(_dataset_current_row_changed)) {
        if (!isPropertyBlocked(p))
            onDatasetCurrentRowChangedTimeout(p, currentIndex(p));
    }
    _dataset_current_row_changed.clear();
}

void View::sl_datasetCurrentColumnChangedTimeout()
{
    if (_dataset_current_column_changed.isValid())
        onDatasetCurrentColumnChangedTimeout(_dataset_current_column_changed, currentIndex(_dataset_current_column_changed));
}

void View::sl_searchWidgetEdited(const QModelIndex& index, bool by_keys)
{
    Q_UNUSED(by_keys)

    if (!index.isValid())
        return;

    QVariant id = _search_widget->currentValue();
    Z_CHECK(!id.isNull() && id.isValid());
    bool ok;
    int id_int = id.toInt(&ok);
    Z_CHECK(ok);

    // надо перезагрузить модель с новым идентификатором. Эти действия должно произойти извне, путем вызова метода View::setModel
    emit sg_searchWidgetEdited(Uid::entity(entityCode(), EntityID(id_int), databaseId()));
}

void View::sl_model_finishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    if (objectExtensionDestroyed())
        return;

    if (_request_load_after_loading) {
        _request_load_after_loading = false;
        if (loadModel())
            return; // вернемся сюда после окончания загрузки
    }

    restoreReloadState();
    onModelFinishLoad(error, load_options, properties);
    requestUpdateView();

    emit sg_model_finishLoad(error, load_options, properties);

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));

    if (error.isError()) {
        processErrorHelper(ObjectActionType::View, error);

    } else if (states().testFlag(ViewState::EditWindow)) {
        fillLookupValues();
    }

    onFinishLoading();
}

void View::sl_model_finishSave(
    const Error& error, const zf::DataPropertySet& requested_properties, const zf::DataPropertySet& saved_properties, const Uid& persistent_uid)
{
    if (objectExtensionDestroyed())
        return;

    onModelFinishSave(error, requested_properties, saved_properties, persistent_uid);
    requestUpdateView();

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));

    if (error.isError())
        processErrorHelper(ObjectActionType::Modify, error);
}

void View::sl_model_finishRemove(const Error& error)
{
    if (objectExtensionDestroyed())
        return;

    onModelFinishRemove(error);
    requestUpdateView();

    widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model()));

    if (error.isError())
        processErrorHelper(ObjectActionType::Remove, error);
}

void View::sl_highlight_itemInserted(const HighlightItem& item)
{
    Q_UNUSED(item)
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_HIGHLIGHT_CHANGED_KEY);

    if (item.property().isValid())
        requestUpdatePropertyStatus(item.property());
}

void View::sl_highlight_itemRemoved(const HighlightItem& item)
{
    Q_UNUSED(item)
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_HIGHLIGHT_CHANGED_KEY);
    if (item.property().isValid())
        requestUpdatePropertyStatus(item.property());
}

void View::sl_highlight_itemChanged(const HighlightItem& item)
{
    sl_highlight_itemInserted(item);
}

void View::sl_highlight_beginUpdate()
{
}

void View::sl_highlight_endUpdate()
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_HIGHLIGHT_CHANGED_KEY);
}

void View::sl_datasetActionActivated(const DataProperty& dataset_property, ObjectActionType type)
{
    auto action = datasetAction(dataset_property, type);
    if (!action->isVisible() || !Utils::isActionEnabled(action))
        return;

    auto index = currentIndex(dataset_property);

    DataProperty column_property;
    if (index.isValid() && dataset_property.columnCount() > index.column())
        column_property = dataset_property.columns().at(index.column());

    onDatasetActionActivated(dataset_property, type, index, column_property);
}

void View::sl_datasetActivated(const QModelIndex& index)
{
    QAbstractItemView* sender = qobject_cast<QAbstractItemView*>(this->sender());
    Z_CHECK_NULL(sender);
    auto p = widgetProperty(sender);
    Z_CHECK(p.isValid());
    onDatasetActivated(p, index);
}

void View::sl_datasetClicked(const QModelIndex& index)
{
    if (states() & ViewState::SelectItemModelDialog)
        return;

    QAbstractItemView* sender = qobject_cast<QAbstractItemView*>(this->sender());
    Z_CHECK_NULL(sender);
    auto dataset_property = widgetProperty(sender);
    Z_CHECK(dataset_property.isValid());

    auto source_index = sourceIndex(index);
    Z_CHECK(source_index.isValid());

    DataProperty column_property;
    if (source_index.isValid() && dataset_property.columnCount() > source_index.column())
        column_property = dataset_property.columns().at(source_index.column());

    onDatasetClicked(dataset_property, source_index, column_property);

    if (_dataset_current_cell_changed_timer->isActive()) {
        _dataset_current_cell_changed_timer->stop();
        sl_datasetCurrentCellChangedTimeout();
    }

    if (_dataset_current_row_changed_timer->isActive()) {
        _dataset_current_row_changed_timer->stop();
        sl_datasetCurrentRowChangedTimeout();
    }

    if (_dataset_current_column_changed_timer->isActive()) {
        _dataset_current_column_changed_timer->stop();
        sl_datasetCurrentColumnChangedTimeout();
    }
}

void View::sl_datasetDoubleClicked(const QModelIndex& index)
{
    if (states() & ViewState::SelectItemModelDialog)
        return;

    QAbstractItemView* sender = qobject_cast<QAbstractItemView*>(this->sender());
    Z_CHECK_NULL(sender);
    auto dataset_property = widgetProperty(sender);
    Z_CHECK(dataset_property.isValid());

    auto source_index = sourceIndex(index);
    Z_CHECK(source_index.isValid());

    DataProperty column_property;
    if (source_index.isValid() && dataset_property.columnCount() > source_index.column())
        column_property = dataset_property.columns().at(source_index.column());

    bool processed = onDatasetDoubleClicked(dataset_property, source_index, column_property);
    if (!processed) {
        if (auto action = widgets()->datasetAction(dataset_property, ObjectActionType::Modify); Utils::isActionEnabled(action))
            processed = onDatasetActionActivated(dataset_property, ObjectActionType::Modify, source_index, column_property);

        if (!processed) {
            if (auto action = widgets()->datasetAction(dataset_property, ObjectActionType::View, false); Utils::isActionEnabled(action))
                onDatasetActionActivated(dataset_property, ObjectActionType::View, source_index, column_property);
        }
    }
}

void View::sl_datasetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    DataProperty p;
    if (!getDatasetSelectionProperty(sender(), p))
        return;

    onDatasetSelectionChanged(p, selected, deselected);
}

void View::sl_datasetCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    DataProperty p;
    if (!getDatasetSelectionProperty(sender(), p))
        return;

    onDatasetCurrentCellChanged(p, sourceIndex(current), sourceIndex(previous));

    _dataset_current_cell_changed = p;
    _dataset_current_cell_changed_timer->start();

    requestUpdateView();
    updateStatusBarErrorInfo();
}

void View::sl_datasetCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    DataProperty p;
    if (!getDatasetSelectionProperty(sender(), p))
        return;

    onDatasetCurrentRowChanged(p, sourceIndex(current), sourceIndex(previous));

    if (!isPropertyBlocked(p)) {
        _dataset_current_row_changed << p;
        _dataset_current_row_changed_timer->start();
    }
}

void View::sl_datasetCurrentColumnChanged(const QModelIndex& current, const QModelIndex& previous)
{
    DataProperty p;
    if (!getDatasetSelectionProperty(sender(), p))
        return;

    onDatasetCurrentColumnChanged(p, sourceIndex(current), sourceIndex(previous));

    _dataset_current_column_changed = p;
    _dataset_current_column_changed_timer->start();
}

void View::sl_datasetContextMenuRequested(const QPoint& point)
{
    Q_UNUSED(point);

    if (states() & ViewState::SelectItemModelDialog)
        return;

    QAbstractItemView* sender = qobject_cast<QAbstractItemView*>(this->sender());
    Z_CHECK_NULL(sender);

    auto p = widgetProperty(sender);
    Z_CHECK(p.isValid());

    QMenu menu(sender);
    getDatasetContextMenu(p, menu);

    if (Utils::compressActionList(menu.actions(), false).first)
        menu.exec(QCursor::pos());
}

void View::afterCreated()
{
}

void View::onDataReady(const QMap<QString, QVariant>& custom_data)
{
    Q_UNUSED(custom_data);
}

bool View::isDataReady() const
{
    return _is_data_ready_called;
}

void View::onWidgetsReady()
{
}

bool View::confirmOperation(const Operation& op)
{
    Q_UNUSED(op)
    return QMessageBox::question(moduleUI(), ZF_TR(ZFT_QUESTION), ZF_TR(ZFT_EXECUTE_QUESTION).arg(op.name()), QMessageBox::Yes | QMessageBox::No)
        == QMessageBox::Yes;
}

void View::onPropertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    EntityObject::onPropertyChanged(p, old_values);
    requestUpdatePropertyStatus(p);
    requestUpdateAccessRights();
}

void View::onAllPropertiesUnBlocked()
{
    EntityObject::onAllPropertiesUnBlocked();

    auto datasets = structure()->propertiesByType(PropertyType::Dataset);
    for (auto& p : qAsConst(datasets)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        _dataset_current_row_changed << p;
        _dataset_current_row_changed_timer->start();
    }
}

void View::onPropertyUnBlocked(const DataProperty& p)
{
    EntityObject::onPropertyUnBlocked(p);

    if (p.isDataset() && !p.options().testFlag(PropertyOption::SimpleDataset)) {
        _dataset_current_row_changed << p;
        _dataset_current_row_changed_timer->start();
    }
}

void View::onConnectionChanged(const ConnectionInformation& info)
{
    EntityObject::onConnectionChanged(info);

    requestUpdateView();
    emit sg_readOnlyChanged(isReadOnly());
}

bool View::onDatasetActionActivated(
    const zf::DataProperty& dataset_property, zf::ObjectActionType action_type, const QModelIndex& index, const zf::DataProperty& column_property)
{
    Q_UNUSED(column_property);

    if (action_type == ObjectActionType::Create) {
        if (index.isValid()) {
            data()->insertRow(dataset_property, index.row() + 1, index.parent());
            setCurrentIndex(dataset_property, cellIndex(dataset_property, index.row() + 1, index.column(), index.parent()));
        } else {
            data()->appendRow(dataset_property);
            setCurrentIndex(
                dataset_property, cellIndex(dataset_property, data()->rowCount(dataset_property) - 1, qMax(firstVisibleColumn(dataset_property), 0)));
        }

        widgets()->field(dataset_property)->setFocus();
        return true;

    } else if (action_type == ObjectActionType::CreateLevelDown) {
        widgets()->field(dataset_property)->setFocus();
        return false;

    } else if (action_type == ObjectActionType::Remove) {
        Rows rows = selectedRows(dataset_property);
        bool res = false;
        if (!rows.isEmpty() && doAskRemoveDatasetRows(dataset_property, rows)) {
            res = true;
            data()->removeRows(dataset_property, rows);
        }

        widgets()->field(dataset_property)->setFocus();
        return res;

    } else if (action_type == ObjectActionType::Modify) {
        if (index.isValid())
            return editCellInternal(dataset_property, index, nullptr, QString());
    }

    return false;
}

void View::checkDatasetActionProperties(const DataProperty& dataset_property, ObjectActionType action_type, bool& enabled, bool& visible) const
{
    enabled = isStandardActionEnabledDefault(dataset_property, action_type, false);
    visible = isStandardActionVisibleDefault(dataset_property, action_type, false);
}

bool View::isStandardActionVisibleDefault(const DataProperty& dataset_property, ObjectActionType action_type, bool ignore_read_only) const
{
    bool read_only
        = !dataset_property.options().testFlag(PropertyOption::IgnoreReadOnly) && (!ignore_read_only && (isReadOnly() || isPropertyReadOnly(dataset_property)));

    switch (action_type) {
    case ObjectActionType::View:
        return dataset_property.options().testFlag(PropertyOption::EnableViewOperation);

    case ObjectActionType::Create:
        return !read_only;

    case ObjectActionType::CreateLevelDown:
        return !read_only && dataset_property.dataType() == DataType::Tree;

    case ObjectActionType::Remove:
        return !read_only;

    case ObjectActionType::Modify:
        return !read_only;

    case ObjectActionType::Export:
    case ObjectActionType::Print:
    case ObjectActionType::Configurate:
    case ObjectActionType::Administrate:
        return true;

    case ObjectActionType::Undefined:
        Z_HALT_INT;
        return true;
    }
    return true;
}

bool View::isStandardActionVisibleDefault(const PropertyID& dataset_property_id, ObjectActionType action_type, bool ignore_read_only) const
{
    return isStandardActionVisibleDefault(property(dataset_property_id), action_type, ignore_read_only);
}

bool View::isStandardActionEnabledDefault(const DataProperty& dataset_property, ObjectActionType action_type, bool ignore_read_only) const
{
    auto current_index = currentIndex(dataset_property);
    auto current_column = currentColumn(dataset_property);

    bool read_only_dataset = (!dataset_property.options().testFlag(PropertyOption::IgnoreReadOnly)
                                 && (!ignore_read_only && (isReadOnly() || isPropertyReadOnly(dataset_property))))
        || isBlockControls();
    bool read_only_cell
        = (!dataset_property.options().testFlag(PropertyOption::IgnoreReadOnly) && !current_column.options().testFlag(PropertyOption::IgnoreReadOnly)
              && (!ignore_read_only && (isReadOnly() || isPropertyReadOnly(dataset_property) || current_column.options().testFlag(PropertyOption::ReadOnly))))
        || isBlockControls();

    bool res = true;
    if (action_type == ObjectActionType::CreateLevelDown) {
        res = !read_only_dataset && dataset_property.dataType() == DataType::Tree;

    } else {
        if (action_type == ObjectActionType::Remove) {
            res = !read_only_dataset && current_index.isValid();

        } else if (action_type == ObjectActionType::Modify) {
            res = !read_only_cell && current_index.isValid();

        } else if (action_type == ObjectActionType::View) {
            res = current_index.isValid();

        } else {
            if (action_type == ObjectActionType::Create)
                res = !read_only_dataset;
            else
                res = true;
        }
    }

    return res;
}

bool View::isStandardActionEnabledDefault(const PropertyID& dataset_property_id, ObjectActionType action_type, bool ignore_read_only) const
{
    return isStandardActionEnabledDefault(property(dataset_property_id), action_type, ignore_read_only);
}

void View::updateAction(const Operation& operation, const DataProperty& dataset_property, ObjectActionType action_type, bool ignore_read_only)
{
    QAction* a = operationAction(operation);
    Z_CHECK_NULL(a);
    a->setEnabled(isStandardActionEnabledDefault(dataset_property, action_type, ignore_read_only));
    a->setVisible(isStandardActionVisibleDefault(dataset_property, action_type, ignore_read_only));
}

void View::updateAction(const Operation& operation, const PropertyID& dataset_property_id, ObjectActionType action_type, bool ignore_read_only)
{
    updateAction(operation, property(dataset_property_id), action_type, ignore_read_only);
}

void View::updateAction(const OperationID& operation_id, const DataProperty& dataset_property, ObjectActionType action_type, bool ignore_read_only)
{
    updateAction(operation(operation_id), dataset_property, action_type, ignore_read_only);
}

void View::updateAction(const OperationID& operation_id, const PropertyID& dataset_property_id, ObjectActionType action_type, bool ignore_read_only)
{
    updateAction(operation(operation_id), dataset_property_id, action_type, ignore_read_only);
}

void View::updateAction(const Operation& operation, ObjectActionType action_type)
{
    QAction* a = operationAction(operation);
    Z_CHECK_NULL(a);
    a->setVisible(hasDirectAccessRight(Utils::accessForAction(action_type)));
}

void View::updateAction(const OperationID& operation_id, ObjectActionType action_type)
{
    updateAction(operation(operation_id), action_type);
}

void View::updateAction(QAction* operation_action, ObjectActionType action_type)
{
    Z_CHECK_NULL(operation_action);
    bool access = hasDirectAccessRight(Utils::accessForAction(action_type));
    if (!operation_action->isVisible() || access)
        return;

    operation_action->setVisible(false);
}

void View::setDatasetToolbarHidden(const DataProperty& property, bool hidden)
{
    Z_CHECK(property.isDataset() && !property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property);
    info->toolbar_hidded = hidden? Answer::Yes : Answer::No;
    requestUpdatePropertyStatus(property);
}

void View::setDatasetToolbarHidden(const PropertyID& property_id, bool hidden)
{
    setDatasetToolbarHidden(property(property_id), hidden);
}

bool View::doAskRemoveDatasetRows(const DataProperty& p, const Rows& rows)
{
    Q_UNUSED(p)
    Q_UNUSED(rows)
    return true;
}

void View::onDatasetClicked(const DataProperty& dataset_property, const QModelIndex& index, const DataProperty& column_property)
{
    Q_UNUSED(dataset_property);
    Q_UNUSED(column_property);
    Q_UNUSED(index);
}

bool View::onDatasetDoubleClicked(const DataProperty& dataset_property, const QModelIndex& index, const DataProperty& column_property)
{
    Q_UNUSED(dataset_property);
    Q_UNUSED(column_property);
    Q_UNUSED(index);
    return false;
}

void View::onDatasetActivated(const DataProperty& p, const QModelIndex& index)
{
    Q_UNUSED(p)
    Q_UNUSED(index)
}

void View::onDatasetSelectionChanged(const DataProperty& p, const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(p)
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
}

void View::onDatasetCurrentCellChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void View::onDatasetCurrentRowChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void View::onDatasetCurrentColumnChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void View::onDatasetCurrentCellChangedTimeout(const DataProperty& p, const QModelIndex& current)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
}

void View::onDatasetCurrentRowChangedTimeout(const DataProperty& p, const QModelIndex& current)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
}

void View::onDatasetCurrentColumnChangedTimeout(const DataProperty& p, const QModelIndex& current)
{
    Q_UNUSED(p)
    Q_UNUSED(current)
}

void View::onModelFirstLoadRequest()
{
}

bool View::onEditCell(const DataProperty& dataset_property, const QModelIndex& cell_index, QKeyEvent* e, const QString& text)
{
    Error error;

    if ((cell_index.flags() & Qt::ItemIsEditable) == 0)
        Z_HALT("Редактирование в ячейке не поддерживается на уровне модели");

    DataProperty column = indexColumn(cell_index);
    bool res = false;

    if (column.dataType() == DataType::Bool) {
        // Это чекбокс. Реагируем на нажатие пробела
        if (text.isEmpty() || text == " ")
            error = data()->setCellValue(dataset_property, cell_index.row(), column, cell_index.column(), !cell(cell_index.row(), column).toBool(),
                Qt::DisplayRole, cell_index.parent(), false, QLocale::AnyLanguage);
        res = true;

    } else {
        auto view = object<QAbstractItemView>(dataset_property);

        if (!editingDataset().isValid() || QApplication::focusWidget() == view) {
            object<QAbstractItemView>(dataset_property)->edit(proxyIndex(cell_index));
            res = true;
        }

        if (QApplication::focusWidget() == view) {
            // случилось непонятное - редактор не активирован
            Core::logError("View::onEditCell error");
            return res;
        }

        // Активен редактор - надо передать туда введенный символ
        if (e && e->key() == Qt::Key_Backspace) {
            // При нажатии Backspace очистить содержимое
            Utils::clearWidget(QApplication::focusWidget());

        } else if (!text.isEmpty()) {
            QApplication::sendEvent(QApplication::focusWidget(), e);
        }
    }

    if (error.isError())
        Core::error(error);

    return res;
}

void View::getDatasetContextMenu(const DataProperty& p, QMenu& m)
{
    if (widgets()->datasetAction(p, ObjectActionType::Modify)->isVisible())
        m.addAction(widgets()->datasetAction(p, ObjectActionType::Modify));

    if (widgets()->datasetAction(p, ObjectActionType::Create)->isVisible())
        m.addAction(widgets()->datasetAction(p, ObjectActionType::Create));

    m.addSeparator();

    if (widgets()->datasetAction(p, ObjectActionType::Remove)->isVisible())
        m.addAction(widgets()->datasetAction(p, ObjectActionType::Remove));
}

Error View::onLoadConfiguration()
{
    return {};
}

void View::afterLoadConfiguration()
{
}

Error View::onSaveConfiguration() const
{
    return {};
}

bool View::isSaveSplitterConfiguration(const QSplitter* splitter) const
{
    Q_UNUSED(splitter)
    return true;
}

void View::onModelBeforeLoad(const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
}

bool View::processOperation(const Operation& op, Error& error)
{
    Q_UNUSED(op)
    Q_UNUSED(error)
    return false;
}

void View::updateWidgetStatus(QWidget* w, const DataProperty& p)
{
    Z_CHECK_NULL(w);

    // надо исключить изменение доступности тулбаров
    for (auto& ds : structure()->propertiesByType(PropertyType::Dataset)) {
        if (ds.options().testFlag(PropertyOption::SimpleDataset))
            continue;
        if (w == widgets()->datasetToolbarWidget(ds) || w == widgets()->datasetToolbar(ds)) {
            return;
        }
    }

    if (p.isDataset() && !p.options().testFlag(PropertyOption::SimpleDataset)) {
        auto info = propertyInfo(p);
        if (info->toolbar_hidded != Answer::Undefined)
            widgets()->setDatasetToolbarHidden(p, info->toolbar_hidded == Answer::Yes, true);
        else if (info->toolbar_has_visible_actions != Answer::Undefined)
            widgets()->setDatasetToolbarHidden(p, info->toolbar_has_visible_actions == Answer::No, true);
        return;
    }

    auto need = isNeedWidgetReadonly(w, p);
    if (need == Answer::Undefined)
        return;

    setWidgetReadOnly(w, need == Answer::Yes);
}

Answer View::isNeedWidgetReadonly(QWidget* w, const DataProperty& p) const
{
    Z_CHECK_NULL(w);

    bool need_change = (p.isValid() && (p.propertyType() != PropertyType::Dataset || p.options().testFlag(PropertyOption::SimpleDataset))
        && !p.options().testFlag(PropertyOption::IgnoreReadOnly));
    if (!need_change)
        return Answer::Undefined;

    return isReadOnly() || isBlockControls() || isPropertyReadOnly(p) ? Answer::Yes : Answer::No;
}

bool View::isWidgetReadOnly(const QWidget* w)
{
    return Utils::isWidgetReadOnly(w);
}

bool View::setWidgetReadOnly(QWidget* w, bool read_only)
{
    return Utils::setWidgetReadOnly(w, read_only);
}

bool View::setWidgetReadOnly(const QString& widget_name, bool read_only)
{
    return setWidgetReadOnly(object<QWidget>(widget_name), read_only);
}

bool View::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Destroy || event->type() == QEvent::Timer || _is_constructing || _is_destructing || !Core::isActive()
        || objectExtensionDestroyed())
        return EntityObject::eventFilter(obj, event);

    if (processTopWidgetFilter(obj, event))
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::HoverMove:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave: {
        // блокировка действий пользователя при обработке processEvents
        if (Utils::isProcessingEvent()) {
            event->ignore();
            return true;
        }

        // обработка нажатия кнопок в наборах данных
        if (event->type() == QEvent::KeyPress) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (w != nullptr) {
                DataProperty current_dataset = currentDataset();
                if (current_dataset.isValid() && !current_dataset.options().testFlag(PropertyOption::SimpleDataset)
                    && object<QAbstractItemView>(current_dataset) == w) {
                    if (processDatasetKeyPressHelper(current_dataset, qobject_cast<QAbstractItemView*>(w), static_cast<QKeyEvent*>(event)))
                        return true;
                }
            }
        }

        if (event->type() == QEvent::MouseButtonPress) {
            // запоминаем начало потенциального перетаскивания
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            _drag_start_pos = me->pos();

        } else if (event->type() == QEvent::MouseButtonRelease) {
            // сбрасываем начало перетаскивания
            _drag_start_pos = QPoint();

        } else if (event->type() == QEvent::MouseMove) {
            // запоминаем начало потенциального перетаскивания
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if ((me->buttons() & Qt::LeftButton) && !_drag_start_pos.isNull()
                && (me->pos() - _drag_start_pos).manhattanLength() >= QApplication::startDragDistance()) {
                QWidget* drag_widget = Utils::getMainWidget(QApplication::widgetAt(me->globalPos()));
                if (drag_widget != nullptr && isCatchDragDropEvents(drag_widget)) {
                    QPixmap drag_pixmap;
                    QMimeData* mime_data = new QMimeData;
                    if (isDragBegin(drag_widget, widgetProperty(drag_widget), me->globalPos(), mime_data, drag_pixmap)) {
                        QDrag* drag = new QDrag(drag_widget);
                        drag->setPixmap(drag_pixmap);
                        drag->setHotSpot(QPoint(drag_pixmap.width() / 2, drag_pixmap.height() / 2));
                        drag->setMimeData(mime_data);
                        drag->exec(Qt::MoveAction);

                    } else {
                        delete mime_data;
                    }
                }
                _drag_start_pos = QPoint();
            }
        }

        break;
    }
    case QEvent::ParentChange: {
        if (obj == _body || obj == _main_widget || !Utils::hasParent(obj, _main_widget)) {
            requestInitTabOrder();
            requestUpdateParentView();
        }
        break;
    }

    case QEvent::ActionAdded: {
        QActionEvent* ae = static_cast<QActionEvent*>(event);
        auto aw = ae->action()->associatedWidgets();
        for (auto w : qAsConst(aw)) {
            if (auto tb = qobject_cast<QToolButton*>(w)) {
                Utils::prepareToolbarButton(tb);
            }
        }
        break;
    }

    case QEvent::ChildAdded: {
        break;
    }
    case QEvent::Resize:
        updateBlockUiGeometry(false);
        break;

    case QEvent::Show:
        updateBlockUiGeometry(false);
        requestUpdateParentView();
        requestInitTabOrder();
        break;

    case QEvent::ShowToParent:
        requestUpdateParentView();
        requestInitTabOrder();
        break;

    case QEvent::FocusIn:
    case QEvent::FocusOut: {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (event->type() == QEvent::FocusIn) {
            _last_focused = w;
        }

        // Сохраняем последний набор данных, который имел фокус ввода (или просто считался активным)
        if (w != nullptr) {
            DataProperty p = widgetProperty(w);
            if (p.isValid() && p.propertyType() == PropertyType::Dataset) {
                _last_active_dataset = p;
            }

            requestUpdateWidgetStatus(w);
        }

        updateStatusBarErrorInfo();

        emit sg_activeControlChanged();
        break;
    }
    case QEvent::DragEnter:
        if (obj->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (isCatchDragDropEvents(w) && onDragEnterHelper(w, static_cast<QDragEnterEvent*>(event)))
                return true;
        }
        break;
    case QEvent::DragMove:
        if (obj->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (isCatchDragDropEvents(w) && onDragMoveHelper(w, static_cast<QDragMoveEvent*>(event)))
                return true;
        }
        break;
    case QEvent::Drop:
        if (obj->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (isCatchDragDropEvents(w) && onDropHelper(w, static_cast<QDropEvent*>(event)))
                return true;
        }
        break;
    case QEvent::DragLeave:
        if (obj->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(obj);
            if (isCatchDragDropEvents(w) && onDragLeaveHelper(w, static_cast<QDragLeaveEvent*>(event)))
                return true;
        }
        break;

    default:
        break;
    }

    return EntityObject::eventFilter(obj, event);
}

zf::Error View::processInboundMessage(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)

    if (Z_VALID_HANDLE(subscribe_handle) && _lookup_subscribe.contains(subscribe_handle)) {
        // реакция на изменения в lookup для таблиц, чтобы обновить отображение
        UidList uids_changed;
        EntityCodeList entity_changed;
        if (message.messageType() == zf::MessageType::DBEventEntityChanged) {
            DBEventEntityChangedMessage msg(message);
            uids_changed = msg.entityUids();
            entity_changed = msg.entityCodes();

        } else if (message.messageType() == zf::MessageType::DBEventEntityRemoved) {
            uids_changed = DBEventEntityRemovedMessage(message).entityUids();
        }

        if (!uids_changed.isEmpty() || !entity_changed.isEmpty()) {
            for (auto& c : structure()->propertiesByType(PropertyType::ColumnFull)) {
                if (c.lookup() == nullptr || c.lookup()->type() != LookupType::Dataset || c.dataset().options().testFlag(PropertyOption::SimpleDataset))
                    continue;

                for (auto& u : uids_changed) {
                    if (u.code() == c.lookup()->listEntity().code())
                        object<QAbstractItemView>(c.dataset())->viewport()->update();
                }
                for (auto& u : entity_changed) {
                    if (u == c.lookup()->listEntity().code())
                        object<QAbstractItemView>(c.dataset())->viewport()->update();
                }
            }
        }
    }
    return {};
}

bool View::isHighlightViewIsCheckRequired() const
{
    return !isReadOnly() && moduleDataOptions().testFlag(ModuleDataOption::HighlightEnabled) && EntityObject::isHighlightViewIsCheckRequired();
}

bool View::isLoading() const
{
    return _loading;
}

bool View::isLoadingComplete() const
{
    if (isLoading())
        return false;

    return _model->isLoadingComplete();
}

ChangeInfo View::reloadReason() const
{
    return _reload_reason;
}

UidList View::reloadReason(ObjectActionType type) const
{
    if (!_reload_reason.isValid() || !_reload_reason.message().isValid())
        return {};

    switch (type) {
    case ObjectActionType::Create: {
        if (_reload_reason.message().messageType() != MessageType::DBEventEntityCreated)
            return {};

        return DBEventEntityCreatedMessage(_reload_reason.message()).entityUids();
    }

    case ObjectActionType::Modify: {
        if (_reload_reason.message().messageType() != MessageType::DBEventEntityChanged)
            return {};

        return DBEventEntityChangedMessage(_reload_reason.message()).entityUids();
    }

    case ObjectActionType::Remove: {
        if (_reload_reason.message().messageType() != MessageType::DBEventEntityRemoved)
            return {};

        return DBEventEntityRemovedMessage(_reload_reason.message()).entityUids();
    }

    default:
        Z_HALT(QString("unsupported %1").arg(Utils::qtEnumToString(type)));
        break;
    }

    return {};
}

bool View::exportToExcel(const DataProperty& property, const QString& file_name) const
{
    Z_CHECK(this->property(property.id()).isDataset());

    QString fname = zf::Utils::validFileName(file_name);
    if (fname.isEmpty()) {
        QString dataset_name = property.name();
        bool ok;
        dataset_name.toInt(&ok);
        if (!ok) {
            fname = zf::Utils::validFileName(dataset_name);

        } else {
            if (structure()->propertiesByType(PropertyType::Dataset).count() == 1) {
                auto name_p = structure()->nameProperty();
                if (name_p.isValid())
                    fname = zf::Utils::validFileName(toString(name_p));

                if (fname.isEmpty())
                    fname = zf::Utils::validFileName(plugin()->getModuleInfo().name());
            }
        }
    }
    if (fname.isEmpty())
        fname = ZF_TR(ZFT_TABLE).toLower();

    if (!comp(QFileInfo(fname).suffix(), "xlsx"))
        fname += ".xlsx";

    fname = zf::Utils::getSaveFileName(ZF_TR(ZFT_TMC_EXPORT_TABLE_EXCEL_1), fname, ZF_TR(ZFT_XLSX_SAVE_FORMAT));
    if (fname.isEmpty())
        return false;

    auto error = Core::waitFunction(
        ZF_TR(ZFT_TMC_EXPORTING_TABLE_EXCEL), [this, property, fname]() -> zf::Error { return Utils::itemModelToExcel(proxyDataset(property), fname, this); },
        "", false, false);
    if (error.isError()) {
        Core::error(error);
        return false;
    }
    Utils::openFile(fname);
    return true;
}

bool View::exportToExcel(const PropertyID& dataset_property_id, const QString& file_name) const
{
    return exportToExcel(property(dataset_property_id), file_name);
}

void View::processCallbackInternal(int key, const QVariant& data)
{
    EntityObject::processCallbackInternal(key, data);

    if (_model == nullptr)
        return;

    if (key == Framework::VIEW_REQUEST_UPDATE_ACCESS_RIGHTS) {
        updateAccessRightsHelper();

    } else if (key == Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS) {
        updateAllWidgetsStatusHelper();

    } else if (key == Framework::VIEW_REQUEST_UPDATE_WIDGET_STATUS) {
        for (auto& w : qAsConst(_widgets_to_update)) {
            if (w.isNull())
                continue;

            updateWidgetStatus(w, widgetProperty(w));
        }
        _widgets_to_update.clear();

    } else if (key == Framework::VIEW_INIT_TAB_ORDER_KEY) {
        initTabOrder();

    } else if (key == Framework::VIEW_INVALIDATED_KEY) {
        auto info = data.value<ChangeInfo>();

        if (!loadModel()) {
            onFinishLoading();

        } else {
            _reload_reason = info;
        }

        onInvalidated(info);

    } else if (key == Framework::VIEW_UPDATE_BLOCK_UI_GEOMETRY_KEY) {
        updateBlockUiGeometryHelper();

    } else if (key == Framework::VIEW_HIDE_BLOCK_UI_KEY) {
#ifdef RNIKULENKOV
//        qDebug() << "process VIEW_HIDE_BLOCK_UI_KEY" << objectName();
#endif

        hideBlockUiHelper();

    } else if (key == Framework::VIEW_REQUEST_UPDATE_ALL_PROPERTY_STATUS) {
        for (auto& p : structure()->propertiesMain()) {
            updateWidgetStatus(widget(p), p);
        }

    } else if (key == Framework::VIEW_UPDATE_PROPERTY_STATUS_KEY) {
        for (auto& p : qAsConst(_properties_to_update)) {
            Z_CHECK(p.isValid());
            updateWidgetStatus(widget(p), p);
        }
        _properties_to_update.clear();

    } else if (key == Framework::VIEW_HIGHLIGHT_CHANGED_KEY) {
        onHighlightChanged(highlight(false));
        for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;
            object<QAbstractItemView>(p)->viewport()->update();
        }

    } else if (key == Framework::VIEW_COMPRESS_ACTIONS_KEY) {
        compressActionsHelper();

    } else if (key == Framework::VIEW_LOAD_CONDITION_FILTER_KEY) {
        // загружаем фильтр по условиям
        for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
            if (p.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            if (hasConditionFilter(p)) {
                Error error = Core::fr()->getConditionFilter(conditionFilterUniqueKey(), p, conditionFilter(p));
                if (error.isError())
                    Core::logError(error);
            }

            // контролируем изменение фильтра
            connect(conditionFilter(p), &ComplexCondition::sg_changed, this, [&, p]() { conditionFilterChanged(p); });
            connect(conditionFilter(p), &ComplexCondition::sg_inserted, this, [&, p]() { conditionFilterChanged(p); });
            connect(conditionFilter(p), &ComplexCondition::sg_removed, this, [&, p]() { conditionFilterChanged(p); });

            if (hasConditionFilter(p))
                conditionFilterChanged(p);
        }

    } else if (key == Framework::VIEW_AFTER_LOAD_CONFIG_KEY) {
        afterLoadConfiguration();

    } else if (key == Framework::VIEW_UPDATE_PARENT_VIEW) {
        updateParentView();

    } else if (key == Framework::VIEW_AUTO_SELECT_FIRST_ROW) {
        processAutoSelectFirstRow();

    } else if (key == Framework::VIEW_REQUEST_PROCESS_VISIBLE_CHANGE) {
        processVisibleChange();

    } else if (key == Framework::VIEW_DATA_READY) {
        onDataReadyHelper();
    }
}

QString View::customConfigurationCodeInternal() const
{
    QString cname = EntityObject::customConfigurationCodeInternal();
    if (!cname.isEmpty())
        cname += QStringLiteral("_");

    if (isMdiWindow())
        return cname + QStringLiteral("mdi");
    if (isEditWindow())
        return cname + QStringLiteral("ed");
    if (isViewWindow())
        return cname + QStringLiteral("vi");
    if (isEmbeddedWindow())
        return cname + QStringLiteral("em");

    return cname + QStringLiteral("v");
}

void View::registerError(const Error& error, ObjectActionType type)
{
    Q_UNUSED(type);
    if (_block_show_errors == 0)
        Core::postError(error);
}

void View::onDataInvalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    EntityObject::onDataInvalidate(p, invalidate, info);
}

void View::onDataInvalidateChanged(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    EntityObject::onDataInvalidateChanged(p, invalidate, info);

    if (objectExtensionDestroyed())
        return;

    if (invalidate) {
        if (_visible_status) {
            ChangeInfo new_info;
            if (internalCallbackManager()->isRequested(this, Framework::VIEW_INVALIDATED_KEY)) {
                new_info = internalCallbackManager()->data(this, Framework::VIEW_INVALIDATED_KEY).value<ChangeInfo>();
                if (!new_info.isValid()) {
                    // нет старой инфы - ерем новую
                    new_info = info;

                } else {
                    if (info.isValid()) {
                        // есть старая и новая инфа - смешиваем
                        auto check_info = ChangeInfo::compress(new_info, info);
                        if (check_info.isValid())
                            new_info = check_info;
                    }
                }

            } else {
                new_info = info;
            }

            internalCallbackManager()->addRequest(this, Framework::VIEW_INVALIDATED_KEY, QVariant::fromValue(new_info));

            onStartLoading();

        } else {
            _data_update_required = true;
        }

    } else {
        widgets()->requestUpdateWidget(p, DataWidgets::modelStatusToUpdateReason(model()));
    }
}

void View::onInvalidated(const zf::ChangeInfo& info)
{
    Q_UNUSED(info);
}

bool View::highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const
{
    // при изменении тут - поправить HighlightDialog::highlightViewGetSortOrder
    if (property1.propertyType() == PropertyType::Cell || property2.propertyType() == PropertyType::Cell) {
        if (property1.propertyType() != PropertyType::Cell)
            return true;
        if (property2.propertyType() != PropertyType::Cell)
            return false;

        if (property1.dataset() != property2.dataset())
            return highlightViewGetSortOrder(property1.dataset(), property2.dataset());

        QModelIndex index1 = findDatasetRowID(property1.dataset(), property1.rowId());
        index1 = cellIndex(index1.row(), property1.column(), index1.parent());

        QModelIndex index2 = findDatasetRowID(property2.dataset(), property2.rowId());
        index2 = cellIndex(index2.row(), property2.column(), index2.parent());

        return comp(index1, index2);
    }

    QWidget* w1 = Utils::getMainWidget(widgets()->field(property1, false));
    QWidget* w2 = Utils::getMainWidget(widgets()->field(property2, false));

    int pos1 = -1;
    if (w1 != nullptr)
        pos1 = _tab_order.indexOf(w1);

    int pos2 = -1;
    if (w2 != nullptr)
        pos2 = _tab_order.indexOf(w2);

    if (pos1 < 0 && pos2 < 0)
        return property1 < property2;

    if (pos1 < 0)
        return false;

    if (pos2 < 0)
        return true;

    return pos1 < pos2;
}

bool View::isHighlightViewIsSortOrderInitialized() const
{
    return !_tab_order.isEmpty();
}

ComplexCondition* View::getConditionFilter(const DataProperty& dataset) const
{
    return conditionFilter(dataset);
}

bool View::calculateConditionFilter(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    auto c_filter = conditionFilter(dataset);
    if (c_filter->isEmpty())
        return true;

    QList<ModelPtr> data_not_ready;
    Error error;
    bool res = c_filter->calculateOnView(this, data_not_ready, error, { DataStructure::propertyRow(dataset, datasetRowID(dataset, row, parent)) });

    if (!data_not_ready.isEmpty()) {
        // надо обновить отфильтровку после окончания загрузки данных
        auto info = propertyInfo(dataset);

        for (auto const& p : qAsConst(data_not_ready)) {
            if (!p->isLoading())
                continue; // это странно, но на всякий случай

            for (auto& wi : qAsConst(info->condition_lookup_load_waiting_info)) {
                if (wi.lookup_model.data() == p.get())
                    return false;
            }

            if (info->condition_lookup_load_waiting_info.isEmpty()) {
                // qDebug() << "+++++++++ 4";
                const_cast<View*>(this)->blockUi();
            }

            auto connection = connect(p.get(), &Model::sg_finishLoad, this, &View::sl_conditionLookupModelLoaded);
            info->condition_lookup_load_waiting_info << PropertyInfo::ConditionLookupLoadWaitingInfo { p.get(), connection };
        }
    }

    return res;
}

void View::sl_conditionLookupModelLoaded(const Error&, const LoadOptions&, const DataPropertySet&)
{
    Model* m_finished = qobject_cast<Model*>(sender());

    for (auto& dataset : structure()->propertiesByType(PropertyType::Dataset)) {
        auto info = propertyInfo(dataset);
        if (info->condition_lookup_load_waiting_info.isEmpty())
            continue;

        for (int i = info->condition_lookup_load_waiting_info.count() - 1; i >= 0; i--) {
            auto wi = info->condition_lookup_load_waiting_info.at(i);
            if (wi.lookup_model.isNull() || wi.lookup_model == m_finished) {
                disconnect(wi.connection);
                info->condition_lookup_load_waiting_info.removeAt(i);
            }
        }

        if (info->condition_lookup_load_waiting_info.isEmpty()) {
            filter()->refilter(dataset); // все загрузилось
            // qDebug() << "--------- 3";
            const_cast<View*>(this)->unBlockUi();
        }
    }
}

void View::sl_lookupModelLoaded(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    Model* m_finished = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(m_finished);
    auto property = _lookup_model_map.value(m_finished);
    Z_CHECK(property.isValid());

    if (error.isOk())
        fillLookupValues(property);

    registerHighlightCheck(property);
}

void View::sl_lookupModelChanged()
{
    auto is = qobject_cast<ItemSelector*>(sender());
    Z_CHECK_NULL(is);
    registerHighlightCheck(widgetProperty(is));
}

void View::sl_checkedRowsChanged()
{
    TableView* sender = qobject_cast<TableView*>(this->sender());
    Z_CHECK_NULL(sender);
    auto p = widgetProperty(sender);
    Z_CHECK(p.isValid());

    onCheckedRowsChanged(p);
    requestUpdateView();
}

void View::sl_checkPanelVisibleChanged()
{
    TableView* sender = qobject_cast<TableView*>(this->sender());
    Z_CHECK_NULL(sender);
    auto p = widgetProperty(sender);
    Z_CHECK(p.isValid());

    onCheckPanelVisibleChanged(p);
    requestUpdateView();
}

void View::sl_error(ObjectActionType type, const Error& error)
{
    processErrorHelper(type, error);
}

void View::sl_parentViewUiBlocked()
{
    hideBlockUiHelper();
}

void View::sl_proxy_rowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    Z_CHECK_NULL(model);
    autoSelectFirstRow(data()->datasetProperty(model));
}

void View::sl_proxy_modelReset()
{
    QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(sender());
    Z_CHECK_NULL(model);
    autoSelectFirstRow(data()->datasetProperty(model));
}

void View::sl_containerObjectAdded(QObject* obj)
{
    auto w = qobject_cast<QWidget*>(obj);
    if (w == nullptr)
        return;

    DataProperty p = widgetProperty(w);
    if (!p.isValid()) // свойства инициализируются при создании представления
        prepareWidget(w, DataProperty());

    requestInitTabOrder();
    requestProcessVisibleChange();
}

void View::sl_containerObjectRemoved(QObject* obj)
{
    auto w = qobject_cast<QWidget*>(obj);
    if (w != nullptr)
        disconnectFromWidget(w);

    if (!objectExtensionDestroyed()) {
        requestInitTabOrder();
        requestProcessVisibleChange();
    }
}

void View::sl_widgetsUpdated()
{
    if (_is_widgets_ready_called)
        return;

    _is_widgets_ready_called = true;
    onWidgetsReady();
}

void View::sl_topWidgetDestroyed(QObject* obj)
{
    stopProcessTopWidgetHide(obj);
}

void View::sl_refiltered(const DataProperty& dataset_property)
{
    onRefiltered(dataset_property);
}

void View::sl_onModelFirstLoadRequest()
{
    onModelFirstLoadRequest();
}

Error View::convertDatasetItemValue(
    const QModelIndex& index, const QVariant& original_value, const VisibleValueOptions& options, QString& value, QList<ModelPtr>& data_not_ready) const
{
    Z_CHECK(index.isValid());
    QModelIndex source_idx = sourceIndex(index);
    Z_CHECK(source_idx.isValid());

    DataProperty dataset = data()->datasetProperty(source_idx.model());
    DataProperty column = propertyColumn(dataset, source_idx.column());
    return getDatasetCellVisibleValue(source_idx.row(), column, source_idx.parent(), original_value, options, value, data_not_ready);
}

void View::getDatasetRowVisibleInfo(const QModelIndex& index, QModelIndex& visible_index) const
{
    Z_CHECK(index.isValid());
    visible_index = proxyIndex(index);
}

void View::getDatasetColumnVisibleInfo(const QAbstractItemModel* model, int column, bool& visible_now, bool& can_be_visible, int& visible_pos) const
{
    Z_CHECK(column >= 0);
    auto dataset_p = data()->datasetProperty(model);
    Z_CHECK(dataset_p.isValid());

    if (dataset_p.options().testFlag(PropertyOption::SimpleDataset)) {
        visible_now = true;
        can_be_visible = true;
        visible_pos = column;

    } else {
        if (column >= horizontalRootHeaderItem(dataset_p)->bottomCount()) {
            visible_now = false;
            can_be_visible = false;
            visible_pos = -1;

        } else {
            auto root = horizontalRootHeaderItem(dataset_p);
            auto item = root->bottomItem(column);
            visible_now = !item->isHidden();
            can_be_visible = !item->isPermanentHidded();
            visible_pos = item->bottomVisualPos(Qt::SortOrder::AscendingOrder, true);
        }
    }
}

QString View::getDatasetColumnName(const QAbstractItemModel* model, int column) const
{
    Z_CHECK(column >= 0);
    auto dataset_p = data()->datasetProperty(model);
    Z_CHECK(dataset_p.isValid());

    if (dataset_p.options().testFlag(PropertyOption::SimpleDataset)) {
        if (column < dataset_p.columnCount())
            return dataset_p.columns().at(column).name();
        else
            return QString::number(column);

    } else {
        auto root_item = horizontalRootHeaderItem(dataset_p);

        if (column < root_item->bottomCount()) {
            auto item = root_item->bottomItem(column);
            QStringList parent_names;
            parent_names << item->label();
            while (item->parent() != root_item) {
                item = item->parent();
                parent_names.prepend(item->label());
            }

            return parent_names.join("; ");

        } else {
            return QString::number(column);
        }
    }
}

void View::getPropertyLookupModel(const DataProperty& property, ModelPtr& model, DataFilterPtr& filter, DataProperty& lookup_column_display,
    DataProperty& lookup_column_id, int& lookup_role_display, int& lookup_role_id) const
{
    Z_CHECK(structure()->contains(property));
    Z_CHECK(property.lookup() != nullptr && property.lookup()->type() == LookupType::Dataset && !property.lookup()->listEntity().isValid());

    PropertyID id_lookup_column_display;
    PropertyID id_lookup_column_id;
    getDynamicLookupModel(property, model, filter, id_lookup_column_display, id_lookup_column_id, lookup_role_display, lookup_role_id);
    if (model != nullptr) {
        lookup_column_display = model->property(id_lookup_column_display);
        lookup_column_id = model->property(id_lookup_column_id);

        if (model == this->model())
            Z_CHECK_X(filter == nullptr, "Нельзя использовать фильтр для собственной модели");

    } else {
        Z_CHECK(filter == nullptr);
    }
}

void View::onModelStartLoad()
{
}

void View::onModelStartSave()
{
}

void View::onModelStartRemove()
{
}

void View::onModelFinishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error)
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
}

void View::onModelFinishSave(
    const Error& error, const zf::DataPropertySet& requested_properties, const zf::DataPropertySet& saved_properties, const Uid& persistent_uid)
{
    Q_UNUSED(error)
    Q_UNUSED(requested_properties)
    Q_UNUSED(saved_properties)
    Q_UNUSED(persistent_uid)
}

void View::onModelFinishRemove(const Error& error)
{
    Q_UNUSED(error)
}

void View::onRefiltered(const DataProperty& dataset_property)
{
    Q_UNUSED(dataset_property);
}

void View::onProgressBegin(const QString& text, int start_persent)
{
    // qDebug() << "+++++++++ 3";
    blockUi();
    EntityObject::onProgressBegin(text, start_persent);
}

void View::onProgressEnd()
{
    EntityObject::onProgressEnd();
    // qDebug() << "--------- 2";
    unBlockUi();
}

void View::beforeModelChange(const ModelPtr& old_model, const ModelPtr& new_model)
{
    Q_UNUSED(old_model);
    Q_UNUSED(new_model);
}

void View::afterModelChange(const ModelPtr& model)
{
    Q_UNUSED(model);
}

void View::saveReloadState()
{
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto info = propertyInfo(p);
        QModelIndex idx = currentIndex(p);
        if (idx.isValid()) {
            info->reload_id = datasetRowID(p, idx.row(), idx.parent());
            info->reload_id.setPositionInfo(idx);
            info->reload_column = idx.column();
        } else {
            info->reload_id.clear();
            info->reload_column = -1;
        }
    }
}

void View::restoreReloadState()
{
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto info = propertyInfo(p);
        if (info->reload_id.isValid()) {
            // сначала просто пытаемся найти такой же идентификатор строки
            QModelIndex index = findDatasetRowID(p, info->reload_id);
            if (index.isValid()) {
                // нашли предудущее значение, значит таблица содержит нормальные идентификаторы строк
                index = cellIndex(p, index.row(), info->reload_column, index.parent());

            } else if (info->reload_id.hasPositionInfo()) {
                // если не удалось, то вероятно набор данных не имеет колонки с PropertyOption::Id и
                // нам надо позиционироваться по предыдущему положению курсора в таблице
                index = info->reload_id.restoreModelIndex(data()->dataset(p), info->reload_column);
            }

            if (index.isValid())
                setCurrentIndex(p, index);
        }
    }
}

void View::onStatesChanged(const ViewStates& new_states, const ViewStates& old_states)
{
    Q_UNUSED(new_states)
    Q_UNUSED(old_states)
}

bool View::onModalWindowOkSave()
{
    return true;
}

bool View::onModalWindowCancel()
{
    return true;
}

void View::afterModalWindowSave()
{
}

bool View::onModalWindowCustomSave(Error& error)
{
    Q_UNUSED(error)
    return false;
}

bool View::isModalWindowOkSaveEnabled() const
{
    return _eit_window_ok_save_enabled;
}

void View::setModalWindowOkSaveEnabled(bool b)
{
    if (_eit_window_ok_save_enabled == b)
        return;

    _eit_window_ok_save_enabled = b;
    emit sg_editWindowOkSaveEnabledChanged();
}

QWidget* View::createDatasetEditor(QWidget* parent_widget, int row, const DataProperty& column_property, int column_pos, const QModelIndex& parent_index)
{
    Q_UNUSED(parent_widget)
    Q_UNUSED(row)
    Q_UNUSED(column_property)
    Q_UNUSED(column_pos)
    Q_UNUSED(parent_index)
    return nullptr;
}

bool View::prepareDatasetEditor(QWidget* editor, int row, const DataProperty& column_property, int column_pos, const QModelIndex& parent_index)
{
    Q_UNUSED(editor)
    Q_UNUSED(row)
    Q_UNUSED(column_property)
    Q_UNUSED(column_pos)
    Q_UNUSED(parent_index)
    return false;
}

bool View::setDatasetEditorData(QWidget* editor, int row, const DataProperty& column_property, int column_pos, const QModelIndex& parent_index)
{
    Q_UNUSED(editor)
    Q_UNUSED(row)
    Q_UNUSED(column_property)
    Q_UNUSED(column_pos)
    Q_UNUSED(parent_index)
    return false;
}

bool View::setDatasetEditorModelData(QWidget* editor, int row, const DataProperty& column_property, int column_pos, const QModelIndex& parent_index)
{
    Q_UNUSED(editor)
    Q_UNUSED(row)
    Q_UNUSED(column_property)
    Q_UNUSED(column_pos)
    Q_UNUSED(parent_index)
    return false;
}

void View::setHighlightWidgetMapping(const DataProperty& property, QWidget* w)
{
    const_cast<HighlightModel*>(model()->highlight(false))->beginUpdate();
    highligtMapping()->setHighlightWidgetMapping(property, w);    
    const_cast<HighlightModel*>(model()->highlight(false))->endUpdate();
}

void View::setHighlightWidgetMapping(const PropertyID& property_id, QWidget* w)
{
    setHighlightWidgetMapping(property(property_id), w);
}

void View::setHighlightPropertyMapping(const DataProperty& source_property, const DataProperty& dest_property)
{
    const_cast<HighlightModel*>(model()->highlight(false))->beginUpdate();
    highligtMapping()->setHighlightPropertyMapping(source_property, dest_property);
    const_cast<HighlightModel*>(model()->highlight(false))->endUpdate();
}

void View::setHighlightPropertyMapping(const PropertyID& source_property_id, const PropertyID& dest_property_id)
{
    setHighlightPropertyMapping(property(source_property_id), property(dest_property_id));
}

QWidget* View::getHighlightWidget(const DataProperty& property) const
{
    return highligtMapping()->getHighlightWidget(property);
}

DataProperty View::getHighlightProperty(QWidget* w) const
{
    return highligtMapping()->getHighlightProperty(w);
}

void View::beforeFocusHighlight(const DataProperty& property)
{
    Q_UNUSED(property)
}

void View::afterFocusHighlight(const DataProperty& property)
{
    Q_UNUSED(property)
}

void View::onHighlightChanged(const HighlightModel* hm)
{
    Q_UNUSED(hm)
}

void View::setHighlightWidgetFrame(QWidget* w, InformationType type)
{
    Utils::setHighlightWidgetFrame(w, type);
}

void View::removeHighlightWidgetFrame(QWidget* w)
{
    Utils::removeHighlightWidgetFrame(w);
}

void View::embedViewAttached(View* view)
{
    Q_UNUSED(view)
}

void View::embedViewDetached(View* view)
{
    Q_UNUSED(view)
}

void View::onEmbedViewError(View* view, ObjectActionType type, const Error& error)
{
    Q_UNUSED(type);
    Q_UNUSED(view);
    if (_block_show_errors == 0)
        Core::postError(error);
}

void View::getHighlight(const DataProperty& p, HighlightInfo& info) const
{
    EntityObject::getHighlight(p, info);

    info.blockCheckId();

    // проверяем ItemSelector на наличие значений, которых нет в лукап при наличии фильтра
    if (_item_selectors == nullptr) {
        _item_selectors = std::make_unique<QHash<DataProperty, ItemSelector*>>();
        auto fields = structure()->propertiesByType(PropertyType::Field);
        for (auto& p : qAsConst(fields)) {
            if (p.lookup() != nullptr && (p.lookup()->type() == LookupType::Dataset || p.lookup()->type() == LookupType::List)
                && !p.options().testFlag(PropertyOption::SimpleDataset)) {
                auto is = object<ItemSelector>(p);
                _item_selectors->insert(p, is);
                connect(is, &ItemSelector::sg_lookupModelChanged, this, &View::sl_lookupModelChanged);
            }
        }
    }

    QHash<DataProperty, ItemSelector*> to_check;
    if (p.isEntity()) {
        to_check = *_item_selectors;

    } else if (p.isField() && _item_selectors->contains(p)) {
        to_check[p] = _item_selectors->value(p);
    }

    for (auto i = to_check.constBegin(); i != to_check.constEnd(); ++i) {
        Z_CHECK_NULL(i.key().lookup());
        if (i.key().lookup()->type() == LookupType::Dataset) {
            // после окончания загрузки оно приедет сюда еще раз
            if (i.value()->model() == nullptr || i.value()->model()->isLoading()) {
                info.empty(i.key(), Framework::HIGHLIGHT_ID_BAD_LOOKUP_VALUE);
                continue;
            }

        } else if (i.key().lookup()->type() == LookupType::List) {
            // ничего не делаем

        } else {
            Z_HALT_INT;
        }

        ItemSelector* is = i.value();

        if (Utils::hasParentWindow(is) && is->isVisible() && is->hasCurrentValue() && !is->isCurrentValueFound() && !is->isReadOnly()) {
            QString display_text = is->displayText().simplified();
            if (display_text.isEmpty()) {
                if (is->currentValue().isValid()) { // это обходное решение. вообще такого быть не должно и значит что-то не так с ItemSelector
                    info.insert(i.key(), Framework::HIGHLIGHT_ID_BAD_LOOKUP_VALUE, ZF_TR(ZFT_BAD_LOOKUP_VALUE).arg(Utils::variantToString(is->currentValue())),
                        InformationType::Warning, -1, QVariant(), false);
                } else {
                    info.empty(i.key(), Framework::HIGHLIGHT_ID_BAD_LOOKUP_VALUE);
                }
            } else {
                info.insert(i.key(), Framework::HIGHLIGHT_ID_BAD_LOOKUP_VALUE, ZF_TR(ZFT_BAD_LOOKUP_VALUE).arg(is->displayText()), InformationType::Warning, -1,
                    QVariant(), false);
            }
        } else {
            info.empty(i.key(), Framework::HIGHLIGHT_ID_BAD_LOOKUP_VALUE);
        }
    }

    info.unBlockCheckId();
}

void View::onCheckedRowsChanged(const DataProperty& p)
{
    Q_UNUSED(p)
}

void View::onCheckPanelVisibleChanged(const DataProperty& p)
{
    Q_UNUSED(p)
}

bool View::isDragBegin(QWidget* drag_widget, const zf::DataProperty& property, const QPoint& pos, QMimeData* drag_data, QPixmap& drag_pixmap)
{
    Q_UNUSED(drag_widget)
    Q_UNUSED(property)
    Q_UNUSED(pos)
    Q_UNUSED(drag_data)
    Q_UNUSED(drag_pixmap)
    return false;
}

bool View::onDragEnter(View* source_view, QWidget* source_widget, const zf::DataProperty& source_property, QWidget* target,
    const zf::DataProperty& target_property, const QPoint& pos, const QMimeData* drag_data)
{
    Q_UNUSED(source_view)
    Q_UNUSED(source_property)
    Q_UNUSED(source_widget)
    Q_UNUSED(target)
    Q_UNUSED(target_property)
    Q_UNUSED(pos)
    Q_UNUSED(drag_data)
    return false;
}

bool View::onDragMove(View* source_view, QWidget* source_widget, const zf::DataProperty& source_property, QWidget* target,
    const zf::DataProperty& target_property, const QPoint& pos, const QMimeData* drag_data)
{
    Q_UNUSED(source_view)
    Q_UNUSED(source_property)
    Q_UNUSED(source_widget)
    Q_UNUSED(target)
    Q_UNUSED(target_property)
    Q_UNUSED(pos)
    Q_UNUSED(drag_data)
    return false;
}

bool View::onDrop(View* source_view, QWidget* source_widget, const zf::DataProperty& source_property, QWidget* target, const zf::DataProperty& target_property,
    const QPoint& pos, const QMimeData* drag_data)
{
    Q_UNUSED(source_view)
    Q_UNUSED(source_property)
    Q_UNUSED(source_widget)
    Q_UNUSED(target)
    Q_UNUSED(target_property)
    Q_UNUSED(pos)
    Q_UNUSED(drag_data)
    return false;
}

void View::onDragLeave(QWidget* drag_widget, const zf::DataProperty& property)
{
    Q_UNUSED(drag_widget)
    Q_UNUSED(property)
}

View::ModalWindowButtonAction View::onModalWindowButtonClick(QDialogButtonBox::StandardButton button, const Operation& op, bool cross_button)
{
    Q_UNUSED(button);
    Q_UNUSED(op);
    Q_UNUSED(cross_button);

    return ModalWindowButtonAction::Continue;
}

bool View::getModalWindowShowHightlightFrame(
    const HighlightModel* h, QMap<QDialogButtonBox::StandardButton, InformationType>& buttons, QMap<OperationID, InformationType>& operations)
{
    Q_UNUSED(h);
    Q_UNUSED(buttons);
    Q_UNUSED(operations);

    return true;
}

void View::setModalWindowInterface(I_ModalWindow* i_modal_window)
{
    _i_modal_window = i_modal_window;
    if (_i_modal_window != nullptr)
        onModalWindowActivated();
}

void View::setModuleWindowInterface(I_ModuleWindow* i_module_window)
{
    _i_module_window = i_module_window;
    if (_i_module_window != nullptr)
        onModuleWindowActivated();
}

void View::onModalWindowActivated()
{
}

void View::onModuleWindowActivated()
{
}

bool View::hasUnsavedData(Error& error) const
{
    return model()->hasUnsavedData(error, DataPropertyList {}, DataContainer::ChangedBinaryMethod::Ignore);
}

bool View::hasUnsavedData() const
{
    Error error;
    bool res = hasUnsavedData(error);
    Z_CHECK_ERROR(error);
    return res;
}

void View::setModalWindowOperations(const OperationIDList& operations)
{
    _modal_window_operations.clear();
    qDeleteAll(_modal_window_buttons);
    _modal_window_buttons.clear();

    for (auto& id : operations) {
        auto op = operation(id);
        Z_CHECK(op.scope() == OperationScope::Entity);
        _modal_window_operations << op;
    }
}

I_ModalWindow* View::modalWindowInterface() const
{
    Z_CHECK_NULL(_i_modal_window);
    return _i_modal_window;
}

bool View::isModalWindowInterfaceInstalled() const
{
    return _i_modal_window != nullptr;
}

I_ModuleWindow* View::moduleWindowInterface() const
{
    Z_CHECK_NULL(_i_module_window);
    return _i_module_window;
}

bool View::isModuleWindowInterfaceInstalled() const
{
    return _i_module_window != nullptr;
}

Error View::setUI(const QString& resource_name)
{
    // устанавливаем форму в главный виджет представления
    Error error;
    QWidget* form = Utils::openUI(resource_name, error, moduleUI());
    if (error.isError())
        return error;

    moduleUI()->setLayout(new QVBoxLayout);
    moduleUI()->layout()->setObjectName("zflui");
    moduleUI()->layout()->setMargin(0);
    moduleUI()->layout()->addWidget(form);

    return Error();
}

//! Найти view model на которой находится виджет
static View* dragSourceView(QObject* obj)
{
    if (obj == nullptr)
        return nullptr;

    auto top = Utils::findTopParentWidget(obj, false, {}, View::MAIN_WIDGET_OBJECT_NAME);
    auto value = top->property(View::MAIN_WIDGET_VIEW_PRT_NAME);
    if (!value.isValid())
        return nullptr;

    auto ptr = value.value<qintptr>();
    Z_CHECK(ptr != 0);

    return reinterpret_cast<View*>(ptr);
}

//! Найти виджет - источник
static QWidget* dragSourceWidget(QObject* obj)
{
    if (obj == nullptr || !obj->isWidgetType())
        return nullptr;

    return Utils::getMainWidget(qobject_cast<QWidget*>(obj));
}

bool View::onDragEnterHelper(QWidget* widget, QDragEnterEvent* event)
{
    auto view = dragSourceView(event->source());
    auto source_widget = dragSourceWidget(event->source());
    auto target_widget = dragSourceWidget(widget);

    bool accept = onDragEnter(view, source_widget, source_widget && view ? view->widgetProperty(source_widget) : DataProperty(), target_widget,
        target_widget ? widgetProperty(target_widget) : DataProperty(), widget->mapToGlobal(event->pos()), event->mimeData());

    if (accept) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    return true;
}

bool View::onDragMoveHelper(QWidget* widget, QDragMoveEvent* event)
{
    auto view = dragSourceView(event->source());
    auto source_widget = dragSourceWidget(event->source());
    auto target_widget = dragSourceWidget(widget);

    bool accept = onDragMove(view, source_widget, source_widget && view ? view->widgetProperty(source_widget) : DataProperty(), target_widget,
        target_widget ? widgetProperty(target_widget) : DataProperty(), widget->mapToGlobal(event->pos()), event->mimeData());

    if (accept) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    return true;
}

bool View::onDropHelper(QWidget* widget, QDropEvent* event)
{
    auto view = dragSourceView(event->source());
    auto source_widget = dragSourceWidget(event->source());
    auto target_widget = dragSourceWidget(widget);

    bool accept = onDrop(view, source_widget, source_widget && view ? view->widgetProperty(source_widget) : DataProperty(), target_widget,
        target_widget ? widgetProperty(target_widget) : DataProperty(), widget->mapToGlobal(event->pos()), event->mimeData());

    if (accept) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    return true;
}

bool View::onDragLeaveHelper(QWidget* widget, QDragLeaveEvent* event)
{
    Q_UNUSED(event)

    auto target_widget = dragSourceWidget(widget);
    onDragLeave(target_widget, target_widget ? widgetProperty(target_widget) : DataProperty());

    return true;
}

QString View::conditionFilterUniqueKey() const
{
    return customConfigurationCodeInternal();
}

void View::searchText(const DataProperty& dataset, const QString& text)
{
    Z_CHECK(dataset.isValid());

    QAbstractItemModel* m = proxyDataset(dataset);
    if (m->rowCount() == 0)
        return;

    QModelIndex index = proxyIndex(currentIndex(dataset));

    if (!index.isValid()) {
        index = m->index(0, firstVisibleColumn(dataset));
        setCurrentIndex(dataset, index);
    } else
        index = m->index(0, index.column());

    index = Utils::searchItemModel(index, text, true, this);
    if (index.isValid())
        setCurrentIndex(dataset, index);
}

void View::searchTextNext(const DataProperty& dataset, const QString& text)
{
    QModelIndex index = proxyIndex(currentIndex(dataset));
    if (!index.isValid()) {
        searchText(dataset, text);
        return;
    }

    // Ищем следующий по порядку
    QModelIndex next = Utils::getNextItemModelIndex(index, true);
    if (next.isValid())
        next = next.model()->index(next.row(), index.column(), next.parent());

    // Ищем в текст
    next = Utils::searchItemModel(next, text, true, this);
    if (next.isValid())
        setCurrentIndex(dataset, next);
}

void View::searchTextPrevious(const DataProperty& dataset, const QString& text)
{
    QModelIndex index = proxyIndex(currentIndex(dataset));
    if (!index.isValid()) {
        searchText(dataset, text);
        return;
    }

    // Ищем предыдущий
    QModelIndex next = Utils::getNextItemModelIndex(index, false);
    if (next.isValid())
        next = next.model()->index(next.row(), index.column(), next.parent());

    // Ищем текст
    next = Utils::searchItemModel(next, text, false, this);
    if (next.isValid())
        setCurrentIndex(dataset, next);
}

bool View::isAutoSearchActive() const
{
    return _search_timer->isActive();
}

QString View::autoSearchText() const
{
    return _last_search;
}

void View::setDatasetSortEnabled(const DataProperty& dataset_property, bool enabled)
{
    widgets()->setDatasetSortEnabled(dataset_property, enabled);
}

void View::setDatasetSortEnabled(const PropertyID& dataset_property_id, bool enabled)
{
    widgets()->setDatasetSortEnabled(dataset_property_id, enabled);
}

void View::setDatasetSortEnabledAll(bool enabled)
{
    widgets()->setDatasetSortEnabledAll(enabled);
}

void View::sortDataset(const DataProperty& column_property, Qt::SortOrder order)
{
    widgets()->sortDataset(column_property, order);
}

void View::sortDataset(const PropertyID& column_property_id, Qt::SortOrder order)
{
    widgets()->sortDataset(column_property_id, order);
}

void View::addToolbar(QToolBar* toolbar)
{
    Z_CHECK_NULL(toolbar);
    prepareToolbar(toolbar);

    for (auto& t : _additional_toolbars) {
        if (t == toolbar)
            return;
    }

    _additional_toolbars << toolbar;
    connect(toolbar, &QToolBar::destroyed, this, &View::sl_additionalToolbarDestroyed);
    emit sg_toolbarAdded(toolbar);
}

void View::removeToolbar(QToolBar* toolbar)
{
    Z_CHECK_NULL(toolbar);

    for (int i = 0; i < _additional_toolbars.count(); i++) {
        if (_additional_toolbars.at(i) == toolbar) {
            _additional_toolbars.removeAt(i);
            disconnect(toolbar, &QToolBar::destroyed, this, &View::sl_additionalToolbarDestroyed);
            emit sg_toolbarRemoved();
            return;
        }
    }

    Z_HALT_INT; // попытка удалить не добавленный ранее тулбар
}

//***** структура хранения конфигурации
// размер MDI окна
#define CONFIG_WINDOW_SIZE_KEY QStringLiteral("WSIZE")
// состояние MDI окна
#define CONFIG_WINDOW_STATE_KEY QStringLiteral("WSTATE")
// префикс для хранения свойств
#define CONFIG_PROPERTY_KEY_PREFIX QStringLiteral("PROP")
// префикс для хранения состояния сплиттеров
#define CONFIG_SPLIT_KEY_PREFIX QStringLiteral("SPLIT_")

Error View::saveConfiguration(const WindowConfiguration& window_config) const
{
    onSaveConfiguration();

    for (auto v : embedViews()) {
        v->saveConfiguration();
    }

    QVariantMap data; // QMap<QString, QVariant>
    auto conf = activeConfiguration(CONFIG_VERSION);

    // считываем старые данные
    data = conf->value(VIEW_CONFIG_KEY, CONFIG_KEY_VERSION).value<QVariantMap>();
    if (conf->status() != QSettings::NoError)
        data.clear();

    // записываем новые данные
    if ((isMdiWindow() || isEditWindow() || isViewWindow()) && window_config.isValid()) {
        // размеры окон
        data[CONFIG_WINDOW_SIZE_KEY] = QVariant::fromValue(window_config.size);
        // состояния окон
        data[CONFIG_WINDOW_STATE_KEY] = QVariant::fromValue(QString::number(static_cast<int>(window_config.state)));
    }

    // спилиттеры
    QObjectList splitters;
    _objects_finder->findObjectsByClass(splitters, QStringLiteral("QSplitter"));
    QStringList splitter_names;
    int s_num = 1;
    for (QObject* obj : qAsConst(splitters)) {
        QSplitter* s = qobject_cast<QSplitter*>(obj);
        Z_CHECK_NULL(s);
        if (s->objectName().isEmpty() || !isSaveSplitterConfiguration(s))
            continue;

        // исключаем проблему с дублированием имен сплиттеров
        QString s_name;
        if (splitter_names.contains(s->objectName())) {
            s_name = s->objectName() + "_" + QString::number(s_num);
            s_num++;
        } else {
            s_name = s->objectName();
        }
        splitter_names << s_name;

        data[CONFIG_SPLIT_KEY_PREFIX + s_name] = QVariant::fromValue(Utils::toVariantList(s->sizes()));
    }

    // свойства
    for (auto& p : structure()->properties()) {
        if (p.propertyType() == PropertyType::Dataset) {
            if (p.options().testFlag(PropertyOption::SimpleDataset) ||
                // при завершении работы виджет может быть уже удален
                widgets()->field(p, false) == nullptr)
                continue;

            // нет смысла сохранять параметры заголовка если в нем только одна видимая секция и она автоматически растянута
            if (horizontalRootHeaderItem(p)->bottomPermanentVisibleCount() == 1 && horizontalHeaderView(p)->stretchLastSection()) {
                data[configPropertyKeyEncode(p)] = QVariant();
                continue;
            }

            Error error;
            QByteArray ba;
            if (TableView* table = qobject_cast<TableView*>(object<QAbstractItemView>(p)))
                error = table->serialize(ba);
            else if (TreeView* tree = qobject_cast<TreeView*>(object<QAbstractItemView>(p)))
                error = tree->serialize(ba);
            else
                Z_HALT_INT;

            if (error.isError())
                return error;

            data[configPropertyKeyEncode(p)] = QVariant::fromValue(ba);
        }
    }

    conf->setValue(VIEW_CONFIG_KEY, QVariant::fromValue(data), CONFIG_KEY_VERSION);
    if (conf->status() != QSettings::NoError)
        return Error::fileIOError(conf->fileNames().join("; "));

    _config_time = QDateTime::currentDateTime();
    globalConfigTime()->insert(entityCode(), _config_time);

    return Error();
}

Error View::saveConfiguration() const
{
    return saveConfiguration(WindowConfiguration());
}

Error View::loadConfiguration(WindowConfiguration& window_config) const
{
    auto conf = activeConfiguration(CONFIG_VERSION);

    QDateTime global_time = globalConfigTime()->value(entityCode());
    if (!_config_time.isNull() && conf->configCode() == _config_code) {
        if (global_time.isNull() || _config_time >= global_time)
            return Error(); // конфигурация не менялась
    }

    // без этого не будет правильно сформирован код активной конфигурации
    const_cast<View*>(this)->updateParentView();

    _config_time = QDateTime::currentDateTime();
    _config_code = conf->configCode();

    const_cast<View*>(this)->onLoadConfiguration();

    for (auto v : embedViews()) {
        v->loadConfiguration();
    }

    QVariantMap data = conf->value(VIEW_CONFIG_KEY, CONFIG_KEY_VERSION).value<QVariantMap>();
    if (conf->status() != QSettings::NoError) {
        const_cast<View*>(this)->afterLoadConfigurationHelper();
        return Error::fileIOError(conf->fileNames().join("; "));
    }

    // все спилиттеры
    QObjectList splitters;
    _objects_finder->findObjectsByClass(splitters, QStringLiteral("QSplitter"));
    QHash<QString, QSplitter*> splitters_hash;
    QStringList splitter_names;
    int s_num = 1;
    for (QObject* obj : qAsConst(splitters)) {
        QSplitter* s = qobject_cast<QSplitter*>(obj);
        Z_CHECK_NULL(s);
        if (s->objectName().isEmpty() || !isSaveSplitterConfiguration(s))
            continue;

        // исключаем проблему с дублированием имен сплиттеров
        QString s_name;
        if (splitter_names.contains(s->objectName())) {
            s_name = s->objectName() + "_" + QString::number(s_num);
            s_num++;
        } else {
            s_name = s->objectName();
        }
        splitter_names << s_name;

        splitters_hash[s_name] = s;
    }

    for (auto i = data.constBegin(); i != data.constEnd(); ++i) {
        const QString& key = i.key();
        bool ok;

        if (key == CONFIG_WINDOW_SIZE_KEY) {
            // размеры окон
            if (isMdiWindow() || isEditWindow() || isViewWindow())
                window_config.size = i.value().toSize();

        } else if (key == CONFIG_WINDOW_STATE_KEY) {
            // состояние окон
            if (isMdiWindow() || isEditWindow() || isViewWindow()) {
                QString status = i.value().toString();
                int status_int = status.toInt(&ok);
                if (ok)
                    window_config.state = static_cast<Qt::WindowState>(status_int);
            }
        } else if (key.startsWith(CONFIG_SPLIT_KEY_PREFIX)) {
            // сплиттеры
            QString object_name = key.right(key.length() - CONFIG_SPLIT_KEY_PREFIX.length());
            QSplitter* splitter = splitters_hash.value(object_name);
            if (splitter != nullptr) {
                auto sizes = i.value().value<QVariantList>();
                if (sizes.count() == splitter->sizes().count())
                    splitter->setSizes(Utils::toIntList(sizes));
            }

        } else {
            DataProperty prop = configPropertyKeyDecode(key);
            if (!prop.isValid() || prop.propertyType() != PropertyType::Dataset)
                continue;

            TableView* table = qobject_cast<TableView*>(object<QWidget>(prop));
            TreeView* tree = qobject_cast<TreeView*>(object<QWidget>(prop));
            if (table == nullptr && tree == nullptr)
                continue;

            QByteArray ba = i.value().toByteArray();
            if (table != nullptr)
                table->deserialize(ba);
            else
                tree->deserialize(ba);
        }
    }

    const_cast<View*>(this)->afterLoadConfigurationHelper();
    return Error();
}

Error View::loadConfiguration() const
{
    WindowConfiguration conf;
    return loadConfiguration(conf);
}

QString View::configPropertyKeyEncode(const DataProperty& prop) const
{
    Z_CHECK(prop.isValid());
    return CONFIG_PROPERTY_KEY_PREFIX + prop.id().string();
}

DataProperty View::configPropertyKeyDecode(const QString& key) const
{
    DataProperty prop;
    if (key.startsWith(CONFIG_PROPERTY_KEY_PREFIX)) {
        QString prop_key = key.right(key.length() - CONFIG_PROPERTY_KEY_PREFIX.length());

        bool ok;
        int property_id = prop_key.toInt(&ok);
        if (ok && property_id >= Consts::MINUMUM_PROPERTY_ID && structure()->contains(PropertyID(property_id)))
            prop = property(PropertyID(property_id));
    }

    return prop;
}

void View::afterCreatedHelper()
{
    connectToProperties();

    if (!options().testFlag(ViewOption::SortDefaultDisable))
        setDatasetSortEnabledAll();

    // создаем UI вручную
    createUI();

    // если UI не был создан вручную, то создаем его автоматически
    if (moduleUI()->layout() == nullptr)
        createUiByScheme();

    if (moduleUI()->baseSize().isEmpty())
        moduleUI()->setBaseSize(640, 480);

    replaceWidgets();
    translate(moduleUI());

    Utils::prepareWidgetScale(moduleUI());

    // инициализируем все виджеты
    auto ls = _objects_finder->childWidgets();
    DataPropertySet initialized;
    for (const QWidgetList& ws : qAsConst(ls)) {
        for (QWidget* w : ws) {
            DataProperty p = widgetProperty(w);
            if (p.isValid()) {
                if (initialized.contains(p))
                    continue;

                initialized << p;
            }

            prepareWidget(w, p);
        }
    }

    if (mainToolbar() != nullptr) {
        Utils::prepareToolbar(mainToolbar(), ToolbarType::Window);
        mainToolbar()->installEventFilter(this);
        for (auto& operation : operationMenuManager()->operations()) {
            QToolButton* button = operationMenuManager()->button(operation);
            if (button != nullptr)
                button->installEventFilter(this);
        }
    }

    if (isLoading() || !data()->notInitializedProperties().isEmpty() || data()->invalidatedProperties().isEmpty()) {
        if (_visible_status) {
            internalCallbackManager()->addRequest(this, Framework::VIEW_INVALIDATED_KEY);
            onStartLoading();

        } else {
            _data_update_required = true;
        }

    } else {
        // отклыдываем вызов onDataReady, чтобы не было фриза из-за операций, которые там могут быть
        internalCallbackManager()->addRequest(this, Framework::VIEW_DATA_READY);
    }

    if (widgets()->isLookupLoading()) {
        // qDebug() << "+++++++++ 2";
        blockUi();
    }

    connect(widgets(), &DataWidgets::sg_beginLookupLoad, this, [&]() {
        // qDebug() << "+++++++++ 1";
        blockUi();
    });
    connect(widgets(), &DataWidgets::sg_endLookupLoad, this, [&]() {
        if (_is_destructing)
            return;
        // qDebug() << "--------- 1";
        unBlockUi();
    });

    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto root = horizontalRootHeaderItem(p);
        if (root->count() == 0) {
            // если не добавлены заголовки для таблиц наборов данных, то добавляем все
            initDatasetHorizontalHeader(p);
        }
    }

    if (_ui_auto_created && !_ui_auto_created_has_borders) {
        Z_CHECK_NULL(moduleUI()->layout());
        QVBoxLayout* la = qobject_cast<QVBoxLayout*>(moduleUI()->layout());
        Z_CHECK_NULL(la);

        if (la->count() > 0) {
            // добавляем линию под тулбаром
            if (hasToolbars(true)) {
                la->insertWidget(0, Utils::createLine(Qt::Horizontal));
                la->setSpacing(0);
            }

            // добавляем линию перед кнопками
            if (isEditWindow()) {
                la->addWidget(Utils::createLine(Qt::Horizontal));
                la->setSpacing(0);
            }
        }
    }

    // подготавливаем тулбары
    QObjectList tool_buttons;
    _objects_finder->findObjectsByClass(tool_buttons, QStringLiteral("QToolButton"), false, false);
    for (auto& b : qAsConst(tool_buttons)) {
        Utils::prepareToolbarButton(qobject_cast<QToolButton*>(b));
    }

    connect(widgets(), &DataWidgets::sg_fieldVisibleChanged, this, &View::sg_fieldVisibleChanged);

    afterCreated();

    _is_full_created = true;
    _main_widget->setUpdatesEnabled(true);

    while (_block_ui_counter_before_create > 0) {
        _block_ui_counter_before_create--;
        blockUi();
    }
}

void View::onDataReadyHelper()
{
    if (_is_data_ready_called)
        return;

    _is_data_ready_called = true;

    if (objectExtensionDestroyed())
        return;

    // заполняем лукап поля
    fillLookupValues();
    // грузим ошибки
    loadAllHighlights();

    // запрашиваем фильтры по условиям
    internalCallbackManager()->addRequest(this, Framework::VIEW_LOAD_CONDITION_FILTER_KEY);

    // запрашиваем обновление экшенов
    requestUpdateView();
    // запрашиваем обновление порядка табуляции
    requestInitTabOrder();
    // инициалищзируем виджет поиска
    updateSearchWidget();
    // запрашиваем актуализацию данных для видимых виджетов
    requestProcessVisibleChange();
    // запрашиваем проерку на ошибки
    registerHighlightCheck();
    // при открытии представления надо один раз вызвать onHighlightChanged
    internalCallbackManager()->addRequest(this, Framework::VIEW_HIGHLIGHT_CHANGED_KEY);

    onDataReady(_custom_data);

    // запрашиваем обновление данных в виджетах
    if (widgets()->requestUpdateWidgets(DataWidgets::modelStatusToUpdateReason(model())))
        connect(widgets(), &DataWidgets::sg_widgetsUpdated, this, &View::sl_widgetsUpdated);
    else
        sl_widgetsUpdated();
}

void View::createUiByScheme()
{
    if (moduleUI()->layout() != nullptr)
        delete moduleUI()->layout();

    WidgetScheme scheme(widgets());
    configureWidgetScheme(&scheme);

    // помещаем все это хозяйство в вертикальный layout
    QVBoxLayout* la = new QVBoxLayout();
    la->setObjectName(QStringLiteral("zf_ui_la"));
    la->setMargin(0);

    la->addWidget(scheme.generate(QString::number(entityCode().value()), _ui_auto_created_has_borders, true, !isEditWindow()));

    moduleUI()->setLayout(la);

    _ui_auto_created = true;
}

void View::prepareToolbar(QToolBar* toolbar)
{
    Z_CHECK_NULL(toolbar);
    Utils::prepareToolbar(toolbar, ToolbarType::Window);
}

void View::setTabOrder(const QList<QWidget*>& tab_order)
{
    _tab_order = tab_order;
    Utils::setTabOrder(_tab_order);
    emit sg_highlightViewSortOrderChanged();
}

QList<QObject*> View::findElementHelper(const QStringList& path, const char* class_name) const
{
    Z_CHECK(!path.isEmpty());

    if (path.constLast().startsWith(WidgetReplacerDataStructure::PREFIX_DATA_PROPERTY, Qt::CaseInsensitive)) {
        // видимо это свойство
        QString base_name = path.constLast().right(path.constLast().length() - WidgetReplacerDataStructure::PREFIX_DATA_PROPERTY.length());
        bool ok;
        int id = base_name.toInt(&ok);
        if (ok && id >= Consts::MINUMUM_PROPERTY_ID) {
            QWidget* w = object<QWidget>(PropertyID(id), false);
            if (w != nullptr && ObjectsFinder::compareObjectName(path, w, true))
                return QList<QObject*>({ w });
        }
    } else if (path.constLast().startsWith(WidgetReplacerDataStructure::PREFIX_ENTITY_SEARCH_WIDGET, Qt::CaseInsensitive)) {
        // видимо это виджет поиска
        if (searchWidget() != nullptr && ObjectsFinder::compareObjectName(path, searchWidget(), true))
            return QList<QObject*>({ searchWidget() });
    }

    if (mainToolbar() != nullptr && !ChildObjectsContainer::inContainer(mainToolbar()) && !_objects_finder->hasSource(mainToolbar()))
        _objects_finder->addSource(mainToolbar());

    for (auto tb : additionalToolbars()) {
        if (!ChildObjectsContainer::inContainer(tb) && !_objects_finder->hasSource(tb))
            _objects_finder->addSource(tb);
    }

    return _objects_finder->findObjects(path, class_name);
}

void View::updateWidgetHighlight(const HighlightItem& item)
{
    if (item.property().propertyType() == PropertyType::Cell) {
        QAbstractItemView* w = object<QAbstractItemView>(item.property().dataset(), false);
        if (w != nullptr && w->isVisible()) {
            QModelIndex idx = cellIndex(item.property());
            if (idx.isValid()) {
                w->update(proxyIndex(idx));
            }
        }

    } else if (item.property().isDatasetPart() || item.property().propertyType() == PropertyType::Dataset) {
        if (!item.property().dataset().options().testFlag(PropertyOption::SimpleDataset)) {
            QAbstractItemView* w = object<QAbstractItemView>(item.property().dataset(), false);
            if (w != nullptr && w->isVisible()) {
                w->update();
            }
        }
    }

    if (item.property().propertyType() == PropertyType::Field || item.property().propertyType() == PropertyType::Dataset) {
        // обновляем тултип с ошибкой
        QStringList errors;
        QStringList warnings;
        auto items = highlight(false)->items(item.property());
        for (auto& i : items) {
            if (i.type() == InformationType::Error || i.type() == InformationType::Critical || i.type() == InformationType::Fatal)
                errors << i.text();
            else if (i.type() == InformationType::Warning)
                warnings << i.text();
        }

        QString text = QStringList(errors + warnings).join("<br>");
        QWidget* w = object<QWidget>(item.property());
        w->setToolTip(text.simplified());

        if (text.isEmpty() && qApp->widgetAt(QCursor::pos()) == w)
            QToolTip::hideText();
    }
}

void View::conditionFilterChanged(const DataProperty& dataset)
{
    horizontalHeaderView(dataset)->viewport()->update();
    emit sg_conditionFilterChanged(dataset);
}

void View::compressActionsHelper()
{
    if (mainToolbar() != nullptr)
        Utils::compressActionList(mainToolbar()->actions(), false);

    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        Z_CHECK_NULL(widgets()->datasetToolbar(p));
        bool has_visible = Utils::compressActionList(widgets()->datasetToolbar(p)->actions(), false).first;
        auto info = propertyInfo(p);
        if (info->toolbar_has_visible_actions == Answer::Undefined || (info->toolbar_has_visible_actions == Answer::Yes) != has_visible) {
            info->toolbar_has_visible_actions = has_visible? Answer::Yes : Answer::No;
            requestUpdatePropertyStatus(p);
        }
    }
}

OperationMenuManager* View::operationMenuManager() const
{
    if (_operation_menu_manager == nullptr) {
        _operation_menu_manager = new OperationMenuManager(
            QStringLiteral("toolbar_%1").arg(entityCode().value()), createOperationMenu(), OperationScope::Entity, ToolbarType::Window);
        connect(_operation_menu_manager.get(), &OperationMenuManager::sg_operationActivated, this, &View::sl_operationActionActivated);

        if (_is_full_created)
            updateAccessRightsHelper();
        else
            requestUpdateAccessRights();
    }
    return _operation_menu_manager.get();
}

void View::prepareWidget(QWidget* w, const DataProperty& p)
{
    Z_CHECK_NULL(w);

    Utils::prepareWidgetScale(w);

    if (auto b = qobject_cast<QToolButton*>(w)) {
        Utils::prepareToolbarButton(b);

    } else if (auto tb = qobject_cast<QTabBar*>(w)) {
        if (tb->styleSheet().isEmpty())
            Utils::prepareTabBar(tb);

    } else if (auto tw = qobject_cast<QTabWidget*>(w)) {
        tw->setAutoFillBackground(true);
        for (int i = 0; i < tw->count(); i++) {
            tw->widget(i)->setAutoFillBackground(true);
        }

        // Делаем активной первую доступную вкладку
        for (int i = 0; i < tw->count(); i++) {
            if (!tw->isTabEnabled(i))
                continue;

            tw->setCurrentIndex(i);
            break;
        }

        if (tw->styleSheet().isEmpty())
            Utils::prepareTabBar(tw);

    } else if (auto toolBox = qobject_cast<QToolBox*>(w)) {
        // Делаем активной первую доступную вкладку
        for (int i = 0; i < toolBox->count(); i++) {
            if (!toolBox->isItemEnabled(i))
                continue;

            toolBox->setCurrentIndex(i);
            break;
        }

    } else if (auto gb = qobject_cast<CollapsibleGroupBox*>(w)) {
        // По умолчанию закрываем
        gb->collapse();

    } else if (auto item_view = qobject_cast<QAbstractItemView*>(w)) {
        // отключаем автоматическое переключение в режим редактирования ячеек. Оно будет запущено программно
        item_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

        QTableView* tv = qobject_cast<QTableView*>(item_view);
        if (tv != nullptr) {
            // подгоняем высоту строк под текущий шрифт
            if (auto tv_zf = qobject_cast<zf::TableView*>(item_view)) {
                if (!tv_zf->isAutoResizeRowsHeight())
                    tv_zf->verticalHeader()->setDefaultSectionSize(UiSizes::defaultTableRowHeight());

            } else {
                tv->verticalHeader()->setDefaultSectionSize(UiSizes::defaultTableRowHeight());
            }

        } else {
            QTreeView* tv = qobject_cast<QTreeView*>(item_view);
            if (tv != nullptr) {
                tv->setUniformRowHeights(true);
            }
        }
    }

    if (p.propertyType() == PropertyType::Dataset && !p.options().testFlag(PropertyOption::SimpleDataset)) {
        // если всего одна видимая колонка, то делаем ее авторасширяемой
        auto pv_items = horizontalRootHeaderItem(p)->bottomPermanentVisible();
        if (pv_items.count() == 1) {
            pv_items.at(0)->setResizeMode(QHeaderView::Stretch);
        }

        bool has_frame = object<QAbstractItemView>(p)->frameShape() != QFrame::NoFrame;
        DataWidgets::setDatasetToolbarWidgetStyle(p.dataType() == DataType::Table ? horizontalHeaderView(p)->stretchLastSection() : false, has_frame, has_frame,
            has_frame, widgets()->datasetToolbarWidget(p));
    }

    // Запоминаем состояние readOnly, чтобы оно не было сброшено на основании PropertyOptions
    if (p.isValid() && isWidgetReadOnly(w) && propertyInfo(p)->read_only == Answer::Undefined)
        setPropertyReadOnly(p, true);

    connectToWidget(w, p);
    // Дополнительные действия после создания виджета
    onWidgetCreated(w, p);
    // Инициализируем стили всех виджетов
    initWidgetStyle(w, p);
}

void View::connectToWidget(QWidget* w, const DataProperty& property)
{
    w->installEventFilter(this);

    if (auto gb = qobject_cast<CollapsibleGroupBox*>(w)) {
        connect(gb, &CollapsibleGroupBox::sg_expandedChanged, this, &View::sl_expandedChanged);

    } else if (auto tb = qobject_cast<QTabBar*>(w)) {
        // Для реакции на смену табов, подписываемся на сигналы
        connect(tb, &QTabBar::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto tw = qobject_cast<QTabWidget*>(w)) {
        connect(tw, &QTabWidget::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto toolBox = qobject_cast<QToolBox*>(w)) {
        connect(toolBox, &QToolBox::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto s = qobject_cast<QSplitter*>(w)) {
        connect(s, &QSplitter::splitterMoved, this, &View::sl_splitterMoved);
    }

    if (property.isValid() && property.lookup() != nullptr && property.lookup()->type() == LookupType::Dataset) {
        auto is = object<ItemSelector>(property);
        if (is->model() != nullptr) {
            auto info = propertyInfo(property);
            if (!info->connection_lookup) {
                info->connection_lookup = connect(is->model().get(), &Model::sg_finishLoad, this, &View::sl_lookupModelLoaded);
                _lookup_model_map[is->model().get()] = property;
            }
        }
    }
}

void View::disconnectFromWidget(QWidget* w)
{
    w->removeEventFilter(this);

    if (auto gb = qobject_cast<CollapsibleGroupBox*>(w)) {
        disconnect(gb, &CollapsibleGroupBox::sg_expandedChanged, this, &View::sl_expandedChanged);

    } else if (auto tb = qobject_cast<QTabBar*>(w)) {
        // Для реакции на смену табов, подписываемся на сигналы
        disconnect(tb, &QTabBar::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto tw = qobject_cast<QTabWidget*>(w)) {
        disconnect(tw, &QTabWidget::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto toolBox = qobject_cast<QToolBox*>(w)) {
        disconnect(toolBox, &QToolBox::currentChanged, this, &View::sl_currentTabChanged);

    } else if (auto s = qobject_cast<QSplitter*>(w)) {
        disconnect(s, &QSplitter::splitterMoved, this, &View::sl_splitterMoved);
    }
}

void View::updateBlockUiGeometryHelper()
{
    if (!Core::isBootstraped() || !isBlockedUi())
        return;

    // если представление встроено в другое, то блокировку не показываем
    if (isParentViewUiBlocked())
        return;

    if (_block_ui_top_widget && _block_ui_top_widget->isHidden())
        _block_ui_top_widget->show();

    QWidget* top_modal = qApp->activeModalWidget();
#ifndef QT_DEBUG
    Dialog* parent_dialog = qobject_cast<Dialog*>(Utils::findTopParentWidget(mainWidget(), false, { QStringLiteral("zf::Dialog") }));
#endif

    // мы не хотим показывать "крутилку" если имеется модальный диалог верхнего уровня или некий виджет сверху
    if (top_modal != nullptr) {
        // надо отследить скрытие/удаление верхнего виджета и вернуть крутилку если надо
        if (!_top_widget_filtered.contains(top_modal)) {
            // Qt не имеет сигнала hide для виджетов - приходится ловить его так
            _top_widget_filtered << top_modal;
            top_modal->installEventFilter(this);
        }

        if (!_top_widget_destroyed_connection.contains(top_modal))
            _top_widget_destroyed_connection[top_modal] = connect(top_modal, &Dialog::destroyed, this, &View::sl_topWidgetDestroyed);
    }

#ifndef QT_DEBUG
    if (top_modal == nullptr || top_modal == parent_dialog) {
        if (_wait_movie_label->isHidden())
            _wait_movie_label->show();

        if (_wait_movie_label->movie()->state() == QMovie::NotRunning)
            _wait_movie_label->movie()->start();

        QList<QSize> sizes = QIcon(_wait_movie_label->movie()->fileName()).availableSizes();
        Z_CHECK(!sizes.isEmpty());

        int x = (_body->geometry().width() - sizes.first().width()) / 2;
        int y = (_body->geometry().height() - sizes.first().height()) / 2;

        _wait_movie_label->move(x, y);

#ifdef RNIKULENKOV
//        qDebug() << "show wait movie:" << objectName();
#endif

    } else {
        _wait_movie_label->hide();

#ifdef RNIKULENKOV
//        qDebug() << "hide wait movie:" << objectName();
#endif
    }

    if (_block_ui_top_widget)
        _block_ui_top_widget->setGeometry(_body->geometry());
#endif

    Utils::processEvents();
}

void View::updateBlockUiGeometry(bool force)
{
    if (objectExtensionDestroyed())
        return;

    if (force)
        updateBlockUiGeometryHelper();
    else
        internalCallbackManager()->addRequest(this, Framework::VIEW_UPDATE_BLOCK_UI_GEOMETRY_KEY);
}

void View::showBlockUi(bool force)
{
#ifdef RNIKULENKOV
//    qDebug() << "hideBlockUi" << objectName();
#endif

    if (force) {
        showBlockUiHelper(force);

    } else {
#ifdef RNIKULENKOV
//        qDebug() << "removeRequest VIEW_HIDE_BLOCK_UI_KEY" << objectName();
#endif

        internalCallbackManager()->removeRequest(this, Framework::VIEW_HIDE_BLOCK_UI_KEY);
        _show_block_ui_timer->start();
    }
}

void View::hideBlockUi()
{
    if (objectExtensionDestroyed())
        return;

#ifdef RNIKULENKOV
//    qDebug() << "addRequest VIEW_HIDE_BLOCK_UI_KEY" << objectName();
#endif

    internalCallbackManager()->addRequest(this, Framework::VIEW_HIDE_BLOCK_UI_KEY);
    _show_block_ui_timer->stop();

#ifdef RNIKULENKOV
//    qDebug() << "hideBlockUi" << objectName();
#endif
}

void View::showBlockUiHelper(bool force)
{
    if (!isBlockedUi())
        return;

    // если представление встроено в другое и оно ужу заблокировано, то блокировку не показываем
    if (isParentViewUiBlocked())
        return;

    for (auto v : qAsConst(_embed_views)) {
        v->hideBlockUiHelper();
    }

    _show_block_ui_timer->stop();

#ifndef QT_DEBUG
    QGraphicsColorizeEffect* effect = new QGraphicsColorizeEffect(_body);
    effect->setColor(QColor(50, 50, 50));

    _body->setGraphicsEffect(effect);
    _body->stackUnder(_block_ui_top_widget);
    _body->stackUnder(_wait_movie_label);
    _block_ui_top_widget->stackUnder(_wait_movie_label);
#endif

    for (auto t : toolbarsWithActions(false))
        t->setEnabled(false);

    updateBlockUiGeometry(force);

#ifdef RNIKULENKOV
//    qDebug() << "show block IU:" << objectName();
#endif
}

void View::hideBlockUiHelper()
{
#ifndef QT_DEBUG
    _body->setGraphicsEffect(nullptr);

    _main_widget->stackUnder(nullptr);
    _wait_movie_label->hide();
    _wait_movie_label->movie()->stop();

    _block_ui_top_widget->hide();
    _main_widget->stackUnder(nullptr);
    _block_ui_top_widget->stackUnder(nullptr);
#endif

    for (auto t : toolbarsWithActions(false))
        t->setEnabled(true);

#ifdef RNIKULENKOV
//    qDebug() << "hide block IU and movie:" << objectName();
#endif
}

void View::requestUpdateParentView()
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_UPDATE_PARENT_VIEW);   
}

void View::updateParentView()
{
    // ищем родительский диалог    
    Dialog* parent_dialog = nullptr;
    if (auto top = Utils::findParentByClass(moduleUI(), "zf::Dialog")) {
        if (qobject_cast<ModalWindow*>(top) == nullptr) {
            // диалог, но не стандартное окно редактирования
            parent_dialog = qobject_cast<Dialog*>(top);
            Z_CHECK_NULL(parent_dialog);
        }
    }

    bool parent_dialog_changed = false;
    if (_parent_dialog != parent_dialog) {
        // встроен в обычный диалог
        if (!_parent_dialog.isNull()) {
            _parent_dialog->embedViewDetachedHelper(this);
        }

        _parent_dialog = parent_dialog;
        _parent_dialog->embedViewAttachedHelper(this);

        emit sg_parentDialogChanged(_parent_dialog);
        parent_dialog_changed = true;

        loadConfiguration();
    }

    // ищем родительское представление
    // может быть цепочка родителей и надо отслеживать все, чтобы контролировать блокировку UI каждого
    QList<View*> parent_views;
    QHash<QObject*, QWidgetList> all_widgets = _objects_finder->childWidgets();
    for (auto i = all_widgets.constBegin(); i != all_widgets.constEnd(); ++i) {
        for (auto w : i.value()) {
            auto parents = Utils::findAllParents(w, {}, { View::MAIN_WIDGET_OBJECT_NAME });
            for (auto p : qAsConst(parents)) {
                auto value = p->property(View::MAIN_WIDGET_VIEW_PRT_NAME);
                Z_CHECK(value.isValid());
                auto ptr = value.value<qintptr>();
                Z_CHECK(ptr != 0);
                auto view = reinterpret_cast<View*>(ptr);
                if (view != this) {
                    if (!parent_views.contains(view))
                        parent_views << view;
                }
            }
        }
    }

    bool parent_view_changed = false;
    if (_parent_views.count() == parent_views.count()) {
        for (int i = 0; i < _parent_views.count(); i++) {
            if (_parent_views.at(i)->view == parent_views.at(i))
                continue;
            parent_view_changed = true;
            break;
        }
    } else {
        parent_view_changed = true;
    }

    if (parent_view_changed) {
        // встроен в другое представление

        // отключаемся от старого набора родителей
        for (auto& v : qAsConst(_parent_views)) {
            if (v->view.isNull())
                continue;

            if (!_view_options.testFlag(ViewOption::ShowBlockEmbedUi))
                disconnect(v->parent_view_block_ui_connection);

            disconnect(v->parent_view_readonly_changed_connection);
            v->view->embedViewDetachedHelper(this);
        }

        // подключаемся к новому
        _parent_views.clear();
        for (auto v : qAsConst(parent_views)) {
            auto v_ptr = Z_MAKE_SHARED(ParentViewInfo);
            v_ptr->view = v;

            if (!_view_options.testFlag(ViewOption::ShowBlockEmbedUi))
                v_ptr->parent_view_block_ui_connection = connect(v, &View::sg_uiBlocked, this, &View::sl_parentViewUiBlocked);

            v_ptr->parent_view_readonly_changed_connection = connect(v, &View::sg_readOnlyChanged, this, &View::onReadOnlyChangedHelper);
            v->embedViewAttachedHelper(this);
            _parent_views << v_ptr;
        }

        emit sg_parentViewChanged(parent_views);
        loadConfiguration();
    }

    if (parent_dialog_changed || parent_view_changed) {
        requestUpdateView();
        if (!isHighlightViewIsCheckRequired()) {
            setHightlightCheckRequired(false);

        } else {
            registerHighlightCheck();
        }

        requestProcessVisibleChange();

        if (isParentViewUiBlocked())
            hideBlockUiHelper();
    }
}

bool View::isParentViewUiBlocked() const
{
    if (_view_options.testFlag(ViewOption::ShowBlockEmbedUi))
        return false;

    for (auto& v : qAsConst(_parent_views)) {
        if (!v->view.isNull() && v->view->isBlockedUi())
            return true;
    }
    return false;
}

bool View::isCatchDragDropEvents(QWidget* w) const
{
    // заголовки таблиц перетаскиваются средствами самой таблицы
    if (qobject_cast<QHeaderView*>(w) != nullptr || qobject_cast<QHeaderView*>(Utils::getMainWidget(w)) != nullptr
        || qobject_cast<PictureSelector*>(Utils::getMainWidget(w)) != nullptr || qobject_cast<ImageList*>(Utils::getMainWidget(w)) != nullptr)
        return false;
    return true;
}

void View::setStates(const ViewStates& s)
{
    if (s == _states)
        return;

    auto old = _states;
    _states = s;

    if (!_is_constructing) {
        onStatesChanged(s, old);
        emit sg_statesChanged(s, old);
    }
}

void View::requestInitTabOrder()
{
    if (objectExtensionDestroyed())
        return;

    // Автоматическая расстановка табуляции
    internalCallbackManager()->addRequest(this, Framework::VIEW_INIT_TAB_ORDER_KEY);
}

void View::initTabOrder()
{
    QWidget* w = qobject_cast<QWidget*>(Utils::findParentByClass(moduleUI(), "QDialog"));
    if (w == nullptr)
        w = qobject_cast<QWidget*>(Utils::findParentByClass(moduleUI(), "QMdiSubWindow"));
    if (w == nullptr)
        w = qobject_cast<QWidget*>(Utils::findParentByClass(moduleUI(), "QMainWindow"));

    if (w == nullptr || w->layout() == nullptr)
        w = moduleUI();

    Z_CHECK_NULL(moduleUI()->layout());

    QList<QWidget*> tab_order;
    Utils::createTabOrder(w->layout(), tab_order);
    setTabOrder(tab_order);
}

void View::loadAllHighlights()
{
    _widget_highlighter->clear();
    for (int i = 0; i < highlight(false)->count(); i++) {
        HighlightItem item = highlight(false)->item(i);
        QWidget* w = getHighlightWidget(item.property());
        if (w == nullptr)
            continue;

        _widget_highlighter->addHighlight(w, item);
    }
    updateStatusBarErrorInfo();
}

void View::updateStatusBarErrorInfo()
{
    MessageUpdateStatusBar::updateStatusBarErrorInfo(widgets(), highlightProcessor());
}

bool View::getDatasetSelectionProperty(QObject* sender, DataProperty& p) const
{
    QItemSelectionModel* selection_model = qobject_cast<QItemSelectionModel*>(sender);
    Z_CHECK_NULL(selection_model);
    p = widgetProperty(qobject_cast<QWidget*>(selection_model->parent()));
    Z_CHECK(p.isValid());

    TableView* table_view = qobject_cast<TableView*>(selection_model->parent());
    if (table_view != nullptr && table_view->isReloading())
        return false;

    TreeView* tree_view = qobject_cast<TreeView*>(selection_model->parent());
    if (tree_view != nullptr && tree_view->isReloading())
        return false;

    return true;
}

void View::getDatasetCellPropertiesHelper(const DataProperty& column, const QModelIndex& index, QStyle::State state, QFont& font, QBrush& background,
    QPalette& palette, QIcon& icon, Qt::Alignment& alignment) const
{
    getDatasetCellProperties(index.row(), column, index.parent(), state, font, background, palette, icon, alignment);
}

bool View::processDatasetKeyPressHelper(const DataProperty& dataset_property, QAbstractItemView* view, QKeyEvent* e)
{
    if (_process_dataset_press_helper)
        return true; // для исключения рекурсии
    _process_dataset_press_helper = true;

    if (processDatasetKeyPress(dataset_property, view, e)) {
        _process_dataset_press_helper = false;
        return true;
    }

    QString clipboard_text;
    if (e->matches(QKeySequence::Paste)) {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard_text = clipboard->text();
    }

    QModelIndex current_index = currentIndex(dataset_property);

    DataProperty column_property;
    if (current_index.isValid() && dataset_property.columnCount() > current_index.column())
        column_property = dataset_property.columns().at(current_index.column());

    if (e->modifiers() == Qt::NoModifier) {
        // Когда активно несколькор наборов данных, может получиться что Qt не может оппределить какой из них
        // должен обработать нажатие del или ins
        if (e->key() == Qt::Key_Insert) {
            // Принудительно проверяем доступность действий
            updateAccessRightsHelper();
            if (Utils::isActionEnabled(widgets()->datasetAction(dataset_property, ObjectActionType::Create)))
                onDatasetActionActivated(dataset_property, ObjectActionType::Create, current_index, column_property);

            _process_dataset_press_helper = false;
            return true;

        } else if (e->key() == Qt::Key_Delete) {
            // Принудительно проверяем доступность действий
            updateAccessRightsHelper();
            if (Utils::isActionEnabled(widgets()->datasetAction(dataset_property, ObjectActionType::Remove)))
                onDatasetActionActivated(dataset_property, ObjectActionType::Remove, current_index, column_property);

            _process_dataset_press_helper = false;
            return true;

        } else if (e->key() == Qt::Key_F2 || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            // Принудительно проверяем доступность действий
            updateAccessRightsHelper();
            if (QApplication::focusWidget() == view) {
                if (Utils::isActionEnabled(widgets()->datasetAction(dataset_property, ObjectActionType::Modify)))
                    onDatasetActionActivated(dataset_property, ObjectActionType::Modify, current_index, column_property);
                else if (Utils::isActionEnabled(widgets()->datasetAction(dataset_property, ObjectActionType::View)))
                    onDatasetActionActivated(dataset_property, ObjectActionType::View, current_index, column_property);
            }

            _process_dataset_press_helper = false;
            return true;
        }
    }

    if (((e->modifiers() & Qt::AltModifier) > 0) && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) && view->currentIndex().isValid()
        && dataset_property.dataType() == DataType::Tree) {
        // expand/collapse для treeview по CTRL LEFT/RIGHT
        QTreeView* tw = qobject_cast<QTreeView*>(view);
        Z_CHECK_NULL(tw);

        QModelIndex idx = tw->currentIndex();
        idx = tw->model()->index(idx.row(), tw->treePosition(), idx.parent());
        if (e->key() == Qt::Key_Left)
            tw->collapse(idx);
        else
            tw->expand(idx);

        _process_dataset_press_helper = false;
        e->ignore();
        return true;
    }

    if (((e->modifiers() & Qt::ControlModifier) > 0) && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) && view->currentIndex().isValid()) {
        // переход в лево или вправо набора данных
        auto root = horizontalRootHeaderItem(dataset_property);
        auto visual_headers = root->allBottomVisual(Qt::AscendingOrder, true);
        if (!visual_headers.isEmpty()) {
            QModelIndex idx;
            if (e->key() == Qt::Key_Left)
                idx = view->model()->index(view->currentIndex().row(), visual_headers.first()->bottomPos(), view->currentIndex().parent());
            else
                idx = view->model()->index(view->currentIndex().row(), visual_headers.last()->bottomPos(), view->currentIndex().parent());

            view->setCurrentIndex(idx);
        }

        _process_dataset_press_helper = false;
        e->ignore();
        return true;
    }

    if (((e->modifiers() & Qt::ControlModifier) > 0) && (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) && view->currentIndex().isValid()) {
        // переход в начало или конец набора данных
        QModelIndex idx;
        if (e->key() == Qt::Key_Up)
            idx = view->model()->index(0, view->currentIndex().column());
        else {
            idx = Utils::findLastDatasetIndex(view->model(), QModelIndex());
            idx = view->model()->index(idx.row(), view->currentIndex().column(), idx.parent());
        }
        view->setCurrentIndex(idx);

        _process_dataset_press_helper = false;
        e->ignore();
        return true;
    }

    if (QApplication::focusWidget() == view) {
        if (column_property.isValid()
            && (column_property.options().testFlag(PropertyOption::IgnoreReadOnly) || dataset_property.options().testFlag(PropertyOption::IgnoreReadOnly)
                || (!isReadOnly() && !dataset_property.options().testFlag(PropertyOption::ReadOnly)
                    && !column_property.options().testFlag(PropertyOption::ReadOnly)))) {
            // Правка в ячейке
            QString text;
            if (e->key() == Qt::Key_Space)
                text = " ";
            else if (e->key() != Qt::Key_Escape && e->key() != Qt::Key_Backspace)
                text = e->text().simplified();

            bool is_text_key = !e->text().isEmpty() && e->text().at(0).isPrint();
            if (e->key() == Qt::Key_Backspace || is_text_key || !clipboard_text.isEmpty()) {
                // Активирована правка в ячейке
                editCellInternal(dataset_property, current_index, e, clipboard_text.isEmpty() ? text : clipboard_text);
                _process_dataset_press_helper = false;
                return true;
            }

        } else {
            if (!e->modifiers().testFlag(Qt::AltModifier) && !e->modifiers().testFlag(Qt::ControlModifier) && data()->rowCount(dataset_property) > 0) {
                // слепой поиск
                QString new_text = e->text() == " " ? " " : e->text().simplified();
                if (!new_text.isEmpty()) {
                    _last_search += new_text;
                    _last_search = _last_search.trimmed();
                    if (!_last_search.isEmpty()) {
                        searchText(dataset_property, _last_search);
                        bool started = !_search_timer->isActive();
                        _search_timer->start();

                        if (started)
                            emit sg_autoSearchStatusChanged(true);
                        emit sg_autoSearchTextChanged(_last_search);
                    }
                }
            }
        }
    }

    _process_dataset_press_helper = false;
    return false;
}

void View::requestUpdateAccessRights() const
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_REQUEST_UPDATE_ACCESS_RIGHTS);
}

void View::requestUpdateWidgetStatuses() const
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS);
    _widgets_to_update.clear();
    internalCallbackManager()->removeRequest(this, Framework::VIEW_REQUEST_UPDATE_WIDGET_STATUS);
}

void View::requestUpdateWidgetStatus(QWidget* w) const
{
    if (objectExtensionDestroyed())
        return;

    if (!internalCallbackManager()->isRequested(this, Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS)) {
        auto p = widgetProperty(w);
        if (p.isValid()
            && (_properties_to_update.contains(p) || internalCallbackManager()->isRequested(this, Framework::VIEW_REQUEST_UPDATE_ALL_PROPERTY_STATUS)))
            return; // и так обновится через обновление свойств

        _widgets_to_update << w;
        internalCallbackManager()->addRequest(this, Framework::VIEW_REQUEST_UPDATE_WIDGET_STATUS);
    }
}

void View::requestUpdateWidgetStatus(const QString& widget_name) const
{
    requestUpdateWidgetStatus(object<QWidget>(widget_name));
}

void View::updateAllWidgetsStatusHelper()
{
    internalCallbackManager()->removeRequest(this, Framework::VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS);

    QObjectList ls;
    _objects_finder->findObjectsByClass(ls, QStringLiteral("QWidget"), false, false);
    for (QObject* obj : qAsConst(ls)) {
        QWidget* w = static_cast<QWidget*>(obj);
        auto property = widgetProperty(w);
        if (property.isValid() && widget(property) != w)
            continue; // для исключения ситуации, когда виджет свойства содержит вложенные виджеты (например QPlainTextEdit содержит QScrollArea)

        updateWidgetStatus(w, property);
        if (property.isValid())
            _properties_to_update.remove(property);
    }
}

void View::updateAccessRightsHelper() const
{
    internalCallbackManager()->removeRequest(this, Framework::VIEW_REQUEST_UPDATE_ACCESS_RIGHTS);
    const_cast<View*>(this)->onUpdateAccessRights();
    const_cast<View*>(this)->compressActionsHelper();
}

bool View::editCellInternal(const DataProperty& dataset_property, QModelIndex current_index, QKeyEvent* e, const QString& text)
{
    Z_CHECK(current_index.isValid());
    if (isEmbeddedWindow())
        return false;

    auto column = indexColumn(current_index);
    if ((isReadOnly() || dataset_property.options().testFlag(PropertyOption::ReadOnly) || column.options().testFlag(PropertyOption::ReadOnly))
        && !dataset_property.options().testFlag(PropertyOption::IgnoreReadOnly) && !column.options().testFlag(PropertyOption::IgnoreReadOnly))
        return false;

    if (column.isValid() && isDatasetCellReadOnly(current_index.row(), column, current_index.parent()))
        return false;

    return onEditCell(dataset_property, current_index, e, text);
}

void View::updateSearchWidget()
{
    if (_search_widget.isNull())
        return;

    _search_widget->setCurrentValue(entityUid().isTemporary() ? QVariant() : entityUid().id().value());
}

void View::afterLoadConfigurationHelper()
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_AFTER_LOAD_CONFIG_KEY);
}

void View::fixFocusedWidget()
{
    if (_last_focused == nullptr || !Utils::isVisible(_last_focused))
        return;

    if (widgetProperty(_last_focused).isValid() || widgetProperty(Utils::getBaseChildWidget(_last_focused)).isValid())
        return; // это свойство

    if (Utils::findTopParentWidget(_last_focused, false, {}, View::MAIN_WIDGET_OBJECT_NAME) == nullptr)
        return; // не это view

    for (auto& dataset : structure()->propertiesByType(PropertyType::Dataset)) {
        if (dataset.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        if (!Utils::hasParent(object<QAbstractItemView>(dataset), _last_focused))
            continue;

        setFocus(dataset);
        break;
    }
}

void View::autoSelectFirstRow(const DataProperty& dataset_property)
{
    if (objectExtensionDestroyed())
        return;

    Z_CHECK(dataset_property.isDataset());
    _auto_select_first_row_requests << dataset_property;
    internalCallbackManager()->addRequest(this, Framework::VIEW_AUTO_SELECT_FIRST_ROW);
}

void View::processAutoSelectFirstRow()
{
    for (auto& p : qAsConst(_auto_select_first_row_requests)) {
        auto item_view = object<QAbstractItemView>(p);
        if (item_view->currentIndex().isValid() || proxyDataset(p)->rowCount() == 0)
            continue;

        int column = firstVisibleColumn(p);
        if (column < 0)
            continue;

        QModelIndex index = proxyDataset(p)->index(0, column);

        setCurrentIndex(p, index);
    }

    _auto_select_first_row_requests.clear();
}

bool View::processTopWidgetFilter(QObject* obj, QEvent* event)
{
    if (!_top_widget_filtered.contains(obj))
        return false;

    if (event->type() == QEvent::Hide)
        stopProcessTopWidgetHide(obj);

    return true;
}

void View::stopProcessTopWidgetHide(QObject* obj)
{
    Z_CHECK(_top_widget_filtered.contains(obj));
    _top_widget_filtered.remove(obj);
    obj->removeEventFilter(this);

    Z_CHECK(_top_widget_destroyed_connection.contains(obj));
    disconnect(_top_widget_destroyed_connection.value(obj));
    _top_widget_destroyed_connection.remove(obj);

    // надо решить показывать ли снова "крутилку"
    updateBlockUiGeometry(false);
}

bool View::loadModel()
{
    if (!_visible_status)
        return true;

    if (model()->isLoading()) {
        _request_load_after_loading = true;
        return true;
    }

    // qDebug() << __FUNCTION__ << " " << this->debugName();
    // qDebug() << isModalWindowInterfaceInstalled();

    auto window_options = isModalWindowInterfaceInstalled() ? modalWindowInterface()->options() : zf::ModuleWindowOption::NoOptions;
    if (!window_options.testFlag(zf::ModuleWindowOption::EditWindowDisableAutoLoadModel)) {
        // какие свойства видны
        DataPropertySet load_props;
        auto visible_props = visibleProperties();

        for (auto& p : structure()->propertiesMain()) {
            if (isPartialLoad(p)) {
                if (visible_props.contains(p))
                    load_props << p;

            } else {
                load_props << p;
            }
        }

        if (!load_props.isEmpty())
            model()->load(LoadOptions(), load_props);
    }
    return model()->isLoading();
}

void View::requestProcessVisibleChange() const
{
    if (objectExtensionDestroyed())
        return;

    internalCallbackManager()->addRequest(this, Framework::VIEW_REQUEST_PROCESS_VISIBLE_CHANGE);
}

void View::processVisibleChange()
{
    loadModel();
    for (auto& v : qAsConst(_embed_views)) {
        v->requestProcessVisibleChange();
    }
}

void View::onReadOnlyChangedHelper()
{
    requestUpdateView();

    if (!isHighlightViewIsCheckRequired())
        highlightProcessor()->clearHighlights();
    else
        registerHighlightCheck();

    emit sg_readOnlyChanged(isReadOnly());
}

void View::embedViewAttachedHelper(View* view)
{
    Z_CHECK(!_embed_views.contains(view));
    _embed_views << view;

    //    qDebug() << view->entityCode().value() << "attached to" << entityCode().value();

    connect(view, &QObject::destroyed, this, [&](QObject* obj) {
        for (int i = 0; i < _embed_views.count(); i++) {
            if (_embed_views.at(i) != obj)
                continue;

            _embed_views.removeAt(i);
            break;
        }
    });

    embedViewAttached(view);
}

void View::embedViewDetachedHelper(View* view)
{
    Z_CHECK(_embed_views.removeOne(view));
    embedViewDetached(view);
}

void View::processErrorHelper(ObjectActionType type, const Error& error)
{
    Z_CHECK(error.isError());

    if (isNotInWindow())
        return; // окно вообще никуда не встроно, показывать ошибки не надо

    if (_block_show_errors > 0)
        return;

    if (!isEmbeddedWindow()) {
        if (Utils::isVisible(moduleUI(), true))
            Core::postError(error);

    } else {
        if (!_parent_views.isEmpty()) {
            topParentView()->onEmbedViewError(this, type, error);

        } else if (!_parent_dialog.isNull()) {
            _parent_dialog->onEmbedViewError(this, type, error);

        } else {
            Z_HALT_INT;
        }
    }
}

void View::onStartLoading()
{
    if (_loading)
        return;

    _loading = true;
    emit sg_startLoad();
}

void View::onFinishLoading()
{
    if (!_loading)
        return;

    _loading = false;
    emit sg_finishLoad();

    onDataReadyHelper();

    _reload_reason.clear();
}

void View::fillLookupValues(const DataProperty& property)
{
    // если в лукапе один элемент и поле обязательное, то подставляем значение сразу

    if (isPersistent() || model()->isLoading() || !states().testFlag(ViewState::EditWindow))
        return;

    if (property.isValid()
        && (property.options().testFlag(PropertyOption::NoInitLookup)
            || (!property.options().testFlag(PropertyOption::ForceInitLookup) && !property.hasConstraints(ConditionType::Required))
            || property.lookup() == nullptr || (property.lookup()->type() != LookupType::Dataset && property.lookup()->type() != LookupType::List)))
        return;

    if (!property.isValid()) {
        for (auto& p : structure()->propertiesMain()) {
            fillLookupValues(p);
        }

        return;
    }

    if (property.propertyType() == PropertyType::Field
        || (property.propertyType() == PropertyType::Dataset && property.options().testFlag(PropertyOption::SimpleDataset))) {
        if (property.propertyType() == PropertyType::Field) {
            auto v = value(property);
            if (v.isValid() && !v.isNull())
                return;
        } else {
            if (rowCount(property) > 0)
                return;
        }

        QVariant lookup_value;
        if (property.lookup()->type() == LookupType::List) {
            if (property.lookup()->listValues().count() != 1)
                return;
            lookup_value = property.lookup()->listValues().constFirst();

        } else {
            auto is = object<ItemSelector>(property);
            Z_CHECK_NULL(is->itemModel());
            if (is->itemModel()->rowCount() != 1)
                return;

            Z_CHECK_NULL(is->model());
            lookup_value = is->model()->cell(0, property.lookup()->dataColumnID(), property.lookup()->dataColumnRole());
            lookup_value = DataWidgets::convertFromLookupValue(property, lookup_value);
        }

        if (property.propertyType() == PropertyType::Field)
            setValue(property, lookup_value);
        else
            data()->setSimpleValues(property, { lookup_value });
    }
}

void View::setCustomData(const QMap<QString, QVariant>& custom_data)
{
    Z_CHECK(_custom_data.isEmpty());
    _custom_data = custom_data;
}

WidgetHighligterController* View::widgetHighligterController() const
{
    // _widget_highligter_controller инициализируется внутри widgets()
    widgets();
    return _widget_highligter_controller.get();
}

HighlightMapping* View::highligtMapping() const
{
    // _highligt_mapping инициализируется внутри widgets()
    widgets();
    return _highligt_mapping;
}

View::PropertyInfo* View::propertyInfo(const DataProperty& p) const
{
    Z_CHECK(structure()->contains(p));
    auto info = _property_info.value(p);
    if (info == nullptr) {
        info = Z_MAKE_SHARED(PropertyInfo);
        _property_info[p] = info;
    }
    return info.get();
}

QHash<EntityCode, QDateTime>* View::globalConfigTime()
{
    if (_global_config_time == nullptr)
        _global_config_time = std::make_unique<QHash<EntityCode, QDateTime>>();
    return _global_config_time.get();
}

bool View::WindowConfiguration::isValid() const
{
    return state != Qt::WindowNoState || !size.isEmpty();
}

void zf::View::onActivatedInternal(bool active)
{
    EntityObject::onActivatedInternal(active);
    requestUpdateView();

    if (active)
        requestProcessVisibleChange();

    auto embed_objects = embedViews();
    for (auto v : qAsConst(embed_objects)) {
        v->requestProcessVisibleChange();
    }
}

DataWidgets* View::hightlightControllerGetWidgets() const
{
    return widgets();
}

HighlightProcessor* View::hightlightControllerGetProcessor() const
{
    return highlightProcessor();
}

HighlightMapping* View::hightlightControllerGetMapping() const
{
    return highligtMapping();
}

DataProperty View::hightlightControllerGetCurrentFocus() const
{
    return currentFocus();
}

bool View::hightlightControllerSetFocus(const DataProperty& p)
{
    return setFocus(p);
}

void View::hightlightControllerBeforeFocusHighlight(const DataProperty& property)
{
    beforeFocusHighlight(property);
}

void View::hightlightControllerAfterFocusHighlight(const DataProperty& property)
{
    afterFocusHighlight(property);
}

void View::onEntityNameChangedHelper()
{
    EntityObject::onEntityNameChangedHelper();
}

} // namespace zf
