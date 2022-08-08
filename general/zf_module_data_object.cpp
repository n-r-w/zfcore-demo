#include "zf_module_data_object.h"
#include "zf_core.h"
#include "zf_proxy_item_model.h"
#include "zf_framework.h"
#include "zf_translation.h"
#include "zf_data_change_processor.h"

namespace zf
{
ModuleDataObject::ModuleDataObject(
    const EntityCode& module_code, const DataStructurePtr& data_structure, HighlightProcessor* master_highlight, const ModuleDataOptions& options)
    : ModuleObject(module_code)
    , _data(Z_MAKE_SHARED(DataContainer, data_structure))
    , _data_processor(new DataChangeProcessor(_data.get(), this, this))
    , _options(options)
    , _highlight_processor(new HighlightProcessor(_data.get(), this, _options.testFlag(ModuleDataOption::SimpleHighlight)))
    , _master_highlight_processor(master_highlight)
{
    internalCallbackManager()->addRequest(this, Framework::MODULE_DATA_OBJECT_AFTER_CREATED);

    _highlight_processor->setObjectName(module_code.string());
    _highlight_processor->installExternalProcessing(this);

    if (_master_highlight_processor != nullptr)
        _highlight_processor->installMasterProcessor(_master_highlight_processor);

    if (options.testFlag(ModuleDataOption::HighlightEnabled) && _highlight_processor->isStopped())
        _highlight_processor->start();

    Z_CHECK_NULL(data_structure);
    Z_CHECK(data_structure->isValid());

    auto data = _data.get();
    data->setRowIdGenerator(this);

    connect(data, &DataContainer::sg_invalidateChanged, this, &ModuleDataObject::sl_invalidateChanged);
    connect(data, &DataContainer::sg_dataset_dataChanged, this, &ModuleDataObject::sl_dataset_dataChanged);
    connect(data, &DataContainer::sg_dataset_rowsInserted, this, &ModuleDataObject::sl_dataset_rowsInserted);
    connect(data, &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &ModuleDataObject::sl_dataset_rowsAboutToBeRemoved);
}

ModuleDataObject::~ModuleDataObject()
{
    _highlight_processor->removeExternalProcessing(this);
    _highlight_processor->objectExtensionDestroy();
}

void ModuleDataObject::doCopyFrom(const ModuleDataObject* source)
{    
    Z_CHECK_NULL(source);
    Z_CHECK(source->entityCode() == entityCode());        
    data()->copyFrom(source->data());
}

void ModuleDataObject::beforeCopyFrom(const ModuleDataObject* source)
{
    Q_UNUSED(source)
}

void ModuleDataObject::afterCopyFrom(const ModuleDataObject* source)
{
    Q_UNUSED(source)
}

ModuleInfo ModuleDataObject::moduleInfo() const
{
    ModuleInfo info = ModuleObject::moduleInfo();

    // модуль может не иметь структуры данных по умолчанию. в таком случае она зависит от конкретной сущности
    if (!info.isDataStructureInitialized())
        return info.createWithDataStructure(structure());

    return info;
}

DataContainerPtr ModuleDataObject::data() const
{
    return _data;
}

ModuleDataOptions ModuleDataObject::moduleDataOptions() const
{
    return _options;
}

void ModuleDataObject::copyFrom(const ModuleDataObject* source)
{
    Z_CHECK(!_is_copy_from);
    _is_copy_from = true;
    blockAllProperties();
    beforeCopyFrom(source);
    doCopyFrom(source);
    afterCopyFrom(source);
    unBlockAllProperties();
    _is_copy_from = false;
}

bool ModuleDataObject::isCopyFrom() const
{
    return _is_copy_from;
}

bool ModuleDataObject::isActive() const
{
    return _is_active;
}

Rows ModuleDataObject::findRows(const DataProperty& column, const QVariant& value, bool case_insensitive, int role) const
{
    return data()->hash()->findRows(column, value, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const PropertyID& column, const QVariant& value, bool case_insensitive, int role) const
{
    return findRows(property(column), value, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const DataProperty& column, const Uid& uid, bool case_insensitive, int role) const
{
    if (!uid.isValid())
        return Rows();

    if (uid.isTemporary())
        return findRows(column, uid.tempId(), case_insensitive, role);

    return findRows(column, uid.id().value(), case_insensitive, role);
}

Rows ModuleDataObject::findRows(const PropertyID& column, const Uid& uid, bool case_insensitive, int role) const
{
    return findRows(property(column), uid, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const DataProperty& column, const QVariantList& values, bool case_insensitive, int role) const
{
    return hash()->findRows(column, values, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const PropertyID& column, const QVariantList& values, bool case_insensitive, int role) const
{
    return hash()->findRows(property(column), values, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const DataProperty& column, const QStringList& values, bool case_insensitive, int role) const
{
    return hash()->findRows(column, values, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const PropertyID& column, const QStringList& values, bool case_insensitive, int role) const
{
    return hash()->findRows(property(column), values, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const DataProperty& column, const QList<int>& values, bool case_insensitive, int role) const
{
    return hash()->findRows(column, values, case_insensitive, role);
}

Rows ModuleDataObject::findRows(const PropertyID& column, const QList<int>& values, bool case_insensitive, int role) const
{
    return hash()->findRows(property(column), values, case_insensitive, role);
}

DataHashed* ModuleDataObject::hash() const
{
    return data()->hash();
}

void ModuleDataObject::debPrint() const
{
    data()->debPrint();
}

DataStructurePtr ModuleDataObject::structure() const
{
    return _data->structure();
}

QLocale::Language ModuleDataObject::language() const
{
    return _data->language();
}

void ModuleDataObject::setLanguage(QLocale::Language language)
{
    _data->setLanguage(language);
}

bool ModuleDataObject::invalidate(const ChangeInfo& info) const
{
    return data()->setInvalidateAll(true, info);
}

void ModuleDataObject::processCallbackInternal(int key, const QVariant& data)
{
    ModuleObject::processCallbackInternal(key, data);

    if (key == Framework::MODULE_DATA_OBJECT_AFTER_CREATED) {
        _data->setObjectName("Data, " + objectName());
        _highlight_processor->setObjectName("HP, " + objectName());
        _data_processor->setObjectName("DP, " + objectName());
    }
}

void ModuleDataObject::onActivated(bool active)
{
    Q_UNUSED(active)
}

void ModuleDataObject::onActivatedInternal(bool active)
{
    if (_is_active == active)
        return;
    _is_active = active;
    onActivated(active);
}

QVariant ModuleDataObject::getLookupValue(const DataProperty& column_property, const QVariant& lookup_value) const
{
    Z_CHECK_NULL(column_property.lookup());

    if (column_property.lookup()->type() == LookupType::List) {
        return column_property.lookup()->listName(lookup_value);

    } else if (column_property.lookup()->type() == LookupType::Dataset) {
        DataProperty lookup_dataset;
        int lookup_display_column = -1, lookup_data_column = -1;
        structure()->getLookupInfo(column_property, lookup_dataset, lookup_display_column, lookup_data_column);

        Z_CHECK(lookup_dataset.isValid());

        QModelIndexList rows = data()->hash()->findRows(column_property, lookup_value);
        if (!rows.isEmpty())
            return data()->cell(rows.first().row(), column_property.dataset().columns().at(lookup_display_column),
                column_property.lookup()->displayColumnRole(), rows.first().parent());

        return QVariant();

    } else {
        Z_HALT_INT; // тут быть не должны
        return QVariant();
    }
}

DataProperty ModuleDataObject::property(const PropertyID& property_id) const
{
    return structure()->property(property_id);
}

DataProperty ModuleDataObject::propertyRow(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    return structure()->propertyRow(dataset, datasetRowID(dataset, row, parent));
}

DataProperty ModuleDataObject::propertyColumn(const DataProperty& dataset, int column) const
{
    return structure()->propertyColumn(dataset, column);
}

DataProperty ModuleDataObject::propertyColumn(const DataProperty& dataset, const DataProperty& column_property) const
{
    return propertyColumn(dataset, column_property.dataset().columnPos(column_property));
}

DataProperty ModuleDataObject::propertyCell(const DataProperty& dataset, int row, int column, const QModelIndex& parent) const
{
    return data()->propertyCell(dataset, row, column, parent);
}

DataProperty ModuleDataObject::propertyCell(const DataProperty& row_property, const DataProperty& column_property) const
{
    return data()->propertyCell(row_property, column_property);
}

DataProperty ModuleDataObject::propertyCell(const DataProperty& row_property, const PropertyID& column_property_id) const
{
    return data()->propertyCell(row_property, column_property_id);
}

DataProperty ModuleDataObject::propertyCell(int row, const DataProperty& column_property, const QModelIndex& parent) const
{
    return data()->propertyCell(row, column_property, parent);
}

DataProperty ModuleDataObject::propertyCell(int row, const PropertyID& column_property, const QModelIndex& parent) const
{
    return data()->propertyCell(row, column_property, parent);
}

DataProperty ModuleDataObject::propertyCell(const QModelIndex& index) const
{
    return data()->propertyCell(index);
}

bool ModuleDataObject::containsProperty(const DataProperty& p) const
{
    return data()->containsProperty(p);
}

bool ModuleDataObject::propertyAvailable(const DataProperty& p) const
{
    return !data()->isInvalidated(p) && data()->isInitialized(p);
}

bool ModuleDataObject::propertyAvailable(const PropertyID& property_id) const
{
    return propertyAvailable(property(property_id));
}

int ModuleDataObject::column(const DataProperty& column_property) const
{
    return structure()->column(column_property);
}

int ModuleDataObject::column(const PropertyID& column_property_id) const
{
    return structure()->column(column_property_id);
}

RowID ModuleDataObject::datasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    RowID row_id = readDatasetRowID(dataset_property, row, parent);

    if (!row_id.isValid()) {
        auto info = datasetInfo(dataset_property, false);
        if (info->id_column >= 0) {
            // формируем на основе колонки id
            QVariant value = info->item_model()->data(row, info->id_column, parent);

            if (info->id_column_property.dataType() == DataType::UInteger || info->id_column_property.dataType() == DataType::Integer) {
                bool ok = false;
                int key = value.toInt(&ok);
                if (ok && key >= 0)
                    row_id = RowID::createRealKey(key);

            } else {
                if (info->id_column_property.dataType() == DataType::String) {
                    bool ok = false;
                    int key = value.toInt(&ok);
                    if (ok && key >= 0)
                        row_id = RowID::createRealKey(key);
                }

                if (!row_id.isValid()) {
                    QString s_id = Utils::variantToString(value, LocaleType::Universal);
                    if (!s_id.isEmpty())
                        row_id = RowID::createRealKey(s_id);
                }
            }
        }

        QModelIndex index = data()->cellIndex(dataset_property.id(), row, 0, parent);
        if (!row_id.isValid())
            row_id = RowID::createGenerated(); // не смогли создать реальный ID, генерируем случайный

        bool signals_blocked = index.model()->signalsBlocked();
        if (!signals_blocked)
            const_cast<QAbstractItemModel*>(index.model())->blockSignals(true);
        writeDatasetRowID(dataset_property, row, parent, row_id);
        if (!signals_blocked)
            const_cast<QAbstractItemModel*>(index.model())->blockSignals(false);
    }

    return row_id;
}

RowID ModuleDataObject::datasetRowID(const PropertyID& dataset_property_id, int row, const QModelIndex& parent) const
{
    return datasetRowID(property(dataset_property_id), row, parent);
}

RowID ModuleDataObject::datasetRowID(int row, const QModelIndex& parent) const
{
    return datasetRowID(structure()->singleDatasetId(), row, parent);
}

RowID ModuleDataObject::readDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    return data()->datasetRowID(dataset_property, row, parent);
}

void ModuleDataObject::writeDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id) const
{
    data()->setDatasetRowID(dataset_property, row, parent, row_id);
}

QModelIndex ModuleDataObject::findDatasetRowID(const DataProperty& dataset_property, const RowID& row_id) const
{
    return data()->findDatasetRowID(dataset_property, row_id);
}

QModelIndex ModuleDataObject::findDatasetRowID(const PropertyID& dataset_property_id, const RowID& row_id) const
{
    return findDatasetRowID(property(dataset_property_id), row_id);
}

QModelIndex ModuleDataObject::findDatasetRowID(const RowID& row_id) const
{
    return findDatasetRowID(structure()->singleDatasetId(), row_id);
}

DataPropertyList ModuleDataObject::getAllRowsProperties(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    forceCalculateRowID(dataset_property);
    return data()->getAllRowsProperties(dataset_property, parent);
}

DataPropertyList ModuleDataObject::getAllRowsProperties(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRowsProperties(property(dataset_property_id), parent);
}

QModelIndexList ModuleDataObject::getAllRowsIndexes(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    return data()->getAllRowsIndexes(dataset_property, parent);
}

QModelIndexList ModuleDataObject::getAllRowsIndexes(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRowsIndexes(property(dataset_property_id), parent);
}

Rows ModuleDataObject::getAllRows(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    return data()->getAllRows(dataset_property, parent);
}

Rows ModuleDataObject::getAllRows(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRows(property(dataset_property_id), parent);
}

void ModuleDataObject::forceCalculateRowID(const DataProperty& dataset_property) const
{
    QModelIndexList indexes = getAllRowsIndexes(dataset_property);
    for (const auto& i : qAsConst(indexes)) {
        // для формирования rowID (в случае его отсутствия) достаточно вызвать этот метод
        datasetRowID(dataset_property, i.row(), i.parent());
    }
}

void ModuleDataObject::generateRowId(const DataProperty& dataset, int row, const QModelIndex& parent)
{
    datasetRowID(dataset, row, parent);
}

void ModuleDataObject::onPropertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    Q_UNUSED(p);
    Q_UNUSED(old_values);
}

void ModuleDataObject::onDatasetCellChanged(
    const DataProperty left_column, const DataProperty& right_column, int top_row, int bottom_row, const QModelIndex& parent)
{
    Q_UNUSED(left_column)
    Q_UNUSED(right_column)
    Q_UNUSED(top_row)
    Q_UNUSED(bottom_row)
    Q_UNUSED(parent)
}

void ModuleDataObject::onDatasetDataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(p)
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)
}

void ModuleDataObject::onDatasetHeaderDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(orientation)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetRowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetRowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetRowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetRowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetColumnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetColumnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetColumnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetColumnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void ModuleDataObject::onDatasetModelAboutToBeReset(const DataProperty& p)
{
    Q_UNUSED(p)
}

void ModuleDataObject::onDatasetModelReset(const DataProperty& p)
{
    Q_UNUSED(p)
}

void ModuleDataObject::onDatasetRowsAboutToBeMoved(
    const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    Q_UNUSED(p)
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationRow)
}

void ModuleDataObject::onDatasetRowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(row)
}

