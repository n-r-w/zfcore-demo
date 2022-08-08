#include "zf_data_widgets.h"

#include "zf_collapsible_group_box.h"
#include "zf_core.h"
#include "zf_date_edit.h"
#include "zf_date_time_edit.h"
#include "zf_double_spin_box.h"
#include "zf_framework.h"
#include "zf_header_view.h"
#include "zf_item_selector.h"
#include "zf_line_edit.h"
#include "zf_plain_text_edit.h"
#include "zf_proxy_item_model.h"
#include "zf_spin_box.h"
#include "zf_table_view.h"
#include "zf_text_edit.h"
#include "zf_tree_view.h"
#include "zf_uid.h"
#include "zf_view.h"
#include "zf_data_filter.h"
#include "zf_translation.h"
#include "zf_picture_selector.h"
#include "zf_checkbox.h"
#include "zf_time_edit.h"
#include "zf_request_selector.h"
#include "zf_entity_name_widget.h"
#include "zf_colors_def.h"
#include "zf_html_tools.h"
#include "zf_ui_size.h"
#include "services/item_model/item_model_service.h"

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QApplication>

namespace zf
{
static QString loagingTextHelper(const QWidget* w)
{
#ifdef RNIKULENKOV
    return w->objectName().isEmpty()? QString("%1...").arg(w->metaObject()->className()) : QString("%1...").arg(w->objectName());
#else
    Q_UNUSED(w);
    return ZF_TR(ZFT_LOADING);
#endif
}

DataWidgets::UpdateReason DataWidgets::modelStatusToUpdateReason(const ModelPtr& model)
{
    Z_CHECK_NULL(model);
    if (model->isLoading())
        return UpdateReason::Loading;
    if (model->isRemoving())
        return UpdateReason::Removing;
    if (model->isSaving())
        return UpdateReason::Saving;

    return UpdateReason::NoReason;
}

DataWidgets::UpdateReason DataWidgets::getImportantUpdateReason(DataWidgets::UpdateReason r1, DataWidgets::UpdateReason r2)
{
    return static_cast<int>(r1) > static_cast<int>(r2) ? r1 : r2;
}

DataWidgets::DataWidgets(const DatabaseID& database_id, const DataContainerPtr& data, I_DataWidgetsLookupModels* dynamic_lookup,
    CallbackManager* callback_manager, HighlightProcessor* highlight)
    : _dynamic_lookup(dynamic_lookup)
    , _filter(nullptr)
    , _callback_manager(callback_manager ? callback_manager : Framework::internalCallbackManager())
{
    Z_CHECK(database_id.isValid());
    bootstrap();
    setSource(database_id, data, highlight);
}

DataWidgets::DataWidgets(const DataContainerPtr& data, CallbackManager* callback_manager, HighlightProcessor* highlight)
    : _callback_manager(callback_manager ? callback_manager : Framework::internalCallbackManager())
{
    bootstrap();
    setSource({}, data, highlight);
}

DataWidgets::DataWidgets(View* view, DataFilter* filter, I_DataWidgetsLookupModels* dynamic_lookup, CallbackManager* callback_manager)
    : _dynamic_lookup(dynamic_lookup)
    , _callback_manager(callback_manager ? callback_manager : Framework::internalCallbackManager())
{
    bootstrap();
    setSource(view, filter);

    connect(view, &View::sg_afterModelChange, this, &DataWidgets::sl_afterModelChange);
}

DataWidgets::~DataWidgets()
{
    clearWidgets();
    delete _object_extension;
}

bool DataWidgets::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void DataWidgets::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant DataWidgets::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool DataWidgets::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void DataWidgets::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void DataWidgets::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void DataWidgets::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void DataWidgets::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void DataWidgets::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void DataWidgets::setSource(const DatabaseID& database_id, const DataContainerPtr& data, HighlightProcessor* highlight)
{
    Z_CHECK_NULL(data);

    if (_database_id == database_id && _data == data)
        return;

    _database_id = database_id;
    _highlight = highlight;
    setDataSource(data);
}

void DataWidgets::setSource(View* view, DataFilter* filter)
{
    Z_CHECK_NULL(view);

    if (_view == view && _filter == filter)
        return;

    _callback_manager->removeRequest(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY);

    if (_view != nullptr)
        disconnect(_view->model().get(), &Model::sg_finishLoad, this, &DataWidgets::sl_model_finishLoad);

    _view = view;
    _highlight = _view->highlightProcessor();
    _filter = filter;

    _database_id = view->databaseId();

    if (_filter != nullptr)
        Z_CHECK(_filter->source()->entityCode() == _view->entityCode());

    setDataSource(_view->data());

    connect(view, &View::sg_model_finishLoad, this, &DataWidgets::sl_model_finishLoad);
}

DataContainerPtr DataWidgets::data() const
{
    return _data;
}

DataStructurePtr DataWidgets::structure() const
{
    return _data->structure();
}

DataFilter* DataWidgets::filter() const
{
    return _filter;
}

DataFilter* DataWidgets::lookupFilter(const PropertyID& property_id) const
{
    return lookupFilter(property(property_id));
}

DataFilter* DataWidgets::lookupFilter(const DataProperty& property) const
{
    DataFilter* filter = propertyInfo(property, true)->lookup_filter.get();
    Z_CHECK_X(filter != nullptr, QString("lookup not found %1:%2").arg(property.entityCode().value()).arg(property.id().value()));
    return filter;
}

void DataWidgets::generateWidgets()
{
    clearWidgets();

    for (auto& p : _data->structure()->propertiesMain()) {
        generateWidgetHelper(p);
    }
}

void DataWidgets::clearWidgets()
{
    for (auto& w : _properties_info) {
        if (!w->field.isNull()) {
            I_ObjectExtension* o_ext = dynamic_cast<I_ObjectExtension*>(w->field.data());
            if (o_ext != nullptr) {
                Utils::deleteLater(w->field);

            } else {
                delete w->field;
            }
        }

        if (!w->dataset_group.isNull())
            delete w->dataset_group;
    }
    _properties_info.clear();
    _properties.clear();
}

void DataWidgets::updateWidgets(UpdateReason reason)
{
    if (_update_widgets_in_process)
        return;

    _update_widgets_in_process = true;

    Core::messageDispatcher()->stop();
    CallbackManager::stopAll();

    QElapsedTimer timer;
    timer.start();

    // порядок обновления: сначала основные, а потом зависимые от них
    DataPropertySet updated;
    for (auto& p : structure()->propertiesMain()) {
        if (updated.contains(p))
            continue;
        updated << p;

        for (auto& link : p.links()) {
            PropertyIDList parents;
            switch (link->type()) {
                case PropertyLinkType::DataSource:
                    // данные находятся в другой модели - ничего не делаем
                    break;
                case PropertyLinkType::DataSourcePriority:
                    parents << link->dataSourcePriority();
                    break;
                case PropertyLinkType::LookupFreeText:
                    parents << link->linkedPropertyId();
                    break;
                default:
                    Z_HALT_INT;
                    break;
            }
            for (auto& p : qAsConst(parents)) {
                updateWidgetHelper(property(p), reason);
                updated << property(p);
            }
        }

        updateWidgetHelper(p, reason);

        // обновляем экран не слишком часто
        if (timer.elapsed() > 100) {
            Utils::processEvents(QEventLoop::ExcludeUserInputEvents);
            timer.restart();
        }
    }

    _update_widgets_in_process = false;

    CallbackManager::startAll();
    Core::messageDispatcher()->start();

    emit sg_widgetsUpdated();
}

DataProperty DataWidgets::property(const PropertyID& property_id) const
{
    return data()->property(property_id);
}

QWidget* DataWidgets::field(const DataProperty& property, bool halt_if_not_found) const
{
    checkProperty(property, {PropertyType::Field, PropertyType::Dataset});
    auto info = propertyInfo(property, halt_if_not_found);
    if (info != nullptr) {
        if (!halt_if_not_found && info->field == nullptr)
            return nullptr;
        Z_CHECK_NULL(info->field);
    }
    return info == nullptr ? nullptr : info->field;
}

QWidget* DataWidgets::field(const PropertyID& property_id, bool halt_if_not_found) const
{
    return field(property(property_id), halt_if_not_found);
}

QWidget* DataWidgets::complexField(const DataProperty& property, bool halt_if_not_found) const
{
    if (property.propertyType() == PropertyType::Dataset && !property.options().testFlag(PropertyOption::SimpleDataset))
        return datasetGroup(property, halt_if_not_found);

    return field(property, halt_if_not_found);
}

QWidget* DataWidgets::complexField(const PropertyID& property_id, bool halt_if_not_found) const
{
    return complexField(property(property_id), halt_if_not_found);
}

DataProperty DataWidgets::widgetProperty(const QWidget* widget) const
{
    return _properties.value(Utils::getMainWidget(const_cast<QWidget*>(widget)));
}

QWidget* DataWidgets::datasetGroup(const DataProperty& property, bool halt_if_not_found) const
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property, halt_if_not_found);
    return info == nullptr ? nullptr : info->dataset_group;
}

QWidget* DataWidgets::datasetGroup(const PropertyID& property_id, bool halt_if_not_found) const
{
    return datasetGroup(property(property_id), halt_if_not_found);
}

QToolBar* DataWidgets::datasetToolbar(const DataProperty& property, bool halt_if_not_found) const
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property, halt_if_not_found);
    return info == nullptr ? nullptr : info->dataset_toolbar;
}

QToolBar* DataWidgets::datasetToolbar(const PropertyID& property_id, bool halt_if_not_found) const
{
    return datasetToolbar(property(property_id), halt_if_not_found);
}

QFrame* DataWidgets::datasetToolbarWidget(const DataProperty& property, bool halt_if_not_found) const
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property, halt_if_not_found);
    return info == nullptr ? nullptr : info->dataset_toolbar_widget;
}

QFrame* DataWidgets::datasetToolbarWidget(const PropertyID& property_id, bool halt_if_not_found) const
{
    return datasetToolbarWidget(property(property_id), halt_if_not_found);
}

void DataWidgets::setDatasetToolbarWidgetStyle(bool has_left_border, bool has_right_border, bool has_top_border, bool has_bottom_border, QFrame* toolbar_widget)
{
    Z_CHECK_NULL(toolbar_widget);
    toolbar_widget->setStyleSheet(QStringLiteral("QFrame {"
                                                 "border: 1px %1;"
                                                 "border-top-style: %2; "
                                                 "border-right-style: %3; "
                                                 "border-bottom-style: %4; "
                                                 "border-left-style: %5;"
                                                 "}")
                                      .arg(Colors::uiLineColor(true).name())
                                      .arg(has_top_border ? "solid" : "none")
                                      .arg(has_right_border ? "solid" : "none")
                                      .arg(has_bottom_border ? "solid" : "none")
                                      .arg(has_left_border ? "solid" : "none"));
}

void DataWidgets::setDatasetToolbarHidden(const DataProperty& property, bool hidden, bool halt_if_not_found)
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property, halt_if_not_found);
    if (info == nullptr || info->dataset_toolbar_widget->isHidden() == hidden)
        return;
    info->dataset_toolbar_widget->setHidden(hidden);
}

void DataWidgets::setDatasetToolbarHidden(const PropertyID& property_id, bool hidden, bool halt_if_not_found)
{
    setDatasetToolbarHidden(property(property_id), hidden, halt_if_not_found);
}

bool DataWidgets::isDatasetToolbarHidden(const DataProperty& property, bool halt_if_not_found) const
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    auto info = propertyInfo(property, halt_if_not_found);
    return info == nullptr ? true : info->dataset_toolbar_widget->isHidden();
}

bool DataWidgets::isDatasetToolbarHidden(const PropertyID& property_id, bool halt_if_not_found) const
{
    return isDatasetToolbarHidden(property(property_id), halt_if_not_found);
}

QAction* DataWidgets::datasetAction(const DataProperty& property, ObjectActionType type, bool halt_if_not_found) const
{
    checkProperty(property, PropertyType::Dataset);
    Z_CHECK(!property.options().testFlag(PropertyOption::SimpleDataset));

    Z_CHECK(type != ObjectActionType::Undefined);
    auto info = propertyInfo(property, halt_if_not_found);
    if (info == nullptr)
        return nullptr;
    QAction* a = info->dataset_actions.value(type);
    Z_CHECK_NULL(a);
    return a;
}

QAction* DataWidgets::datasetAction(const PropertyID& property_id, ObjectActionType type, bool halt_if_not_found) const
{
    return datasetAction(property(property_id), type, halt_if_not_found);
}

bool DataWidgets::activateDatasetAction(const DataProperty& property, ObjectActionType type, bool halt_if_not_found)
{
    auto a = datasetAction(property, type, halt_if_not_found);
    if (Utils::isActionEnabled(a)) {
        a->activate(QAction::Trigger);
        return true;
    } else {
        return false;
    }
}

bool DataWidgets::activateDatasetAction(const PropertyID& property_id, ObjectActionType type, bool halt_if_not_found)
{
    return activateDatasetAction(property(property_id), type, halt_if_not_found);
}

QToolButton* DataWidgets::datasetActionButton(const DataProperty& property, ObjectActionType type) const
{
    auto button = qobject_cast<QToolButton*>(datasetToolbar(property)->widgetForAction(datasetAction(property, type)));
    Z_CHECK_NULL(button);
    return button;
}

