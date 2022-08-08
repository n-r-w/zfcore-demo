#pragma once

#include "zf_global.h"
#include "zf_data_container.h"
#include "zf_i_dataset_visible_info.h"
#include "zf_i_data_filter.h"
#include <QPointer>

namespace zf
{
class ProxyItemModel;
class ModuleDataObject;
class I_ConditionFilter;

//! Фильтрация и сортировка данных ModuleDataObject
class ZCORESHARED_EXPORT DataFilter : public QObject, public I_DataFilter
{
    Q_OBJECT
public:
    DataFilter(
        //! Источник данных
        ModuleDataObject* source,
        //! Интерфейс для условных фильтров
        I_ConditionFilter* i_condition_filter = nullptr,
        //! Интерфейс для преобразования значений наборов данных
        I_DatasetVisibleInfo* i_convert_item_value = nullptr);
    virtual ~DataFilter();

    //! Источник данных
    ModuleDataObject* source() const;

    //! Данные модели
    DataContainerPtr data() const;
    //! Структура данных модели
    DataStructurePtr structure() const;

    /*! Свойство по его идентификатору.
     * Доступны только поля (Field) и наборы данных (Dataset). Колонки, строки и ячейки не могут быть
     * получены через данный метод */
    DataProperty property(const PropertyID& property_id) const;

    //! Методы хэшированного поиска
    DataHashed* hash() const;

    //! Перефильтровать данные
    void refilter(const DataProperty& dataset_property);
    void refilter(const PropertyID& dataset_property_id);

    //! Пересортировать данные
    void resort(const DataProperty& dataset_property);
    void resort(const PropertyID& dataset_property_id);

    /*! Набор данных ProxyItemModel, созданный на базе оригинального ItemModel. Набор данных должен быть инициализирован
     * Используется для сортировки и фильтрации (см. filterAcceptsRow) */
    ProxyItemModel* proxyDataset(const DataProperty& dataset_property) const;
    ProxyItemModel* proxyDataset(const PropertyID& dataset_property_id) const;

    //! Индекс, приведенный из прокси набора данных к базовому
    QModelIndex sourceIndex(const QModelIndex& index) const;
    //! Индекс, приведенный из базовому набора данных к прокси
    QModelIndex proxyIndex(const QModelIndex& index) const;

    //! Строка, приведенная из прокси набора данных к базовому (-1, если не найдено)
    int sourceRow(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex()) const;
    int sourceRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Строка, приведенная из базовому набора данных к прокси (-1, если не найдено)
    int proxyRow(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex()) const;
    int proxyRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Есть ли данные в отфильтрованном наборе данных
    bool isEmpty(const DataProperty& dataset_property) const;
    //! Есть ли данные в отфильтрованном наборе данных
    bool isEmpty(const PropertyID& property_id) const;

    //! Установить внешний интерфейс фильтрации. В один момент времени действует только один интерфейс, который был добавлен последним.
    //! Отменяет любые easyFilter/easySort и реализацию методов filterAcceptsRow/lessThen в данном классе
    //! Класс, реализующий I_DataFilter должен быть наследником QObject
    void installExternalFilter(I_DataFilter* f);
    //! Удалить внешний интерфейс фильтрации
    void removeExternalFilter(I_DataFilter* f);
    //! Текущий интерфейс фильтрации
    I_DataFilter* currentExternalFilter() const;

public: // фильтрация по условию
    struct EasyFilter
    {
        //! колонка
        DataProperty column;
        //! значение
        QList<QVariant> values;
        //! оператор сравнения
        CompareOperator op = CompareOperator::Equal;
        //! роль
        int role = Qt::DisplayRole;
        //! Параметры
        CompareOptions options = CompareOption::NoOption;

        bool operator==(const EasyFilter& f) const;

    private:
        //! Номер колонки
        int dataset_column_index = -1;

        friend class DataFilter;
    };

