#include "zf_data_container.h"
#include "zf_core.h"
#include "zf_resource_manager.h"
#include "zf_module_data_object.h"
#include "zf_logging.h"

#include <QDebug>
#include <QTimeZone>
#include <QJsonObject>
#include <QJsonArray>

namespace zf
{
static void _variantToDebug(QDebug debug, const QVariant& value)
{
    if (value.type() == QVariant::ByteArray || value.type() == QVariant::Icon || value.type() == QVariant::Pixmap)
        debug << QStringLiteral("QVariant(%1, binary)").arg(value.typeName());
    else
        debug << value;
}

//! Значение свойства
struct DataContainerValue
{
    DataProperty property;
    std::unique_ptr<LanguageMap> variant;
    std::unique_ptr<ItemModel> item_model;
    //! Были ли изменения (только для обычных полей, т.к. для наборов данных это сложно контролировать)
    bool changed = false;
    //! Инициализировано ли значение item_model. Особенность в том, что нам надо заранее иметь пустую ItemModel чтобы привязывать к ней ItemView
    //! Но то что она существует, не означает что данные инициализированы
    //! Из этого следует, что для наборов данных надо заранее создать указатели на DataContainerValue в векторе значений
    bool item_model_initialized = false;
    //! Данные не валидны (устарели и требуют обновления)
    bool invalidated = true;
};

//! Данные для DataContainer
class DataContainer_SharedData : public QSharedData
{
public:
    DataContainer_SharedData();
    DataContainer_SharedData(const DataContainer_SharedData& d);
    DataContainer_SharedData(const QString& id, const DataStructurePtr& data_structure);
    ~DataContainer_SharedData();

    void clear();
    void copyFrom(const DataContainer_SharedData* d);

    //! Структура данных
    DataStructurePtr data_structure;

    //! Уникальный идентификатор контейнера
    QString id;
    //! Данные. Индекс равен id соответствующего свойства
    QVector<std::shared_ptr<DataContainerValue>> properties;
    //! Источник для режима прокси
    DataContainerPtr source;
    //! Свойства, которые надо перенапрявлять в режиме прокси.
    //! Ключ - id свойства этого объекта, Значение - id свойства из source
    QHash<PropertyID, PropertyID> proxy_mapping;
    //! Ключ - id свойства из source, Значение - id свойства этого объекта
    QHash<PropertyID, PropertyID> proxy_mapping_inverted;

    //! Идентификаторы наборов данных по их указателю
    QHash<const QAbstractItemModel*, PropertyID> dataset_id;
    //! Язык контейнера по умолчанию
    QLocale::Language language = QLocale::AnyLanguage;

    //! Быстрый поиск по комбинации полей набора данных
    mutable std::unique_ptr<DataHashed> find_by_columns_hash;

    //! Управление освобождением временных ресурсов
    mutable std::unique_ptr<ResourceManager> resource_manager;

    //! Произвольные данные
    QMap<QString, std::shared_ptr<AnyData>> any_data;
};

DataContainer_SharedData::DataContainer_SharedData()
{
    Z_DEBUG_NEW("DataContainer_SharedData");
}

DataContainer_SharedData::DataContainer_SharedData(const DataContainer_SharedData& d)
    : QSharedData(d)
{
    copyFrom(&d);
    Z_DEBUG_NEW("DataContainer_SharedData");
}

DataContainer_SharedData::DataContainer_SharedData(const QString& id, const DataStructurePtr& data_structure)
    : data_structure(data_structure)
    , id(id)
{
    Z_CHECK_NULL(data_structure);
    if (data_structure->isValid())
        properties.resize(data_structure->maxPropertyId().value() + 1);

    Z_DEBUG_NEW("DataContainer_SharedData");
}

DataContainer_SharedData::~DataContainer_SharedData()
{
    clear();
    Z_DEBUG_DELETE("DataContainer_SharedData");
}

void DataContainer_SharedData::clear()
{
    data_structure.reset();
    id.clear();
    properties.clear();
    source.reset();
    proxy_mapping.clear();
    proxy_mapping_inverted.clear();
    dataset_id.clear();
    find_by_columns_hash.reset();
    resource_manager.reset();
    language = QLocale::AnyLanguage;
    any_data.clear();
}

void DataContainer_SharedData::copyFrom(const DataContainer_SharedData* d)
{
    clear();

    id = d->id;
    any_data = d->any_data;
    source = d->source;
    proxy_mapping = d->proxy_mapping;
    language = d->language;
    proxy_mapping_inverted = d->proxy_mapping_inverted;
    data_structure = d->data_structure;

    properties.resize(d->properties.count());
    for (int i = 0; i < d->properties.count(); i++) {
        if (d->properties.at(i) == nullptr)
            continue;

        std::unique_ptr<DataContainerValue> dest = std::make_unique<DataContainerValue>();
        DataContainerValue* from = d->properties.at(i).get();
        dest->property = from->property;
        dest->invalidated = from->invalidated;
        dest->changed = from->changed;

        if (from->variant != nullptr) {
            dest->variant = std::make_unique<LanguageMap>(*from->variant);

        } else {
            Z_CHECK(from->item_model != nullptr);
            dest->item_model_initialized = from->item_model_initialized;
            dest->item_model = std::make_unique<ItemModel>(from->item_model->rowCount(), from->item_model->columnCount());
            Utils::cloneItemModel(from->item_model.get(), dest->item_model.get());
            dataset_id[dest->item_model.get()] = dest->property.id();
        }

        properties[i] = std::move(dest);
    }
}

DataContainer::DataContainer()
    : _d(new DataContainer_SharedData)
{
    init();
}

DataContainer::DataContainer(const DataContainer& d)
    : QObject()
    , _d(d._d)
{
    init();
}

DataContainer::DataContainer(const DataStructurePtr& data_structure, const DataPropertySet& initialization)
    : _d(new DataContainer_SharedData(Utils::generateUniqueStringDefault(), data_structure))
{
    init();
    for (auto& p : initialization) {
        initValue(p);
    }
}

DataContainer::DataContainer(const PropertyID& id, int structure_version, const QList<DataProperty>& properties, const DataPropertySet& initialization)
    : _d(new DataContainer_SharedData(Utils::generateUniqueStringDefault(), DataStructure::independentStructure(id, structure_version, properties)))
{
    init();
    for (auto& p : initialization) {
        initValue(p);
    }
}

DataContainer::DataContainer(
    const PropertyID& id, int structure_version, const QList<DataProperty>& properties, const QHash<DataProperty, QVariant>& value_properties, const QHash<DataProperty, ItemModel*>& value_tables)
    : DataContainer(id, structure_version, properties)
{
    for (auto i = value_properties.constBegin(); i != value_properties.constEnd(); ++i) {
        setValue(i.key(), i.value());
    }
    for (auto i = value_tables.constBegin(); i != value_tables.constEnd(); ++i) {
        setDataset(i.key(), i.value(), CloneContainerDatasetMode::CopyPointer);
    }
}

DataContainer::~DataContainer()
{
}

bool DataContainer::isValid() const
{
    return _d->data_structure != nullptr;
}

void DataContainer::clear()
{
    *this = DataContainer();
}

void DataContainer::debPrint() const
{
    Core::debPrint(variant());
}

QString DataContainer::id() const
{
    return _d->id;
}

DataContainer& DataContainer::operator=(const DataContainer& d)
{
    if (this != &d)
        _d = d._d;
    return *this;
}

bool DataContainer::operator<(const DataContainer& d) const
{
    return id() < d.id();
}

bool DataContainer::operator==(const DataContainer& d) const
{
    return id() == d.id();
}

bool DataContainer::containsProperty(const DataProperty& p) const
{
    if (p.propertyType() == PropertyType::Entity)
        return structure()->entity() == p;

    if (p.propertyType() == PropertyType::Dataset || p.propertyType() == PropertyType::Field)
        return structure()->contains(p);

    if ((p.isRow() || p.isColumn() || p.propertyType() == PropertyType::Cell) && !structure()->contains(p.dataset()))
        return false;

    if (p.isRow())
        return findDatasetRowID(p.dataset(), p.rowId()).isValid();

    if (p.isColumn()) {
        if (isInitialized(p.dataset()))
            return dataset(p.dataset())->columnCount() > p.dataset().columnPos(p);
        else
            return true;
    }

    if (p.propertyType() == PropertyType::Cell)
        return containsProperty(p.row()) && containsProperty(p.column());

    return false;
}

DataProperty DataContainer::propertyCell(const DataProperty& dataset, int row, int column, const QModelIndex& parent) const
{
    return structure()->propertyCell(dataset, datasetRowID(dataset, row, parent), column);
}

DataProperty DataContainer::propertyCell(int row, const PropertyID& column_property, const QModelIndex& parent) const
{
    return propertyCell(row, property(column_property), parent);
}

DataProperty DataContainer::propertyCell(const DataProperty& row_property, const DataProperty& column_property) const
{
    Z_CHECK(row_property.dataset() == column_property.dataset());
    Z_CHECK(row_property.propertyType() == PropertyType::RowFull);
    auto idx = findDatasetRowID(row_property.dataset(), row_property.rowId());
    Z_CHECK(idx.isValid());
    return propertyCell(idx.row(), column_property, idx.parent());
}

DataProperty DataContainer::propertyCell(const DataProperty& row_property, const PropertyID& column_property_id) const
{
    return propertyCell(row_property, property(column_property_id));
}

DataProperty DataContainer::propertyCell(int row, const DataProperty& column_property, const QModelIndex& parent) const
{
    return propertyCell(column_property.dataset(), row, column_property.dataset().columnPos(column_property), parent);
}

DataProperty DataContainer::propertyCell(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    return propertyCell(datasetProperty(index.model()), index.row(), index.column(), index.parent());
}

DataProperty DataContainer::propertyRow(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    return structure()->propertyRow(dataset, datasetRowID(dataset, row, parent));
}

void DataContainer::invalidateLookupModels() const
{
    return structure()->invalidateLookupModels();
}

bool DataContainer::findDiff(const DataContainer* c, const QSet<DataProperty>& ignored_properties, bool ignore_bad_datasets, ChangedBinaryMethod changed_binary_method,
    DataPropertyList& changed_fields, QMap<DataProperty, DataPropertyList>& new_rows, QMap<DataProperty, DataPropertyList>& removed_rows,
    QMap<DataProperty, QMultiHash<zf::RowID, zf::DataProperty>>& changed_cells, Error& error) const
{
    Z_CHECK_NULL(c);
    Z_CHECK(structure() == c->structure());
    error.clear();

    changed_fields.clear();
    new_rows.clear();
    removed_rows.clear();
    changed_cells.clear();

    if (this == c)
        return false;

    // поля
    auto fields = structure()->propertiesByType(PropertyType::Field);
    for (auto const& p : qAsConst(fields)) {
        if (ignored_properties.contains(p))
            continue;

        QVariant v1;
        if (isInitialized(p))
            v1 = value(p);
        QVariant v2;
        if (c->isInitialized(p))
            v2 = c->value(p);

        if (p.dataType() == DataType::Image || p.dataType() == DataType::Bytes) {
            if (changed_binary_method == ChangedBinaryMethod::Ignore) {
                changed_fields << p;

            } else if (changed_binary_method == ChangedBinaryMethod::ThisContainer) {
                if (isChanged(p))
                    changed_fields << p;

            } else if (changed_binary_method == ChangedBinaryMethod::OtherContainer) {
                if (c->isChanged(p))
                    changed_fields << p;
            }

        } else if (!Utils::compareVariant(v1, v2, CompareOperator::Equal)) {
            changed_fields << p;
        }
    }

    // наборы данных
    auto datasets = structure()->propertiesByType(PropertyType::Dataset);
    for (auto const& p : qAsConst(datasets)) {
        if (!isInitialized(p) || !c->isInitialized(p) || ignored_properties.contains(p))
            continue;

        auto r1 = Utils::toSet(getAllRowsProperties(p));
        auto r2 = Utils::toSet(c->getAllRowsProperties(p));

        DataPropertySet intersection = r1;
        intersection.intersect(r2);

        DataPropertySet new_rows_calc = r2;
        new_rows_calc.subtract(intersection);
        if (!new_rows_calc.isEmpty()) {
            auto nr = new_rows_calc.values();

            for (int i = nr.count() - 1; i >= 0; i--) {
                auto& r = nr.at(i);
                if (!r.rowId().isGenerated()) {
                    if (ignore_bad_datasets) {
                        nr.removeAt(i);
#ifdef QT_DEBUG
                        if (!p.options().testFlag(zf::PropertyOption::DBReadIgnored)) {
                            debPrint();
                            c->debPrint();
                            Z_HALT_INT;
                        }
#endif
                        continue;
                    }

                    debPrint();
                    c->debPrint();
                    Z_HALT_INT;
                }
            }
            new_rows[p] = nr;
        }

        DataPropertySet removed_rows_calc = r1;
        removed_rows_calc.subtract(intersection);
        if (!removed_rows_calc.isEmpty())
            removed_rows[p] = removed_rows_calc.values();

        for (const auto& row_property : removed_rows_calc) {
            // удаленные строки должны иметь реальные ключи
            if (!row_property.rowId().isRealKey()) {
                if (ignore_bad_datasets) {
                    removed_rows.remove(p);
                    break;
                } else {
                    error = Error(QString("Rows without real key for dataset %1:%2").arg(row_property.entityCode().value()).arg(row_property.dataset().id().value()));
                    return false;
                }
            }
        }

        QMultiHash<zf::RowID, zf::DataProperty> changed_cells_calc;
        for (const auto& row_property : qAsConst(intersection)) {
            if (!row_property.rowId().isRealKey()) {
                if (ignore_bad_datasets) {
                    changed_cells_calc.clear();
                    break;
                }
                error = Error(QString("Rows without real key for dataset %1:%2").arg(p.entity().id().value()).arg(p.id().value()));
                return false;
            }

            QModelIndex index1 = findDatasetRowID(p, row_property.rowId());
            QModelIndex index2 = c->findDatasetRowID(p, row_property.rowId());
            for (const auto& col_property : qAsConst(p.columns())) {
                if (ignored_properties.contains(col_property))
                    continue;

                QVariant v1 = cell(index1.row(), col_property, Qt::DisplayRole, index1.parent());
                QVariant v2 = c->cell(index2.row(), col_property, Qt::DisplayRole, index2.parent());

                bool changed = false;

                if (col_property.dataType() == DataType::Bytes || col_property.dataType() == DataType::Image) {
                    if (changed_binary_method == ChangedBinaryMethod::Ignore) {
                        changed = true;

                    } else if (changed_binary_method == ChangedBinaryMethod::ThisContainer) {
                        changed = isChanged(index1.row(), col_property, Qt::DisplayRole, index1.parent());

                    } else if (changed_binary_method == ChangedBinaryMethod::OtherContainer) {
                        changed = c->isChanged(index2.row(), col_property, Qt::DisplayRole, index2.parent());
                    }

                } else if (!Utils::compareVariant(v1, v2, CompareOperator::Equal)) {
                    changed = true;
                }

                if (changed)
                    changed_cells_calc.insert(row_property.rowId(), DataStructure::propertyCell(row_property.rowId(), col_property));
            }
        }

        if (!changed_cells_calc.isEmpty())
            changed_cells[p] = changed_cells_calc;
    }

    return !changed_fields.isEmpty() || !new_rows.isEmpty() || !removed_rows.isEmpty() || !changed_cells.isEmpty();
}

Error DataContainer::createDiff(const QSet<DataProperty>& ignored_properties, DataPropertyList& changed_fields, QMap<DataProperty, DataPropertyList>& new_rows,
    QMap<DataProperty, QMultiHash<zf::RowID, zf::DataProperty>>& changed_cells) const
{
    for (const auto& p : qAsConst(structure()->propertiesMain())) {
        if (ignored_properties.contains(p))
            continue;

        if (p.propertyType() == PropertyType::Field) {
            changed_fields << p;

        } else if (p.propertyType() == PropertyType::Dataset) {
            // для наборов данных, которые мы не хотим сохранять в БД нет смысла делать diff
            if (p.options().testFlag(PropertyOption::DBWriteIgnored))
                continue;

            DataPropertyList all_rows = getAllRowsProperties(p);
            DataPropertyList ds_new_rows;
            QMultiHash<zf::RowID, zf::DataProperty> ds_changed_cells;

            for (const auto& row : qAsConst(all_rows)) {
                if (row.rowId().isRealKey()) {
                    // если настоящий ключ, то пишем все ячейки строки
                    for (const auto& col : p.columns()) {
                        if (ignored_properties.contains(col))
                            continue;

                        ds_changed_cells.insert(row.rowId(), cellProperty(row.rowId(), col));
                    }

                } else {
                    // новая строка
                    ds_new_rows << row;
                }
            }

            new_rows[p] = ds_new_rows;
            changed_cells[p] = ds_changed_cells;

        } else {
            Z_HALT(QString("Bad property type mapping %1:%2").arg(p.entityCode().value()).arg(p.id().value()));
        }
    }

    return Error();
}

bool DataContainer::hasDiff(const DataContainer* c, const QSet<DataProperty>& ignored_properties, Error& error, ChangedBinaryMethod changed_binary_method) const
{
    Z_CHECK_NULL(c);

    DataPropertyList changed_fields;
    QMap<DataProperty, DataPropertyList> new_rows;
    QMap<DataProperty, DataPropertyList> removed_rows;
    QMap<DataProperty, QMultiHash<zf::RowID, zf::DataProperty>> changed_cells;
    return findDiff(c, ignored_properties, true, changed_binary_method, changed_fields, new_rows, removed_rows, changed_cells, error);
}

QLocale::Language DataContainer::language() const
{
    if (isProxyMode())
        return proxyContainer()->language();

    return _d->language;
}

void DataContainer::setLanguage(QLocale::Language language)
{
    if (isProxyMode()) {
        proxyContainer()->setLanguage(language);
        return;
    }

    if (_d->language == language)
        return;

    _d->language = language;
    for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
        dataset(p)->setLanguage(language);
    }

