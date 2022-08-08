#include "zf_highlight_dialog.h"
#include "ui_zf_highlight_dialog.h"
#include "zf_highlight_view.h"
#include "zf_highlight_view_controller.h"
#include "zf_data_change_processor.h"
#include "zf_widget_highlighter.h"
#include "zf_widget_highligter_controller.h"
#include "zf_highlight_mapping.h"
#include "zf_framework.h"
#include "zf_colors_def.h"

#define SPLITTER_CONFIG "_HD_MAIN_SPLITTER"

namespace zf
{
HighlightDialog::HighlightDialog(const DataContainerPtr& container, const QList<int>& buttons_list, const QStringList& buttons_text,
                                 const QList<int>& buttons_role, const HighlightDialogOptions& options)
    : Dialog(buttons_list, buttons_text, buttons_role)
    , _ui(new Ui::HighlightDialog)
    , _structure(container->structure())
    , _data(container)
    , _options(options)
{
    Z_CHECK_NULL(_data);
    Z_CHECK_NULL(_structure);

    bootstrap();
}

HighlightDialog::HighlightDialog(const DataContainerPtr& container, Type type, const HighlightDialogOptions& options)
    : Dialog(type)
    , _ui(new Ui::HighlightDialog)
    , _structure(container->structure())
    , _data(container)
    , _options(options)
{
    Z_CHECK_NULL(_data);
    Z_CHECK_NULL(_structure);

    bootstrap();
}

HighlightDialog::HighlightDialog(const DataStructurePtr& structure, const QList<int>& buttons_list, const QStringList& buttons_text,
                                 const QList<int>& buttons_role, const HighlightDialogOptions& options)
    : Dialog(buttons_list, buttons_text, buttons_role)
    , _ui(new Ui::HighlightDialog)
    , _structure(structure)
    , _options(options)

{
    Z_CHECK_NULL(_structure);

    bootstrap();
}

HighlightDialog::HighlightDialog(const DataStructurePtr& structure, Type type, const HighlightDialogOptions& options)
    : Dialog(type)
    , _ui(new Ui::HighlightDialog)
    , _structure(structure)
    , _options(options)
{
    Z_CHECK_NULL(_structure);

    bootstrap();
}

HighlightDialog::HighlightDialog(const HighlightDialogOptions& options, I_HighlightView* highlight_view_interface,
                                 I_HightlightViewController* highlight_controller_interface, const QList<int>& buttons_list,
                                 const QStringList& buttons_text, const QList<int>& buttons_role)
    : Dialog(buttons_list, buttons_text, buttons_role)
    , _ui(new Ui::HighlightDialog)
    , _highlight_view_interface(highlight_view_interface)
    , _highlight_controller_interface(highlight_controller_interface)
    , _options(options)
{
    Z_CHECK_NULL(_highlight_view_interface);
    Z_CHECK_NULL(_highlight_controller_interface);

    bootstrap();
}

HighlightDialog::HighlightDialog(const HighlightDialogOptions& options, I_HighlightView* highlight_view_interface,
                                 I_HightlightViewController* highlight_controller_interface, Type type)
    : Dialog(type)
    , _ui(new Ui::HighlightDialog)
    , _highlight_view_interface(highlight_view_interface)
    , _highlight_controller_interface(highlight_controller_interface)
    , _options(options)
{
    Z_CHECK_NULL(_highlight_view_interface);
    Z_CHECK_NULL(_highlight_controller_interface);

    bootstrap();
}

HighlightDialog::~HighlightDialog()
{
    _highlight_processor->removeExternalProcessing(this);

    _highlight_view->setHidden(true);
    _highlight_view->setParent(nullptr);
    _highlight_view->objectExtensionDestroy();
}

void HighlightDialog::open()
{    
    Dialog::open();
}

int HighlightDialog::exec()
{    
    return Dialog::exec();
}

HighlightProcessor* HighlightDialog::highlightProcessor() const
{
    return hightlightControllerGetProcessor();
}

DataWidgets* HighlightDialog::widgets() const
{
    createUI_helper();
    return hightlightControllerGetWidgets();
}

DataContainer* HighlightDialog::data() const
{
    return widgets()->data().get();
}

const DataStructure* HighlightDialog::structure() const
{
    return data()->structure().get();
}

QWidget* HighlightDialog::body() const
{
    return _ui->hd_body;
}

Error HighlightDialog::setUI(const QString& resource_name)
{
    return Dialog::setUI(resource_name, body());
}

DataProperty HighlightDialog::property(const PropertyID& property_id) const
{
    return structure()->property(property_id);
}

int HighlightDialog::column(const DataProperty& column_property) const
{
    return structure()->column(column_property);
}

int HighlightDialog::column(const PropertyID& column_property_id) const
{
    return structure()->column(column_property_id);
}

void HighlightDialog::setTabOrder(const QList<QWidget*>& tab_order)
{
    Dialog::setTabOrder(tab_order);
    emit sg_highlightViewSortOrderChanged();
}

const HighlightModel* HighlightDialog::highlight(bool execute_check) const
{
    if (execute_check)
        _highlight_processor->executeHighlightCheckRequests();

    return _highlight_processor->highlight();
}

void HighlightDialog::registerHighlightCheck(const DataProperty& property) const
{
    _highlight_processor->registerHighlightCheck(property);
}

void HighlightDialog::registerHighlightCheck(const PropertyID& property_id) const
{
    _highlight_processor->registerHighlightCheck(property_id);
}

void HighlightDialog::registerHighlightCheck(int row, const DataProperty& column, const QModelIndex& parent) const
{
    registerHighlightCheck(data()->cellProperty(row, column, parent));
}

void HighlightDialog::registerHighlightCheck(int row, const PropertyID& column, const QModelIndex& parent) const
{
    registerHighlightCheck(row, property(column), parent);
}

void HighlightDialog::registerHighlightCheck(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    registerHighlightCheck(data()->rowProperty(dataset, row, parent));
}

void HighlightDialog::registerHighlightCheck(const PropertyID& dataset, int row, const QModelIndex& parent) const
{
    registerHighlightCheck(property(dataset), row, parent);
}

void HighlightDialog::registerHighlightCheck() const
{
    _highlight_processor->registerHighlightCheck();
}

void HighlightDialog::executeHighlightCheckRequests() const
{
    _highlight_processor->executeHighlightCheckRequests();
}

void HighlightDialog::clearHighlightCheckRequests() const
{
    _highlight_processor->clearHighlightCheckRequests();
}

QFrame* HighlightDialog::highlightPanelFrame() const
{
    return _highlight_panel;
}

bool HighlightDialog::highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const
{
    // при изменении тут - поправить View::highlightViewGetSortOrder
    if (_highlight_view_interface)
        return _highlight_view_interface->highlightViewGetSortOrder(property1, property2);

    if (property1.propertyType() == PropertyType::Cell || property2.propertyType() == PropertyType::Cell) {
        if (property1.propertyType() != PropertyType::Cell)
            return true;
        if (property2.propertyType() != PropertyType::Cell)
            return false;

        if (property1.dataset() != property2.dataset())
            return highlightViewGetSortOrder(property1.dataset(), property2.dataset());

        QModelIndex index1 = data()->findDatasetRowID(property1.dataset(), property1.rowId());
        index1 = data()->cellIndex(index1.row(), property1.column(), index1.parent());

        QModelIndex index2 = data()->findDatasetRowID(property2.dataset(), property2.rowId());
        index2 = data()->cellIndex(index2.row(), property2.column(), index2.parent());

        return comp(index1, index2);
    }

    QWidget* w1 = Utils::getMainWidget(widgets()->field(property1, false));
    QWidget* w2 = Utils::getMainWidget(widgets()->field(property2, false));

    int pos1 = -1;
    if (w1 != nullptr)
        pos1 = tabOrder().indexOf(w1);

    int pos2 = -1;
    if (w2 != nullptr)
        pos2 = tabOrder().indexOf(w2);

    if (pos1 < 0 && pos2 < 0)
        return property1 < property2;

    if (pos1 < 0)
        return false;

    if (pos2 < 0)
        return true;

    return pos1 < pos2;
}

bool HighlightDialog::isHighlightViewIsSortOrderInitialized() const
{
    return _highlight_view_interface ? _highlight_view_interface->isHighlightViewIsSortOrderInitialized() : !tabOrder().isEmpty();
}

bool HighlightDialog::isHighlightViewIsCheckRequired() const
{
    return _highlight_view_interface ? _highlight_view_interface->isHighlightViewIsCheckRequired() : true;
}

DataWidgets* HighlightDialog::hightlightControllerGetWidgets() const
{    
    return _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetWidgets() : _widgets.get();
}

HighlightProcessor* HighlightDialog::hightlightControllerGetProcessor() const
{
    return _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetProcessor()
                                           : _highlight_processor.get();
}

HighlightMapping* HighlightDialog::hightlightControllerGetMapping() const
{
    return _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetMapping() : _highligt_mapping;
}

DataProperty HighlightDialog::hightlightControllerGetCurrentFocus() const
{
    if (_highlight_controller_interface)
        return _highlight_controller_interface->hightlightControllerGetCurrentFocus();

    DataProperty current_property;
    QWidget* focused = workspace()->focusWidget();
    if (focused != nullptr) {
        current_property = widgets()->widgetProperty(focused);
        if (current_property.propertyType() == PropertyType::Dataset && !current_property.options().testFlag(PropertyOption::SimpleDataset)
            && widgets()->currentIndex(current_property).isValid()) {
            QModelIndex current_index = widgets()->currentIndex(current_property);
            DataProperty dataset_property = data()->datasetProperty(current_index.model());
            current_property = structure()->propertyCell(
                dataset_property, data()->datasetRowID(dataset_property, current_index.row(), current_index.parent()),
                current_index.column());
        }
    }

    return current_property;
}

bool HighlightDialog::hightlightControllerSetFocus(const DataProperty& p)
{
    return _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerSetFocus(p)
                                           : Utils::setFocus(p, _highligt_mapping);
}

void HighlightDialog::hightlightControllerBeforeFocusHighlight(const DataProperty& property)
{
    if (_highlight_controller_interface)
        _highlight_controller_interface->hightlightControllerBeforeFocusHighlight(property);
    else
        beforeFocusHighlight(property);
}

void HighlightDialog::hightlightControllerAfterFocusHighlight(const DataProperty& property)
{
    if (_highlight_controller_interface)
        _highlight_controller_interface->hightlightControllerAfterFocusHighlight(property);
    else
        afterFocusHighlight(property);
}

QString HighlightDialog::keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent,
                                                 const QVariantList& key_values) const
{
    Q_UNUSED(dataset);
    Q_UNUSED(row);
    Q_UNUSED(parent);
    return Utils::generateUniqueString(key_values);
}