void ModuleDataObject::onDatasetColumnsAboutToBeMoved(
    const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn)
{
    Q_UNUSED(p)
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceStart)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationColumn)
}

void ModuleDataObject::onDatasetColumnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column)
{
    Q_UNUSED(p)
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(column)
}

void ModuleDataObject::sl_invalidateChanged(const DataProperty& p, bool invalidate)
{
    if (invalidate)
        emit sg_invalidateChanged(p, invalidate);
}

void ModuleDataObject::sl_dataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(roles)

    // регистрация изменений
    if (!_tracking_info.isEmpty()) {
        for (int col = topLeft.column(); col <= bottomRight.column(); col++) {
            if (col >= p.columns().count())
                break;
            DataProperty column_prop = p.columns().at(col);
            for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
                registerTrackingChanges_cellChanged(row, column_prop, topLeft.parent());
            }
        }
    }
}

void ModuleDataObject::sl_dataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    registerTrackingChanges_rowAdded(p, first, last, parent);
}

void ModuleDataObject::sl_dataset_rowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    // регистрация изменений
    registerTrackingChanges_rowRemoving(p, first, last, parent);
}

QString ModuleDataObject::hashedDatasetkeyValuesToUniqueString(const QString& key, int row, const QModelIndex& parent, const QVariantList& keyValues) const
{
    // ключ в данном случае - id набора данных
    return keyValuesToUniqueString(property(PropertyID(key.toInt())), row, parent, keyValues);
}

