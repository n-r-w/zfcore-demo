#pragma once

#include "zf_db_srv.h"
#include "zf_numeric.h"
#include "zf_error.h"
#include <QPointer>

class QAbstractItemModel;

namespace zf
{
class ItemModel;

//! Результат запроса к БД в виде таблицы. Базовый абстрактный класс
class ZCORESHARED_EXPORT DataTable
{
public:
    virtual ~DataTable();

    //! Информация о запросе
    QString info() const;

    //! Содержит данные
    virtual bool isValid() const = 0;
    //! Содержит ошибку
    virtual bool isError() const = 0;

    //! Количество строк
    virtual int rowCount() const = 0;
    //! Количество колонок
    virtual int columnCount() const = 0;

    //! Создать на основе таблицы объект ItemModel. Данные копируются. За удаление отвечает вызывающий
    ItemModel* createItemModel(QObject* parent = nullptr) const;
    //! Возвращает указатель на QAbstractItemModel, которая содержит данные DataTable
    //! Владельцем остается DataTable. Копирования данных не происходит
    QAbstractItemModel* itemModel() const;
    //! Порядковый номер для колонки в ItemModel
    virtual int columnPos(const FieldID& column) const = 0;

    //! Проверка ячейки на null
    virtual bool isNull(int row, const FieldID& column) const = 0;

    //! Bool в ячейке
    virtual bool getBool(int row, const FieldID& column) const = 0;
    //! Целое в ячейке
    virtual int getInt(int row, const FieldID& column) const = 0;
    //! Жирное целое в ячейке
    virtual qint64 getInt64(int row, const FieldID& column) const = 0;
    //! Вещественное в ячейке
    virtual double getDouble(int row, const FieldID& column) const = 0;
    //! Строка в ячейке
    virtual QString getString(int row, const FieldID& column) const = 0;
    //! Дата в ячейке
    virtual QDate getDate(int row, const FieldID& column) const = 0;
    //! Дата и время в ячейке
    virtual QDateTime getDateTime(int row, const FieldID& column) const = 0;
    //! Время в ячейке
    virtual QTime getTime(int row, const FieldID& column) const = 0;
    //! Numeric в ячейке
    virtual Numeric getNumeric(int row, const FieldID& column) const = 0;
    //! Money в ячейке
    virtual Money getMoney(int row, const FieldID& column) const = 0;
    //! Значение в ячейке как QVariant
    virtual QVariant getVariant(int row, const FieldID& column) const = 0;
    //! Тип данных в колонке
    virtual zf::DataType dataType(const FieldID& column) const = 0;
    //! Ключевая колонка
    int keyColumn() const;

    //! Получить диапазон значений ячеек (целые)
    QList<int> getIntList(int row, const FieldIdList& columns) const;
    //! Получить диапазон значений ячеек (вещественные)
    QList<double> getDoubleList(int row, const FieldIdList& columns) const;
    //! Получить диапазон значений ячеек (строки)
    QStringList getStringList(int row, const FieldIdList& columns) const;
    //! Получить диапазон значений ячеек (QVariant)
    QVariantList getVariantList(int row, const FieldIdList& columns) const;

    //! Получить список значений по всем строкам для указанной колонки
    QList<int> getFullColumnInt(const FieldID& column) const;
    //! Получить список значений по всем строкам для указанной колонки
    QStringList getFullColumnString(const FieldID& column) const;
    //! Получить список значений по всем строкам для указанной колонки
    QVariantList getFullColumnVariant(const FieldID& column) const;