void HighlightDialog::checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent, QString& error_text,
                                     DataProperty& error_property) const
{
    Q_UNUSED(dataset);
    Q_UNUSED(row);
    Q_UNUSED(parent);
    Q_UNUSED(error_text);
    Q_UNUSED(error_property);
}

void HighlightDialog::getFieldHighlight(const DataProperty& field, HighlightInfo& info) const
{
    Q_UNUSED(field)
    Q_UNUSED(info)
}

void HighlightDialog::getDatasetHighlight(const DataProperty& dataset, HighlightInfo& info) const
{
    Q_UNUSED(dataset)
    Q_UNUSED(info)
}

void HighlightDialog::getCellHighlight(const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent,
                                       HighlightInfo& info) const
{
    Q_UNUSED(dataset)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)
    Q_UNUSED(info)
}

void HighlightDialog::getHighlight(const DataProperty& p, HighlightInfo& info) const
{
    Q_UNUSED(p)
    Q_UNUSED(info)
}

void HighlightDialog::checkButtonsEnabled()
{
    Dialog::checkButtonsEnabled();
    setOkButtonEnabled(!highlight()->hasErrorsWithoutOptions(HighlightOption::NotBlocked));
}

void HighlightDialog::setFocus(const DataProperty& property)
{
    hightlightControllerSetFocus(property);
}

