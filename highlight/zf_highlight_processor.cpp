#include "zf_highlight_processor.h"
#include "zf_core.h"
#include "zf_framework.h"

//! Код группы highlight для ошибок с совпадающими ключевыми значениями
#define KEY_ERROR_GROUP_CODE std::numeric_limits<int>::max()

namespace zf
{
HighlightProcessor::HighlightProcessor(const DataContainer* data, I_HighlightProcessorKeyValues* i_key_values, bool simple)
    : QObject()
    , _highlight(new HighlightModel(this))
    , _data(data)
    , _i_key_values(i_key_values)
    , _is_simple(simple)
    , _object_extension(new ObjectExtension(this))
{
    bootstrap();
}

HighlightProcessor::~HighlightProcessor()
{
    delete _object_extension;
}

bool HighlightProcessor::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void HighlightProcessor::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant HighlightProcessor::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool HighlightProcessor::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void HighlightProcessor::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void HighlightProcessor::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void HighlightProcessor::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void HighlightProcessor::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void HighlightProcessor::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

const HighlightModel* HighlightProcessor::highlight() const
{
    return _master_processor ? _master_processor->highlight() : _highlight;
}

const DataContainer* HighlightProcessor::data() const
{
    return _master_processor ? _master_processor->data() : _data;
}

const DataStructure* HighlightProcessor::structure() const
{
    return _master_processor ? _master_processor->structure() : _data->structure().get();
}

bool HighlightProcessor::isStopped() const
{
    return _master_processor ? _master_processor->isStopped() : isStoppedHelper();
}

void HighlightProcessor::stop()
{
    if (_master_processor != nullptr)
        _master_processor->stop();
    else
        stopHelper();
}

void HighlightProcessor::start()
{
    if (_master_processor != nullptr)
        _master_processor->start();
    else
        startHelper();
}

void HighlightProcessor::stopHelper()
{
    _stop_counter++;
    if (_stop_counter == 1) {
        disconnect(_data, &DataContainer::sg_invalidateChanged, this, &HighlightProcessor::sl_invalidateChanged);
        disconnect(_data, &DataContainer::sg_allPropertiesUnBlocked, this, &HighlightProcessor::sl_allPropertiesUnBlocked);
        disconnect(_data, &DataContainer::sg_propertyUnBlocked, this, &HighlightProcessor::sl_propertyUnBlocked);
        disconnect(_data, &DataContainer::sg_propertyChanged, this, &HighlightProcessor::sl_propertyChanged);

        disconnect(_data, &DataContainer::sg_dataset_dataChanged, this, &HighlightProcessor::sl_dataset_dataChanged);
        disconnect(_data, &DataContainer::sg_dataset_rowsInserted, this, &HighlightProcessor::sl_dataset_rowsInserted);
        disconnect(_data, &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &HighlightProcessor::sl_dataset_rowsAboutToBeRemoved);
        disconnect(_data, &DataContainer::sg_dataset_rowsRemoved, this, &HighlightProcessor::sl_dataset_rowsRemoved);
        disconnect(_data, &DataContainer::sg_dataset_columnsInserted, this, &HighlightProcessor::sl_dataset_columnsInserted);
        disconnect(_data, &DataContainer::sg_dataset_columnsAboutToBeRemoved, this, &HighlightProcessor::sl_dataset_columnsAboutToBeRemoved);
        disconnect(_data, &DataContainer::sg_dataset_columnsRemoved, this, &HighlightProcessor::sl_dataset_columnsRemoved);
        disconnect(_data, &DataContainer::sg_dataset_modelReset, this, &HighlightProcessor::sl_dataset_modelReset);

        clearHighlightsHelper();
    }
}

void HighlightProcessor::startHelper()
{
    _stop_counter--;
    Z_CHECK(_stop_counter >= 0);
    if (_stop_counter == 0) {
        connect(_data, &DataContainer::sg_invalidateChanged, this, &HighlightProcessor::sl_invalidateChanged);
        connect(_data, &DataContainer::sg_allPropertiesUnBlocked, this, &HighlightProcessor::sl_allPropertiesUnBlocked);
        connect(_data, &DataContainer::sg_propertyUnBlocked, this, &HighlightProcessor::sl_propertyUnBlocked);
        connect(_data, &DataContainer::sg_propertyChanged, this, &HighlightProcessor::sl_propertyChanged);

        connect(_data, &DataContainer::sg_dataset_dataChanged, this, &HighlightProcessor::sl_dataset_dataChanged);
        connect(_data, &DataContainer::sg_dataset_rowsInserted, this, &HighlightProcessor::sl_dataset_rowsInserted);
        connect(_data, &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &HighlightProcessor::sl_dataset_rowsAboutToBeRemoved);
        connect(_data, &DataContainer::sg_dataset_rowsRemoved, this, &HighlightProcessor::sl_dataset_rowsRemoved);
        connect(_data, &DataContainer::sg_dataset_columnsInserted, this, &HighlightProcessor::sl_dataset_columnsInserted);
        connect(_data, &DataContainer::sg_dataset_columnsAboutToBeRemoved, this, &HighlightProcessor::sl_dataset_columnsAboutToBeRemoved);
        connect(_data, &DataContainer::sg_dataset_columnsRemoved, this, &HighlightProcessor::sl_dataset_columnsRemoved);
        connect(_data, &DataContainer::sg_dataset_modelReset, this, &HighlightProcessor::sl_dataset_modelReset);

        registerHighlightCheck();
    }
}

void HighlightProcessor::registerHighlightCheck(const DataProperty& property)
{
    if (objectExtensionDestroyed())
        return;

    if (!_master_processor.isNull()) {
        _master_processor->registerHighlightCheck(property);
        return;
    }

    if (isStoppedHelper())
        return;

    Z_CHECK(property.isValid());
    Z_CHECK_X(property.entity() == _structure->entity(), "Попытка добавить проверку по другому модулю");
    _registered_highlight_check << property;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_PROCESSOR_CHECK_CALLBACK_KEY);
}

