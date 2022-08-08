#include "zf_child_objects_container.h"
#include "zf_core.h"
#include <QAbstractTextDocumentLayout>

namespace zf
{
const char* ChildObjectsContainer::_group_id_property_name = "_zfchg_";
const char* ChildObjectsContainer::_id_property_name = "_zfchi_";
const char* ChildObjectsContainer::_ignore_property_name = "_zfchn_";

ChildObjectsContainer::ChildObjectsContainer(QObject* source, int group_id, bool keep_until_destroyed, QObject* parent)
    : QObject(parent)
    , _group_id(static_cast<uint>(group_id))
    , _code(Handle<uint>::generate())
    , _source(source)
    , _keep_until_destroyed(keep_until_destroyed)
{
    Z_CHECK(group_id > 0);
    Z_CHECK_NULL(_source);
    Z_CHECK(objectContainerID(_source) == 0 && objectContainerGroup(_source) == 0);

    _source->setProperty(_group_id_property_name, _group_id);
    _source->setProperty(_id_property_name, _code.id());

    _source->installEventFilter(this);
    connect(_source, &QObject::destroyed, this, &ChildObjectsContainer::sl_destroyed);
}

QObject* ChildObjectsContainer::source() const
{
    return _source;
}

QObjectList ChildObjectsContainer::childObjects() const
{
    fillCache();
    return _objects_cache;
}

QWidgetList ChildObjectsContainer::childWidgets() const
{
    fillCache();
    return _widgets_cache;
}

void ChildObjectsContainer::findObjectsByClass(QObjectList& list, const QString& class_name, bool is_check_source, bool has_names) const
{
    if (_source.isNull())
        return;

    auto c_name = class_name.toLatin1();

    if (is_check_source && (has_names ? (!_source->objectName().isEmpty()) : true) && _source->inherits(c_name.constData()))
        list.append(_source);

    auto objects = this->childObjects();
    for (auto obj : qAsConst(objects)) {
        if ((has_names ? (!obj->objectName().isEmpty()) : true) && obj->inherits(c_name.constData()))
            list.append(obj);
    }
}

bool ChildObjectsContainer::eventFilter(QObject* watched, QEvent* event)
{
    if (_source.isNull())
        return QObject::eventFilter(watched, event);

    if (event->type() == QEvent::ChildAdded) {
        clearCache();

        // инициализируем динамически добавляемые объекты
        QChildEvent* chEvent = static_cast<QChildEvent*>(event);
        if (!prepareObject(chEvent->child()))
            return QObject::eventFilter(watched, event);

        QObjectList ls;
        Utils::findObjectsByClass(ls, chEvent->child(), QStringLiteral("QObject"), false, false);
        for (QObject* obj : qAsConst(ls)) {
            prepareObject(obj);
        }

    } else if (event->type() == QEvent::ChildRemoved) {
        if (!_keep_until_destroyed)
            clearCache();

    } else if (event->type() == QEvent::ParentChange) {
        if (!_keep_until_destroyed && !Utils::hasParent(watched, _source)) {
            emit sg_objectRemoved(watched);
        }
    }

    return QObject::eventFilter(watched, event);
}

bool ChildObjectsContainer::inContainer(QObject* obj)
{
    Z_CHECK_NULL(obj);
    return objectContainerID(obj) > 0;
}

void ChildObjectsContainer::markIgnored(QObject* obj)
{
    Z_CHECK(!inContainer(obj));
    obj->setProperty(_ignore_property_name, true);
}

void ChildObjectsContainer::sl_destroyed(QObject* obj)
{
    if (_keep_until_destroyed) {
        if (!_objects_cache.removeOne(obj)) {
            Z_CHECK(obj == _source); // других вариантов быть не должно
            return;
        }
    }

    clearCache();

    emit sg_objectRemoved(obj);
}

void ChildObjectsContainer::clearCache()
{
    if (_widgets_cache.isEmpty())
        return;

    if (!_keep_until_destroyed)
        _objects_cache.clear();
    _widgets_cache.clear();

    emit sg_contentChanged();
}

void ChildObjectsContainer::fillCache() const
{
    Z_CHECK_NULL(_source);

    if (!_widgets_cache.isEmpty())
        return;

    if (_keep_until_destroyed) {
        for (auto o : qAsConst(_objects_cache)) {
            if (o->isWidgetType())
                _widgets_cache << qobject_cast<QWidget*>(o);
        }

    } else {
        _objects_cache = _source->findChildren<QObject*>();
        _widgets_cache = _source->findChildren<QWidget*>();
    }
}

bool ChildObjectsContainer::prepareObject(QObject* object)
{
    if (objectContainerIgnored(object))
        return false;

    if (!object->isWidgetType() && !object->isWindowType() && qobject_cast<QButtonGroup*>(object) == nullptr
        && qobject_cast<QLayout*>(object) == nullptr
        // не совсем понятно что скрывается под className равным непосредственно QObject, но без этого не будут найдены QLayout и прочее
        && object->metaObject()->className() != QStringLiteral("QObject")) {
        return false;
    }

    if (_keep_until_destroyed) {
        // не трогаем если этот объект уже принадлежит ChildObjectsContainer
        if (objectContainerID(object) > 0 && objectContainerGroup(object) == _group_id)
            return false;

        _objects_cache << object;

        object->setProperty(_group_id_property_name, _group_id);
        object->setProperty(_id_property_name, _code.id());

        auto children = object->children();
        for (auto o : qAsConst(children)) {
            prepareObject(o);
        }
    }

    object->installEventFilter(this);
    connect(object, &QObject::destroyed, this, &ChildObjectsContainer::sl_destroyed);
    connect(object, &QObject::objectNameChanged, this, [this]() { emit sg_objectNameChanged(sender()); });

    emit sg_objectAdded(object);

    return true;
}

uint ChildObjectsContainer::objectContainerGroup(QObject* object)
{
    return object->property(_group_id_property_name).toUInt();
}

uint ChildObjectsContainer::objectContainerID(QObject* object)
{
    return object->property(_id_property_name).toUInt();
}

bool ChildObjectsContainer::objectContainerIgnored(QObject* object)
{
    return object->property(_ignore_property_name).toUInt() > 0;
}

} // namespace zf
