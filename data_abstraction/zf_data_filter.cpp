#include "zf_data_filter.h"
#include "zf_core.h"
#include "zf_proxy_item_model.h"
#include "zf_condition.h"
#include "zf_view.h"

namespace zf
{
DataFilter::DataFilter(ModuleDataObject* source, I_ConditionFilter* i_condition_filter, I_DatasetVisibleInfo* i_convert_item_value)
    : QObject()
    , _source(source)
    , _i_condition_filter(i_condition_filter)
    , _i_convert_item_value(i_convert_item_value)
{
    Z_CHECK_NULL(source);
}

DataFilter::~DataFilter()
{
}

ModuleDataObject* DataFilter::source() const
{
    return _source;
}

DataContainerPtr DataFilter::data() const
{
    return source()->data();
}

DataStructurePtr DataFilter::structure() const
{
    return source()->structure();
}

DataProperty DataFilter::property(const PropertyID& property_id) const
{
    return source()->property(property_id);
}

DataHashed* DataFilter::hash() const
{
    if (_hash == nullptr) {
        _resource_manager = std::make_unique<ResourceManager>();

        _hash = std::make_unique<DataHashed>();
        _hash->setResourceManager(_resource_manager.get());

        for (auto& p : structure()->propertiesByType(PropertyType::Dataset)) {
            if (!data()->isInitialized(p))
                data()->initDataset(p, 0);
            _hash->add(p.id(), proxyDataset(p));
        }
    }
    return _hash.get();
}

void DataFilter::refilter(const DataProperty& dataset_property)
{
    lazyInit();
    proxyDataset(dataset_property)->invalidateFilter();
    emit sg_refiltered(dataset_property);
}

void DataFilter::refilter(const PropertyID& dataset_property)
{
    refilter(property(dataset_property));
}

void DataFilter::resort(const DataProperty& dataset_property)
{
    auto proxy = proxyDataset(dataset_property);

    // такое извращение, т.к. не найдена нормальная функция пересортировки QSortFilterProxyModel
    // для Qt 5.14.2 setDynamicSortFilter(true) вызывает пересортировку и больше ничего
    if (proxy->dynamicSortFilter()) {
        proxy->setDynamicSortFilter(false);
        proxy->setDynamicSortFilter(true);
    } else {
        proxy->setDynamicSortFilter(true);
        proxy->setDynamicSortFilter(false);
    }
}

void DataFilter::resort(const PropertyID& dataset_property_id)
{
    resort(property(dataset_property_id));
}

ProxyItemModel* DataFilter::proxyDataset(const DataProperty& dataset_property) const
{
    lazyInit();
    Z_CHECK(dataset_property.isValid() && dataset_property.propertyType() == PropertyType::Dataset);

    auto proxy_info = _dataset_by_prop.value(dataset_property);
    Z_CHECK_NULL(proxy_info);
    return proxy_info->proxy_item_model.get();
}

ProxyItemModel* DataFilter::proxyDataset(const PropertyID& property_id) const
{
    return proxyDataset(property(property_id));
}

QModelIndex DataFilter::sourceIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    lazyInit();

    if (_dataset_by_pointer.contains(index.model())) {
        // это не прокси
        return index;
    }

    QModelIndex proxy_index = index;

    std::shared_ptr<_DatasetInfo> proxy_info = _proxy_by_pointer.value(proxy_index.model());
    if (proxy_info == nullptr) {
        auto proxy_model = qobject_cast<const QAbstractProxyModel*>(proxy_index.model());
        while (true) {
            proxy_index = proxy_model->mapToSource(proxy_index);
            if (proxy_index.isValid()) {
                proxy_info = _proxy_by_pointer.value(proxy_index.model());
                if (proxy_info != nullptr)
                    break;

                proxy_model = qobject_cast<const QAbstractProxyModel*>(proxy_index.model());
                if (proxy_model == nullptr)
                    break;
            }
        }
    }

    Z_CHECK_X(proxy_info != nullptr, "invalid index model");
    return proxy_info->proxy_item_model->mapToSource(proxy_index);
}