void HighlightProcessor::registerHighlightCheck(const PropertyID& property_id)
{
    if (!_master_processor.isNull()) {
        _master_processor->registerHighlightCheck(property_id);
        return;
    }

    if (isStoppedHelper())
        return;

    registerHighlightCheck(_structure->property(property_id));
}

void HighlightProcessor::registerHighlightCheck()
{
    if (!_master_processor.isNull()) {
        _master_processor->registerHighlightCheck();
        return;
    }

    if (isStoppedHelper())
        return;

    _highlight->clear();
    registerHighlightCheck(_structure->entity());
}

void HighlightProcessor::registerDatasetRowKeyDuplicateCheck(const DataProperty& dataset)
{
    if (!_master_processor.isNull()) {
        _master_processor->registerDatasetRowKeyDuplicateCheck(dataset);
        return;
    }

    if (isStoppedHelper())
        return;

    // проверяем уже существующие строки с дубликатами
    auto records = _highlight->groupItems(KEY_ERROR_GROUP_CODE);
    for (auto i = records.constBegin(); i != records.constEnd(); ++i) {
        Z_CHECK(i->property().propertyType() == PropertyType::Cell);
        if (i->property().dataset() != dataset)
            continue;

        QModelIndex check_idx = data()->findDatasetRowID(dataset, i->property().rowId());
        if (!check_idx.isValid())
            continue;
        registerHighlightCheck(data()->propertyRow(dataset, check_idx.row(), check_idx.parent()));
    }
}

void HighlightProcessor::executeHighlightCheckRequests()
{
    if (objectExtensionDestroyed())
        return;

    if (!_master_processor.isNull()) {
        _master_processor->executeHighlightCheckRequests();
        return;
    }

    if (isStoppedHelper())
        return;

    Framework::internalCallbackManager()->removeRequest(this, Framework::HIGHLIGHT_PROCESSOR_CHECK_CALLBACK_KEY);

    if (_registered_highlight_check.isEmpty())
        return;

    DataPropertyList to_check;

    // сначала необходимо исключить перекрывающие друг друга свойства
    QMultiMap<PropertyType, DataProperty> by_type;
    for (auto& p : qAsConst(_registered_highlight_check)) {
        by_type.insert(p.propertyType(), p);
    }

    DataPropertyList entity_prop = by_type.values(PropertyType::Entity);
    if (!entity_prop.isEmpty()) {
        // проверка модели в целом
        to_check << entity_prop.first();

    } else {
        DataPropertyList dataset_prop = by_type.values(PropertyType::Dataset);
        to_check << dataset_prop;

        DataPropertySet row_prop;
        DataPropertySet column_prop;
        DataPropertyList cell_prop;
        // ищем запросы на проверки по строк и колонок по другим наборам данных
        for (auto& p : qAsConst(_registered_highlight_check)) {
            if (p.propertyType() == PropertyType::Field) {
                to_check << p;
                continue;
            }

            if (p.propertyType() == PropertyType::Cell) {
                if (!dataset_prop.contains(p.dataset()))
                    cell_prop << p;
                continue;
            }

            if (p.isRow()) {
                if (!dataset_prop.contains(p.dataset())) {
                    row_prop << p;
                    to_check << p;
                }
                continue;
            }

            if (p.isColumn()) {
                if (!dataset_prop.contains(p.dataset())) {
                    column_prop << p;
                    to_check << p;
                }
                continue;
            }
        }

        // проверяем ячейки на необходимость проверки
        for (auto& p : qAsConst(cell_prop)) {
            if (column_prop.contains(p.column()) && row_prop.contains(p.row()))
                continue;
            to_check << p;
        }
    }

    _registered_highlight_check.clear();

    HighlightInfo info(_structure);
    DataPropertySet direct_datasets;
    DataPropertySet checked_datasets;
    for (auto& p : qAsConst(to_check)) {
        Z_CHECK(p.isValid());
        if (!data()->containsProperty(p) || !data()->isInitialized(p))
            continue;

        Z_CHECK(p.propertyType() == PropertyType::Entity || p.propertyType() == PropertyType::Dataset || p.propertyType() == PropertyType::RowFull
                || p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Cell);

        if (p.propertyType() == PropertyType::Dataset)
            checked_datasets << p;

        for (auto hp : qAsConst(_i_processors)) {
            getHighlight(hp, p, info, direct_datasets);
        }
    }

    if (_is_simple) {
        // смотрим какие наборы данных надо проверить отдельно
        for (auto& p : qAsConst(direct_datasets)) {
            if (checked_datasets.contains(p))
                continue;
            for (auto hp : qAsConst(_i_processors)) {
                info.setCurrentProperty(p);
                hp->getDatasetHighlight(p, info);
                info.setCurrentProperty();
            }
        }
    }

    if (_block_highlight_auto == 0) {
        // автоматические проверки выполняем в конце
        for (auto& p : qAsConst(to_check)) {
            Z_CHECK(p.isValid());
            if (!data()->containsProperty(p) || !data()->isInitialized(p))
                continue;

            info.blockCheckId();
            getHighlightAuto(p, info);
            info.unBlockCheckId();
        }
    }

    DataPropertySet all_properties = Utils::toSet<DataProperty>(info.data().keys());
    for (auto p = info.data().constBegin(); p != info.data().constEnd(); ++p) {
        auto data = p.value();
        for (auto i = data->constBegin(); i != data->constEnd(); ++i) {
            if (i.value() == nullptr) {
                _highlight->remove(p.key(), i.key(), all_properties);

            } else {
                // нельзя добавлять свойства в setConstraintXXX как showHiglightProperty и при этом регистрировать для них ошибку
                Z_CHECK_X(!structure()->constraintsShowHiglightProperties().contains(p.key()),
                    QString("Incorrect showHiglightProperty property: %1:%2").arg(p.key().entityCode().string(), p.key().id().string()));

                _highlight->change(*i.value());
            }
        }
    }

    Z_CHECK_X(_registered_highlight_check.isEmpty(), "В процессе проверки на ошибки были добавлены новые");
}

