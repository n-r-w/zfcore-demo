#pragma once

#include "zf.h"
#include <QModelIndex>
#include <QStandardItem>
#include <QSharedDataPointer>

namespace zf
{
class Rows_data;
class DataContainer;

/*! Список строк набора данных. Для не иерархических наборов данных работает как QList<int>, для иерархических как
 * QModelIndexList */
class ZCORESHARED_EXPORT Rows
{
public:
    Rows();
    Rows(const Rows& r);
    Rows(const QModelIndexList& indexes);
    Rows(const QSet<QModelIndex>& r);
    Rows(const QModelIndex& r);
    ~Rows();

    int count() const;
    int size() const;
    bool isEmpty() const;

    operator QList<int>() const;
    //! Возвращает первый и единственный номер строки. Если содержит не одну строку, то ошибка
    operator int() const;
    //! Возвращает первый и единственный QModelIndex. Если содержит не одну строку, то ошибка
    operator QModelIndex() const;
    //! Возвращает первый и единственный QStandardItem. Если содержит не одну строку, то ошибка
    operator QStandardItem*() const;

    operator QModelIndexList() const;
    operator QList<QStandardItem*>() const;

    QModelIndex at(int i) const;
    QModelIndex first() const;
    QModelIndex last() const;
    int atRow(int i) const;
    int firstRow() const;
    int lastRow() const;

    QList<int> toListInt() const;
    QSet<QModelIndex> toSet() const;
    QModelIndexList toList() const;
    QList<QStandardItem*> toStandardItems() const;
    QSet<int> toSetInt() const;

    Rows& append(const Rows& r);

    Rows& append(const QModelIndex& r);
    Rows& append(const QModelIndexList& r);
    //! Добавление QSet. Порядок выстраивается непредсказуемо
    Rows& append(const QSet<QModelIndex>& r);

    //! Объединить с удалением дубликатов
    Rows& unite(const Rows& r);
    //! Найди общие элементы
    Rows& intersect(const Rows& r);
    //! Удалить элементы
    Rows& subtract(const Rows& r);

    Rows operator&(const Rows& r) const;
    Rows& operator&=(const Rows& r);
    Rows operator+(const Rows& r) const;
    Rows& operator+=(const Rows& r);
    Rows operator+(const QModelIndexList& r) const;
    Rows& operator+=(const QModelIndexList& r);
    Rows operator-(const Rows& r) const;
    Rows& operator-=(const Rows& r);
    Rows& operator<<(const Rows& r);
    Rows operator|(const Rows& r) const;
    Rows& operator|=(const Rows& r);
    Rows operator=(const Rows& r);

    //! Сортировка по возрастанию
    Rows& sort(
        //! Сортировка на основании значения в указанной роли. Если -1, то сортировка по порядку в модели
        int role = -1,
        //! Локаль для сравнения по значению (по умолчанию - локаль пользовательского интерфейса)
        const QLocale* locale = nullptr,
        //! Параметры сравнения по значению
        CompareOptions options = CompareOption::CaseInsensitive);
    //! Сортировка по возрастанию и возврат нового отсортированного объекта
    Rows sorted(
        //! Сортировка на основании значения в указанной роли. Если -1, то сортировка по порядку в модели
        int role = -1) const;

    void clear();

    // итератор
    typedef QModelIndexList::const_iterator const_iterator;    
    const_iterator cbegin() const;
    const_iterator cend() const;
    const_iterator constBegin() const { return cbegin(); }
    const_iterator constEnd() const { return cend(); }
    const_iterator begin() const;
    const_iterator end() const;

private:
    QSharedDataPointer<Rows_data> _d;
};

} // namespace zf
