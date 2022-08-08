#pragma once

#include "zf.h"

namespace zf
{
class DataProperty;
class HighlightInfo;

//! Внешняя обработка ошибок из HighlightProcessor
class I_HighlightProcessor
{
public:
    //! Проверить поле на ошибки
    virtual void getFieldHighlight(const DataProperty& field, HighlightInfo& info) const = 0;
    //! Проверить набор данных на ошибки
    virtual void getDatasetHighlight(const DataProperty& dataset, HighlightInfo& info) const = 0;
    //! Проверить ячейку набора данных на ошибки
    virtual void getCellHighlight(const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent,
                                  HighlightInfo& info) const = 0;
    //! Проверить свойство на ошибки и занести результаты проверки в HighlightInfo
    virtual void getHighlight(const DataProperty& p, HighlightInfo& info) const = 0;
};

//! Внешняя обработка ошибок из HighlightProcessor. Обработка ключевых значений
class I_HighlightProcessorKeyValues
{
public:
    //! Преобразовать набор значений ключевых полей в уникальную строку.
    //! Если возвращена пустая строка, то проверка ключевых полей для данной записи набора данных игнорируется
    virtual QString keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent,
                                            const QVariantList& key_values) const = 0;
    //! Проверка ключевых значений на уникальность. Использовать для реализации нестандартных алгоритмов проверки
    //! ключевых полей По умолчанию используется алгоритм работы по ключевым полям, заданным через свойство
    //! PropertyOption::Key для колонок наборов данных
    virtual void checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent,
                                //! Если ошибка есть, то текст не пустой
                                QString& error_text,
                                //! Ячейка где должна быть ошибка если она есть
                                DataProperty& error_property) const = 0;
};

} // namespace zf