void HighlightProcessor::blockHighlightAuto()
{
    _block_highlight_auto++;
    if (_block_highlight_auto == 1)
        registerHighlightCheck();
}

void HighlightProcessor::unBlockHighlightAuto()
{
    _block_highlight_auto--;
    Z_CHECK(_block_highlight_auto >= 0);
    if (_block_highlight_auto == 0)
        registerHighlightCheck();
}

void HighlightProcessor::clearHighlightCheckRequests()
{
    if (!_master_processor.isNull()) {
        _master_processor->clearHighlightCheckRequests();
        return;
    }

    if (isStoppedHelper())
        return;

    clearHighlightCheckRequestsHelper();
}

void HighlightProcessor::clearHighlightCheckRequestsHelper()
{
    if (objectExtensionDestroyed())
        return;

    _registered_highlight_check.clear();
    Framework::internalCallbackManager()->removeRequest(this, Framework::HIGHLIGHT_PROCESSOR_CHECK_CALLBACK_KEY);
}

void HighlightProcessor::clearHighlights()
{
    if (!_master_processor.isNull()) {
        _master_processor->clearHighlights();
        return;
    }

    if (isStoppedHelper())
        return;

    clearHighlightsHelper();
}

QList<HighlightItem> HighlightProcessor::cellHighlight(const QModelIndex& index, bool execute_check, bool halt_if_not_found) const
{
    if (!_master_processor.isNull())
        return _master_processor->cellHighlight(index, execute_check, halt_if_not_found);

    Z_CHECK(index.isValid());

    DataProperty dataset = data()->datasetProperty(index.model());
    if (!halt_if_not_found && !dataset.isValid())
        return {};

    Z_CHECK(dataset.isValid());

    DataProperty cell = structure()->propertyCell(dataset, data()->datasetRowID(dataset, index.row(), index.parent()), index.column());

    if (execute_check)
        const_cast<HighlightProcessor*>(this)->executeHighlightCheckRequests();

    return highlight()->items(cell, true);
}

void HighlightProcessor::clearHighlightsHelper()
{
    _highlight->clear();
    clearHighlightCheckRequestsHelper();
}

void HighlightProcessor::removeMasterProcessor(bool start)
{
    if (_master_processor == nullptr)
        return;

    disconnect(_master_processor->highlight(), &HighlightModel::sg_itemInserted, this, &HighlightProcessor::sl_masterHighlightItemInserted);
    disconnect(_master_processor->highlight(), &HighlightModel::sg_itemRemoved, this, &HighlightProcessor::sl_masterHighlightItemRemoved);
    disconnect(_master_processor->highlight(), &HighlightModel::sg_itemChanged, this, &HighlightProcessor::sl_masterHighlightItemChanged);
    disconnect(_master_processor->highlight(), &HighlightModel::sg_beginUpdate, this, &HighlightProcessor::sl_masterHighlightBeginUpdate);
    disconnect(_master_processor->highlight(), &HighlightModel::sg_endUpdate, this, &HighlightProcessor::sl_masterHighlightEndUpdate);

    _master_processor.clear();

    if (start)
        startHelper();
    else
        clearHighlightsHelper();
}

void HighlightProcessor::installMasterProcessor(HighlightProcessor* master_processor)
{
    Z_CHECK_NULL(master_processor);
    Z_CHECK(_master_processor == nullptr);

    _master_processor = master_processor;
    if (!isStoppedHelper())
        stopHelper();

    connect(_master_processor->highlight(), &HighlightModel::sg_itemInserted, this, &HighlightProcessor::sl_masterHighlightItemInserted);
    connect(_master_processor->highlight(), &HighlightModel::sg_itemRemoved, this, &HighlightProcessor::sl_masterHighlightItemRemoved);
    connect(_master_processor->highlight(), &HighlightModel::sg_itemChanged, this, &HighlightProcessor::sl_masterHighlightItemChanged);
    connect(_master_processor->highlight(), &HighlightModel::sg_beginUpdate, this, &HighlightProcessor::sl_masterHighlightBeginUpdate);
    connect(_master_processor->highlight(), &HighlightModel::sg_endUpdate, this, &HighlightProcessor::sl_masterHighlightEndUpdate);
}

HighlightProcessor* HighlightProcessor::masterProcessor() const
{
    return _master_processor;
}

void HighlightProcessor::installExternalProcessing(I_HighlightProcessor* i_processor)
{
    Z_CHECK_NULL(i_processor);
    Z_CHECK(!_i_processors.contains(i_processor));
    _i_processors << i_processor;

    auto obj = dynamic_cast<QObject*>(i_processor);
    Z_CHECK_NULL(obj);
    connect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(sl_highlightProcessorDestroyed(QObject*)));

    _i_processors_map[obj] = i_processor;
}