QModelIndex DataFilter::proxyIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    lazyInit();

    if (!_dataset_by_pointer.contains(index.model())) {
        // это не source
        return index;
    }

    std::shared_ptr<_DatasetInfo> source_info = _dataset_by_pointer.value(index.model());
    Z_CHECK_X(source_info != nullptr, "invalid index model");
    return source_info->proxy_item_model->mapFromSource(index);
}

int DataFilter::sourceRow(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    return sourceIndex(proxyDataset(dataset_property)->index(row, 0, parent)).row();
}

int DataFilter::sourceRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent) const
{
    return sourceIndex(proxyDataset(dataset_property_id)->index(row, 0, parent)).row();
}

int DataFilter::proxyRow(const DataProperty& dataset_property, int row, const QModelIndex& parent) const
{
    return proxyIndex(proxyDataset(dataset_property)->index(row, 0, parent)).row();
}

int DataFilter::proxyRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent) const
{
    return proxyIndex(proxyDataset(dataset_property_id)->index(row, 0, parent)).row();
}

bool DataFilter::isEmpty(const DataProperty& dataset_property) const
{
    return proxyDataset(dataset_property)->rowCount() == 0;
}

bool DataFilter::isEmpty(const PropertyID& property_id) const
{
    return isEmpty(property(property_id));
}

void DataFilter::installExternalFilter(I_DataFilter* f)
{
    Z_CHECK_NULL(f);
    Z_CHECK(!_external_filters.contains(f));

    QObject* obj = dynamic_cast<QObject*>(f);
    Z_CHECK_NULL(obj);

    connect(obj, &QObject::destroyed, this, &DataFilter::sl_externalFilterDestroyed);

    _external_filters << f;
    _external_filters_map[obj] = f;
}

void DataFilter::removeExternalFilter(I_DataFilter* f)
{
    Z_CHECK_NULL(f);
    Z_CHECK(_external_filters.removeOne(f));

    QObject* obj = dynamic_cast<QObject*>(f);
    Z_CHECK_NULL(obj);

    disconnect(obj, &QObject::destroyed, this, &DataFilter::sl_externalFilterDestroyed);

    Z_CHECK(_external_filters_map.remove(obj));
}

I_DataFilter* DataFilter::currentExternalFilter() const
{
    return _external_filters.isEmpty() || _disable_external ? nullptr : _external_filters.constLast();
}

void DataFilter::setEasyFilter(
    const DataPropertyList& columns, const QVariantList& values, const QList<CompareOperator> ops, const QList<int> roles, const QList<CompareOptions>& options)
{
    QList<QVariantList> values_prep;
    for (auto& v : values) {
        values_prep << QVariantList {v};
    }

    setEasyFilterHelper(columns, values_prep, ops, roles, options);
}

void DataFilter::removeEasyFilter(const DataProperty& dataset)
{
    lazyInit();

    Z_CHECK(dataset.isValid() && dataset.propertyType() == PropertyType::Dataset);
    auto info = _dataset_by_prop.value(dataset);
    Z_CHECK_NULL(info);
    info->easy_filter.clear();

    refilter(dataset);
}

void DataFilter::removeEasyFilter(const PropertyID& dataset_property_id)
{
    removeEasyFilter(property(dataset_property_id));
}

QList<std::shared_ptr<const DataFilter::EasyFilter>> DataFilter::easyFilter(const DataProperty& dataset) const
{
    auto info = _dataset_by_prop.value(dataset);
    Z_CHECK_NULL(info);

    QList<std::shared_ptr<const DataFilter::EasyFilter>> res;
    for (auto& f : qAsConst(info->easy_filter)) {
        res << f;
    }
    return res;
}

QList<std::shared_ptr<const DataFilter::EasyFilter>> DataFilter::easyFilter(const PropertyID& dataset_property_id) const
{
    return easyFilter(property(dataset_property_id));
}

void DataFilter::setEasyFilter(const DataProperty& column, const QVariant& value, CompareOperator op, int role, CompareOptions options)
{
    setEasyFilter(DataPropertyList {column}, QVariantList {value}, QList<CompareOperator> {op}, QList<int> {role}, QList<CompareOptions> {options});
}