    //! Задать фильтрацию по нескольким колонкам
    //! Фильтрация EasyFilter заменяется на новую только для наборов данных, колонки которых указаны
    void setEasyFilter(
        //! Колонки набора данных
        const DataPropertyList& columns,
        //! Значения для сравнения. Количество должно совпадать с количеством в columns
        const QVariantList& values,
        //! Операторы сравнения: [значение набора данных] [оператор] [value]
        //! Если не задано, то CompareOperator::Equal. Иначе количество должно совпадать с количеством в columns
        const QList<CompareOperator> ops = {},
        //! Роли для получения значения набора данных. Если не задано, то Qt::DisplayRole. Иначе количество должно совпадать с количеством в columns
        const QList<int> roles = {},
        //! Параметры фильтрации. Если не задано, то NoOption. Иначе количество должно совпадать с количеством в columns
        const QList<CompareOptions>& options = {});
    //! Задать фильтрацию по одной колонке
    //! Заменяет EasyFilter для набора данных этой колонки
    void setEasyFilter(
        //! Колонка набора данных
        const DataProperty& column,
        //! Значение для сравнения
        const QVariant& value,
        //! Оператор сравнения: [значение набора данных] [оператор] [value]
        CompareOperator op = CompareOperator::Equal,
        //! Роль для получения значения набора данных
        int role = Qt::DisplayRole,
        //! Параметры фильтрации
        CompareOptions options = CompareOption::NoOption);

    //! Задать фильтрацию по нескольким колонкам
    //! Фильтрация EasyFilter заменяется на новую только для наборов данных, колонки которых указаны
    void setEasyFilter(
        //! Колонки набора данных (идентификаторы)
        const PropertyIDList& columns,
        //! Значения для сравнения. Количество должно совпадать с количеством в columns
        const QVariantList& values,
        //! Операторы сравнения: [значение набора данных] [оператор] [value]
        //! Если не задано, то CompareOperator::Equal. Иначе количество должно совпадать с количеством в columns
        const QList<CompareOperator> ops = {},
        //! Роли для получения значения набора данных. Если не задано, то Qt::DisplayRole. Иначе количество должно совпадать с количеством в columns
        const QList<int> roles = {},
        //! Параметры фильтрации. Если не задано, то NoOption. Иначе количество должно совпадать с количеством в columns
        const QList<CompareOptions>& options = {});
    //! Задать фильтрацию по одной колонке
    //! Заменяет EasyFilter для набора данных этой колонки
    void setEasyFilter(const PropertyID& column_property_id, const QVariant& value,
        //! Оператор сравнения: [значение набора данных] [оператор] [value]
        CompareOperator op = CompareOperator::Equal,
        //! Роль
        int role = Qt::DisplayRole,
        //! Параметры фильтрации
        CompareOptions options = CompareOption::NoOption);

    //! Задать фильтрацию по одной колонке. Позволяет указать несколько вариантов значения
    //! Заменяет EasyFilter для набора данных этой колонки
    void setEasyFilter(
        //! Колонка набора данных
        const DataProperty& column,
        //! Значения для сравнения
        const QVariantList& values,
        //! Оператор сравнения: [значение набора данных] [оператор] [value]
        CompareOperator op = CompareOperator::Equal,
        //! Роль для получения значения набора данных
        int role = Qt::DisplayRole,
        //! Параметры фильтрации
        CompareOptions options = CompareOption::NoOption);

    //! Задать фильтрацию по одной колонке. Позволяет указать несколько вариантов значения
    //! Заменяет EasyFilter для набора данных этой колонки
    void setEasyFilter(
        //! Колонка набора данных
        const PropertyID& column,
        //! Значения для сравнения
        const QVariantList& values,
        //! Оператор сравнения: [значение набора данных] [оператор] [value]
        CompareOperator op = CompareOperator::Equal,
        //! Роль для получения значения набора данных
        int role = Qt::DisplayRole,
        //! Параметры фильтрации
        CompareOptions options = CompareOption::NoOption);

    //! Убрать фильтр для набора данных
    void removeEasyFilter(
        //! Набор данных
        const DataProperty& dataset);
    //! Убрать фильтр для набора данных
    void removeEasyFilter(
        //! Набор данных
        const PropertyID& dataset_property_id);