    emit sg_languageChanged(language);
}

DataStructurePtr DataContainer::structure() const
{
    return _d->data_structure;
}

DataProperty DataContainer::property(const PropertyID& property_id) const
{
    return structure()->property(property_id);
}

void DataContainer::blockAllProperties() const
{
    if (isProxyMode()) {
        proxyContainer()->blockAllProperties();
        return;
    }

    _block_all_counter++;
    if (_block_all_counter == 1) {
        for (auto& p : structure()->properties()) {
            blockProperty(p);
        }

        clearRowHash();
        emit const_cast<DataContainer*>(this)->sg_allPropertiesBlocked();
    }
}

void DataContainer::unBlockAllProperties() const
{
    if (isProxyMode()) {
        proxyContainer()->unBlockAllProperties();
        return;
    }

    if (_block_all_counter == 0)
        Z_HALT(QString("DataContainer::unBlockAllProperties %1 - counter error").arg(structure()->id().value()));

    _block_all_counter--;
    if (_block_all_counter == 0) {
        for (auto& p : structure()->properties()) {
            unBlockProperty(p);
        }

        clearRowHash();
        emit const_cast<DataContainer*>(this)->sg_allPropertiesUnBlocked();
    }
}

void DataContainer::blockProperty(const DataProperty& p) const
{
    if (isProxyMode()) {
        proxyContainer()->blockProperty(p);
        return;
    }

    Z_CHECK(isValid() && structure()->contains(p));
    int counter = _block_property_info.value(p.id(), 0) + 1;
    _block_property_info[p.id()] = counter;

    if (counter == 1) {
        if (p.propertyType() == PropertyType::Dataset)
            clearRowHash(p);
        emit const_cast<DataContainer*>(this)->sg_propertyBlocked(p);
    }
}

void DataContainer::unBlockProperty(const DataProperty& p) const
{
    if (isProxyMode()) {
        proxyContainer()->unBlockProperty(p);
        return;
    }

    Z_CHECK(isValid() && structure()->contains(p));
    int counter = _block_property_info.value(p.id(), 0);
    if (counter == 0)
        Z_HALT(QString("DataContainer::unBlockAllProperties %1:%2 - counter error").arg(structure()->id().value()).arg(p.id().value()));

    _block_property_info[p.id()] = counter - 1;

    if (counter == 1) {
        if (p.propertyType() == PropertyType::Dataset)
            clearRowHash(p);
        emit const_cast<DataContainer*>(this)->sg_propertyUnBlocked(p);
    }
}

bool DataContainer::isAllPropertiesBlocked() const
{
    return isProxyMode() ? proxyContainer()->isAllPropertiesBlocked() : _block_all_counter > 0;
}

bool DataContainer::isPropertyBlocked(const DataProperty& p) const
{
    if (isProxyMode())
        return proxyContainer()->isPropertyBlocked(p);

    Z_CHECK(isValid() && structure()->contains(p));
    if (isAllPropertiesBlocked())
        return true;

    return _block_property_info.value(p.id(), 0) > 0;
}

void DataContainer::allDataChanged()
{
    if (isAllPropertiesBlocked())
        return;

    for (auto& p : structure()->properties()) {
        if (isPropertyBlocked(p) || !p.isField() || isInvalidated(p) || !isInitialized(p))
            continue;

        emit sg_propertyChanged(p, valueLanguages(p));
    }
}

bool DataContainer::contains(const PropertyID& property_id) const
{
    return structure()->contains(property_id);
}

bool DataContainer::contains(const DataProperty& p) const
{
    return structure()->contains(p);
}

bool DataContainer::setInvalidate(const DataProperty& p, bool invalidated, const ChangeInfo& info)
{
    Z_CHECK(p.isField() || p.isDataset());

    auto c = container(p.id());
    if (c != this)
        return c->setInvalidate(p, invalidated);

    bool changed = true;
    if (!isInitialized(p))
        initValue(p);
    else if (isInvalidated(p) == invalidated)
        changed = false;

    if (changed) {
        bool w_init;
        auto val = c->valueHelper(p, true, w_init);

        val->invalidated = invalidated;

        if (p.isDataset() && invalidated)
            clearRowHash(p);

        emit sg_invalidateChanged(p, invalidated, info);
    }

    emit sg_invalidate(p, invalidated, info);

    return changed;
}

bool DataContainer::setInvalidate(const PropertyID& property_id, bool invalidated, const ChangeInfo& info)
{
    return setInvalidate(property(property_id), invalidated, info);
}

bool DataContainer::setInvalidateAll(bool invalidated, const ChangeInfo& info)
{
    bool res = false;
    for (auto& p : structure()->propertiesMain()) {
        // отмечать как invalidated имеет смысл только те свойства, которые можно загружать из БД
        if (!p.options().testFlag(PropertyOption::DBReadIgnored)) {
            res |= setInvalidate(p, invalidated, info);
        }
    }
    return res;
}

bool DataContainer::isInvalidated(const DataProperty& p) const
{
    if (p.propertyType() == PropertyType::Entity || p.options().testFlag(PropertyOption::DBReadIgnored))
        return false;

    if (p.isDatasetPart())
        return isInvalidated(p.dataset());

    auto c = container(p.id());
    if (c != this)
        return c->isInvalidated(p);

    if (!isInitialized(p))
        return false;

    bool w_init;
    auto val = c->valueHelper(p, false, w_init);
    return val ? val->invalidated : false;
}

bool DataContainer::isInvalidated(const PropertyID& property_id) const
{
    return isInvalidated(property(property_id));
}

bool DataContainer::isInvalidated(const DataPropertySet& p, bool invalidated) const
{
    for (auto& prop : p) {
        if (isInvalidated(prop) != invalidated)
            return false;
    }
    return true;
}

bool DataContainer::isInvalidated(const DataPropertyList& p, bool invalidated) const
{
    for (auto& prop : p) {
        if (isInvalidated(prop) != invalidated)
            return false;
    }
    return true;
}

bool DataContainer::isInvalidated(const PropertyIDList& property_ids, bool invalidated) const
{
    for (auto& prop : property_ids) {
        if (isInvalidated(prop) != invalidated)
            return false;
    }
    return true;
}

bool DataContainer::isInvalidated(const PropertyIDSet& property_ids, bool invalidated) const
{
    for (auto& prop : property_ids) {
        if (isInvalidated(prop) != invalidated)
            return false;
    }
    return true;
}

DataPropertySet DataContainer::invalidatedProperties() const
{
    DataPropertySet res;
    for (auto& p : structure()->propertiesMain()) {
        if (isInvalidated(p))
            res << p;
    }
    return res;
}

bool DataContainer::hasInvalidatedProperties() const
{
    for (auto& p : structure()->propertiesMain()) {
        if (isInvalidated(p))
            return true;
    }
    return false;
}

DataPropertySet DataContainer::whichPropertiesInvalidated(const DataPropertySet& p, bool invalidated) const
{
    DataPropertySet res;
    for (auto& prop : p) {
        if (isInvalidated(prop) == invalidated)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInvalidated(const DataPropertyList& p, bool invalidated) const
{
    DataPropertySet res;
    for (auto& prop : p) {
        if (isInvalidated(prop) == invalidated)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInvalidated(const PropertyIDList& property_ids, bool invalidated) const
{
    DataPropertySet res;
    for (auto& id : property_ids) {
        auto prop = property(id);
        if (isInvalidated(prop) == invalidated)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInvalidated(const PropertyIDSet& property_ids, bool invalidated) const
{
    DataPropertySet res;
    for (auto& id : property_ids) {
        auto prop = property(id);
        if (isInvalidated(prop) == invalidated)
            res << prop;
    }
    return res;
}

QVariant DataContainer::value(const DataProperty& p, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(p.isField());

    if (!isInitialized(p))
        return QVariant();

    if (p.isCell())
        return cell(p, Qt::DisplayRole, language, from_lookup);

    LanguageMap* values = allLanguagesHelper(p);
    if (values == nullptr)
        return QVariant();

    QVariant value = Utils::valueToLanguage(values, propertyLanguage(p, language, false));

    if (from_lookup && p.lookup() != nullptr) {
        if (p.lookup()->type() == LookupType::List) {
            value = p.lookup()->listName(value);
        } else {
            Error error;
            ModelPtr source_model;
            Core::getEntityValue(p.lookup(), QVariant(value), value, source_model, error);
        }
    }

    return value;
}

QVariant DataContainer::value(const PropertyID& property_id, QLocale::Language language, bool from_lookup) const
{
    return value(property(property_id), language, from_lookup);
}

Numeric DataContainer::toNumeric(const DataProperty& p) const
{
    Z_CHECK(p.dataType() == DataType::Numeric || p.dataType() == DataType::UNumeric);
    QVariant v = value(p);
    if (v.isNull() || !v.isValid())
        return Numeric();

    if (v.type() == QVariant::Double)
        return Numeric(v.toDouble());

    return v.value<Numeric>();
}

Numeric DataContainer::toNumeric(const PropertyID& property_id) const
{
    return toNumeric(property(property_id));
}

double DataContainer::toDouble(const DataProperty& p) const
{
    return (p.dataType() == DataType::Numeric || p.dataType() == DataType::UNumeric) ? toNumeric(p).toDouble() : value(p).toDouble();
}

double DataContainer::toDouble(const PropertyID& property_id) const
{
    return toDouble(property(property_id));
}

LanguageMap DataContainer::valueLanguages(const DataProperty& p) const
{
    return valueLanguages(p.id());
}

LanguageMap DataContainer::valueLanguages(const PropertyID& property_id) const
{
    auto values = allLanguagesHelper(property(property_id));
    return values ? *values : LanguageMap();
}

bool DataContainer::isNull(const DataProperty& p, QLocale::Language language) const
{
    return isNullValue(p, value(p, language));
}

bool DataContainer::isNull(const PropertyID& property_id, QLocale::Language language) const
{
    return isNull(property(property_id), language);
}

bool DataContainer::isNullValue(const DataProperty& p, const QVariant& value)
{
    if (!value.isValid() || value.isNull())
        return true;

    if (p.dataType() == DataType::Date) {
        auto d = value.toDate();
        return d.isNull() || !d.isValid();
    }

    if (p.dataType() == DataType::DateTime) {
        auto d = value.toDateTime();
        return d.isNull() || !d.isValid();
    }

    if (p.dataType() == DataType::Time) {
        auto d = value.toTime();
        return d.isNull() || !d.isValid();
    }

    if (value.type() == QVariant::String)
        return value.toString().trimmed().isEmpty();

    return false;
}

bool DataContainer::isNullCell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return isNullValue(column, cell(row, column, role, parent, language));
}

bool DataContainer::isNullCell(int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return isNullValue(property(column_property_id), cell(row, column_property_id, role, parent, language));
}

bool DataContainer::isNullCell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language) const
{
    return isNullValue(column, cell(row_id, column, role, language));
}

bool DataContainer::isNullCell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language) const
{
    return isNullValue(property(column_property_id), cell(row_id, column_property_id, role, language));
}

bool DataContainer::isNullCell(const DataProperty& cell_property, int role, QLocale::Language language) const
{
    return isNullValue(cell_property.column(), cell(cell_property, role, language));
}

LanguageMap* DataContainer::allLanguagesHelper(const DataProperty& prop) const
{
    Z_CHECK(prop.isValid());
    Z_CHECK_X(prop.propertyType() != PropertyType::Dataset, QString("Property not variant: %1:%2").arg(structure()->id().value()).arg(prop.id().value()));
    Z_CHECK_X(prop.entity() == structure()->entity(), QString("Property entity not equal %1 != %2").arg(structure()->entity().id().value()).arg(prop.entity().id().value()));

    auto c = container(prop.id());
    if (c != this)
        return c->allLanguagesHelper(prop);

    bool w_init;
    auto val = c->valueHelper(prop, false, w_init);
    return val ? val->variant.get() : nullptr;
}

void DataContainer::datasetToDebugHelper(QDebug debug, const ItemModel* m, const QModelIndex& parent) const
{
    for (int row = 0; row < m->rowCount(parent); row++) {
        debug << '\n';
        debug << Core::debugIndent();
        for (int col = 0; col < m->columnCount(parent); col++) {
            QMap<int, LanguageMap> data = m->dataHelperLanguageRoleMap(m->index(row, col, parent));
            // всего один элемент с обычной ролью
            bool is_simple_value = (data.count() == 1 && (data.constBegin().key() == Qt::DisplayRole || data.constBegin().key() == Qt::EditRole) && data.constBegin().value().count() == 1
                                    && data.constBegin().value().constBegin().key() == QLocale::AnyLanguage);

            if (!is_simple_value)
                debug << "[";

            for (auto d_it = data.constBegin(); d_it != data.constEnd(); ++d_it) {
                int role = d_it.key();
                LanguageMap l_map = d_it.value();
                if (is_simple_value) {
                    _variantToDebug(debug, l_map.constBegin().value());
                } else {
                    if (role < Qt::UserRole)
                        debug << "(" << static_cast<Qt::ItemDataRole>(role) << ",";
                    else
                        debug << "(" << role << ",";
                    for (auto l_it = l_map.constBegin(); l_it != l_map.constEnd(); ++l_it) {
                        debug << "<" << l_it.key() << ",";
                        _variantToDebug(debug, l_it.value());
                        debug << ">";
                    }
                    debug << ")";
                }
            }

            if (!is_simple_value)
                debug << "]";

            if (col < m->columnCount(parent) - 1)
                debug << " | ";
        }
        Core::beginDebugOutput();
        datasetToDebugHelper(debug, m, m->index(row, 0, parent));
        Core::endDebugOutput();
    }
}

QModelIndex DataContainer::findRowID_helper(const DataProperty& dataset_property, ItemModel* model, QHash<RowID, QModelIndex>* hash, const RowID& row_id, const QModelIndex& parent) const
{
    QModelIndex res;

    for (int row = 0; row < model->rowCount(parent); row++) {
        QModelIndex index = model->index(row, 0, parent);
        RowID id = datasetRowID(dataset_property, row, parent);
        Z_CHECK(id.isValid());
        hash->insert(id, index);
        if (id == row_id)
            res = index;

        QModelIndex found = findRowID_helper(dataset_property, model, hash, row_id, index);
        if (!res.isValid() && found.isValid())
            res = found;
    }

    return res;
}

void DataContainer::getAllRowsProperties_helper(DataPropertyList& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        rows << structure()->propertyRow(dataset_property, datasetRowID(dataset_property, row, parent));
        getAllRowsProperties_helper(rows, dataset_property, model, model->index(row, 0, parent));
    }
}

void DataContainer::getAllRowsIndexes_helper(QModelIndexList& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        rows << model->index(row, 0, parent);
        getAllRowsIndexes_helper(rows, dataset_property, model, rows.last());
    }
}

void DataContainer::getAllRows_helper(Rows& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        rows << model->index(row, 0, parent);
        getAllRows_helper(rows, dataset_property, model, rows.last());
    }
}

void DataContainer::clearRowId(const DataProperty& dataset_property)
{
    auto ds = dataset(dataset_property);
    ds->beginResetModel();

    clearRowHash(dataset_property);
    clearRowIdHelper(ds, QModelIndex());

    ds->endResetModel();
}

void DataContainer::clearRowIdHelper(ItemModel* model, const QModelIndex& parent)
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        model->setData(row, 0, QVariant(), Consts::UNIQUE_ROW_COLUMN_ROLE, parent);
        clearRowIdHelper(model, model->index(row, 0, parent));
    }
}

void DataContainer::createUninitializedDataset(const DataProperty& dataset_property)
{
    Z_CHECK(dataset_property.isDataset());

    auto c = container(dataset_property.id());
    if (c != this)
        return c->createUninitializedDataset(dataset_property);

    bool w_init;
    auto val = c->valueHelper(dataset_property, false, w_init);
    Z_CHECK_NULL(val);
    Z_CHECK_NULL(val->item_model);

    disconnectFromDataset(val->item_model.get());
    val->item_model = std::make_unique<ItemModel>(0, dataset_property.columnCount());
    connectToDataset(val->item_model.get());
}

void DataContainer::fillSameDatasets(const DataProperty& changed_dataset, bool force_source)
{
    auto c = container(changed_dataset.id());
    if (c != this) {
        c->fillSameDatasets(changed_dataset, force_source);
        return;
    }

    Z_CHECK(changed_dataset.isDataset());
    if (c->_same_props_block_count > 0 || c->_same_props_changing.contains(changed_dataset.id()))
        return;

    DataProperty control;
    if (force_source)
        control = changed_dataset;
    else
        control = structure()->samePropertiesMaster(changed_dataset);

    QList<std::shared_ptr<PropertyIDSet>> same_props = structure()->getSameProperties(control);
    if (!same_props.isEmpty()) {
        auto source = dataset(control);

        for (const std::shared_ptr<PropertyIDSet>& same_group : qAsConst(same_props)) {
            for (const PropertyID& prop : qAsConst(*same_group)) {
                if (!c->_same_props_changing.contains(prop)) {
                    auto p = property(prop);
                    auto ds = dataset(p);
                    if (ds == source)
                        continue;

                    Z_CHECK(p.isDataset());
                    Z_CHECK(ds->columnCount() == source->columnCount());

                    c->_same_props_changing << prop;
                    ds->beginResetModel();

                    // клонируем
                    Utils::cloneItemModel(source, ds);
                    // очищаем RowId
                    clearRowId(p);

                    ds->endResetModel();
                    c->_same_props_changing.remove(prop);
                }
            }
        }
    }
}

const QMultiMap<DataProperty, DataProperty>& DataContainer::dataSourcePriorities() const
{
    if (_data_source_priorities == nullptr) {
        _data_source_priorities = std::make_unique<QMultiMap<DataProperty, DataProperty>>();

        // надо отслеживать изменения свойств dataSourcePriority для формирования ID
        for (auto& property : structure()->propertiesMain()) {
            if (property.isDataset() || property.options().testFlag(PropertyOption::SimpleDataset))
                continue;

            auto links = property.links(PropertyLinkType::DataSourcePriority);
            if (links.isEmpty())
                continue;

            Z_CHECK(links.count() == 1);
            for (auto& p : links.at(0)->dataSourcePriority()) {
                _data_source_priorities->insertMulti(structure()->property(p), property);
            }
        }
    }
    return *_data_source_priorities;
}

