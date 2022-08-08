#pragma once

#include "zf.h"
#include "zf_row_id.h"
#include <QSortFilterProxyModel>

namespace zf
{
//! Фильтрация древовидных item-моделей
class ZCORESHARED_EXPORT LeafFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    //! Режим работы
    enum Mode
    {
        /*! Поиск
         *  Если головной узел доступен, то видны все его дети
         *  Если головной узел недоступен, но виден хотябы один его ребенок, то и головной становится доступен */
        SearchMode,
        //! Фильтрация. Если головной узел не доступен, то не доступен как он сам, так и все его дети
        FilterMode
    };

    LeafFilterProxyModel(QObject* parent = nullptr);
    LeafFilterProxyModel(Mode mode, QObject* parent = nullptr);

    //! Использовать кэширование (имеет смысл для readonly наборов данных)
    bool useCache() const;
    void setUseCache(bool b);
    //! Очистить кэш. Вызывать перед перефильтрацией
    void resetCache();

    //! Поиск. В отличие от стандартного поиска добавлено кэширование в случае:
    //! start - корневой индекс, Qt::MatchFixedString и Qt::MatchCaseSensitive
    QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits = 1,
                          Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

protected:
    //! Уникальный ключ строки (только если используется кэширование)
    virtual RowID getRowID(int source_row, const QModelIndex& source_parent) const;
    //! Видимость строки бех учета дочерних
    virtual bool filterAcceptsRowItself(int source_row, const QModelIndex& source_parent,
            //! Безусловно исключить строку из иерархии
            bool& exclude_hierarchy) const;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    //! Сортировка
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    //! Видимость строки без учета дочерних или родителя
    bool filterAcceptsRowItselfHelper(int source_row, const QModelIndex& source_parent,
            //! Безусловно исключить строку из иерархии
            bool& exclude_hierarchy) const;
    //! Узел имеет доступных дочерних
    bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const;
    //! Все родители узла доступны
    bool allAcceptedParents(int source_row, const QModelIndex& source_parent) const;

    //! Использовать кэширование (имеет смысл для readonly наборов данных)
    bool _use_cache = false;
    //! Кэш видимости строк. Ключ - getRowKey. Значение - 0(false), 1(true). Первое значение: видимость, второе:
    //! exclude_hierarchy
    mutable QHash<RowID, QPair<int, int>> _accepted_cache;

    //! Режим работы
    Mode _mode = FilterMode;
};
} // namespace zf