void HighlightDialog::focusError()
{
    // если уже находится в поле с ошибкой, то ничего не делаем
    QWidget* focused = body()->focusWidget();
    if (focused != nullptr) {
        DataProperty current_property = widgets()->widgetProperty(focused);
        if (current_property.isValid())
            if (highlight()->contains(current_property, InformationType::Error | InformationType::Critical | InformationType::Fatal))
                return;
    }

    auto items = highlight()->items(InformationType::Error | InformationType::Critical | InformationType::Fatal);
    if (items.isEmpty())
        return;

    setFocus(items.first().property());
}

void HighlightDialog::focusNextError(InformationTypes info_type)
{
    DataProperty current_property;
    QWidget* focused = focusWidget();
    if (focused != nullptr) {
        current_property = widgets()->widgetProperty(focused);
        if (current_property.propertyType() == PropertyType::Dataset && widgets()->currentIndex(current_property).isValid())
            current_property = data()->propertyCell(widgets()->currentIndex(current_property));
    }

    DataPropertyList properties = highlight(false)->properties(info_type);
    Z_CHECK(!properties.isEmpty());

    DataProperty next_property;

    if (current_property.isValid()) {
        int pos = properties.indexOf(current_property);
        if (pos >= 0) {
            if (pos == properties.count() - 1)
                pos = 0;
            else
                pos++;

            next_property = properties.at(pos);
        }
    }
    if (!next_property.isValid())
        next_property = properties.first();

    if (next_property == current_property)
        return;

    setFocus(next_property);
}