void DataContainer::clearRowHash(const DataProperty& dataset_property) const
{
    if (_d->source != nullptr)
        _d->source->clearRowHash(dataset_property);

    if (dataset_property.isValid()) {
        Z_CHECK(contains(dataset_property));
        _row_by_id.remove(dataset_property);

    } else {
        for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
            clearRowHash(p);
        }
    }
}

QString DataContainer::toString(const DataProperty& p, LocaleType conv, QLocale::Language language) const
{
    return Utils::variantToString(value(p, language), conv);
}

QString DataContainer::toString(const PropertyID& property_id, LocaleType conv, QLocale::Language language) const
{
    return toString(property(property_id), conv, language);
}

QDate DataContainer::toDate(const DataProperty& p) const
{
    return value(p).toDate();
}

QDate DataContainer::toDate(const PropertyID& property_id) const
{
    return toDate(property(property_id));
}

QDateTime DataContainer::toDateTime(const DataProperty& p) const
{
    return value(p).toDateTime();
}

QDateTime DataContainer::toDateTime(const PropertyID& property_id) const
{
    return toDateTime(property(property_id));
}

QTime DataContainer::toTime(const DataProperty& p) const
{
    QVariant v = value(p);
    if (v.type() == QVariant::Time)
        return v.toTime();
    if (v.type() == QVariant::Date)
        return QTime();
    if (v.type() == QVariant::DateTime)
        return v.toDateTime().time();

    return value(p).toTime();
}

QTime DataContainer::toTime(const PropertyID& property_id) const
{
    return toTime(property(property_id));
}

Uid DataContainer::toUid(const DataProperty& p) const
{
    return Uid::fromVariant(value(p));
}

Uid DataContainer::toUid(const PropertyID& property_id) const
{
    return toUid(property(property_id));
}

int DataContainer::toInt(const DataProperty& p, int default_value) const
{
    bool ok;
    int res = value(p).toInt(&ok);
    return ok ? res : default_value;
}

int DataContainer::toInt(const PropertyID& property_id, int default_value) const
{
    return toInt(property(property_id), default_value);
}

bool DataContainer::toBool(const DataProperty& p) const
{
    return value(p).toBool();
}

bool DataContainer::toBool(const PropertyID& property_id) const
{
    return value(property_id).toBool();
}

EntityID DataContainer::toEntityID(const DataProperty& p) const
{
    return EntityID(value(p).toInt());
}

EntityID DataContainer::toEntityID(const PropertyID& property_id) const
{
    return EntityID(value(property_id).toInt());
}

InvalidValue DataContainer::toInvalidValue(const DataProperty& p) const
{
    return InvalidValue::fromVariant(value(p));
}

InvalidValue DataContainer::toInvalidValue(const PropertyID& property_id) const
{
    return toInvalidValue(property(property_id));
}

QByteArray DataContainer::toByteArray(const DataProperty& p) const
{
    if (p.dataType() == DataType::Bytes || p.dataType() == DataType::Image)
        return value(p).toByteArray();

    if (p.dataType() == DataType::String)
        return toString(p).toUtf8();

    return value(p).toByteArray();
}

QByteArray DataContainer::toByteArray(const PropertyID& property_id) const
{
    return toByteArray(property(property_id));
}

bool DataContainer::isInvalidValue(const DataProperty& p, QLocale::Language language) const
{
    if (p.isCell())
        return isInvalidCell(p.rowId(), p.column(), Qt::DisplayRole, language);
    return InvalidValue::isInvalidValueVariant(value(p, language));
}

bool DataContainer::isInvalidValue(const PropertyID& property_id, QLocale::Language language) const
{
    return isInvalidValue(property(property_id), language);
}

bool DataContainer::isInvalidCell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return InvalidValue::isInvalidValueVariant(cell(row, column, role, parent, language));
}

bool DataContainer::isInvalidCell(int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language) const
{
    return InvalidValue::isInvalidValueVariant(cell(row, column_property_id, role, parent, language));
}

bool DataContainer::isInvalidCell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language) const
{
    return InvalidValue::isInvalidValueVariant(cell(row_id, column, role, language));
}

bool DataContainer::isInvalidCell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language) const
{
    return InvalidValue::isInvalidValueVariant(cell(row_id, column_property_id, role, language));
}

ItemModel* DataContainer::dataset(const PropertyID& property_id) const
{
    return dataset(property(property_id));
}

ItemModel* DataContainer::dataset(const DataProperty& prop) const
{
    Z_CHECK(prop.isValid());
    Z_CHECK_X(prop.propertyType() == PropertyType::Dataset, QString("Property not dataset: %1:%2").arg(structure()->id().value()).arg(prop.id().value()));
    Z_CHECK_X(prop.entity().entityCode() == structure()->entity().entityCode(), QString("Property entity not equal %1 != %2").arg(structure()->entity().id().value()).arg(prop.entity().id().value()));

    auto c = container(prop.id());
    if (c != this)
        return c->dataset(prop);

    if (prop.options().testFlag(PropertyOption::ClientOnly) && !c->isInitialized(prop))
        const_cast<DataContainer*>(c)->initDataset(prop, 0);

    bool w_init;
    auto val = c->valueHelper(prop, false, w_init);
    Z_CHECK(val != nullptr && val->item_model != nullptr);
    return val->item_model.get();
}

ItemModel* DataContainer::dataset() const
{
    return dataset(structure()->singleDatasetId());
}

Error DataContainer::initValues(const DataPropertySet& properties)
{
    return initValues(properties.toList());
}

Error DataContainer::initValues(const DataPropertyList& properties)
{
    Error error;
    for (auto& p : properties) {
        if (!isInitialized(p)) {
            if (p.propertyType() == PropertyType::Dataset)
                initDataset(p, 0);
            else
                error += initValue(p);
        }
    }
    return error;
}

Error DataContainer::initValues(const PropertyIDList& properties)
{
    DataPropertyList props;
    for (auto& p : properties) {
        props << property(p);
    }
    return initValues(props);
}

Error DataContainer::initAllValues()
{
    return initValues(structure()->propertiesMain());
}

Error DataContainer::initValue(const DataProperty& p)
{
    if (isInitialized(p))
        return Error();

    return resetValue(p);
}

Error DataContainer::initValue(const PropertyID& property_id)
{
    return initValue(property(property_id));
}

Error DataContainer::resetValue(const DataProperty& p)
{
    if (p.isDataset()) {
        setRowCount(p, 0);
        return Error();
    }

    return setValue(p, p.defaultValue());
}

Error DataContainer::resetValue(const PropertyID& property_id)
{
    return resetValue(property(property_id));
}

Error DataContainer::resetValues()
{
    Error error;
    for (auto& p : structure()->propertiesMain()) {
        error = resetValue(p);
        if (error.isError())
            return error;
    }
    return error;
}

Error DataContainer::clearValue(const DataProperty& p)
{
    if (p.propertyType() == PropertyType::Field) {
        return setValue(p, QVariant());

    } else if (p.propertyType() == PropertyType::Dataset) {
        setRowCount(p, 0);

    } else {
        Z_HALT_INT;
    }

    return {};
}

Error DataContainer::clearValue(const PropertyID& property_id)
{
    return clearValue(property(property_id));
}

Error DataContainer::setValue(const PropertyID& property_id, const QVariant& v, QLocale::Language language)
{
    return setValue(property(property_id), v, language);
}

Error DataContainer::setValue(const DataProperty& p, const Numeric& v)
{
    return setValue(p, v.toVariant());
}

Error DataContainer::setValue(const PropertyID& property_id, const Numeric& v)
{
    return setValue(property(property_id), v);
}

Error DataContainer::setValue(const DataProperty& p, const QByteArray& v, QLocale::Language language)
{
    return setValue(p, QVariant(v), language);
}

Error DataContainer::setValue(const PropertyID& property_id, const QByteArray& v, QLocale::Language language)
{
    return setValue(property_id, QVariant(v), language);
}

Error DataContainer::setValue(const DataProperty& p, const QVariant& v, QLocale::Language language)
{
    Z_CHECK(p.isValid());
    Z_CHECK_X(p.propertyType() == PropertyType::Field, QString("Property not field: %1:%2").arg(structure()->id().value()).arg(p.id().value()));
    Z_CHECK_X(p.entity() == structure()->entity(), QString("Property entity not equal %1 != %2").arg(structure()->entity().id().value()).arg(p.entity().id().value()));

    auto c = container(p.id());
    if (c != this) {
        return c->setValue(p, v, language);
    }

    Z_CHECK(!isWriteDisabled());

    language = propertyLanguage(p, language, true);

    Error error;
    bool changed = false;
    bool w_init;
    auto val = c->valueHelper(p, true, w_init);

    if (val->variant == nullptr)
        val->variant = std::make_unique<LanguageMap>();

    // старое значение
    QVariant old_value;
    LanguageMap old_values = *val->variant;
    bool process_same_props_required = false;

    // можно ли сравнивать значения
    bool comparible_data = (p.dataType() != DataType::Bytes && p.dataType() != DataType::Image);

    QVariant newValue;
    error = p.convertValue(v, newValue);

    if (val->variant->isEmpty()) {
        changed = v.isValid() && !v.isNull();
        if (changed)
            val->variant->insert(language, newValue);

        process_same_props_required = comparible_data;

    } else if (val->variant->contains(language)) {
        if (!comparible_data) {
            changed = true;

        } else {
            old_value = val->variant->value(language);
            changed = !Utils::compareVariant(old_value, newValue);
            process_same_props_required = changed;
        }
        // прямая установка значения языка
        if (changed)
            val->variant->insert(language, newValue);

    } else {
        changed = true;
        val->variant->insert(language, newValue);
        process_same_props_required = comparible_data;
    }

    if (changed)
        val->changed = true;

    if (!isPropertyBlocked(p)) {
        if (w_init) {
            if (p.propertyType() == PropertyType::Dataset)
                clearRowHash(p);

            emit sg_propertyInitialized(p);
        }

        if ((w_init || changed)) {
            if (p.propertyType() == PropertyType::Dataset)
                clearRowHash(p);

            emit sg_propertyChanged(p, old_values);
        }
    }

    // если язык не QLocale::AnyLanguage, то это явно не поле, связанное с ключевым значением
    if (language == QLocale::AnyLanguage && p.propertyType() == PropertyType::Field && Utils::isMainThread())
        error << processDataSourceChanged(p);

    setInvalidate(p, false);

    // поддерживаем изменения в одинаковых свойствах
    if (changed && process_same_props_required && c->_same_props_block_count == 0 && !c->_same_props_changing.contains(p.id())) {
        QList<std::shared_ptr<PropertyIDSet>> same_props = structure()->getSameProperties(p);
        if (!same_props.isEmpty()) {
            Z_CHECK(p.isField());

            c->_same_props_changing << p.id();
            for (const std::shared_ptr<PropertyIDSet>& same_group : qAsConst(same_props)) {
                for (const PropertyID& prop : qAsConst(*same_group)) {
                    if (!c->_same_props_changing.contains(prop)) {
                        // обновляем "одинаковые" свойства только если у них совпадают исходные значения
                        QVariant same_p_val = c->value(prop, language);
                        if (Utils::compareVariant(old_value, same_p_val)) {
                            c->_same_props_changing << prop;
                            error << c->setValue(prop, v, language);
                            c->_same_props_changing.remove(prop);
                        }
                    }
                }
            }
            c->_same_props_changing.remove(p.id());
        }
    }

    // аналогично поддерживаем изменения на основе PropertyLinkType::DataSourcePriority
    if (changed && c->_data_source_priority_block_count == 0 && !c->_data_source_priority_props_changing.contains(p.id())) {
        c->_data_source_priority_props_changing << p.id();

        auto dsp_props = dataSourcePriorities().values(p);
        for (auto& prop : qAsConst(dsp_props)) {
            auto links = prop.links(PropertyLinkType::DataSourcePriority);
            Z_CHECK(!links.isEmpty());
            Z_CHECK(links.count() == 1);
            Z_CHECK(links.at(0)->dataSourcePriority().count() > 0);
            // выбираем первое, содержащее значение
            QVariant dsp_value;
            for (auto& sp : links.at(0)->dataSourcePriority()) {
                if (isNull(sp))
                    continue;

                dsp_value = c->value(sp, language);
                Z_CHECK(dsp_value.isValid());
                break;
            }

            error << c->setValue(prop, dsp_value);
        }

        c->_data_source_priority_props_changing.remove(p.id());
    }

    return error;
}

Error DataContainer::setValue(const DataProperty& p, const QString& v, QLocale::Language language)
{
    return setValue(p, QVariant(v), language);
}

Error DataContainer::setValue(const PropertyID& property_id, const QString& v, QLocale::Language language)
{
    return setValue(property_id, QVariant(v), language);
}

Error DataContainer::setValue(const DataProperty& p, const char* v, QLocale::Language language)
{
    return setValue(p, QVariant(QString::fromUtf8(v)), language);
}

Error DataContainer::setValue(const PropertyID& property_id, const char* v, QLocale::Language language)
{
    return setValue(property_id, QVariant(QString::fromUtf8(v)), language);
}

