#pragma once

#include "zf_leaf_filter_proxy_item_model.h"
#include "zf_data_filter.h"

namespace zf
{
//! Прокси модель, для организации головных и подчиненных наборов данных
class ZCORESHARED_EXPORT ProxyItemModel : public LeafFilterProxyModel
{
    Q_OBJECT
public:
    ProxyItemModel(
        //! Где находятся данные
        DataFilter* filter,
        //! Набор данных, на который назначен ProxyItemModel
        const DataProperty& dataset_property,
        //! Родитель
        QObject* parent = nullptr);
    ~ProxyItemModel() override;

    //! Набор данных, на который назначен ProxyItemModel
    DataProperty datasetProperty() const;

    //! Перегрузить данные с учетом нового фильтра
    void invalidateFilter();

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

    using LeafFilterProxyModel::data;

    //! Упрощенный метод доступа к данным
    QVariant data(int row, int column, const QModelIndex& parent = QModelIndex(), int role = Qt::DisplayRole) const;

signals:
    //! Выполнена вставка строк. В отличие от сигнала QAbstractItemModel::rowsInserted вызывается после обработки
    //! всей внутренней кухни Qt по вставке строк и не приводит к глюкам при работе с прокси
    void sg_rowsInserted(const QModelIndex& parent, int first, int last);
    //! Выполнена вставка колонок. В отличие от сигнала QAbstractItemModel::columnsInserted вызывается после обработки
    //! всей внутренней кухни Qt по вставке колонок и не приводит к глюкам при работе с прокси
    void sg_columnsInserted(const QModelIndex& parent, int first, int last);

protected:
    //! Видимость строки бех учета дочерних
    bool filterAcceptsRowItself(int source_row, const QModelIndex& source_parent,
        //! Безусловно исключить строку из иерархии
        bool& exclude_hierarchy) const override;
    //! Уникальный ключ строки
    RowID getRowID(int source_row, const QModelIndex& source_parent) const override;
    //! Сортировка
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    //! Установить базовую модель
    void setSourceModel(QAbstractItemModel* m) override;

    //! Где находятся данные
    QPointer<DataFilter> _filter;    
    //! Набор данных, на который назначен ProxyItemModel
    DataProperty _dataset_property;
};

} // namespace zf