void DataFilter::setEasyFilter(
    const PropertyIDList& columns, const QVariantList& values, const QList<CompareOperator> ops, const QList<int> roles, const QList<CompareOptions>& options)
{
    DataPropertyList p;
    for (auto& c : qAsConst(columns)) {
        p << property(c);
    }
    setEasyFilter(p, values, ops, roles, options);
}

void DataFilter::setEasyFilter(const PropertyID& column_property_id, const QVariant& value, CompareOperator op, int role, CompareOptions options)
{
    setEasyFilter(property(column_property_id), value, op, role, options);
}

void DataFilter::setEasyFilter(const DataProperty& column, const QVariantList& values, CompareOperator op, int role, CompareOptions options)
{
    setEasyFilterHelper({column}, {values}, {op}, {role}, {options});
}

void DataFilter::setEasyFilter(const PropertyID& column, const QVariantList& values, CompareOperator op, int role, CompareOptions options)
{
    setEasyFilter(property(column), values, op, role, options);
}

void DataFilter::setEasySort(const DataProperty& column, Qt::SortOrder order, int role)
{
    lazyInit();

    Z_CHECK(column.propertyType() == PropertyType::ColumnFull);

    auto info = _dataset_by_prop.value(column.dataset());
    Z_CHECK_NULL(info);

    if (info->easy_sort_column == column && info->easy_sort_order == order && info->easy_sort_role == role)
        return;

    info->easy_sort_column = column;
    info->easy_sort_order = order;
    info->easy_sort_role = role;
    info->easy_sort_dataset_column_index = -1;

    if (info->easy_sort_column.isValid()) {
        info->easy_sort_dataset_column_index = structure()->column(info->easy_sort_column);
        info->proxy_item_model->sort(info->easy_sort_dataset_column_index, info->easy_sort_order);

    } else {
        resort(column.dataset());
    }
}

void DataFilter::setEasySort(const PropertyID& column_property_id, Qt::SortOrder order, int role)
{
    setEasySort(property(column_property_id), order, role);
}

DataProperty DataFilter::easySortColumn(const DataProperty& dataset) const
{
    Z_CHECK(dataset.propertyType() == PropertyType::Dataset);

    lazyInit();
    auto info = _dataset_by_prop.value(dataset);
    Z_CHECK_NULL(info);
    return info->easy_sort_column;
}

DataProperty DataFilter::easySortColumn(const PropertyID& dataset_property_id) const
{
    return easySortColumn(property(dataset_property_id));
}

Qt::SortOrder DataFilter::easySortOrder(const DataProperty& dataset) const
{
    lazyInit();
    auto info = _dataset_by_prop.value(dataset);
    Z_CHECK_NULL(info);
    return info->easy_sort_order;
}

Qt::SortOrder DataFilter::easySortOrder(const PropertyID& dataset_property_id) const
{
    return easySortOrder(property(dataset_property_id));
}

int DataFilter::easySortRole(const DataProperty& dataset) const
{
    lazyInit();
    auto info = _dataset_by_prop.value(dataset);
    Z_CHECK_NULL(info);
    return info->easy_sort_role;
}

int DataFilter::easySortRole(const PropertyID& dataset_property_id) const
{
    return easySortRole(property(dataset_property_id));
}

bool DataFilter::filterAcceptsRow(const DataProperty& dataset_property, int row, const QModelIndex& parent, bool& exclude_hierarchy)
{
    Q_UNUSED(dataset_property)
    Q_UNUSED(row)
    Q_UNUSED(parent)
    Q_UNUSED(exclude_hierarchy)

    return true;
}

int DataFilter::lessThan(const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right)
{
    Q_UNUSED(dataset_property)
    Q_UNUSED(source_left)
    Q_UNUSED(source_right)

    return 0;
}

void DataFilter::sl_propertyInitialized(const DataProperty& p)
{
    if (p.propertyType() != PropertyType::Dataset)
        return;
}

void DataFilter::sl_propertyUnInitialized(const DataProperty& p)
{
    if (p.propertyType() != PropertyType::Dataset)
        return;
}

void DataFilter::sl_allPropertiesUnBlocked()
{
    for (auto& p : _source->structure()->propertiesByType(PropertyType::Dataset)) {
        refilter(p);
    }
}

