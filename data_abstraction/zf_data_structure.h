#ifndef ZF_data_structure_H
#define ZF_data_structure_H

#include "zf_error.h"
#include "zf_uid.h"
#include "zf_row_id.h"
#include "zf_defs.h"

#include <QModelIndex>
#include <QSet>
#include <QExplicitlySharedDataPointer>
#include <QVector>
#include <QValidator>

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::PropertyConstraint& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::PropertyConstraint& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::PropertyLookup& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::PropertyLookup& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataProperty& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataProperty& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataPropertyPtr& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataPropertyPtr& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataStructure& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataStructure& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::DataStructurePtr& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::DataStructurePtr& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::PropertyLink& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::PropertyLink& obj);

namespace zf
{

class I_ExternalRequest;

//! Параметры Lookup запроса к CoreUids::MODEL_SERVICE
struct ModelServiceLookupRequestOptions
{
    //! Идентификатор сущности, из которой делать выборку
    zf::Uid lookup_entity_uid;
    //! Идентификатор свойства набора данных для выборки
    zf::PropertyID dataset_property_id;
    //! ID свойства - колонки для отображения данных
    zf::PropertyID display_column_id;
    //! Qt::UserRole для display_column_id
    int display_column_role = Qt::DisplayRole;
    //! ID свойства - колонки для внесения данных данных
    zf::PropertyID data_column_id;
    //! Qt::UserRole для data_column_id
    int data_column_role = Qt::DisplayRole;
};

/*! Свойство.
 * Для оптимизации скорости доступа, данные связанные со свойствами, хранятся в векторах, размер которых определяется
 * максимальным id свойств данной структуры. Поэтому желательно в рамках одной структуры задавать непрерывные номера
 * id начиная с 0 или 1 */
class ZCORESHARED_EXPORT DataProperty
{
public:
    DataProperty();
    DataProperty(const DataProperty& p);
    ~DataProperty();

    //! Преобразовать в QVariant
    QVariant toVariant() const;
    //! Восстановить из QVariant
    static DataProperty fromVariant(const QVariant& v);

    //! Преобразование в список id
    static PropertyIDList toIDList(const DataPropertyList& props);
    static PropertyIDList toIDList(const DataPropertySet& props);

    //! Поиск свойства для указанного модуля. Нельзя вызывать в плагинах. Результаты кэшируются
    static DataProperty property(
        //! Уникальный код модуля
        const EntityCode& entity_code,
        //! Идентификатор свойства
        const PropertyID& property_id,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);
    //! Поиск свойства для конкретной сущности. Нельзя вызывать в плагинах. Результаты НЕ кэшируются
    static DataProperty property(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Идентификатор свойства
        const PropertyID& property_id,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);

    //! Найти свойства по параметрам. Нельзя вызывать в плагинах. Результаты НЕ кэшируются
    static DataPropertyList propertiesByOptions(
        //! Уникальный код модуля
        const EntityCode& entity_code,
        //! Вид свойства
        PropertyType type,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options);
    //! Найти свойства по параметрам. Нельзя вызывать в плагинах. Результаты НЕ кэшируются
    static DataPropertyList propertiesByOptions(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Вид свойства
        PropertyType type,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options);

    //! Найти свойства по параметрам. Нельзя вызывать в плагинах. Результаты НЕ кэшируются
    static DataPropertyList propertiesByType(
        //! Уникальный код модуля
        const EntityCode& entity_code,
        //! Вид свойства
        PropertyType type);
    //! Найти свойства по параметрам. Нельзя вызывать в плагинах. Результаты НЕ кэшируются
    static DataPropertyList propertiesByType(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Вид свойства
        PropertyType type);

    //! Получить идентификатор сущности по свойству. Не вызывать в плагинах
    static Uid findPropertyUid(const DataProperty& property,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);
    //! Получить структуру по свойству. Не вызывать в плагинах
    static DataStructurePtr findStructure(const DataProperty& property,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);

    //! Найти колонки для указанного набора данных
    static DataPropertyList datasetColumnsByOptions(
        //! Уникальный код модуля
        const EntityCode& entity_code,
        //! ID свойства набора данных
        const PropertyID& dataset_property_id,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options);
    //! Найти колонки для указанного набора данных
    static DataPropertyList datasetColumnsByOptions(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! ID свойства набора данных
        const PropertyID& dataset_property_id,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options);

