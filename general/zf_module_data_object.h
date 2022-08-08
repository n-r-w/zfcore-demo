#ifndef ZF_MODULEDATAOBJECT_H
#define ZF_MODULEDATAOBJECT_H

#include "zf_data_container.h"
#include "zf_error.h"
#include "zf_module_object.h"
#include "zf_proxy_item_model.h"
#include "zf_highlight_model.h"
#include "zf_i_highlight_view.h"
#include "zf_highlight_processor.h"
#include "zf_i_data_change_processor.h"
#include "zf_change_info.h"

#include <QValidator>

namespace zf
{
class DataChangeProcessor;

//! Идентификатор отслеживания изменений данных в модели
class ZCORESHARED_EXPORT TrackingID : public Handle<int>
{
public:
    TrackingID();
    TrackingID(int id);
    TrackingID(const Handle<int>& h);
};

//! Базовый класс для классов, которые входят в модуль и имеют свои данные
class ZCORESHARED_EXPORT ModuleDataObject : public ModuleObject,
                                            public I_HashedDatasetCutomize,
                                            public I_HighlightView,
                                            public I_HighlightProcessor,
                                            public I_HighlightProcessorKeyValues,
                                            public I_DataChangeProcessor
{
    Q_OBJECT

    Q_PROPERTY(zf::DataStructure* structure READ getDataStructure CONSTANT)
    Q_PROPERTY(zf::DataContainer* dataContainer READ getDataContainer CONSTANT)

public:
    ModuleDataObject(
        //! Уникальный код модуля
        const EntityCode& module_code,
        //! Структура данных модели
        const DataStructurePtr& data_structure,
        //! Объект, который отвечает за проверку ошибок и генерирует запросы автоматические на проверку. Может быть
        //! равно nullptr, тогда назначается этот объект
        HighlightProcessor* master_highlight,
        //! Параметры
        const ModuleDataOptions& options);
    ~ModuleDataObject();

    //! Информация о модуле
    ModuleInfo moduleInfo() const override;

    //! Структура данных
    DataStructurePtr structure() const;
    //! Данные
    DataContainerPtr data() const;

    //! Параметры ModuleDataObject
    virtual ModuleDataOptions moduleDataOptions() const;

    //! Скопировать данные из source. Копирование доступно только для ModuleDataObject одного типа
    void copyFrom(const ModuleDataObject* source);
    //! Находимся в состоянии копирования через copyFrom
    bool isCopyFrom() const;

    //! Активен ли данный объект (открыт диалог, MDI окно получило фокус, активна рабочая зона лаунчера)
    bool isActive() const;

    //! Найти строки набора данных по одному полю
    Rows findRows(const DataProperty& column, const QVariant& value, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю
    Rows findRows(const PropertyID& column, const QVariant& value, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю
    Rows findRows(const DataProperty& column, const Uid& uid, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю
    Rows findRows(const PropertyID& column, const Uid& uid, bool case_insensitive = false, int role = Qt::DisplayRole) const;

    //! Найти строки набора данных по одному полю. Результаты поиска обобщаются
    Rows findRows(const DataProperty& column, const QVariantList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    Rows findRows(const PropertyID& column, const QVariantList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю. Результаты поиска обобщаются
    Rows findRows(const DataProperty& column, const QStringList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    Rows findRows(const PropertyID& column, const QStringList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю. Результаты поиска обобщаются
    Rows findRows(const DataProperty& column, const QList<int>& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    Rows findRows(const PropertyID& column, const QList<int>& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;

    //! Все варианты методов хэшированного поиска
    DataHashed* hash() const;

    //! Вывести содержимое контейнера с данными в файл и показать его
    void debPrint() const;

public: // методы работы с данными. Для сокращения кода основые функции вынесены сюда из DataContainer
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
    Error setCell(int row, const DataProperty& column, const Uid& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const Uid& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const DataProperty& column, const Numeric& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Записать в набор данных (вызывает его инициализацию)
    Error setCell(int row, const PropertyID& column_property_id, const Numeric& value, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());

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
    QVariant cell(
        //! Номер строки
        int row, const DataProperty& column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Номер строки
        int row, const PropertyID& column_property_id, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex(),
        QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Идентификатор строки
        const RowID& row_id, const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Идентификатор строки
        const RowID& row_id, const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;

    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Свойство типа PropertyType::RowFull
        const DataProperty& row_property, const DataProperty& column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
        //! Конвертировать код в lookup расшифровку. Только для свойств с заданным lookup
        bool from_lookup = false) const;
    //! Значение набора данных (ошибка, если не набор данных не инициализирован)
    QVariant cell(
        //! Свойство типа PropertyType::RowFull
        const DataProperty& row_property, const PropertyID& column_property_id, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage,
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

    //! Индекс первой колонки для указанной строки
    QModelIndex rowIndex(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex rowIndex(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Создать свойство "строка"
    DataProperty rowProperty(const DataProperty& dataset, const RowID& row_id) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const PropertyID& dataset_id, const RowID& row_id) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Создать свойство "строка"
    DataProperty rowProperty(const PropertyID& dataset_id, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Создать свойство "ячейка". property может быть типа RowFull или Cell. Если свойство Cell, то из него возмется row
    DataProperty cellProperty(const DataProperty& property, const PropertyID& column_id) const;
    DataProperty cellProperty(const DataProperty& property, const DataProperty& column) const;
    //! Создать свойство "ячейка"
    DataProperty cellProperty(const RowID& row_id, const PropertyID& column_id) const;
    DataProperty cellProperty(const RowID& row_id, const DataProperty& column) const;
    //! Создать свойство "ячейка"
    DataProperty cellProperty(int row, const PropertyID& column, const QModelIndex& parent = QModelIndex()) const;
    //! Создать свойство "ячейка"
    DataProperty cellProperty(int row, const DataProperty& column, const QModelIndex& parent = QModelIndex()) const;

    //! Инициализировать значения всех свойств
    Error initAllValues();

    //! Инициализировать свойство (записать в него значение по умолчанию, если оно не инициализировано)
    Error initValue(const DataProperty& p);
    Error initValue(const PropertyID& property_id);

    //! Записать в свойство значение по умолчанию
    Error resetValue(const DataProperty& p);
    Error resetValue(const PropertyID& property_id);

    //! Сделать свойство не инициализированым
    void unInitialize(const DataProperty& p);
    //! Сделать свойство неинициализированным
    void unInitialize(const PropertyID& property_id);

    //! Инициализировать набор данных. Если набор данных уже инициализирован, то ничего не пооисходит
    void initDataset(const DataProperty& dataset_property, int row_count = 0);
    //! Инициализировать набор данных. Если набор данных уже инициализирован, то ничего не пооисходит
    void initDataset(const PropertyID& property_id, int row_count = 0);
    //! Инициализировать набор данных. Для содержащих всего один набор данных. Если набор данных уже инициализирован, то
    //! ничего не пооисходит
    void initDataset(int row_count);

    //! Получить указатель на набор данных.
    ItemModel* dataset(const DataProperty& dataset_property) const;
    ItemModel* dataset(const PropertyID& dataset_property_id) const;
    //! Получить указатель на набор данных. Для содержащих всего один набор данных
    ItemModel* dataset() const;

    //! Колонка набора данных со свойством PropertyOption::Id
    DataProperty datasetIdColumn(const DataProperty& dataset_property) const;
    //! Колонка набора данных со свойством PropertyOption::Id
    DataProperty datasetIdColumn(const PropertyID& dataset_property_id) const;

    //! Создать объект Rows из списка номеров строк
    Rows makeRows(const DataProperty& p, const QList<int>& rows, const QModelIndex& parent = QModelIndex()) const;
    //! Создать объект Rows из списка номеров строк
    Rows makeRows(const DataProperty& p, const QSet<int>& rows, const QModelIndex& parent = QModelIndex()) const;
    //! Создать объект Rows из списка номеров строк
    Rows makeRows(const PropertyID& property_id, const QList<int>& rows, const QModelIndex& parent = QModelIndex()) const;
    //! Создать объект Rows из списка номеров строк
    Rows makeRows(const PropertyID& property_id, const QSet<int>& rows, const QModelIndex& parent = QModelIndex()) const;
    //! Создать объект Rows из списка номеров строк. Для структур с одним набором данных
    Rows makeRows(const QList<int>& rows, const QModelIndex& parent = QModelIndex()) const;
    //! Создать объект Rows из списка номеров строк. Для структур с одним набором данных
    Rows makeRows(const QSet<int>& rows, const QModelIndex& parent = QModelIndex()) const;

    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<RowID>& rows = {}, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QModelIndexList& rows, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<int>& rows, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<RowID>& rows = {}, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QModelIndexList& rows, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<int>& rows, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const Rows& rows, int role = Qt::DisplayRole,
        //! Только содержащие значения
        bool valid_only = false) const;

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

public:
    /*! Язык данных по умолчанию (если равен QLocale::AnyLanguage, то при чтении будет использоваться Core::language, а
     * при записи QLocale::AnyLanguage) */
    QLocale::Language language() const;
    void setLanguage(QLocale::Language language);

    //! Пометить данные как удаленные. Возвращает истину если состояние invalidate поменялось хотя бы у одного свойства
    virtual bool invalidate(const ChangeInfo& info = {}) const;

    //! Получить lookup значение
    QVariant getLookupValue(const DataProperty& column_property, const QVariant& lookup_value) const;

    /*! Свойство по его идентификатору.
     * Доступны только поля (Field) и наборы данных (Dataset). Колонки, строки и ячейки не могут быть
     * получены через данный метод */
    DataProperty property(const PropertyID& property_id) const;
    //! Cвойство строки
    DataProperty propertyRow(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Cвойство колонки
    DataProperty propertyColumn(const DataProperty& dataset, int column) const;
    DataProperty propertyColumn(const DataProperty& dataset, const DataProperty& column_property) const;

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

    //! Содержит ли такое свойство
    bool containsProperty(const DataProperty& p) const;

    //! Свойство доступно (данные не помечены как удаленные (isInvalidate) и свойство инициализировано)
    bool propertyAvailable(const DataProperty& p) const;
    //! Свойство доступно (данные не помечены как удаленные (isInvalidate) и свойство инициализировано)
    bool propertyAvailable(const PropertyID& property_id) const;

    //! Порядковый номер колонки для набора данных
    int column(const DataProperty& column_property) const;
    //! Порядковый номер колонки для набора данных по id свойств
    int column(const PropertyID& column_property_id) const;

    /*! Уникальный ключ строки набора данных. Запрашивает значение из getDatasetRowID и если его нет, то формирует
     * уникальный идентификатор, который затем записывается в методе writeDatasetRowID. Запись производится прямо в этом методе,
     * с временным отключением генерации сигналов ItemModel.
     * RowID хранится в самой первой колонке в роли Consts::UNIQUE_ROW_COLUMN_ROLE.
     * В момент добавления новой строки, в ней нет данных. Следовательно даже при наличии колонки с признаком PropertyOption::Id,
     * невозможно сформировать уникальный идентификатор на основе реального ID. Поэтому при добавлении строк, генерируется уникальный идентификатор, 
     * не привязанный к данным через метод RowID::createFull */
    RowID datasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent = QModelIndex()) const;
    RowID datasetRowID(const PropertyID& dataset_property_id, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Уникальный ключ строки набора данных. Упрощенный метод для моделей с одним набором данных
    RowID datasetRowID(int row, const QModelIndex& parent = QModelIndex()) const;

    //! Найти строку по идентификатору
    QModelIndex findDatasetRowID(const DataProperty& dataset_property, const RowID& row_id) const;
    //! Найти строку по идентификатору
    QModelIndex findDatasetRowID(const PropertyID& dataset_property_id, const RowID& row_id) const;
    //! Найти строку по идентификатору. Упрощенный метод для моделей с одним набором данных
    QModelIndex findDatasetRowID(const RowID& row_id) const;

    //! Все строки (свойства)
    DataPropertyList getAllRowsProperties(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    DataPropertyList getAllRowsProperties(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Все строки (индексы)
    QModelIndexList getAllRowsIndexes(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    QModelIndexList getAllRowsIndexes(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Все строки (zf::Rows)
    Rows getAllRows(const DataProperty& dataset_property, const QModelIndex& parent = QModelIndex()) const;
    Rows getAllRows(const PropertyID& dataset_property_id, const QModelIndex& parent = QModelIndex()) const;

    //! Принудительно задать rowID для всех строк набора данных.
    //! По умолчанию rowID формируется при первом запросе. Этот метод принудительно формирует rowID для всех строк
    void forceCalculateRowID(const DataProperty& dataset_property) const;

    //! Генерация RowId для строки, если еще не сгенерировано
    void generateRowId(const DataProperty& dataset, int row, const QModelIndex& parent);

public:
    /*! Начать отслеживание изменения данных. Возвращает уникальный ID отслеживания. 
     * Таким образом можно отслеживать изменения не беспокоясь о нарушении чужой логики. 
     * Отслеживание изменений всегда сбрасывается перед обновлением данных из БД */
    TrackingID startTracking(const DataProperty& dataset_property);
    TrackingID startTracking(const PropertyID& dataset_property_id);
    //! Закончить отслеживание изменения данных
    void stopTracking(const TrackingID& id);
    //! Имеется ли отслеживание изменения данных
    bool isTracking(const TrackingID& id) const;
    //! Активные идентификаторы отслеживания изменения данных
    QList<TrackingID> trackingIDs() const;
    //! Наборы данных по которым идет отслеживание изменений
    DataPropertyList trackingDatasets() const;

    //! Удаленные строки (свойства PropertyType::RowFull). Идентификатор строки в DataProperty::rowId()
    DataPropertyList trackingRemovedRows(const TrackingID& id) const;
    //! Новые строки (свойства PropertyType::RowFull). Идентификатор строки в DataProperty::rowId()
    DataPropertyList trackingNewRows(const TrackingID& id) const;
    //! Измененные ячейки (свойства PropertyType::Cell). Идентификатор строки в DataProperty::rowId(),
    //! идентификатор колонки в DataProperty::column()
    DataPropertyList trackingModifiedCells(const TrackingID& id) const;

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    //! Вывод состояния отслеживания изменений
    void debugTracking() const;
#endif

public: // Управление ошибками
    //! Процессор управления ошибками
    HighlightProcessor* highlightProcessor() const;
    //! Головной процессор управления ошибками
    HighlightProcessor* masterHighlightProcessor() const;

    //! Состояние ошибок
    const HighlightModel* highlight(bool execute_check = true) const;
    //! Очистить все ошибки
    void clearHighlights();

    //! Блокировка выполнения автоматических проверок их плагина
    void blockHighlightAuto();
    //! Разблокировка выполнения автоматических проверок их плагина
    void unBlockHighlightAuto();

    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const DataProperty& property) const;
    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const PropertyID& property_id) const;
    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const DataPropertyList& properties) const;
    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const PropertyIDList& property_ids) const;
    //! Запросить проверку на ошибки для данной ячейки набора данных
    void registerHighlightCheck(int row, const DataProperty& column, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для данной ячейки набора данных
    void registerHighlightCheck(int row, const PropertyID& column, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для строки набора данных
    void registerHighlightCheck(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для строки набора данных
    void registerHighlightCheck(const PropertyID& dataset, int row, const QModelIndex& parent = QModelIndex()) const;

    //! Запросить проверку на ошибки для всех свойств
    void registerHighlightCheck() const;

    //! Принудительно запустить зарегистрирированные проверки на ошибки
    void executeHighlightCheckRequests() const;
    //! Очистить зарегистрирированные проверки на ошибки
    void clearHighlightCheckRequests() const;

    //! Требуется ли проверка на ошибки
    virtual void setHightlightCheckRequired(bool b);

    //! Ошибки для ячейки, отсортированные от наивысшего приоритета к наименьшему
    QList<HighlightItem> cellHighlight(const QModelIndex& index, bool execute_check = true, bool halt_if_not_found = true) const;

protected:
    //! Скопировать данные из source. Копирование доступно только для ModuleDataObject одного типа
    virtual void doCopyFrom(const ModuleDataObject* source);
    //! Вызывается до doCopyFrom
    virtual void beforeCopyFrom(const ModuleDataObject* source);
    //! Вызывается после doCopyFrom
    virtual void afterCopyFrom(const ModuleDataObject* source);

    //! Делает невалидными все используемые модели (лукапы и встроенные представления для view)
    virtual void invalidateUsingModels() const;

    //! Проверить свойство на ошибки на основании PropertyConstraint и занести результаты проверки в HighlightInfo
    void getHighlightAuto(const DataProperty& highlight_property, HighlightInfo& info) const;

    // *** реализация I_HighlightProcessor
protected:
    //! Проверить свойство на ошибки и занести результаты проверки в HighlightInfo
    //! Имеет смысл переопределять только если объект отвечает за обработку ошибок (к примеру представление по умолчанию
    //! их не проверяет (если у него структура данных совпадает с моделью), а делегирует это модели)
    void getHighlight(const DataProperty& p, HighlightInfo& info) const override;
    // упрощенные методы работы с Highlight. Вызываются только при активном свойстве zf::ModuleDataOption::SimpleHighlight
    //! Проверить поле на ошибки
    void getFieldHighlight(const DataProperty& field, HighlightInfo& info) const override;
    //! Проверить набор данных на ошибки
    void getDatasetHighlight(const DataProperty& dataset, HighlightInfo& info) const override;
    //! Проверить ячейку набора данных на ошибки
    void getCellHighlight(const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent, HighlightInfo& info) const override;

    // *** реализация I_HighlightProcessorKeyValues
protected:
    //! Проверка ключевых значений на уникальность. Использовать для реализации нестандартных алгоритмов проверки
    //! ключевых полей По умолчанию используется алгоритм работы по ключевым полям, заданным через свойство
    //! PropertyOption::Key для колонок наборов данных
    void checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent,
        //! Если ошибка есть, то текст не пустой
        QString& error_text,
        //! Ячейка где должна быть ошибка если она есть
        DataProperty& error_property) const override;
    //! Преобразовать набор значений ключевых полей в уникальную строку.
    //! Если возвращена пустая строка, то проверка ключевых полей для данной записи набора данных игнорируется
    QString keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent, const QVariantList& key_values) const override;

protected:
    //! Содержит элемент с ошибкой
    bool containsHighlight(const DataProperty& property) const;
    bool containsHighlight(const PropertyID& property_id) const;
    //! Содержит элемент с ошибкой
    bool containsHighlight(const DataProperty& property, int id) const;
    bool containsHighlight(const PropertyID& property_id, int id) const;
    //! Содержит ошибки указанного типа для данного свойства
    bool containsHighlight(const DataProperty& property, InformationType type) const;
    bool containsHighlight(const PropertyID& property_id, InformationType type) const;

    //! Задать головной процессор управления ошибками
    void installMasterHighlightProcessor(HighlightProcessor* master_processor);

protected: // I_HighlightView
    //! Определение порядка сортировки свойств при отображении HighlightView
    //! Вернуть истину если property1 < property2
    bool highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const override;
    //! Инициализирован ли порядок отображения ошибок
    bool isHighlightViewIsSortOrderInitialized() const override;
    //! А надо ли вообще проверять на ошибки
    bool isHighlightViewIsCheckRequired() const override;
    //! Сигнал об изменении порядка отображения
    Q_SIGNAL void sg_highlightViewSortOrderChanged() override;

protected:
    //! Сформировать уникальный ключ строки набора данных. По умолчанию получает значение из роли
    //! Consts::UNIQUE_ROW_COLUMN_ROLE нулевой колонки
    virtual RowID readDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent) const;
    /*! Установить значение уникального ключа строки набора данных. Вызывается, если метод readDatasetRowID вернул
     * пустое значение и ядро сгенерировало новое. По умолчанию записывает значение в роль
     * Consts::UNIQUE_ROW_COLUMN_ROLE нулевой колонки. ВАЖНО: при вызове этого метода ядро блокирует сигналы QAbstractItemModel */
    virtual void writeDatasetRowID(const DataProperty& dataset_property, int row, const QModelIndex& parent, const RowID& row_id) const;

    // *** реализация I_DataChangeProcessor
protected:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void onDataInvalidate(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;
    //! Изменилось состояние валидности данных
    void onDataInvalidateChanged(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;

    //! Сменился текущий язык данных контейнера
    void onLanguageChanged(QLocale::Language language) override;

    /*! Вызывается при любых изменениях свойства, которые могут потребовать обновления связанных данных:
     * Когда свойство инициализируется, изменяется, разблокируется
     * Этот метод нужен, если мы хотим отслеживать изменения какого-то свойства и реагировать на это в тот момент, когда
     * в нем есть данные и их можно считывать */
    void onPropertyUpdated(
        /*! Может быть:
         * PropertyType::Field - при инициализации, изменении или разблокировке поля
         * PropertyType::Dataset - при инициализации, разблокировке набора данных, 
         *                         перемещении строк, колонок, ячеек
         * PropertyType::RowFull - перед удалением или после добавлении строки
         * PropertyType::ColumnFull - перед удалении или после добавлении колонок
         * PropertyType::Cell - при изменении данных в ячейке */
        const DataProperty& p,
        //! Вид действия. Для полей только изменения, для таблиц изменения, удаление и добавление строк
        ObjectActionType action) override;

    //! Все свойства были заблокированы
    void onAllPropertiesBlocked() override;
    //! Все свойства были разблокированы
    void onAllPropertiesUnBlocked() override;

    //! Свойство были заблокировано
    void onPropertyBlocked(const DataProperty& p) override;
    //! Свойство были разаблокировано
    void onPropertyUnBlocked(const DataProperty& p) override;

    //! Изменилось значение свойства. Не вызывается если свойство заблокировано
    void onPropertyChanged(const DataProperty& p,
        //! Старые значение (работает только для полей)
        const LanguageMap& old_values) override;

    //! Изменились ячейки в наборе данных. Вызывается только если изменения относятся к колонкамЮ входящим в структуру данных
    void onDatasetCellChanged(
        //! Начальная колонка
        const DataProperty left_column,
        //! Конечная колонка
        const DataProperty& right_column,
        //! Начальная строка
        int top_row,
        //! Конечная строка
        int bottom_row,
        //! Родительский индекс
        const QModelIndex& parent) override;

    //! Изменились ячейки в наборе данных
    void onDatasetDataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) override;
    //! Изменились заголовки в наборе данных (лучше не использовать, т.к. представление не использует данные табличных заголовков Qt)
    void onDatasetHeaderDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last) override;
    //! Перед добавлением строк в набор данных
    void onDatasetRowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После добавления строк в набор данных
    void onDatasetRowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед удалением строк из набора данных
    void onDatasetRowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После удаления строк из набора данных
    void onDatasetRowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед добавлением колонок в набор данных
    void onDatasetColumnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После добавления колонок в набор данных
    void onDatasetColumnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед удалением колонок в набор данных
    void onDatasetColumnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После удаления колонок из набора данных
    void onDatasetColumnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Модель данных будет сброшена (специфический случай - не использовать)
    void onDatasetModelAboutToBeReset(const DataProperty& p) override;
    //! Модель данных была сброшена (специфический случай - не использовать)
    void onDatasetModelReset(const DataProperty& p) override;
    //! Перед перемещением строк в наборе данных
    void onDatasetRowsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex& destinationParent, int destinationRow) override;
    //! После перемещения строк в наборе данных
    void onDatasetRowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row) override;
    //! Перед перемещением колонок в наборе данных
    void onDatasetColumnsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex& destinationParent, int destinationColumn) override;
    //! После перемещения колонок в наборе данных
    void onDatasetColumnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column) override;

protected:
    /*! Вызывается ядром или лаунчером в момент, когда модель или представление активируется
     * (открывается диалог, MDI окно получает фокус, активируется рабочая зона лаунчера и т.п.) или наоброт становится неактивным */
    virtual void onActivated(bool active);

    //! Обработчик менеджера обратных вызовов
    void processCallbackInternal(int key, const QVariant& data) override;

public:
    /*! Метод вызывается ядром или лаунчером в момент, когда модель или представление активируется
     * (открывается диалог, MDI окно получает фокус, активируется рабочая зона лаунчера и т.п.) или наоброт становится неактивным
     * Не вызывать */
    virtual void onActivatedInternal(bool active);

signals:
    //! Изменилось состояние валидности данных
    void sg_invalidateChanged(const zf::DataProperty& p, bool invalidate);
    //! Сменился текущий язык данных
    void sg_languageChanged(QLocale::Language language);

private slots:
    //! Изменилось состояние валидности данных
    void sl_invalidateChanged(const zf::DataProperty& p, bool invalidate);
    //! Сигналы наборов данных
    void sl_dataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void sl_dataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

protected:
    //! Объект, который отвечает за проверку ошибок и генерирует запросы автоматические на проверку
    //! Может быть как этим объектом, так и другим (в зависимости от вызова конструктора)
    //    ModuleDataObject* highlightObjectInternal();

private:
    //! Реализация интерфейса I_HashedDatasetCutomize
    //! Преобразовать набор значений ключевых полей в уникальную строку
    QString hashedDatasetkeyValuesToUniqueString(
        //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
        const QString& key,
        //! Строка набора данных
        int row,
        //! Ролитель строки
        const QModelIndex& parent,
        //! Ключевые значения
        const QVariantList& keyValues) const override;

    struct DatasetInfo;

    //! Структура данных модели
    DataStructure* getDataStructure() const;
    //! Данные модели
    DataContainer* getDataContainer() const;

    //! Регистрация изменений данных - удаление строки
    void registerTrackingChanges_rowRemoving(const DataProperty& dataset_property, int first, int last, const QModelIndex& parent);
    //! Регистрация изменений данных - добавление строки
    void registerTrackingChanges_rowAdded(const DataProperty& dataset_property, int first, int last, const QModelIndex& parent);
    //! Регистрация изменений данных - изменения в ячейках
    void registerTrackingChanges_cellChanged(int row, const DataProperty& column_property, const QModelIndex& parent);

    //! Данные модели
    DataContainerPtr _data;
    //! Отслеживание измнений данных в DataContainer
    DataChangeProcessor* _data_processor = nullptr;

    //! Информация об отслеживании изменения данных
    struct TrackingInfo
    {
        //! Удаленные строки (свойства PropertyType::RowFull)
        DataPropertySet removed_rows;
        //! Новые строки (свойства PropertyType::RowFull)
        DataPropertySet new_rows;
        //! Измененные ячейки (свойства PropertyType::Cell)
        DataPropertySet modified_cells;
        //! Информация о наборе данных
        DatasetInfo* dataset_info = nullptr;
    };
    typedef std::shared_ptr<TrackingInfo> TrackingInfoPtr;

    //! Все отслеживания изменений
    QMap<TrackingID, TrackingInfoPtr> _tracking_info;
    //! К какому набору данных относится отслеживание изменений
    QMultiMap<DataProperty, TrackingInfoPtr> _tracking_info_by_dataset;

    //! Информация о наборе данных
    struct DatasetInfo
    {
        DatasetInfo(ModuleDataObject* d_object, const DataProperty& ds);

        //! Указатель на данные для оптимизации скорости доступа
        ItemModel* item_model() const;

        ModuleDataObject* data_object = nullptr;
        //! Набор данных
        DataProperty dataset;
        //! Колонка с признаком PropertyOption::Id
        DataProperty id_column_property;
        int id_column = -1;

        //! Информация об отслеживании изменения данных
        QMap<TrackingID, TrackingInfoPtr> tracking;

    private:
        //! Указатель на данные для оптимизации скорости доступа
        mutable QPointer<ItemModel> _item_model;
    };
    mutable QMap<DataProperty, std::shared_ptr<DatasetInfo>> _dataset_info;
    DatasetInfo* datasetInfo(const DataProperty& dataset, bool null_if_not_exists) const;

    //! Активен ли данный объект (открыт диалог, MDI окно получило фокус, активна рабочая зона лаунчера)
    bool _is_active = false;

    //! Параметры ModuleDataObject
    ModuleDataOptions _options;

    //! Управление ошибками
    HighlightProcessor* _highlight_processor = nullptr;
    //! Главный процессор управления ошибками
    QPointer<HighlightProcessor> _master_highlight_processor;

    //! в пороцессе doCopyFrom
    bool _is_copy_from = false;
};

} // namespace zf

#endif // MODULEDATAOBJECT_H