void HighlightProcessor::removeExternalProcessing(I_HighlightProcessor* i_processor)
{
    Z_CHECK_NULL(i_processor);
    Z_CHECK(_i_processors.removeOne(i_processor));

    auto obj = dynamic_cast<QObject*>(i_processor);
    Z_CHECK_NULL(obj);
    disconnect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(sl_highlightProcessorDestroyed(QObject*)));

    _i_processors_map.remove(obj);
}

bool HighlightProcessor::isExternalProcessingInstalled(I_HighlightProcessor* i_processor) const
{
    Z_CHECK_NULL(i_processor);
    return _i_processors.contains(i_processor);
}

void HighlightProcessor::setHashedDatasetCutomization(I_HashedDatasetCutomize* i_hashed_customize)
{
    Z_CHECK_NULL(i_hashed_customize);
    Z_CHECK(_i_hashed_customize == nullptr);
    _i_hashed_customize = i_hashed_customize;

    auto obj = dynamic_cast<QObject*>(i_hashed_customize);
    Z_CHECK_NULL(obj);
    connect(obj, SIGNAL(destroyed(QObject*)), this, SLOT(sl_hashedDatasetCutomizeDestroyed(QObject*)));
}

QString HighlightProcessor::hashedDatasetkeyValuesToUniqueString(const QString& key, int row, const QModelIndex& parent, const QVariantList& keyValues) const
{
    if (_i_hashed_customize != nullptr)
        return _i_hashed_customize->hashedDatasetkeyValuesToUniqueString(key, row, parent, keyValues);

    return Utils::generateUniqueString(keyValues);
}

void HighlightProcessor::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data);

    if (objectExtensionDestroyed())
        return;

    if (key == Framework::HIGHLIGHT_PROCESSOR_CHECK_CALLBACK_KEY)
        executeHighlightCheckRequests();
}

void HighlightProcessor::sl_allPropertiesUnBlocked()
{
    registerHighlightCheck(_structure->entity());
}

void HighlightProcessor::sl_propertyUnBlocked(const DataProperty& p)
{
    registerHighlightCheck(p);
}

void HighlightProcessor::sl_propertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    Q_UNUSED(old_values);

    if ( // Наборы данных автоматически проверяются на уровне строк
        p.propertyType() != PropertyType::Dataset) {
        registerHighlightCheck(p);
    }
}

void HighlightProcessor::sl_highlightItemInserted(const HighlightItem& item)
{
    if (_master_processor == nullptr)
        emit sg_highlightItemInserted(item);
}

void HighlightProcessor::sl_highlightItemRemoved(const HighlightItem& item)
{
    if (_master_processor == nullptr)
        emit sg_highlightItemRemoved(item);
}

void HighlightProcessor::sl_highlightItemChanged(const HighlightItem& item)
{
    if (_master_processor == nullptr)
        emit sg_highlightItemChanged(item);
}

void HighlightProcessor::sl_highlightBeginUpdate()
{
    if (_master_processor == nullptr)
        emit sg_highlightBeginUpdate();
}

void HighlightProcessor::sl_highlightEndUpdate()
{
    if (_master_processor == nullptr)
        emit sg_highlightEndUpdate();
}

void HighlightProcessor::sl_masterHighlightItemInserted(const HighlightItem& item)
{
    emit sg_highlightItemInserted(item);
}

void HighlightProcessor::sl_masterHighlightItemRemoved(const HighlightItem& item)
{
    emit sg_highlightItemRemoved(item);
}

void HighlightProcessor::sl_masterHighlightItemChanged(const HighlightItem& item)
{
    emit sg_highlightItemChanged(item);
}

void HighlightProcessor::sl_masterHighlightBeginUpdate()
{
    emit sg_highlightBeginUpdate();
}

void HighlightProcessor::sl_masterHighlightEndUpdate()
{
    emit sg_highlightEndUpdate();
}

void HighlightProcessor::sl_highlightProcessorDestroyed(QObject* obj)
{
    auto p = _i_processors_map.value(obj);
    Z_CHECK_NULL(p);
    _i_processors_map.remove(obj);
    Z_CHECK(_i_processors.removeOne(p));
}

void HighlightProcessor::sl_hashedDatasetCutomizeDestroyed(QObject* obj)
{
    Q_UNUSED(obj);
    _i_hashed_customize = nullptr;
}

bool HighlightProcessor::isStoppedHelper() const
{
    return _stop_counter > 0;
}

void HighlightProcessor::sl_dataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if ( // При изменении тултипа незачем проверять ошибки
        (roles.count() != 1 || roles.constFirst() != Qt::ToolTipRole)) {
        for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
            for (int col = topLeft.column(); col <= bottomRight.column(); col++) {
                registerHighlightCheck(data()->propertyCell(p, row, col, topLeft.parent()));
            }
        }

        // Если были изменены ключевые значения, то надо зарегистрировать проверку дубликатов
        auto info = datasetInfo(p, false);
        if (!info->key_columns.isEmpty()) {
            for (int col : qAsConst(info->key_columns)) {
                if (col < topLeft.column() || col > bottomRight.column())
                    continue;

                // проверяем уже существующие строки с дубликатами
                registerDatasetRowKeyDuplicateCheck(info->dataset);

                // проверяем новые, где появились дубликаты
                auto hash = info->getKeyColumnsCheckHash();
                Z_CHECK_NULL(hash);
                hash->clearIfNeed(topLeft, bottomRight);
                for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
                    QVariantList keyValues;
                    QVariantList keyBaseValues;
                    getKeyValues(info, row, topLeft.parent(), keyValues, keyBaseValues);

                    // надо проверить заполнены ли базовые значения
                    for (auto i = keyBaseValues.constBegin(); i != keyBaseValues.constEnd(); ++i) {
                        if (i->toString().trimmed().isEmpty())
                            continue;
                    }

                    QString hash_key = _i_key_values->keyValuesToUniqueString(info->dataset, row, topLeft.parent(), keyValues);
                    if (!hash_key.isEmpty()) {
                        // надо искать по хэш-строке, т.к. она может быть кастомизирована в модели
                        QModelIndexList duplicates = hash->findRowsByHash(hash_key);
                        if (duplicates.count() > 1) {
                            for (auto i = duplicates.constBegin(); i != duplicates.constEnd(); ++i) {
                                if (i->row() == row && i->parent() == topLeft.parent())
                                    continue;

                                registerHighlightCheck(data()->propertyRow(info->dataset, i->row(), i->parent()));
                            }
                        }
                    }
                }

                break;
            }
        }
    }
}