Error DataContainer::setValue(const DataProperty& p, bool v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, bool v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, int v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, int v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, uint v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, uint v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, qint64 v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, qint64 v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, quint64 v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, quint64 v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, double v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, double v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, float v)
{
    return setValue(p, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, float v)
{
    return setValue(property_id, QVariant(v), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, const Uid& v)
{
    return setValue(p, v.variant(), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, const Uid& v)
{
    return setValue(property_id, v.variant(), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, const ID_Wrapper& v)
{
    return setValue(p, v.value(), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const PropertyID& property_id, const ID_Wrapper& v)
{
    return setValue(property_id, v.value(), QLocale::AnyLanguage);
}

Error DataContainer::setValue(const DataProperty& p, const LanguageMap& values)
{
    Error error;
    for (auto i = values.constBegin(); i != values.constEnd(); ++i) {
        error = setValue(p, i.value(), i.key());
        if (error.isError())
            return error;
    }
    return Error();
}

Error DataContainer::setValue(const PropertyID& property_id, const LanguageMap& values)
{
    return setValue(property(property_id), values);
}

Error DataContainer::setValue(const DataProperty& p, const DataProperty& v)
{
    return setValue(p, value(v));
}

Error DataContainer::setValue(const PropertyID& property_id, const DataProperty& v)
{
    return setValue(property_id, value(v));
}

Error DataContainer::setValue(const DataProperty& p, const PropertyID& v)
{
    return setValue(p, value(v));
}

Error DataContainer::setValue(const PropertyID& property_id, const PropertyID& v)
{
    return setValue(property_id, value(v));
}

Error DataContainer::setSimpleValues(const DataProperty& property, const QVariantList& values)
{
    Z_CHECK(property.propertyType() == PropertyType::Dataset);
    Z_CHECK(property.options().testFlag(PropertyOption::SimpleDataset));

    auto lookup = property.lookup();
    Z_CHECK_NULL(lookup);

    QVariantList to_add = values;

    Error error;

    // что выбрано
    for (int row = rowCount(property) - 1; row >= 0; row--) {
        QVariant id = cell(row, lookup->datasetKeyColumnID(), lookup->datasetKeyColumnRole());
        int pos = Utils::indexOfVariantList(to_add, id, Core::locale(LocaleType::Universal));
        if (pos >= 0) {
            // такой id есть, надо удалить из списка на добавление
            to_add.removeAt(pos);
        } else {
            // такая строка не выбрана
            removeRow(property, row);
        }
    }

    // добавляем отсутствующие
    for (auto& v : qAsConst(to_add)) {
        int row = appendRow(property);
        error << setCell(row, lookup->datasetKeyColumnID(), v, lookup->datasetKeyColumnRole());
    }

    return error;
}

Error DataContainer::setSimpleValues(const PropertyID& property_id, const QVariantList& values)
{
    return setSimpleValues(property(property_id), values);
}

QVariantList DataContainer::getSimpleValues(const DataProperty& property) const
{
    Z_CHECK(property.propertyType() == PropertyType::Dataset);
    Z_CHECK(property.options().testFlag(PropertyOption::SimpleDataset));
    auto lookup = property.lookup();
    Z_CHECK_NULL(lookup);

    QVariantList values;

    for (int row = 0; row < rowCount(property); row++) {
        values << cell(row, lookup->datasetKeyColumnID(), lookup->datasetKeyColumnRole());
    }

    return values;
}

QVariantList DataContainer::getSimpleValues(const PropertyID& property_id) const
{
    return getSimpleValues(property(property_id));
}

bool DataContainer::isChanged(const DataProperty& p) const
{
    Z_CHECK(p.isField());

    if (!isInitialized(p))
        return false;

    auto c = container(p.id());
    if (c != this)
        return c->isChanged(p);

    bool w_init;
    auto val = c->valueHelper(p, false, w_init);
    return val == nullptr ? false : val->changed;
}

bool DataContainer::isChanged(const PropertyID& property_id) const
{
    return isChanged(property(property_id));
}

bool DataContainer::isChanged(int row, const DataProperty& column, int role, const QModelIndex& parent) const
{
    Z_CHECK(column.isColumn());
    if (!isInitialized(column.dataset()))
        return false;

    auto ds = dataset(column.dataset());

    return ds->isChanged(ds->index(row, column.pos(), parent), role);
}

bool DataContainer::isChanged(int row, const PropertyID& column, int role, const QModelIndex& parent) const
{
    return isChanged(row, property(column), role, parent);
}

void DataContainer::resetChanged(const DataProperty& p)
{
    Z_CHECK(p.isField());

    if (!isInitialized(p))
        return;

    auto c = container(p.id());
    if (c != this) {
        c->unInitialize(p);
        return;
    }

    bool w_init;
    auto val = c->valueHelper(p, false, w_init);
    if (val == nullptr)
        return;

    val->changed = false;
}

void DataContainer::resetChanged(const PropertyID& property_id)
{
    resetChanged(property(property_id));
}

void DataContainer::resetChanged(int row, const DataProperty& column, int role, const QModelIndex& parent)
{
    Z_CHECK(column.isColumn());
    if (!isInitialized(column.dataset()))
        return;

    auto ds = dataset(column.dataset());
    ds->resetChanged(ds->index(row, column.pos(), parent), role);
}

void DataContainer::resetChanged(int row, const PropertyID& column, int role, const QModelIndex& parent)
{
    resetChanged(row, property(column), role, parent);
}

void DataContainer::resetChanged()
{
    for (auto& p : qAsConst(structure()->propertiesMain())) {
        if (!isInitialized(p))
            continue;

        if (p.isField())
            resetChanged(p);
        else if (p.isDataset())
            dataset(p)->resetChanged();
        else
            Z_HALT_INT;
    }
}

void DataContainer::disableWrite()
{
    _disable_write_counter++;
}

void DataContainer::enableWrite()
{
    _disable_write_counter--;
    Z_CHECK(_disable_write_counter > 0);
}

bool DataContainer::isWriteDisabled() const
{
    return _disable_write_counter > 0;
}

void DataContainer::blockSameProperties()
{
    _same_props_block_count++;
}

void DataContainer::unBlockSameProperties()
{
    _same_props_block_count--;
    Z_CHECK(_same_props_block_count >= 0);
}

bool DataContainer::isSamePropertiesBlocked() const
{
    return _same_props_block_count > 0;
}

void DataContainer::fillSameProperties(bool no_db_read_ignored)
{
    blockSameProperties();

    for (const PropertyIDList& same : structure()->sameProperties()) {
        if (same.isEmpty())
            continue;

        if (property(same.at(0)).isDataset()) {
            // для наборов данных у нас только один выход - полное клонирование
            DataProperty source_property;
            for (const PropertyID& prop : same) {
                auto p = property(prop);
                if (no_db_read_ignored && p.options().testFlag(PropertyOption::DBReadIgnored))
                    continue; // исходные значения берем только из тех, кто может быть считан из БД

                auto ds = dataset(p);
                if (!source_property.isValid() && ds->rowCount() > 0) {
                    source_property = p;
                    break;
                }
            }

            if (source_property.isValid())
                fillSameDatasets(source_property, true);

        } else {
            QVariant* same_value = nullptr;
            for (const PropertyID& p : same) {
                if (no_db_read_ignored && property(p).options().testFlag(PropertyOption::DBReadIgnored))
                    continue; // исходные значения берем только из тех, кто может быть считан из БД

                QVariant v = value(p);
                if (same_value == nullptr && (v.isValid() && (v.type() != QVariant::String || !v.toString().trimmed().isEmpty()))) {
                    same_value = new QVariant(v);
                    break;
                }
            }

            if (same_value != nullptr) {
                for (const PropertyID& p : same) {
                    setValue(p, *same_value);
                }
                delete same_value;
            }
        }
    }

    unBlockSameProperties();
}

void DataContainer::blockDataSourcePriority()
{
    _data_source_priority_block_count++;
    if (_d->source != nullptr)
        _d->source->_data_source_priority_block_count++;
}

void DataContainer::unBlockDataSourcePriority()
{
    _data_source_priority_block_count--;
    Z_CHECK(_data_source_priority_block_count >= 0);

    if (_d->source != nullptr) {
        _d->source->_data_source_priority_block_count--;
        Z_CHECK(_d->source->_data_source_priority_block_count >= 0);
    }
}

bool DataContainer::isDataSourcePriorityBlocked() const
{
    return _data_source_priority_block_count > 0;
}

void DataContainer::fillDataSourcePriority(const DataProperty& property)
{
    blockDataSourcePriority();

    const DataPropertySet* props;
    std::unique_ptr<DataPropertySet> props_one;
    if (property.isValid()) {
        props_one = std::make_unique<DataPropertySet>();
        *props_one << property;
        props = props_one.get();

    } else {
        props = &structure()->propertiesMain();
    }

    for (auto& p : *props) {
        auto links = p.links(PropertyLinkType::DataSourcePriority);
        if (links.isEmpty())
            continue;
        Z_CHECK(links.count() == 1);
        Z_CHECK(links.at(0)->dataSourcePriority().count() > 0);
        // выбираем первое, содержащее значение
        QVariant dsp_value;
        for (auto& sp : links.at(0)->dataSourcePriority()) {
            if (isNull(sp))
                continue;

            dsp_value = value(sp);
            Z_CHECK(dsp_value.isValid());
            break;
        }

        // надо избежать случайной ициализации поля, чтобы корректно работала подгрузка данных с сервера
        if (dsp_value.isValid())
            setValue(p, dsp_value);
        else if (isInitialized(p))
            clearValue(p);
    }

    unBlockDataSourcePriority();
}

void DataContainer::unInitialize(const DataProperty& p)
{
    auto c = container(p.id());
    if (c != this) {
        c->unInitialize(p);
        return;
    }

    bool w_init;
    auto val = c->valueHelper(p, false, w_init);
    if (val == nullptr)
        return;

    LanguageMap old_value;

    if (p.propertyType() == PropertyType::Dataset) {
        if (val->item_model != nullptr) {
            val->item_model_initialized = false;
            val->item_model->setRowCount(0);
        }
    } else {
        Z_CHECK_NULL(val->variant);
        old_value = *val->variant;
        val->variant.reset();
    }

    _d->properties[p.id().value()].reset();

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyUnInitialized(p);
    emit sg_propertyChanged(p, old_value);
}

void DataContainer::unInitialize(const PropertyID& property_id)
{
    unInitialize(property(property_id));
}

bool DataContainer::initDataset(const DataProperty& p, int row_count)
{
    if (isInitialized(p))
        return false;

    auto c = container(p.id());
    if (c != this)
        return c->initDataset(p, row_count);

    clearRowHash(p);

    bool w_init;
    c->valueHelper(p, true, w_init); //-V1071
    return w_init;
}

bool DataContainer::initDataset(const PropertyID& property_id, int row_count)
{
    return initDataset(property(property_id), row_count);
}

bool DataContainer::initDataset(int row_count)
{
    return initDataset(structure()->singleDatasetId(), row_count);
}

void DataContainer::setDataset(const DataProperty& p, ItemModel* ds, CloneContainerDatasetMode mode)
{
    Z_CHECK_X(p.propertyType() == PropertyType::Dataset, QString("Property not dataset: %1:%2").arg(structure()->id().value()).arg(p.id().value()));

    auto c = container(p.id());
    if (c != this) {
        c->setDataset(p, ds, mode);
        return;
    }

    clearRowHash(p);

    bool w_init;
    auto val = c->valueHelper(p, true, w_init);

    Z_CHECK(val->item_model.get() != ds);

    // если модель уже создана, то к ней может быть подключено представление, поэтому нельзя ее удалять
    ItemModel* new_ds = val->item_model.get();
    bool need_connect = (new_ds == nullptr);

    if (mode == CloneContainerDatasetMode::CopyPointer) {
        new_ds = ds;

    } else if (mode == CloneContainerDatasetMode::Clone) {
        if (new_ds == nullptr)
            new_ds = new ItemModel(ds->rowCount(), ds->columnCount());
        blockSameProperties();
        Utils::cloneItemModel(ds, new_ds);
        unBlockSameProperties();

    } else if (mode == CloneContainerDatasetMode::MoveContent) {
        if (new_ds == nullptr)
            new_ds = new ItemModel();
        blockSameProperties();
        new_ds->moveData(ds);
        unBlockSameProperties();

    } else {
        Z_HALT_INT;
    }

    if (val->item_model == nullptr)
        val->item_model = std::unique_ptr<ItemModel>(new_ds);
    _d->dataset_id[new_ds] = p.id();

    if (hash()->contains(p.id()))
        hash()->remove(p.id());
    hash()->add(p.id(), new_ds);

    if (need_connect)
        connectToDataset(new_ds);

    if (w_init)
        emit sg_propertyInitialized(p);

    setInvalidate(p, false);
}

void DataContainer::setDataset(const PropertyID& property_id, ItemModel* ds, CloneContainerDatasetMode mode)
{
    setDataset(property(property_id), ds, mode);
}

ItemModel* DataContainer::takeDataset(const PropertyID& property_id)
{
    return takeDataset(property(property_id));
}

ItemModel* DataContainer::takeDataset(const DataProperty& p)
{
    Z_CHECK_X(p.propertyType() == PropertyType::Dataset, QString("Property not dataset: %1:%2").arg(structure()->id().value()).arg(p.id().value()));

    auto c = container(p.id());
    if (c != this)
        return c->takeDataset(p.id());

    bool w_init;
    auto val = c->valueHelper(p, false, w_init);
    Z_CHECK_X(val != nullptr, QString("Property not initialized %1:%2").arg(structure()->id().value()).arg(p.id().value()));

    disconnectFromDataset(val->item_model.get());
    ItemModel* dataset = val->item_model.release();
    createUninitializedDataset(p);

    unInitialize(p);

    return dataset;
}

DataProperty DataContainer::datasetProperty(const QAbstractItemModel* m) const
{
    auto res = _d->dataset_id.find(m);
    if (res != _d->dataset_id.constEnd())
        return structure()->property(res.value());

    if (_d->source != nullptr)
        return _d->source->datasetProperty(m);

    auto datasets = structure()->propertiesByType(PropertyType::Dataset);
    for (auto& p : qAsConst(datasets)) {
        if (Utils::getTopSourceModel(dataset(p)) == Utils::getTopSourceModel(m))
            return p;
    }

    return {};
}

DataProperty DataContainer::datasetProperty() const
{
    return property(structure()->singleDatasetId());
}

QModelIndex DataContainer::cellIndex(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent) const
{
    return dataset(dataset_property)->index(row, column, parent);
}

QModelIndex DataContainer::cellIndex(const PropertyID& dataset_property_id, int row, int column, const QModelIndex& parent) const
{
    return dataset(dataset_property_id)->index(row, column, parent);
}

QModelIndex DataContainer::cellIndex(int row, const DataProperty& column, const QModelIndex& parent) const
{
    return dataset(column.dataset())->index(row, column.pos(), parent);
}

QModelIndex DataContainer::cellIndex(int row, const PropertyID& column_property_id, const QModelIndex& parent) const
{
    return cellIndex(row, property(column_property_id), parent);
}

QModelIndex DataContainer::cellIndex(const DataProperty& cell_property) const
{
    if (!cell_property.isValid())
        return QModelIndex();

    Z_CHECK(cell_property.propertyType() == PropertyType::Cell);
    QModelIndex idx = findDatasetRowID(cell_property.dataset(), cell_property.rowId());
    if (!idx.isValid())
        return QModelIndex();

    return cellIndex(idx.row(), cell_property.column(), idx.parent());
}

Error DataContainer::setCellValue(const DataProperty& dataset_property, int row, const DataProperty& column_property, int column, const QVariant& value, int role, const QModelIndex& parent,
    bool block_signals, QLocale::Language language)
{
    Z_CHECK(dataset_property.isValid());
    Z_CHECK(!isWriteDisabled());

    auto d = dataset(dataset_property);
    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(dataset_property.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(dataset_property.toPrintable()).arg(column).arg(d->columnCount(parent)));

    bool is_blocked = d->signalsBlocked();
    if (block_signals && !is_blocked)
        d->blockSignals(true);

    // если не инициализировано, надо сделать это
    initDataset(dataset_property, 0);

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        role = Qt::DisplayRole;

    Error error;

    if (role == Qt::DisplayRole && column_property == dataset_property.idColumnPropertyId()) {
        // меняется значение в колонке отмеченной PropertyOption::Id, значит надо перегенерить RowId
        clearDatasetRowID(dataset_property, row, parent);
    }

    if (column_property.dataType() == DataType::Bool && role != Qt::CheckStateRole) {
        error = d->setDataHelper(d->index(row, column, parent), value.toBool() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole, language)
                    ? Error()
                    : Error(QString("setCell property error: %1:%2").arg(structure()->id().value()).arg(dataset_property.id().value()));
    }

    if (error.isOk()) {
        QVariant converted;
        if (column_property.isValid() && (role == Qt::DisplayRole || role == Qt::EditRole))
            error = Utils::convertValue(value, column_property.dataType(), Core::locale(LocaleType::UserInterface), ValueFormat::Internal, Core::locale(LocaleType::Universal), converted);
        else
            converted = value;

        if (error.isOk()) {
#ifdef QT_DEBUG
            if (role == Consts::UNIQUE_ROW_COLUMN_ROLE) {
                auto cl = dataset_property.idColumnProperty();
                if (cl.isValid()) {
                    RowID rid = RowID::fromVariant(value);
                    QString key = d->data(row, cl.pos(), parent).toString();
                    Z_CHECK(!rid.isValid() || !rid.isRealKey() || key == rid.key());
                }
            }
#endif
            error = d->setDataHelper(d->index(row, column, parent), converted, role, language)
                        ? Error()
                        : Error(QString("setCell property error: %1:%2").arg(structure()->id().value()).arg(dataset_property.id().value()));
        }
    }

    if (block_signals && !is_blocked)
        d->blockSignals(false);

    return error;
}

Error DataContainer::setCellValue(const DataProperty& dataset_property, int row, int column, const LanguageMap& value, int role, const QModelIndex& parent, bool block_signals)
{
    Z_CHECK(dataset_property.isValid());
    auto d = dataset(dataset_property);

    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(dataset_property.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(dataset_property.toPrintable()).arg(column).arg(d->columnCount(parent)));

    bool is_blocked = d->signalsBlocked();
    if (block_signals && !is_blocked)
        d->blockSignals(true);

    // если не инициализировано, надо сделать это
    initDataset(dataset_property, 0);

    Error error = d->setDataHelperLanguageMap(d->index(row, column, parent), value, role)
                      ? Error()
                      : Error(QString("setCell property error: %1:%2").arg(structure()->id().value()).arg(dataset_property.id().value()));

    if (block_signals && !is_blocked)
        d->blockSignals(false);

    return error;
}

Error DataContainer::setCellValue(const DataProperty& p, int row, int column, const QMap<int, LanguageMap>& value, const QModelIndex& parent, bool block_signals)
{
    Z_CHECK(p.isValid());
    auto d = dataset(p);

    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(p.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(p.toPrintable()).arg(column).arg(d->columnCount(parent)));

    bool is_blocked = d->signalsBlocked();
    if (block_signals && !is_blocked)
        d->blockSignals(true);

    Error error = d->setDataHelperLanguageRoleMap(d->index(row, column, parent), value) ? Error() : Error(QString("setCell property error: %1:%2").arg(structure()->id().value()).arg(p.id().value()));

    if (block_signals && !is_blocked)
        d->blockSignals(false);

    return error;
}

Error DataContainer::setCell(int row, const DataProperty& column, const QVariant& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCellValue(column.dataset(), row, column, column.pos(), value, role, parent, false, propertyLanguage(column, language, true));
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const QVariant& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCell(row, property(column_property_id), value, role, parent, language);
}

Error DataContainer::setCell(int row, const DataProperty& column, const char* value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCell(row, column, QString::fromUtf8(value), role, parent, language);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const char* value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCell(row, column_property_id, QString::fromUtf8(value), role, parent, language);
}

Error DataContainer::setCell(int row, const DataProperty& column, const QString& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCell(row, column, QVariant(value), role, parent, language);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const QString& value, int role, const QModelIndex& parent, QLocale::Language language)
{
    return setCell(row, column_property_id, QVariant(value), role, parent, language);
}

Error DataContainer::setCell(int row, const DataProperty& column, const Numeric& value, int role, const QModelIndex& parent)
{
    return setCell(row, column, value.toVariant(), role, parent);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const Numeric& value, int role, const QModelIndex& parent)
{
    return setCell(row, column_property_id, value.toVariant(), role, parent);
}

Error DataContainer::setCell(int row, const DataProperty& column, const QIcon& value, const QModelIndex& parent)
{
    return setCell(row, column, QVariant::fromValue(value), Qt::DecorationRole, parent);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const QIcon& value, const QModelIndex& parent)
{
    return setCell(row, column_property_id, QVariant::fromValue(value), Qt::DecorationRole, parent);
}

Error DataContainer::setCell(int row, const DataProperty& column, const Uid& value, int role, const QModelIndex& parent)
{
    return setCell(row, column, value.variant(), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const Uid& value, int role, const QModelIndex& parent)
{
    return setCell(row, column_property_id, value.variant(), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, const ID_Wrapper& value, int role, const QModelIndex& parent)
{
    return setCell(row, column, value.value(), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const ID_Wrapper& value, int role, const QModelIndex& parent)
{
    return setCell(row, column_property_id, value.value(), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, bool value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, bool value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, int value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, int value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, uint value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, uint value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, qint64 value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, qint64 value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, quint64 value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, quint64 value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, double value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, double value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, float value, int role, const QModelIndex& parent)
{
    return setCell(row, column, QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, float value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), QVariant(value), role, parent, QLocale::AnyLanguage);
}

Error DataContainer::setCell(int row, const DataProperty& column, const LanguageMap& value, int role, const QModelIndex& parent)
{
    return setCellValue(column.dataset(), row, column.pos(), value, role, parent, false);
}

Error DataContainer::setCell(int row, const PropertyID& column_property_id, const LanguageMap& value, int role, const QModelIndex& parent)
{
    return setCell(row, property(column_property_id), value, role, parent);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const QVariant& value, int role, QLocale::Language language)
{
    auto index = findDatasetRowID(column.dataset(), row_id);
    Z_CHECK(index.isValid());

    return setCell(index.row(), column, value, role, index.parent(), language);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const QVariant& value, int role, QLocale::Language language)
{
    return setCell(row_id, property(column_property_id), value, role, language);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const char* value, int role, QLocale::Language language)
{
    return setCell(row_id, column, QString::fromUtf8(value), role, language);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const char* value, int role, QLocale::Language language)
{
    return setCell(row_id, column_property_id, QString::fromUtf8(value), role, language);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const QString& value, int role, QLocale::Language language)
{
    return setCell(row_id, column, QVariant(value), role, language);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const QString& value, int role, QLocale::Language language)
{
    return setCell(row_id, column_property_id, QVariant(value), role, language);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const Numeric& value, int role)
{
    return setCell(row_id, column, value.toVariant(), role);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const Numeric& value, int role)
{
    return setCell(row_id, column_property_id, value.toVariant(), role);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const Uid& value, int role)
{
    return setCell(row_id, column, value.variant(), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const Uid& value, int role)
{
    return setCell(row_id, column_property_id, value.variant(), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const ID_Wrapper& value, int role)
{
    return setCell(row_id, column, value.value(), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const ID_Wrapper& value, int role)
{
    return setCell(row_id, column_property_id, value.value(), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, bool value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, bool value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, int value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, int value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, uint value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, uint value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, qint64 value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, qint64 value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, quint64 value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, quint64 value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, double value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, double value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, float value, int role)
{
    return setCell(row_id, column, QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, float value, int role)
{
    return setCell(row_id, property(column_property_id), QVariant(value), role, QLocale::AnyLanguage);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const LanguageMap& value, int role)
{
    auto index = findDatasetRowID(column.dataset(), row_id);
    Z_CHECK(index.isValid());

    return setCellValue(column.dataset(), index.row(), column.pos(), value, role, index.parent(), false);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const LanguageMap& value, int role)
{
    return setCell(row_id, property(column_property_id), value, role);
}

Error DataContainer::setCell(const RowID& row_id, const DataProperty& column, const QIcon& value)
{
    return setCell(row_id, column, QVariant::fromValue(value), Qt::DecorationRole);
}

Error DataContainer::setCell(const RowID& row_id, const PropertyID& column_property_id, const QIcon& value)
{
    return setCell(row_id, column_property_id, QVariant::fromValue(value), Qt::DecorationRole);
}

QVariant DataContainer::cellValue(const DataProperty& dataset_property, int row, int column, int role, const QModelIndex& parent, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(dataset_property.isValid());

    auto d = dataset(dataset_property);

    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(dataset_property.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(dataset_property.toPrintable()).arg(column).arg(d->columnCount(parent)));

    QVariant value = d->dataHelper(d->index(row, column, parent), role, propertyLanguage(dataset_property, language, false));
    if (from_lookup) {
        auto& lookup_column = dataset_property.columns().at(column);
        if (lookup_column.lookup() != nullptr) {
            if (lookup_column.lookup()->type() == LookupType::List) {
                value = lookup_column.lookup()->listName(value);

            } else if (lookup_column.lookup()->type() == LookupType::Dataset) {
                Error error;
                ModelPtr source_model;
                Core::getEntityValue(lookup_column.lookup(), QVariant(value), value, source_model, error);
            }
        }
    }

    return value;
}

LanguageMap DataContainer::cellValue(const DataProperty& dataset_property, int row, int column, int role, const QModelIndex& parent) const
{
    Z_CHECK(dataset_property.isValid());

    auto d = dataset(dataset_property);

    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(dataset_property.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(dataset_property.toPrintable()).arg(column).arg(d->columnCount(parent)));

    return d->dataHelperLanguageMap(d->index(row, column, parent), role);
}

QMap<int, LanguageMap> DataContainer::cellValueMap(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent) const
{
    Z_CHECK(dataset_property.isValid());

    auto d = dataset(dataset_property);

    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(dataset_property.toPrintable()).arg(row).arg(d->rowCount(parent)));
    Z_CHECK_X(d->columnCount(parent) > column && column >= 0, QString("%1, column: %2, total: %3").arg(dataset_property.toPrintable()).arg(column).arg(d->columnCount(parent)));

    return d->dataHelperLanguageRoleMap(d->index(row, column, parent));
}

RowID DataContainer::datasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    RowID id;
    if (!_is_row_id_generating && _row_id_generator != nullptr) {
        _is_row_id_generating = true;
        id = _row_id_generator->datasetRowID(dataset_property, row, parent);
        _is_row_id_generating = false;
        return id;
    }

    id = RowID::fromVariant(cellValue(dataset_property, row, 0, Consts::UNIQUE_ROW_COLUMN_ROLE, parent, QLocale::AnyLanguage, false));
    if (!id.isValid() && _row_id_generator == nullptr) {
        id = RowID::createGenerated();
        const_cast<DataContainer*>(this)->setDatasetRowID(dataset_property, row, parent, id);
    }
    return id;
}

void DataContainer::setDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id)
{
    Z_CHECK(row_id.isValid());
    setDatasetRowID_helper(dataset_property, row, parent, row_id);
}

void DataContainer::clearDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent)
{
    setDatasetRowID_helper(dataset_property, row, parent, RowID());
    clearRowHash(dataset_property);
}

class _SortModel : public QSortFilterProxyModel
{
public:
    _SortModel(ItemModel* m, const QList<int>& columns, const QList<Qt::SortOrder>& order, const QList<int>& role)
        : _m(m)
        , _columns(columns)
        , _order(order)
        , _role(role)
    {
        setSourceModel(m); //-V1053
        sort(0); //-V1053
    }

    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override
    {
        for (int i = 0; i < _columns.count(); i++) {
            QVariant left = _m->data(source_left.row(), _columns.at(i), _role.at(i), source_left.parent());
            QVariant right = _m->data(source_right.row(), _columns.at(i), _role.at(i), source_right.parent());

            if (Utils::compareVariant(left, right, CompareOperator::Equal, Core::locale(LocaleType::UserInterface), CompareOption::NoOption))
                continue;

            return Utils::compareVariant(
                left, right, _order.at(i) == Qt::AscendingOrder ? CompareOperator::Less : CompareOperator::More, Core::locale(LocaleType::UserInterface), CompareOption::NoOption);
        }
        return false;
    }

    ItemModel* _m;
    QList<int> _columns;
    QList<Qt::SortOrder> _order;
    QList<int> _role;
};

void DataContainer::sort(const DataProperty& dataset_property, const DataPropertyList& columns, const QList<Qt::SortOrder>& orders, const QList<int>& roles)
{
    QList<int> col_int;
    for (auto& c : columns) {
        col_int << c.pos();
    }

    dataset(dataset_property)->sort(col_int, orders, roles);
}

void DataContainer::sort(const PropertyID& dataset_property_id, const PropertyIDList& columns, const QList<Qt::SortOrder>& orders, const QList<int>& roles)
{
    DataPropertyList cols;
    for (auto& c : columns) {
        cols << property(c);
    }

    sort(property(dataset_property_id), cols, orders, roles);
}

void DataContainer::sort(const DataProperty& dataset_property, FlatItemModelSortFunction sort_function)
{
    dataset(dataset_property)->sort(sort_function);
}

void DataContainer::sort(const PropertyID& dataset_property_id, FlatItemModelSortFunction sort_function)
{
    sort(property(dataset_property_id), sort_function);
}

QModelIndex DataContainer::findDatasetRowID(const DataProperty& dataset_property, const RowID& row_id) const
{
    Z_CHECK(contains(dataset_property));

    std::shared_ptr<QHash<RowID, QModelIndex>> hash = _row_by_id.value(dataset_property);
    if (hash == nullptr) {
        hash = std::make_shared<QHash<RowID, QModelIndex>>();
        _row_by_id[dataset_property] = hash;
        return findRowID_helper(dataset_property, dataset(dataset_property), hash.get(), row_id, QModelIndex());
    } else {
        auto res = hash->constFind(row_id);
        if (res == hash->constEnd())
            return QModelIndex();

        Z_CHECK(res.value().isValid());
        return res.value();
    }
}

QModelIndex DataContainer::findDatasetRowID(const PropertyID& dataset_property_id, const RowID& row_id) const
{
    return findDatasetRowID(property(dataset_property_id), row_id);
}

DataPropertyList DataContainer::getAllRowsProperties(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    DataPropertyList res;
    getAllRowsProperties_helper(res, dataset_property, dataset(dataset_property), parent);
    return res;
}

DataPropertyList DataContainer::getAllRowsProperties(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRowsProperties(property(dataset_property_id), parent);
}

QModelIndexList DataContainer::getAllRowsIndexes(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    QModelIndexList res;
    getAllRowsIndexes_helper(res, dataset_property, dataset(dataset_property), parent);
    return res;
}

QModelIndexList DataContainer::getAllRowsIndexes(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRowsIndexes(property(dataset_property_id), parent);
}

Rows DataContainer::getAllRows(const DataProperty& dataset_property, const QModelIndex& parent) const
{
    Rows res;
    getAllRows_helper(res, dataset_property, dataset(dataset_property), parent);
    return res;
}

Rows DataContainer::getAllRows(const PropertyID& dataset_property_id, const QModelIndex& parent) const
{
    return getAllRows(property(dataset_property_id), parent);
}

QVariantList DataContainer::getColumnValues(const DataProperty& column, const QList<RowID>& rows, int role, const DataFilter* filter, bool valid_only) const
{
    QModelIndexList rows_idx;
    for (auto& r : rows) {
        QModelIndex idx = findDatasetRowID(column.dataset(), r);
        Z_CHECK(idx.isValid());
        rows_idx << idx;
    }

    return getColumnValues(column, rows_idx, role, filter, valid_only);
}

QVariantList DataContainer::getColumnValues(const DataProperty& column, const QModelIndexList& rows, int role, const DataFilter* filter, bool valid_only) const
{
    if (filter != nullptr)
        Z_CHECK(filter->structure() == structure());

    QVariantList res;
    QModelIndexList indexes = getAllRowsIndexes(column.dataset());

    QSet<QModelIndex> rows_set;
    for (int i = 0; i < rows.count(); i++) {
        const QModelIndex& idx = rows.at(i);
        Z_CHECK(idx.isValid());
        rows_set << idx.model()->index(idx.row(), 0, idx.parent());
    }

    for (auto& i : qAsConst(indexes)) {
        if (!rows.isEmpty() && !rows_set.contains(i))
            continue;

        if (filter != nullptr && !filter->proxyIndex(i).isValid())
            continue;

        QVariant value = cell(i.row(), column, role, i.parent());
        if (valid_only && (!value.isValid() || value.isNull() || (value.type() == QVariant::String && value.toString().isEmpty())))
            continue;

        res << value;
    }
    return res;
}

QVariantList DataContainer::getColumnValues(const DataProperty& column, const QList<int>& rows, int role, const DataFilter* filter, bool valid_only) const
{
    QModelIndexList rows_idx;
    for (auto& r : rows) {
        QModelIndex idx = cellIndex(r, column);
        Z_CHECK(idx.isValid());
        rows_idx << idx;
    }

    return getColumnValues(column, rows_idx, role, filter, valid_only);
}

QVariantList DataContainer::getColumnValues(const PropertyID& column, const QList<RowID>& rows, int role, const DataFilter* filter, bool valid_only) const
{
    return getColumnValues(property(column), rows, role, filter, valid_only);
}

QVariantList DataContainer::getColumnValues(const PropertyID& column, const QModelIndexList& rows, int role, const DataFilter* filter, bool valid_only) const
{
    return getColumnValues(property(column), rows, role, filter, valid_only);
}

QVariantList DataContainer::getColumnValues(const PropertyID& column, const QList<int>& rows, int role, const DataFilter* filter, bool valid_only) const
{
    return getColumnValues(property(column), rows, role, filter, valid_only);
}

QVariantList DataContainer::getColumnValues(const PropertyID& column, const Rows& rows, int role, const DataFilter* filter, bool valid_only) const
{
    return getColumnValues(column, rows.toList(), role, filter, valid_only);
}

DataProperty DataContainer::indexColumn(const QModelIndex& index) const
{
    auto dataset = datasetProperty(index.model());
    if (!dataset.isValid() || dataset.columns().count() <= index.column())
        return {};

    return dataset.columns().at(index.column());
}

DataProperty DataContainer::cellProperty(int row, const DataProperty& column, const QModelIndex& parent) const
{
    return cellProperty(datasetRowID(column.dataset(), row, parent), column);
}

DataProperty DataContainer::cellProperty(int row, const PropertyID& column, const QModelIndex& parent) const
{
    return cellProperty(row, property(column), parent);
}

DataProperty DataContainer::cellProperty(const RowID& row_id, const DataProperty& column) const
{
    return DataProperty::cellProperty(DataProperty::rowProperty(column.dataset(), row_id), column);
}

QVariant DataContainer::cell(int row, const DataProperty& column, int role, const QModelIndex& parent, QLocale::Language language, bool from_lookup) const
{
    return cellValue(column.dataset(), row, column.pos(), role, parent, language, from_lookup);
}

QVariant DataContainer::cell(int row, const PropertyID& column_property_id, int role, const QModelIndex& parent, QLocale::Language language, bool from_lookup) const
{
    return cell(row, property(column_property_id), role, parent, language, from_lookup);
}

QVariant DataContainer::cell(const RowID& row_id, const DataProperty& column, int role, QLocale::Language language, bool from_lookup) const
{
    QModelIndex index = findDatasetRowID(column.dataset(), row_id);
    Z_CHECK(index.isValid());
    return cell(index.row(), column, role, index.parent(), language, from_lookup);
}

QVariant DataContainer::cell(const RowID& row_id, const PropertyID& column_property_id, int role, QLocale::Language language, bool from_lookup) const
{
    return cell(row_id, property(column_property_id), role, language, from_lookup);
}

QVariant DataContainer::cell(const DataProperty& cell_property, int role, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(cell_property.isCell());
    return cell(cell_property.rowId(), cell_property.column(), role, language, from_lookup);
}

QVariant DataContainer::cell(const QModelIndex& index, const DataProperty& column, int role, QLocale::Language language, bool from_lookup) const
{
    Z_CHECK(index.isValid());
    Z_CHECK(column.isColumn());
    Z_CHECK(index.model() == dataset(column.dataset()));
    return cell(index.row(), column, role, index.parent(), language, from_lookup);
}

QVariant DataContainer::cell(const QModelIndex& index, const PropertyID& column_property_id, int role, QLocale::Language language, bool from_lookup) const
{
    return cell(index, property(column_property_id), role, language, from_lookup);
}

int DataContainer::rowCount(const DataProperty& p, const QModelIndex& parent) const
{
    return dataset(p)->rowCount(parent);
}

int DataContainer::rowCount(const PropertyID& property_id, const QModelIndex& parent) const
{
    return dataset(property_id)->rowCount(parent);
}

int DataContainer::rowCount(const QModelIndex& parent) const
{
    return dataset()->rowCount(parent);
}

void DataContainer::setColumnCount(const DataProperty& p, int count)
{
    dataset(p)->setColumnCount(count);
}

void DataContainer::setColumnCount(const PropertyID& property_id, int count)
{
    dataset(property_id)->setColumnCount(count);
}

void DataContainer::setColumnCount(int count)
{
    dataset()->setColumnCount(count);
}

int DataContainer::columnCount(const DataProperty& p) const
{
    return dataset(p)->columnCount();
}

int DataContainer::columnCount(const PropertyID& property_id) const
{
    return dataset(property_id)->columnCount();
}

int DataContainer::columnCount() const
{
    return dataset()->columnCount();
}

void DataContainer::setRowCount(const DataProperty& p, int count, const QModelIndex& parent)
{
    if (parent.isValid()) {
        initDataset(p, 0);
        dataset(p)->setRowCount(count, parent);
        return;
    }

    if (initDataset(p, count))
        return;
    dataset(p)->setRowCount(count);
}

void DataContainer::setRowCount(const PropertyID& property_id, int count, const QModelIndex& parent)
{
    setRowCount(property(property_id), count, parent);
}

void DataContainer::setRowCount(int count, const QModelIndex& parent)
{
    setRowCount(structure()->singleDatasetId(), count, parent);
}

int DataContainer::appendRow(const DataProperty& p, const QModelIndex& parent, int num)
{
    return insertRow(p, rowCount(p, parent), parent, num);
}

int DataContainer::appendRow(const PropertyID& property_id, const QModelIndex& parent, int num)
{
    return insertRow(property_id, rowCount(property_id), parent, num);
}

int DataContainer::appendRow(const QModelIndex& parent, int num)
{
    return insertRow(rowCount(parent), parent, num);
}

int DataContainer::insertRow(const DataProperty& p, int row, const QModelIndex& parent, int num)
{
    auto ds = dataset(p);
    Z_CHECK_X(ds->rowCount(parent) >= row && row >= 0, QString("%1, row: %2, total: %3").arg(p.toPrintable()).arg(row).arg(ds->rowCount(parent)));

    if (!isInitialized(p)) {
        // если добавляем строку в не инициализированный набор данных - значит он станет инициализированным
        Z_CHECK(ds->rowCount() == 0);
        initDataset(p, 0);
    }

    ds->insertRows(row, num, parent);

    if (parent.isValid()) {
        int c_count = ds->columnCount();
        if (ds->columnCount(parent) < c_count)
            ds->setColumnCount(c_count, parent);
    }
    return row;
}

int DataContainer::insertRow(const PropertyID& property_id, int row, const QModelIndex& parent, int num)
{
    return insertRow(property(property_id), row, parent, num);
}

int DataContainer::insertRow(int row, const QModelIndex& parent, int num)
{
    return insertRow(structure()->singleDataset(), row, parent, num);
}

void DataContainer::removeRow(const DataProperty& p, int row, const QModelIndex& parent, int num)
{
    Z_CHECK(p.isValid());
    Z_CHECK(!isWriteDisabled());

    auto d = dataset(p);
    Z_CHECK_X(d->rowCount(parent) > row && row >= 0, QString("%1, row: %2, total: %3").arg(p.toPrintable()).arg(row).arg(d->rowCount(parent)));
    d->removeRows(row, num, parent);
}

void DataContainer::removeRow(const PropertyID& property_id, int row, const QModelIndex& parent, int num)
{
    removeRow(property(property_id), row, parent, num);
}

void DataContainer::removeRow(int row, const QModelIndex& parent, int num)
{
    removeRow(structure()->singleDataset(), row, parent, num);
}

void DataContainer::removeRows(const DataProperty& p, const Rows& rows)
{
    Rows sorted = rows.sorted();
    for (int i = sorted.count() - 1; i >= 0; i--) {
        removeRow(p, sorted.at(i).row(), sorted.at(i).parent());
    }
}

void DataContainer::removeRows(const PropertyID& property_id, const Rows& rows)
{
    removeRows(property(property_id), rows);
}

void DataContainer::removeRows(const Rows& rows)
{
    removeRows(structure()->singleDataset(), rows);
}

void DataContainer::moveRows(const DataProperty& dataset_property, int source_row, const QModelIndex& source_parent, int count, int destination_row, const QModelIndex& destination_parent)
{
    Z_CHECK(dataset(dataset_property)->moveRows(source_parent, source_row, count, destination_parent, destination_row));
}

void DataContainer::moveRows(const PropertyID& dataset_property_id, int source_row, const QModelIndex& source_parent, int count, int destination_row, const QModelIndex& destination_parent)
{
    moveRows(property(dataset_property_id), source_row, source_parent, count, destination_row, destination_parent);
}

void DataContainer::moveRows(const DataProperty& dataset_property, int source_row, int count, int destination_row)
{
    moveRows(dataset_property, source_row, QModelIndex(), count, destination_row, QModelIndex());
}

void DataContainer::moveRows(const PropertyID& dataset_property_id, int source_row, int count, int destination_row)
{
    moveRows(property(dataset_property_id), source_row, count, destination_row);
}

void DataContainer::moveRow(const DataProperty& dataset_property, int source_row, int destination_row)
{
    moveRows(dataset_property, source_row, QModelIndex(), 1, destination_row, QModelIndex());
}

void DataContainer::moveRow(const PropertyID& dataset_property_id, int source_row, int destination_row)
{
    moveRow(property(dataset_property_id), source_row, destination_row);
}

QModelIndex DataContainer::rowIndex(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    return dataset(dataset_property)->index(row, 0, parent);
}

QModelIndex DataContainer::rowIndex(const PropertyID& dataset_property_id, int row, const QModelIndex& parent) const
{
    return dataset(dataset_property_id)->index(row, 0, parent);
}

void DataContainer::copyFrom(const DataContainerPtr& source, const DataPropertySet& properties, bool halt_if_error, CloneContainerDatasetMode mode)
{
    copyFrom(source.get(), properties, halt_if_error, mode);
}

void DataContainer::copyFrom(const DataContainer* source, const DataPropertySet& properties, bool halt_if_error, CloneContainerDatasetMode mode)
{
    Z_CHECK_NULL(source);
    Z_CHECK(source->isValid() && isValid());

    if (this == source)
        return;

    blockAllProperties();

    // нельзя просто замещать данные
    _d->any_data.insert(source->_d->any_data);

    DataPropertySet source_props;
    if (!properties.isEmpty())
        source_props = properties;
    else
        source_props = source->structure()->propertiesMain();

    // для всех, которые инициализированны в source
    for (auto& p : qAsConst(source_props)) {
        if (p.isDatasetPart())
            continue;

        if (!structure()->contains(p)) {
            if (halt_if_error)
                Z_HALT(QString("Property not found %1:%2").arg(structure()->id().value()).arg(p.id().value()));
            continue;
        }

        if (!source->isInitialized(p)) {
            unInitialize(p);
            continue;
        }

        bool source_invalidated = source->isInvalidated(p);

        if (p.propertyType() == PropertyType::Dataset) {
            ItemModel* dataset_pointer = nullptr;
            if (mode == CloneContainerDatasetMode::MoveContent) {
                // при перемещении, набор данных source становится невалидным
                auto c = source->container(p.id());
                bool w_init;
                DataContainerValue* val = const_cast<DataContainerValue*>(c->valueHelper(p, false, w_init));
                Z_CHECK_NULL(val);
                dataset_pointer = val->item_model.release();
                Z_CHECK_NULL(dataset_pointer);

            } else {
                dataset_pointer = source->dataset(p);
            }

            setDataset(p, dataset_pointer, mode);

        } else {
            setValue(p, source->value(p));
        }

        if (source_invalidated) {
            // блокируем генерацию сигнала sg_invalidateChanged, т.к. свойство и так было раньше invalidated true
            bool is_signals_blocked = signalsBlocked();
            if (!is_signals_blocked)
                blockSignals(true);

            setInvalidate(p, true);

            if (!is_signals_blocked)
                blockSignals(false);
        }
    }

    unBlockAllProperties();
}

bool DataContainer::isInitialized(const DataPropertySet& p, bool initialized) const
{
    for (auto& prop : p) {
        if (isInitialized(prop) != initialized)
            return false;
    }
    return true;
}

bool DataContainer::isInitialized(const DataPropertyList& p, bool initialized) const
{
    for (auto& prop : p) {
        if (isInitialized(prop) != initialized)
            return false;
    }
    return true;
}

void DataContainer::setProxyContainer(const DataContainerPtr& source, const QMap<DataProperty, DataProperty>& proxy_mapping)
{
    if (_d->source != nullptr && _d->source != source) {
        disconnect(_d->source.get(), &DataContainer::sg_invalidate, this, &DataContainer::sl_sourceInvalidate);
        disconnect(_d->source.get(), &DataContainer::sg_invalidateChanged, this, &DataContainer::sl_sourceInvalidateChanged);
        disconnect(_d->source.get(), &DataContainer::sg_languageChanged, this, &DataContainer::sg_languageChanged);
        disconnect(_d->source.get(), &DataContainer::sg_propertyInitialized, this, &DataContainer::sl_sourcePropertyInitialized);
        disconnect(_d->source.get(), &DataContainer::sg_propertyUnInitialized, this, &DataContainer::sl_sourcePropertyUnInitialized);
        disconnect(_d->source.get(), &DataContainer::sg_allPropertiesBlocked, this, &DataContainer::sl_sourceAllPropertiesBlocked);
        disconnect(_d->source.get(), &DataContainer::sg_allPropertiesUnBlocked, this, &DataContainer::sl_sourceAllPropertiesUnBlocked);
        disconnect(_d->source.get(), &DataContainer::sg_propertyBlocked, this, &DataContainer::sl_sourcePropertyBlocked);
        disconnect(_d->source.get(), &DataContainer::sg_propertyUnBlocked, this, &DataContainer::sl_sourcePropertyUnBlocked);
        disconnect(_d->source.get(), &DataContainer::sg_propertyChanged, this, &DataContainer::sl_sourcePropertyChanged);

        disconnect(_d->source.get(), &DataContainer::sg_dataset_dataChanged, this, &DataContainer::sl_sourceDataset_dataChanged);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_headerDataChanged, this, &DataContainer::sl_sourceDataset_headerDataChanged);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsAboutToBeInserted, this, &DataContainer::sl_sourceDataset_rowsAboutToBeInserted);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsInserted, this, &DataContainer::sl_sourceDataset_rowsInserted);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &DataContainer::sl_sourceDataset_rowsAboutToBeRemoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsRemoved, this, &DataContainer::sl_sourceDataset_rowsRemoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsAboutToBeInserted, this, &DataContainer::sl_sourceDataset_columnsAboutToBeInserted);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsInserted, this, &DataContainer::sl_sourceDataset_columnsInserted);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsAboutToBeRemoved, this, &DataContainer::sl_sourceDataset_columnsAboutToBeRemoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsRemoved, this, &DataContainer::sl_sourceDataset_columnsRemoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_modelAboutToBeReset, this, &DataContainer::sl_sourceDataset_modelAboutToBeReset);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_modelReset, this, &DataContainer::sl_sourceDataset_modelReset);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsAboutToBeMoved, this, &DataContainer::sl_sourceDataset_rowsAboutToBeMoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_rowsMoved, this, &DataContainer::sl_sourceDataset_rowsMoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsAboutToBeMoved, this, &DataContainer::sl_sourceDataset_columnsAboutToBeMoved);
        disconnect(_d->source.get(), &DataContainer::sg_dataset_columnsMoved, this, &DataContainer::sl_sourceDataset_columnsMoved);
    }

    _d->proxy_mapping.clear();
    _d->proxy_mapping_inverted.clear();

    if (source != nullptr) {
        QMap<DataProperty, DataProperty> proxy_mapping_prepared;
        if (proxy_mapping.isEmpty()) {
            for (auto& p : structure()->properties()) {
                if (!source->contains(p.id())) {
                    zf::Core::debPrint(structure()->variant());
                    Z_HALT(QString("Property %1 not contains in %2").arg(p.id().value()).arg(source->id()));
                }

                proxy_mapping_prepared[p] = source->structure()->property(p.id());
            }

        } else
            proxy_mapping_prepared = proxy_mapping;

        for (auto i = proxy_mapping_prepared.constBegin(); i != proxy_mapping_prepared.constEnd(); ++i) {
            Z_CHECK(structure()->contains(i.key()));
            Z_CHECK(source->structure()->contains(i.value()));
            Z_CHECK(i.key().dataType() == i.value().dataType());
            _d->proxy_mapping[i.key().id()] = i.value().id();
            _d->proxy_mapping_inverted[i.value().id()] = i.key().id();
        }
        if (_d->source != source) {
            connect(source.get(), &DataContainer::sg_invalidate, this, &DataContainer::sl_sourceInvalidate);
            connect(source.get(), &DataContainer::sg_invalidateChanged, this, &DataContainer::sl_sourceInvalidateChanged);
            connect(source.get(), &DataContainer::sg_languageChanged, this, &DataContainer::sg_languageChanged);
            connect(source.get(), &DataContainer::sg_propertyInitialized, this, &DataContainer::sl_sourcePropertyInitialized);
            connect(source.get(), &DataContainer::sg_propertyUnInitialized, this, &DataContainer::sl_sourcePropertyUnInitialized);
            connect(source.get(), &DataContainer::sg_allPropertiesBlocked, this, &DataContainer::sl_sourceAllPropertiesBlocked);
            connect(source.get(), &DataContainer::sg_allPropertiesUnBlocked, this, &DataContainer::sl_sourceAllPropertiesUnBlocked);
            connect(source.get(), &DataContainer::sg_propertyBlocked, this, &DataContainer::sl_sourcePropertyBlocked);
            connect(source.get(), &DataContainer::sg_propertyUnBlocked, this, &DataContainer::sl_sourcePropertyUnBlocked);
            connect(source.get(), &DataContainer::sg_propertyChanged, this, &DataContainer::sl_sourcePropertyChanged);

            connect(source.get(), &DataContainer::sg_dataset_dataChanged, this, &DataContainer::sl_sourceDataset_dataChanged);
            connect(source.get(), &DataContainer::sg_dataset_headerDataChanged, this, &DataContainer::sl_sourceDataset_headerDataChanged);
            connect(source.get(), &DataContainer::sg_dataset_rowsAboutToBeInserted, this, &DataContainer::sl_sourceDataset_rowsAboutToBeInserted);
            connect(source.get(), &DataContainer::sg_dataset_rowsInserted, this, &DataContainer::sl_sourceDataset_rowsInserted);
            connect(source.get(), &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &DataContainer::sl_sourceDataset_rowsAboutToBeRemoved);
            connect(source.get(), &DataContainer::sg_dataset_rowsRemoved, this, &DataContainer::sl_sourceDataset_rowsRemoved);
            connect(source.get(), &DataContainer::sg_dataset_columnsAboutToBeInserted, this, &DataContainer::sl_sourceDataset_columnsAboutToBeInserted);
            connect(source.get(), &DataContainer::sg_dataset_columnsInserted, this, &DataContainer::sl_sourceDataset_columnsInserted);
            connect(source.get(), &DataContainer::sg_dataset_columnsAboutToBeRemoved, this, &DataContainer::sl_sourceDataset_columnsAboutToBeRemoved);
            connect(source.get(), &DataContainer::sg_dataset_columnsRemoved, this, &DataContainer::sl_sourceDataset_columnsRemoved);
            connect(source.get(), &DataContainer::sg_dataset_modelAboutToBeReset, this, &DataContainer::sl_sourceDataset_modelAboutToBeReset);
            connect(source.get(), &DataContainer::sg_dataset_modelReset, this, &DataContainer::sl_sourceDataset_modelReset);
            connect(source.get(), &DataContainer::sg_dataset_rowsAboutToBeMoved, this, &DataContainer::sl_sourceDataset_rowsAboutToBeMoved);
            connect(source.get(), &DataContainer::sg_dataset_rowsMoved, this, &DataContainer::sl_sourceDataset_rowsMoved);
            connect(source.get(), &DataContainer::sg_dataset_columnsAboutToBeMoved, this, &DataContainer::sl_sourceDataset_columnsAboutToBeMoved);
            connect(source.get(), &DataContainer::sg_dataset_columnsMoved, this, &DataContainer::sl_sourceDataset_columnsMoved);
        }
    }

    _d->source = source;
}

DataContainerPtr DataContainer::proxyContainer() const
{
    return _d->source;
}

bool DataContainer::isProxyMode() const
{
    return proxyContainer() != nullptr;
}

DataProperty DataContainer::proxyMapping(const DataProperty& p) const
{
    return proxyMapping(p.id());
}

DataProperty DataContainer::proxyMapping(const PropertyID& property_id) const
{
    Z_CHECK(_d->source != nullptr);
    Z_CHECK(structure()->contains(property_id));

    if (!_d->proxy_mapping.contains(property_id))
        return DataProperty();

    return _d->source->structure()->property(_d->proxy_mapping.value(property_id));
}

DataProperty DataContainer::sourceMapping(const DataProperty& source_property) const
{
    return sourceMapping(source_property.id());
}

DataProperty DataContainer::sourceMapping(const PropertyID& source_property_id) const
{
    return structure()->property(_d->proxy_mapping_inverted.value(source_property_id));
}

bool DataContainer::isProxyProperty(const DataProperty& p) const
{
    return isProxyProperty(p.id());
}

bool DataContainer::isProxyProperty(const PropertyID& property_id) const
{
    Z_CHECK(_d->source != nullptr);
    Z_CHECK(structure()->contains(property_id));

    return _d->proxy_mapping.contains(property_id);
}

bool DataContainer::isSourceProperty(const DataProperty& source_property) const
{
    return isSourceProperty(source_property.id());
}

bool DataContainer::isSourceProperty(const PropertyID& source_property_id) const
{
    return _d->proxy_mapping_inverted.contains(source_property_id);
}

void DataContainer::sl_sourceInvalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    emit sg_invalidate(p, invalidate, info);
}

