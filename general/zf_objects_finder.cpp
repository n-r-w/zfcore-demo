#include "zf_objects_finder.h"
#include "zf_core.h"
#include "zf_child_objects_container.h"

namespace zf
{
ObjectsFinder::ObjectsFinder(const QString& base_name, const QObjectList& sources)
    : _base_name(base_name)
{    
    for (QObject* source : sources) {
        addSource(source);
    }
}

bool ObjectsFinder::hasSource(QObject* source) const
{
    Z_CHECK_NULL(source);
    for (auto& s : qAsConst(_sources)) {
        if (s->object == source)
            return true;
    }
    return false;
}

void ObjectsFinder::addSource(QObject* source)
{
    Z_CHECK(!hasSource(source));

    auto info = Z_MAKE_SHARED(SourceInfo, source);
    connect(info->container, &ChildObjectsContainer::sg_objectNameChanged, this, &ObjectsFinder::sl_objectNameChanged);
    connect(info->container, &ChildObjectsContainer::sg_contentChanged, this, &ObjectsFinder::sl_contentChanged);
    connect(info->container, &ChildObjectsContainer::sg_objectAdded, this, &ObjectsFinder::sg_objectAdded);
    connect(info->container, &ChildObjectsContainer::sg_objectRemoved, this, &ObjectsFinder::sg_objectRemoved);
    _sources << info;
}

QHash<QObject*, QObjectList> ObjectsFinder::childObjects() const
{
    QHash<QObject*, QObjectList> l;
    for (auto& s : qAsConst(_sources)) {
        l[s->object] = s->container->childObjects();
    }
    return l;
}

QHash<QObject*, QWidgetList> ObjectsFinder::childWidgets() const
{
    QHash<QObject*, QWidgetList> l;
    for (auto& s : qAsConst(_sources)) {
        l[s->object] = s->container->childWidgets();
    }
    return l;
}

void ObjectsFinder::clearCache()
{
    _find_element_hash.clear();
}

void ObjectsFinder::setBaseName(const QString& s)
{
    _base_name = s;
}

void ObjectsFinder::sl_objectNameChanged(QObject* object)
{
    clearCache();
    emit sg_objectNameChanged(object);
}

void ObjectsFinder::sl_contentChanged()
{
    clearCache();
    emit sg_contentChanged();
}

// Проверка совпадения пути к объекту
bool ObjectsFinder::compareObjectName(const QStringList& path, QObject* object, bool path_only)
{
    if (object->objectName().isEmpty())
        return false;

    if (path_only) {
        if (path.count() == 1)
            return true;

    } else {
        if (!comp(path.constLast(), object->objectName()))
            return false;

        if (path.count() == 1)
            return true;
    }

    object = object->parent();
    if (object == nullptr)
        return false;

    for (int i = path.count() - 2; i >= 0; i--) {
        // пропускаем родительские объекты с несовпадающим именем
        while (i != path.count() - 1 && object != nullptr && !comp(path.at(i), object->objectName())) {
            object = object->parent();
        }
        if (object == nullptr)
            return false;

        if (i == 0)
            return true;

        object = object->parent();
        if (object == nullptr)
            return false;
    }

    return false;
}

void ObjectsFinder::findObjectsByClass(QObjectList& list, const QString& class_name, bool is_check_source, bool has_names) const
{
    for (auto& source : qAsConst(_sources)) {
        source->container->findObjectsByClass(list, class_name, is_check_source, has_names);
    }
}

QList<QObject*> ObjectsFinder::findObjects(const QStringList& path, const char* class_name) const
{
    Z_CHECK(!path.isEmpty());

    QList<QObject*> res;
    QList<QPointer<QObject>> hash_item;

    QStringList path_prepared;
    for (auto& s : qAsConst(path)) {
        path_prepared << s.trimmed().toLower();
        Z_CHECK(!path_prepared.last().isEmpty());
    }

    QString hash_key = path_prepared.join(Consts::KEY_SEPARATOR);

    auto it = _find_element_hash.find(hash_key);
    if (it == _find_element_hash.end()) {
        for (auto& source : qAsConst(_sources)) {
            if (source->object.isNull())
                continue;

            findObjectByName(source, path_prepared, class_name, false, res);
        }

        for (auto& o : qAsConst(res)) {
            Z_CHECK_NULL(o);
            hash_item << o;
        }

        _find_element_hash[hash_key] = hash_item;

    } else {
        hash_item = it.value();
        for (auto& o : qAsConst(hash_item)) {
            Z_CHECK(!o.isNull()); // не должно быть такого что объект удален, а хэш не сброшен
            res << o.data();
        }
    }

    return res;
}

void ObjectsFinder::findObjectByName(const std::shared_ptr<SourceInfo>& source, const QStringList& path, const char* class_name,
                                     bool is_check_parent, QList<QObject*>& objects)
{
    if (is_check_parent && source->object->inherits(class_name) && compareObjectName(path, source->object, false))
        objects << source->object;

    auto all_objects = source->container->childObjects();
    for (auto o : qAsConst(all_objects)) {
        if (o->inherits(class_name) && compareObjectName(path, o, false))
            objects << o;
    }
}

#define ObjectsFinder_GROUP 1
ObjectsFinder::SourceInfo::SourceInfo(QObject* obj)
    : object(obj)
    , container(new ChildObjectsContainer(obj, ObjectsFinder_GROUP, true))
{
}

ObjectsFinder::SourceInfo::~SourceInfo()
{
    delete container;
}

} // namespace zf
