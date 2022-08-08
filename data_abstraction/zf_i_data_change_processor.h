#pragma once

#include "zf.h"
#include <QModelIndex>

namespace zf
{
class DataProperty;
class ChangeInfo;

class I_DataChangeProcessor
{
public:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    virtual void onDataInvalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info) = 0;
    //! Изменилось состояние валидности данных
    virtual void onDataInvalidateChanged(const DataProperty& p, bool invalidate, const ChangeInfo& info) = 0;

    //! Сменился текущий язык данных контейнера
    virtual void onLanguageChanged(QLocale::Language language) = 0;

    /*! Вызывается при любых изменениях свойства, которые могут потребовать обновления связанных данных:
     * Когда свойство инициализируется, изменяется, разблокируется
     * Этот метод нужен, если мы хотим отслеживать изменения какого-то свойства и реагировать на это в тот момент, когда
     * в нем есть данные и их можно считывать */
    virtual void onPropertyUpdated(
        /*! Может быть:
         * PropertyType::Field - при инициализации, изменении или разблокировке поля
         * PropertyType::Dataset - при инициализации, разблокировке набора данных, 
         *                         перемещении строк, колонок, ячеек
         * PropertyType::RowFull - перед удалением или после добавлении строки
         * PropertyType::ColumnFull - перед удалении или после добавлении колонок
         * PropertyType::Cell - при изменении данных в ячейке */
        const DataProperty& p,
        //! Вид действия. Для полей только изменения, для таблиц изменения, удаление и добавление строк
        ObjectActionType action)
        = 0;

    //! Все свойства были заблокированы
    virtual void onAllPropertiesBlocked() = 0;
    //! Все свойства были разблокированы
    virtual void onAllPropertiesUnBlocked() = 0;

    //! Свойство были заблокировано
    virtual void onPropertyBlocked(const DataProperty& p) = 0;
    //! Свойство были разаблокировано
    virtual void onPropertyUnBlocked(const DataProperty& p) = 0;

    //! Изменилось значение свойства. Не вызывается если свойство заблокировано
    virtual void onPropertyChanged(const DataProperty& p,
                                   //! Старые значение (работает только для полей)
                                   const LanguageMap& old_values)
        = 0;
    //! Изменились ячейки в наборе данных. Вызывается только если изменения относятся к колонкамЮ входящим в структуру данных
    virtual void onDatasetCellChanged(
        //! Начальная колонка
        const DataProperty left_column,
        //! Конечная колонка
        const DataProperty& right_column,
        //! Начальная строка
        int top_row,
        //! Конечная строка
        int bottom_row,
        //! Родительский индекс
        const QModelIndex& parent)
        = 0;

    //! Изменились ячейки в наборе данных
    virtual void onDatasetDataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                      const QVector<int>& roles)
        = 0;
    //! Изменились заголовки в наборе данных (лучше не использовать, т.к. представление не использует данные табличных заголовков Qt)
    virtual void onDatasetHeaderDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last) = 0;
    //! Перед добавлением строк в набор данных
    virtual void onDatasetRowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! После добавления строк в набор данных
    virtual void onDatasetRowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! Перед удалением строк из набора данных
    virtual void onDatasetRowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! После удаления строк из набора данных
    virtual void onDatasetRowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! Перед добавлением колонок в набор данных
    virtual void onDatasetColumnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! После добавления колонок в набор данных
    virtual void onDatasetColumnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! Перед удалением колонок в набор данных
    virtual void onDatasetColumnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! После удаления колонок из набора данных
    virtual void onDatasetColumnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) = 0;
    //! Модель данных будет сброшена (специфический случай - не использовать)
    virtual void onDatasetModelAboutToBeReset(const DataProperty& p) = 0;
    //! Модель данных была сброшена (специфический случай - не использовать)
    virtual void onDatasetModelReset(const DataProperty& p) = 0;
    //! Перед перемещением строк в наборе данных
    virtual void onDatasetRowsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                             const QModelIndex& destinationParent, int destinationRow)
        = 0;
    //! После перемещения строк в наборе данных
    virtual void onDatasetRowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                                    int row)
        = 0;
    //! Перед перемещением колонок в наборе данных
    virtual void onDatasetColumnsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                                const QModelIndex& destinationParent, int destinationColumn)
        = 0;
    //! После перемещения колонок в наборе данных
    virtual void onDatasetColumnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                                       int column)
        = 0;
};
} // namespace zf