    //********** Конструкторы свойств **************
    //! Свойство поля, которое используется в отрыве от сущностей (например при внесении произвольных данных в DataContainer)
    static DataProperty independentFieldProperty(
        //! Идентификатор свойства
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {PropertyOption::Filtered});
    //! Набор данных, который используется в отрыве от сущностей (например при внесении произвольных данных в DataContainer)
    static DataProperty independentDatasetProperty(
        //! Идентификатор набора данных
        const PropertyID& id,
        //! Тип данных (Table или Tree)
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Сущность целиком - независимая от модулей
    static DataProperty independentEntityProperty();

    //! Сущность целиком
    static DataProperty entityProperty(
        //! Код сущности
        const EntityCode& entity_code,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Сущность целиком
    static DataProperty entityProperty(
        //! Идентификатор сущности. Для сущностей, которые имеют разную структуру данных в рамках одного кода
        const Uid& entity_uid,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Поле данных
    static DataProperty fieldProperty(
        //! Сущность
        const DataProperty& entity,
        //! Идентификатор поля
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Набор данных
    static DataProperty datasetProperty(
        //! Сущность
        const DataProperty& entity,
        //! Идентификатор набора данных
        const PropertyID& id,
        //! Тип данных (Table или Tree)
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Строка набора данных целиком
    static DataProperty rowProperty(
        //! Набор данных
        const DataProperty& dataset,
        //! Идентификатор строки
        const RowID& row_id,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Колонка набора данных целиком
    static DataProperty columnProperty(
        //! Набор данных
        const DataProperty& dataset,
        //! Идентификатор
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {PropertyOption::Filtered},
        //! Группа колонок
        const DataProperty& column_group = DataProperty());

    //! Группа колонок набора данных
    static DataProperty columnGroupProperty(
        //! Набор данных
        const DataProperty& dataset,
        //! Идентификатор
        const PropertyID& id);

    //! Любая колонка для строки набора данных
    static DataProperty rowPartialProperty(
        //! Набор данных
        const DataProperty& dataset,
        //! Идентификатор строки
        const RowID& row_id,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Любая строка для колонки набора данных
    static DataProperty columnPartialProperty(
        //! Набор данных
        const DataProperty& dataset,
        //! Идентификатор колонки
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Ячейка набора данных
    static DataProperty cellProperty(
        //! Строка набора данных
        const DataProperty& row,
        //! Колонка набора данных
        const DataProperty& column,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    DataProperty& operator=(const DataProperty& p);
    bool operator==(const DataProperty& p) const;
    bool operator!=(const DataProperty& p) const;
    bool operator<(const DataProperty& p) const;

    bool isValid() const;
    //! Свойство может иметь значение (поле, ячейка таблицы и т.п.)
    bool canHaveValue() const;
    //! Свойство не связано с конкретной сущностью
    bool isEntityIndependent() const;

    //! Свойство - сущность (PropertyType::Entity)
    bool isEntity() const;
    //! Свойство - поле (PropertyType::Field)
    bool isField() const;
    //! Свойство - набор данных (PropertyType::Dataset)
    bool isDataset() const;
    //! Свойство - ячейка набора данных (PropertyType::Cell)
    bool isCell() const;
    //! Свойство - строка (PropertyType::RowFull или PropertyType::RowPartial)
    bool isRow() const;
    //! Свойство - колонка (PropertyType::ColumnFull или PropertyType::ColumnPartial)
    bool isColumn() const;
    //! Свойство - колонка, строка или ячейка набора данных
    bool isDatasetPart() const;

    /*! Идентификатор свойства. Действителен для полей, наборов данных, колонок наборов данных.
     * Для моделей совпадает с entityСode */
    PropertyID id() const;

    //! Код сущности модели владельца (для свойств, связанных с сущностями)
    EntityCode entityCode() const;
    //! Сущность - владелец (для свойств, связанных с сущностями)
    DataProperty entity() const;
    //! Идентикатор сущности модели владельца (для свойств, связанных с сущностями)
    //! Используется для сущностей, которые имеют разную структуру в рамках одного кода
    Uid entityUid() const;

    //! Возвращает указатель на структуру данных (не использовать в плагинах)
    //! Только для свойств, связанных с сущностями
    DataStructurePtr structure(bool halt_if_not_found = true) const;

    //! Текстовая расшифровка для отладки
    QString toPrintable() const;

    //! Вид свойства
    PropertyType propertyType() const;
    //! Тип данных
    DataType dataType() const;

    //! Параметры свойства
    PropertyOptions options() const;

    //! id перевода
    QString translationID() const;
    //! Название на текущем языке
    QString name(
        //! Показывать предупреждение при отсутствии перевода
        bool show_translation_warning = true) const;
    //! Имя задано
    bool hasName() const;

    //! Значение по умолчанию (для всех кроме DataType::Dataset)
    QVariant defaultValue() const;

    //! Содержится ли свойство property в этом свойстве (для одинаковых - true)
    bool contains(const DataProperty& property) const;

    //! Привести значение к типу данных свойства
    Error convertValue(const QVariant& value, QVariant& result) const;

    //! Очистить свойство
    void clear();

    //! Набор данных (для свойств, связанных с наборами данных)
    DataProperty dataset() const;

    //! Группа колонок (для свойств, входящих в группу - PropertyType::ColumnGroup)
    DataProperty columnGroup() const;

    //! Строка (для свойств, связанных с наборами данных)
    DataProperty row() const;
    //! Идентификатор строки (для свойств, связанных с наборами данных)
    const RowID& rowId() const;

    //! Колонка
    DataProperty column() const;

    //! Задать колонки для набора данных или группы колонок
    void setColumns(const DataPropertyList& columns);
    //! Добавить колонку для набора данных или группы колонок
    DataProperty addColumn(const DataProperty& column);
    //! Добавить колонку для набора данных или группы колонок
    DataProperty& operator<<(const DataProperty& column);
    //! Добавить колонку для набора данных или группы колонок
    DataProperty addColumn(
        //! Идентификатор
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {PropertyOption::Filtered});

    //! Колонки набора данных или состав группы колонок (только для PropertyType::Dataset или PropertyType::ColumnGroup)
    const DataPropertyList& columns() const;
    //! Колонки набора данных в которых есть ограничения (только для PropertyType::Dataset или PropertyType::ColumnGroup)
    const DataPropertyList& columnsConstraint() const;
    //! Номер колонки (только для PropertyType::Dataset)
    int columnPos(const DataProperty& column_property) const;
    //! Номер колонки по идентификатору ее свойтсва
    //! Для DataType::Dataset - порядковый номер в рамках всего набора данных
    //! Для PropertyType::ColumnGroup - порядковый номер в рамках группы
    int columnPos(const PropertyID& column_property_id) const;
    //! Номера колонок с указанным свойством
    QList<int> columnPos(PropertyOption option) const;
    //! Порядковый номер колонки в наборе данных либо в группе
    int pos() const;
    //! Количество колонок для набора данных (только для PropertyType::Dataset)
    int columnCount() const;

    //! Задать тектст перевода (в большинстве случаев текст перевода надо задавать при создании свойств)
    //! Этот метод для особых случаев
    void setTranslationID(const QString& translation_id);

    //! Список колонок набора данных по свойствам. Только для наборов данных
    DataPropertyList columnsByOptions(const PropertyOptions& options) const;
    //! Колонка с id
    DataProperty idColumnProperty() const;
    const PropertyID& idColumnPropertyId() const;
    //! Колонка с именем
    DataProperty nameColumn() const;
    //! Колонка с полным именем
    DataProperty fullNameColumn() const;

public:
    //! Откуда должен производится выбор значений
    PropertyLookupPtr lookup() const;

    //! Установить выборку из фиксированного списка значений
    DataProperty& setLookupList(
        //! Список значений
        const QVariantList& list_values,
        //! Список расшифровок значений (константы перевода)
        const QStringList& list_names = QStringList(),
        //! Значение можно редактировать напрямую (пока не поддерживается)
        bool editable = false);
    //! Установить выборку из набора данных указанной сущности
    DataProperty& setLookupEntity(
        //! Код сущности
        const EntityCode& entity_code,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! Qt::ItemDataRole для display_column_id
        int display_column_role = Qt::DisplayRole,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id = {},
        //! Qt::ItemDataRole для dataColumnID
        int data_column_role = Qt::DisplayRole,
        //! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        const PropertyID& dataset_key_column_id = PropertyID(),
        //! Роль для колонки набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        int dataset_key_column_role = Qt::DisplayRole);
    //! Установить выборку из набора данных указанной сущности
    DataProperty& setLookupEntity(
        //! Идентификатор сущности
        const Uid& entity,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! Qt::ItemDataRole для display_column_id
        int display_column_role,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Qt::ItemDataRole для dataColumnID
        int data_column_role,
        //! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        const PropertyID& dataset_key_column_id = PropertyID(),
        //! Роль для колонки набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        int dataset_key_column_role = Qt::DisplayRole);
    //! Установить выборку из набора данных указанной сущности
    DataProperty& setLookupEntity(
        //! Код сущности
        const EntityCode& entity_code,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id = {},
        //! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        const PropertyID& dataset_key_column_id = PropertyID());
    //! Установить выборку из набора данных указанной сущности
    DataProperty& setLookupEntity(
        //! Идентификатор сущности
        const Uid& entity,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id = {},
        //! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        const PropertyID& dataset_key_column_id = PropertyID());

    //! Установить выборку из набора данных указанной сущности. Информация об колонках отображения и ID берется из
    //! PropertyOptions набора данных lookup
    DataProperty& setLookupEntity(
        //! Код сущности
        const EntityCode& entity_code,
        //! Идентификатор свойства набора данных для выборки. Если не задано, то сущность должна иметь только один набор
        //! данных
        const PropertyID& dataset_property_id = {});
    //! Установить выборку из набора данных указанной сущности. Информация об колонках отображения и ID берется из
    //! PropertyOptions набора данных lookup
    DataProperty& setLookupEntity(
        //! Идентификатор сущности
        const Uid& entity,
        //! Идентификатор свойства набора данных для выборки. Если не задано, то сущность должна иметь только один набор
        //! данных
        const PropertyID& dataset_property_id = {});

    //! Установить выборку из набора данных, который определяется динамически через интерфейс I_DataWidgetsLookupModels
    DataProperty& setLookupDynamic(
        //! Значение можно редактировать напрямую (пока не поддерживается)
        bool editable = false);

    //! Установить выборку из внешнего сервиса
    DataProperty& setLookupRequest(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const Uid& request_service_uid,
        //! Тип запроса (зависит от специфики внешнего сервиса)
        const QString& request_type = {},
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property = {},
        /*! Поля или колонки таблицы, содержащие родительский идентификатор. По порядку, начиная с головного
         * Только для LookupType::Request
         * Родительские идентификаторы будут необходимы для активации работы пользователя со свойством. Пока они не заполнены - поле заблокировано
         * Также они передаются в запрос на получения списка вариантов */
        const PropertyIDList& parent_key_properties = {},
        //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
        const PropertyIDMultiMap& attributes = {},
        //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
        const QStringList& attributes_to_clear = {},
        /*! Набор полей, на основании которых надо формировать идентификатор для запроса к внешнему сервису. Если не задано, то используется свойство для которого задан лукап
         * Идентификатор формируется на основании первого не пустого значения в последовательности. 
         * Т.е. если есть поля CITY_GUID, AREA_GUID, REGION_GUID,  и из них заполнены AREA_GUID и REGION_GUID, то используется значение из AREA_GUID
         * Только для LookupType::Request
         * Выполняет вызов addLinkDataSourcePriority(request_id_source_properties) */
        const PropertyIDList& request_id_source_properties = {},
        //! Параметры (зависит от специфики внешнего сервиса)
        const QVariant& options = {});
    //! Установить выборку из внешнего сервиса
    DataProperty& setLookupRequest(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const Uid& request_service_uid,
        //! Тип запроса (зависит от специфики внешнего сервиса)
        const QString& request_type,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property,
        //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
        const PropertyIDMultiMap& attributes,
        //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
        const QStringList& attributes_to_clear = {},
        /*! Набор полей, на основании которых надо формировать идентификатор для запроса к внешнему сервису. Если не задано, то используется свойство для которого задан лукап
         * Идентификатор формируется на основании первого не пустого значения в последовательности. 
         * Т.е. если есть поля CITY_GUID, AREA_GUID, REGION_GUID,  и из них заполнены AREA_GUID и REGION_GUID, то используется значение из AREA_GUID
         * Только для LookupType::Request
         * Выполняет вызов addLinkDataSourcePriority(request_id_source_properties) */
        const PropertyIDList& request_id_source_properties = {},
        //! Параметры (зависит от специфики внешнего сервиса)
        const QVariant& options = {});
    //! Установить выборку из внешнего сервиса
    DataProperty& setLookupRequest(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const EntityCode& request_service_entity_code,
        //! Тип запроса (зависит от специфики внешнего сервиса)
        const QString& request_type = {},
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property = {},
        /*! Поля или колонки таблицы, содержащие родительский идентификатор. По порядку, начиная с головного
         * Только для LookupType::Request
         * Родительские идентификаторы будут необходимы для активации работы пользователя со свойством. Пока они не заполнены - поле заблокировано
         * Также они передаются в запрос на получения списка вариантов */
        const PropertyIDList& parent_key_properties = {},
        //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
        const PropertyIDMultiMap& attributes = {},
        //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
        const QStringList& attributes_to_clear = {},
        /*! Набор полей, на основании которых надо формировать идентификатор для запроса к внешнему сервису. Если не задано, то используется свойство для которого задан лукап
         * Идентификатор формируется на основании первого не пустого значения в последовательности. 
         * Т.е. если есть поля CITY_GUID, AREA_GUID, REGION_GUID,  и из них заполнены AREA_GUID и REGION_GUID, то используется значение из AREA_GUID
         * Только для LookupType::Request
         * Выполняет вызов addLinkDataSourcePriority(request_id_source_properties) */
        const PropertyIDList& request_id_source_properties = {},
        //! Параметры (зависит от специфики внешнего сервиса)
        const QVariant& options = {});
    //! Установить выборку из внешнего сервиса
    DataProperty& setLookupRequest(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const EntityCode& request_service_entity_code,
        //! Тип запроса (зависит от специфики внешнего сервиса)
        const QString& request_type,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property,
        //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
        const PropertyIDMultiMap& attributes,
        //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
        const QStringList& attributes_to_clear = {},
        /*! Набор полей, на основании которых надо формировать идентификатор для запроса к внешнему сервису. Если не задано, то используется свойство для которого задан лукап
         * Идентификатор формируется на основании первого не пустого значения в последовательности. 
         * Т.е. если есть поля CITY_GUID, AREA_GUID, REGION_GUID,  и из них заполнены AREA_GUID и REGION_GUID, то используется значение из AREA_GUID
         * Только для LookupType::Request
         * Выполняет вызов addLinkDataSourcePriority(request_id_source_properties) */
        const PropertyIDList& request_id_source_properties = {},
        //! Параметры (зависит от специфики внешнего сервиса)
        const QVariant& options = {});

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Идентификатор сущности, из которой делать выборку
        const Uid& lookup_entity_uid,
        //! Идентификатор свойства набора данных для выборки. Если не задано, то сущность должна иметь только один набор
        //! данных
        const PropertyID& dataset_property_id = {});
    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Код сущности, из которой делать выборку
        const EntityCode& lookup_entity_code,
        //! Идентификатор свойства набора данных для выборки. Если не задано, то сущность должна иметь только один набор
        //! данных
        const PropertyID& dataset_property_id = {});

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Идентификатор сущности, из которой делать выборку
        const Uid& lookup_entity_uid,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! Qt::ItemDataRole для display_column_id
        int display_column_role,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Qt::ItemDataRole для data_column_id
        int data_column_role,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property);

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Код сущности, из которой делать выборку
        const EntityCode& lookup_entity_code,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! Qt::ItemDataRole для display_column_id
        int display_column_role,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Qt::ItemDataRole для data_column_id
        int data_column_role,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property);

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Код сущности, из которой делать выборку
        const EntityCode& lookup_entity_code,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id);

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Код сущности, из которой делать выборку
        const Uid& lookup_entity_uid,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id);

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Идентификатор сущности, из которой делать выборку
        const Uid& lookup_entity_uid,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property);

    //! Установить выборку из сервиса MODEL_SERVICE
    DataProperty& setModelRequest(
        //! Код сущности, из которой делать выборку
        const EntityCode& lookup_entity_code,
        //! Идентификатор свойства набора данных для выборки
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных
        const PropertyID& display_column_id,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Поле или колонка таблицы, содержащая расшифровку. Выполняет вызов addLinkLookupFreeText
        const PropertyID& request_service_value_property);

    //! Ограничения
    QList<PropertyConstraintPtr> constraints() const;
    //! Указать, что свойство необходимо инициализировать
    DataProperty& setConstraintRequired(
        //! Сообщение об ошибке
        const QString& message = QString(),
        //! Уровень ошибки
        InformationType highlight_type = InformationType::Error,
        //! К какому свойству привязывать отображение ошибки в окне редактирования
        const PropertyID& show_higlight_property = {},
        //! Разные параметры
        ConstraintOptions options = ConstraintOption::Default,
        //! Параметры ошибки
        const HighlightOptions& highlight_options = {});
    //! Указать, что свойство необходимо инициализировать
    DataProperty& setConstraintRequired(
        //! Уровень ошибки
        InformationType highlight_type,
        //! К какому свойству привязывать отображение ошибки в окне редактирования
        const PropertyID& show_higlight_property = {},
        //! Разные параметры
        ConstraintOptions options = ConstraintOption::Default,
        //! Параметры ошибки
        const HighlightOptions& highlight_options = {});
    //! Максимально допустимое количество символов для текстового поля.
    DataProperty& setConstraintMaxTextLength(
        //! Максимальное количество символов
        int length = 100,
        //! Сообщение об ошибке
        const QString& message = QString(),
        //! Уровень ошибки
        InformationType highlight_type = InformationType::Error,
        //! К какому свойству привязывать отображение ошибки в окне редактирования
        const PropertyID& show_higlight_property = {},
        //! Разные параметры
        ConstraintOptions options = ConstraintOption::Default,
        //! Параметры ошибки
        const HighlightOptions& highlight_options = {});
    //! Регулярное выражение
    DataProperty& setConstraintRegexp(
        //! Регулярное выражение
        const QString& regexp,
        //! Сообщение об ошибке
        const QString& message = QString(),
        //! Уровень ошибки
        InformationType highlight_type = InformationType::Error,
        //! К какому свойству привязывать отображение ошибки в окне редактирования
        const PropertyID& show_higlight_property = {},
        //! Разные параметры
        ConstraintOptions options = ConstraintOption::Default,
        //! Параметры ошибки
        const HighlightOptions& highlight_options = {});
    //! Регулярное выражение
    DataProperty& setConstraintValidator(
        //! Валидатор. У валидатора должен быть установлен parent
        const QValidator* validator,
        //! Сообщение об ошибке
        const QString& message = QString(),
        //! Уровень ошибки
        InformationType highlight_type = InformationType::Error,
        //! К какому свойству привязывать отображение ошибки в окне редактирования
        const PropertyID& show_higlight_property = {},
        //! Разные параметры
        ConstraintOptions options = ConstraintOption::Default,
        //! Параметры ошибки
        const HighlightOptions& highlight_options = {});

    //! Имеет ли ограничения
    bool hasConstraints(
        //! Если не задано, то все
        ConditionTypes types = {}) const;

public:
    //! Указывает на поле, в которое надо помещать результат свободного ввода текста
    //! Используется для работы с ItemSelector в режиме editable
    //! Добавляет PropertyLink типа PropertyLinkType::LookupFreeText
    DataProperty& addLinkLookupFreeText(const PropertyID& property_id);

    /*! Правило автоматического обновления значения свойства на основании другого свойства
     * Сейчас поддерживается только обновление из lookup таблиц. Пример:
     * В нашей модели есть поле ID_USER, где хранится id пользователя и поле USER_NAME, куда надо автоматически поместить 
     * имя пользователя. Устанавливаем setDataSource для поля USER_NAME. При этом мы связываем наше поле ID_USER с моделью 
     * списка пользователей и указываем его набор данных и колонку, отвечающую за имя пользователя. 
     * После этого любое изменение ID_USER приведет к заполнению поля USER_NAME
     * Добавляет PropertyLink типа PropertyLinkType::DataSource */
    DataProperty& addLinkDataSource(
        //! Идентификатор свойства, которое хранит ключевое значение сущности откуда надо забирать данные
        const PropertyID& key_property_id,
        //! Сущность, из которой надо забирать данные
        Uid source_entity,
        //! Идентификатор колонки набора данных, из которой надо брать значение
        const PropertyID& source_column_id);
    //! Задать правило автоматического обновления значения свойства на основании lookup модели
    //! Добавляет PropertyLink типа PropertyLinkType::DataSource
    DataProperty& addLinkDataSource(
        //! Идентификатор свойства, которое хранит ключевое значение сущности откуда надо забирать данные
        const PropertyID& key_property_id,
        //! Сущность, из которой надо забирать данные
        const EntityCode& source_entity_code,
        //! Идентификатор колонки набора данных, из которой надо брать значение
        const PropertyID& source_column_id);
    //! Задать правило автоматического обновления значения свойства на основании lookup модели
    //! Добавляет PropertyLink типа PropertyLinkType::DataSource
    DataProperty& addLinkDataSourceCatalog(
        //! Идентификатор свойства, которое хранит ключевое значение сущности откуда надо забирать данные
        const PropertyID& key_property_id,
        //! Код каталога, из которой надо забирать данные
        const EntityCode& catalog_code,
        //! Идентификатор колонки каталога, из которой надо брать значение
        const PropertyID& source_column_id);

    //! Правило автоматического обновления поля на основании первого заданного значения в списке других свойств
    DataProperty& addLinkDataSourcePriority(
        //! Список полей, из которых берется значение. Поля перебираются по порядку и как только поле содержит значение, оно заносится в основное поле
        //! Если нигде нет значений, то основное поле обнуляется
        const PropertyIDList& properties);

    //! Связи с другими свойствами
    const QList<PropertyLinkPtr>& links() const;
    //! Связи с другими свойствами для указанного типа
    QList<PropertyLinkPtr> links(PropertyLinkType type) const;

    //! Есть ли связи с другими свойствами
    bool hasLinks() const;
    //! Есть ли связи с другими свойствами для указанного типа
    bool hasLinks(PropertyLinkType type) const;

public:
    //! Уникальный ключ свойства
    QString uniqueKey() const;
    //! Требуется ли тип данных для свойства
    static bool isDataTypeRequired(PropertyType property_type);

    //! Ключ для работы qHash
    uint hashKey() const;

    //! Номер для сортировки свойств в порядке их добавления
    int naturalIndex() const;

    //! Номер типа метаданных QVariant
    static int metaType();

private:
    DataProperty(const PropertyID& id, PropertyType property_type, DataType data_type, const PropertyOptions& options, const DataProperty& entity,
        const EntityCode& entity_code, const Uid& entity_uid, const DataProperty& dataset, const DataProperty& column_group, const QVariant& default_value,
        const DataProperty& row, const RowID& row_id, const DataProperty& column, const QString& translation_id);

    void addConstraintHelper(const PropertyConstraintPtr& c);
    void addLinkHelper(const PropertyLinkPtr& link);

    //! Привести значение к типу данных свойства
    Error convertValueHelper(const QVariant& value, QVariant& result) const;

    //! Задать значение набора данных
    void setDataset(const DataProperty& dataset);

    //! Задать группу
    void setColumnGroup(const DataProperty& column_group);

    void detachHelper();

    //! DataProperty в JSON (выгрузка только минимального набора свойств)
    QJsonObject toJson() const;
    //! Восстановить из JSON
    static DataProperty fromJson(const QJsonObject& json,
        //! Сущность, набор данных
        const DataProperty& parent, Error& error);
    //! Поле в отрыве от сущности
    static DataProperty fromJson(const QJsonObject& json, Error& error);
    static Error fromJsonHelper(const QJsonObject& json, int& id, PropertyType& property_type, DataType& data_type);

    //! Разделяемые данные
    QExplicitlySharedDataPointer<DataProperty_SharedData> _d;

    //! Порядковые номера колонок. Количество равно максимальному id из всех колонок
    mutable std::unique_ptr<QVector<int>> _columns_pos;

    //! Хэшированные свойства для быстрого поиска через property()
    static std::unique_ptr<QHash<QString, DataProperty>> _property_hash;
    static QMutex _property_hash_mutex;

    static int _metatype;

    friend class DataStructure;
    friend class Framework;
    friend QDataStream& ::operator<<(QDataStream& out, const zf::DataProperty& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::DataProperty& obj);
};

inline DataPropertySet operator+(const DataProperty& p1, const DataProperty& p2)
{
    return DataPropertySet({p1}).unite(DataPropertySet({p2}));
}
inline DataPropertySet operator+(const DataPropertySet& p1, const DataProperty& p2)
{
    return DataPropertySet(p1).unite(DataPropertySet({p2}));
}
inline DataPropertySet operator+(const DataProperty& p1, const DataPropertySet& p2)
{
    return DataPropertySet({p1}).unite(DataPropertySet(p2));
}
inline bool operator==(const DataProperty& p, const PropertyID& property_id)
{
    return p.id() == property_id
           && (p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Dataset || p.propertyType() == PropertyType::ColumnFull);
}
inline bool operator!=(const DataProperty& p, const PropertyID& property_id)
{
    return !operator==(p, property_id);
}
inline bool operator==(const PropertyID& property_id, const DataProperty& p)
{
    return operator==(p, property_id);
}
inline bool operator!=(const PropertyID& property_id, const DataProperty& p)
{
    return operator!=(p, property_id);
}

//! Связь между свойствами
class ZCORESHARED_EXPORT PropertyLink
{
public:
    PropertyLink();
    PropertyLink(const PropertyLink& l);

    PropertyLink& operator=(const PropertyLink& l);
    bool operator==(const PropertyLink& l) const;

    bool isValid() const;

    //! Вид связи
    PropertyLinkType type() const;
    //! Главное свойство
    PropertyID mainPropertyId() const;

    //! Связанное свойство
    PropertyID linkedPropertyId() const;

    //**** Для PropertyLinkType::DataSource ****
    //! Сущность, из которой надо забирать данные
    Uid dataSourceEntity() const;
    //! Идентификатор колонки, связанного свойства, из которой надо брать значение
    PropertyID dataSourceColumnId() const;

    //*** Для PropertyLinkType::DataSourcePriority ***
    //! Правило автоматического обновления одного поля на основании первого заданного значения в списке других свойств
    const PropertyIDList& dataSourcePriority() const;

private:
    //! PropertyLinkType::LookupFreeText
    PropertyLink(const PropertyID& main_property_id, const PropertyID& linked_property_id);
    //! PropertyLinkType::DataSource
    PropertyLink(
        const PropertyID& main_property_id, const PropertyID& linked_property_id, const Uid& data_source_entity, const PropertyID& data_source_column_id);
    //! PropertyLinkType::DataSourcePriority
    PropertyLink(const PropertyIDList& sources);

    //! Вид связи
    PropertyLinkType _type = PropertyLinkType::Undefined;
    //! Главное свойство
    PropertyID _main_property_id;
    //! Связанное свойство
    PropertyID _linked_property_id;

    // Только для PropertyLinkType::DataSource
    //! Сущность, из которой надо забирать данные
    Uid _data_source_entity;
    //! Идентификатор колонки набора данных, из которой надо брать значение
    PropertyID _data_source_column_id;

    // Только для PropertyLinkType::DataSourcePriority
    //! Правило автоматического обновления одного поля на основании первого заданного значения в списке других свойств
    PropertyIDList _data_source_priority;

    friend class DataProperty;
    friend QDataStream& ::operator<<(QDataStream& out, const zf::PropertyLink& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::PropertyLink& obj);
};

//! Ограничение, накладываемое на свойство
class ZCORESHARED_EXPORT PropertyConstraint
{
public:
    PropertyConstraint();
    PropertyConstraint(const PropertyConstraint& p);
    PropertyConstraint& operator=(const PropertyConstraint& p);

    bool isValid() const;

    //! Тип ограничения
    ConditionType type() const;
    //! Сообщение при невыполнении ограничения
    QString message() const;
    //! Тип (уровень) ошибки при обработке ограничения
    InformationType highlightType() const;
    //! Параметры ошибки
    HighlightOptions highlightOptions() const;
    //! Дополнительный данные для ограничения (зависит от типа)
    QVariant addData() const;
    //! Дополнительный указатель для ограничения (зависит от типа)
    void* addPtr() const;
    //! Параметры ограничения
    ConstraintOptions options() const;
    //! К какому свойству привязывать отображение ошибки в окне редактирования
    PropertyID showHiglightProperty() const;

    //! Регулярное выражение (только для ConditionType::RegExp)
    QRegularExpression regularExpression() const;
    //! Валидатор (только для ConditionType::Validator иначе nullptr)
    QValidator* validator() const;

private:
    PropertyConstraint(ConditionType type, const QString& message, InformationType highlight_type, const PropertyOptions& options, QVariant add_data,
        void* add_ptr, ConstraintOptions c_options, const PropertyID& show_higlight_property, const HighlightOptions& highlight_options);

private:
    ConditionType _type = ConditionType::Undefined;
    QString _message;
    InformationType _highlight_type = InformationType::Invalid;
    PropertyOptions _options;
    QVariant _add_data;
    QRegularExpression _regexp;
    void* _add_ptr = nullptr;
    ConstraintOptions _c_options;
    //! К какому свойству привязывать отображение ошибки в окне редактирования
    PropertyID _show_higlight_property;
    HighlightOptions _highlight_options;

    friend class DataProperty;
    friend QDataStream& ::operator<<(QDataStream& out, const zf::PropertyConstraint& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::PropertyConstraint& obj);
};

//! Определение выборки из списка значений для свойства
class ZCORESHARED_EXPORT PropertyLookup
{
public:
    PropertyLookup();

    bool isValid() const;

    //! К какому свойству относиться lookup
    PropertyID masterPropertyID() const;

    //! Тип выборки
    LookupType type() const;

    //! Сущность, из которой надо выбирать значения (только для LookupType::Dataset)
    Uid listEntity() const;
    /*! ID свойства - набора данных из listEntity (только для LookupType::Dataset)
     * Если при инициализации lookup в плагине, не задано (или указано -1), то lookup сущность должна содержать только
     * один набор данных. Не вызывать в плагинах */
    PropertyID datasetPropertyID() const;
    //! DataProperty, полученный на основании datasetPropertyID. Не вызывать в плагинах
    DataProperty datasetProperty() const;

    /*! ID свойства - колонки для отображения данных (только для LookupType::Dataset)
     * Если при инициализации lookup в плагине, колонка была не задана (или указано -1), то она определяется
     * автоматически на основании PropertyOption::Id при первом вызове данного метода. Не вызывать в плагинах */
    PropertyID displayColumnID() const;
    //! DataProperty, полученный на основании displayColumnID. Не вызывать в плагинах
    DataProperty displayColumn() const;
    /*! Qt::ItemDataRole для displayColumnID. При автоматическом определении displayColumnID, должна быть не задана или
     * Qt::DisplayRole */
    int displayColumnRole() const;

    /*! ID свойства - колонка listEntity, где лежат значения, которые надо внести в свойство, связанное с данным lookup (только для LookupType::Dataset)
     * Если при инициализации lookup в плагине, колонка была не задана (или указано -1), то она определяется
     * автоматически на основании PropertyOption::Name при первом вызове данного метода. Не вызывать в плагинах */
    PropertyID dataColumnID() const;
    //! DataProperty, полученный на основании dataColumnID. Не вызывать в плагинах
    DataProperty dataColumn() const;
    /*! Qt::ItemDataRole для dataColumnID. При автоматическом определении displayColumnID, должна быть не задана или
     * Qt::DisplayRole */
    int dataColumnRole() const;

    /*! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
     * Если при инициализации lookup в плагине, колонка была не задана (или указано -1), то она определяется
    * автоматически на основании PropertyOption::SimpleDatasetID при первом вызове данного метода. Не вызывать в плагинах */
    PropertyID datasetKeyColumnID() const;
    //! Роль для колонки набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
    int datasetKeyColumnRole() const;

    //! Фактические значения для выбора из списка (только для LookupType::List)
    const QVariantList& listValues() const;
    //! Отображаемые значения для выбора из списка (только для LookupType::List)
    const QStringList& listNames() const;
    //! Получить расшифровку значения для LookupType::List
    QString listName(const QVariant& id_value) const;

    //! Значение поля можно редактировать напрямую, минуя выбор из lookup (пока не поддерживается)
    bool isEditable() const;

    //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису. Только для LookupType::Request
    Uid requestServiceUid() const;
    //! Тип запроса (зависит от специфики внешнего сервиса)
    QString requestType() const;
    //! Интерфейс для запросов к внешнему сервису. Не вызывать в плагинах
    I_ExternalRequest* requestService() const;

    //! Параметры запроса к CoreUids::MODEL_SERVICE (DataProperty::setModelRequest)
    ModelServiceLookupRequestOptions modelServiceOptions() const;

    /*! Поля или колонки таблицы, содержащие родительский идентификатор. По порядку, начиная с головного
     * Только для LookupType::Request
     * Родительские идентификаторы будут необходимы для активации работы пользователя со свойством. Пока они не заполнены - поле заблокировано
     * Также они передаются в запрос на получения списка вариантов */
    const PropertyIDList& parentKeyProperties() const;

    //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
    const PropertyIDMultiMap& attributes() const;
    //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
    const QStringList& attributesToClear() const;

    //! Произвольные данные
    QVariant data() const;

private:
    PropertyLookup(
        //! К какому свойству относиться lookup
        DataProperty* master_property,
        //! Тип выборки
        LookupType type,
        //! Сущность, из которой надо выбирать значения (только для LookupType::Dataset)
        const Uid& list_entity,
        //! ID свойства - набора данных из listEntity (только для LookupType::Dataset)
        const PropertyID& dataset_property_id,
        //! ID свойства - колонки для отображения данных (только для LookupType::Dataset)
        const PropertyID& display_column_id,
        //! Qt::ItemDataRole для display_column_id
        int display_column_role,
        //! ID свойства - колонки для внесения данных данных
        const PropertyID& data_column_id,
        //! Qt::ItemDataRole для dataColumnID
        int data_column_role,
        //! Колонка набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        const PropertyID& dataset_key_column_id,
        //! Роль для колонки набора данных, к которому привязан этот  lookup (только для PropertyOption::SimpleDataset)
        int dataset_key_column_id_role,
        //! Список значений (только для LookupType::List)
        const QVariantList& list_values,
        //! Список расшифровок значений (только для LookupType::List)
        const QStringList& list_names,
        //! Значение поля можно редактировать напрямую, минуя выбор из lookup (пока не поддерживается)
        bool editable,
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису. Только для LookupType::Request
        const Uid& request_service_uid,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type,
        /*! Поля или колонки таблицы, содержащие родительский идентификатор. По порядку, начиная с головного
         * Только для LookupType::Request
         * Родительские идентификаторы будут необходимы для активации работы пользователя со свойством. Пока они не заполнены - поле заблокировано
         * Также они передаются в запрос на получения списка вариантов */
        const PropertyIDList& parent_key_properties,
        //! Произвольные данные.Только для LookupType::Request (Зависит от специфики внешнего сервиса)
        const QVariant& data,
        //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов.
        //! Только для LookupType::Request. Зависит от специфики внешнего сервиса
        const PropertyIDMultiMap& attributes,
        //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
        const QStringList& attributes_to_clear);

    void init();

private:
    LookupType _type = LookupType::Undefined;

    Uid _list_entity;
    mutable PropertyID _dataset_property_id;

    mutable PropertyID _display_column_id;
    int _display_column_role = Qt::DisplayRole;

    mutable PropertyID _data_column_id;
    int _data_column_role = Qt::DisplayRole;

    mutable PropertyID _dataset_key_column_id;
    int _dataset_key_column_id_role = Qt::DisplayRole;

    QVariantList _list_values;
    QStringList _list_names;
    bool _editable = false;

    //! К какому свойству привязан lookup (сущность)
    EntityCode _master_entity;
    //! К какому свойству привязан lookup (свойство)
    PropertyID _master_property_id;
    mutable DataProperty _master_property;

    //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису. Только для LookupType::Request
    Uid _request_service_uid;
    //! Тип запроса (зависит от специфики внешнего сервиса:
    //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
    QString _request_type;
    //! Интерфейс запросов к внешнему сервису
    mutable I_ExternalRequest* _request_service = nullptr;

    //! Поля или колонки таблицы, содержащие родительский идентификатор. По порядку, начиная с головного
    //! Только для LookupType::Request
    PropertyIDList _parent_key_properties;

    //! Идентификаторы аттрибутов и поля, в которые должны быть занесены значения этих атрибутов. Зависит от специфики внешнего сервиса
    PropertyIDMultiMap _attributes;
    //! Идентификаторы аттрибутов, которые надо очистить при необходимости сброса атрибутов. Зависит от специфики внешнего сервиса
    QStringList _attributes_to_clear;

    //! Произвольные данные
    QVariant _data;

    friend class DataProperty;
    friend QDataStream& ::operator<<(QDataStream& out, const zf::PropertyLookup& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::PropertyLookup& obj);
    friend QDataStream& ::operator<<(QDataStream& out, const zf::DataProperty& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::DataProperty& obj);
};

//! Описание структуры данных
class ZCORESHARED_EXPORT DataStructure : public QObject
{
    Q_OBJECT
public:
    DataStructure();
    DataStructure(const DataStructure& m);

    /*! Получить структуру сущности с указанным кодом. Не вызывать в плагинах
     * Для сущностей, структура которых определена в конструкторе плагина */
    static DataStructurePtr structure(const EntityCode& code,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);
    //! Получить структуру сущности для конкретной сущности. Не вызывать в плагинах
    static DataStructurePtr structure(const Uid& entity_uid,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);
    //! Получить структуру сущности для свойства типа Entity. Не вызывать в плагинах
    static DataStructurePtr structure(const DataProperty& entity_property,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true);

    //! Преобразование в список id
    static PropertyIDList toIDList(const DataPropertyList& props);
    static PropertyIDList toIDList(const DataPropertySet& props);
    //! Преобразование в список DataProperty
    DataPropertyList toPropertyList(const PropertyIDList& props) const;
    DataPropertyList toPropertyList(const PropertyIDSet& props) const;
    //! Преобразование списка id в DataPropertyList
    DataPropertyList fromIDList(const PropertyIDList& props) const;
    //! Преобразование списка DataProperty  в DataPropertyIDList
    static PropertyIDList fromPropertyList(const DataPropertyList& props);

    //! Создать структуру не связанную с конкретной сущностью
    static DataStructurePtr independentStructure(
        //! Идентификатор структуры (> 0)
        const PropertyID& id = PropertyID(1),
        //! Версия структуры данных
        int structure_version = 1,
        //! Набор свойств
        const DataPropertyList& properties = DataPropertyList());

    //! Сущность целиком - независимая от модулей
    static DataProperty independentEntityProperty();

    //! Создать структуру связанную с конкретной сущностью
    static DataStructurePtr entityStructure(
        //! Сущность
        const DataProperty& entity_property,
        //! Версия структуры данных
        int structure_version = 1,
        //! Набор свойств
        const DataPropertyList& properties = DataPropertyList());
    //! Создать структуру связанную с конкретной сущностью
    static DataStructurePtr entityStructure(
        //! Сущность
        const zf::EntityCode& entity_code,
        //! Версия структуры данных
        int structure_version = 1);

    //! Создать из структуры на базе сущностей, независимую от сущностей структуру
    static DataStructurePtr toIndependent(DataStructurePtr entity_structure);
    //! Создать из независимой от сущностей структуры, структуру на базе сущностей
    static DataStructurePtr toEntityBased(DataStructurePtr independent_structure, const DataProperty& entity);

    ~DataStructure();

    //! Версия структуры данных
    int structureVersion() const;

    bool isValid() const;
    //! Структура не связана с конкретной сущностью
    bool isEntityIndependent() const;

    //! Идентификатор структуры (для структур сущностей совпадает с entityCode)
    PropertyID id() const;

    //! Код сущности
    EntityCode entityCode() const;
    //! Сущность (для структур сущностей)
    DataProperty entity() const;

    //! Задать набор свойств
    void setProperties(const DataPropertyList& properties);
    //! Добавить свойство
    DataProperty addProperty(const DataProperty& property);
    //! Добавить свойство
    DataStructure& operator<<(const DataProperty& property);
    DataStructure& operator=(const DataPropertySet& properties);
    DataStructure& operator=(const DataPropertyList& properties);

    //! Конвертация контейнеров для удобства
    DataPropertySet propertiesToSet(const PropertyIDList& properties) const;
    //! Конвертация контейнеров для удобства
    DataPropertySet propertiesToSet(const PropertyIDSet& properties) const;
    //! Конвертация контейнеров для удобства
    DataPropertySet propertiesToSet(const PropertyID& property) const;

    //! Добавить сущность целиком
    DataProperty addEntityProperty(
        //! Код сущности
        const EntityCode& entity_code,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Добавить сущность целиком
    DataProperty addEntityProperty(
        //! Идентификатор сущности. Для сущностей, которые имеют разную структуру данных в рамках одного кода
        const Uid& entity_uid,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Добавить поле данных
    DataProperty addFieldProperty(
        //! Сущность
        const DataProperty& entity,
        //! Идентификатор поля
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Добавить набор данных
    DataProperty addDatasetProperty(
        //! Сущность
        const DataProperty& entity,
        //! Идентификатор набора данных
        const PropertyID& id,
        //! Тип данных (Table или Tree)
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Добавить поле данных, которое используется в отрыве от сущностей (например при внесении произвольных данных в DataContainer)
    DataProperty addIndependentFieldProperty(
        //! Идентификатор поля
        const PropertyID& id,
        //! Тип данных
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Значение по умолчанию
        const QVariant& default_value = QVariant(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Добавить набор данных, который используется в отрыве от сущностей (например при внесении произвольных данных в DataContainer)
    DataProperty addIndependentDatasetProperty(
        //! Идентификатор набора данных
        const PropertyID& id,
        //! Тип данных (Table или Tree)
        DataType data_type,
        //! id перевода
        const QString& translation_id = QString(),
        //! Параметры свойства
        const PropertyOptions& options = {});

    //! Содержит ли свойство
    bool contains(const PropertyID& property_id) const;
    //! Содержит ли свойство
    bool contains(const DataProperty& p) const;
    /*! Свойство по его идентификатору. Только для полей, наборов данных и их колонок */
    DataProperty property(const PropertyID& property_id,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true) const;
    //! Параметры для свойства по его идентификатору
    PropertyOptions propertyOptions(const PropertyID& property_id,
        //! Остановка если свойство не надено
        bool halt_if_not_found = true) const;

    //! Cвойство строки
    static DataProperty propertyRow(const DataProperty& dataset, const RowID& row_id);
    //! Cвойство колонки по ее логическому номеру (порядку)
    static DataProperty propertyColumn(const DataProperty& dataset,
        //! Логический индекс колонки в наборе данных
        int column,
        //! Остановка, если не найдено
        bool halt_if_not_found = true);

    //! Cвойство ячейки
    static DataProperty propertyCell(const DataProperty& dataset, const RowID& row_id, int column);
    //! Cвойство ячейки
    static DataProperty propertyCell(const RowID& row_id, const DataProperty& column_property);

    //! Порядковый номер колонки для набора данных
    int column(const DataProperty& column_property) const;
    //! Порядковый номер колонки для набора данных по id свойств
    int column(const PropertyID& column_property_id) const;

    //! Имеется ли всего один набор данных
    bool isSingleDataset() const;
    //! id набора данных, если он всего один
    PropertyID singleDatasetId() const;
    //! id набора данных, если он всего один
    DataProperty singleDataset() const;

    //! Список всех свойств, включая колонки наборов данных
    const DataPropertySet& properties() const;
    //! Список всех свойств, без колонок наборов данных
    const DataPropertySet& propertiesMain() const;

    //! Список всех свойств, включая колонки наборов данных. Отсортированы по id
    DataPropertyList propertiesSorted() const;
    //! Список всех свойств, без колонок наборов данных. Отсортированы по id
    DataPropertyList propertiesMainSorted() const;

    //! Свойства определенного типа (Только для полей, наборов данных и их колонок)
    const DataPropertyList& propertiesByType(PropertyType type) const;
    //! Найти свойства по параметрам
    DataPropertyList propertiesByOptions(
        //! Вид свойства
        PropertyType type,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options) const;
    //! Найти свойства по видам связи
    DataPropertyList propertiesByLinkType(
        //! Вид свойства
        PropertyType ptype,
        //! Вид связи
        PropertyLinkType ltype) const;

    //! Свойства по идентификатору LookupRequest
    DataPropertyList propertiesByLookupRequest(const Uid& uid) const;
    //! Свойства по коду сущности LookupRequest
    DataPropertyList propertiesByLookupRequest(const EntityCode& uid) const;

    //! Свойство, содержащее имя
    DataProperty nameProperty() const;

    //! Колонка, содержащая имя
    DataProperty nameColumn(const DataProperty& dataset) const;
    //! Колонка, содержащая имя
    DataProperty nameColumn(const PropertyID& dataset_property_id) const;

    //! Имеет ограничения на какие-то поля или колонки
    bool hasConstraints() const;

    //! Найти колонки для указанного набора данных
    DataPropertyList datasetColumnsByOptions(const DataProperty& dataset,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options) const;
    //! Найти колонки для указанного набора данных
    DataPropertyList datasetColumnsByOptions(const PropertyID& dataset_property_id,
        //! Параметры (поиск идет по логическому OR)
        const PropertyOptions& options) const;

    //! Максимальный номер id свойства
    PropertyID maxPropertyId() const;

    //! Преобразовать структуру в QVariant
    QVariant variant() const;
    //! Сформировать структуру из QVariant
    static DataStructure fromVariant(const QVariant& v);

    //! Информация о lookup
    void getLookupInfo(
        //! Поле/Колонка, которая имеет lookup
        const DataProperty& property,
        //! Из какого набора данных берутся значения
        DataProperty& dataset,
        //! Какая колонка набора данных dataset отвечает за отображение информации
        int& display_column,
        //! Какая колонка набора данных dataset отвечает за ID
        int& data_column) const;

    /*! Выгрузить в JSON. Это не полноценная сериализация. Выгружается только структура данных без constraint и т.д. */
    QJsonObject toJson() const;
    //! Восстановить из JSON
    static DataStructurePtr fromJson(const QJsonObject& source, Error& error);

    //! Указать набор свойств, которые будут восприниматься ядром как одинаковые и в них будут поддерживаться одинаковые значения
    void setSameProperties(const PropertyIDList& properties);
    void setSameProperties(const DataPropertyList& properties);

    //! Получить наборы "одинаковых" свойств
    QList<std::shared_ptr<PropertyIDSet>> getSameProperties(const PropertyID& p) const;
    QList<std::shared_ptr<PropertyIDSet>> getSameProperties(const DataProperty& p) const;

    //! Для группы одинаковых свойств найти "управляющий"
    DataProperty samePropertiesMaster(const DataProperty& property) const;
    DataProperty samePropertiesMaster(const PropertyID& p) const;

    //! Наборы свойств, которые будут восприниматься ядром как одинаковые и в них будут поддерживаться одинаковые значения
    const QList<PropertyIDList>& sameProperties() const;
    //! Все свойства из PropertyConstraint::showHiglightProperty
    const QSet<DataProperty>& constraintsShowHiglightProperties() const;

    //! Делает невалидными все lookup модели
    void invalidateLookupModels() const;

private:
    DataStructure(
        //! Уникальный идентификатор структуры
        const PropertyID& id,
        //! Сущность
        const DataProperty& entity,
        //! Версия структуры данных
        int structure_version,
        //! Набор свойств
        const DataPropertyList& properties);

    //! Извлечь дочерние свойства из списка и получить полный список
    static void expandProperties(const DataPropertyList& properties, DataPropertyList& expanded, PropertyID& max_id);

    //! Список свойств вместе с колонками наборов данных - ленивая инициализация
    const DataPropertySet& expanded() const;

    //! Идентификатор структуры
    PropertyID _id;
    //! Сущность
    DataProperty _entity;
    //! Версия структуры данных
    int _structure_version = 0;
    //! Свойства
    mutable QVector<DataPropertyPtr> _properties;
    //! Список свойств без колонок наборов данных
    DataPropertySet _properties_list_main;
    //! Список свойств вместе с колонками наборов данных
    mutable std::unique_ptr<DataPropertySet> _properties_list_expanded;
    //! Свойтства по типу
    mutable QMap<PropertyType, DataPropertyList> _properties_by_type;

    //! id набора данных, если он всего один
    PropertyID _single_dataset_id;

    //! Наборы свойств, которые будут восприниматься ядром как одинаковые и в них будут поддерживаться одинаковые значения
    //! Ключ - свойство. Значения - наборы одинаковых свойств, в которые оно входит
    QMultiHash<PropertyID, std::shared_ptr<PropertyIDSet>> _same_properties_hash;
    //! наборы "одинаковых" свойств
    QList<PropertyIDList> _same_properties;

    //! Для группы одинаковых свойств - "управляющий"
    mutable QMap<PropertyID,  DataProperty> _same_props_master_info;
    mutable QMutex _same_props_master_mutex;

    //! Все свойства из PropertyConstraint::showHiglightProperty
    mutable std::unique_ptr<QSet<DataProperty>> _constraints_show_higlight_properties;    

    friend QDataStream& ::operator<<(QDataStream& out, const zf::DataStructure& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::DataStructure& obj);
};

inline uint qHash(const DataProperty& p)
{
    return p.hashKey();
}

} // namespace zf

Q_DECLARE_METATYPE(zf::DataProperty)
Q_DECLARE_METATYPE(zf::DataPropertyPtr)
Q_DECLARE_METATYPE(zf::DataPropertySet)
Q_DECLARE_METATYPE(zf::DataPropertyList)

Q_DECLARE_METATYPE(zf::DataStructure)
Q_DECLARE_METATYPE(zf::DataStructurePtr)

Q_DECLARE_METATYPE(zf::PropertyConstraint)
Q_DECLARE_METATYPE(zf::PropertyLookup)

Q_DECLARE_METATYPE(zf::ModelServiceLookupRequestOptions);

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataProperty& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataStructure& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataStructure* c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::DataStructurePtr c);

#endif // ZF_data_structure_H
