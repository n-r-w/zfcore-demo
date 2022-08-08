#pragma once

#include <QSharedDataPointer>
#include <QLocale>
#include "zf_defs.h"
#include "zf_data_structure.h"
#include "zf_uid.h"
#include "zf_item_model.h"
#include "zf_hashed_dataset.h"
#include "zf_numeric.h"
#include "zf_data_hashed.h"
#include "zf_change_info.h"

namespace zf
{
class DataFilter;
class DataContainer_SharedData;
class DataContainer;
class ChangeInfo;
struct DataContainerValue;
typedef QList<DataContainer> DataContainerList;
typedef std::shared_ptr<DataContainer> DataContainerPtr;
typedef QList<DataContainerPtr> DataContainerPtrList;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataContainer& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataContainer& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataContainerPtr& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataContainerPtr& obj);

namespace zf
{
//! Абстрактный класс для хранения произвольных данных
class ZCORESHARED_EXPORT AnyData
{
public:
    virtual ~AnyData();
    //! Создать копию
    virtual AnyData* clone() = 0;
};

//! Данные на основе DataStructure
class ZCORESHARED_EXPORT DataContainer : public QObject
{
    Q_OBJECT
public:
    DataContainer();
    DataContainer(const DataContainer& d);
    //! Инициализация на основании внешней структуры
    DataContainer(
        //! Структура данных
        const DataStructurePtr& data_structure,
        //! Список свойств, которые считаются инициализированными (будут содержать значения по умолчанию). Кроме
        //! таблиц. Если пусто, то ничего не инициализировано
        const DataPropertySet& initialization = DataPropertySet());
    //! Структура создается и хранится внутри DataContainer
    DataContainer(
        //! Уникальный идентификатор структуры (не контейнера!) > 0
        const PropertyID& id,
        //! Версия структуры данных
        int structure_version,
        //! Набор свойств
        const QList<DataProperty>& properties,
        //! Список свойств, которые считаются инициализированными (будут содержать значения по умолчанию). Кроме
        //! таблиц. Если пусто, то ничего не инициализировано
        const DataPropertySet& initialization = DataPropertySet());
    //! Структура создается и хранится внутри DataContainer. Инициализированными считаются свойства, по которым
    //! переданы значения
    DataContainer(
        //! Уникальный идентификатор структуры (не данных!)
        const PropertyID& id,
        //! Версия структуры данных
        int structure_version,
        //! Набор свойств
        const QList<DataProperty>& properties,
        //! Значения свойств. Если пусто, то ничего не инициализировано
        const QHash<DataProperty, QVariant>& value_properties,
        //! Таблицы. Владение ItemModel переходит к этому объекту (не удалять переданные
        //! ItemModel!). Если пусто, то таблицы не инициализированы
        const QHash<DataProperty, ItemModel*>& value_tables = QHash<DataProperty, ItemModel*>());

    ~DataContainer();

    bool isValid() const;
    void clear();

    //! Вывести содержимое для отладки
    void debPrint() const;

    //! Уникальный идентификатор контейнера. Генерируется автоматически
    QString id() const;

    DataContainer& operator=(const DataContainer& d);
    bool operator<(const DataContainer& d) const;
    bool operator==(const DataContainer& d) const;

    //! Содержит ли такое свойство
    bool containsProperty(const DataProperty& p) const;

    //! Cвойство ячейки
    DataProperty propertyCell(
        //! Набор данных
        const DataProperty& dataset,
        //! Номер строки
        int row,
        //! Номер колонки
        int column,
        //! Индекс родителя
        const QModelIndex& parent = QModelIndex()) const;
    //! Cвойство ячейки
    DataProperty propertyCell(
        //! Строка набора данных (PropertyType::RowFull)
        const DataProperty& row_property,
        //! Колонка набора данных
        const DataProperty& column_property) const;
    //! Cвойство ячейки
    DataProperty propertyCell(
        //! Строка набора данных (PropertyType::RowFull)
        const DataProperty& row_property,
        //! Колонка набора данных
        const PropertyID& column_property_id) const;
    //! Cвойство ячейки
    DataProperty propertyCell(
        //! Номер строки
        int row,
        //! Колонка набора данных
        const DataProperty& column_property,
        //! Индекс родителя
        const QModelIndex& parent = QModelIndex()) const;
    //! Cвойство ячейки
    DataProperty propertyCell(
        //! Номер строки
        int row,
        //! Колонка набора данных
        const PropertyID& column_property,
        //! Индекс родителя
        const QModelIndex& parent = QModelIndex()) const;
    DataProperty propertyCell(const QModelIndex& index) const;