void HighlightDialog::onDataInvalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    Q_UNUSED(p)
    Q_UNUSED(invalidate)
    Q_UNUSED(info)
}

void HighlightDialog::onDataInvalidateChanged(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    Q_UNUSED(p)
    Q_UNUSED(invalidate)
    Q_UNUSED(info)
}

void HighlightDialog::onLanguageChanged(QLocale::Language language)
{
    Q_UNUSED(language)
}

void HighlightDialog::onPropertyUpdated(const DataProperty& p, ObjectActionType action)
{
    Q_UNUSED(p)
    Q_UNUSED(action)
}

void HighlightDialog::onAllPropertiesBlocked()
{
}

void HighlightDialog::onAllPropertiesUnBlocked()
{
}

void HighlightDialog::onPropertyBlocked(const DataProperty& p)
{
    Q_UNUSED(p)
}

void HighlightDialog::onPropertyUnBlocked(const DataProperty& p)
{
    Q_UNUSED(p)
}

void HighlightDialog::onPropertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    Q_UNUSED(p)
    Q_UNUSED(old_values)
}

void HighlightDialog::onDatasetCellChanged(const DataProperty left_column, const DataProperty& right_column, int top_row, int bottom_row,
                                           const QModelIndex& parent)
{
    Q_UNUSED(left_column)
    Q_UNUSED(right_column)
    Q_UNUSED(top_row)
    Q_UNUSED(bottom_row)
    Q_UNUSED(parent)
}

void HighlightDialog::onDatasetDataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                           const QVector<int>& roles)
{
    Q_UNUSED(p)
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)
}

void HighlightDialog::onDatasetHeaderDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(orientation)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetRowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetRowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetRowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetRowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetColumnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetColumnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetColumnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetColumnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void HighlightDialog::onDatasetModelAboutToBeReset(const DataProperty& p)
{
    Q_UNUSED(p)
}

void HighlightDialog::onDatasetModelReset(const DataProperty& p)
{
    Q_UNUSED(p)
}

void HighlightDialog::onDatasetRowsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                                  const QModelIndex& destinationParent, int destinationRow)
{
    Q_UNUSED(p)
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationRow)
}

void HighlightDialog::onDatasetRowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                                         const QModelIndex& destination, int row)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(row)
}

void HighlightDialog::onDatasetColumnsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                                     const QModelIndex& destinationParent, int destinationColumn)
{
    Q_UNUSED(p)
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationColumn)
}

void HighlightDialog::onDatasetColumnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                                            const QModelIndex& destination, int column)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(column)
}

void HighlightDialog::beforeFocusHighlight(const DataProperty& property)
{
    Q_UNUSED(property)
}

void HighlightDialog::afterFocusHighlight(const DataProperty& property)
{
    Q_UNUSED(property)
}

void HighlightDialog::onChangeBottomLineVisible()
{
    Dialog::onChangeBottomLineVisible();

    QMargins m = workspace()->contentsMargins();

    if (isBottomLineVisible()) {
        highlightPanelFrame()->setStyleSheet(QString());
        m.setBottom(0);

    } else {
        m.setBottom(6);
        highlightPanelFrame()->setStyleSheet(QStringLiteral("QFrame#%2 {border: 1px %1;"
                                                            "border-top-style: none;"
                                                            "border-right-style: none;"
                                                            "border-bottom-style: solid;"
                                                            "border-left-style: none;"
                                                            "}")
                                                 .arg(Colors::uiLineColor(true).name(), highlightPanelFrame()->objectName()));
    }

    workspace()->setContentsMargins(m);
}

