#include "zf_hierarchy_list.h"
#include <QDataStream>

namespace zf
{

HierarchyList::HierarchyList(const QString& id, const QVariant& value, const QMap<QString, QVariant>& data, const QList<HierarchyList>& children)
    : _id(id)
    , _value(value)
    , _data(data)
    , _children(children)
{
}

bool HierarchyList::isEmpty() const
{
    return _id.isEmpty() && !_value.isValid() && _data.isEmpty() && _children.isEmpty();
}

QString HierarchyList::id() const
{
    return _id;
}

QVariant HierarchyList::value() const
{
    return _value;
}

const QMap<QString, QVariant>& HierarchyList::data() const
{
    return _data;
}

const QList<HierarchyList>& HierarchyList::children() const
{
    return _children;
}

void HierarchyList::addData(const QString& key, const QVariant& value)
{
    _data[key] = value;
}

QVariant HierarchyList::value(const QString& key) const
{
    return _data.value(key);
}

void HierarchyList::addChild(const HierarchyList& c)
{
    _children << c;
}

void HierarchyList::addData(const QMap<QString, QVariant>& c)
{
    _data.insert(c);
}

void HierarchyList::addChild(const QString& id, const QVariant& value)
{
    _children << HierarchyList(id, value);
}

void HierarchyList::clear()
{
    *this = HierarchyList();
}

static const int _HierarchyList_stream_version = 1;
QDataStream& operator<<(QDataStream& s, const zf::HierarchyList& v)
{
    return s << _HierarchyList_stream_version << v._id << v._value << v._data << v._children;
}
QDataStream& operator>>(QDataStream& s, zf::HierarchyList& v)
{
    v.clear();

    int version;
    s >> version;
    if (version != _HierarchyList_stream_version) {
        if (s.status() == QDataStream::Ok)
            s.setStatus(QDataStream::ReadCorruptData);
        return s;
    }

    s >> v._id >> v._value >> v._data >> v._children;
    if (s.status() != QDataStream::Ok)
        v.clear();

    return s;
}

} // namespace zf