void DataFilter::sl_propertyUnBlocked(const DataProperty& p)
{
    if (p.propertyType() == PropertyType::Dataset)
        refilter(p);
}

void DataFilter::sl_conditionFilterChanged(const zf::DataProperty& dataset)
{
    refilter(dataset);
}

void DataFilter::lazyInit() const
{
    if (_initialized)
        return;

    _initialized = true;
    const_cast<DataFilter*>(this)->setObjectName(_source->objectName());

    View* view = qobject_cast<View*>(_source);
    if (view != nullptr) {
        if (_i_condition_filter == nullptr)
            _i_condition_filter = view;
        if (_i_convert_item_value == nullptr)
            _i_convert_item_value = view;
    }

    if (_i_condition_filter != nullptr) {
        connect(dynamic_cast<QObject*>(_i_condition_filter), SIGNAL(sg_conditionFilterChanged(zf::DataProperty)), this,
            SLOT(sl_conditionFilterChanged(zf::DataProperty)));
    }

    connect(_source->data().get(), &DataContainer::sg_propertyInitialized, this, &DataFilter::sl_propertyInitialized);
    connect(_source->data().get(), &DataContainer::sg_propertyUnInitialized, this, &DataFilter::sl_propertyUnInitialized);
    connect(_source->data().get(), &DataContainer::sg_allPropertiesUnBlocked, this, &DataFilter::sl_allPropertiesUnBlocked);
    connect(_source->data().get(), &DataContainer::sg_propertyUnBlocked, this, &DataFilter::sl_propertyUnBlocked);

    for (auto& p : _source->structure()->propertiesByType(PropertyType::Dataset)) {
        const_cast<DataFilter*>(this)->initDatasetProxy(p);
    }
}

void DataFilter::initDatasetProxy(const DataProperty& dataset_property)
{
    Z_CHECK(dataset_property.propertyType() == PropertyType::Dataset);
    Z_CHECK(structure()->contains(dataset_property));

    std::shared_ptr<_DatasetInfo> ds_info = _dataset_by_prop.value(dataset_property);
    if (ds_info != nullptr) {
        // удаляем информацию о прокси
        Z_CHECK(_proxy_by_pointer.remove(ds_info->proxy_item_model.get()));
        // удаляем информацию о наборе данных
        Z_CHECK(_dataset_by_pointer.remove(ds_info->item_model));
        Z_CHECK(_dataset_by_prop.remove(ds_info->dataset_property));
    }

    // вносим информацию о наборе данных
    ds_info = Z_MAKE_SHARED(_DatasetInfo);
    ds_info->dataset_property = dataset_property;
    ds_info->item_model = data()->dataset(dataset_property);
    _dataset_by_pointer[ds_info->item_model] = ds_info;
    _dataset_by_prop[dataset_property] = ds_info;

    // создаем прокси
    ds_info->proxy_item_model = std::make_unique<ProxyItemModel>(this, dataset_property);

    // вносим информацию о прокси
    _proxy_by_pointer[ds_info->proxy_item_model.get()] = ds_info;
}

