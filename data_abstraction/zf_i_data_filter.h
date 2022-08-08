#pragma once

class QModelIndex;

namespace zf
{
class DataProperty;
class ModuleDataObject;

//! Интерфейс фильтрации и сортировки
class I_DataFilter
{
public:
    //! Отфильтровать строку набора данных. По умолчанию - не фильтровать
    virtual bool externalFilterAcceptsRow(
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
        bool& exclude_hierarchy)
        = 0;
    /*! Сортировка. Результат:
     * -1 = left < right
     * +1 = left > right
     * 0 = использовать стандартную сортировку (по умолчанию) */
    virtual int externalLessThan(
        //! Источник данных
        ModuleDataObject* source,
        //! Фильтр, который мы переопределили. Может быть null
        I_DataFilter* original_filter,
        //! Набор данных
        const DataProperty& dataset_property, const QModelIndex& source_left, const QModelIndex& source_right)
        = 0;
};
} // namespace zf