void ModuleDataObject::onDataInvalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    Q_UNUSED(p)
    Q_UNUSED(invalidate)
    Q_UNUSED(info);
}

void ModuleDataObject::onDataInvalidateChanged(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    Q_UNUSED(p)
    Q_UNUSED(invalidate)
    Q_UNUSED(info);
}

void ModuleDataObject::onLanguageChanged(QLocale::Language language)
{
    Q_UNUSED(language)
}

void ModuleDataObject::onPropertyUpdated(const DataProperty& p, ObjectActionType action)
{
    Q_UNUSED(p)
    Q_UNUSED(action)
}

void ModuleDataObject::onAllPropertiesBlocked()
{
}

void ModuleDataObject::onAllPropertiesUnBlocked()
{
}

void ModuleDataObject::onPropertyBlocked(const DataProperty& p)
{
    Q_UNUSED(p)
}

void ModuleDataObject::onPropertyUnBlocked(const DataProperty& p)
{
    Q_UNUSED(p)
}

const zf::HighlightModel* ModuleDataObject::highlight(bool execute_check) const
{
    if (execute_check)
        _highlight_processor->executeHighlightCheckRequests();

    return _highlight_processor->highlight();
}

void ModuleDataObject::getHighlightAuto(const DataProperty& highlight_property, HighlightInfo& info) const
{
    info.checkProperty(highlight_property);

    info.blockCheckId();
    info.blockCheckCurrent();
    _highlight_processor->getHighlightAuto(highlight_property, info);
    info.unBlockCheckCurrent();
    info.unBlockCheckId();
}

void ModuleDataObject::blockHighlightAuto()
{
    _highlight_processor->blockHighlightAuto();
}

void ModuleDataObject::unBlockHighlightAuto()
{
    _highlight_processor->unBlockHighlightAuto();
}

void ModuleDataObject::clearHighlights()
{
    _highlight_processor->clearHighlights();
}

void ModuleDataObject::registerHighlightCheck(const DataProperty& property) const
{
    _highlight_processor->registerHighlightCheck(property);
}

void ModuleDataObject::registerHighlightCheck(const PropertyID& property_id) const
{
    _highlight_processor->registerHighlightCheck(property_id);
}

void ModuleDataObject::registerHighlightCheck(const DataPropertyList& properties) const
{
    for (auto& p : properties) {
        registerHighlightCheck(p);
    }
}

void ModuleDataObject::registerHighlightCheck(const PropertyIDList& property_ids) const
{
    for (auto& p : property_ids) {
        registerHighlightCheck(p);
    }
}

void ModuleDataObject::registerHighlightCheck(int row, const DataProperty& column, const QModelIndex& parent) const
{
    registerHighlightCheck(cellProperty(row, column, parent));
}

void ModuleDataObject::registerHighlightCheck(int row, const PropertyID& column, const QModelIndex& parent) const
{
    registerHighlightCheck(row, property(column), parent);
}

void ModuleDataObject::registerHighlightCheck(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    registerHighlightCheck(rowProperty(dataset, row, parent));
}

void ModuleDataObject::registerHighlightCheck(const PropertyID& dataset, int row, const QModelIndex& parent) const
{
    registerHighlightCheck(property(dataset), row, parent);
}

void ModuleDataObject::registerHighlightCheck() const
{
    _highlight_processor->registerHighlightCheck();
}

void ModuleDataObject::executeHighlightCheckRequests() const
{
    _highlight_processor->executeHighlightCheckRequests();
}

void ModuleDataObject::clearHighlightCheckRequests() const
{
    _highlight_processor->clearHighlightCheckRequests();
}

