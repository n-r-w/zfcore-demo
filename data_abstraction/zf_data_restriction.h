#pragma once

#include "zf_db_srv.h"

namespace zf
{
class DataRestriction;
typedef std::shared_ptr<DataRestriction> DataRestrictionPtr;

//! Ограничения поиска при выборке из серверных таблиц. Аналог серверного класса TblRestr. Базовый абстрактный класс
//! Объекты ограничения создаются с помощью метода zf::Core::createDataRestriction, т.к. за их генерацию отвечает драйвер
class ZCORESHARED_EXPORT DataRestriction
{
public:
    enum SearchType
    {
        SubString,
        ExactString,
        BeginString,
        EndString,
        WholeWord,
        BeginWord,
        EndWord,
    };

    virtual ~DataRestriction();

    //! Создание копии
    virtual DataRestrictionPtr clone() const = 0;

    //! Поиск по строке
    virtual void addString(
        //! Код поля
        const FieldID& field_id,
        //! Строка
        const QStringList& str,
        //! Тип поиска
        SearchType type = ExactString, Qt::CaseSensitivity c = Qt::CaseSensitive,
        //! Проверка NOT
        bool invert = false)
        = 0;
    //! Поиск по строке
    virtual void addString(
        //! Код поля
        const FieldID& field_id,
        //! Строка
        const QString& str,
        //! Тип поиска
        SearchType type = ExactString, Qt::CaseSensitivity c = Qt::CaseSensitive,
        //! Проверка NOT
        bool invert = false)
        = 0;
    //! Поиск по целому
    virtual void addInt(
        //! Код поля
        const FieldID& field_id,
        //! Целые
        const QList<int>& keys,
        //! Проверка NOT
        bool invert = false)
        = 0;
    //! Поиск по целому
    virtual void addInt(
        //! Код поля
        const FieldID& field_id,
        //! Целое
        int key,
        //! Проверка NOT
        bool invert = false)
        = 0;
    //! Поиск пустых значений
    virtual void addNull(
        //! Код поля
        const FieldID& field_id)
        = 0;
    //! Поиск по маске
    virtual void addMask(
        //! Код поля
        const FieldID& field_id,
        //! маска
        int mask,
        //! Любой бит или все биты
        bool anybit,
        //! Проверка NOT
        bool invert = false)
        = 0;
    //! Поиск по интервалу целых
    virtual void addIntInterval(
        //! Код поля
        const FieldID& field_id,
        //! От
        int from,
        //! До
        int to)
        = 0;
    //! Поиск по интервалу дат со временем
    virtual void addDateTimeInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если null, то игнорируется
        const QDateTime& from,
        //! До. Если null, то игнорируется
        const QDateTime& to)
        = 0;
    //! Поиск по интервалу дат. Время игнорируется
    virtual void addDateInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если null, то игнорируется
        const QDate& from,
        //! До. Если null, то игнорируется
        const QDate& to)
        = 0;
    //! Поиск по временному интервалу годов
    virtual void addYearInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если 0, то игнорируется
        int from,
        //! До. Если 0, то игнорируется
        int to)
        = 0;
    //! Поиск по временному интервалу. День и время игнорируются
    virtual void addYearMonthInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если null, то игнорируется
        const QDate& from,
        //! До. Если null, то игнорируется
        const QDate& to)
        = 0;
    //! Поиск по временному интервалу дней. Все остальное игнорируется
    virtual void addDayInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если 0, то игнорируется
        int from,
        //! До. Если 0, то игнорируется
        int to)
        = 0;
    //! Поиск по временному интервалу. Учитывается только час
    virtual void addHourInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если 0, то игнорируется
        int from,
        //! До. Если 0, то игнорируется
        int to)
        = 0;
    //! Поиск по временному интервалу. Даты игнорируются, учитывается только время
    virtual void addTimeInterval(
        //! Код поля
        const FieldID& field_id,
        //! От. Если null, то игнорируется
        const QTime& from,
        //! До. Если null, то игнорируется
        const QTime& to)
        = 0;
    //! Поиск по попаданию текущей даты+время в интервал между указанными полями
    virtual void addActualDateTime(
        //! Код поля
        const FieldID& field_id_from,
        //! Код поля
        const FieldID& field_id_to)
        = 0;
};

//! Интерфейс для создания ограничений
class I_DataRestriction
{
public:
    virtual ~I_DataRestriction() { }
    virtual DataRestriction* createDataRestriction(const TableID& table_id) = 0;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::DataRestrictionPtr)

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(zf::I_DataRestriction, "ru.ancor.metastaff.I_DataRestriction/1.0")
QT_END_NAMESPACE