void HighlightProcessor::sl_dataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    for (int i = first; i <= last; i++) {
        registerHighlightCheck(data()->propertyRow(p, i, parent));
    }
}

void HighlightProcessor::sl_dataset_rowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    for (int i = first; i <= last; i++) {
        _highlight->remove(data()->propertyRow(p, i, parent));
    }

    auto info = datasetInfo(p, false);
    if (!info->key_columns.isEmpty()) {
        // проверяем строки с дубликатами
        registerDatasetRowKeyDuplicateCheck(p);
    }
}

void HighlightProcessor::sl_dataset_rowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    registerHighlightCheck(p);
}

void HighlightProcessor::sl_dataset_columnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    for (int i = first; i <= last; i++) {
        registerHighlightCheck(_structure->propertyColumn(p, i));
    }
}

void HighlightProcessor::sl_dataset_columnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)

    for (int i = first; i <= last; i++) {
        _highlight->remove(_structure->propertyColumn(p, i));
    }

    auto info = datasetInfo(p, false);
    if (!info->key_columns.isEmpty()) {
        // проверяем строки с дубликатами
        registerDatasetRowKeyDuplicateCheck(p);
    }
}

void HighlightProcessor::sl_dataset_columnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    registerHighlightCheck(p);
}

void HighlightProcessor::sl_dataset_modelReset(const DataProperty& p)
{
    registerHighlightCheck(p);
}

void HighlightProcessor::sl_invalidateChanged(const DataProperty& p, bool invalidate)
{
    if (!invalidate)
        registerHighlightCheck(p);
}

void HighlightProcessor::checkHightlightValue(
    const DataContainer* data, const QVariant& value, const DataProperty& property, PropertyConstraint* constraint, HighlightInfo& info)
{
    // TODO все эти проверки надо перевести на расчет через ComplexCondition, т.к. сейчас это дублирование логики

    Z_CHECK(property.isValid());
    Z_CHECK_NULL(constraint);

    if (constraint->type() == ConditionType::Required) {
        bool has_data = false;

        if (property.lookup() != nullptr && property.lookup()->type() == LookupType::Request) {
            if (constraint->options().testFlag(ConstraintOption::RequiredID))
                has_data = !value.isNull() && value.isValid() && (value.type() != QVariant::String || !value.toString().trimmed().isEmpty());

            if (!has_data && constraint->options().testFlag(ConstraintOption::RequiredText)) {
                auto links = property.links(PropertyLinkType::LookupFreeText);
                Z_CHECK(links.count() == 1);
                QVariant text_value = data->value(links.at(0)->linkedPropertyId());
                has_data
                    = !text_value.isNull() && text_value.isValid() && (text_value.type() != QVariant::String || !text_value.toString().trimmed().isEmpty());
            }

        } else {
            bool valid_zero = property.options().testFlag(PropertyOption::ValidZero);

            if (!valid_zero && (property.dataType() == DataType::Integer || property.dataType() == DataType::UInteger))
                has_data = value.toInt() != 0;
            else if (!valid_zero && (property.dataType() == DataType::Float || property.dataType() == DataType::UFloat))
                has_data = !fuzzyCompare(value.toDouble(), 0);
            else if (!valid_zero && (property.dataType() == DataType::Numeric || property.dataType() == DataType::UNumeric))
                has_data = Numeric::fromVariant(value) != 0;
            else
                has_data = !value.isNull() && value.isValid() && (value.type() != QVariant::String || !value.toString().trimmed().isEmpty());
        }

        if (!has_data) {
            QString msg;
            if (constraint->message().isEmpty()) {
                if (property.name().isEmpty())
                    msg = ZF_TR(ZFT_NOT_DEFINED_VALUE);
                else
                    msg = ZF_TR(ZFT_NOT_DEFINED).arg(property.name());

            } else {
                msg = constraint->message();
            }

            info.insertHelper(
                property, Framework::HIGHLIGHT_ID_REQUIRED, msg, constraint->highlightType(), -1, QVariant(), false, constraint->highlightOptions());

        } else {
            info.empty(property, Framework::HIGHLIGHT_ID_REQUIRED);
        }

    } else if (constraint->type() == ConditionType::MaxTextLength) {
        if (value.isValid() && (Utils::variantToString(value).length() > constraint->addData().toInt())) {
            QString msg;
            if (constraint->message().isEmpty()) {
                if (property.name().isEmpty())
                    msg = ZF_TR(ZFT_EXCEEDED_MAX_LENGTH);
                else
                    msg = ZF_TR(ZFT_FIELD_EXCEEDED_MAX_LENGTH).arg(property.name()).arg(constraint->addData().toInt());

            } else {
                msg = constraint->message();
            }

            info.insertHelper(
                property, Framework::HIGHLIGHT_ID_MAX_LENGTH_TEXT, msg, constraint->highlightType(), -1, QVariant(), false, constraint->highlightOptions());
        } else {
            info.empty(property, Framework::HIGHLIGHT_ID_MAX_LENGTH_TEXT);
        }

    } else if (constraint->type() == ConditionType::RegExp) {
        if (!Validators::reqExpCheck(constraint->regularExpression(), value)) {
            QString msg;
            if (constraint->message().isEmpty()) {
                if (property.name().isEmpty())
                    msg = ZF_TR(ZFT_BAD_VALUE);
                else
                    msg = ZF_TR(ZFT_BAD_FIELD_VALUE).arg(property.name());

            } else {
                msg = constraint->message();
            }

            info.insert(property, Framework::HIGHLIGHT_ID_REGEXP, msg, constraint->highlightType(), -1, QVariant(), false, constraint->highlightOptions());

        } else {
            info.empty(property, Framework::HIGHLIGHT_ID_REGEXP);
        }

    } else if (constraint->type() == ConditionType::Validator) {
        if (!Validators::validatorCheck(constraint->validator(), value)) {
            QString msg;
            if (constraint->message().isEmpty()) {
                if (property.name().isEmpty())
                    msg = ZF_TR(ZFT_BAD_VALUE);
                else
                    msg = ZF_TR(ZFT_BAD_FIELD_VALUE).arg(property.name());

            } else {
                msg = constraint->message();
            }

            info.insert(property, Framework::HIGHLIGHT_ID_VALIDATOR, msg, constraint->highlightType(), -1, QVariant(), false, constraint->highlightOptions());

        } else {
            info.empty(property, Framework::HIGHLIGHT_ID_VALIDATOR);
        }
    }
}

