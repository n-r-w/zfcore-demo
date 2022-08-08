#include "zf_db_srv.h"
#include "zf_core.h"

namespace zf
{
TableID::TableID()
    : _id(0)
{
}

TableID::TableID(const TableID& t)
    : _id(t._id)
{
}

bool TableID::isValid() const
{
    return _id > 0;
}

int TableID::value() const
{
    return _id;
}

QVariant TableID::variant() const
{
    return QVariant::fromValue(*this);
}

TableID TableID::fromVariant(const QVariant& v)
{
    return v.value<TableID>();
}

bool TableID::operator==(const TableID& t) const
{
    return t._id == _id;
}

TableID& TableID::operator=(const TableID& t)
{
    _id = t._id;
    return *this;
}

bool TableID::operator<(const TableID& t) const
{
    return t._id < _id;
}

TableID TableID::createTableID(int id)
{
    Z_CHECK(id >= 0);
    return TableID(id);
}

TableID::TableID(int id)
    : _id(id)
{
}

FieldID::FieldID()
    : _field(0)
{
}

bool FieldID::isValid() const
{
    return _table.isValid();
}

TableID FieldID::table() const
{
    return _table;
}

int FieldID::value() const
{
    return _field;
}

QVariant FieldID::variant() const
{
    return QVariant::fromValue(*this);
}

FieldID FieldID::fromVariant(const QVariant& v)
{
    return v.value<FieldID>();
}

bool FieldID::operator==(const FieldID& f) const
{
    return f._table == _table;
}

bool FieldID::operator<(const FieldID& f) const // Для использования в QMap
{
    if (f._table < _table)
        return true;
    return f._field < _field;
}

FieldID FieldID::createFieldID(const TableID& table, int field)
{
    Z_CHECK(table.isValid());
    Z_CHECK(field > 0);

    return FieldID(table, field);
}

FieldID::FieldID(const TableID& table, int field)
    : _table(table)
    , _field(field)
{
}

FieldIdList::FieldIdList()
{
}

FieldIdList::FieldIdList(const std::initializer_list<FieldID>& fields)
    : QList<FieldID>(fields)
{
    for (auto& f : fields) {
        check(f);
    }
}

FieldIdList::FieldIdList(const QList<FieldID>& fields)
    : QList<FieldID>(fields)
{
    for (auto& f : fields) {
        check(f);
    }
}

FieldIdList::FieldIdList(const QSet<FieldID>& fields)
    : QList<FieldID>(fields.values())
{
    for (auto& f : fields) {
        check(f);
    }
}

FieldIdList& FieldIdList::insert(int pos, const FieldID& f)
{
    QList<FieldID>::insert(pos, f);
    check(f);
    return *this;
}

FieldIdList& FieldIdList::append(const FieldID& f)
{
    return insert(count(), f);
}

FieldIdList& FieldIdList::prepend(const FieldID& f)
{
    return insert(0, f);
}

FieldIdList& FieldIdList::operator+=(const FieldID& f)
{
    return append(f);
}

FieldIdList& FieldIdList::operator+=(const FieldIdList& fl)
{
    for (auto& f : fl)
        append(f);
    return *this;
}

FieldIdList& FieldIdList::operator<<(const FieldID& f)
{
    return append(f);
}

FieldIdList& FieldIdList::operator<<(const FieldIdList& f)
{
    return operator+=(f);
}

void FieldIdList::check(const FieldID& f) const
{
    Z_CHECK_X(f.table() == at(0).table(),
              QString("Для поля %1 не совпадает таблица %2 != %3").arg(f.value()).arg(f.table().value()).arg(at(0).table().value()));
}

FieldIdList operator+(const FieldID& f1, const FieldID& f2)
{
    return zf::FieldIdList({f1, f2});
}
FieldIdList operator+(const FieldIdList& f1, const FieldID& f2)
{
    return zf::FieldIdList(f1).append(f2);
}

FieldIdList operator|(const FieldID& f1, const FieldID& f2)
{
    return zf::FieldIdList({f1, f2});
}

FieldIdList operator|(const FieldIdList& f1, const FieldID& f2)
{
    return zf::FieldIdList(f1).append(f2);
}

FieldIdList operator<<(const FieldID& f1, const FieldID& f2)
{
    return zf::FieldIdList({f1, f2});
}

FieldIdList operator<<(const FieldIdList& f1, const FieldID& f2)
{
    return zf::FieldIdList(f1).append(f2);
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::TableID& c)
{
    return s << c._id;
}

QDataStream& operator>>(QDataStream& s, zf::TableID& c)
{
    return s >> c._id;
}

QDataStream& operator<<(QDataStream& s, const zf::FieldID& c)
{
    return s << c._table << c._field;
}

QDataStream& operator>>(QDataStream& s, zf::FieldID& c)
{
    return s >> c._table >> c._field;
}

QDebug operator<<(QDebug debug, const zf::TableID& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << c.value();

    return debug;
}

QDebug operator<<(QDebug debug, const zf::FieldID& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << c.table() << ":" << c.value();

    return debug;
}

QDebug operator<<(QDebug debug, const zf::FieldIdList& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << "(";
    for (int i = 0; i < c.count(); i++) {
        if (i > 0)
            debug << ",";
        debug << c.at(i);
    }
    debug << ")";

    return debug;
}