void ModuleDataObject::setHightlightCheckRequired(bool b)
{
    if (b) {
        if (!_highlight_processor->isStopped())
            return; // и так уже все запущено

        if (_master_highlight_processor != nullptr) {
            Z_CHECK(_highlight_processor->masterProcessor() == nullptr);
            // надо установить головной процессор, т.к. он был отключен при вызове setHightlightCheckRequired
            _highlight_processor->installMasterProcessor(_master_highlight_processor);
            // теоретически головной процессор может быть уже остановлен, но это уже не наши проблемы

        } else {
            _highlight_processor->start();
        }

    } else {
        if (_highlight_processor->isStopped())
            return; // и так уже все остановлено

        if (_highlight_processor->masterProcessor() != nullptr) {
            // отключаемся от головного процессора и останавливаем наш процессор
            _highlight_processor->removeMasterProcessor(
                // процессор был ранее остановлен при установке головного процессора и мы его просто не запускаем
                false);

        } else {
            _highlight_processor->stop();
        }
    }
}

bool ModuleDataObject::isHighlightViewIsCheckRequired() const
{
    return !_highlight_processor->isStopped();
}

QList<HighlightItem> ModuleDataObject::cellHighlight(const QModelIndex& index, bool execute_check, bool halt_if_not_found) const
{
    return _highlight_processor->cellHighlight(index, execute_check, halt_if_not_found);
}

void ModuleDataObject::invalidateUsingModels() const
{
    invalidate();
    data()->invalidateLookupModels();
}

void ModuleDataObject::getHighlight(const DataProperty& p, zf::HighlightInfo& info) const
{
    Q_UNUSED(p)
    Q_UNUSED(info)
}

void ModuleDataObject::checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent, QString& error_text, DataProperty& error_property) const
{
    Q_UNUSED(dataset);
    Q_UNUSED(row);
    Q_UNUSED(parent);
    Q_UNUSED(error_text);
    Q_UNUSED(error_property);
}

QString ModuleDataObject::keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent, const QVariantList& key_values) const
{
    Q_UNUSED(dataset);
    Q_UNUSED(row);
    Q_UNUSED(parent);
    return Utils::generateUniqueString(key_values);
}

bool ModuleDataObject::containsHighlight(const DataProperty& property) const
{
    return highlight()->contains(property);
}

bool ModuleDataObject::containsHighlight(const PropertyID& property_id) const
{
    return highlight()->contains(property(property_id));
}

bool ModuleDataObject::containsHighlight(const DataProperty& property, int id) const
{
    return highlight()->contains(property, id);
}

bool ModuleDataObject::containsHighlight(const PropertyID& property_id, int id) const
{
    return highlight()->contains(property(property_id), id);
}

bool ModuleDataObject::containsHighlight(const DataProperty& property, InformationType type) const
{
    return highlight()->contains(property, type);
}

bool ModuleDataObject::containsHighlight(const PropertyID& property_id, InformationType type) const
{
    return highlight()->contains(property(property_id), type);
}

void ModuleDataObject::installMasterHighlightProcessor(HighlightProcessor* master_processor)
{
    if (_master_highlight_processor == master_processor)
        return;

    _master_highlight_processor = master_processor;

    if (_highlight_processor->masterProcessor() != nullptr)
        _highlight_processor->removeMasterProcessor(false);

    if (master_processor != nullptr)
        _highlight_processor->installMasterProcessor(master_processor);
}

void ModuleDataObject::getFieldHighlight(const DataProperty& field, HighlightInfo& info) const
{
    Q_UNUSED(field)
    Q_UNUSED(info)
}

void ModuleDataObject::getDatasetHighlight(const DataProperty& dataset, HighlightInfo& info) const
{
    Q_UNUSED(dataset)
    Q_UNUSED(info)
}

void ModuleDataObject::getCellHighlight(const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent, HighlightInfo& info) const
{
    Q_UNUSED(dataset)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)
    Q_UNUSED(info)
}

bool ModuleDataObject::highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const
{
    return property1 < property2;
}

bool ModuleDataObject::isHighlightViewIsSortOrderInitialized() const
{
    return true;
}

DataStructure* ModuleDataObject::getDataStructure() const
{
    return structure().get();
}

DataContainer* ModuleDataObject::getDataContainer() const
{
    return data().get();
}

ModuleDataObject::DatasetInfo* ModuleDataObject::datasetInfo(const DataProperty& dataset, bool null_if_not_exists) const
{
    auto info = _dataset_info.value(dataset);
    if (info == nullptr) {
        if (null_if_not_exists)
            return nullptr;

        Z_CHECK(data()->contains(dataset));
        info = Z_MAKE_SHARED(DatasetInfo, const_cast<ModuleDataObject*>(this), dataset);
        _dataset_info[dataset] = info;
    }
    return info.get();
}

QVariant ModuleDataObject::value(const DataProperty& p, QLocale::Language language, bool from_lookup) const
{
    return data()->value(p, language, from_lookup);
}

QVariant ModuleDataObject::value(const PropertyID& property_id, QLocale::Language language, bool from_lookup) const
{
    return data()->value(property_id, language, from_lookup);
}

Numeric ModuleDataObject::toNumeric(const DataProperty& p) const
{
    return data()->toNumeric(p);
}

Numeric ModuleDataObject::toNumeric(const PropertyID& property_id) const
{
    return data()->toNumeric(property_id);
}

double ModuleDataObject::toDouble(const DataProperty& p) const
{
    return data()->toDouble(p);
}

double ModuleDataObject::toDouble(const PropertyID& property_id) const
{
    return data()->toDouble(property_id);
}

QString ModuleDataObject::toString(const DataProperty& p, LocaleType conv, QLocale::Language language) const
{
    return data()->toString(p, conv, language);
}

QString ModuleDataObject::toString(const PropertyID& property_id, LocaleType conv, QLocale::Language language) const
{
    return data()->toString(property_id, conv, language);
}

QDate ModuleDataObject::toDate(const DataProperty& p) const
{
    return data()->toDate(p);
}

QDate ModuleDataObject::toDate(const PropertyID& property_id) const
{
    return data()->toDate(property_id);
}

QDateTime ModuleDataObject::toDateTime(const DataProperty& p) const
{
    return data()->toDateTime(p);
}

QDateTime ModuleDataObject::toDateTime(const PropertyID& property_id) const
{
    return data()->toDateTime(property_id);
}

QTime ModuleDataObject::toTime(const DataProperty& p) const
{
    return data()->toTime(p);
}

QTime ModuleDataObject::toTime(const PropertyID& property_id) const
{
    return data()->toTime(property_id);
}

Uid ModuleDataObject::toUid(const DataProperty& p) const
{
    return data()->toUid(p);
}