bool HighlightProcessor::checkInvalidHighlightValue(const DataProperty& property, const QVariant& value, HighlightInfo& info)
{
    if (!property.options().testFlag(PropertyOption::IgnoreInvalidHighlight) && !property.options().testFlag(PropertyOption::ReadOnly)) {
        if (!info.hasLevel(property, InformationType::Warning) && InvalidValue::isInvalidValueVariant(value)) {
            InformationType required_type = InformationType::Invalid;
            HighlightOptions required_options;
            for (auto& constraint : property.constraints()) {
                if (constraint->type() == ConditionType::Required || constraint->type() == ConditionType::RegExp
                    || constraint->type() == ConditionType::Validator) {
                    required_type = constraint->highlightType();
                    required_options = constraint->highlightOptions();
                    break;
                }
            }

            QString msg = ZF_TR(ZFT_BAD_FIELD_VALUE).arg(property.name());

            InformationType invalid_type;
            HighlightOptions invalid_options;
            if (property.options().testFlag(PropertyOption::ErrorIfInvalid)) {
                invalid_type = InformationType::Error;
                invalid_options = HighlightOptions();

            } else if (required_type != InformationType::Invalid) {
                invalid_type = required_type;
                invalid_options = required_options;

            } else {
                invalid_type = InformationType::Warning;
                invalid_options = HighlightOptions();
            }

            info.insert(property, Framework::HIGHLIGHT_ID_INVALID_VALUE, msg, invalid_type, -1, QVariant(), false, invalid_options);

            return true;

        } else {
            info.empty(property, Framework::HIGHLIGHT_ID_INVALID_VALUE);
        }
    }

    return false;
}

void HighlightProcessor::getKeyValues(DatasetInfo* info, int row, const QModelIndex& parent, QVariantList& key_values, QVariantList& key_base_values) const
{
    if (info->key_columns.isEmpty())
        return;

    int colCount = info->item_model()->columnCount(parent);
    for (int col : qAsConst(info->key_columns)) {
        Z_CHECK(col < colCount);
        QVariant data = info->item_model()->index(row, col, parent).data(Qt::DisplayRole);

        key_values << data;
        if (info->key_columns_base.contains(col))
            key_base_values << data;
    }
}

QString HighlightProcessor::keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent, const QVariantList& key_values) const
{
    return _i_key_values->keyValuesToUniqueString(dataset, row, parent, key_values);
}

void HighlightProcessor::checkKeyValues(
    const DataProperty& dataset, int row, const QModelIndex& parent, QString& error_text, DataProperty& error_property) const
{
    _i_key_values->checkKeyValues(dataset, row, parent, error_text, error_property);
    if (error_property.isValid())
        return;

    auto info = datasetInfo(dataset, false);

    QVariantList key_values;
    QVariantList key_base_values;
    getKeyValues(info, row, parent, key_values, key_base_values);

    // надо проверить заполнены ли базовые значения
    for (auto i = key_base_values.constBegin(); i != key_base_values.constEnd(); ++i) {
        if (i->toString().trimmed().isEmpty())
            return;
    }

    auto hash = info->getKeyColumnsCheckHash();
    if (hash == nullptr)
        return;

    error_property = data()->propertyCell(info->dataset, row, info->key_column_error, parent);

    // надо искать по хэш-строке, т.к. она может быть кастомизирована
    QString hash_key = keyValuesToUniqueString(info->dataset, row, parent, key_values);
    if (!hash_key.isEmpty() && hash->findRowsByHash(hash_key).count() > 1) {
        error_text = ZF_TR(ZFT_ROW_ALREADY_EXISTS);
    }
}