QToolButton* DataWidgets::datasetActionButton(const PropertyID& property_id, ObjectActionType type) const
{
    return datasetActionButton(property(property_id), type);
}

void DataWidgets::createDatasetActionMenu(const DataProperty& property, ObjectActionType type, const QList<QAction*>& actions)
{
    if (actions.isEmpty())
        return;

    QToolButton* button = datasetActionButton(property, type);
    QMenu* menu = new QMenu(button);
    menu->addActions(actions);
    datasetAction(property, type)->setMenu(menu);

    Utils::prepareToolbarButton(button);

    button->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

void DataWidgets::createDatasetActionMenu(const PropertyID& property_id, ObjectActionType type, const QList<QAction*>& actions)
{
    createDatasetActionMenu(property(property_id), type, actions);
}

QWidget* DataWidgets::content() const
{
    if (_content == nullptr) {
        _content = createContentWidget();
        _content->installEventFilter(const_cast<DataWidgets*>(this));
    }

    return _content;
}

bool DataWidgets::isLookupLoading() const
{
    return _lookup_loading_counter > 0;
}

DataProperty DataWidgets::indexColumn(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());

    QModelIndex source_idx;
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        source_idx = Utils::alignIndexToModel(index, data()->dataset(p));
        if (source_idx.isValid())
            break;
    }

    if (!source_idx.isValid())
        return DataProperty();

    return data()->indexColumn(source_idx);
}

QModelIndex DataWidgets::currentIndex(const DataProperty& dataset_property) const
{
    QModelIndex index = field<QAbstractItemView>(dataset_property)->currentIndex();
    if (!index.isValid())
        return QModelIndex();

    return Utils::alignIndexToModel(index, data()->dataset(dataset_property));
}

QModelIndex DataWidgets::currentIndex(const PropertyID& dataset_property_id) const
{
    return currentIndex(property(dataset_property_id));
}

QModelIndex DataWidgets::currentIndex() const
{
    return currentIndex(structure()->singleDataset());
}

int DataWidgets::currentRow(const DataProperty& dataset_property) const
{
    Z_CHECK(dataset_property.dataType() == DataType::Table);
    return currentIndex(dataset_property).row();
}

int DataWidgets::currentRow(const PropertyID& dataset_property_id) const
{
    return currentRow(property(dataset_property_id));
}

int DataWidgets::currentRow() const
{
    return currentRow(structure()->singleDataset());
}

QModelIndexList DataWidgets::selectedIndexes(const DataProperty& dataset_property) const
{
    QModelIndexList selection = field<QAbstractItemView>(dataset_property)->selectionModel()->selectedRows();
    QModelIndexList res;

    if (selection.isEmpty()) {
        QModelIndex current = currentIndex(dataset_property);
        if (current.isValid())
            res << current.model()->index(current.row(), 0, current.parent());

    } else {
        for (auto& index : qAsConst(selection)) {
            res << Utils::alignIndexToModel(index, data()->dataset(dataset_property));
        }
    }
    return res;
}

QModelIndexList DataWidgets::selectedIndexes(const PropertyID& dataset_property_id) const
{
    return selectedIndexes(property(dataset_property_id));
}

QModelIndexList DataWidgets::selectedIndexes() const
{
    return selectedIndexes(structure()->singleDataset());
}

DataProperty DataWidgets::currentColumn(const DataProperty& dataset_property) const
{
    auto index = currentIndex(dataset_property);
    if (!index.isValid())
        return {};

    return data()->indexColumn(index);
}

DataProperty DataWidgets::currentColumn(const PropertyID& dataset_property_id) const
{
    return currentColumn(property(dataset_property_id));
}

DataProperty DataWidgets::currentColumn() const
{
    return currentColumn(structure()->singleDataset());
}

bool DataWidgets::setCurrentIndex(const DataProperty& dataset_property, const QModelIndex& index, bool current_column) const
{
    auto iv = field<QAbstractItemView>(dataset_property);
    QModelIndex idx = index.isValid() ? Utils::alignIndexToModel(index, iv->model()) : QModelIndex();
    if (!idx.isValid())
        return false;

    if (current_column) {
        auto column = currentColumn(dataset_property);
        if (column.isValid()) {
            idx = idx.model()->index(idx.row(), column.pos(), idx.parent());
        } else {
            idx = idx.model()->index(idx.row(), horizontalRootHeaderItem(dataset_property)->firstVisibleSection(), idx.parent());
        }
    }

    iv->setCurrentIndex(idx);

    if (idx.isValid())
        iv->scrollTo(idx);

    return iv->currentIndex() == idx;
}

bool DataWidgets::setCurrentIndex(const PropertyID& dataset_property_id, const QModelIndex& index, bool current_column) const
{
    return setCurrentIndex(property(dataset_property_id), index, current_column);
}

bool DataWidgets::setCurrentIndex(const QModelIndex& index, bool current_column) const
{
    return setCurrentIndex(structure()->singleDataset(), index, current_column);
}

I_DataWidgetsLookupModels* DataWidgets::dynamicLookup() const
{
    return _dynamic_lookup;
}

void DataWidgets::showAllDatasetHeaders(const DataProperty& dataset_property)
{
    HeaderItem* root = nullptr;
    if (dataset_property.dataType() == DataType::Table)
        root = field<TableView>(dataset_property)->horizontalRootHeaderItem();
    else if (dataset_property.dataType() == DataType::Tree)
        root = field<TreeView>(dataset_property)->rootHeaderItem();
    else
        Z_HALT_INT;

    root->beginUpdate();
    for (int i = root->bottomCount(); i < dataset_property.columnCount(); i++) {
        root->append(dataset_property.columns().at(i).name());
    }

    for (int i = 0; i < root->bottomCount(); i++) {
        root->bottomItem(i)->setVisible(true);
    }
    root->endUpdate();
}

void DataWidgets::showAllDatasetHeaders(const PropertyID& dataset_property_id)
{
    showAllDatasetHeaders(property(dataset_property_id));
}

std::shared_ptr<DataWidgets::WidgetInfo> DataWidgets::generateWidget(
    const DataProperty& property, bool show_frame, bool force_read_only, bool force_editable) const
{
    return generateWidget(
        _data->structure().get(), property, show_frame, force_read_only, force_editable, structure(), _view, _dynamic_lookup, this, _highlight);
}