void DataContainer::sl_sourceInvalidateChanged(const zf::DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    if (invalidate && p.isDataset())
        clearRowHash(p);

    emit sg_invalidateChanged(p, invalidate, info);
}

bool DataContainer::isInitialized(const DataProperty& p) const
{
    if (p.propertyType() == PropertyType::Entity || p.options().testFlag(PropertyOption::ClientOnly))
        return true;

    if (p.isDatasetPart())
        return isInitialized(p.dataset());

    bool w_init;
    auto val = valueHelper(p, false, w_init);
    if (p.isDataset()) {
        Z_CHECK_NULL(val);
        return val->item_model_initialized;

    } else {
        return val != nullptr && val->variant != nullptr;
    }
}

bool DataContainer::isInitialized(const PropertyID& property_id) const
{
    return isInitialized(property(property_id));
}

bool DataContainer::isInitialized(const PropertyIDList& property_ids, bool initialized) const
{
    for (auto& id : property_ids) {
        if (isInitialized(id) != initialized)
            return false;
    }
    return true;
}

bool DataContainer::isInitialized(const PropertyIDSet& property_ids, bool initialized) const
{
    for (auto& id : property_ids) {
        if (isInitialized(id) != initialized)
            return false;
    }
    return true;
}

bool DataContainer::isAllInitialized() const
{
    for (auto& p : structure()->propertiesMain()) {
        if (!isInitialized(p))
            return false;
    }
    return true;
}