bool DataFilter::filterAcceptsRowHelper(const DataProperty& dataset_property, int source_row, const QModelIndex& source_parent, bool& exclude_hierarchy)
{
    lazyInit();

    if (_source->data()->isPropertyBlocked(dataset_property))
        return false;

    bool res;

    I_DataFilter* ext = currentExternalFilter();
    if (ext != nullptr) {
        res = ext->externalFilterAcceptsRow(_source, this, dataset_property, source_row, source_parent, exclude_hierarchy);

    } else {
        auto info = _dataset_by_prop.value(dataset_property);
        Z_CHECK_NULL(info);

        if (!info->easy_filter.isEmpty()) {
            for (auto& f : qAsConst(info->easy_filter)) {
                QVariant value = info->item_model->index(source_row, f->dataset_column_index, source_parent).data(f->role);
                bool found = false;
                for (auto& v : qAsConst(f->values)) {
                    if (Utils::compareVariant(value, v, f->op, nullptr, f->options)) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return false;
            }
        }

        res = filterAcceptsRow(dataset_property, source_row, source_parent, exclude_hierarchy);
    }

    if (res && _i_condition_filter != nullptr)
        res = _i_condition_filter->calculateConditionFilter(dataset_property, source_row, source_parent);

    return res;
}

bool DataFilter::lessThanHelper(const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right)
{
    lazyInit();

    if (_source->data()->isPropertyBlocked(dataset_property))
        return false;

    int res;

    I_DataFilter* ext = currentExternalFilter();
    if (ext != nullptr)
        res = ext->externalLessThan(_source, this, dataset_property, source_left, source_right);
    else
        res = lessThan(dataset_property, source_left, source_right);
    if (res != 0)
        return res < 0 ? true : false;

    auto info = _dataset_by_prop.value(dataset_property);
    Z_CHECK_NULL(info);

    QVariant left_value;
    QVariant right_value;
    CompareOperator compare_operator;
    Qt::ItemDataRole role;
    QModelIndex left_index;
    QModelIndex right_index;

    if (ext == nullptr && info->easy_sort_column.isValid()) {
        compare_operator = info->easy_sort_order == Qt::AscendingOrder ? CompareOperator::Less : CompareOperator::More;
        role = static_cast<Qt::ItemDataRole>(info->easy_sort_role);

        left_index = info->item_model->index(source_left.row(), info->easy_sort_dataset_column_index, source_left.parent());
        right_index = info->item_model->index(source_right.row(), info->easy_sort_dataset_column_index, source_right.parent());

    } else {
        compare_operator = CompareOperator::Less;
        role = Qt::DisplayRole;

        left_index = source_left;
        right_index = source_right;
    }

    left_value = left_index.data(role);
    right_value = right_index.data(role);

    if (_i_convert_item_value != nullptr) {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            QString left_string = Utils::variantToString(left_value);
            QString left_string_converted = left_string;
            QList<ModelPtr> data_not_ready;
            _i_convert_item_value->convertDatasetItemValue(left_index, left_value, VisibleValueOption::Application, left_string_converted, data_not_ready);
            if (!data_not_ready.isEmpty()) {
                // сортируем по пустой строке
                left_value.clear();
                right_value.clear();

                // подписываемся на окончание загрузки lookup
                for (auto& dnr : qAsConst(data_not_ready)) {
                    bool is_new = true;
                    for (auto& wi : qAsConst(info->lookup_load_waiting_info)) {
                        if (wi.lookup_model == dnr) {
                            is_new = false;
                            break;
                        }
                    }
                    if (is_new) {
                        auto connection = connect(dnr.get(), &Model::sg_finishLoad, this, &DataFilter::sl_sortLookupModelLoaded);
                        info->lookup_load_waiting_info << _DatasetInfo::LookupLoadWaitingInfo {dnr, connection};
                    }
                }

            } else {
                if (left_string_converted != left_string)
                    left_value = left_string_converted;
            }

            if (data_not_ready.isEmpty()) {
                // данные получены - можем показывать
                QString right_string = Utils::variantToString(right_value);
                QString right_string_converted = right_string;
                _i_convert_item_value->convertDatasetItemValue(
                    right_index, right_value, VisibleValueOption::Application, right_string_converted, data_not_ready);
                Z_CHECK(data_not_ready.isEmpty());

                if (right_string_converted != right_string)
                    right_value = right_string_converted;
            }
        }
    }

    if (left_index.column() < dataset_property.columns().count()) {
        if (!left_value.isValid() || left_value.isNull() || (left_value.type() == QVariant::String && left_value.toString().isEmpty())) {
            if (dataset_property.columns().at(left_index.column()).dataType() == DataType::Bool)
                left_value = (Qt::CheckState)left_index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        }
        if (!right_value.isValid() || right_value.isNull() || (right_value.type() == QVariant::String && right_value.toString().isEmpty())) {
            if (dataset_property.columns().at(left_index.column()).dataType() == DataType::Bool)
                right_value = (Qt::CheckState)right_index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        }
    }

    return Utils::compareVariant(left_value, right_value, compare_operator, Core::locale(LocaleType::UserInterface), CompareOption::NoOption);
}