std::shared_ptr<DataWidgets::WidgetInfo> DataWidgets::generateWidget(const DataStructure* data_structure, const DataProperty& property, bool show_frame,
    bool force_read_only, bool force_editable, const DataStructurePtr& structure, const View* view, const I_DataWidgetsLookupModels* dynamic_lookup,
    const DataWidgets* widgets, const HighlightProcessor* highlight)
{
    Z_CHECK_NULL(data_structure);
    Z_CHECK(property.isValid());
    if (force_read_only || force_editable)
        Z_CHECK(force_read_only != force_editable);

    QWidget* field = nullptr;
    QFrame* dataset_toolbar_widget = nullptr;
    QToolBar* dataset_toolbar = nullptr;
    QWidget* dataset_group = nullptr;
    QFrame* dataset_v_line = nullptr;
    QMap<ObjectActionType, QAction*> dataset_actions;

    ModelPtr lookup_model;
    DataFilterPtr lookup_filter;
    DataProperty lookup_column_display;
    DataProperty lookup_column_id;
    int lookup_role_display = -1;
    int lookup_role_id = -1;

    if (property.lookup() != nullptr) {
        auto lookup = property.lookup();
        if (lookup->type() == LookupType::Dataset) {
            ItemSelector* is = nullptr;
            if (!lookup->listEntity().isValid()) {
                // получаем через интерфейс I_DataWidgetsLookupModels
                Z_CHECK_NULL(dynamic_lookup);
                getLookupInfo(
                    dynamic_lookup, property, lookup_model, lookup_filter, lookup_column_display, lookup_column_id, lookup_role_display, lookup_role_id);

            } else {
                DataProperty lookup_dataset;
                getLookupInfo(DataStructure::structure(lookup->listEntity()).get(), property, lookup_dataset, lookup_column_display, lookup_column_id,
                    lookup_role_display, lookup_role_id);

                Error error;
                lookup_model = Core::getModel<Model>(lookup->listEntity(), {lookup_dataset}, error);
                Z_CHECK_ERROR(error);
                // создаем фильтр
                lookup_filter = Z_MAKE_SHARED(DataFilter, lookup_model.get());
                Z_CHECK_NULL(Core::catalogsInterface());

                // инициализируем каталоги
                if (Core::catalogsInterface()->isCatalogUid(lookup->listEntity())) {
                    Core::catalogsInterface()->initCatalogFilter(
                        Core::catalogsInterface()->catalogId(lookup->listEntity()), lookup->listEntity().database(), lookup_filter.get());
                }
            }
            is = new ItemSelector(lookup_model, lookup_column_display.dataset(), lookup_filter);

            auto links = property.links(PropertyLinkType::LookupFreeText);
            if (!links.isEmpty()) {
                Z_CHECK(links.count() == 1);
                is->setEditable(true);
            }

            is->setDisplayColumn(lookup_column_display.pos());
            is->setDisplayRole(lookup_role_display);
            is->setIdColumn(lookup_column_id.pos());
            is->setIdRole(lookup_role_id);

            is->setFrame(show_frame);
            if (!force_editable && property.options().testFlag(PropertyOption::ReadOnly))
                is->setReadOnly(true);

            if (property.options().testFlag(PropertyOption::SimpleDataset))
                is->setSelectionMode(SelectionMode::Multi);

            field = is;
        } else if (lookup->type() == LookupType::Request) {
            auto rs = new RequestSelector(property.lookup()->requestService());
            rs->setRequestType(property.lookup()->requestType());
            if (property.lookup()->requestServiceUid() == CoreUids::MODEL_SERVICE) {
                rs->setRequestOptions(property.lookup()->data());
                rs->setClearOnExit(true);
                rs->setComboboxMode(true);
            }

            field = rs;

        } else if (lookup->type() == LookupType::List) {
            ItemSelector* sel;
            field = sel = new ItemSelector;
            sel->setFrame(show_frame);

            FlatItemModel* model = new FlatItemModel(sel);
            model->setColumnCount(2);
            model->setRowCount(lookup->listValues().count());
            for (int i = 0; i < lookup->listValues().count(); i++) {
                model->setData(i, 0, lookup->listValues().at(i));
                model->setData(i, 1, lookup->listNames().at(i));
            }

            sel->setItemModel(model);
            sel->setIdColumn(0);
            sel->setDisplayColumn(1);

        } else
            Z_HALT_INT;

    } else if (force_read_only
               && (property.dataType() == DataType::Integer || property.dataType() == DataType::UInteger || property.dataType() == DataType::Float
                   || property.dataType() == DataType::UFloat || property.dataType() == DataType::Date || property.dataType() == DataType::DateTime
                   || property.dataType() == DataType::Time || property.dataType() == DataType::String || property.dataType() == DataType::Variant
                   || property.dataType() == DataType::Numeric || property.dataType() == DataType::UNumeric)) {
        field = new QLabel;

    } else if (property.dataType() == DataType::Integer || property.dataType() == DataType::UInteger) {
        LineEdit* w = new LineEdit;
        w->setFrame(show_frame);

        auto validator = Validators::createInt(INT_MIN, INT_MAX, w);
        if (property.dataType() == DataType::UInteger)
            validator->setBottom(0);

        w->setValidator(validator);

        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);

        field = w;

    } else if (property.dataType() == DataType::Float || property.dataType() == DataType::UFloat || property.dataType() == DataType::Numeric
               || property.dataType() == DataType::UNumeric) {
        LineEdit* w = new LineEdit;
        w->setFrame(show_frame);

        auto validator = Validators::createDouble(-DBL_MAX, DBL_MAX, Consts::DOUBLE_DECIMALS, w);
        if (property.dataType() == DataType::UFloat || property.dataType() == DataType::UNumeric)
            validator->setBottom(0);

        w->setValidator(validator);
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);

        field = w;

    } else if (property.dataType() == DataType::Date) {
        DateEdit* w = new DateEdit;
        w->setAlignment(Qt::AlignLeft);
        w->setFrameShape(show_frame ? QFrame::Shape::StyledPanel : QFrame::Shape::NoFrame);
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);

        field = w;

    } else if (property.dataType() == DataType::DateTime) {
        DateTimeEdit* w = new DateTimeEdit;
        w->setAlignment(Qt::AlignLeft);
        w->setNullable(true);
        w->setFrame(show_frame);
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);
        field = w;

    } else if (property.dataType() == DataType::Time) {
        TimeEdit* w = new TimeEdit;
        w->setAlignment(Qt::AlignLeft);
        w->setFrame(show_frame);
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);
        field = w;

    } else if (property.dataType() == DataType::Memo) {
        PlainTextEdit* w = new PlainTextEdit;
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);
        field = w;

    } else if (property.dataType() == DataType::RichMemo) {
        TextEdit* w = new TextEdit;
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);
        field = w;

    } else if (property.dataType() == DataType::Bool) {
        CheckBox* w = new CheckBox;
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setEnabled(false);
        field = w;

    } else if (property.dataType() == DataType::Image) {
        PictureSelector* ps = new PictureSelector;
        field = ps;
        if (!force_editable && (property.options() & PropertyOption::ReadOnly))
            ps->setReadOnly(true);

    } else if (property.dataType() == DataType::String || property.dataType() == DataType::Variant) {
        LineEdit* w = new LineEdit;
        w->setFrame(show_frame);
        if (!force_editable && property.options() & PropertyOption::ReadOnly)
            w->setReadOnly(true);

        field = w;

    } else if (property.dataType() == DataType::Uid) {
        EntityNameWidget* w = new EntityNameWidget;
        field = w;

    } else if (property.dataType() == DataType::Bytes) {
        QLabel* w = new QLabel;
        field = w;

    } else if (property.dataType() == DataType::Table || property.dataType() == DataType::Tree) {
        Z_CHECK_X(!property.options().testFlag(PropertyOption::SimpleDataset),
            QString("Dataset property %1:%2 has SimpleDataset option without lookup").arg(property.entityCode().value()).arg(property.id().value()));

        if (property.dataType() == DataType::Table) {
            TableView* w;
            if (view != nullptr)
                w = new TableView(view, property);
            else if (widgets != nullptr)
                w = new TableView(widgets, property, highlight);
            else
                w = new TableView(structure, property);

            field = w;

        } else if (property.dataType() == DataType::Tree) {
            TreeView* w;
            if (view != nullptr)
                w = new TreeView(view, property);
            else if (widgets != nullptr)
                w = new TreeView(widgets, property, highlight);
            else
                w = new TreeView(structure, property);

            field = w;

        } else
            Z_HALT_INT;

        // создаем составной виджет из тублара и набора данных
        dataset_toolbar_widget = new QFrame;
        dataset_toolbar_widget->setObjectName(QStringLiteral("zfdtw%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_toolbar_widget->setLayout(new QHBoxLayout);
        dataset_toolbar_widget->layout()->setObjectName(QStringLiteral("zfla"));
        dataset_toolbar_widget->layout()->setMargin(0);
        dataset_toolbar_widget->layout()->setSpacing(0);
        setDatasetToolbarWidgetStyle(false, true, true, true, dataset_toolbar_widget);

        dataset_toolbar = new QToolBar;
        dataset_toolbar->setObjectName(QStringLiteral("zfdt%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_toolbar->layout()->setMargin(1);
        dataset_toolbar->setAutoFillBackground(true);
        dataset_toolbar->setIconSize({UiSizes::defaultToolbarIconSize(ToolbarType::Internal), UiSizes::defaultToolbarIconSize(ToolbarType::Internal)});
        dataset_toolbar->setMovable(false);
        dataset_toolbar->setOrientation(Qt::Vertical);
        dataset_toolbar->setStyleSheet(QStringLiteral("QToolBar#%1 {border:none}").arg(dataset_toolbar->objectName()));
        dataset_toolbar->setFocusPolicy(Qt::NoFocus);

        QAction* edit_item_action = new QAction(Utils::toolButtonIcon(":/share_icons/blue/edit.svg"), ZF_TR(ZFT_EDIT_ROW), field);
        edit_item_action->setObjectName(QStringLiteral("zfea%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_actions[ObjectActionType::Modify] = edit_item_action;

        QAction* view_item_action = new QAction(Utils::toolButtonIcon(":/share_icons/blue/document.svg"), ZF_TR(ZFT_VIEW_ROW), field);
        view_item_action->setObjectName(QStringLiteral("zfva%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_actions[ObjectActionType::View] = view_item_action;

        QAction* add_item_action = new QAction(Utils::toolButtonIcon(":/share_icons/blue/circle-plus.svg"), ZF_TR(ZFT_ADD_ROW), field);
        add_item_action->setObjectName(QStringLiteral("zfaa%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_actions[ObjectActionType::Create] = add_item_action;

        QAction* add_item_level_down_action = new QAction(Utils::toolButtonIcon(":/share_icons/level.svg"), ZF_TR(ZFT_ADD_ROW_LEVEL), field);
        add_item_level_down_action->setObjectName(QStringLiteral("zfal%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_actions[ObjectActionType::CreateLevelDown] = add_item_level_down_action;
        if (property.dataType() != DataType::Tree)
            add_item_level_down_action->setVisible(false);

        QAction* remove_item_action = new QAction(Utils::toolButtonIcon(":/share_icons/blue/circle-minus.svg"), ZF_TR(ZFT_REMOVE_ROW), field);
        remove_item_action->setObjectName(QStringLiteral("zfra%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_actions[ObjectActionType::Remove] = remove_item_action;

        dataset_toolbar->addAction(edit_item_action);
        dataset_toolbar->addAction(view_item_action);
        dataset_toolbar->addAction(add_item_action);
        dataset_toolbar->addAction(add_item_level_down_action);
        dataset_toolbar->addAction(remove_item_action);

        dataset_toolbar->widgetForAction(edit_item_action)->setObjectName(edit_item_action->objectName() + "w");
        dataset_toolbar->widgetForAction(view_item_action)->setObjectName(view_item_action->objectName() + "w");
        dataset_toolbar->widgetForAction(add_item_action)->setObjectName(add_item_action->objectName() + "w");
        dataset_toolbar->widgetForAction(add_item_level_down_action)->setObjectName(add_item_level_down_action->objectName() + "w");
        dataset_toolbar->widgetForAction(remove_item_action)->setObjectName(remove_item_action->objectName() + "w");

        Utils::prepareToolbarButton(dataset_toolbar, edit_item_action);
        Utils::prepareToolbarButton(dataset_toolbar, view_item_action);
        Utils::prepareToolbarButton(dataset_toolbar, add_item_action);
        Utils::prepareToolbarButton(dataset_toolbar, add_item_level_down_action);
        Utils::prepareToolbarButton(dataset_toolbar, remove_item_action);

        dataset_v_line = new QFrame;
        dataset_v_line->setObjectName(QStringLiteral("dataset_v_line_%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_v_line->setMinimumWidth(2);
        dataset_v_line->setMaximumWidth(2);
        dataset_v_line->setStyleSheet(QString("border: 2px %1;"
                                              "border-top-style: none; "
                                              "border-right-style: ridge; "
                                              "border-bottom-style: none; "
                                              "border-left-style: none;"
                                              "margin-top: 1px;")
                                          .arg(Colors::uiLineColor(true).name()));

        dataset_toolbar_widget->layout()->addWidget(dataset_v_line);
        dataset_v_line->setHidden(true); // возможно линия вообще не нужна, пока отключаем
        dataset_toolbar_widget->layout()->addWidget(dataset_toolbar);

        dataset_group = new QWidget;
        dataset_group->setObjectName(QStringLiteral("zfdg%1_%2").arg(data_structure->id().value()).arg(property.id().value()));
        dataset_group->setFocusPolicy(Qt::NoFocus);
        dataset_group->setLayout(new QHBoxLayout);
        dataset_group->layout()->setObjectName(QStringLiteral("zfla"));
        dataset_group->layout()->setMargin(0);
        dataset_group->layout()->setSpacing(0);

        dataset_group->layout()->addWidget(field);
        dataset_group->layout()->addWidget(dataset_toolbar_widget);

    } else
        Z_HALT(QString::number(static_cast<int>(property.dataType())));

    Z_CHECK_NULL(field);

    field->setObjectName(QString("p_%1_%2").arg(data_structure->id().value()).arg(property.id().value()));

    auto w_info = Z_MAKE_SHARED(WidgetInfo);
    w_info->property = property;
    w_info->field = field;
    w_info->force_read_only = force_read_only;
    w_info->dataset_toolbar_widget = dataset_toolbar_widget;
    w_info->dataset_toolbar = dataset_toolbar;
    w_info->dataset_group = dataset_group;
    w_info->dataset_actions = dataset_actions;

    // данные по lookup
    w_info->lookup_model = lookup_model;
    w_info->lookup_filter = lookup_filter;
    w_info->lookup_column_display = lookup_column_display;
    w_info->lookup_column_id = lookup_column_id;
    w_info->lookup_role_display = lookup_role_display;
    w_info->lookup_role_id = lookup_role_id;

    return w_info;
}

QVariant DataWidgets::convertToLookupValue(const DataProperty& property, const QVariant& value)
{
    if (property.dataType() == DataType::Bool) {
        // специфический случай Bool вместе с lookup. Поскольку коды в лукапе не могут идти с нуля, то уменьшаем их на 1
        if (value.isValid()) {
            bool ok;
            int v = value.toInt(&ok);
            if (ok)
                return v + 1;
            else
                return QVariant();
        }
    }
    return value;
}

void DataWidgets::updateWidget(
    QWidget* widget, const DataProperty& main_property, const QMap<PropertyID, QVariant>& values, UpdateReason reason, int cursor_pos)
{
    Z_CHECK_NULL(widget);
    Z_CHECK(main_property.isValid());
    if (main_property.propertyType() == PropertyType::Dataset)
        return;
    Z_CHECK(values.contains(main_property.id()));

    QVariant value = values.value(main_property.id());

    QVariant converted;
    Utils::convertValue(value, main_property.dataType(), nullptr, ValueFormat::Edit, Core::locale(LocaleType::UserInterface), converted);

    if (main_property.lookup() != nullptr) {
        auto lookup = main_property.lookup();
        if (lookup->type() == LookupType::Dataset || lookup->type() == LookupType::List) {
            ItemSelector* w = qobject_cast<ItemSelector*>(widget);
            Z_CHECK_NULL(w);

            if (reason == UpdateReason::Loading) {
                w->setOverrideText(loagingTextHelper(widget));

            } else {
                w->setOverrideText("");

                auto value = convertToLookupValue(main_property, converted);

                if (!value.isValid() || value.isNull())
                    w->clear();
                else
                    w->setCurrentValue(value);

                auto links = main_property.links(PropertyLinkType::LookupFreeText);
                if (!links.isEmpty()) {
                    // значит у нас хранится как id, так и свободный текст
                    if (!w->currentIndex().isValid()) {
                        // позиционирование по id не удалось, значит используем свободный текст
                        Z_CHECK(values.contains(links.constFirst()->linkedPropertyId()));
                        w->setCurrentText(Utils::variantToString(values.value(links.constFirst()->linkedPropertyId())));
                    }
                }
            }

        } else if (lookup->type() == LookupType::Request) {
            RequestSelector* w = qobject_cast<RequestSelector*>(widget);
            Z_CHECK_NULL(w);

            if (reason == UpdateReason::Loading) {
                w->setOverrideText(loagingTextHelper(widget));

            } else {
                w->setOverrideText("");

                if (!converted.isValid() || converted.isNull() || (converted.type() == QVariant::String && converted.toString().isEmpty())) {
                    QString text_value;
                    auto links = main_property.links(PropertyLinkType::LookupFreeText);
                    if (!links.isEmpty() && !w->currentID().isValid() && main_property.lookup() != nullptr) {
                        // надо не просто очистить, а внести туда текст из поля, содержащего расшифровку
                        text_value = values.value(links.at(0)->linkedPropertyId()).toString();
                    }

                    w->clear(text_value);

                } else {
                    if (Uid::isUidVariant(converted))
                        w->setCurrentID(Uid::fromVariant(converted));
                    else
                        w->setCurrentID(Uid::general(converted));
                }

                w->updateEnabled();
            }

        } else
            Z_HALT_INT;

    } else if (main_property.dataType() == DataType::Date) {
        DateEdit* w = qobject_cast<DateEdit*>(widget);
        if (w != nullptr) {
            if (InvalidValue::isInvalidValueVariant(converted)) {
                auto invalid_value = InvalidValue::fromVariant(converted);
                w->setText(invalid_value.string());

            } else if ((w->isNull() && !w->isInvalid()) && !converted.isValid()) {
                // совпадает

            } else if (!converted.isValid()) {
                w->setDate(QDate());

            } else if (w->date() != converted.toDate()) {
                w->setDate(converted.toDate());
                if (cursor_pos >= 0)
                    w->setCursorPosition(cursor_pos);
            }

            w->setOverrideText(reason == UpdateReason::Loading ? loagingTextHelper(widget) : QString());

        } else {
            QLabel* lb = qobject_cast<QLabel*>(widget);
            Z_CHECK_NULL(lb);
            QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
            lb->setText(v);
        }

    } else if (main_property.dataType() == DataType::DateTime) {
        DateTimeEdit* w = qobject_cast<DateTimeEdit*>(widget);
        if (w != nullptr) {
            if ((w->dateTime().isNull() || !w->dateTime().isValid()) && !converted.isValid()) {
                // совпадает
            } else if (!converted.isValid()) {
                w->setDateTime(QDateTime());
            } else if (w->date() != converted.toDate()) {
                w->setDateTime(converted.toDateTime());
            }

            w->setOverrideText(reason == UpdateReason::Loading ? loagingTextHelper(widget) : QString());

        } else {
            QLabel* lb = qobject_cast<QLabel*>(widget);
            Z_CHECK_NULL(lb);
            QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
            lb->setText(v);
        }

    } else if (main_property.dataType() == DataType::Time) {
        TimeEdit* w = qobject_cast<TimeEdit*>(widget);
        if (w != nullptr) {
            if ((w->time().isNull() || !w->time().isValid()) && !converted.isValid()) {
                // совпадает
            } else if (!converted.isValid()) {
                w->setTime(QTime());
            } else if (w->time() != converted.toTime()) {
                w->setTime(converted.toTime());
            }

        } else {
            QLabel* lb = qobject_cast<QLabel*>(widget);
            Z_CHECK_NULL(lb);
            QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
            lb->setText(v);
        }

    } else if (main_property.dataType() == DataType::Memo) {
        PlainTextEdit* w = qobject_cast<PlainTextEdit*>(widget);
        Z_CHECK_NULL(w);
        QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
        if (w->toPlainText() != v)
            w->setPlainText(v);

    } else if (main_property.dataType() == DataType::RichMemo) {
        TextEdit* w = qobject_cast<TextEdit*>(widget);
        Z_CHECK_NULL(w);
        QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
        if (w->toPlainText() != v) {
            if (HtmlTools::isHtml(v))
                w->setText(v);
            else
                w->setPlainText(v);
        }

    } else if (main_property.dataType() == DataType::Bool) {
        QCheckBox* w = qobject_cast<QCheckBox*>(widget);
        Z_CHECK_NULL(w);
        if (w->isChecked() != converted.toBool())
            w->setChecked(converted.toBool());

    } else if (main_property.dataType() == DataType::Bytes) {
        // пока ничего не делаем

    } else if (main_property.dataType() == DataType::String || main_property.dataType() == DataType::Integer || main_property.dataType() == DataType::UInteger
               || main_property.dataType() == DataType::Float || main_property.dataType() == DataType::UFloat || main_property.dataType() == DataType::Numeric
               || main_property.dataType() == DataType::UNumeric || main_property.dataType() == DataType::Variant) {
        QLineEdit* w = qobject_cast<QLineEdit*>(widget);
        if (w != nullptr) {
            QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
            if (w->text() != v) {
                if (v == "0" && main_property.dataType() != DataType::String) {
                    w->setText("");

                } else {
                    w->setText(v);
                    if (cursor_pos >= 0)
                        w->setCursorPosition(cursor_pos);
                }
            }
        } else {
            QLabel* lb = qobject_cast<QLabel*>(widget);
            Z_CHECK_NULL(lb);
            QString v = (reason == UpdateReason::Loading) ? loagingTextHelper(widget) : Utils::variantToString(converted);
            lb->setText(v);
        }

    } else if (main_property.dataType() == DataType::Uid) {
        EntityNameWidget* w = qobject_cast<EntityNameWidget*>(widget);
        Z_CHECK_NULL(w);

        if (reason == UpdateReason::Loading)
            w->setOverrideText(loagingTextHelper(widget));
        else
            w->setOverrideText("");

        w->setEntityUid(Uid::fromVariant(converted));

    } else if (main_property.dataType() == DataType::Image) {
        PictureSelector* w = qobject_cast<PictureSelector*>(widget);
        Z_CHECK_NULL(w);
        Error error;
        if (converted.type() == QVariant::String)
            w->setIcon(Utils::variantFromSerialized(converted.toString(), error).value<QIcon>());
        else if (converted.type() == QVariant::ByteArray)
            w->setFileData(converted.toByteArray());
        else if (converted.type() == QVariant::Icon)
            w->setIcon(qvariant_cast<QIcon>(converted));
        else if (converted.type() == QVariant::Pixmap)
            w->setIcon(QIcon(qvariant_cast<QPixmap>(converted)));
        else
            w->clear();
    } else
        Z_HALT_INT;
}

void DataWidgets::updateWidget(QWidget* widget, const DataProperty& property, const QVariantList& values, UpdateReason reason)
{
    Q_UNUSED(reason)

    Z_CHECK_NULL(widget);
    Z_CHECK(property.isValid());
    Z_CHECK(property.propertyType() == PropertyType::ColumnFull);
    Z_CHECK(property.dataset().options().testFlag(PropertyOption::SimpleDataset));
    Z_CHECK_NULL(property.dataset().lookup());

    ItemSelector* w = qobject_cast<ItemSelector*>(widget);
    Z_CHECK_NULL(w);
    Z_CHECK(w->selectionMode() == SelectionMode::Multi);

    QVariantList prepared;
    for (auto& v : values) {
        QVariant converted;
        Utils::convertValue(v, property.dataType(), nullptr, ValueFormat::Internal, Core::locale(LocaleType::Universal), converted);
        prepared << converted;
    }

    w->setCurrentValues(prepared);
}

QVariant DataWidgets::convertFromLookupValue(const DataProperty& property, const QVariant& value)
{
    Z_CHECK(property.isValid());
    if (property.dataType() == DataType::Bool) {
        // специфический случай Bool вместе с lookup. Поскольку коды в лукапе не могут идти с нуля, то уменьшаем их на 1
        bool ok;
        int v = value.toInt(&ok);
        if (ok && v >= 1 && v <= 2)
            return bool(v - 1);
        else
            return QVariant();
    } else {
        return value;
    }
}

QMap<PropertyID, QVariant> DataWidgets::extractWidgetValues(const DataProperty& p, QWidget* widget, const I_DataWidgetsLookupModels* dynamic_lookup)
{
    Q_UNUSED(dynamic_lookup)
    Z_CHECK(p.isValid());

    QMap<PropertyID, QVariant> res;

    if (ItemSelector* w = qobject_cast<ItemSelector*>(widget)) {
        Z_CHECK_NULL(p.lookup());
        auto links = p.links(PropertyLinkType::LookupFreeText);
        if (!links.isEmpty()) {
            Z_CHECK_X(w->isEditable(), QString("ItemSelector with links must be editable %1").arg(p.toPrintable()));
            Z_CHECK(links.count() == 1);
            Z_CHECK(links.constFirst()->isValid());
        } else {
            Z_CHECK_X(!w->isEditable(), QString("ItemSelector without links must be not editable %1").arg(p.toPrintable()));
        }

        if (w->isEditable()) {
            if (w->currentIndex().isValid()) {
                res[p.id()] = w->currentValue();
                res[links.constFirst()->linkedPropertyId()] = QVariant();
            } else {
                res[p.id()] = QVariant();
                res[links.constFirst()->linkedPropertyId()] = w->currentText();
            }

        } else {
            Z_CHECK(w->selectionMode() == SelectionMode::Single);
            if (w->currentIndex().isValid()) {
                res[p.id()] = convertFromLookupValue(p, w->currentValue());
            }
        }

    } else if (RequestSelector* w = qobject_cast<RequestSelector*>(widget)) {
        Z_CHECK_NULL(p.lookup());
        res[p.id()] = w->currentID().variant();
        // linkedPropertyId будет обновлен в updateRequestSelectorFreeTextProperty

    } else if (LineEdit* w = qobject_cast<LineEdit*>(widget)) {
        QVariant value;
        if (!w->inputMask().isEmpty())
            value = Utils::extractMaskValue(w->displayText(), w->inputMask());
        else
            value = w->text();

        res[p.id()] = value;

    } else if (QLineEdit* w = qobject_cast<QLineEdit*>(widget)) {
        QVariant value;
        if (!w->inputMask().isEmpty())
            value = Utils::extractMaskValue(w->displayText(), w->inputMask());
        else
            value = w->text();

        res[p.id()] = value;

    } else if (DateEdit* w = qobject_cast<DateEdit*>(widget)) {
        if (!w->isNull() && w->date().isValid() && !w->date().isNull() && !w->isInvalid())
            res[p.id()] = w->date();
        else if (w->isInvalid())
            res[p.id()] = InvalidValue(w->text()).variant();

    } else if (QDateEdit* w = qobject_cast<QDateEdit*>(widget)) {
        if (w->date().isValid() && !w->date().isNull())
            res[p.id()] = w->date();

    } else if (DateTimeEdit* w = qobject_cast<DateTimeEdit*>(widget)) {
        if (!w->isNull() && w->dateTime().isValid() && !w->dateTime().isNull())
            res[p.id()] = w->dateTime();

    } else if (QDateTimeEdit* w = qobject_cast<QDateTimeEdit*>(widget)) {
        if (!w->dateTime().isNull() && w->dateTime().isValid())
            res[p.id()] = w->dateTime();

    } else if (CheckBox* w = qobject_cast<CheckBox*>(widget)) {
        res[p.id()] = (w->checkState() == Qt::Checked) ? true : false;

    } else if (QCheckBox* w = qobject_cast<QCheckBox*>(widget)) {
        res[p.id()] = (w->checkState() == Qt::Checked) ? true : false;

    } else if (PlainTextEdit* w = qobject_cast<PlainTextEdit*>(widget)) {
        res[p.id()] = w->toPlainText();

    } else if (QPlainTextEdit* w = qobject_cast<QPlainTextEdit*>(widget)) {
        res[p.id()] = w->toPlainText();

    } else if (TextEdit* w = qobject_cast<TextEdit*>(widget)) {
        res[p.id()] = w->toPlainText();

    } else if (QTextEdit* w = qobject_cast<QTextEdit*>(widget)) {
        res[p.id()] = w->toPlainText();

    } else if (PictureSelector* w = qobject_cast<PictureSelector*>(widget)) {
        if (w->fileData().isEmpty())
            res[p.id()] = w->icon().isNull() ? QVariant() : w->icon();
        else
            res[p.id()] = w->fileData();

    } else if (EntityNameWidget* w = qobject_cast<EntityNameWidget*>(widget)) {
        if (w->entityUid().isValid())
            res[p.id()] = w->entityUid().variant();

    } else if (QLabel* w = qobject_cast<QLabel*>(widget)) {
        Q_UNUSED(w);

    } else
        Z_HALT(widget->metaObject()->className());

    if (res.isEmpty())
        res[p.id()] = QVariant();

    return res;
}

bool DataWidgets::eventFilter(QObject* obj, QEvent* event)
{
    bool res = QObject::eventFilter(obj, event);

    if (obj->isWidgetType()) {
        auto w = qobject_cast<QWidget*>(obj);
        auto prop = widgetProperty(w);
        if (prop.isValid() && (event->type() == QEvent::Show || event->type() == QEvent::Hide))
            emit sg_fieldVisibleChanged(prop, w);
    }

    return res;
}

void DataWidgets::sl_afterModelChange(const ModelPtr& old_model, const ModelPtr& new_model)
{
    Q_UNUSED(new_model);

    // надо просмотреть все динамические лукапы и если они были назначены на старую модель - обновить на новую
    if (old_model == nullptr)
        return;

    auto lookup_info = _lookup_info_by_model.value(old_model.get());
    if (lookup_info == nullptr)
        return;

    for (auto& p : qAsConst(lookup_info->properties)) {
        if (p.lookup() == nullptr || p.lookup()->listEntity().isValid())
            continue; // это не динамический лукап

        auto w_info = _properties_info.value(p);
        if (w_info == nullptr || w_info->field == nullptr)
            continue;

        ModelPtr lookup_model;
        DataFilterPtr lookup_filter;
        DataProperty lookup_column_display;
        DataProperty lookup_column_id;
        int lookup_role_display = -1;
        int lookup_role_id = -1;
        DataProperty lookup_dataset;

        getLookupInfo(_dynamic_lookup, p, lookup_model, lookup_filter, lookup_column_display, lookup_column_id, lookup_role_display, lookup_role_id);
        Z_CHECK_NULL(lookup_model);

        auto item_selector = qobject_cast<ItemSelector*>(w_info->field);
        Z_CHECK_NULL(item_selector);

        item_selector->setModel(lookup_model, lookup_column_display.dataset(), lookup_filter);

        // обновляем информацию о лукап моделях
        addLookupInfo(p, lookup_model);
    }

    // удаляем старую информацию
    removeLookupInfo(old_model);

    // обновляем информацию о лукапе в виджетах
    for (auto i = _properties_info.cbegin(); i != _properties_info.cend(); ++i) {
        if (i.value()->lookup_model == old_model) {
            Z_CHECK(i.value()->lookup_filter == nullptr);
            i.value()->lookup_model = new_model;
        }
    }
}

void DataWidgets::sl_propertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    Q_UNUSED(old_values);

    updateRequestSelectorWidget(p, UpdateReason::NoReason);

    if (isBlockUpdateProperty(p))
        return;

    if (Core::mode().testFlag(CoreMode::Test)) {
        updateWidgetHelper(p, UpdateReason::NoReason);

    } else {
        if (!isUpdating())
            requestUpdateWidget(p, UpdateReason::NoReason);
    }
}

void DataWidgets::sl_dataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    if (isBlockUpdateProperty(p))
        return;

    if (Core::mode().testFlag(CoreMode::Test)) {
        updateWidgetHelper(p, UpdateReason::NoReason);
    } else {
        if (!isUpdating())
            requestUpdateWidget(p, UpdateReason::NoReason);
    }
}

void DataWidgets::sl_dataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    if (isBlockUpdateProperty(p))
        return;

    if (Core::mode().testFlag(CoreMode::Test)) {
        updateWidgetHelper(p, UpdateReason::NoReason);
    } else {
        if (!isUpdating())
            requestUpdateWidget(p, UpdateReason::NoReason);
    }
}

void DataWidgets::sl_dataset_rowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    if (isBlockUpdateProperty(p))
        return;

    if (Core::mode().testFlag(CoreMode::Test)) {
        updateWidgetHelper(p, UpdateReason::NoReason);
    } else {
        if (!isUpdating())
            requestUpdateWidget(p, UpdateReason::NoReason);
    }
}

void DataWidgets::sl_dataset_modelReset(const DataProperty& p)
{
    if (isBlockUpdateProperty(p))
        return;

    if (Core::mode().testFlag(CoreMode::Test)) {
        updateWidgetHelper(p, UpdateReason::NoReason);
    } else {
        if (!isUpdating())
            requestUpdateWidget(p, UpdateReason::NoReason);
    }
}

void DataWidgets::sl_datasetActionTriggered()
{
    QAction* a = qobject_cast<QAction*>(sender());
    Z_CHECK_NULL(a);
    auto data = a->data().value<QPair<DataProperty, ObjectActionType>>();
    Z_CHECK(data.first.isValid() && data.first.propertyType() == PropertyType::Dataset);

    emit sg_datasetActionTriggered(data.first, data.second);
}

void DataWidgets::sl_invalidateChanged(const DataProperty& p, bool invalidate)
{
    Q_UNUSED(invalidate)
    if (!isUpdating())
        requestUpdateWidget(p, UpdateReason::NoReason);
}

void DataWidgets::sl_propertyInitialized(const DataProperty& p)
{
    if (!isUpdating())
        requestUpdateWidget(p, UpdateReason::NoReason);
}

void DataWidgets::sl_propertyUnInitialized(const DataProperty& p)
{
    if (!isUpdating())
        requestUpdateWidget(p, UpdateReason::NoReason);
}

void DataWidgets::sl_allPropertiesBlocked()
{
    if (!isUpdating())
        requestUpdateWidgets(UpdateReason::NoReason);
}

void DataWidgets::sl_allPropertiesUnBlocked()
{
    if (!isUpdating())
        requestUpdateWidgets(UpdateReason::NoReason);
}

void DataWidgets::sl_propertyBlocked(const DataProperty& p)
{
    if (!isUpdating())
        requestUpdateWidget(p, UpdateReason::NoReason);
}

void DataWidgets::sl_propertyUnBlocked(const DataProperty& p)
{
    if (!isUpdating())
        requestUpdateWidget(p, UpdateReason::NoReason);
}

void DataWidgets::sl_ItemSelector_edited(const QModelIndex& index, bool by_keys)
{
    Q_UNUSED(index);
    Q_UNUSED(by_keys);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_ItemSelector_edited(const QString& text)
{
    Q_UNUSED(text);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_ItemSelector_editedMulti(const QVariantList& values)
{
    if (isUpdating())
        return;

    ItemSelector* w = qobject_cast<ItemSelector*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isDataset());
    setNewValueSimpleDataset(p, values);
}

void DataWidgets::sl_ItemSelectorList_edited(const QModelIndex& index, bool by_keys)
{
    Q_UNUSED(index);
    Q_UNUSED(by_keys);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w), {p.id()});
}

void DataWidgets::sl_RequestSelector_edited(const Uid& key)
{
    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());

    // если заданы DataSourcePriority, то обновление данных будет на основе них в DataContainer
    if (p.links(PropertyLinkType::DataSourcePriority).isEmpty())
        setNewValue(p, extractWidgetValues(p, w), {p.id()});

    updateRequestSelectorFreeTextProperty(p);

    if (!key.isValid()) {
        // надо очистить значения атрибутов
        data()->blockDataSourcePriority();
        for (auto a : p.lookup()->attributesToClear()) {
            auto a_p = p.lookup()->attributes().value(a);
            Z_CHECK(a_p.isValid());
            setNewValue(property(a_p), {{a_p, QVariant()}}, {p.id()});
        }
        data()->unBlockDataSourcePriority();
        data()->fillDataSourcePriority(p);
    }
}

void DataWidgets::sl_RequestSelector_textEdited(const QString text)
{
    Q_UNUSED(text);

    if (isUpdating())
        return;

    RequestSelector* w = qobject_cast<RequestSelector*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    Z_CHECK_NULL(p.lookup());

    // нам надо обновить только поле, хранящее текстовую расшифровку
    updateRequestSelectorFreeTextProperty(p);
}

void DataWidgets::sl_RequestSelector_attributesChanged(
    bool started_by_user, const QMap<QString, QVariant>& old_attributes, const QMap<QString, QVariant>& new_attributes)
{
    Q_UNUSED(old_attributes);
    Q_UNUSED(new_attributes);

    if (isUpdating() || !started_by_user)
        return;

    RequestSelector* w = qobject_cast<RequestSelector*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());

    // надо обновить значения на основе атирибутов
    data()->blockDataSourcePriority();
    for (auto i = p.lookup()->attributes().cbegin(); i != p.lookup()->attributes().cend(); ++i) {
        setNewValue(property(i.value()), {{i.value(), w->attributes().value(i.key())}}, {p.id()});
    }
    data()->unBlockDataSourcePriority();
    data()->fillDataSourcePriority(p);
}

void DataWidgets::sl_RequestSelector_searchStarted(bool started_by_user)
{
    Q_UNUSED(started_by_user);
}

void DataWidgets::sl_RequestSelector_searchFinished(bool started_by_user, bool cancelled)
{
    Q_UNUSED(started_by_user);
    Q_UNUSED(cancelled);
}

void DataWidgets::sl_LineEdit_textEdited(const QString& text)
{
    Q_UNUSED(text);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_DateEdit_dateChanged(const QDate& old_date, const QDate& new_date)
{
    Q_UNUSED(old_date);
    Q_UNUSED(new_date);

    onDateChanged(qobject_cast<QWidget*>(sender()));
}

void DataWidgets::sl_DateEdit_textChanged(const QString& old_text, const QString& new_text)
{
    Q_UNUSED(old_text);
    Q_UNUSED(new_text);

    onDateChanged(qobject_cast<QWidget*>(sender()));
}

void DataWidgets::sl_DateTimeEdit_dateTimeChanged(const QDateTime& datetime)
{
    Q_UNUSED(datetime);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_TimeEdit_dateTimeChanged(const QTime& time)
{
    Q_UNUSED(time);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_CheckBox_stateChanged(int state)
{
    Q_UNUSED(state);

    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_PlainTextEdit_textChanged()
{
    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_RichTextEdit_textChanged()
{
    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_PictureSelectorEdited()
{
    if (isUpdating())
        return;

    QWidget* w = qobject_cast<QWidget*>(sender());
    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::sl_model_finishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error)
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
}

void DataWidgets::sl_currentCellChanged(const QModelIndex& current, const QModelIndex& previous)
{
    auto source_current = Utils::getTopSourceIndex(current);
    auto source_previous = Utils::getTopSourceIndex(previous);

    DataProperty cell_property_current;
    if (source_current.isValid())
        cell_property_current = data()->cellProperty(source_current, false);

    DataProperty cell_property_previous;
    if (source_previous.isValid())
        cell_property_previous = data()->cellProperty(source_previous, false);

    if (_focus_cell_previous != cell_property_previous || _focus_cell_current != cell_property_current) {
        _focus_cell_previous = cell_property_previous;
        _focus_cell_current = cell_property_current;

        QWidget* prev_dataset_widget = cell_property_previous.isValid() ? field(cell_property_previous.dataset()) : nullptr;
        QWidget* current_dataset_widget = cell_property_current.isValid() ? field(cell_property_current.dataset()) : nullptr;

        emit sg_focusChanged(prev_dataset_widget, cell_property_previous, current_dataset_widget, cell_property_current);
    }
}

void DataWidgets::sl_focusChanged(QWidget* prev, QWidget* current)
{
    auto prop_prev = widgetProperty(prev);
    auto prop_current = widgetProperty(current);

    if (_focus_previous != prop_prev || _focus_current != prop_current) {
        _focus_previous = prop_prev;
        _focus_current = prop_current;

        if (!_focus_current.isValid())
            _focus_cell_current.clear();

        emit sg_focusChanged(prev, prop_prev, current, prop_current);
    }

    QModelIndex idx_prev;
    if (prop_prev.propertyType() == PropertyType::Dataset && !prop_prev.options().testFlag(PropertyOption::SimpleDataset))
        idx_prev = currentIndex(prop_prev);

    QModelIndex idx_current;
    if (prop_current.propertyType() == PropertyType::Dataset && !prop_current.options().testFlag(PropertyOption::SimpleDataset)) {
        idx_current = currentIndex(prop_current);
        sl_currentCellChanged(idx_current, idx_prev);
    }
}

void DataWidgets::sl_lookupModelStartLoad()
{
    Model* m = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(m);
    auto info = _lookup_info_by_model.value(m);
    Z_CHECK_NULL(info);
    info->loading_counter++;
    _lookup_loading_counter++;

    //    qDebug() << "sl_lookupModelStartLoad" << m->entityUid() << info->loading_counter << _lookup_loading_counter;
    if (_lookup_loading_counter == 1) {
        emit sg_beginLookupLoad();
        //        qDebug() << "sg_beginLookupLoad";
    }
}

void DataWidgets::sl_lookupModelFinishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    if (error.isError())
        Core::logError(error);

    auto m = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(m);
    auto info = _lookup_info_by_model.value(m);
    Z_CHECK_NULL(info);

    Z_CHECK_NULL(info);
    info->loading_counter--;
    Z_CHECK(info->loading_counter >= 0);
    _lookup_loading_counter--;
    Z_CHECK(_lookup_loading_counter >= 0);

    //    qDebug() << "sl_lookupModelFinishLoad" << m->entityUid() << info->loading_counter << _lookup_loading_counter;
    if (_lookup_loading_counter == 0) {
        // обновляем связанные таблицы, чтобы обновилось отображение в ItemDelegate
        DataPropertySet datasets;
        for (auto& p : qAsConst(info->properties)) {
            if (!p.isColumn())
                continue;
            datasets << p.dataset();
        }
        for (auto& p : qAsConst(datasets)) {
            auto ds_view = field<QAbstractItemView>(p);
            ds_view->update();

            if (auto v = qobject_cast<TableView*>(ds_view)) {
                if (v->isAutoResizeRowsHeight())
                    v->requestResizeRowsToContents();
            }
        }

        emit sg_endLookupLoad();
        //        qDebug() << "sg_endLookupLoad";
    }
}

void DataWidgets::sl_beforeResizeRowsToContent()
{
    auto tv = qobject_cast<TableView*>(sender());
    Z_CHECK_NULL(tv);

    QModelIndex index = tv->currentIndex();
    if (!index.isValid())
        return;

    QRect index_rect = tv->visualRect(index);
    _need_resize_scroll_to = tv->rect().intersects(index_rect) || tv->rect().contains(index_rect);
}

void DataWidgets::sl_afterResizeRowsToContent()
{
    if (!_need_resize_scroll_to)
        return;

    auto tv = qobject_cast<TableView*>(sender());
    Z_CHECK_NULL(tv);

    QTimer::singleShot(500, tv, [tv]() {
        if (tv->currentIndex().isValid())
            tv->scrollTo(tv->currentIndex());
    });
}

void DataWidgets::sl_lineEditCursorPositionChanged(int oldPos, int newPos)
{
    Q_UNUSED(oldPos);

    auto e = qobject_cast<QLineEdit*>(sender());
    propertyInfo(widgetProperty(e), true)->cursor_pos = newPos;
}

void DataWidgets::sl_dateEditCursorPositionChanged(int oldPos, int newPos)
{
    Q_UNUSED(oldPos);

    auto e = qobject_cast<DateEdit*>(sender());
    propertyInfo(widgetProperty(e), true)->cursor_pos = newPos;
}

void DataWidgets::bootstrap()
{
    _object_extension = new ObjectExtension(this);

    connect(qApp, &QApplication::focusChanged, this, &DataWidgets::sl_focusChanged);
    _callback_manager->registerObject(this, "sl_callback");
}

void DataWidgets::setDataSource(const DataContainerPtr& data)
{
    bool same_data = (_data == data);

    if (_data != nullptr) {
        if (!same_data) {
            disconnectFromItemModels();

            disconnect(_data.get(), &DataContainer::sg_languageChanged, this, &DataWidgets::sl_languageChanged);
            disconnect(_data.get(), &DataContainer::sg_propertyChanged, this, &DataWidgets::sl_propertyChanged);
            disconnect(_data.get(), &DataContainer::sg_invalidateChanged, this, &DataWidgets::sl_invalidateChanged);
            disconnect(_data.get(), &DataContainer::sg_propertyInitialized, this, &DataWidgets::sl_propertyInitialized);
            disconnect(_data.get(), &DataContainer::sg_propertyUnInitialized, this, &DataWidgets::sl_propertyUnInitialized);
            disconnect(_data.get(), &DataContainer::sg_allPropertiesBlocked, this, &DataWidgets::sl_allPropertiesBlocked);
            disconnect(_data.get(), &DataContainer::sg_allPropertiesUnBlocked, this, &DataWidgets::sl_allPropertiesUnBlocked);
            disconnect(_data.get(), &DataContainer::sg_propertyBlocked, this, &DataWidgets::sl_propertyBlocked);
            disconnect(_data.get(), &DataContainer::sg_propertyUnBlocked, this, &DataWidgets::sl_propertyUnBlocked);

            disconnect(_data.get(), &DataContainer::sg_dataset_dataChanged, this, &DataWidgets::sl_dataset_dataChanged);
            disconnect(_data.get(), &DataContainer::sg_dataset_rowsInserted, this, &DataWidgets::sl_dataset_rowsInserted);
            disconnect(_data.get(), &DataContainer::sg_dataset_rowsRemoved, this, &DataWidgets::sl_dataset_rowsRemoved);
            disconnect(_data.get(), &DataContainer::sg_dataset_modelReset, this, &DataWidgets::sl_dataset_modelReset);
        }
    }

    Z_CHECK_NULL(data);
    _data = data;

    if (!same_data && !_properties_info.isEmpty())
        clearWidgets();

    if (!same_data) {
        connect(_data.get(), &DataContainer::sg_languageChanged, this, &DataWidgets::sl_languageChanged);
        connect(_data.get(), &DataContainer::sg_propertyChanged, this, &DataWidgets::sl_propertyChanged);
        connect(_data.get(), &DataContainer::sg_invalidateChanged, this, &DataWidgets::sl_invalidateChanged);
        connect(_data.get(), &DataContainer::sg_propertyInitialized, this, &DataWidgets::sl_propertyInitialized);
        connect(_data.get(), &DataContainer::sg_propertyUnInitialized, this, &DataWidgets::sl_propertyUnInitialized);
        connect(_data.get(), &DataContainer::sg_allPropertiesBlocked, this, &DataWidgets::sl_allPropertiesBlocked);
        connect(_data.get(), &DataContainer::sg_allPropertiesUnBlocked, this, &DataWidgets::sl_allPropertiesUnBlocked);
        connect(_data.get(), &DataContainer::sg_propertyBlocked, this, &DataWidgets::sl_propertyBlocked);
        connect(_data.get(), &DataContainer::sg_propertyUnBlocked, this, &DataWidgets::sl_propertyUnBlocked);

        connect(_data.get(), &DataContainer::sg_dataset_dataChanged, this, &DataWidgets::sl_dataset_dataChanged);
        connect(_data.get(), &DataContainer::sg_dataset_rowsInserted, this, &DataWidgets::sl_dataset_rowsInserted);
        connect(_data.get(), &DataContainer::sg_dataset_rowsRemoved, this, &DataWidgets::sl_dataset_rowsRemoved);
        connect(_data.get(), &DataContainer::sg_dataset_modelReset, this, &DataWidgets::sl_dataset_modelReset);

        if (_view != nullptr) {
            connect(_view, &View::sg_uiUnBlocked, this, [&]() {});
        }

        connectToItemModels();
    }

    requestUpdateWidgets(UpdateReason::NoReason);
}

QWidget* DataWidgets::createContentWidget() const
{
    QWidget* w = new QWidget;
    w->setObjectName(QStringLiteral("zfc").arg(structure()->entityCode().value()));
    QVBoxLayout* la = new QVBoxLayout;
    la->setObjectName(QStringLiteral("zfla"));
    w->setLayout(la);
    la->setMargin(0);

    if (structure()->properties().isEmpty())
        return w;

    for (auto& p : structure()->properties()) {
        QWidget* w_p = complexField(p);
        la->addWidget(w_p);
    }

    QScrollArea* scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("zfcs%1").arg(structure()->entityCode().value()));
    scroll->setWidget(w);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    return scroll;
}

void DataWidgets::generateWidgetsLazy() const
{
    if (!_properties_info.isEmpty() || _lazy_generation)
        return;

    _lazy_generation = true;
    const_cast<DataWidgets*>(this)->generateWidgets();
    const_cast<DataWidgets*>(this)->connectToItemModels();
    _lazy_generation = false;
}

QWidget* DataWidgets::generateWidgetHelper(const DataProperty& property)
{
    blockUpdateProperty(property);

    bool force_read_only = property.options() & PropertyOption::ForceReadOnly;
    auto w_info = generateWidget(property, true, force_read_only, false);

    if (property.lookup() != nullptr && property.lookup()->type() == LookupType::Dataset) {
        ItemSelector* item_selector = qobject_cast<ItemSelector*>(w_info->field.data());
        Z_CHECK_NULL(item_selector);

        connect(item_selector, qOverload<const QModelIndex&, bool>(&ItemSelector::sg_edited), this,
            qOverload<const QModelIndex&, bool>(&DataWidgets::sl_ItemSelector_edited));
        connect(item_selector, qOverload<const QString&>(&ItemSelector::sg_edited), this, qOverload<const QString&>(&DataWidgets::sl_ItemSelector_edited));
        connect(item_selector, &ItemSelector::sg_editedMulti, this, &DataWidgets::sl_ItemSelector_editedMulti);

        // контроль загрузки lookup
        addLookupInfo(property, item_selector->model());

    } else if (property.lookup() != nullptr && property.lookup()->type() == LookupType::List) {
        auto sel = qobject_cast<ItemSelector*>(w_info->field.data());
        Z_CHECK_NULL(sel);
        connect(sel, qOverload<const QModelIndex&, bool>(&ItemSelector::sg_edited), this,
            qOverload<const QModelIndex&, bool>(&DataWidgets::sl_ItemSelectorList_edited));

    } else if (property.lookup() != nullptr && property.lookup()->type() == LookupType::Request) {
        RequestSelector* request_selector = qobject_cast<RequestSelector*>(w_info->field.data());
        Z_CHECK_NULL(request_selector);

        connect(request_selector, &RequestSelector::sg_edited, this, &DataWidgets::sl_RequestSelector_edited);
        connect(request_selector, &RequestSelector::sg_textEdited, this, &DataWidgets::sl_RequestSelector_textEdited);
        connect(request_selector, &RequestSelector::sg_attributesChanged, this, &DataWidgets::sl_RequestSelector_attributesChanged);
        connect(request_selector, &RequestSelector::sg_searchStarted, this, &DataWidgets::sl_RequestSelector_searchStarted);
        connect(request_selector, &RequestSelector::sg_searchFinished, this, &DataWidgets::sl_RequestSelector_searchFinished);

        // надо отслеживать изменения родителя
        for (auto& p : property.lookup()->parentKeyProperties()) {
            _request_selector_parent.insertMulti(structure()->property(p), property);
        }

    } else if (property.dataType() == DataType::Memo) {
        PlainTextEdit* plain_edit = qobject_cast<PlainTextEdit*>(w_info->field.data());
        Z_CHECK_NULL(plain_edit);
        connect(plain_edit, &QPlainTextEdit::textChanged, this, &DataWidgets::sl_PlainTextEdit_textChanged);

    } else if (property.dataType() == DataType::RichMemo) {
        TextEdit* rich_edit = qobject_cast<TextEdit*>(w_info->field.data());
        Z_CHECK_NULL(rich_edit);
        connect(rich_edit, &TextEdit::textChanged, this, &DataWidgets::sl_RichTextEdit_textChanged);

    } else if (property.dataType() == DataType::Bool) {
        QCheckBox* check_box = qobject_cast<QCheckBox*>(w_info->field.data());
        Z_CHECK_NULL(check_box);
        connect(check_box, &QCheckBox::stateChanged, this, &DataWidgets::sl_CheckBox_stateChanged);

    } else if (property.dataType() == DataType::Image) {
        PictureSelector* ps = qobject_cast<PictureSelector*>(w_info->field.data());
        Z_CHECK_NULL(ps);
        connect(ps, &PictureSelector::sg_edited, this, &DataWidgets::sl_PictureSelectorEdited);

    } else if (property.dataType() == DataType::Uid) {
        // не на что подписываться

    } else if (property.dataType() == DataType::Bytes) {
        // игнорируем

    } else if (property.dataType() == DataType::Table || property.dataType() == DataType::Tree) {
        // подключение к экшенам
        for (auto it = w_info->dataset_actions.constBegin(); it != w_info->dataset_actions.constEnd(); ++it) {
            QPair<DataProperty, ObjectActionType> action_info(property, it.key());
            it.value()->setData(QVariant::fromValue(action_info));
            connect(it.value(), &QAction::triggered, this, &DataWidgets::sl_datasetActionTriggered);
        }
        // подключение к лукап-моделям
        Z_CHECK(property.propertyType() == PropertyType::Dataset);
        for (auto& col : property.columns()) {
            if (col.lookup() == nullptr || col.lookup()->type() != LookupType::Dataset)
                continue;

            // контроль загрузки lookup
            // временно создаем ItemSelector чтобы получить lookup модель, которая будет использоваться для lookup колонки,
            // не очень эстетично, но зато просто
            auto item_selector = qobject_cast<ItemSelector*>(generateWidget(col, false, false, false)->field);
            Z_CHECK_NULL(item_selector);
            addLookupInfo(col, item_selector->model());
            Utils::deleteLater(item_selector);
        }

        if (property.dataType() == DataType::Table) {
            TableView* tv = qobject_cast<TableView*>(w_info->field.data());
            Z_CHECK_NULL(tv);
            connect(tv, &TableView::sg_beforeResizeRowsToContent, this, &DataWidgets::sl_beforeResizeRowsToContent);
            connect(tv, &TableView::sg_afterResizeRowsToContent, this, &DataWidgets::sl_afterResizeRowsToContent);
        }

    } else if (!force_read_only) {
        if (property.dataType() == DataType::Integer || property.dataType() == DataType::UInteger || property.dataType() == DataType::Float
            || property.dataType() == DataType::UFloat || property.dataType() == DataType::String || property.dataType() == DataType::Variant
            || property.dataType() == DataType::Numeric || property.dataType() == DataType::UNumeric) {
            LineEdit* line_edit = qobject_cast<LineEdit*>(w_info->field.data());
            Z_CHECK_NULL(line_edit);
            connect(line_edit, &QLineEdit::textEdited, this, &DataWidgets::sl_LineEdit_textEdited);
            connect(line_edit, &QLineEdit::cursorPositionChanged, this, &DataWidgets::sl_lineEditCursorPositionChanged);

        } else if (property.dataType() == DataType::Date) {
            DateEdit* date_edit = qobject_cast<DateEdit*>(w_info->field.data());
            Z_CHECK_NULL(date_edit);
            connect(date_edit, &DateEdit::sg_dateChanged, this, &DataWidgets::sl_DateEdit_dateChanged);
            connect(date_edit, &DateEdit::sg_textChanged, this, &DataWidgets::sl_DateEdit_textChanged);
            connect(date_edit, &DateEdit::sg_cursorPositionChanged, this, &DataWidgets::sl_dateEditCursorPositionChanged);

        } else if (property.dataType() == DataType::DateTime) {
            DateTimeEdit* date_edit = qobject_cast<DateTimeEdit*>(w_info->field.data());
            Z_CHECK_NULL(date_edit);
            connect(date_edit, &QDateEdit::dateTimeChanged, this, &DataWidgets::sl_DateTimeEdit_dateTimeChanged);

        } else if (property.dataType() == DataType::Time) {
            TimeEdit* time_edit = qobject_cast<TimeEdit*>(w_info->field.data());
            Z_CHECK_NULL(time_edit);
            connect(time_edit, &TimeEdit::timeChanged, this, &DataWidgets::sl_TimeEdit_dateTimeChanged);

        } else
            Z_HALT(QString::number(static_cast<int>(property.dataType())));
    }

    _properties_info[property] = w_info;
    _properties[w_info->field.data()] = property;

    w_info->field->installEventFilter(this);

    unBlockUpdateProperty(property);

    if (property.propertyType() == PropertyType::Dataset) {
        initDatasetHorizontalHeader(property);
        if (property.options().testFlag(PropertyOption::ReadOnly) && !property.options().testFlag(PropertyOption::EnableViewOperation))
            setDatasetToolbarHidden(property, true);
    }

    return w_info->field;
}

void DataWidgets::getLookupInfo(const I_DataWidgetsLookupModels* dynamic_lookup, const DataProperty& property, ModelPtr& lookup_model,
    DataFilterPtr& lookup_filter, DataProperty& lookup_column_display, DataProperty& lookup_column_id, int& lookup_role_display, int& lookup_role_id)
{
    Z_CHECK_NULL(dynamic_lookup);
    Z_CHECK(property.isValid());

    lookup_filter.reset();
    lookup_role_display = Qt::DisplayRole;
    lookup_role_id = Qt::DisplayRole;
    dynamic_lookup->getPropertyLookupModel(property, lookup_model, lookup_filter, lookup_column_display, lookup_column_id, lookup_role_display, lookup_role_id);
    Z_CHECK_X(lookup_model != nullptr, QString("dynamic lookup not initialized %1:%2").arg(property.entityCode().value()).arg(property.id().value()));
    Z_CHECK(lookup_role_display >= 0);
    Z_CHECK(lookup_role_id >= 0);
    if (lookup_filter != nullptr)
        Z_CHECK(lookup_filter->source() == lookup_model.get());

    if (!lookup_column_display.isValid()) {
        // определяем колонку автоматически
        Z_CHECK(lookup_model->structure()->isSingleDataset());

        auto name_col = lookup_model->structure()->datasetColumnsByOptions(lookup_model->structure()->singleDataset(), PropertyOption::FullName);
        if (name_col.isEmpty())
            name_col = lookup_model->structure()->datasetColumnsByOptions(lookup_model->structure()->singleDataset(), PropertyOption::Name);
        if (!name_col.isEmpty())
            lookup_column_display = name_col.constFirst();
    }
    Z_CHECK(lookup_column_display.isColumn());

    if (!lookup_column_id.isValid()) {
        // определяем колонку автоматически
        Z_CHECK(lookup_model->structure()->isSingleDataset());

        auto id_col = lookup_model->structure()->datasetColumnsByOptions(lookup_model->structure()->singleDataset(), PropertyOption::Id);
        if (!id_col.isEmpty())
            lookup_column_id = id_col.constFirst();
    }
    Z_CHECK(lookup_column_id.isColumn());

    Z_CHECK(lookup_column_display.dataset() == lookup_column_id.dataset());
}

void DataWidgets::getLookupInfo(const DataStructure* lookup_data_structure, const DataProperty& property, DataProperty& lookup_dataset,
    DataProperty& lookup_column_display, DataProperty& lookup_column_id, int& lookup_role_display, int& lookup_role_id)
{
    Z_CHECK_NULL(lookup_data_structure);
    Z_CHECK(property.isValid());
    Z_CHECK_NULL(property.lookup());
    int lookup_column_display_pos, lookup_column_id_pos;
    lookup_data_structure->getLookupInfo(property, lookup_dataset, lookup_column_display_pos, lookup_column_id_pos);
    lookup_role_display = property.lookup()->displayColumnRole();
    lookup_role_id = property.lookup()->dataColumnRole();

    Z_CHECK(lookup_column_display_pos >= 0 && lookup_column_display_pos < lookup_dataset.columns().count());
    Z_CHECK(lookup_column_id_pos >= 0 && lookup_column_id_pos < lookup_dataset.columns().count());
    lookup_column_display = lookup_dataset.columns().at(lookup_column_display_pos);
    lookup_column_id = lookup_dataset.columns().at(lookup_column_id_pos);
}

int DataWidgets::defaultSectionSizePx(const PropertyID& dataset_property_id, int section_id) const
{
    return defaultSectionSizePx(property(dataset_property_id), section_id);
}

int DataWidgets::defaultSectionSizePx(const DataProperty& dataset_property, int section_id) const
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    Z_CHECK(structure()->contains(dataset_property));

    auto root = horizontalRootHeaderItem(dataset_property);
    int col_index = root->item(section_id)->bottomPos();
    Z_CHECK(dataset_property.columns().count() > col_index);

    auto column_property = dataset_property.columns().at(col_index);

    DataType data_type;

    if (column_property.lookup() == nullptr)
        data_type = column_property.dataType();
    else
        data_type = DataType::String;

    int default_size = defaultSectionSizeChar(data_type);
    return default_size <= 0 ? root->defaultSectionSize()
                             : default_size * field<QAbstractItemView>(dataset_property)->fontMetrics().horizontalAdvance(Consts::AVERAGE_CHAR);
}

int DataWidgets::defaultSectionSizePx(const DataProperty& column_property) const
{
    return defaultSectionSizePx(column_property.dataset(), column_property.id().value());
}

int DataWidgets::defaultSectionSizeChar(DataType data_type)
{
    switch (data_type) {
        case DataType::String:
            return 20;
        case DataType::Memo:
        case DataType::RichMemo:
            return 30;
        case DataType::Integer:
        case DataType::UInteger:
        case DataType::Float:
        case DataType::UFloat:
            return 10;
        case DataType::Date:
            return 10;
        case DataType::DateTime:
            return 15;
        case DataType::Time:
            return 5;
        case DataType::Bool:
            return 5;
        case DataType::Numeric:
        case DataType::UNumeric:
            return 15;

        default:
            return -1;
    }
}

HeaderItem* DataWidgets::rootHeaderItem(const DataProperty& dataset_property, Qt::Orientation orientation) const
{
    QAbstractItemView* view = field<QAbstractItemView>(dataset_property);
    TableView* table = qobject_cast<TableView*>(view);
    if (table != nullptr)
        return table->rootHeaderItem(orientation);
    TreeView* tree = qobject_cast<TreeView*>(view);
    if (tree != nullptr) {
        Z_CHECK(orientation == Qt::Horizontal);
        return tree->rootHeaderItem();
    }
    Z_HALT_INT;
    return nullptr;
}

HeaderItem* DataWidgets::horizontalRootHeaderItem(const DataProperty& dataset_property) const
{
    return rootHeaderItem(dataset_property, Qt::Horizontal);
}

HeaderItem* DataWidgets::horizontalRootHeaderItem(const PropertyID& dataset_property_id) const
{
    return horizontalRootHeaderItem(property(dataset_property_id));
}

HeaderItem* DataWidgets::verticalRootHeaderItem(const DataProperty& dataset_property) const
{
    return rootHeaderItem(dataset_property, Qt::Vertical);
}

HeaderItem* DataWidgets::verticalRootHeaderItem(const PropertyID& dataset_property_id) const
{
    return verticalRootHeaderItem(property(dataset_property_id));
}

HeaderItem* DataWidgets::horizontalHeaderItem(const DataProperty& dataset_property, int section_id) const
{
    return horizontalRootHeaderItem(dataset_property)->item(section_id);
}

HeaderItem* DataWidgets::horizontalHeaderItem(const PropertyID& dataset_property_id, int section_id) const
{
    return horizontalHeaderItem(property(dataset_property_id), section_id);
}

HeaderItem* DataWidgets::horizontalHeaderItem(const DataProperty& section_id) const
{
    return horizontalHeaderItem(section_id.dataset(), section_id.id().value());
}

HeaderItem* DataWidgets::horizontalHeaderItem(const PropertyID& section_id) const
{
    return horizontalHeaderItem(property(section_id));
}

HeaderItem* DataWidgets::verticalHeaderItem(const DataProperty& dataset_property, int section_id) const
{
    return verticalRootHeaderItem(dataset_property)->item(section_id);
}

HeaderItem* DataWidgets::verticalHeaderItem(const PropertyID& dataset_property_id, int section_id) const
{
    return verticalHeaderItem(property(dataset_property_id), section_id);
}

HeaderView* DataWidgets::horizontalHeaderView(const DataProperty& dataset_property) const
{
    auto w = field(dataset_property);

    if (auto table = qobject_cast<TableView*>(w))
        return table->horizontalHeader();

    if (auto tree = qobject_cast<TreeView*>(w))
        return tree->horizontalHeader();

    Z_HALT_INT;
    return nullptr;
}

HeaderView* DataWidgets::horizontalHeaderView(const PropertyID& dataset_property_id) const
{
    return horizontalHeaderView(property(dataset_property_id));
}

void DataWidgets::setSectionSizePx(const DataProperty& dataset_property, int section_id, int size)
{
    horizontalHeaderItem(dataset_property, section_id)->setSectionSize(size);
}

void DataWidgets::setSectionSizePx(const PropertyID& dataset_property_id, int section_id, int size)
{
    setSectionSizePx(property(dataset_property_id), section_id, size);
}

void DataWidgets::setSectionSizePx(const DataProperty& column_property, int size)
{
    setSectionSizePx(column_property.dataset(), column_property.id().value(), size);
}

void DataWidgets::setSectionSizeChar(const DataProperty& dataset_property, int section_id, int size)
{
    horizontalHeaderItem(dataset_property, section_id)->setSectionSizeCharCount(size);
}

void DataWidgets::setSectionSizeChar(const PropertyID& dataset_property_id, int section_id, int size)
{
    setSectionSizeChar(property(dataset_property_id), section_id, size);
}

void DataWidgets::setSectionSizeChar(const DataProperty& column_property, int size)
{
    setSectionSizeChar(column_property.dataset(), column_property.id().value(), size);
}

int DataWidgets::sectionSizePx(const DataProperty& dataset_property, int section_id) const
{
    return horizontalHeaderItem(dataset_property, section_id)->sectionSize();
}

int DataWidgets::sectionSizePx(const PropertyID& dataset_property_id, int section_id) const
{
    return sectionSizePx(property(dataset_property_id), section_id);
}

int DataWidgets::sectionSizePx(const DataProperty& column_property) const
{
    return sectionSizePx(column_property.dataset(), column_property.id().value());
}

void DataWidgets::initDatasetHorizontalHeader(const DataProperty& dataset_property)
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    if (dataset_property.options().testFlag(PropertyOption::SimpleDataset))
        return;

    DataProperty sort_column;

    auto root = horizontalRootHeaderItem(dataset_property);
    root->beginUpdate();
    root->clear();
    for (auto& col : dataset_property.columns()) {
        auto header = root->append(col);
        if (col.options().testFlag(PropertyOption::Hidden) && dataset_property.columns().count() > 1)
            header->setPermanentHidden(true);
        else
            resetSectionSize(col);

        if (!sort_column.isValid() && col.options().testFlag(PropertyOption::SortColumn))
            sort_column = col;

        if (col.dataType() == DataType::Bool) {
            if (dataset_property.dataType() == DataType::Table)
                object<TableView>(dataset_property)->setBooleanColumn(col.pos(), true);
            else
                object<TreeView>(dataset_property)->setBooleanColumn(col.pos(), true);
        }

        if (dataset_property.options().testFlag(PropertyOption::NoEditTriggers) || col.options().testFlag(PropertyOption::NoEditTriggers)) {
            if (dataset_property.dataType() == DataType::Table)
                object<TableView>(dataset_property)->setNoEitTriggersColumn(col.pos(), true);
            else
                object<TreeView>(dataset_property)->setNoEitTriggersColumn(col.pos(), true);
        }
    }
    root->endUpdate();

    if (sort_column.isValid())
        sortDataset(sort_column, Qt::AscendingOrder);
}

void DataWidgets::initDatasetHorizontalHeaders()
{
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        initDatasetHorizontalHeader(p);
    }
}

void DataWidgets::resetSectionSize(const PropertyID& dataset_property_id, int section_id)
{
    setSectionSizePx(dataset_property_id, section_id, defaultSectionSizePx(dataset_property_id, section_id));
}

void DataWidgets::resetSectionSize(const DataProperty& column_property)
{
    resetSectionSize(column_property.dataset().id(), column_property.id().value());
}

void DataWidgets::setDatasetSortEnabled(const DataProperty& dataset_property, bool enabled)
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    if (dataset_property.options().testFlag(PropertyOption::SortDisabled))
        return;

    Z_CHECK(filter() == nullptr || !filter()->easySortColumn(dataset_property).isValid());

    if (dataset_property.dataType() == DataType::Table)
        field<zf::TableView>(dataset_property)->setSortingEnabled(enabled);
    else if (dataset_property.dataType() == DataType::Tree)
        field<zf::TreeView>(dataset_property)->setSortingEnabled(enabled);
    else
        Z_HALT(QString("Not dataset %1:%2").arg(structure()->entityCode().value()).arg(dataset_property.id().value()));
}

void DataWidgets::setDatasetSortEnabled(const PropertyID& dataset_property_id, bool enabled)
{
    setDatasetSortEnabled(property(dataset_property_id), enabled);
}

void DataWidgets::setDatasetSortEnabledAll(bool enabled)
{
    auto props = structure()->propertiesByType(PropertyType::Dataset);
    for (auto& p : qAsConst(props)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset) || (filter() != nullptr && filter()->easySortColumn(p).isValid()))
            continue;

        setDatasetSortEnabled(p, enabled);
    }
}

void DataWidgets::sortDataset(const DataProperty& column_property, Qt::SortOrder order)
{
    Z_CHECK(column_property.propertyType() == PropertyType::ColumnFull);
    Z_CHECK(filter() == nullptr || !filter()->easySortColumn(column_property.dataset()).isValid());

    if (column_property.dataset().dataType() == DataType::Table)
        field<zf::TableView>(column_property.dataset())->sortByColumn(column_property.pos(), order);
    else if (column_property.dataset().dataType() == DataType::Tree)
        field<zf::TreeView>(column_property.dataset())->sortByColumn(column_property.pos(), order);
    else
        Z_HALT_INT;
}

void DataWidgets::sortDataset(const PropertyID& column_property_id, Qt::SortOrder order)
{
    sortDataset(property(column_property_id), order);
}

int DataWidgets::datasetFrozenGroupCount(const DataProperty& dataset_property) const
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    Z_CHECK(dataset_property.dataType() == DataType::Table);

    return object<TableView>(dataset_property)->frozenGroupCount();
}

int DataWidgets::datasetFrozenGroupCount(const PropertyID& dataset_property_id) const
{
    return datasetFrozenGroupCount(property(dataset_property_id));
}

void DataWidgets::setDatasetFrozenGroupCount(const DataProperty& dataset_property, int count)
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    Z_CHECK(dataset_property.dataType() == DataType::Table);
    object<TableView>(dataset_property)->setFrozenGroupCount(count);
}

void DataWidgets::setDatasetFrozenGroupCount(const PropertyID& dataset_property_id, int count)
{
    setDatasetFrozenGroupCount(property(dataset_property_id), count);
}

void DataWidgets::beginUpdate(UpdateReason reason)
{
    _updating_counter++;
    requestUpdateWidgets(reason);
}

void DataWidgets::endUpdate(UpdateReason reason)
{
    Z_CHECK(_updating_counter > 0);
    _updating_counter--;
    requestUpdateWidgets(reason);
}

bool DataWidgets::isUpdating() const
{
    return _updating_counter > 0;
}

bool DataWidgets::requestUpdateWidgets(UpdateReason reason, bool force_reason)
{
    Z_CHECK(reason != UpdateReason::Undefined);

    if (Utils::isAppHalted())
        return false;

    bool requested = false;
    if (_block_update.isEmpty()) {
        if (_callback_manager->isRequested(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY)) {
            auto current_reason = static_cast<UpdateReason>(_callback_manager->data(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY).toInt());

            auto new_reason = force_reason? reason : getImportantUpdateReason(current_reason, reason);
            if (new_reason != current_reason)
                _callback_manager->addRequest(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY, QVariant(static_cast<int>(new_reason)));

        } else {
            _callback_manager->addRequest(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY, QVariant(static_cast<int>(reason)));
        }

        _callback_manager->removeRequest(this, Framework::DATA_WIDGETS_UPDATE_REQUESTS_CALLBACK_KEY);
        _update_requests.clear();

        requested = true;

    } else {
        for (auto& p : structure()->propertiesMain()) {
            if (isBlockUpdateProperty(p))
                continue;
            requested |= requestUpdateWidget(p, reason);
        }
    }

    return requested;
}

bool DataWidgets::contains(const DataProperty& property) const
{
    return _properties_info.contains(property);
}

bool DataWidgets::contains(const PropertyID& property_id) const
{
    return _view->structure()->contains(property_id) && contains(_view->structure()->property(property_id));
}

bool DataWidgets::requestUpdateWidget(const DataProperty& property, UpdateReason reason, bool ignore_block)
{
    Z_CHECK(reason != UpdateReason::Undefined);

    if (objectExtensionDestroyed() || (!ignore_block && isBlockUpdateProperty(property)))
        return false;

    if (_callback_manager->isRequested(this, Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY))
        return true;

    auto current_reason = _update_requests.value(property, UpdateReason::Undefined);
    auto new_reason = getImportantUpdateReason(current_reason, reason);
    if (new_reason != current_reason) {
        _update_requests[property] = new_reason;
        _callback_manager->addRequest(this, Framework::DATA_WIDGETS_UPDATE_REQUESTS_CALLBACK_KEY);
        return true;
    }
    return false;
}

void DataWidgets::sl_callback(int key, const QVariant& data)
{
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::DATA_WIDGETS_UPDATE_REQUESTS_CALLBACK_KEY) {
        auto props = _update_requests;
        _update_requests.clear();
        for (auto i = props.constBegin(); i != props.constEnd(); ++i) {
            updateWidgetHelper(i.key(), i.value());
        }

    } else if (key == Framework::DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY) {
        updateWidgets(static_cast<UpdateReason>(data.toInt()));
    }
}

void DataWidgets::sl_languageChanged(QLocale::Language language)
{
    Q_UNUSED(language)
    if (!isUpdating())
        requestUpdateWidgets(UpdateReason::NoReason);
}

void DataWidgets::updateWidgetHelper(const DataProperty& property, UpdateReason reason, bool ignore_block, int cursor_pos)
{
    if (!ignore_block && isBlockUpdateProperty(property))
        return;

    if (field(property, false) == nullptr)
        return;

    blockUpdateProperty(property);

    if (!property.isDatasetPart()) {
        if (property.propertyType() == PropertyType::Dataset) {
            if (property.options().testFlag(PropertyOption::SimpleDataset)) {
                Z_CHECK_NULL(property.lookup());

                QVariantList values;
                if (_data->isInitialized(property))
                    values = _data->getSimpleValues(property);

                QWidget* widget = field(property);
                updateWidget(widget, structure()->property(property.lookup()->datasetKeyColumnID()), values, reason);
            }

        } else {
            QMap<PropertyID, QVariant> values;

            // дополнительно обновить поле, в которое надо помещать результат свободного ввода текста
            auto links = property.links(PropertyLinkType::LookupFreeText);
            if (!links.isEmpty())
                values[links.constFirst()->linkedPropertyId()] = _data->value(links.constFirst()->linkedPropertyId());

            values[property.id()] = _data->value(property);

            updateRequestSelectorWidget(property, reason);
            updateWidget(field(property), property, values, reason, cursor_pos);
        }
    }

    unBlockUpdateProperty(property);
}

void DataWidgets::updateRequestSelectorWidget(const DataProperty& property, UpdateReason reason)
{
    Q_UNUSED(reason);

    // Обновить код родителя для RequestSelector
    DataPropertyList rsp = _request_selector_parent.values(property);
    for (const DataProperty& p : qAsConst(rsp)) {
        // изменилось родительское свойство для RequestSelector
        Z_CHECK_NULL(p.lookup());
        Z_CHECK(!p.lookup()->parentKeyProperties().isEmpty());
        auto request_selector = field<RequestSelector>(p);

        QVariantList values;
        for (auto& p : p.lookup()->parentKeyProperties()) {
            values << _data->value(p);
        }

        request_selector->setParentKeys(values);
    }
}

void DataWidgets::updateRequestSelectorFreeTextProperty(const DataProperty& property)
{
    auto links = property.links(PropertyLinkType::LookupFreeText);
    if (!links.isEmpty()) {
        Z_CHECK(links.count() == 1);
        Z_CHECK(links.constFirst()->isValid());

        // linkedPropertyId можно обновлять через LookupFreeText только если оно не обновляется через attributes или если RequestSelector не содержит ID
        auto rs = object<RequestSelector>(property);
        auto free_text_p = links.constFirst()->linkedPropertyId();
        if (!rs->currentID().isValid() || !property.lookup()->attributes().values().contains(free_text_p))
            setNewValue(this->property(free_text_p), {{free_text_p, rs->currentText()}}, {free_text_p});
    }
}

void DataWidgets::onDateChanged(QWidget* w)
{
    if (isUpdating())
        return;

    Z_CHECK_NULL(w);
    DataProperty p = _properties.value(w);
    Z_CHECK(p.isValid());
    setNewValue(p, extractWidgetValues(p, w, _dynamic_lookup), {p.id()});
}

void DataWidgets::setNewValueSimpleDataset(const DataProperty& property, const QVariantList& values)
{
    if (isBlockUpdateProperty(property))
        return;

    blockUpdateProperty(property);
    data()->setSimpleValues(property, values);
    unBlockUpdateProperty(property);
}

int DataWidgets::getCursorPos(const DataProperty& property) const
{
    QWidget* w = field(property);
    if (auto ed = qobject_cast<QLineEdit*>(w))
        return ed->cursorPosition();
    if (auto ed = qobject_cast<zf::LineEdit*>(w))
        return ed->cursorPosition();

    return -1;
}

void DataWidgets::setNewValue(const DataProperty& main_property, const QMap<PropertyID, QVariant>& values, const PropertyIDList& block_properties)
{
    QMap<PropertyID, QVariant> values_prepared;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (isBlockUpdateProperty(it.key()))
            continue;
        values_prepared[it.key()] = it.value();
    }

    for (auto it = block_properties.constBegin(); it != block_properties.constEnd(); ++it) {
        blockUpdateProperty(*it);
    }

    bool main_changed = false;
    for (auto it = values_prepared.constBegin(); it != values_prepared.constEnd(); ++it) {
        Z_CHECK(it.key().isValid());

        auto prop = property(it.key());

        QVariant v;
        auto error = Utils::convertValue(
            it.value(), prop.dataType(), Core::locale(LocaleType::UserInterface), ValueFormat::Internal, Core::locale(LocaleType::Universal), v);
        if (error.isOk()) {
            QVariant old_value = _data->value(prop);

            // мы не хотим перетирать значения таких свойств (см. PropertyOption::EditedByUser)
            if (main_property.lookup() != nullptr && prop != main_property && prop.options().testFlag(PropertyOption::EditedByUser)
                && main_property.lookup()->attributes().values().contains(prop.id()) && old_value.isValid())
                continue;

            // надо избежать случайной ициализации поля, чтобы корректно работала подгрузка данных с сервера
            if (v.isValid())
                error = _data->setValue(prop, v);
            else if (_data->isInitialized(prop))
                error = _data->clearValue(prop);

            if (error.isOk() && prop == main_property)
                main_changed = !Utils::compareVariant(old_value, v);
        }

        if (error.isError()) {
            // восстанавливаем состояние виджета.
            updateWidgetHelper(prop, UpdateReason::NoReason, true, propertyInfo(prop, true)->cursor_pos);
        }
    }

    for (auto it = block_properties.constBegin(); it != block_properties.constEnd(); ++it) {
        unBlockUpdateProperty(*it);
    }

    if (!isBlockUpdateProperty(main_property) && main_changed) {
        DataPropertyList rsp = _request_selector_parent.values(main_property);
        for (const DataProperty& p : qAsConst(rsp)) {
            // изменилось родительское свойство для RequestSelector
            Z_CHECK_NULL(p.lookup());
            Z_CHECK(!p.lookup()->parentKeyProperties().isEmpty());
            setNewValue(p, QMap<PropertyID, QVariant> {{p.id(), QVariant()}}, {p.id()});
            if (!isUpdating())
                requestUpdateWidget(p, UpdateReason::NoReason);
        }
    }
}

void DataWidgets::disconnectFromItemModels()
{
    if (!_connected)
        return;

    _connected = false;

    for (const auto& p : _data->structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto view = field<QAbstractItemView>(p);

        if (view->selectionModel() != nullptr)
            disconnect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &DataWidgets::sl_currentCellChanged);

        if (view->model() != nullptr)
            view->setModel(nullptr);
    }
}