DataPropertySet DataContainer::initializedProperties() const
{
    DataPropertySet res;
    for (auto& p : structure()->propertiesMain()) {
        if (isInitialized(p))
            res << p;
    }

    return res;
}

DataPropertySet DataContainer::notInitializedProperties() const
{
    DataPropertySet res;
    for (auto& p : structure()->propertiesMain()) {
        if (!isInitialized(p))
            res << p;
    }

    return res;
}

DataPropertySet DataContainer::whichPropertiesInitialized(const DataPropertySet& p, bool initialized) const
{
    DataPropertySet res;
    for (auto& prop : p) {
        if (isInitialized(prop) == initialized)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInitialized(const DataPropertyList& p, bool initialized) const
{
    DataPropertySet res;
    for (auto& prop : p) {
        if (isInitialized(prop) == initialized)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInitialized(const PropertyIDList& property_ids, bool initialized) const
{
    DataPropertySet res;
    for (auto& id : property_ids) {
        auto prop = property(id);
        if (isInitialized(prop) == initialized)
            res << prop;
    }
    return res;
}

DataPropertySet DataContainer::whichPropertiesInitialized(const PropertyIDSet& property_ids, bool initialized) const
{
    DataPropertySet res;
    for (auto& id : property_ids) {
        auto prop = property(id);
        if (isInitialized(prop) == initialized)
            res << prop;
    }
    return res;
}

void DataContainer::detach()
{
    _d.detach();
}

QVariant DataContainer::variant() const
{
    return QVariant::fromValue(*this);
}

DataContainer DataContainer::fromVariant(const QVariant& v)
{
    return v.value<DataContainer>();
}

DataContainer DataContainer::createVariantTable(int row_count, int column_count, const QList<QVariantList>& data, int structure_version, const PropertyID& id, QLocale::Language language)
{
    DataProperty dataset_property = DataProperty::independentDatasetProperty(PropertyID::def(), DataType::Table);
    for (int col = 0; col < column_count; col++) {
        dataset_property << DataProperty::columnProperty(dataset_property, PropertyID(col), DataType::Variant);
    }

    DataContainer d(id, structure_version, {dataset_property});
    d.initDataset(PropertyID::def(), row_count);

    Z_CHECK(data.count() < row_count);
    for (int row = 0; row < row_count; row++) {
        Z_CHECK(data.at(row).count() < column_count);
        for (int col = 0; col < column_count; col++) {
            d.setCellValue(dataset_property, row, DataProperty(), col, data.at(row).at(col), Qt::DisplayRole, QModelIndex(), false, language);
        }
    }

    return d;
}

DataContainer DataContainer::createVariantProperties(const QVariantList& data, int structure_version, const PropertyID& id)
{
    QList<DataProperty> properties;
    QHash<DataProperty, QVariant> values;

    for (int i = 0; i < data.count(); i++) {
        properties << DataProperty::independentFieldProperty(PropertyID(i + Consts::MINUMUM_PROPERTY_ID), DataType::Variant);
        values[properties.last()] = data.at(i);
    }

    return DataContainer(id, structure_version, properties, values);
}

QJsonArray DataContainer::toJson() const
{
    QJsonArray records;

    for (auto& p : structure()->properties()) {
        if (!isInitialized(p) || (p.propertyType() != PropertyType::Field && p.propertyType() != PropertyType::Dataset))
            continue;

        QJsonObject record;
        if (p.propertyType() == PropertyType::Field) {
            auto values = languageMapToJson(valueLanguages(p), p.dataType());
            if (values.isEmpty())
                continue;
            record["values"] = values;

        } else {
            auto values = datasetToJson(p, QModelIndex());
            if (values.isEmpty())
                continue;
            record["dataset"] = values;
        }

        record["property"] = p.id().value();
        records.append(record);
    }

    return records;
}

DataContainerPtr DataContainer::fromJson(const QJsonArray& source, const DataStructurePtr& data_structure, Error& error)
{
    auto container = Z_MAKE_SHARED(DataContainer, data_structure);

    for (const auto& p : source) {
        auto record = p.toObject();
        if (record.isEmpty()) {
            error = Error::jsonError("DataContainer");
            return nullptr;
        }

        auto v = record.value("property");
        if (v.isNull() || v.isUndefined()) {
            error = Error::jsonError("property");
            return nullptr;
        }
        int property_id = v.toInt();
        if (property_id < Consts::MINUMUM_PROPERTY_ID) {
            error = Error::jsonError("property");
            return nullptr;
        }

        DataProperty property = data_structure->property(PropertyID(property_id), false);
        if (!property.isValid()) {
            error = Error::jsonError(QStringLiteral("property not found %1").arg(property_id));
            return nullptr;
        }

        if (record.contains("values")) {
            auto v = record.value("values");
            if (v.isUndefined()) {
                error = Error::jsonError({"data", "values"});
                return nullptr;
            }
            error = container->languageMapFromJson(v, property, QStringList {"values"});
            if (error.isError())
                return nullptr;

        } else if (record.contains("dataset")) {
            auto v = record.value("dataset");
            if (v.isUndefined()) {
                error = Error::jsonError({"data", "dataset"});
                return nullptr;
            }
            error = container->datasetFromJson(v, property, QStringList {"value"}, QModelIndex());
            if (error.isError())
                return nullptr;

        } else {
            error = Error::jsonError("property");
            return nullptr;
        }
    }

    return container;
}

QJsonArray DataContainer::languageMapToJson(LanguageMap values, DataType type) const
{
    QJsonArray json_values;
    for (auto i = values.constBegin(); i != values.constEnd(); ++i) {
        if (i.value().isNull() || !i.value().isValid())
            continue;

        QJsonObject json_lang;
        json_lang["language"] = static_cast<int>(i.key());
        json_lang["value"] = Utils::variantToJsonValue(i.value(), type);

        json_values.append(json_lang);
    }
    return json_values;
}

QJsonArray DataContainer::datasetToJson(const DataProperty& p, const QModelIndex& parent) const
{
    QJsonArray rows;
    auto model = dataset(p);
    int column_count = model->columnCount();
    for (int row = 0; row < model->rowCount(parent); row++) {
        QJsonArray columns;
        for (int col = 0; col < qMin(column_count, p.columns().count()); col++) {
            auto values = languageMapToJson(cellValue(p, row, col, Qt::DisplayRole, parent), p.columns().at(col).dataType());
            if (values.isEmpty())
                continue;

            QJsonObject json_column;
            json_column["id"] = p.columns().at(col).id().value();
            json_column["values"] = values;
            columns.append(json_column);
        }

        QJsonObject json_row;
        json_row["columns"] = columns;

        QJsonArray children = datasetToJson(p, model->index(row, 0, parent));
        if (!children.isEmpty())
            json_row["children"] = children;

        rows.append(json_row);
    }

    return rows;
}

Error DataContainer::languageMapFromJson(const QJsonValue& json, const DataProperty& p, const QStringList& path)
{
    LanguageMap map;
    Error error = languageMapFromJson(json, p, path, map);
    if (error.isError())
        return error;

    return setValue(p, map);
}

Error DataContainer::languageMapFromJson(const QJsonValue& json, const DataProperty& p, const QStringList& path, LanguageMap& map)
{
    if (json.isNull())
        return Error();

    if (!json.isArray())
        return Error::jsonError(path);

    map.clear();

    auto values_json = json.toArray();
    for (const auto& v : qAsConst(values_json)) {
        auto obj = v.toObject();
        if (obj.isEmpty())
            return Error::jsonError(path);

        int language = obj.value("language").toInt(-1);
        if (language < 0)
            return Error::jsonError(path, "language");

        QVariant value;

        auto value_json = obj.value("value");
        if (!value_json.isUndefined())
            value = Utils::variantFromJsonValue(value_json, p.dataType());

        map[static_cast<QLocale::Language>(language)] = value;
    }

    return Error();
}

Error DataContainer::datasetFromJson(const QJsonValue& json, const DataProperty& dataset_property, const QStringList& path, const QModelIndex& parent)
{
    if (!json.isArray())
        return Error::jsonError(path);

    Error error;

    auto rows_json = json.toArray();
    int row = 0;
    for (const auto& row_json : qAsConst(rows_json)) {
        if (!row_json.isObject())
            return Error::jsonError(path);
        auto row_json_obj = row_json.toObject();

        auto columns_json = row_json_obj.value("columns");
        if (!columns_json.isArray())
            return Error::jsonError(path, "columns");

        bool row_added = false;

        auto columns = columns_json.toArray();
        for (const auto& column_json : qAsConst(columns)) {
            if (!column_json.isObject())
                return Error::jsonError(path, "columns");

            auto column_json_obj = column_json.toObject();

            int id = column_json_obj.value("id").toInt(-1);
            DataProperty column_property = (id >= Consts::MINUMUM_PROPERTY_ID ? structure()->property(PropertyID(id), false) : DataProperty());
            if (!column_property.isValid())
                return Error::jsonError(path, {"columns", "id"});

            LanguageMap map;
            error = languageMapFromJson(column_json_obj.value("values"), column_property, path + QStringList {"columns", "values"}, map);
            if (error.isError())
                return error;

            if (!row_added) {
                if (!isInitialized(dataset_property))
                    initDataset(dataset_property, 0);

                row = appendRow(dataset_property, parent);
                row_added = true;
            }

            error = setCell(row, column_property, map, Qt::DisplayRole, parent);
            if (error.isError())
                return error;
        }

        if (!row_json_obj.contains("children"))
            continue;

        auto children_json = row_json_obj.value("children");

        error = datasetFromJson(children_json, dataset_property, path, cellIndex(dataset_property, row, 0, parent));
        if (error.isError())
            return error;
    }

    return Error();
}

void DataContainer::sl_sourcePropertyInitialized(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyInitialized(sourceMapping(p));
}

void DataContainer::sl_sourcePropertyUnInitialized(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyUnInitialized(sourceMapping(p));
}

void DataContainer::sl_sourceAllPropertiesBlocked()
{
    clearRowHash();
    emit sg_allPropertiesBlocked();
}

void DataContainer::sl_sourceAllPropertiesUnBlocked()
{
    clearRowHash();
    emit sg_allPropertiesUnBlocked();
}

void DataContainer::sl_sourcePropertyBlocked(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyBlocked(sourceMapping(p));
}

void DataContainer::sl_sourcePropertyUnBlocked(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyUnBlocked(sourceMapping(p));
}

void DataContainer::sl_sourcePropertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    if (p.propertyType() == PropertyType::Dataset)
        clearRowHash(p);

    emit sg_propertyChanged(prop, old_values);
}

void DataContainer::sl_sourceDataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_dataChanged(prop, topLeft, bottomRight, roles);
}

void DataContainer::sl_sourceDataset_headerDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_headerDataChanged(prop, orientation, first, last);
}

void DataContainer::sl_sourceDataset_rowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeInserted(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_rowsInserted(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_rowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeRemoved(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_rowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_rowsRemoved(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_columnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_columnsAboutToBeInserted(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_columnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_columnsInserted(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_columnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_columnsAboutToBeRemoved(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_columnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_columnsRemoved(prop, parent, first, last);
}

void DataContainer::sl_sourceDataset_modelAboutToBeReset(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_modelAboutToBeReset(prop);
}

void DataContainer::sl_sourceDataset_modelReset(const DataProperty& p)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_modelReset(prop);
}

void DataContainer::sl_sourceDataset_rowsAboutToBeMoved(
    const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeMoved(prop, sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);
}

void DataContainer::sl_sourceDataset_rowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_rowsMoved(prop, parent, start, end, destination, row);
}

void DataContainer::sl_sourceDataset_columnsAboutToBeMoved(
    const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_columnsAboutToBeMoved(prop, sourceParent, sourceStart, sourceEnd, destinationParent, destinationColumn);
}

void DataContainer::sl_sourceDataset_columnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column)
{
    if (!isSourceProperty(p))
        return;

    DataProperty prop = sourceMapping(p);
    if (isPropertyBlocked(prop))
        return;

    emit sg_dataset_columnsMoved(prop, parent, start, end, destination, column);
}

void DataContainer::sl_dataset_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (roles.contains(Consts::UNIQUE_ROW_COLUMN_ROLE))
        return;

    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        clearRowHash(p);

        emit sg_dataset_dataChanged(p, topLeft, bottomRight, roles);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_headerDataChanged(Qt::Orientation orientation, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    emit sg_dataset_headerDataChanged(p, orientation, first, last);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_rowsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeInserted(p, parent, first, last);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_rowsInserted(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        // значения по уполчанию
        for (auto& c : p.columns()) {
            if (!c.defaultValue().isValid())
                continue;

            for (int row = first; row <= last; row++) {
                setCell(row, c, c.defaultValue(), Qt::DisplayRole, parent);
            }
        }

        emit sg_dataset_rowsInserted(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_rowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeRemoved(p, parent, first, last);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_rowsRemoved(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        emit sg_dataset_rowsRemoved(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_columnsAboutToBeInserted(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        clearRowHash(p);
        emit sg_dataset_columnsAboutToBeInserted(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }
}

void DataContainer::sl_dataset_columnsInserted(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        emit sg_dataset_columnsInserted(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_columnsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        clearRowHash(p);
        emit sg_dataset_columnsAboutToBeRemoved(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }
}

void DataContainer::sl_dataset_columnsRemoved(const QModelIndex& parent, int first, int last)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p)) {
        emit sg_dataset_columnsRemoved(p, parent, first, last);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_modelAboutToBeReset()
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    clearRowHash(p);
    emit sg_dataset_modelAboutToBeReset(p);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_modelReset()
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        clearRowHash(p);
        emit sg_dataset_modelReset(p);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, true);
}

void DataContainer::sl_dataset_rowsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    clearRowHash(p);
    emit sg_dataset_rowsAboutToBeMoved(p, sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_rowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        emit sg_dataset_rowsMoved(p, parent, start, end, destination, row);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, false);
}

void DataContainer::sl_dataset_columnsAboutToBeMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (isPropertyBlocked(p))
        return;

    emit sg_dataset_columnsAboutToBeMoved(p, sourceParent, sourceStart, sourceEnd, destinationParent, destinationColumn);
    emit sg_propertyChanged(p, {});
}

void DataContainer::sl_dataset_columnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column)
{
    ItemModel* m = qobject_cast<ItemModel*>(sender());
    Z_CHECK_NULL(m);

    DataProperty p = datasetProperty(m);
    if (!isPropertyBlocked(p)) {
        emit sg_dataset_columnsMoved(p, parent, start, end, destination, column);
        emit sg_propertyChanged(p, {});
    }

    fillSameDatasets(p, false);
}

void DataContainer::init()
{
}

void DataContainer::setDatasetRowID_helper(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id)
{
    QModelIndex index = cellIndex(dataset_property.id(), row, 0, parent);
    bool signals_blocked = index.model()->signalsBlocked();
    if (!signals_blocked)
        const_cast<QAbstractItemModel*>(index.model())->blockSignals(true);

    Z_CHECK_ERROR(setCellValue(dataset_property, row, DataProperty(), 0, row_id.isValid() ? row_id.variant() : QVariant(), Consts::UNIQUE_ROW_COLUMN_ROLE, parent, false, QLocale::AnyLanguage));
    if (!signals_blocked)
        const_cast<QAbstractItemModel*>(index.model())->blockSignals(false);
}

QLocale::Language DataContainer::propertyLanguage(const DataProperty& p, QLocale::Language language, bool writing) const
{
    if (p.options() & PropertyOption::MultiLanguage) {
        return language == QLocale::AnyLanguage ? this->language() : language;
    } else {
        if (writing && language != QLocale::AnyLanguage)
            Core::logError(QStringLiteral("Property not MultiLanguage: %1:%2").arg(structure()->entityCode().value()).arg(p.id().value()));

        return QLocale::AnyLanguage;
    }
}

DataContainer::DataSourceInfo* DataContainer::dataSourceInfoBySource(const DataProperty& source) const
{
    initDataSourceInfo();
    return _data_source_by_source->value(source).get();
}

QList<std::shared_ptr<DataContainer::DataSourceTargetInfo>> DataContainer::dataSourceInfoByModel(Model* model) const
{
    Z_CHECK_NULL(model);
    initDataSourceInfo();
    return _data_source_by_model->values(model);
}

void DataContainer::initDataSourceInfo() const
{
    if (_data_source_by_source != nullptr)
        return;

    _data_source_by_source = std::make_unique<QMap<DataProperty, std::shared_ptr<DataSourceInfo>>>();
    _data_source_by_model = std::make_unique<QMultiMap<Model*, std::shared_ptr<DataSourceTargetInfo>>>();

    auto fields = structure()->propertiesByType(PropertyType::Field);
    for (auto& p : qAsConst(fields)) {
        auto ds_links = p.links(PropertyLinkType::DataSource);
        Z_CHECK(ds_links.count() <= 1);

        if (ds_links.isEmpty())
            continue;

        auto& data_source = ds_links.constFirst();

        auto source_property = property(data_source->linkedPropertyId());

        auto data_column = DataProperty::property(data_source->dataSourceEntity(), data_source->dataSourceColumnId());
        Z_CHECK(data_column.propertyType() == PropertyType::ColumnFull);

        auto key_column = DataStructure::structure(data_source->dataSourceEntity())->datasetColumnsByOptions(data_column.dataset(), PropertyOption::Id);
        Z_CHECK_X(!key_column.isEmpty(), QStringLiteral("PropertyOption::Id - %1::%2").arg(data_source->dataSourceEntity().code()).arg(data_column.dataset().id().value()));

        auto info = _data_source_by_source->value(source_property);
        if (info == nullptr) {
            info = Z_MAKE_SHARED(DataSourceInfo);
            info->source = source_property;
            _data_source_by_source->insert(info->source, info);
        }

        // сохраняем указатель на модель dataSource
        Error error;
        auto model = Core::getModel<Model>(data_source->dataSourceEntity(), {}, {data_column.dataset()}, error);
        if (error.isError())
            Z_HALT(error);

        if (!_data_source_by_model->contains(model.get()))
            connect(model.get(), &Model::sg_finishLoad, this, &DataContainer::sl_dataSourceFinishLoad);

        auto target_info = Z_MAKE_SHARED(DataSourceTargetInfo);
        target_info->source_info = info.get();
        target_info->target = p;
        target_info->data_column = data_column;
        target_info->key_column = key_column.first();
        target_info->model = model;
        info->targets << target_info;

        _data_source_by_model->insert(model.get(), target_info);
    }
}

Error DataContainer::processDataSourceChanged(const DataProperty& p)
{
    Z_CHECK(p.propertyType() == PropertyType::Field);

    auto info = dataSourceInfoBySource(p);
    if (info == nullptr)
        return Error();

    Error error;
    for (auto& target_info : qAsConst(info->targets)) {
        auto err = processDataSourceChangedHelper(target_info.get());
        if (err.isError())
            error << err;
    }

    return error;
}

Error DataContainer::processDataSourceChangedHelper(DataContainer::DataSourceTargetInfo* target_info)
{
    if (target_info->model->data()->isInvalidated(target_info->key_column.dataset())) {
        target_info->model->load({}, {target_info->key_column.dataset()});
        return Error(); // обновление будет после загрузки
    }

    if (target_info->model->isLoading())
        return Error(); // обновление будет после загрузки

    // обновляем данные
    QVariant target_value;
    auto rows = target_info->model->hash()->findRows(target_info->key_column, value(target_info->source_info->source));
    if (!rows.isEmpty())
        target_value = target_info->model->cell(rows.first().row(), target_info->data_column, Qt::DisplayRole, rows.first().parent());

    return setValue(target_info->target, target_value);
}

void DataContainer::sl_dataSourceFinishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error)
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    auto m = qobject_cast<Model*>(sender());
    auto info_list = dataSourceInfoByModel(m);
    Z_CHECK(!info_list.isEmpty());

    for (auto& target_info : qAsConst(info_list)) {
        auto err = processDataSourceChangedHelper(target_info.get());
        if (err.isError())
            Z_HALT(err);
    }
}

void DataContainer::connectToDataset(QAbstractItemModel* dataset) const
{
    Z_CHECK_NULL(dataset);

    connect(dataset, &QAbstractItemModel::dataChanged, this, &DataContainer::sl_dataset_dataChanged);
    connect(dataset, &QAbstractItemModel::headerDataChanged, this, &DataContainer::sl_dataset_headerDataChanged);
    connect(dataset, &QAbstractItemModel::rowsAboutToBeInserted, this, &DataContainer::sl_dataset_rowsAboutToBeInserted);
    connect(dataset, &QAbstractItemModel::rowsInserted, this, &DataContainer::sl_dataset_rowsInserted);
    connect(dataset, &QAbstractItemModel::rowsAboutToBeRemoved, this, &DataContainer::sl_dataset_rowsAboutToBeRemoved);
    connect(dataset, &QAbstractItemModel::rowsRemoved, this, &DataContainer::sl_dataset_rowsRemoved);
    connect(dataset, &QAbstractItemModel::columnsAboutToBeInserted, this, &DataContainer::sl_dataset_columnsAboutToBeInserted);
    connect(dataset, &QAbstractItemModel::columnsInserted, this, &DataContainer::sl_dataset_columnsInserted);
    connect(dataset, &QAbstractItemModel::columnsAboutToBeRemoved, this, &DataContainer::sl_dataset_columnsAboutToBeRemoved);
    connect(dataset, &QAbstractItemModel::columnsRemoved, this, &DataContainer::sl_dataset_columnsRemoved);
    connect(dataset, &QAbstractItemModel::modelAboutToBeReset, this, &DataContainer::sl_dataset_modelAboutToBeReset);
    connect(dataset, &QAbstractItemModel::modelReset, this, &DataContainer::sl_dataset_modelReset);
    connect(dataset, &QAbstractItemModel::rowsAboutToBeMoved, this, &DataContainer::sl_dataset_rowsAboutToBeMoved);
    connect(dataset, &QAbstractItemModel::rowsMoved, this, &DataContainer::sl_dataset_rowsMoved);
    connect(dataset, &QAbstractItemModel::columnsAboutToBeMoved, this, &DataContainer::sl_dataset_columnsAboutToBeMoved);
    connect(dataset, &QAbstractItemModel::columnsMoved, this, &DataContainer::sl_dataset_columnsMoved);
}

void DataContainer::disconnectFromDataset(QAbstractItemModel* dataset) const
{
    Z_CHECK_NULL(dataset);

    disconnect(dataset, &QAbstractItemModel::dataChanged, this, &DataContainer::sl_dataset_dataChanged);
    disconnect(dataset, &QAbstractItemModel::headerDataChanged, this, &DataContainer::sl_dataset_headerDataChanged);
    disconnect(dataset, &QAbstractItemModel::rowsAboutToBeInserted, this, &DataContainer::sl_dataset_rowsAboutToBeInserted);
    disconnect(dataset, &QAbstractItemModel::rowsInserted, this, &DataContainer::sl_dataset_rowsInserted);
    disconnect(dataset, &QAbstractItemModel::rowsAboutToBeRemoved, this, &DataContainer::sl_dataset_rowsAboutToBeRemoved);
    disconnect(dataset, &QAbstractItemModel::rowsRemoved, this, &DataContainer::sl_dataset_rowsRemoved);
    disconnect(dataset, &QAbstractItemModel::columnsAboutToBeInserted, this, &DataContainer::sl_dataset_columnsAboutToBeInserted);
    disconnect(dataset, &QAbstractItemModel::columnsInserted, this, &DataContainer::sl_dataset_columnsInserted);
    disconnect(dataset, &QAbstractItemModel::columnsAboutToBeRemoved, this, &DataContainer::sl_dataset_columnsAboutToBeRemoved);
    disconnect(dataset, &QAbstractItemModel::columnsRemoved, this, &DataContainer::sl_dataset_columnsRemoved);
    disconnect(dataset, &QAbstractItemModel::modelAboutToBeReset, this, &DataContainer::sl_dataset_modelAboutToBeReset);
    disconnect(dataset, &QAbstractItemModel::modelReset, this, &DataContainer::sl_dataset_modelReset);
    disconnect(dataset, &QAbstractItemModel::rowsAboutToBeMoved, this, &DataContainer::sl_dataset_rowsAboutToBeMoved);
    disconnect(dataset, &QAbstractItemModel::rowsMoved, this, &DataContainer::sl_dataset_rowsMoved);
    disconnect(dataset, &QAbstractItemModel::columnsAboutToBeMoved, this, &DataContainer::sl_dataset_columnsAboutToBeMoved);
    disconnect(dataset, &QAbstractItemModel::columnsMoved, this, &DataContainer::sl_dataset_columnsMoved);
}

const DataContainer* DataContainer::container(const PropertyID& property_id) const
{
    Z_CHECK(isValid());
    Z_CHECK_X(_d->data_structure->contains(property_id), QString("Property not found: %1:%2").arg(structure()->id().value()).arg(property_id.value()));

    if (_d->source != nullptr && _d->proxy_mapping.contains(property_id))
        return _d->source.get();

    return this;
}

DataContainer* DataContainer::container(const PropertyID& property_id)
{
    Z_CHECK(isValid());
    Z_CHECK_X(_d->data_structure->contains(property_id), QString("Property not found: %1:%2").arg(structure()->id().value()).arg(property_id.value()));

    if (_d->source != nullptr && _d->proxy_mapping.contains(property_id))
        return _d->source.get();

    return this;
}

// два следующих метода одинаковые из-за особенностей реализации PIMPL
// наверное можно исключить дублирование кода, но надо делать это очень аккуратно, чтобы не налететь на клонирование данных
const DataContainerValue* DataContainer::valueHelper(const DataProperty& prop, bool init, bool& was_initialized) const
{
    auto c = container(prop.id());
    if (c != this) {
        return c->valueHelper(prop, init, was_initialized);
    }

    Z_CHECK_X(_d->properties.count() > prop.id().value(), QString("property not found %1").arg(prop.id().value()));
    DataContainerValue* val = _d->properties.at(prop.id().value()).get();
    if (val == nullptr && (init || prop.isDataset())) {
        DataContainer_SharedData* self = const_cast<DataContainer_SharedData*>(_d.data());
        self->properties[prop.id().value()] = Z_MAKE_SHARED(DataContainerValue);
        self->properties.at(prop.id().value())->property = structure()->property(prop.id());
        val = self->properties.at(prop.id().value()).get();

        if (prop.isDataset()) {
            // для наборов данных создаем контейнер всегда, т.к. к ним надо подключать ItemView
            val->item_model = std::make_unique<ItemModel>(0, prop.columnCount());
            connectToDataset(val->item_model.get());

            if (init) {
                val->item_model_initialized = true;
                was_initialized = true;
            }

        } else
            was_initialized = true;

    } else {
        if (prop.isDataset() && init) {
            if (val->item_model_initialized) {
                was_initialized = false;
            } else {
                val->item_model_initialized = true;
                was_initialized = true;
            }

        } else {
            was_initialized = false;
        }
    }

    return val;
}

DataContainerValue* DataContainer::valueHelper(const DataProperty& prop, bool init, bool& was_initialized)
{
    auto c = container(prop.id());
    if (c != this) {
        return c->valueHelper(prop, init, was_initialized);
    }

    Z_CHECK_X(_d->properties.count() > prop.id().value(), QString("property not found %1").arg(prop.id().value()));
    DataContainerValue* val = _d->properties.at(prop.id().value()).get();
    DataContainer_SharedData* self = const_cast<DataContainer_SharedData*>(_d.data());

    if (val == nullptr && (init || prop.isDataset())) {
        auto val_sptr = Z_MAKE_SHARED(DataContainerValue);
        self->properties[prop.id().value()] = val_sptr;
        val_sptr->property = structure()->property(prop.id());
        val = val_sptr.get();

        if (prop.isDataset()) {
            // для наборов данных создаем контейнер всегда, т.к. к ним надо подключать ItemView
            val->item_model = std::make_unique<ItemModel>(0, prop.columnCount());
            connectToDataset(val->item_model.get());

            if (init) {
                val->item_model_initialized = true;
                was_initialized = true;
            }

        } else {
            was_initialized = true;
        }

    } else {
        if (prop.isDataset() && init) {
            if (val->item_model_initialized) {
                was_initialized = false;
            } else {
                val->item_model_initialized = true;
                was_initialized = true;
            }

        } else {
            was_initialized = false;
        }
    }

    return val;
}

void DataContainer::toStream(QDataStream& s) const
{
    s << isValid();
    if (!isValid())
        return;

    s << id();

    s << structure()->propertiesMain().count();
    for (auto& p : structure()->propertiesMain()) {
        bool initialized = isInitialized(p);
        s << initialized;
        if (!initialized)
            continue;

        s << p.id();

        if (p.propertyType() == PropertyType::Dataset) {
            Utils::itemModelToStream(s, dataset(p));

        } else {
            s << value(p);
        }
    }
}

DataContainerPtr DataContainer::fromStream(QDataStream& s, const DataStructurePtr& data_structure, Error& error)
{
    Z_CHECK_NULL(data_structure);
    error.clear();

    using namespace zf;
    bool valid;
    s >> valid;
    if (!valid)
        return Z_MAKE_SHARED(DataContainer);

    QString id;
    s >> id;

    int count;
    s >> count;
    QList<QPair<DataProperty, QVariant>> values;
    QMap<int, ItemModel*> datasets;

    for (int i = 0; i < count; i++) {
        bool initialized;
        s >> initialized;
        if (!initialized)
            continue;

        int property_id;
        s >> property_id;

        if (property_id < Consts::MINUMUM_PROPERTY_ID || !data_structure->contains(PropertyID(property_id))) {
            error = Error(QStringLiteral("DataContainer::fromStream property not found: %1").arg(id));
            return nullptr;
        }

        DataProperty p = data_structure->property(PropertyID(property_id));
        Z_CHECK(p.isValid());
        if (p.propertyType() == PropertyType::Dataset) {
            auto dataset = new ItemModel;
            dataset->setObjectName(p.toPrintable());
            error = Utils::itemModelFromStream(s, dataset);
            if (error.isError()) {
                delete dataset;
                qDeleteAll(datasets);
                return nullptr;
            }
            datasets[property_id] = dataset;

        } else {
            QVariant value;
            s >> value;
            values << QPair<DataProperty, QVariant> {p, value};
        }

        if (s.status() != QDataStream::Ok)
            break;
    }

    if (s.status() != QDataStream::Ok) {
        qDeleteAll(datasets);
        error = Error::corruptedDataError("DataContainer::fromStream corrupted data");
        return nullptr;
    }

    auto container = Z_MAKE_SHARED(DataContainer, data_structure);
    for (auto& v : qAsConst(values)) {
        container->setValue(v.first, v.second);
    }
    for (auto it = datasets.constBegin(); it != datasets.constEnd(); ++it) {
        container->initDataset(PropertyID(it.key()), 0);
        container->setDataset(PropertyID(it.key()), it.value(), CloneContainerDatasetMode::CopyPointer);
    }

    return container;
}

DataHashed* DataContainer::hash() const
{
    if (_d->find_by_columns_hash == nullptr) {
        auto const_this = const_cast<DataContainer*>(this);
        const_this->_d->find_by_columns_hash = std::make_unique<DataHashed>();
        bool test = false;
        for (auto& p : structure()->properties()) {
            if (!p.isDataset())
                continue;

            const_this->_d->find_by_columns_hash->add(p.id(), dataset(p));
            test = true;
        }
        Z_CHECK(test);
    }

    return _d->find_by_columns_hash.get();
}

void DataContainer::datasetToDebug(QDebug debug, const DataProperty& dataset_property) const
{
    ItemModel* m = dataset(dataset_property);

    if (m->columnCount() == 0) {
        debug << "no columns";
        return;
    }

    auto columns = dataset_property.columns();

    debug << '\n';
    Core::beginDebugOutput();
    debug << Core::debugIndent();

    for (int col = 0; col < columns.count(); col++) {
        debug << columns.at(col).name().simplified();
        if (col < columns.count() - 1)
            debug << " | ";
    }

    datasetToDebugHelper(debug, m, QModelIndex());

    Core::endDebugOutput();
}

void DataContainer::debugInternal() const
{
}

ResourceManager* DataContainer::resourceManager() const
{
    if (_d->resource_manager == nullptr) {
        auto const_this = const_cast<DataContainer*>(this);
        const_this->_d->resource_manager = std::make_unique<ResourceManager>();
        hash()->setResourceManager(const_this->_d->resource_manager.get());
    }
    return _d->resource_manager.get();
}

AnyData* DataContainer::getAnyData(const QString& key) const
{
    return _d->any_data.value(key).get();
}

void DataContainer::addAnyData(const QString& key, AnyData* data, bool replace)
{
    Z_CHECK(!key.isEmpty());
    Z_CHECK_NULL(data);

    if (_d->any_data.contains(key) && !replace)
        return;

    _d->any_data[key] = std::shared_ptr<AnyData>(data->clone());
}

void DataContainer::setRowIdGenerator(ModuleDataObject* generator)
{
    _row_id_generator = generator;
    clearRowHash();
}

zf::DataProperty zf::DataContainer::cellProperty(const QModelIndex& index, bool halt_if_column_not_exists) const
{
    Z_CHECK(index.isValid());
    auto dataset = datasetProperty(index.model());
    Z_CHECK(dataset.isValid());

    auto column = indexColumn(index);
    if (!column.isValid()) {
        if (halt_if_column_not_exists)
            Z_HALT_INT;
        else
            return {};
    }

    return cellProperty(index.row(), column, index.parent());
}

DataProperty DataContainer::rowProperty(const DataProperty& dataset, const RowID& row_id) const
{
    return DataStructure::propertyRow(dataset, row_id);
}

DataProperty DataContainer::rowProperty(const PropertyID& dataset_id, const RowID& row_id) const
{
    return rowProperty(property(dataset_id), row_id);
}

DataProperty DataContainer::rowProperty(const DataProperty& dataset, int row, const QModelIndex& parent) const
{
    return rowProperty(dataset, datasetRowID(dataset, row, parent));
}

DataProperty DataContainer::rowProperty(const PropertyID& dataset_id, int row, const QModelIndex& parent) const
{
    return rowProperty(property(dataset_id), row, parent);
}

AnyData::~AnyData()
{
}

} // namespace zf

QDataStream& operator<<(QDataStream& out, const zf::DataContainer& obj)
{
    out << obj.isValid();
    if (obj.isValid()) {
        out << *obj.structure();
        obj.toStream(out);
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, zf::DataContainer& obj)
{
    obj = zf::DataContainer();

    bool valid;
    in >> valid;
    if (!valid)
        return in;

    zf::DataStructurePtr structure;
    in >> structure;
    if (in.status() != QDataStream::Ok || structure == nullptr) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    zf::Error error;
    auto container = zf::DataContainer::fromStream(in, structure, error);
    if (error.isError())
        zf::Core::logError(error);

    if (in.status() != QDataStream::Ok || error.isError()) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    obj = *container;
    return in;
}

QDataStream& operator<<(QDataStream& out, const zf::DataContainerPtr& obj)
{
    if (obj == nullptr)
        return out << zf::DataContainer();
    return out << *obj;
}

QDataStream& operator>>(QDataStream& in, zf::DataContainerPtr& obj)
{
    if (obj == nullptr)
        obj = Z_MAKE_SHARED(zf::DataContainer);

    return in >> *obj;
}

QDebug operator<<(QDebug debug, const zf::DataContainer* c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    using namespace zf;

    if (c == nullptr) {
        debug << "nullptr";

    } else if (!c->isValid()) {
        debug << "invalid";

    } else {
        debug << '\n';
        Core::beginDebugOutput();
        debug << Core::debugIndent() << "language: " << c->language() << '\n';
        debug << Core::debugIndent() << "allPropertiesBlocked: " << c->isAllPropertiesBlocked();

        auto fields = c->structure()->propertiesByType(PropertyType::Field);
        std::sort(fields.begin(), fields.end(), [](const DataProperty& p1, const DataProperty& p2) -> bool { return p1.naturalIndex() < p2.naturalIndex(); });
        if (!fields.isEmpty()) {
            debug << '\n';
            debug << Core::debugIndent() << "fields:" << '\n';
            Core::beginDebugOutput();
            for (int i = 0; i < fields.count(); i++) {
                auto f = fields.at(i);

                QString name;
                if (f.id().value() == f.name().simplified())
                    name = QString("field(%1%2): ").arg(f.id().value()).arg(c->isInvalidated(f) ? ",invalidated" : "");
                else
                    name = QString("field(%1,%2%3): ").arg(f.id().value()).arg(f.name().simplified()).arg(c->isInvalidated(f) ? ",invalidated" : "");

                debug << Core::debugIndent() << name;
                if (!c->isInitialized(f)) {
                    debug << "not initialized";
                } else {
                    _variantToDebug(debug, c->value(f));
                }

                if (i != fields.count() - 1)
                    debug << '\n';
            }
            Core::endDebugOutput();
        }

        auto datasets = c->structure()->propertiesByType(PropertyType::Dataset);
        if (!datasets.isEmpty()) {
            debug << '\n';
            debug << Core::debugIndent() << "datasets:" << '\n';
            Core::beginDebugOutput();
            for (int i = 0; i < datasets.count(); i++) {
                auto ds = datasets.at(i);

                debug << Core::debugIndent() << QString("dataset(%1,%2):").arg(ds.id().value()).arg(ds.name().simplified());
                if (!c->isInitialized(ds)) {
                    if (c->dataset(ds)->rowCount() > 0)
                        debug << " ERROR: not initialized, row count:" << c->dataset(ds)->rowCount();
                    else
                        debug << " not initialized";
                } else {
                    c->datasetToDebug(debug, ds);
                }

                if (i != datasets.count() - 1)
                    debug << '\n';
            }
            Core::endDebugOutput();
        }

        Core::endDebugOutput();
    }

    return debug;
}

QDebug operator<<(QDebug debug, const zf::DataContainerPtr c)
{
    return debug << c.get();
}

QDebug operator<<(QDebug debug, const zf::DataContainer& c)
{
    return debug << &c;
}