void DataFilter::setEasyFilterHelper(const DataPropertyList& columns, const QList<QVariantList>& values, const QList<CompareOperator> ops,
    const QList<int> roles, const QList<CompareOptions>& options)
{
    Z_CHECK(!columns.isEmpty() && columns.count() == values.count());
    if (!ops.isEmpty())
        Z_CHECK(columns.count() == ops.count());
    if (!roles.isEmpty())
        Z_CHECK(columns.count() == roles.count());
    if (!options.isEmpty())
        Z_CHECK(columns.count() == options.count());

    lazyInit();

    DataPropertySet cleared_filters;

    for (int i = 0; i < columns.count(); ++i) {
        auto& column = columns.at(i);
        auto& value = values.at(i);
        auto op = ops.isEmpty() ? CompareOperator::Equal : ops.at(i);
        auto role = roles.isEmpty() ? Qt::DisplayRole : roles.at(i);
        auto option = options.isEmpty() ? CompareOption::NoOption : options.at(i);

        Z_CHECK(column.isValid() && column.propertyType() == PropertyType::ColumnFull);
        auto info = _dataset_by_prop.value(column.dataset());
        Z_CHECK_NULL(info);

        if (!cleared_filters.contains(column.dataset())) {
            // очищаем фильтр
            cleared_filters << column.dataset();
            info->easy_filter.clear();
        }

        auto f = Z_MAKE_SHARED(EasyFilter);
        f->column = column;
        f->values = value;
        f->role = role;
        f->op = op;
        f->options = option;
        f->dataset_column_index = structure()->column(column);

        info->easy_filter << f;
    }

    for (auto& p : qAsConst(cleared_filters)) {
        refilter(p);
    }
}

void DataFilter::sl_sortLookupModelLoaded(const Error&, const LoadOptions&, const DataPropertySet&)
{
    Model* m_finished = qobject_cast<Model*>(sender());

    QList<ModelPtr> keep;
    for (auto& dataset : structure()->propertiesByType(PropertyType::Dataset)) {
        auto info = _dataset_by_prop.value(dataset);
        Z_CHECK_NULL(info);
        if (info->lookup_load_waiting_info.isEmpty())
            continue;

        for (int i = info->lookup_load_waiting_info.count() - 1; i >= 0; i--) {
            auto& wi = info->lookup_load_waiting_info.at(i);
            if (wi.lookup_model.get() == m_finished) {
                disconnect(wi.connection);
                keep << wi.lookup_model; // чтобы не удалилось до рефильтрации
                info->lookup_load_waiting_info.removeAt(i);
                break;
            }
        }

        if (info->lookup_load_waiting_info.isEmpty()) {
            // все загрузилось
            resort(dataset);
        }
    }
}

void DataFilter::sl_externalFilterDestroyed(QObject* obj)
{
    auto f = _external_filters_map.value(obj);
    Z_CHECK_NULL(f);
    Z_CHECK(_external_filters.removeOne(f));
    Z_CHECK(_external_filters_map.remove(obj));
}

bool DataFilter::externalFilterAcceptsRow(
    ModuleDataObject* source, I_DataFilter* original_filter, const DataProperty& dataset_property, int row, const QModelIndex& parent, bool& exclude_hierarchy)
{
    Q_UNUSED(original_filter);
    Z_CHECK(source = _source);
    Z_CHECK(!_disable_external);
    _disable_external = true;
    bool res = filterAcceptsRowHelper(dataset_property, row, parent, exclude_hierarchy);
    _disable_external = false;
    return res;
}

int DataFilter::externalLessThan(ModuleDataObject* source, I_DataFilter* original_filter, const DataProperty& dataset_property, const QModelIndex& source_left,
    const QModelIndex& source_right)
{
    Q_UNUSED(original_filter);
    Z_CHECK(source = _source);
    Z_CHECK(!_disable_external);
    _disable_external = true;
    int res = lessThanHelper(dataset_property, source_left, source_right);
    _disable_external = false;
    return res;
}

bool DataFilter::EasyFilter::operator==(const DataFilter::EasyFilter& f) const
{
    return column == f.column && op == f.op && role == f.role && options == f.options
           && (values.count() == f.values.count() && Utils::contains(values, f.values));
}

} // namespace zf