void DataWidgets::connectToItemModels()
{
    if (_connected || _properties_info.isEmpty())
        return;

    _connected = true;

    for (const auto& p : _data->structure()->propertiesByType(PropertyType::Dataset)) {
        if (p.options().testFlag(PropertyOption::SimpleDataset))
            continue;

        auto view = field<QAbstractItemView>(p);
        if (view->model() != nullptr)
            continue;

        if (_view == nullptr) {
            view->setModel(_data->dataset(p));

        } else {
            if (_filter != nullptr) {
                view->setModel(_filter->proxyDataset(p));

            } else {
                view->setModel(_view->data()->dataset(p));
            }
        }
        Z_CHECK_NULL(view->model());

        connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &DataWidgets::sl_currentCellChanged);
    }
}

void DataWidgets::checkProperty(const DataProperty& p, PropertyType type) const
{
    if (p.propertyType() != type)
        Z_HALT(QString("Invalid property type %1:%2").arg(structure()->entityCode().value()).arg(p.id().value()));
}

void DataWidgets::checkProperty(const DataProperty& p, const QList<PropertyType>& type) const
{
    for (PropertyType t : type) {
        if (p.propertyType() == t)
            return;
    }
    Z_HALT(QString("Invalid property type %1:%2").arg(structure()->entityCode().value()).arg(p.id().value()));
}