void HighlightProcessor::getHighlightAuto(const DataProperty& property, HighlightInfo& info) const
{
    if (property.propertyType() == PropertyType::ColumnPartial || property.propertyType() == PropertyType::RowPartial)
        Z_HALT_INT; // этого быть не должно

    if (!_has_key_columns.isValid()) {
        _has_key_columns = false;
        auto& datasets = _structure->propertiesByType(PropertyType::Dataset);
        for (auto& ds : datasets) {
            if (!ds.columnsByOptions(PropertyOption::KeyColumn).isEmpty()) {
                _has_key_columns = true;
                break;
            }
        }
    }

    DataPropertyList to_check_rows;
    DataPropertyList to_check_datasets;
    DataPropertyList to_check_fields;

    if (property.propertyType() == PropertyType::Entity) {
        // проверяем все поля и наборы данных
        to_check_fields = _structure->propertiesByType(PropertyType::Field);

        auto& datasets = _structure->propertiesByType(PropertyType::Dataset);
        for (auto& ds : datasets) {
            if (ds.hasConstraints() || !ds.columnsByOptions(PropertyOption::KeyColumn).isEmpty())
                // добавляем в проверку только если есть колонки с ограничениями
                to_check_datasets << ds;
        }

    } else if (property.propertyType() == PropertyType::Dataset) {
        // наборы данных разворачиваются в строки
        to_check_datasets << property;

    } else if (property.propertyType() == PropertyType::ColumnFull) {
        // колонки ограничивают проверки по всех строкам
        to_check_datasets << property.dataset();

    } else if (property.propertyType() == PropertyType::RowFull) {
        // строки проверяются полностью
        to_check_rows << property;

    } else if (property.propertyType() == PropertyType::Cell) {
        to_check_rows << _structure->propertyRow(property.dataset(), property.rowId());

    } else {
        to_check_fields << property;
    }

    for (auto& p : to_check_datasets) {
        if (!data()->isInitialized(p))
            continue;

        // наборы данных разворачиваются в строки
        to_check_rows << data()->getAllRowsProperties(p);
    }

    for (auto& p : to_check_fields) {
        if (!p.canHaveValue())
            continue;

        auto value = data()->value(p);

        // сначала проверяем на некорректное значение
        bool ignore_required = checkInvalidHighlightValue(p, value, info);
        // затем автопроверки на основе структуры
        for (auto& constraint : p.constraints()) {
            if (constraint->type() == ConditionType::Required && ignore_required)
                info.empty(p, Framework::HIGHLIGHT_ID_REQUIRED);
            else
                checkHightlightValue(data(), value, p, constraint.get(), info);
        }
    }

    for (auto& row_p : to_check_rows) {
        DataProperty dataset = row_p.dataset();

        QModelIndex idx = data()->findDatasetRowID(dataset, row_p.rowId());
        if (!idx.isValid())
            continue;

        // проверка на уникальность
        QString error_text;
        DataProperty error_property;
        checkKeyValues(dataset, idx.row(), idx.parent(), error_text, error_property);
        if (error_property.isValid()) {
            if (!error_text.isEmpty())
                info.insertHelper(error_property, Framework::HIGHLIGHT_ID_UNIQUE, error_text, InformationType::Error, KEY_ERROR_GROUP_CODE, QVariant(), false,
                    HighlightOptions());
            else
                info.empty(error_property, Framework::HIGHLIGHT_ID_UNIQUE);

        } else {
            Z_CHECK(error_text.isEmpty());
        }

        DataPropertyList columns_to_check;

        if (property.propertyType() == PropertyType::ColumnFull && dataset == property.dataset())
            columns_to_check << property;
        else
            columns_to_check = dataset.columnsConstraint();

        for (auto& col_p : columns_to_check) {
            QVariant value = data()->cell(idx.row(), col_p, Qt::DisplayRole, idx.parent());
            auto cell_p = data()->propertyCell(idx.row(), col_p, idx.parent());

            // сначала проверяем на некорректное значение
            bool ignore_required = checkInvalidHighlightValue(cell_p, value, info);

            // затем автопроверки на основе структуры
            for (auto& constraint : col_p.constraints()) {
                if (constraint->type() == ConditionType::Required && ignore_required)
                    info.empty(cell_p, Framework::HIGHLIGHT_ID_REQUIRED);
                else
                    checkHightlightValue(data(), value, cell_p, constraint.get(), info);
            }
        }
    }
}

void HighlightProcessor::getFieldHighlight(I_HighlightProcessor* processor, const DataProperty& field, HighlightInfo& info)
{
    processor->getFieldHighlight(field, info);
}

void HighlightProcessor::getCellHighlight(
    I_HighlightProcessor* processor, const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent, HighlightInfo& info)
{
    processor->getCellHighlight(dataset, row, column, parent, info);
}

void HighlightProcessor::getDatasetHighlightHelper(I_HighlightProcessor* processor, const DataProperty& dataset, HighlightInfo& info)
{
    info.setCurrentProperty(dataset);
    processor->getDatasetHighlight(dataset, info);
    info.setCurrentProperty();

    auto indexes = data()->getAllRowsIndexes(dataset);
    for (auto& i : qAsConst(indexes)) {
        for (auto& col : dataset.columns()) {
            info.setCurrentProperty(data()->cellProperty(i.row(), col, i.parent()));
            getCellHighlight(processor, dataset, i.row(), col, i.parent(), info);
            info.setCurrentProperty();
        }
    }
}