QString HighlightDialog::hashedDatasetkeyValuesToUniqueString(const QString& key, int row, const QModelIndex& parent,
                                                              const QVariantList& keyValues) const
{
    // ключ в данном случае - id набора данных
    return keyValuesToUniqueString(property(PropertyID(key.toInt())), row, parent, keyValues);
}

void HighlightDialog::createUI_helper() const
{
    Dialog::createUI_helper();
    // заменяем эдиторы
    zf::Core::replaceWidgets(hightlightControllerGetWidgets(), body());
}

void HighlightDialog::onObjectAdded(QObject* obj)
{
    auto w = qobject_cast<QWidget*>(obj);
    if (w == nullptr)
        return;

    requestInitTabOrder();
}

void HighlightDialog::onObjectRemoved(QObject* obj)
{
    auto w = qobject_cast<QWidget*>(obj);
    if (w == nullptr)
        return;

    requestInitTabOrder();
}

void HighlightDialog::sl_highlight_itemInserted(const HighlightItem& item)
{
    Q_UNUSED(item)
    requestCheckButtonsEnabled();
}

void HighlightDialog::sl_highlight_itemRemoved(const HighlightItem& item)
{
    Q_UNUSED(item)
    requestCheckButtonsEnabled();
}

void HighlightDialog::sl_highlight_itemChanged(const HighlightItem& item)
{
    Q_UNUSED(item)
    requestCheckButtonsEnabled();
}

void HighlightDialog::sl_highlight_beginUpdate()
{
}

void HighlightDialog::sl_highlight_endUpdate()
{
    requestCheckButtonsEnabled();
}

void HighlightDialog::bootstrap()
{
    _ui->setupUi(workspace());
    workspace()->layout()->setContentsMargins(0, 1, 0, 0);
    _ui->hd_main_splitter->setCollapsible(0, false);

    setAutoSaveConfiguration(true);

    if (_highlight_controller_interface == nullptr) {
        Z_CHECK_NULL(_structure);
        Z_CHECK(_structure->isValid());
        if (_data == nullptr) {
            _data = std::make_shared<DataContainer>(_structure);
            _data->initAllValues();
        }

        _highlight_model = new HighlightModel(this);
        _highlight_processor = new HighlightProcessor(_data.get(), this, _options.testFlag(HighlightDialogOption::SimpleHighlight));
        _highlight_processor->setHashedDatasetCutomization(this);
        _highlight_processor->start();

        connect(_highlight_processor.get(), &HighlightProcessor::sg_highlightItemInserted, this,
                &HighlightDialog::sl_highlight_itemInserted);
        connect(_highlight_processor.get(), &HighlightProcessor::sg_highlightItemRemoved, this, &HighlightDialog::sl_highlight_itemRemoved);
        connect(_highlight_processor.get(), &HighlightProcessor::sg_highlightItemChanged, this, &HighlightDialog::sl_highlight_itemChanged);
        connect(_highlight_processor.get(), &HighlightProcessor::sg_highlightBeginUpdate, this, &HighlightDialog::sl_highlight_beginUpdate);
        connect(_highlight_processor.get(), &HighlightProcessor::sg_highlightEndUpdate, this, &HighlightDialog::sl_highlight_endUpdate);

        _widgets = std::make_shared<DataWidgets>(_data, nullptr, _highlight_processor.get());

        _data_processor = new DataChangeProcessor(_data.get(), this, this);
        _widget_highlighter = new WidgetHighlighter(this);
        _highligt_mapping = new HighlightMapping(_widgets.get(), this);

        _widget_highligter_controller
            = new WidgetHighligterController(_highlight_processor.get(), _widgets.get(), _widget_highlighter, _highligt_mapping);

    } else {
        Z_CHECK(_structure == nullptr);
        _highlight_processor = _highlight_controller_interface->hightlightControllerGetProcessor();
        _data_processor
            = new DataChangeProcessor(_highlight_controller_interface->hightlightControllerGetWidgets()->data().get(), this, this);
    }
    _highlight_processor->installExternalProcessing(this);

    bootstrapHightlight();
    setBottomLineVisible(true);
}

