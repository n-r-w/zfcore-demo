#include "zf_object_extension.h"
#include "zf_core.h"

namespace zf
{
ObjectExtension::ObjectExtension(I_ObjectExtension* owner, bool control_delete)
    : _owner(owner)
    , _owner_object(dynamic_cast<QObject*>(_owner))
    , _control_delete(control_delete)
{
    Z_CHECK_NULL(_owner);
    Z_CHECK_NULL(_owner_object);
}

ObjectExtension::~ObjectExtension()
{   
    QMutexLocker lock1(&_external_mutex);
    QMutexLocker lock2(&_data_mutex);

    if (!Core::isBootstraped() || Utils::isAppHalted())
        return;

    if (!_destroyed) {
        // работа с объектом не была завершена корректно через objectExtensionDestroy
        // в некоторых случаях это нормальная ситуация (например не всегда можно контролировать удаление виджетов)
        // сообщаем всем, кто использует о своем удалении
        for (auto i : qAsConst(_external_objects)) {
            QObject* qobj = dynamic_cast<QObject*>(i);
            Z_CHECK_NULL(qobj);

            if (qobj->thread() == QThread::currentThread())
                // при совпадении потоков это должно быть безопасно
                i->objectExtensionDeleteInfoExternal(_owner);
            else if (_control_delete)
                Z_HALT(QString("class: %1, name: %2").arg(_owner_object->metaObject()->className(), _owner_object->objectName()));
        }
    }
}

bool ObjectExtension::objectExtensionDestroyed() const
{
    return _destroyed;
}

void ObjectExtension::objectExtensionDestroy()
{    
    QMutexLocker lock(&_external_mutex);

    if (!Core::isBootstraped() || Utils::isAppHalted())
        return;

    if (_destroyed)
        return;

    _destroyed = true;

    // сообщаем всем, кто использует о своем удалении
    for (auto i : qAsConst(_external_objects)) {
        i->objectExtensionDeleteInfoExternal(_owner);
    }

    _owner_object->deleteLater();
}

void ObjectExtension::startUseInternal()
{
    Z_CHECK(QThread::currentThread() == _owner_object->thread());

    _use_internal_counter++;
    if (_use_internal_counter == 1)
        _internal_mutex.lock();
}

void ObjectExtension::finishUseInternal()
{
    Z_CHECK(QThread::currentThread() == _owner_object->thread());

    _use_internal_counter--;
    if (_use_internal_counter == 0)
        _internal_mutex.unlock();
    else
        Z_CHECK(_use_internal_counter > 0);
}

QVariant ObjectExtension::objectExtensionGetData(const QString& data_key) const
{
    QMutexLocker lock(&_data_mutex);
    return _data.value(data_key);
}

bool ObjectExtension::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace)
{
    QMutexLocker lock(&_data_mutex);

    if (!replace && _data.contains(data_key))
        return false;

    _data.insert(data_key, value);
    return true;
}

void ObjectExtension::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    Z_CHECK_NULL(i);
    QMutexLocker lock(&_external_mutex);

    auto f = _external_objects_count.find(i);
    if (f == _external_objects_count.end()) {
        _external_objects << i;
        _external_objects_count[i] = 1;

    } else {
        f.value()++;
    }
}

void ObjectExtension::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    Z_CHECK_NULL(i);
    QMutexLocker lock(&_external_mutex);

    auto f = _external_objects_count.find(i);
    Z_CHECK(f != _external_objects_count.end());
    f.value()--;

    if (f.value() == 0) {
        Z_CHECK(_external_objects_count.remove(i));
        Z_CHECK(_external_objects.remove(i));

    } else {
        Z_CHECK(f.value() >= 0);
    }
}

void ObjectExtension::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    Z_CHECK_NULL(i);
    QMutexLocker lock(&_internal_mutex);

    Z_CHECK(_internal_objects.remove(i));
    Z_CHECK(_internal_objects_count.remove(i));
}

void ObjectExtension::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    Z_CHECK_NULL(i);
    QMutexLocker lock(&_internal_mutex);

    auto f = _internal_objects_count.find(i);
    if (f == _internal_objects_count.end()) {
        _internal_objects << i;
        _internal_objects_count[i] = 1;

    } else {
        f.value()++;
    }

    i->objectExtensionRegisterUseExternal(_owner);
}

bool ObjectExtension::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    Z_CHECK_NULL(i);
    QMutexLocker lock(&_internal_mutex);

    auto f = _internal_objects_count.find(i);
    if (f == _internal_objects_count.end())
        return false;

    f.value()--;

    if (f.value() == 0) {
        Z_CHECK(_internal_objects_count.remove(i));
        Z_CHECK(_internal_objects.remove(i));

    } else {
        Z_CHECK(f.value() >= 0);
    }

    i->objectExtensionUnregisterUseExternal(_owner);

    return true;
}

bool _isShutdown()
{
    return !Core::isBootstraped();
}

} // namespace zf