    //! Фильрация для надора даных. Если не задано, то nullptr
    QList<std::shared_ptr<const DataFilter::EasyFilter>> easyFilter(const DataProperty& dataset) const;
    //! Фильрация для надора даных. Если не задано, то nullptr
    QList<std::shared_ptr<const DataFilter::EasyFilter>> easyFilter(const PropertyID& dataset_property_id) const;

public: // сортировка по условию
    //! Задать сортировку по одной колонке
    //! Не использовать совместно с View::setSortDatasetEnabled/sortDataset, TableView/TreeView::setSortingEnabled/sortByColumn
    void setEasySort(const DataProperty& column, Qt::SortOrder order = Qt::AscendingOrder, int role = Qt::DisplayRole);
    //! Задать сортировку по одной колонке
    //! Не использовать совместно с View::setSortDatasetEnabled/sortDataset, TableView/TreeView::setSortingEnabled/sortByColumn
    void setEasySort(const PropertyID& column_property_id, Qt::SortOrder order = Qt::AscendingOrder, int role = Qt::DisplayRole);

    //! Сортировка по одной колонке - колонка
    DataProperty easySortColumn(const DataProperty& dataset) const;
    //! Сортировка по одной колонке - колонка
    DataProperty easySortColumn(const PropertyID& dataset_property_id) const;

    //! Сортировка по одной колонке - порядок
    Qt::SortOrder easySortOrder(const DataProperty& dataset) const;
    //! Сортировка по одной колонке - порядок
    Qt::SortOrder easySortOrder(const PropertyID& dataset_property_id) const;

    //! Сортировка по одной колонке - роль
    int easySortRole(const DataProperty& dataset) const;
    //! Сортировка по одной колонке - роль
    int easySortRole(const PropertyID& dataset_property_id) const;

protected:
    //! Отфильтровать строку набора данных. По умолчанию - не фильтровать
    virtual bool filterAcceptsRow(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка source
        int row,
        //! Родитель строки
        const QModelIndex& parent,
        //! Безусловно исключить строку из иерархии
        bool& exclude_hierarchy);
    /*! Сортировка. Результат:
     * -1 = left < right
     * +1 = left > right
     * 0 = использовать стандартную сортировку (по умолчанию) */
    virtual int lessThan(
        //! Набор данных
        const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right);

signals:
    void sg_refiltered(
        //! Набор данных
        const zf::DataProperty& dataset_property);

private slots:
    //! Свойство было инициализировано
    void sl_propertyInitialized(const zf::DataProperty& p);
    //! Свойство стало неинициализировано
    void sl_propertyUnInitialized(const zf::DataProperty& p);

    //! Все свойства источника данных разблокированы
    void sl_allPropertiesUnBlocked();
    //! Свойство источника данных было разаблокировано
    void sl_propertyUnBlocked(const zf::DataProperty& p);

    //! Сигнал о том, что поменялся фильтр для набора данных
    void sl_conditionFilterChanged(const zf::DataProperty& dataset);