Uid ModuleDataObject::toUid(const PropertyID& property_id) const
{
    return data()->toUid(property_id);
}

int ModuleDataObject::toInt(const DataProperty& p, int default_value) const
{
    return data()->toInt(p, default_value);
}

int ModuleDataObject::toInt(const PropertyID& property_id, int default_value) const
{
    return data()->toInt(property_id, default_value);
}

bool ModuleDataObject::toBool(const DataProperty& p) const
{
    return data()->toBool(p);
}

bool ModuleDataObject::toBool(const PropertyID& property_id) const
{
    return data()->toBool(property_id);
}

EntityID ModuleDataObject::toEntityID(const DataProperty& p) const
{
    return data()->toEntityID(p);
}

EntityID ModuleDataObject::toEntityID(const PropertyID& property_id) const
{
    return data()->toEntityID(property_id);
}

InvalidValue ModuleDataObject::toInvalidValue(const DataProperty& p) const
{
    return data()->toInvalidValue(p);
}

InvalidValue ModuleDataObject::toInvalidValue(const PropertyID& property_id) const
{
    return data()->toInvalidValue(property_id);
}

QByteArray ModuleDataObject::toByteArray(const DataProperty& p) const
{
    return data()->toByteArray(p);
}

QByteArray ModuleDataObject::toByteArray(const PropertyID& property_id) const
{
    return data()->toByteArray(property_id);
}

bool ModuleDataObject::isNull(const DataProperty& p, QLocale::Language language) const
{
    return data()->isNull(p, language);
}

bool ModuleDataObject::isNull(const PropertyID& property_id, QLocale::Language language) const
{
    return data()->isNull(property_id, language);
}

bool ModuleDataObject::isNullCell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return data()->isNullCell(row, column, role, parent, language);
}

bool ModuleDataObject::isNullCell(int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return data()->isNullCell(row, column_property_id, role, parent, language);
}

bool ModuleDataObject::isNullCell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language) const
{
    return data()->isNullCell(row_id, column, role, language);
}

bool ModuleDataObject::isNullCell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language) const
{
    return data()->isNullCell(row_id, column_property_id, role, language);
}

bool ModuleDataObject::isNullCell(const DataProperty& cell_property, int role, QLocale::Language language) const
{
    return data()->isNullCell(cell_property, role, language);
}

bool ModuleDataObject::isInvalidValue(const DataProperty& p, QLocale::Language language) const
{
    return data()->isInvalidValue(p, language);
}

bool ModuleDataObject::isInvalidValue(const PropertyID& property_id, QLocale::Language language) const
{
    return data()->isInvalidValue(property_id, language);
}

bool ModuleDataObject::isInvalidCell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return data()->isInvalidCell(row, column, role, parent, language);
}

bool ModuleDataObject::isInvalidCell(int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return data()->isInvalidCell(row, column_property_id, role, parent, language);
}

bool ModuleDataObject::isInvalidCell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language) const
{
    return data()->isInvalidCell(row_id, column, role, language);
}

bool ModuleDataObject::isInvalidCell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language) const
{
    return data()->isInvalidCell(row_id, column_property_id, role, language);
}

Error ModuleDataObject::clearValue(const DataProperty& p)
{
    return data()->clearValue(p);
}

Error ModuleDataObject::clearValue(const PropertyID& property_id)
{
    return data()->clearValue(property_id);
}

Error ModuleDataObject::setValue(const DataProperty& p, const QVariant& v, QLocale::Language language)
{
    return data()->setValue(p, v, language);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const QVariant& v, QLocale::Language language)
{
    return data()->setValue(property_id, v, language);
}

Error ModuleDataObject::setValue(const DataProperty& p, const Numeric& v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const Numeric& v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, const QByteArray& v, QLocale::Language language)
{
    return data()->setValue(p, v, language);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const QByteArray& v, QLocale::Language language)
{
    return data()->setValue(property_id, v, language);
}

Error ModuleDataObject::setValue(const DataProperty& p, const QString& v, QLocale::Language language)
{
    return data()->setValue(p, v, language);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const QString& v, QLocale::Language language)
{
    return data()->setValue(property_id, v, language);
}

Error ModuleDataObject::setValue(const DataProperty& p, const char* v, QLocale::Language language)
{
    return data()->setValue(p, v, language);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const char* v, QLocale::Language language)
{
    return data()->setValue(property_id, v, language);
}

Error ModuleDataObject::setValue(const DataProperty& p, bool v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, bool v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, int v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, int v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, uint v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, uint v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, qint64 v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, qint64 v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, quint64 v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, quint64 v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, double v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, double v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, float v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, float v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, const Uid& v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const Uid& v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, const ID_Wrapper& v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const ID_Wrapper& v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, const LanguageMap& values)
{
    return data()->setValue(p, values);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const LanguageMap& values)
{
    return data()->setValue(property_id, values);
}

Error ModuleDataObject::setValue(const DataProperty& p, const DataProperty& v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const DataProperty& v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setValue(const DataProperty& p, const PropertyID& v)
{
    return data()->setValue(p, v);
}

Error ModuleDataObject::setValue(const PropertyID& property_id, const PropertyID& v)
{
    return data()->setValue(property_id, v);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const QVariant& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column, value, role, parent, language);
}

Error ModuleDataObject::setCell(
    int row, const PropertyID& column_property_id, const QVariant& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column_property_id, value, role, parent, language);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const char* value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column, value, role, parent, language);
}

Error ModuleDataObject::setCell(
    int row, const PropertyID& column_property_id, const char* value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column_property_id, value, role, parent, language);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const QString& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column, value, role, parent, language);
}

Error ModuleDataObject::setCell(
    int row, const PropertyID& column_property_id, const QString& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return data()->setCell(row, column_property_id, value, role, parent, language);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const Uid& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, const Uid& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const Numeric& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, const Numeric& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const ID_Wrapper& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, const ID_Wrapper& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, bool value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, bool value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, int value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, int value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, uint value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, uint value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, qint64 value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, qint64 value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, quint64 value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, quint64 value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, double value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, double value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, float value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, float value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const DataProperty& column, const LanguageMap& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column, value, role, parent);
}

Error ModuleDataObject::setCell(int row, const PropertyID& column_property_id, const LanguageMap& value, int role, const QModelIndex& parent)
{
    return data()->setCell(row, column_property_id, value, role, parent);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const QVariant& value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const QVariant& value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column_property_id, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const char* value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const char* value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column_property_id, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const QString& value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const QString& value, int role, QLocale::Language language)
{
    return data()->setCell(row_id, column_property_id, value, role, language);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const Uid& value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const Uid& value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const ID_Wrapper& value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const ID_Wrapper& value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, bool value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, bool value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, int value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, int value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, uint value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, uint value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, qint64 value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, qint64 value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, quint64 value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, quint64 value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, double value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, double value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, float value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, float value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const DataProperty& column, const LanguageMap& value, int role)
{
    return data()->setCell(row_id, column, value, role);
}