void DataWidgets::addLookupInfo(const DataProperty& property, const ModelPtr& model)
{
    auto lookup_info = _lookup_info_by_model.value(model.get());
    if (lookup_info == nullptr) {
        lookup_info = Z_MAKE_SHARED(LookupInfo);
        lookup_info->lookup = model;
        Z_CHECK(Utils::isMainThread(model.get()));
        _lookup_info_by_model[model.get()] = lookup_info;

        if (model->isLoading(false)) {
            lookup_info->loading_counter++;
            _lookup_loading_counter++;

            if (_lookup_loading_counter == 1)
                emit sg_beginLookupLoad();
        }

        connect(model.get(), &Model::sg_startLoad, this, &DataWidgets::sl_lookupModelStartLoad);
        connect(model.get(), &Model::sg_finishLoad, this, &DataWidgets::sl_lookupModelFinishLoad);
    }
    lookup_info->properties << property;
}

void DataWidgets::removeLookupInfo(const ModelPtr& model)
{
    auto lookup_info = _lookup_info_by_model.value(model.get());
    if (lookup_info == nullptr)
        return;

    disconnect(model.get(), &Model::sg_startLoad, this, &DataWidgets::sl_lookupModelStartLoad);
    disconnect(model.get(), &Model::sg_finishLoad, this, &DataWidgets::sl_lookupModelFinishLoad);
    _lookup_info_by_model.remove(model.get());

    if (lookup_info->lookup->isLoading(false)) {
        _lookup_loading_counter--;
        Z_CHECK(_lookup_loading_counter >= 0);

        if (_lookup_loading_counter == 0)
            emit sg_endLookupLoad();
    }
}