    //! Завершена загрузка лукап модели
    void sl_sortLookupModelLoaded(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Удален внешний фильтр
    void sl_externalFilterDestroyed(QObject* obj);

    // реализация I_DataFilter
private:
    //! Отфильтровать строку набора данных. По умолчанию - не фильтровать
    bool externalFilterAcceptsRow(
        //! Источник данных
        ModuleDataObject* source,
        //! Фильтр, который мы переопределили. Может быть null
        I_DataFilter* original_filter,
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка source
        int row,
        //! Родитель строки
        const QModelIndex& parent,
        //! Безусловно исключить строку из иерархии
        bool& exclude_hierarchy) override;
    /*! Сортировка. Результат:
     * -1 = left < right
     * +1 = left > right
     * 0 = использовать стандартную сортировку (по умолчанию) */
    int externalLessThan(
        //! Источник данных
        ModuleDataObject* source,
        //! Фильтр, который мы переопределили. Может быть null
        I_DataFilter* original_filter,
        //! Набор данных
        const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right) override;

private:
    //! Отложенная инициализация
    void lazyInit() const;
    //! Инциализация прокси набора данных
    void initDatasetProxy(const zf::DataProperty& dataset_property);
    //! Отфильтровать строку набора данных. По умолчанию - не фильтровать
    bool filterAcceptsRowHelper(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка source
        int source_row,
        //! Родитель строки
        const QModelIndex& source_parent,
        //! Безусловно исключить строку из иерархии
        bool& exclude_hierarchy);
    /*! Сортировка */
    bool lessThanHelper(
        //! Набор данных
        const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right);

    //! Задать фильтрацию по нескольким колонкам
    //! Фильтрация EasyFilter заменяется на новую только для наборов данных, колонки которых указаны
    void setEasyFilterHelper(
        //! Колонки набора данных
        const DataPropertyList& columns,
        //! Значения для сравнения. Количество должно совпадать с количеством в columns
        const QList<QVariantList>& values,
        //! Операторы сравнения: [значение набора данных] [оператор] [value]
        //! Если не задано, то CompareOperator::Equal. Иначе количество должно совпадать с количеством в columns
        const QList<CompareOperator> ops,
        //! Роли для получения значения набора данных. Если не задано, то Qt::DisplayRole. Иначе количество должно совпадать с количеством в columns
        const QList<int> roles,
        //! Параметры
        const QList<CompareOptions>& options);

    //! Источник данных
    ModuleDataObject* _source = nullptr;
    //! Было ли инициализированно
    mutable bool _initialized = false;

    //! Информация о наборе данных
    struct _DatasetInfo
    {
        //! Свойство набора данных
        DataProperty dataset_property;
        //! Указатель на item model
        const QAbstractItemModel* item_model = nullptr;
        //! Прокси
        std::unique_ptr<ProxyItemModel> proxy_item_model;

        //! Упрощенная фильтрация
        QList<std::shared_ptr<EasyFilter>> easy_filter;

        //! Сортировка по одной колонке - колонка
        DataProperty easy_sort_column;
        //! Порядок сортировки
        Qt::SortOrder easy_sort_order = Qt::AscendingOrder;
        //! Сортировка по одной колонке - роль
        int easy_sort_role = Qt::DisplayRole;
        //! Номер колонки упрощенной фильтрации
        int easy_sort_dataset_column_index = -1;

        struct LookupLoadWaitingInfo
        {
            // модель lookup
            ModelPtr lookup_model;
            //!  Подписка на сигнал об окончании перезагрузки lookup
            QMetaObject::Connection connection;
        };
        //! Список lookup моделей, которые должны загрузиться прежде чем с ними можно будет работать в фильтрации или сортировке
        QList<LookupLoadWaitingInfo> lookup_load_waiting_info;
    };

    //! Информация о наборе данных по свойству
    QHash<DataProperty, std::shared_ptr<_DatasetInfo>> _dataset_by_prop;
    //! Соответствие между набором данных и информацией о нем
    QHash<const QAbstractItemModel*, std::shared_ptr<_DatasetInfo>> _dataset_by_pointer;
    //! Соответствие между прокси и информацией о нем
    QHash<const QAbstractItemModel*, std::shared_ptr<_DatasetInfo>> _proxy_by_pointer;

    //! Интерфейс для условных фильтров
    mutable I_ConditionFilter* _i_condition_filter;
    //! Интерфейс для преобразования значений наборов данных
    mutable I_DatasetVisibleInfo* _i_convert_item_value;

    //! Управление освобождением временных ресурсов
    mutable std::unique_ptr<ResourceManager> _resource_manager;
    //! Методы хэшированного поиска
    mutable std::unique_ptr<DataHashed> _hash;

    //! Внешние интерфейсы фильтрации
    QList<I_DataFilter*> _external_filters;
    QMap<QObject*, I_DataFilter*> _external_filters_map;

    //! Отключение внешней фильтрации
    bool _disable_external = false;

    friend class ProxyItemModel;
};

typedef std::shared_ptr<DataFilter> DataFilterPtr;

} // namespace zf
