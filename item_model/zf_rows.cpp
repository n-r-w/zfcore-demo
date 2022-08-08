#include "zf_rows.h"
#include "zf_core.h"

namespace zf
{
//! Разделяемые данные для ZRows
class Rows_data : public QSharedData
{
public:
    Rows_data();
    Rows_data(const Rows_data& d);
    ~Rows_data();

    //! Строка
    QModelIndexList rows;  
};

Rows::operator int() const
{
    Z_CHECK(count() == 1);
    return firstRow();
}

Rows::operator QList<QStandardItem*>() const
{
    return toStandardItems();
}

Rows::operator QModelIndex() const
{
    Z_CHECK(count() == 1);
    return first();
}

Rows::operator QStandardItem*() const
{
    Z_CHECK(count() == 1);

    const QStandardItemModel* m = qobject_cast<const QStandardItemModel*>(_d->rows.at(0).model());
    Z_CHECK_X(m != nullptr , utf("Модель не является наследником QStandardItemModel"));

    return m->itemFromIndex(first());
}

Rows::Rows()
    : _d(new Rows_data)
{
}

Rows::Rows(const Rows& r)
    : _d(r._d)
{
}

Rows::Rows(const QModelIndexList& indexes)
    : _d(new Rows_data)
{
    append(indexes);
}

Rows::Rows(const QSet<QModelIndex>& r)
    : Rows(r.values())
{
}

Rows::Rows(const QModelIndex& r)
    : Rows(QModelIndexList({r}))
{
}

Rows::~Rows()
{
}

int Rows::count() const
{
    return _d->rows.count();
}

int Rows::size() const
{
    return count();
}

bool Rows::isEmpty() const
{
    return _d->rows.isEmpty();
}

zf::Rows::operator QModelIndexList() const
{
    return _d->rows;
}

QModelIndex Rows::at(int i) const
{
    Z_CHECK(i >= 0 && i < count());
    Z_CHECK(_d->rows.at(i).isValid());
    return _d->rows.at(i);
}

QModelIndex Rows::first() const
{
    return at(0);
}

QModelIndex Rows::last() const
{
    return at(count() - 1);
}

int Rows::atRow(int i) const
{
    return at(i).row();
}

int Rows::firstRow() const
{
    return atRow(0);
}

int Rows::lastRow() const
{
    return atRow(count() - 1);
}

QList<int> Rows::toListInt() const
{
    return QList<int>(*this);
}

QSet<QModelIndex> Rows::toSet() const
{
    return QSet<QModelIndex>(Utils::toSet(_d->rows));
}

QModelIndexList Rows::toList() const
{
    return _d->rows;
}

QList<QStandardItem*> Rows::toStandardItems() const
{
    QList<QStandardItem*> res;
    if (isEmpty())
        return res;

    const QStandardItemModel* m = qobject_cast<const QStandardItemModel*>(_d->rows.at(0).model());
    Z_CHECK_X(m != nullptr, utf("Модель не является наследником QStandardItemModel"));

    for (auto i = _d->rows.constBegin(); i != _d->rows.constEnd(); ++i) {
        res << m->itemFromIndex((*i));
        Z_CHECK_NULL(res.last());
    }

    return res;
}

QSet<int> Rows::toSetInt() const
{
    auto res = QList<int>(*this);
    return QSet<int>(Utils::toSet(res));
}

Rows& Rows::append(const Rows& r)
{
    return append(r.toList());
}

Rows& Rows::append(const QModelIndex& r)
{
    return append(QModelIndexList({r}));
}

Rows& Rows::append(const QModelIndexList& r)
{    
    _d->rows.append(r); 
    return *this;
}

Rows& Rows::append(const QSet<QModelIndex>& r)
{
    return append(r.values());
}

Rows& Rows::unite(const Rows& r)
{
    if (isEmpty()) {
        *this = r;

    } else if (!r.isEmpty()) {
        QSet<QModelIndex> set = QSet<QModelIndex>(Utils::toSet(_d->rows));

        for (int i = 0; i < r._d->rows.count(); i++) {
            if (set.contains(r._d->rows.at(i)))
                continue;

            _d->rows.append(r._d->rows.at(i));
        }
    }
    return *this;
}

Rows& Rows::intersect(const Rows& r)
{
    if (isEmpty() || r.isEmpty()) {
        clear();

    } else {
        QSet<QModelIndex> set = QSet<QModelIndex>(Utils::toSet(r._d->rows));

        QModelIndexList new_rows;
        for (int i = 0; i < _d->rows.count(); i++) {
            if (!set.contains(_d->rows.at(i)))
                continue;

            new_rows.append(_d->rows.at(i));
        }

        clear();
        _d->rows = new_rows;        
    }
    return *this;
}

Rows& Rows::subtract(const Rows& r)
{
    if (!isEmpty() && !r.isEmpty()) {
        QSet<QModelIndex> set = QSet<QModelIndex>(Utils::toSet(r._d->rows));

        for (int i = _d->rows.count() - 1; i >= 0; i--) {
            if (!set.contains(_d->rows.at(i)))
                continue;

            _d->rows.removeAt(i);
        }
    }

    return *this;
}

Rows Rows::operator&(const Rows& r) const
{
    return Rows(*this).intersect(r);
}

Rows& Rows::operator&=(const Rows& r)
{
    return intersect(r);
}

Rows Rows::operator+(const QModelIndexList& r) const
{
    return Rows(*this).append(r);
}

Rows& Rows::operator+=(const QModelIndexList& r)
{
    return append(r);
}

Rows Rows::operator-(const Rows& r) const
{
    return Rows(*this).subtract(r);
}

Rows& Rows::operator-=(const Rows& r)
{
    return subtract(r);
}

Rows& Rows::operator<<(const Rows& r)
{
    return append(r);
}

Rows Rows::operator|(const Rows& r) const
{
    return Rows(*this).unite(r);
}

Rows& Rows::operator|=(const Rows& r)
{
    return unite(r);
}

Rows Rows::operator=(const Rows& r)
{
    if (this != &r)
        _d = r._d;
    return *this;
}

Rows& Rows::operator+=(const Rows& r)
{
    return append(r);
}

Rows Rows::operator+(const Rows& r) const
{
    return Rows(*this).append(r);
}

Rows& Rows::sort(int role, const QLocale* locale, CompareOptions options)
{
    if (!isEmpty()) {
        std::sort(_d->rows.begin(), _d->rows.end(), [role, locale, options](const QModelIndex& i1, const QModelIndex& i2) -> bool {
            return comp(i1, i2, role, locale, options);
        });
    }

    return *this;
}

Rows Rows::sorted(int role) const
{
    return Rows(*this).sort(role);
}

void Rows::clear()
{
    if (!isEmpty()) {
        _d->rows.clear();        
    }
}

Rows::const_iterator Rows::cbegin() const
{
    return _d->rows.cbegin();
}

Rows::const_iterator Rows::cend() const
{
    return _d->rows.cend();
}

Rows::const_iterator Rows::begin() const
{
    return _d->rows.begin();
}

Rows::const_iterator Rows::end() const
{
    return _d->rows.end();
}

Rows::operator QList<int>() const
{
    if (isEmpty())
        return QList<int>();

    QList<int> res;
    QModelIndex parent;
    bool p_init = false;
    for (auto i = _d->rows.constBegin(); i != _d->rows.constEnd(); ++i) {
        Z_CHECK(i->isValid());

        if (!p_init) {
            parent = i->parent();
            p_init = true;
        } else {
            if (parent != i->parent())
                Z_HALT(utf("Попытка преобразовать ZRows в QList<int> для иерархического набора данных, при этом "
                           "найденные строки принадлежат разным родителям"));
        }
        res << i->row();
    }

    std::sort(res.begin(), res.end());
    return res;
}

Rows_data::Rows_data()
{
}

Rows_data::Rows_data(const Rows_data& d)
    : QSharedData(d)
    , rows(d.rows)    
{
}

Rows_data::~Rows_data()
{
}

} // namespace zf