DataWidgets::WidgetInfo* DataWidgets::propertyInfo(const DataProperty& property, bool halt_if_not_found) const
{
    generateWidgetsLazy();

    auto w = _properties_info.value(property);
    Z_CHECK_X(!halt_if_not_found || w != nullptr, QString("widget not found %1,%2").arg(_data->structure()->id().value()).arg(property.id().value()));
    return w.get();
}

bool DataWidgets::isBlockUpdateProperty(const DataProperty& property) const
{
    return _block_update.value(property, 0) > 0;
}

bool DataWidgets::isBlockUpdateProperty(const PropertyID& property_id) const
{
    return isBlockUpdateProperty(property(property_id));
}

void DataWidgets::blockUpdateProperty(const DataProperty& property) const
{
    _block_update[property] = _block_update.value(property, 0) + 1;
}

void DataWidgets::blockUpdateProperty(const PropertyID& property_id) const
{
    blockUpdateProperty(property(property_id));
}

bool DataWidgets::unBlockUpdateProperty(const DataProperty& property) const
{
    auto res = _block_update.find(property);
    int count = res != _block_update.end() ? res.value() : 0;
    Z_CHECK(count > 0);
    if (count - 1 == 0)
        _block_update.remove(property);
    else
        res.value() = count - 1;

    return count == 1;
}

bool DataWidgets::unBlockUpdateProperty(const PropertyID& property_id) const
{
    return unBlockUpdateProperty(property(property_id));
}

} // namespace zf