Error ModuleDataObject::setCell(const RowID& row_id, const PropertyID& column_property_id, const LanguageMap& value, int role)
{
    return data()->setCell(row_id, column_property_id, value, role);
}

QVariant ModuleDataObject::cell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(row, column, role, parent, language, from_lookup);
}

QVariant ModuleDataObject::cell(
    int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(row, column_property_id, role, parent, language, from_lookup);
}

QVariant ModuleDataObject::cell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(row_id, column, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(row_id, column_property_id, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(const DataProperty& row_property, const DataProperty& column, int role, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(row_property.propertyType() == PropertyType::RowFull);
    return cell(row_property.rowId(), column, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(
    const DataProperty& row_property, const PropertyID& column_property_id, int role, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(row_property.propertyType() == PropertyType::RowFull);
    return cell(row_property.rowId(), column_property_id, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(const DataProperty& cell_property, int role, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(cell_property, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(const QModelIndex& index, const DataProperty& column, int role, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(index, column, role, language, from_lookup);
}

QVariant ModuleDataObject::cell(const QModelIndex& index, const PropertyID& column_property_id, int role, QLocale::Language language, bool from_lookup) const
{
    return data()->cell(index, column_property_id, role, language, from_lookup);
}

int ModuleDataObject::rowCount(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    return data()->rowCount(dataset_property, parent);
}

int ModuleDataObject::rowCount(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return data()->rowCount(dataset_property_id, parent);
}

int ModuleDataObject::rowCount(const QModelIndex& parent) const
{
    return data()->rowCount(parent);
}

void ModuleDataObject::setRowCount(const DataProperty& dataset_property, int count, const QModelIndex& parent)
{
    return data()->setRowCount(dataset_property, count, parent);
}

void ModuleDataObject::setRowCount(const PropertyID& dataset_property_id, int count, const QModelIndex& parent)
{
    return data()->setRowCount(dataset_property_id, count, parent);
}

void ModuleDataObject::setRowCount(int count, const QModelIndex& parent)
{
    return data()->setRowCount(count, parent);
}

void ModuleDataObject::setColumnCount(const DataProperty& dataset_property, int count)
{
    return data()->setColumnCount(dataset_property, count);
}

void ModuleDataObject::setColumnCount(const PropertyID& dataset_property_id, int count)
{
    return data()->setColumnCount(dataset_property_id, count);
}

void ModuleDataObject::setColumnCount(int count)
{
    return data()->setColumnCount(count);
}

int ModuleDataObject::columnCount(const DataProperty& dataset_property) const
{
    return data()->columnCount(dataset_property);
}

int ModuleDataObject::columnCount(const PropertyID& dataset_property_id) const
{
    return data()->columnCount(dataset_property_id);
}

int ModuleDataObject::columnCount() const
{
    return data()->columnCount();
}

int ModuleDataObject::appendRow(const DataProperty& dataset_property, const QModelIndex& parent, int num)
{
    return data()->appendRow(dataset_property, parent, num);
}

int ModuleDataObject::appendRow(const PropertyID& dataset_property_id, const QModelIndex& parent, int num)
{
    return data()->appendRow(dataset_property_id, parent, num);
}

int ModuleDataObject::appendRow(const QModelIndex& parent, int num)
{
    return data()->appendRow(parent, num);
}

int ModuleDataObject::insertRow(const DataProperty& dataset_property, int row, const QModelIndex& parent, int num)
{
    return data()->insertRow(dataset_property, row, parent, num);
}

int ModuleDataObject::insertRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent, int num)
{
    return data()->insertRow(dataset_property_id, row, parent, num);
}

int ModuleDataObject::insertRow(int row, const QModelIndex& parent, int num)
{
    return data()->insertRow(row, parent, num);
}

void ModuleDataObject::removeRow(const DataProperty& dataset_property, int row, const QModelIndex& parent, int num)
{
    data()->removeRow(dataset_property, row, parent, num);
}

void ModuleDataObject::removeRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent, int num)
{
    data()->removeRow(dataset_property_id, row, parent, num);
}

void ModuleDataObject::removeRow(int row, const QModelIndex& parent, int num)
{
    data()->removeRow(row, parent, num);
}

void ModuleDataObject::removeRows(const DataProperty& dataset_property, const Rows& rows)
{
    data()->removeRows(dataset_property, rows);
}

void ModuleDataObject::removeRows(const PropertyID& dataset_property_id, const Rows& rows)
{
    data()->removeRows(dataset_property_id, rows);
}

void ModuleDataObject::removeRows(const Rows& rows)
{
    data()->removeRows(rows);
}

void ModuleDataObject::moveRows(const DataProperty& dataset_property, int source_row, const QModelIndex& source_parent, int count, int destination_row,
    const QModelIndex& destination_parent)
{
    data()->moveRows(dataset_property, source_row, source_parent, count, destination_row, destination_parent);
}

void ModuleDataObject::moveRows(const PropertyID& dataset_property_id, int source_row, const QModelIndex& source_parent, int count, int destination_row,
    const QModelIndex& destination_parent)
{
    data()->moveRows(dataset_property_id, source_row, source_parent, count, destination_row, destination_parent);
}

void ModuleDataObject::moveRows(const DataProperty& dataset_property, int source_row, int count, int destination_row)
{
    data()->moveRows(dataset_property, source_row, count, destination_row);
}

void ModuleDataObject::moveRows(const PropertyID& dataset_property_id, int source_row, int count, int destination_row)
{
    data()->moveRows(dataset_property_id, source_row, count, destination_row);
}

void ModuleDataObject::moveRow(const DataProperty& dataset_property, int source_row, int destination_row)
{
    data()->moveRow(dataset_property, source_row, destination_row);
}

void ModuleDataObject::moveRow(const PropertyID& dataset_property_id, int source_row, int destination_row)
{
    data()->moveRow(dataset_property_id, source_row, destination_row);
}

QModelIndex ModuleDataObject::cellIndex(int row, const DataProperty& column, const QModelIndex& parent) const
{
    return data()->cellIndex(row, column, parent);
}

QModelIndex ModuleDataObject::cellIndex(int row, const PropertyID& column_property_id, const QModelIndex& parent) const
{
    return data()->cellIndex(row, column_property_id, parent);
}

QModelIndex ModuleDataObject::cellIndex(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent) const
{
    return data()->cellIndex(dataset_property, row, column, parent);
}

QModelIndex ModuleDataObject::cellIndex(const PropertyID& dataset_property_id, int row, int column, const QModelIndex& parent) const
{
    return data()->cellIndex(dataset_property_id, row, column, parent);
}

QModelIndex ModuleDataObject::rowIndex(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    return data()->rowIndex(dataset_property, row, parent);
}

QModelIndex ModuleDataObject::rowIndex(const PropertyID& dataset_property_id, int row, const QModelIndex& parent) const
{
    return data()->rowIndex(dataset_property_id, row, parent);
}

DataProperty ModuleDataObject::rowProperty(const DataProperty& dataset, const RowID& row_id) const
{
    return data()->rowProperty(dataset, row_id);
}

DataProperty ModuleDataObject::rowProperty(const PropertyID& dataset_id, const RowID& row_id) const
{
    return data()->rowProperty(dataset_id, row_id);
}

DataProperty ModuleDataObject::rowProperty(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    return rowProperty(dataset, datasetRowID(dataset, row, parent));
}

DataProperty ModuleDataObject::rowProperty(const PropertyID& dataset_id, int row, const QModelIndex& parent) const
{
    return data()->rowProperty(dataset_id, row, parent);
}

DataProperty ModuleDataObject::cellProperty(const DataProperty& property, const PropertyID& column_id) const
{
    return cellProperty(property, this->property(column_id));
}

DataProperty ModuleDataObject::cellProperty(const DataProperty& property, const DataProperty& column) const
{
    Z_CHECK(property.isRow() || property.isCell());
    return cellProperty(property.rowId(), column);
}

DataProperty ModuleDataObject::cellProperty(const RowID& row_id, const PropertyID& column_id) const
{
    return cellProperty(row_id, property(column_id));
}

DataProperty ModuleDataObject::cellProperty(const RowID& row_id, const DataProperty& column) const
{
    return DataStructure::propertyCell(row_id, column);
}

DataProperty ModuleDataObject::cellProperty(int row, const PropertyID& column, const QModelIndex& parent) const
{
    return cellProperty(datasetRowID(property(column).dataset(), row, parent), column);
}

DataProperty ModuleDataObject::cellProperty(int row, const DataProperty& column, const QModelIndex& parent) const
{
    return DataStructure::propertyCell(datasetRowID(column.dataset(), row, parent), column);
}

Error ModuleDataObject::initAllValues()
{
    return data()->initAllValues();
}

Error ModuleDataObject::initValue(const DataProperty& p)
{
    return data()->initValue(p);
}

Error ModuleDataObject::initValue(const PropertyID& property_id)
{
    return data()->initValue(property_id);
}

Error ModuleDataObject::resetValue(const DataProperty& p)
{
    return data()->resetValue(p);
}

Error ModuleDataObject::resetValue(const PropertyID& property_id)
{
    return data()->resetValue(property_id);
}

void ModuleDataObject::unInitialize(const DataProperty& p)
{
    data()->unInitialize(p);
}

void ModuleDataObject::unInitialize(const PropertyID& property_id)
{
    data()->unInitialize(property_id);
}

void ModuleDataObject::initDataset(const DataProperty& dataset_property, int row_count)
{
    data()->initDataset(dataset_property, row_count);
}

void ModuleDataObject::initDataset(const PropertyID& property_id, int row_count)
{
    data()->initDataset(property_id, row_count);
}

void ModuleDataObject::initDataset(int row_count)
{
    data()->initDataset(row_count);
}

ItemModel* ModuleDataObject::dataset(const DataProperty& dataset_property) const
{
    return data()->dataset(dataset_property);
}

ItemModel* ModuleDataObject::dataset(const PropertyID& dataset_property_id) const
{
    return data()->dataset(dataset_property_id);
}

ItemModel* ModuleDataObject::dataset() const
{
    return data()->dataset();
}

DataProperty ModuleDataObject::datasetIdColumn(const DataProperty& dataset_property) const
{
    return datasetInfo(dataset_property, false)->id_column_property;
}

DataProperty ModuleDataObject::datasetIdColumn(const PropertyID& dataset_property_id) const
{
    return datasetIdColumn(property(dataset_property_id));
}

Rows ModuleDataObject::makeRows(const DataProperty& p, const QList<int>& rows, const QModelIndex& parent) const
{
    QList<int> rows_sorted = rows;
    std::sort(rows_sorted.begin(), rows_sorted.end());
    QModelIndexList indexes;
    auto dataset = data()->dataset(p);
    for (int row : rows) {
        QModelIndex index = dataset->index(row, 0, parent);
        Z_CHECK_X(index.isValid(), QString("%1 - row not found (%2)").arg(entityCode().value()).arg(row));
        indexes << index;
    }

    return Rows(indexes);
}

Rows ModuleDataObject::makeRows(const DataProperty& p, const QSet<int>& rows, const QModelIndex& parent) const
{
    return makeRows(p, rows.values(), parent);
}

Rows ModuleDataObject::makeRows(const PropertyID& property_id, const QList<int>& rows, const QModelIndex& parent) const
{
    return makeRows(property(property_id), rows, parent);
}

Rows ModuleDataObject::makeRows(const PropertyID& property_id, const QSet<int>& rows, const QModelIndex& parent) const
{
    return makeRows(property(property_id), rows, parent);
}

Rows ModuleDataObject::makeRows(const QList<int>& rows, const QModelIndex& parent) const
{
    return makeRows(structure()->singleDatasetId(), rows, parent);
}

Rows ModuleDataObject::makeRows(const QSet<int>& rows, const QModelIndex& parent) const
{
    return makeRows(structure()->singleDatasetId(), rows, parent);
}

QVariantList ModuleDataObject::getColumnValues(const DataProperty& column, const QList<RowID>& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const DataProperty& column, const QModelIndexList& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const DataProperty& column, const QList<int>& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const PropertyID& column, const QList<RowID>& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const PropertyID& column, const QModelIndexList& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const PropertyID& column, const QList<int>& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

QVariantList ModuleDataObject::getColumnValues(const PropertyID& column, const Rows& rows, int role, bool valid_only) const
{
    return data()->getColumnValues(column, rows, role, nullptr, valid_only);
}

void ModuleDataObject::blockAllProperties() const
{
    data()->blockAllProperties();
}

void ModuleDataObject::unBlockAllProperties() const
{
    data()->unBlockAllProperties();
}

void ModuleDataObject::blockProperty(const DataProperty& p) const
{
    data()->blockProperty(p);
}

void ModuleDataObject::unBlockProperty(const DataProperty& p) const
{
    data()->unBlockProperty(p);
}

bool ModuleDataObject::isAllPropertiesBlocked() const
{
    return data()->isAllPropertiesBlocked();
}

bool ModuleDataObject::isPropertyBlocked(const DataProperty& p) const
{
    return data()->isPropertyBlocked(p);
}

void ModuleDataObject::registerTrackingChanges_rowRemoving(const DataProperty& dataset_property, int first, int last, const QModelIndex& parent)
{
    if (_tracking_info.isEmpty())
        return;

    auto info = datasetInfo(dataset_property, false);

    for (int row = first; row <= last; row++) {
        DataProperty row_prop = DataStructure::propertyRow(dataset_property, datasetRowID(dataset_property, row, parent));

        for (auto it = info->tracking.begin(); it != info->tracking.end(); it++) {
            if (it.value()->new_rows.contains(row_prop))
                continue;

            // удаляем измененные ячейки для данной строки
            auto cells = it.value()->modified_cells;
            for (const auto& c : cells) {
                if (c.row() == row_prop)
                    it.value()->modified_cells.remove(c);
            }

            it.value()->removed_rows.insert(row_prop);
        }
    }
}

void ModuleDataObject::registerTrackingChanges_rowAdded(const DataProperty& dataset_property, int first, int last, const QModelIndex& parent)
{
    if (_tracking_info.isEmpty())
        return;

    auto info = datasetInfo(dataset_property, false);

    for (int row = first; row <= last; row++) {
        DataProperty row_prop = DataStructure::propertyRow(dataset_property, datasetRowID(dataset_property, row, parent));

        for (auto it = info->tracking.begin(); it != info->tracking.end(); it++) {
            it.value()->new_rows.insert(row_prop);
        }
    }
}

void ModuleDataObject::registerTrackingChanges_cellChanged(int row, const DataProperty& column_property, const QModelIndex& parent)
{
    if (_tracking_info.isEmpty())
        return;

    auto info = datasetInfo(column_property.dataset(), false);
    DataProperty cell_prop = DataStructure::propertyCell(datasetRowID(column_property.dataset(), row, parent), column_property);

    for (auto it = info->tracking.begin(); it != info->tracking.end(); it++) {
        if (it.value()->new_rows.contains(cell_prop.row()))
            continue;

        it.value()->modified_cells.insert(cell_prop);
    }
}

TrackingID ModuleDataObject::startTracking(const zf::DataProperty& dataset_property)
{
    Z_CHECK(datasetIdColumn(dataset_property).isValid());

    auto id = TrackingID::generate();
    auto info = datasetInfo(dataset_property, false);

    auto tracking_info = Z_MAKE_SHARED(TrackingInfo);
    tracking_info->dataset_info = info;

    info->tracking[id] = tracking_info;
    _tracking_info[id] = tracking_info;
    _tracking_info_by_dataset.insert(dataset_property, tracking_info);

    return id;
}

TrackingID ModuleDataObject::startTracking(const PropertyID& dataset_property_id)
{
    return startTracking(property(dataset_property_id));
}

void ModuleDataObject::stopTracking(const TrackingID& id)
{
    Z_CHECK(id.isValid());

    auto info = _tracking_info.value(id);
    Z_CHECK_X(info != nullptr, "tracking id not found");

    info->removed_rows.clear();
    info->new_rows.clear();
    info->modified_cells.clear();
    info->dataset_info->tracking.clear();

    Z_CHECK(_tracking_info.remove(id));
    Z_CHECK(_tracking_info_by_dataset.remove(info->dataset_info->dataset, info));
}

bool ModuleDataObject::isTracking(const TrackingID& id) const
{
    Z_CHECK(id.isValid());
    return _tracking_info.contains(id);
}

QList<TrackingID> ModuleDataObject::trackingIDs() const
{
    return _tracking_info.keys();
}

DataPropertyList ModuleDataObject::trackingDatasets() const
{
    return _tracking_info_by_dataset.keys();
}

DataPropertyList ModuleDataObject::trackingRemovedRows(const TrackingID& id) const
{
    auto info = _tracking_info.value(id);
    Z_CHECK_NULL(info);
    return info->removed_rows.values();
}

DataPropertyList ModuleDataObject::trackingNewRows(const TrackingID& id) const
{
    auto info = _tracking_info.value(id);
    Z_CHECK_NULL(info);
    return info->new_rows.values();
}

DataPropertyList ModuleDataObject::trackingModifiedCells(const TrackingID& id) const
{
    auto info = _tracking_info.value(id);
    Z_CHECK_NULL(info);
    return info->modified_cells.values();
}

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
void ModuleDataObject::debugTracking() const
{
    qDebug() << "============= Tracking info =============";

    for (const auto& info : qAsConst(_tracking_info_by_dataset)) {
        qDebug() << "Dataset:" << info->dataset_info->dataset.id();

        qDebug() << "****** removed rows:";
        for (const auto& row : qAsConst(info->removed_rows)) {
            qDebug() << row.rowId();
        }
        qDebug() << "****** new rows:";
        for (const auto& row : qAsConst(info->new_rows)) {
            qDebug() << row.rowId();
        }
        qDebug() << "****** modified cells:";
        for (const auto& cell : qAsConst(info->modified_cells)) {
            qDebug() << cell.rowId() << cell.column().id();
        }
    }

    qDebug() << "========================================";
}
#endif

HighlightProcessor* ModuleDataObject::highlightProcessor() const
{
    return _highlight_processor;
}

HighlightProcessor* ModuleDataObject::masterHighlightProcessor() const
{
    return _master_highlight_processor;
}

ModuleDataObject::DatasetInfo::DatasetInfo(ModuleDataObject* d_object, const DataProperty& ds)
    : data_object(d_object)
    , dataset(ds)
{
    auto id_columns = data_object->structure()->datasetColumnsByOptions(ds, PropertyOption::Id);
    if (!id_columns.isEmpty()) {
        if (id_columns.count() > 1) {
            QString err_string = QString("id column duplication: %1:%2").arg(d_object->entityCode().value()).arg(ds.id().value());
#if DEBUG_BUILD()
            Z_HALT(err_string);
#endif
            Core::logError(err_string); //-V779
        }
        id_column = dataset.columnPos(id_columns.constFirst());
        id_column_property = id_columns.constFirst();
    }
}

ItemModel* ModuleDataObject::DatasetInfo::item_model() const
{
    // _item_model может обнулиться при перезагрузке данных
    if (_item_model == nullptr)
        _item_model = data_object->data()->dataset(dataset);
    return _item_model;
}

TrackingID::TrackingID()
{
}

TrackingID::TrackingID(int id)
    : Handle<int>(id)
{
}

TrackingID::TrackingID(const Handle<int>& h)
    : Handle<int>(h)
{
}

} // namespace zf