    //! Поиск целого значения. Возвращает номер строки
    virtual int findInt(
        //! В какой колонке
        const FieldID& column,
        //! Ключ
        int key,
        //! С какой строки начинать поиск
        int row_from = 0) const = 0;
    //! Поиск целого значения. Возвращает все номера строк, где оно обнаружено
    QList<int> findIntRows(
        //! В какой колонке
        const FieldID& column,
        //! Ключ
        int key,
        //! С какой строки начинать поиск
        int row_from = 0) const;
    //! Поиск целого значения по двум колонкам. Возвращает номер строки
    virtual int findInt(const FieldID& column1, int key1, const FieldID& column2, int key2,
                        //! Первая колонка отсортирована
                        bool first_sorted = false) const = 0;
    //! Поиск целого по колонке при условии что она отсортирована
    virtual int findIntSorted(const FieldID& column, int key) const = 0;
    //! Поиск целого значения в диапазоне строк. Возвращает номер строки
    virtual int findIntInRowRange(int row_from, int count, const FieldID& column, int key) const = 0;
    //! Поиск целого значения. Возвращает диапазон, в для которого совпадают ключи: {номер первой строки, номер последней строки}
    virtual QPair<int, int> findLineRangeForInt(const FieldID& column, int key) const = 0;
    //! Поиск строки с максимальным значением в колонке
    virtual int findMax(const FieldID& column) const = 0;

    //! Поиск строкового значения. Возвращает номер строки
    virtual int findString(const FieldID& column, const QString& str, Qt::CaseSensitivity c = Qt::CaseSensitive) const = 0;

    //! Поиск по колонке с пустым ID. Возвращает номер строки
    virtual int findEmptyID(
        //! В какой колонке
        const FieldID& column,
        //! С какой строки начинать поиск
        int row_from = 0) const = 0;
    //! Поиск по колонке с пустым ID. Возвращает все номера строк, где оно обнаружено
    QList<int> findEmptyIDRows(
        //! В какой колонке
        const FieldID& column,
        //! С какой строки начинать поиск
        int row_from = 0) const;

    //! Поиск целого значения по колонке column. Возвращает набор значений в result_column
    QVariantList findIntValuesVariant(
        //! В какой колонке
        const FieldID& column,
        //! Ключ
        int key,
        //! Колонка из которой брать значения
        const FieldID& result_column) const;
    //! Поиск целого значения по колонке column. Возвращает набор значений в result_column
    QList<int> findIntValuesInt(
        //! В какой колонке
        const FieldID& column,
        //! Ключ
        int key,
        //! Колонка из которой брать значения
        const FieldID& result_column) const;
    //! Поиск целого значения по колонке column. Возвращает набор значений в result_column
    QStringList findIntValuesString(
        //! В какой колонке
        const FieldID& column,
        //! Ключ
        int key,
        //! Колонка из которой брать значения
        const FieldID& result_column) const;

    //! Сортировка
    virtual void sort(
        //! Колонки
        const QList<FieldID>& columns,
        //! По умолчанию для всех Qt::AscendingOrder
        const QList<Qt::SortOrder>& order = {},
        //! По умолчанию для всех Qt::CaseInsensitive
        const QList<Qt::CaseSensitivity>& case_sens = {})
        = 0;

protected:
    DataTable(int key_column, const QString& info);

    //! Вернуть уникальный код ячейки для формирования QModelIndex
    virtual quintptr getCellPointer(int row, const FieldID& column) const = 0;

    //! Преобразовать поле в порядковый номер колонки
    virtual int fieldToColumn(const FieldID& column) const = 0;
    //! Преобразовать порядковый номер колонки в поле
    virtual FieldID fieldFromColumn(int column) const = 0;

    virtual void checkValid() const = 0;
    //! Проверка ячейки
    void checkRow(int row) const;
    //! Проверка колонки
    virtual void checkColumn(const FieldID& column) const = 0;

private:
    int _key_column;
    QString _info;

    //! Модель с данными
    mutable QPointer<QAbstractItemModel> _item_model;
    mutable bool _item_model_initialized = false;

    friend class DataTableItemModel;
};
typedef std::shared_ptr<DataTable> DataTablePtr;

} // namespace zf

Q_DECLARE_METATYPE(zf::DataTablePtr)
