#ifndef ITEMMODEL_H
#define ITEMMODEL_H

#include "zf.h"
#include "zf_flat_item_model.h"
#include "zf_defs.h"

namespace zf
{
/*! Набор данных для использования в рамках данного фреймворка */
class ZCORESHARED_EXPORT ItemModel : public FlatItemModel
{
    Q_OBJECT
public:
    explicit ItemModel(QObject* parent = nullptr);
    ItemModel(int rows, int columns, QObject* parent = nullptr);

    //! Получить указатель на ItemModel, содержащуюся в QVariant, созданный через FlatItemModel::toVariant
    static std::shared_ptr<ItemModel> fromVariant(const QVariant& v);

    //! Поиск
    QVariantList find(
        //! Колонка по которой идет поиск
        const DataProperty& search_column,
        //! Колонка, результ из которой возвращается методом
        const DataProperty& target_column,
        //! Значение поиска по search_column
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole,
        //! Роль поиска
        int target_role = Qt::DisplayRole) const;
    //! Поиск
    QVariantList find(
        //! Код сущности
        const EntityCode& entity_code,
        //! Колонка по которой идет поиск
        const PropertyID& search_column,
        //! Колонка, результ из которой возвращается методом
        const PropertyID& target_column,
        //! Значение поиска по search_column
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole,
        //! Роль поиска
        int target_role = Qt::DisplayRole) const;
    //! Поиск
    QVariantList find(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Колонка по которой идет поиск
        const PropertyID& search_column,
        //! Колонка, результ из которой возвращается методом
        const PropertyID& target_column,
        //! Значение поиска по search_column
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole,
        //! Роль поиска
        int target_role = Qt::DisplayRole) const;

    //! Поиск
    QModelIndexList find(
        //! Индекс строки для старта поиска
        const QModelIndex& start,
        //! Колонка по которой идет поиск
        const DataProperty& search_column,
        //! Значение поиска по search_column
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole) const;
    //! Поиск
    QModelIndexList find(
        //! Индекс строки для старта поиска
        const QModelIndex& start,
        //! Код сущности
        const EntityCode& entity_code,
        //! Колонка по которой идет поиск
        const PropertyID& search_column,
        //! Колонка, результ из которой возвращается методом
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole) const;
    //! Поиск
    QModelIndexList find(
        //! Индекс строки для старта поиска
        const QModelIndex& start,
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Колонка по которой идет поиск
        const PropertyID& search_column,
        //! Значение поиска по search_column
        const QVariant& value,
        //! Роль поиска
        int search_role = Qt::DisplayRole) const;

    //! Поиск (обычный Qt-шный match - для единообразия имен методов поиска)
    QModelIndexList find(const QModelIndex& start, int role, const QVariant& value, int hits = 1,
                         Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const;
};

} // namespace zf

#endif // ITEMMODEL_H
