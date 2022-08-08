#include "zf_qt_object_container.h"
#include "zf_core.h"
#include "zf_utils.h"

namespace zf
{
QtObjectContainer::QtObjectContainer(QObject* parent)
    : QObject(parent)
{
}

QVariant QtObjectContainer::data(const QObject* o) const
{
    Z_CHECK(_data.contains(o));
    return _data.value(o);
}

bool QtObjectContainer::add(QObject* o, const QVariant& data)
{
    bool isNew = !_objects.contains(o);
    if (isNew) {
        connect(o, &QObject::destroyed, this, &QtObjectContainer::sl_destroyed);
        _objects << o;
    }
    _data[o] = data;

    return isNew;
}

bool QtObjectContainer::clearData(QObject* o)
{
    bool isRemoved = _objects.remove(o);

    if (isRemoved) {
        _data.remove(o);
        disconnect(o, &QObject::destroyed, this, &QtObjectContainer::sl_destroyed);
    }

    return isRemoved;
}

void QtObjectContainer::sl_destroyed(QObject* o)
{
    clearData(o);
}

} // namespace zf