void HighlightDialog::onCallbackManagerInternal(int key, const QVariant& data)
{
    Dialog::onCallbackManagerInternal(key, data);

    if (key == Framework::HIGHLIGHT_DIALIG_CREATE_HIGHLIGHT_KEY) {
        if (!isLoading()) {
            if (isHighlightViewIsCheckRequired()) {
                _highlight_view->sync();
                _highlight_view->setVisible(true);
            }
        }
    }
}

QMap<QString, QVariant> HighlightDialog::getConfiguration() const
{
    QMap<QString, QVariant> config;
    if (isHighlightViewIsCheckRequired())
        config[SPLITTER_CONFIG] = QVariant::fromValue(_ui->hd_main_splitter->sizes());
    return config;
}

Error HighlightDialog::applyConfiguration(const QMap<QString, QVariant>& config)
{
    if (isHighlightViewIsCheckRequired()) {
        if (config.contains(SPLITTER_CONFIG))
            _ui->hd_main_splitter->setSizes(config.value(SPLITTER_CONFIG).value<QList<int>>());
        else
            _ui->hd_main_splitter->setSizes({2000, 200});
    }
    return {};
}

bool HighlightDialog::isSaveSplitterConfiguration(const QSplitter* splitter) const
{
    return splitter != _ui->hd_main_splitter;
}

void HighlightDialog::bootstrapHightlight()
{
    _highligt_mapping = new HighlightMapping(
        _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetWidgets() : _widgets.get(), this);

    _highlight_view = new HighlightView(
        _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetProcessor() : _highlight_processor.get(),
        _highlight_view_interface ? _highlight_view_interface : this,
        _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetMapping() : _highligt_mapping);

    _highlight_controller = new HightlightViewController(
        _highlight_controller_interface ? _highlight_controller_interface : this, _highlight_view,
        _highlight_controller_interface ? _highlight_controller_interface->hightlightControllerGetMapping() : _highligt_mapping);

    _highlight_panel = new QFrame;
    _highlight_panel->setFrameShape(QFrame::NoFrame);
    _highlight_panel->setObjectName("h_frame");

    _highlight_panel->setLayout(new QVBoxLayout);
    _highlight_panel->layout()->setMargin(0);
    _highlight_panel->layout()->addWidget(_highlight_view);

    _ui->hd_highlight_layout->addWidget(_highlight_panel);
    _highlight_view->setHidden(true);

    if (!isHighlightViewIsCheckRequired()) {
        _highlight_panel->setHidden(true);
        Utils::hideSplitterPart(_ui->hd_main_splitter, 1);
    }
}

void HighlightDialog::requestHighlightShow()
{
    if (objectExtensionDestroyed())
        return;

    // чтобы не дергалась картинка внутри панели ошибок
    if (_highlight_view != nullptr && !_highlight_view->isVisible())
        Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_DIALIG_CREATE_HIGHLIGHT_KEY);
}

void HighlightDialog::afterPopup()
{
    Dialog::afterPopup();
    if (!_options.testFlag(HighlightDialogOption::NoAutoShowHighlight))
        requestHighlightShow();
}

void HighlightDialog::setHighlightWidgetMapping(const DataProperty& property, QWidget* w)
{
    hightlightControllerGetMapping()->setHighlightWidgetMapping(property, w);
}

void HighlightDialog::setHighlightWidgetMapping(const PropertyID& property_id, QWidget* w)
{
    hightlightControllerGetMapping()->setHighlightWidgetMapping(property_id, w);
}

void HighlightDialog::setHighlightPropertyMapping(const DataProperty& source_property, const DataProperty& dest_property)
{
    hightlightControllerGetMapping()->setHighlightPropertyMapping(source_property, dest_property);
}

void HighlightDialog::setHighlightPropertyMapping(const PropertyID& source_property_id, const PropertyID& dest_property_id)
{
    hightlightControllerGetMapping()->setHighlightPropertyMapping(source_property_id, dest_property_id);
}

QWidget* HighlightDialog::getHighlightWidget(const DataProperty& property) const
{
    return hightlightControllerGetMapping()->getHighlightWidget(property);
}

DataProperty HighlightDialog::getHighlightProperty(QWidget* w) const
{
    return hightlightControllerGetMapping()->getHighlightProperty(w);
}

} // namespace zf