void HighlightProcessor::getHighlight(I_HighlightProcessor* processor, const DataProperty& p, HighlightInfo& info, DataPropertySet& direct_datasets)
{
    processor->getHighlight(p, info);

    if (!_is_simple)
        return;

    if (p.isEntity()) {
        for (auto& prop : _structure->propertiesMain()) {
            if (!data()->isInitialized(prop))
                continue;

            if (prop.isField()) {
                info.setCurrentProperty(prop);
                getFieldHighlight(processor, prop, info);
                info.setCurrentProperty();

            } else if (prop.isDataset())
                getDatasetHighlightHelper(processor, prop, info);
            else
                Z_HALT_INT;
        }

    } else if (p.isDataset()) {
        if (data()->isInitialized(p))
            getDatasetHighlightHelper(processor, p, info);

    } else if (p.isField()) {
        if (data()->isInitialized(p)) {
            info.setCurrentProperty(p);
            getFieldHighlight(processor, p, info);
            info.setCurrentProperty();
        }

    } else if (p.isRow()) {
        if (data()->isInitialized(p.dataset())) {
            QModelIndex row_index = data()->findDatasetRowID(p.dataset(), p.rowId());
            Z_CHECK(row_index.isValid());
            for (auto& col : p.dataset().columns()) {
                info.setCurrentProperty(data()->cellProperty(row_index.row(), col, row_index.parent()));
                getCellHighlight(processor, p.dataset(), row_index.row(), col, row_index.parent(), info);
                info.setCurrentProperty();
            }

            direct_datasets << p.dataset();
        }

    } else if (p.isCell()) {
        if (data()->isInitialized(p.dataset())) {
            QModelIndex row_index = data()->findDatasetRowID(p.dataset(), p.rowId());
            Z_CHECK(row_index.isValid());
            info.setCurrentProperty(p);
            getCellHighlight(processor, p.dataset(), row_index.row(), p.column(), row_index.parent(), info);
            info.setCurrentProperty();

            direct_datasets << p.dataset();
        }

    } else {
        Z_HALT_INT;
    }
}

void HighlightProcessor::bootstrap()
{
    Z_CHECK_NULL(_highlight);
    Z_CHECK_NULL(_data);
    Z_CHECK_NULL(_i_key_values);

    _structure = _data->structure().get();
    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    connect(_highlight, &HighlightModel::sg_itemInserted, this, &HighlightProcessor::sl_highlightItemInserted);
    connect(_highlight, &HighlightModel::sg_itemRemoved, this, &HighlightProcessor::sl_highlightItemRemoved);
    connect(_highlight, &HighlightModel::sg_itemChanged, this, &HighlightProcessor::sl_highlightItemChanged);
    connect(_highlight, &HighlightModel::sg_beginUpdate, this, &HighlightProcessor::sl_highlightBeginUpdate);
    connect(_highlight, &HighlightModel::sg_endUpdate, this, &HighlightProcessor::sl_highlightEndUpdate);
}

HighlightProcessor::DatasetInfo* HighlightProcessor::datasetInfo(const DataProperty& dataset, bool null_if_not_exists) const
{
    auto info = _dataset_info.value(dataset);
    if (info == nullptr) {
        if (null_if_not_exists)
            return nullptr;

        Z_CHECK(data()->contains(dataset));
        info = Z_MAKE_SHARED(DatasetInfo, this, dataset);
        _dataset_info[dataset] = info;
    }
    return info.get();
}

HighlightProcessor::DatasetInfo::DatasetInfo(const HighlightProcessor* _processor, const DataProperty& _dataset)
    : processor(_processor)
    , dataset(_dataset)
{
    Z_CHECK_NULL(processor);
    Z_CHECK(dataset.isValid());

    auto k_columns = processor->structure()->datasetColumnsByOptions(dataset, PropertyOption::KeyColumn);
    auto k_columns_base = processor->structure()->datasetColumnsByOptions(dataset, PropertyOption::KeyColumnBase);
    auto k_columns_error = processor->structure()->datasetColumnsByOptions(dataset, PropertyOption::KeyColumnError);

    Z_CHECK_X(k_columns_error.count() < 2,
        QStringLiteral("Too many PropertyOption::KeyColumnError columns: %1:%2").arg(processor->structure()->entityCode().value()).arg(dataset.id().value()));

    if (!k_columns_error.isEmpty())
        key_column_error = dataset.columnPos(k_columns_error.first());

    if (!k_columns_base.isEmpty()) {
        for (auto& p : k_columns_base)
            Z_CHECK_X(k_columns.contains(p), QStringLiteral("Key base column (PropertyOption::KeyColumnBase) must be in key columns "
                                                            "(PropertyOption::KeyColumnBase): %1:%2:%3")
                                                 .arg(processor->structure()->entityCode().value())
                                                 .arg(dataset.id().value())
                                                 .arg(p.id().value()));
    }

    if (!k_columns.isEmpty()) {
        key_column_error = std::numeric_limits<int>::max();
        for (auto& p : k_columns) {
            key_columns << dataset.columnPos(p);
            key_column_error = qMin(key_column_error, key_columns.last());
        }
    }
    for (auto& p : k_columns_base) {
        key_columns_base << dataset.columnPos(p);
    }
}

HashedDataset* HighlightProcessor::DatasetInfo::getKeyColumnsCheckHash() const
{
    if (key_columns_check_hash != nullptr)
        return key_columns_check_hash.get();

    if (!key_columns.isEmpty()) {
        key_columns_check_hash = std::make_unique<HashedDataset>(item_model(), key_columns);
        key_columns_check_hash->setCustomization(processor, QString::number(dataset.id().value()));
    }

    return key_columns_check_hash.get();
}

ItemModel* HighlightProcessor::DatasetInfo::item_model() const
{
    // _item_model может обнулиться при перезагрузке данных
    if (_item_model == nullptr)
        _item_model = processor->data()->dataset(dataset);
    return _item_model;
}

} // namespace zf