    //! Cвойство строки
    DataProperty propertyRow(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Делает невалидными все lookup модели
    void invalidateLookupModels() const;

    //! Метод сравнения бинарных полей
    enum class ChangedBinaryMethod
    {
        //! Считать всегда измененными
        Ignore,
        //! Проверять этот контейнер на изменения
        ThisContainer,
        //! Проверять сравниваемый контейнер на изменения
        OtherContainer,
    };

    //! Сравнение с другим контейнером такой же структуры и вычисление что поменялось. Возвращает истину если были изменения
    //! ВАЖНО: для сравнение строк наборов данных, в них должны быть записаны rowID через метод setDatasetRowID. В контейнере это автоматически
    //! не происходит. Когда контейнер принадлежит ModuleDataObject, то за это отвечает он (метод ModuleDataObject::forceCalculateRowID).
    bool findDiff(
        //! Контейнер с которым идет сравнение
        const DataContainer* c,
        //! Свойства, которые надо игнорировать
        const QSet<DataProperty>& ignored_properties,
        //! Игнорировать наборы данных с ошибками. Ошибки могут быть связаны с попыткой получить дифф при отсутствии реальных ключей для строк
        bool ignore_bad_datasets,
        //! Правило проверки изменения бинарных полей
        ChangedBinaryMethod changed_binary_method,
        //! Изменения полей (свойства PropertyType::Field)
        DataPropertyList& changed_fields,
        //! Новые строки для наборов данных (ключ) (свойства PropertyType::RowFull)
        //! Идентификатор строки в DataProperty::rowId()
        QMap<DataProperty, DataPropertyList>& new_rows,
        //! Удаленные строки для наборов данных (ключ) (свойства PropertyType::RowFull). Берутся из контейнера, с которым идет сравнение)
        //! Идентификатор строки в DataProperty::rowId()
        QMap<DataProperty, DataPropertyList>& removed_rows,
        //! Измененные строки для наборов данных (ключ) (свойства PropertyType::Cell).
        //! Идентификатор строки в DataProperty::rowId()
        //! Идентификатор колонки в DataProperty::column()
        QMap<DataProperty, QMultiHash<zf::RowID, zf::DataProperty>>& changed_cells,
        //! Ошибка
        Error& error) const;
    //! Создать набор изменений для контейнера. Используется вместо findDiff если имеется новый контейнер
    Error createDiff(
        //! Свойства, которые надо игнорировать
        const QSet<DataProperty>& ignored_properties,
        //! Изменения полей (свойства PropertyType::Field)
        DataPropertyList& changed_fields,
        //! Новые строки для наборов данных (ключ) (свойства PropertyType::RowFull)
        //! Идентификатор строки в DataProperty::rowId()
        QMap<DataProperty, DataPropertyList>& new_rows,
        //! Измененные строки для наборов данных (ключ) (свойства PropertyType::Cell).
        //! Идентификатор строки в DataProperty::rowId()
        //! Идентификатор колонки в DataProperty::column()
        QMap<DataProperty, QMultiHash<zf::RowID, zf::DataProperty>>& changed_cells) const;

    //! Есть ли изменения относительно указанного контейнера
    bool hasDiff(
        //! Контейнер с которым идет сравнение
        const DataContainer* c,
        //! Свойства, которые надо игнорировать
        const QSet<DataProperty>& ignored_properties, Error& error,
        //! Правило проверки изменения бинарных полей
        ChangedBinaryMethod changed_binary_method = ChangedBinaryMethod::Ignore) const;

    /*! Язык контейнера по умолчанию (если равен QLocale::AnyLanguage, то при чтении будет использоваться Core::language, а при записи
     * QLocale::AnyLanguage) */
    QLocale::Language language() const;
    void setLanguage(QLocale::Language language);

    //! Структура данных
    DataStructurePtr structure() const;
    //! Свойство по его идентификатору
    DataProperty property(const PropertyID& property_id) const;

    //! Заблокировать все свойства. При этом не будут генерироваться сигналы об изменении. Допустимы вложенные вызовы
    void blockAllProperties() const;
    //! Разблокировать все свойства
    void unBlockAllProperties() const;
    //! Заблокировать свойство. При этом не будут генерироваться сигналы об изменении. Допустимы вложенные вызовы
    void blockProperty(const DataProperty& p) const;
    //! Разблокировать свойство
    void unBlockProperty(const DataProperty& p) const;

    //! Заблокированны ли все свойства
    bool isAllPropertiesBlocked() const;
    //! Заблокированно ли свойство (учитывается полная блокировка, а не только блокировка конкретного свойтства)
    bool isPropertyBlocked(const DataProperty& p) const;

    //! Принудительно вызвать сигналы об изменении всех данных, кроме тех, которые в состоянии invalidated, не инициализированы или заблокированы
    void allDataChanged();

    //! Содержит ли такое свойство в структуре
    bool contains(const PropertyID& property_id) const;
    //! Содержит ли такое свойство в структуре
    bool contains(const DataProperty& p) const;

    //! Получить значение свойства. Если свойство не иниализировано или Dataset - ошибка
    QVariant value(const DataProperty& p, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Получить значение свойства. Если свойство не иниализировано или Dataset - ошибка
    QVariant value(const PropertyID& property_id, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Получить значение свойства. Если свойство не иниализировано или Dataset - ошибка
    template <typename T>
    inline T value(const DataProperty& p, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const
    {
        return value(p, language, from_lookup).value<T>();
    }
    //! Получить значение свойства. Если свойство не иниализировано или Dataset - ошибка
    template <typename T>
    inline T value(const PropertyID& property_id, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const
    {
        return value(property_id, language, from_lookup).value<T>();
    }

    //! Получить значение типа Numeric
    Numeric toNumeric(const DataProperty& p) const;
    //! Получить значение типа Numeric
    Numeric toNumeric(const PropertyID& property_id) const;

    //! Получить значение типа double
    double toDouble(const DataProperty& p) const;
    //! Получить значение типа double
    double toDouble(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к строке
    QString toString(const DataProperty& p, LocaleType conv = LocaleType::UserInterface, QLocale::Language language = QLocale::AnyLanguage) const;
    QString toString(const PropertyID& property_id, LocaleType conv = LocaleType::UserInterface, QLocale::Language language = QLocale::AnyLanguage) const;

    //! Значение свойства, приведенное к QDate
    QDate toDate(const DataProperty& p) const;
    QDate toDate(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к QDateTime
    QDateTime toDateTime(const DataProperty& p) const;
    QDateTime toDateTime(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к QTime
    QTime toTime(const DataProperty& p) const;
    QTime toTime(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к Uid
    Uid toUid(const DataProperty& p) const;
    Uid toUid(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к целому
    int toInt(const DataProperty& p, int default_value = 0) const;
    int toInt(const PropertyID& property_id, int default_value = 0) const;

    //! Значение свойства, приведенное к bool
    bool toBool(const DataProperty& p) const;
    bool toBool(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к EntityID
    EntityID toEntityID(const DataProperty& p) const;
    EntityID toEntityID(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к InvalidValue
    InvalidValue toInvalidValue(const DataProperty& p) const;
    InvalidValue toInvalidValue(const PropertyID& property_id) const;

    //! Значение свойства, приведенное к QByteArray
    QByteArray toByteArray(const DataProperty& p) const;
    QByteArray toByteArray(const PropertyID& property_id) const;

    //! В поле находится объект InvalidValue
    bool isInvalidValue(const DataProperty& p, QLocale::Language language = QLocale::AnyLanguage) const;
    bool isInvalidValue(const PropertyID& property_id, QLocale::Language language = QLocale::AnyLanguage) const;

    //! В ячейке находится объект InvalidValue
    bool isInvalidCell(int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage) const;
    //! В ячейке находится объект InvalidValue
    bool isInvalidCell(int row, const PropertyID& column_property_id, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage) const;
    //! В ячейке находится объект InvalidValue
    bool isInvalidCell(const RowID& row_id, const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;
    //! В ячейке находится объект InvalidValue
    bool isInvalidCell(
        const RowID& row_id, const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;

    //! Получить значения на всех языках
    LanguageMap valueLanguages(const DataProperty& p) const;
    //! Получить значения на всех языках
    LanguageMap valueLanguages(const PropertyID& property_id) const;

    //! Проверка значения на null
    static bool isNullValue(const DataProperty& p, const QVariant& value);

    //! Проверка значения свойства на null. Если свойство не инициализированно - ошибка
    bool isNull(const DataProperty& p, QLocale::Language language = QLocale::AnyLanguage) const;
    //! Проверка значения свойства на null. Если свойство не инициализированно - ошибка
    bool isNull(const PropertyID& property_id, QLocale::Language language = QLocale::AnyLanguage) const;

    //! Проверка значения ячейки на null
    bool isNullCell(int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage) const;
    //! Проверка значения ячейки на null
    bool isNullCell(int row, const PropertyID& column_property_id, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage) const;

    //! Проверка значения ячейки на null
    bool isNullCell(const RowID& row_id, const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;
    //! Проверка значения ячейки на null
    bool isNullCell(
        const RowID& row_id, const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;
    //! Проверка значения ячейки на null
    bool isNullCell(
        //! Свойство "ячейка" - PropertyType::Cell
        const DataProperty& cell_property, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;

    /*! Получить указатель на набор данных. Если свойство не иниализировано - ошибка
     * С наборами данных невозможно гарантировать что при любом изменении данных произойдет deep copy, т.к. изменения
     * будут извне и подписка на сигналы ItemModel не поможет (изменения будут до сигнала). Поэтому если
     * планируется менять набор данных, то надо вручную сделать detach().
     * Теоретически можно написать свой вариант itemModel, в который будет передаваться указатель на DataContainer и она
     * должна делать detach перед любым изменением данных или структуры (не реализовано) */
    ItemModel* dataset(const DataProperty& p) const;
    ItemModel* dataset(const PropertyID& property_id) const;
    //! Получить указатель на набор данных. Для содержащих всего один набор данных
    ItemModel* dataset() const;

    //! Инициализировать значения указанных свойств
    Error initValues(const DataPropertySet& properties);
    Error initValues(const DataPropertyList& properties);
    Error initValues(const PropertyIDList& properties);
    //! Инициализировать значения всех свойств
    Error initAllValues();

    //! Инициализировать свойство (записать в него значение по умолчанию, если оно не инициализировано)
    Error initValue(const DataProperty& p);
    Error initValue(const PropertyID& property_id);

    //! Инициализировать набор данных. Если набор данных уже инициализирован, то ничего не пооисходит
    //! Возвращает истину если была инициализация
    bool initDataset(const DataProperty& dataset_property, int row_count);
    //! Инициализировать набор данных. Если набор данных уже инициализирован, то ничего не пооисходит
    //! Возвращает истину если была инициализация
    bool initDataset(const PropertyID& property_id, int row_count);
    //! Инициализировать набор данных. Для содержащих всего один набор данных. Если набор данных уже инициализирован, то
    //! ничего не пооисходит. Возвращает истину если была инициализация
    bool initDataset(int row_count);

    //! Установить набор данных  (инициализация)
    void setDataset(const DataProperty& dataset_property, ItemModel* ds, CloneContainerDatasetMode mode);
    void setDataset(const PropertyID& property_id, ItemModel* ds, CloneContainerDatasetMode mode);

    //! Сделать свойство не инициализированым
    void unInitialize(const DataProperty& p);
    //! Сделать свойство неинициализированным
    void unInitialize(const PropertyID& property_id);

    //! Инициализировано ли свойство
    bool isInitialized(const DataProperty& p) const;
    bool isInitialized(const PropertyID& property_id) const;

    //! Проверка группы свойств на инициализацию
    //! Если параметр initialized true, то возвращает истину когда все инициализированы, инача когда все не инициализированы
    bool isInitialized(const DataPropertySet& p, bool initialized) const;
    bool isInitialized(const DataPropertyList& p, bool initialized) const;
    bool isInitialized(const PropertyIDList& property_ids, bool initialized) const;
    bool isInitialized(const PropertyIDSet& property_ids, bool initialized) const;

    //! Все ли свойства инициализированы
    bool isAllInitialized() const;
    //! Все инициализированные свойства
    DataPropertySet initializedProperties() const;
    //! Все неинициализированные свойства
    DataPropertySet notInitializedProperties() const;

    //! Какие из указанных свойств являются не инициализированными
    //! Если параметр initialized true, то возвращает список инициализированных, иначе не инициализированных
    DataPropertySet whichPropertiesInitialized(const DataPropertySet& p, bool initialized) const;
    DataPropertySet whichPropertiesInitialized(const DataPropertyList& p, bool initialized) const;
    DataPropertySet whichPropertiesInitialized(const PropertyIDList& property_ids, bool initialized) const;
    DataPropertySet whichPropertiesInitialized(const PropertyIDSet& property_ids, bool initialized) const;

    //! Пометить данные, как требуемые к обновлению
    bool setInvalidate(const DataProperty& p, bool invalidated, const ChangeInfo& info = {});
    bool setInvalidate(const PropertyID& property_id, bool invalidated, const ChangeInfo& info = {});
    //! Пометить все данные, как требуемые к обновлению
    bool setInvalidateAll(bool invalidated, const ChangeInfo& info = {});

    //! Необходимо ли обновить данные
    bool isInvalidated(const DataProperty& p) const;
    bool isInvalidated(const PropertyID& property_id) const;

    //! Проверка группы свойств на валидность
    //! Если параметр invalidated true, то возвращает истину когда все не валидны, иначе когда все валидны
    bool isInvalidated(const DataPropertySet& p, bool invalidated) const;
    bool isInvalidated(const DataPropertyList& p, bool invalidated) const;
    bool isInvalidated(const PropertyIDList& property_ids, bool invalidated) const;
    bool isInvalidated(const PropertyIDSet& property_ids, bool invalidated) const;

    //! Свойства, которые надо обновить
    DataPropertySet invalidatedProperties() const;
    //! Есть ли свойства, которые надо обновить
    bool hasInvalidatedProperties() const;

    //! Какие из указанных свойств являются не валидными
    //! Если параметр invalidated true, то возвращает список не валидных, иначе  валидных
    DataPropertySet whichPropertiesInvalidated(const DataPropertySet& p, bool invalidated) const;
    DataPropertySet whichPropertiesInvalidated(const DataPropertyList& p, bool invalidated) const;
    DataPropertySet whichPropertiesInvalidated(const PropertyIDList& property_ids, bool invalidated) const;
    DataPropertySet whichPropertiesInvalidated(const PropertyIDSet& property_ids, bool invalidated) const;

    //! Записать в свойство значение по умолчанию. Для таблиц очищает все строки
    Error resetValue(const DataProperty& p);
    Error resetValue(const PropertyID& property_id);
    //! Записать во все свойства значение по умолчанию. Таблицы будут очищены
    Error resetValues();

    //! Очищает значение свойства (выставляет в NULL). Для таблиц - удаляет все строки
    Error clearValue(const DataProperty& p);
    Error clearValue(const PropertyID& property_id);

    //! Записать значение
    Error setValue(const DataProperty& p, const QVariant& v, QLocale::Language language = QLocale::AnyLanguage);
    Error setValue(const PropertyID& property_id, const QVariant& v, QLocale::Language language = QLocale::AnyLanguage);

    Error setValue(const DataProperty& p, const Numeric& v);
    Error setValue(const PropertyID& property_id, const Numeric& v);

    Error setValue(const DataProperty& p, const QByteArray& v, QLocale::Language language = QLocale::AnyLanguage);
    Error setValue(const PropertyID& property_id, const QByteArray& v, QLocale::Language language = QLocale::AnyLanguage);

    Error setValue(const DataProperty& p, const QString& v, QLocale::Language language = QLocale::AnyLanguage);
    Error setValue(const PropertyID& property_id, const QString& v, QLocale::Language language = QLocale::AnyLanguage);

    Error setValue(const DataProperty& p, const char* v, QLocale::Language language = QLocale::AnyLanguage);
    Error setValue(const PropertyID& property_id, const char* v, QLocale::Language language = QLocale::AnyLanguage);

    Error setValue(const DataProperty& p, bool v);
    Error setValue(const PropertyID& property_id, bool v);

    Error setValue(const DataProperty& p, int v);
    Error setValue(const PropertyID& property_id, int v);

    Error setValue(const DataProperty& p, uint v);
    Error setValue(const PropertyID& property_id, uint v);

    Error setValue(const DataProperty& p, qint64 v);
    Error setValue(const PropertyID& property_id, qint64 v);

    Error setValue(const DataProperty& p, quint64 v);
    Error setValue(const PropertyID& property_id, quint64 v);

    Error setValue(const DataProperty& p, double v);
    Error setValue(const PropertyID& property_id, double v);

    Error setValue(const DataProperty& p, float v);
    Error setValue(const PropertyID& property_id, float v);

    Error setValue(const DataProperty& p, const Uid& v);
    Error setValue(const PropertyID& property_id, const Uid& v);

    Error setValue(const DataProperty& p, const ID_Wrapper& v);
    Error setValue(const PropertyID& property_id, const ID_Wrapper& v);

    Error setValue(const DataProperty& p, const LanguageMap& values);
    Error setValue(const PropertyID& property_id, const LanguageMap& values);

    //! Копирование значения свойства
    Error setValue(const DataProperty& p, const DataProperty& v);
    Error setValue(const PropertyID& property_id, const DataProperty& v);
    Error setValue(const DataProperty& p, const PropertyID& v);
    Error setValue(const PropertyID& property_id, const PropertyID& v);

    //! Запись в набор данных типа PropertyOption::SimpleDataset
    Error setSimpleValues(const DataProperty& property, const QVariantList& values);
    Error setSimpleValues(const PropertyID& property_id, const QVariantList& values);
    //! Извлечь ключевые значения из набора данных типа PropertyOption::SimpleDataset
    QVariantList getSimpleValues(const DataProperty& property) const;
    QVariantList getSimpleValues(const PropertyID& property_id) const;

    //! Было ли изменение данных (для полей)
    bool isChanged(const DataProperty& p) const;
    //! Было ли изменение данных (для полей)
    bool isChanged(const PropertyID& property_id) const;

    //! Было ли изменение данных (для ячеек таблиц)
    bool isChanged(int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Было ли изменение данных (для ячеек таблиц)
    bool isChanged(int row, const PropertyID& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;

    //! Сбросить флаг изменения данных (для полей)
    void resetChanged(const DataProperty& p);
    //! Сбросить флаг изменения данных (для полей)
    void resetChanged(const PropertyID& property_id);

    //! Сбросить флаг изменения данных (для ячеек таблиц)
    void resetChanged(int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Сбросить флаг изменения данных (для ячеек таблиц)
    void resetChanged(int row, const PropertyID& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Сбросить флаг изменения данных для всех свойств
    void resetChanged();

    //! Запрет на запись (счетчик)
    void disableWrite();
    //! Снятие запрета на запись (счетчик)
    void enableWrite();
    //! Есть ли запрет на запись
    bool isWriteDisabled() const;

    //! Заблокировать обновление одинаковых свойств
    void blockSameProperties();
    //! Разблокировать обновление одинаковых свойств
    void unBlockSameProperties();
    //! Заблокировано ли обновление одинаковых свойтсв
    bool isSamePropertiesBlocked() const;
    //! Заполнить одинаковые свойства. Для каждой группы свойств выбирается первое не пустое значение, которое записывается во все остальные
    void fillSameProperties(bool no_db_read_ignored);

    //! Заблокировать обновление свойств на основе PropertyLinkType::DataSourcePriority
    void blockDataSourcePriority();
    //! Разаблокировать обновление свойств на основе PropertyLinkType::DataSourcePriority
    void unBlockDataSourcePriority();
    //! Заблокировано ли обновление свойств на основе PropertyLinkType::DataSourcePriority
    bool isDataSourcePriorityBlocked() const;
    //! Заполнить значения на основе PropertyLinkType::DataSourcePriority
    void fillDataSourcePriority(
        //! Обновить только это свойство (иначе все)
        const DataProperty& property = {});

    //! Изъять набор данных. Владение ItemModel переходит к вызывающему. Набор данных становится
    //! неициализированным
    ItemModel* takeDataset(const DataProperty& dataset_property);
    ItemModel* takeDataset(const PropertyID& dataset_property_id);

    //! Свойство по указателю на набор данных. Если не найдено, то invalid
    DataProperty datasetProperty(const QAbstractItemModel* m) const;
    //! Свойство набора данных. При наличии только одного набора данных
    DataProperty datasetProperty() const;

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const QVariant& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const QVariant& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const char* value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const char* value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const QString& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const QString& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const Numeric& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const Numeric& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const QIcon& value, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const QIcon& value, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const Uid& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const Uid& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const ID_Wrapper& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(
        int row, const PropertyID& column_property_id, const ID_Wrapper& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, bool value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, bool value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, int value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, int value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, uint value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, uint value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, qint64 value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, qint64 value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, quint64 value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, quint64 value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, double value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, double value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, float value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, float value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных данные на всех языках (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const LanguageMap& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных данные на всех языках (вызывает его инициализацию)
    Error setCell(
        int row, const PropertyID& column_property_id, const LanguageMap& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(
        const RowID& row_id, const DataProperty& column, const QVariant& value, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const QVariant& value, int role = Qt::DisplayRole,
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(
        const RowID& row_id, const DataProperty& column, const char* value, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const char* value, int role = Qt::DisplayRole,
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(
        const RowID& row_id, const DataProperty& column, const QString& value, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const QString& value, int role = Qt::DisplayRole,
        QLocale::Language language = QLocale::AnyLanguage);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, const Numeric& value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const Numeric& value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, const QIcon& value);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const QIcon& value);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, const Uid& value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const Uid& value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, const ID_Wrapper& value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const ID_Wrapper& value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, bool value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, bool value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, int value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, int value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, uint value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, uint value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, qint64 value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, qint64 value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, quint64 value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, quint64 value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, double value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, double value, int role = Qt::DisplayRole);

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, float value, int role = Qt::DisplayRole);
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, float value, int role = Qt::DisplayRole);

    //! Записать в набор данных данные на всех языках (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const DataProperty& column, const LanguageMap& value, int role = Qt::DisplayRole);
    //! Записать в набор данных данные на всех языках (вызывает его инициализацию)
    Error setCell(const RowID& row_id, const PropertyID& column_property_id, const LanguageMap& value, int role = Qt::DisplayRole);

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(int row, const PropertyID& column_property_id, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(const RowID& row_id, const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(const RowID& row_id, const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Свойство "ячейка" - PropertyType::Cell
        const DataProperty& cell_property, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(const QModelIndex& index,
        //! Колонка
        const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(const QModelIndex& index,
        //! Колонка
        const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Количество строк в наборе данных
    int rowCount(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Количество строк в наборе данных. При наличии только одного набора
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    //! Установить количество строк набора данных
    void setRowCount(const DataProperty& dataset_property, int count, const QModelIndex& parent = QModelIndex());
    void setRowCount(const PropertyID& dataset_property_id, int count, const QModelIndex& parent = QModelIndex());
    //! Установить количество строк набора данных. При наличии только одного набора
    void setRowCount(int count, const QModelIndex& parent = QModelIndex());

    //! Установить количество колонок набора данных
    void setColumnCount(const DataProperty& dataset_property, int count);
    void setColumnCount(const PropertyID& dataset_property_id, int count);
    //! Установить количество колонок набора данных. При наличии только одного набора
    void setColumnCount(int count);

    //! Количество колонок в наборе данных
    int columnCount(const DataProperty& dataset_property) const;
    int columnCount(const PropertyID& dataset_property_id) const;
    //! Количество колонок в наборе данных. При наличии только одного набора
    int columnCount() const;

    //! Добавить строку в конец набора данных. Возвращает номер последней строки
    int appendRow(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    int appendRow(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    //! Добавить строку в конец набора данных. При наличии только одного набора. Возвращает номер последней строки
    int appendRow(const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);

    //! Добавить строку в произвольное место набора данных. Возвращает номер вставленной строки
    int insertRow(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    int insertRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    //! Добавить строку в произвольное место набора данных. При наличии только одного набора. Возвращает номер
    //! вставленной строки
    int insertRow(int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);

    //! Удалить строку из набора данных
    void removeRow(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    void removeRow(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    //! Удалить строку из набора данных. При наличии только одного набора
    void removeRow(int row, const QModelIndex& parent = QModelIndex(),
        //! Количество строк
        int num = 1);
    //! Удалить строки из набора данных
    void removeRows(const DataProperty& dataset_property, const Rows& rows);
    void removeRows(const PropertyID& dataset_property_id, const Rows& rows);
    //! Удалить строки из набора данных. При наличии только одного набора
    void removeRows(const Rows& rows);

    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRows(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка источник
        int source_row,
        //! Родитель источник
        const QModelIndex& source_parent,
        //! Количество строк
        int count,
        //! Строка цель
        int destination_row,
        //! Родитель цель
        const QModelIndex& destination_parent);
    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRows(
        //! Набор данных
        const PropertyID& dataset_property_id,
        //! Строка источник
        int source_row,
        //! Родитель источник
        const QModelIndex& source_parent,
        //! Количество строк
        int count,
        //! Строка цель
        int destination_row,
        //! Родитель цель
        const QModelIndex& destination_parent);
    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRows(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка источник
        int source_row,
        //! Количество строк
        int count,
        //! Строка цель
        int destination_row);
    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRows(
        //! Набор данных
        const PropertyID& dataset_property_id,
        //! Строка источник
        int source_row,
        //! Количество строк
        int count,
        //! Строка цель
        int destination_row);
    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRow(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Строка источник
        int source_row,
        //! Строка цель
        int destination_row);
    /*! Перемещение строк надора данных
     *  Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя,
     *  то destination_row указывает на позицию куда надо вставить строки до перемещения.
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destination_row должен быть равен 2 */
    void moveRow(
        //! Набор данных
        const PropertyID& dataset_property_id,
        //! Строка источник
        int source_row,
        //! Строка цель
        int destination_row);

    //! Индекс ячейки набора данных по номеру строки и идентификатору колонки
    QModelIndex cellIndex(int row, const DataProperty& column, const QModelIndex& parent = QModelIndex()) const;
    //! Индекс ячейки набора данных по номеру строки и идентификатору колонки
    QModelIndex cellIndex(int row, const PropertyID& column_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Индекс ячейки набора данных по номеру строки и номеру колонки (не идентификатору!)
    QModelIndex cellIndex(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent = QModelIndex()) const;
    //! Индекс ячейки набора данных по номеру строки и номеру колонки (не идентификатору!)
    QModelIndex cellIndex(const PropertyID& dataset_property_id, int row, int column, const QModelIndex& parent = QModelIndex()) const;
    //! Получить индекс ячейки по ее свойству
    QModelIndex cellIndex(const DataProperty& cell_property) const;

    //! Индекс первой колонки для указанной строки
    QModelIndex rowIndex(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex rowIndex(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Базовый метод записи в ячейку набора данных (не использовать)
    Error setCellValue(const DataProperty& dataset_property, int row, const DataProperty& column_property, int column, const QVariant& value, int role,
        const QModelIndex& parent, bool block_signals, QLocale::Language language);
    //! Базовый метод записи в ячейку набора данных (не использовать)
    Error setCellValue(const DataProperty& p, int row, int column, const LanguageMap& value, int role, const QModelIndex& parent, bool block_signals);
    //! Базовый метод записи в ячейку набора данных (не использовать)
    Error setCellValue(const DataProperty& p, int row, int column, const QMap<int, LanguageMap>& value, const QModelIndex& parent, bool block_signals);

    //! Базовый метод чтения из ячейки набора данных (не использовать)
    QVariant cellValue(const DataProperty& dataset_property, int row, int column, int role, const QModelIndex& parent, QLocale::Language language,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup) const;
    //! Базовый метод чтения из ячейки набора данных (не использовать)
    LanguageMap cellValue(const DataProperty& dataset_property, int row, int column, int role, const QModelIndex& parent) const;
    //! Значения по всем ролям
    QMap<int, LanguageMap> cellValueMap(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent) const;

    //! Уникальный ключ строки набора данных. Получает значение из роли Consts::UNIQUE_ROW_COLUMN_ROLE нулевой колонки
    RowID datasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent) const;
    /*! Установить значение уникального ключа строки набора данных. Записывает значение в роль
     * Consts::UNIQUE_ROW_COLUMN_ROLE нулевой колонки. ВАЖНО: при вызове этого метода ядро блокирует сигналы QAbstractItemModel */
    void setDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id);
    //! Очистить идентификатор строки
    void clearDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent);

    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке. В большинстве случаев вместо этого метода надо использовать прокси в представлении
    void sort(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Колонки
        const DataPropertyList& columns,
        //! Порядок сортировки. Если не задано, то все по порядку
        const QList<Qt::SortOrder>& orders = {},
        //! Роли. Если не задано, то Qt::DisplayRole
        const QList<int>& roles = {});
    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке. В большинстве случаев вместо этого метода надо использовать прокси в представлении
    void sort(
        //! Набор данных
        const PropertyID& dataset_property_id,
        //! Колонки
        const PropertyIDList& columns,
        //! Если не задано, то все по порядку
        const QList<Qt::SortOrder>& orders = {},
        //! Роли. Если не задано, то Qt::DisplayRole
        const QList<int>& roles = {});
    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке
    void sort(
        //! Набор данных
        const DataProperty& dataset_property,
        //! Функция сортировки
        FlatItemModelSortFunction sort_function);
    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке
    void sort(
        //! Набор данных
        const PropertyID& dataset_property_id,
        //! Функция сортировки
        FlatItemModelSortFunction sort_function);

    //! Найти строку по идентификатору
    QModelIndex findDatasetRowID(const DataProperty& dataset_property, const RowID& row_id) const;
    QModelIndex findDatasetRowID(const PropertyID& dataset_property_id, const RowID& row_id) const;
    //! Очистка хэша поиска по идентификаторам строк
    void clearRowHash(const DataProperty& dataset_property = DataProperty()) const;

    //! Установить генератор идентификаторов строк
    void setRowIdGenerator(ModuleDataObject* generator);

    //! Все строки (свойства)
    DataPropertyList getAllRowsProperties(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    DataPropertyList getAllRowsProperties(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Все строки (индексы)
    QModelIndexList getAllRowsIndexes(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    QModelIndexList getAllRowsIndexes(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Все строки (zf::Rows)
    Rows getAllRows(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    Rows getAllRows(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;

    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<RowID>& rows = {}, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QModelIndexList& rows, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<int>& rows, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<RowID>& rows = {}, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QModelIndexList& rows, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<int>& rows, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const Rows& rows, int role = Qt::DisplayRole,
        //! Фильтрация
        const DataFilter* filter = nullptr,
        //! Только содержащие значения
        bool valid_only = false) const;

    //! Колонка для индекса
    DataProperty indexColumn(const QModelIndex& index) const;

    //! Создать свойство для ячейки набора данных
    DataProperty cellProperty(int row, const DataProperty& column, const QModelIndex& parent = QModelIndex()) const;
    DataProperty cellProperty(int row, const PropertyID& column, const QModelIndex& parent = QModelIndex()) const;
    DataProperty cellProperty(const RowID& row_id, const DataProperty& column) const;
    DataProperty cellProperty(const QModelIndex& index,
        //! Критическая ошибка если колонка не найдена в структуре данных (например добавлена вручную)
        bool halt_if_column_not_exists = true) const;

    //! Создать свойство "строка"
    DataProperty rowProperty(const DataProperty& dataset, const RowID& row_id) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const PropertyID& dataset_id, const RowID& row_id) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const PropertyID& dataset_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Записать данные из другого DataContainer в этот DataContainer
    void copyFrom(const DataContainerPtr& source,
        //! Список свойств, которые надо записать. Если не задано, то все, которые инициализированы в source
        const DataPropertySet& properties = DataPropertySet(),
        //! Если истина, то контролируется: совпадение id структуры, все записываемые свойства должны быть в
        //! структуре данного объекта. При несовпадении - критическая ошибка
        bool halt_if_error = true,
        //! Режим
        CloneContainerDatasetMode mode = CloneContainerDatasetMode::Clone);
    //! Записать данные из другого DataContainer в этот DataContainer
    void copyFrom(const DataContainer* source,
        //! Список свойств, которые надо записать. Если не задано, то все, которые инициализированы в source
        const DataPropertySet& properties = DataPropertySet(),
        //! Если истина, то контролируется: совпадение id структуры, все записываемые свойства должны быть в
        //! структуре данного объекта. При несовпадении - критическая ошибка
        bool halt_if_error = true,
        //! Режим
        CloneContainerDatasetMode mode = CloneContainerDatasetMode::Clone);

    //! Создать персональную копию разделяемых данных для редактирования набора данных
    void detach();

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static DataContainer fromVariant(const QVariant& v);

    //! Создать DataContainer с одной таблицей, содержащей QVariant. Колонки создаются с id равным номеру колонки
    static DataContainer createVariantTable(
        //! Количество строк
        int row_count,
        //! Количество колонок
        int column_count,
        //! Данные: data.at(row).at(column)
        const QList<QVariantList>& data = QList<QVariantList>(),
        //! Версия структуры данных
        int structure_version = 0,
        //! Уникальный идентификатор структуры
        const PropertyID& id = PropertyID::def(),
        //! Язык
        QLocale::Language language = QLocale::AnyLanguage);
    //! Создать DataContainer со списком свойств, содержащих QVariant. Идентификаторы свойст берутся по их порядковому
    //! номеру в data и начинаются с Consts::MINUMUM_PROPERTY_ID
    static DataContainer createVariantProperties(
        //! Данные
        const QVariantList& data,
        //! Версия структуры данных
        int structure_version = 0,
        //! Уникальный идентификатор структуры
        const PropertyID& id = PropertyID::def());

    /*! Сериализация в JSON (без структуры). Только ограниченное количество типов данных */
    QJsonArray toJson() const;
    //! Десериализация из JSON
    static DataContainerPtr fromJson(const QJsonArray& source,
        //! Структура данных
        const DataStructurePtr& data_structure, Error& error);

    /*! Сериализация в QDataStream (без структуры) полная */
    void toStream(QDataStream& s) const;
    //! Десериализация из QDataStream
    static DataContainerPtr fromStream(QDataStream& s,
        //! Структура данных
        const DataStructurePtr& data_structure, Error& error);

    //! Методы хэшированного поиска
    DataHashed* hash() const;

    //! Вывести набор данных в QDebug
    void datasetToDebug(QDebug debug, const DataProperty& dataset_property) const;
    //! Вывести в QDebug внутреннюю отладочную информацию
    void debugInternal() const;

    //! Менеджер ресурсов
    ResourceManager* resourceManager() const;

    //! Произвольные данные
    AnyData* getAnyData(const QString& key) const;
    void addAnyData(const QString& key, AnyData* data,
        //! Заменить если такой ключ уже есть
        bool replace);

public:
    //! Установить внешний источник
    void setProxyContainer(const DataContainerPtr& source,
        /*! Список свойств, которые будут перенаправлены в source.
         * Ключ - свойство этого объекта, Значение - свойство из source
         * Если source == nullptr, то должно быть пусто */
        const QMap<DataProperty, DataProperty>& proxy_mapping = QMap<DataProperty, DataProperty>());
    //! Внешний источник
    DataContainerPtr proxyContainer() const;

    //! Режим прокси
    bool isProxyMode() const;

    //! Какое свойство proxy соответствует данному свойству этого контейнера
    DataProperty proxyMapping(const DataProperty& p) const;
    DataProperty proxyMapping(const PropertyID& property_id) const;
    //! Какое свойство этого контейнера соответствует указанному свойству proxy
    DataProperty sourceMapping(const DataProperty& source_property) const;
    DataProperty sourceMapping(const PropertyID& source_property_id) const;

    //! Проксируется ли данное свойство этого контейнера
    bool isProxyProperty(const DataProperty& p) const;
    bool isProxyProperty(const PropertyID& property_id) const;
    //! Проксируется ли данное свойство source
    bool isSourceProperty(const DataProperty& source_property) const;
    bool isSourceProperty(const PropertyID& source_property_id) const;

signals:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void sg_invalidate(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);
    //! Изменилось состояние валидности данных
    void sg_invalidateChanged(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);

    //! Сменился текущий язык данных контейнера
    void sg_languageChanged(QLocale::Language language);

    //! Свойство было инициализировано
    void sg_propertyInitialized(const zf::DataProperty& p);
    //! Свойство стало неинициализировано
    void sg_propertyUnInitialized(const zf::DataProperty& p);

    //! Все свойства были заблокированы
    void sg_allPropertiesBlocked();
    //! Все свойства были разблокированы
    void sg_allPropertiesUnBlocked();

    //! Свойство были заблокировано
    void sg_propertyBlocked(const zf::DataProperty& p);
    //! Свойство были разаблокировано
    void sg_propertyUnBlocked(const zf::DataProperty& p);

    //! Изменилось значение свойства
    void sg_propertyChanged(const zf::DataProperty& p,
        //! Старые значение (работает только для полей)
        const zf::LanguageMap& old_values);

    //! Сигналы наборов данных
    void sg_dataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void sg_dataset_headerDataChanged(const zf::DataProperty& p, Qt::Orientation orientation, int first, int last);

    void sg_dataset_rowsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sg_dataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sg_dataset_rowsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sg_dataset_rowsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sg_dataset_columnsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sg_dataset_columnsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sg_dataset_columnsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sg_dataset_columnsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sg_dataset_modelAboutToBeReset(const zf::DataProperty& p);
    void sg_dataset_modelReset(const zf::DataProperty& p);

    void sg_dataset_rowsAboutToBeMoved(
        const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);
    void sg_dataset_rowsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

    void sg_dataset_columnsAboutToBeMoved(const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex& destinationParent, int destinationColumn);
    void sg_dataset_columnsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column);

private slots:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void sl_sourceInvalidate(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);
    //! Изменилось состояние валидности данных
    void sl_sourceInvalidateChanged(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);

    //! Свойство proxy было инициализировано
    void sl_sourcePropertyInitialized(const zf::DataProperty& p);
    //! Свойство proxy стало неинициализировано
    void sl_sourcePropertyUnInitialized(const zf::DataProperty& p);

    //! Все свойства были заблокированы
    void sl_sourceAllPropertiesBlocked();
    //! Все свойства были разблокированы
    void sl_sourceAllPropertiesUnBlocked();

    //! Свойство были заблокировано
    void sl_sourcePropertyBlocked(const zf::DataProperty& p);
    //! Свойство были разаблокировано
    void sl_sourcePropertyUnBlocked(const zf::DataProperty& p);

    //! Изменилось значение свойства proxy
    void sl_sourcePropertyChanged(const zf::DataProperty& p,
        //! Старые значение (работает только для полей)
        const zf::LanguageMap& old_values);

    void sl_sourceDataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void sl_sourceDataset_headerDataChanged(const zf::DataProperty& p, Qt::Orientation orientation, int first, int last);
    void sl_sourceDataset_rowsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_rowsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_rowsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_columnsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_columnsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sl_sourceDataset_columnsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_columnsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_sourceDataset_modelAboutToBeReset(const zf::DataProperty& p);
    void sl_sourceDataset_modelReset(const zf::DataProperty& p);
    void sl_sourceDataset_rowsAboutToBeMoved(
        const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);
    void sl_sourceDataset_rowsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

    void sl_sourceDataset_columnsAboutToBeMoved(const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex& destinationParent, int destinationColumn);
    void sl_sourceDataset_columnsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column);

    //! Изменился набор данных
    void sl_dataset_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void sl_dataset_headerDataChanged(Qt::Orientation orientation, int first, int last);

    void sl_dataset_rowsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsInserted(const QModelIndex& parent, int first, int last);

    void sl_dataset_rowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsRemoved(const QModelIndex& parent, int first, int last);

    void sl_dataset_columnsAboutToBeInserted(const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsInserted(const QModelIndex& parent, int first, int last);

    void sl_dataset_columnsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsRemoved(const QModelIndex& parent, int first, int last);

    void sl_dataset_modelAboutToBeReset();
    void sl_dataset_modelReset();

    void sl_dataset_rowsAboutToBeMoved(
        const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);
    void sl_dataset_rowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

    void sl_dataset_columnsAboutToBeMoved(
        const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationColumn);
    void sl_dataset_columnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column);

    //! Завершена загрузка модели, которая нужна для обновления через PropertyDataSource
    void sl_dataSourceFinishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

private:
    void init();

    /*! Установить значение уникального ключа строки набора данных. Записывает значение в роль
     * Consts::UNIQUE_ROW_COLUMN_ROLE нулевой колонки. ВАЖНО: при вызове этого метода ядро блокирует сигналы QAbstractItemModel */
    void setDatasetRowID_helper(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id);

    //! Преобразовать набор значений на разных языках в JSON
    QJsonArray languageMapToJson(LanguageMap values, DataType type) const;
    //! Преобразовать набор данных в JSON
    QJsonArray datasetToJson(const DataProperty& p, const QModelIndex& parent) const;

    //! Загрузить значения по языкам из JSON и записать в контейнер
    Error languageMapFromJson(const QJsonValue& json, const DataProperty& p,
        //! Путь к ключу для генерации понятной ошибки
        const QStringList& path);
    //! Загрузить значения по языкам из JSON
    static Error languageMapFromJson(const QJsonValue& json, const DataProperty& p,
        //! Путь к ключу для генерации понятной ошибки
        const QStringList& path, LanguageMap& map);
    //! Загрузить значения набора данных из JSON и записать в контейнер
    Error datasetFromJson(const QJsonValue& json, const DataProperty& dataset_property,
        //! Путь к ключу для генерации понятной ошибки
        const QStringList& path, const QModelIndex& parent);

    //! Какой язык использовать для свойства
    QLocale::Language propertyLanguage(const DataProperty& p, QLocale::Language language, bool writing) const;

    void connectToDataset(QAbstractItemModel* dataset) const;
    void disconnectFromDataset(QAbstractItemModel* dataset) const;

    //! Какой контейнер содержит данное свойство
    const DataContainer* container(const PropertyID& property_id) const;
    DataContainer* container(const PropertyID& property_id);

    const DataContainerValue* valueHelper(const DataProperty& prop, bool init, bool& was_initialized) const;
    DataContainerValue* valueHelper(const DataProperty& prop, bool init, bool& was_initialized); //-V1071

    //! Получить значения на всех языках
    LanguageMap* allLanguagesHelper(const DataProperty& p) const;

    //! Вывести набор данных в QDebug
    void datasetToDebugHelper(QDebug debug, const ItemModel* m, const QModelIndex& parent) const;

    //! Поиск строк
    QModelIndex findRowID_helper(
        const DataProperty& dataset_property, ItemModel* model, QHash<RowID, QModelIndex>* hash, const RowID& row_id, const QModelIndex& parent) const;
    //! Все строки
    void getAllRowsProperties_helper(DataPropertyList& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const;
    void getAllRowsIndexes_helper(QModelIndexList& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const;
    void getAllRows_helper(Rows& rows, const DataProperty& dataset_property, ItemModel* model, const QModelIndex& parent) const;
    //! Очистка роли Consts::UNIQUE_ROW_COLUMN_ROLE
    void clearRowId(const DataProperty& dataset_property);
    void clearRowIdHelper(ItemModel* model, const QModelIndex& parent);

    //! Создать пустой не инициализированный набор данных
    void createUninitializedDataset(const DataProperty& dataset_property);

    //! Обработка изменений в одинаковых наборах данных
    void fillSameDatasets(const DataProperty& changed_dataset, bool force_source);

    //! Разделяемые данные
    QSharedDataPointer<DataContainer_SharedData> _d;

    //! Счетчик общей блокировки свойств
    mutable int _block_all_counter = 0;
    //! Информация о блокировке конкретных свойств. Ключ - id свойста, значение - счетчик блокировки
    mutable QHash<PropertyID, int> _block_property_info;

    struct DataSourceInfo;
    //! Информация для обновления полей через PropertyDataSource (цель)
    struct DataSourceTargetInfo
    {
        DataSourceInfo* source_info = nullptr;
        //! Свойство, которое надо менять при изменении source
        DataProperty target;
        //! Колонка набора данных из которой надо забирать значения
        DataProperty data_column;
        //! Храним указатель на модель чтобы она не была удалена
        std::shared_ptr<Model> model;
        //! Колонка набора данных с ключевыми полями
        DataProperty key_column;
    };
    //! Информация для обновления полей через PropertyDataSource (общее)
    struct DataSourceInfo
    {
        //! Источник изменений
        DataProperty source;
        //! Свойства, которые надо менять при изменении source
        QList<std::shared_ptr<DataSourceTargetInfo>> targets;
    };
    //! Ключ - источник изменений
    mutable std::unique_ptr<QMap<DataProperty, std::shared_ptr<DataSourceInfo>>> _data_source_by_source;
    //! Ключ - модель с данными, откуда надо брать значения при изменении source
    mutable std::unique_ptr<QMultiMap<Model*, std::shared_ptr<DataSourceTargetInfo>>> _data_source_by_model;

    //! Ключ - свойство содержащееся в PropertyLink::dataSourcePriority, значение - свойство, куда должно попадать значение
    mutable std::unique_ptr<QMultiMap<DataProperty, DataProperty>> _data_source_priorities;
    const QMultiMap<DataProperty, DataProperty>& dataSourcePriorities() const;

    DataSourceInfo* dataSourceInfoBySource(const DataProperty& source) const;
    QList<std::shared_ptr<DataSourceTargetInfo>> dataSourceInfoByModel(Model* model) const;
    void initDataSourceInfo() const;

    //! Обработать связи, заданные через PropertyDataSource
    Error processDataSourceChanged(const DataProperty& p);
    Error processDataSourceChangedHelper(DataSourceTargetInfo* target_info);

    //! Хэш по строкам для поиска по идентификатору. Ключ - свойство набора данных
    mutable QMap<DataProperty, std::shared_ptr<QHash<RowID, QModelIndex>>> _row_by_id;

    //! Генератор идентификаторов строк
    QPointer<ModuleDataObject> _row_id_generator;
    //! В процессе генерации row_id
    mutable bool _is_row_id_generating = false;

    //! Информация для исключения циклов при изменениях одинаковых свойств
    mutable PropertyIDSet _same_props_changing;
    //! Счетчик блокировки изменения одинаковых свойств
    int _same_props_block_count = 0;
    //! Счетчик блокировки обновления свойств на основе PropertyLinkType::DataSourcePriority
    int _data_source_priority_block_count = 0;
    //! Информация для исключения циклов при изменениях на основе PropertyLinkType::DataSourcePriority
    mutable PropertyIDSet _data_source_priority_props_changing;

    //! Счетчик запрета на запись
    int _disable_write_counter = 0;
};

inline DataContainerList operator+(const DataContainer& m1, const DataContainer& m2)
{
    return DataContainerList {m1, m2};
}

inline DataContainerList operator+(const DataContainer& m1, const DataContainerList& m2)
{
    return DataContainerList {m1} + m2;
}

inline uint qHash(const DataContainer& m)
{
    return ::qHash(m.id());
}

} // namespace zf

Q_DECLARE_METATYPE(zf::DataContainer)
Q_DECLARE_METATYPE(zf::DataContainerPtr)
Q_DECLARE_METATYPE(zf::DataContainerList)
Q_DECLARE_METATYPE(zf::DataContainerPtrList)

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataContainer* c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataContainerPtr c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataContainer& c);
