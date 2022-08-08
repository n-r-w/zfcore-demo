#include "zf_callback.h"
#include "zf_core.h"
#include "zf_exception.h"
#include "zf_translation.h"
#include "zf_framework.h"

#include <QDebug>
#include <QApplication>
#include <QEventLoop>

namespace zf
{
QAtomicInt CallbackManager::_stop_all_counter = 0;
QAtomicInt CallbackManager::_ignore_inactive_framework = false;
//! Название свойства объекта, где будет храниться его ключ
const char* CallbackManager::_OBJECT_KEY_PROPERTY = "ZF_CLB";

CallbackManager::CallbackManager(QObject* parent)
    : QObject(parent)
    , _requests_timer(new FeedbackTimer(this))
    , _callback_timer(new FeedbackTimer(this))
    , _object_extension(new ObjectExtension(this))
{
    if (Utils::isAppHalted())
        return;

    connect(_requests_timer, &FeedbackTimer::timeout, this, &CallbackManager::sl_requestsTimeout);
    connect(_callback_timer, &FeedbackTimer::timeout, this, &CallbackManager::sl_callbacksTimeout);
    connect(this, &QObject::objectNameChanged, this, [this](const QString& objectName) {
        _requests_timer->setObjectName(objectName + "_requests_timer");
        _callback_timer->setObjectName(objectName + "_callback_timer");
    });
}

CallbackManager::~CallbackManager()
{
    delete _requests_timer;
    _requests_timer = nullptr;

    delete _callback_timer;
    _callback_timer = nullptr;

    delete _object_extension;
}

bool CallbackManager::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void CallbackManager::objectExtensionDeleteInfoExternal(I_ObjectExtension* obj)
{
    if (Utils::isAppHalted())
        return;

    _object_extension->objectExtensionDeleteInfoExternal(obj);

    _objects_mutex.lock();
    _callback_mutex.lock();
    _requests_mutex.lock();

    _object_info.remove(obj);

    for (int i = _callback_queue.count() - 1; i >= 0; i--) {
        if (_callback_queue.at(i)->object_ptr == obj)
            _callback_queue.removeAt(i);
    }

    removeRequestHelper(obj, -1, true);

    _requests_mutex.unlock();
    _callback_mutex.unlock();
    _objects_mutex.unlock();
}

void CallbackManager::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant CallbackManager::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool CallbackManager::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void CallbackManager::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void CallbackManager::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void CallbackManager::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void CallbackManager::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void CallbackManager::stop()
{
    if (Utils::isAppHalted())
        return;

    if (++_pause_request_count > 1)
        return;

    _requests_timer->stop();
    _callback_timer->stop();
}

void CallbackManager::start()
{
    if (Utils::isAppHalted())
        return;

    if (--_pause_request_count > 0)
        return;

    Z_CHECK(_pause_request_count >= 0);

    _requests_timer->start();
    _callback_timer->start();
}

bool CallbackManager::isActive() const
{    
    return _pause_request_count == 0;
}

void CallbackManager::registerObject(const I_ObjectExtension* object, const QString& callback_slot)
{
    QMutexLocker lock(&_objects_mutex);

    Z_CHECK_NULL(object);
    Z_CHECK(!callback_slot.isEmpty());
    Z_CHECK(!isObjectRegisteredHelper(object));

    auto o_info = Z_MAKE_SHARED(ObjectInfo);
    o_info->object = object;
    o_info->callback_slot = callback_slot;

    _object_info[const_cast<I_ObjectExtension*>(object)] = o_info;

    objectExtensionRegisterUseInternal(const_cast<I_ObjectExtension*>(object));
}

bool CallbackManager::isObjectRegistered(const I_ObjectExtension* object) const
{
    Z_CHECK_NULL(object);
    QMutexLocker lock(&_objects_mutex);
    return isObjectRegisteredHelper(object);
}

bool CallbackManager::isRequested(const I_ObjectExtension* obj, int key) const
{
    Z_CHECK_NULL(obj);
    QMutexLocker lock1(&_callback_mutex);
    QMutexLocker lock2(&_requests_mutex);

    if (getCallbackQueueItem(obj, key).first != nullptr)
        return true;

    return findData(obj, key) != nullptr;
}

void CallbackManager::addRequest(const I_ObjectExtension* obj, int key, const QVariant& data, bool halt_if_not_registered)
{
    if (Utils::isAppHalted())
        return;

    QMutexLocker lock1(&_objects_mutex);
    QMutexLocker lock3(&_requests_mutex);

    auto object_info = _object_info.value(const_cast<I_ObjectExtension*>(obj));
    if (halt_if_not_registered)
        Z_CHECK_NULL(object_info);
    else if (object_info == nullptr)
        return;

    if (findData(obj, key) != nullptr) {
        int pos = indexOf(obj, key);
        Z_CHECK(pos >= 0);
        _requests.at(pos)->data = data;
        if (_requests.count() > 1 && pos != _requests.count() - 1) {
            // Если уже есть, то перемещаем в конец
            _requests.move(pos, _requests.count() - 1);
        }
    } else {
        DataPtr d = Z_MAKE_SHARED(Data);
        d->object_ptr = obj;
        d->key = key;
        d->data = data;
        d->callback_slot = object_info->callback_slot;

        _requests.enqueue(d);
        _requests_by_object.insert(obj, d);
    }

    if (_pause_request_count == 0)
        _requests_timer->start();
}

void CallbackManager::removeRequest(const I_ObjectExtension* obj, int key)
{
    QMutexLocker lock(&_requests_mutex);
    removeRequestHelper(obj, key, false);
}

void CallbackManager::removeObjectRequests(const I_ObjectExtension* obj)
{
    QMutexLocker lock(&_requests_mutex);
    removeRequestHelper(obj, -1, true);
}

QVariant CallbackManager::data(const I_ObjectExtension* obj, int key) const
{
    Z_CHECK_NULL(obj);

    QMutexLocker lock1(&_callback_mutex);
    auto queue_item = getCallbackQueueItem(obj, key);
    if (queue_item.first != nullptr)
        return queue_item.first->data;
    lock1.unlock();

    QMutexLocker lock2(&_requests_mutex);
    auto d = findData(obj, key);
    if (d != nullptr)
        return d->data;

    Z_HALT_INT;
    return QVariant();
}

bool CallbackManager::isAllStopped()
{    
    return _stop_all_counter > 0;
}

void CallbackManager::stopAll()
{
    if (Utils::isAppHalted())
        return;

    ++_stop_all_counter;

}

void CallbackManager::startAll()
{
    if (Utils::isAppHalted())
        return;

    Z_CHECK(--_stop_all_counter >= 0);
}

void CallbackManager::forceIgnoreInactiveFramework(bool b)
{
    Z_CHECK(Utils::isMainThread());
    _ignore_inactive_framework = b;
}

void CallbackManager::sl_callbacksTimeout()
{
    QMutexLocker lock(&_callback_mutex);

    while (!_callback_queue.isEmpty()) {
        auto item = _callback_queue.dequeue();

        QObject* q_obj = dynamic_cast<QObject*>(const_cast<I_ObjectExtension*>(item->object_ptr));
        Z_CHECK_NULL(q_obj);
        QMetaObject::invokeMethod(q_obj, item->callback_slot.toLatin1().data(), Qt::QueuedConnection, Q_ARG(int, item->key),
                                  Q_ARG(QVariant, item->data));
    }
}

bool CallbackManager::isObjectRegisteredHelper(const I_ObjectExtension* object) const
{
    return _object_info.contains(const_cast<I_ObjectExtension*>(object));
}

void CallbackManager::sl_requestsTimeout()
{
    if (Utils::isAppHalted() || _pause_request_count > 0 || !Core::isActive(_ignore_inactive_framework))
        return;

    if (_stop_all_counter > 0) {
        _requests_timer->start();
        return;
    }

    _callback_mutex.lock();
    _requests_mutex.lock();

    while (!_requests.isEmpty()) {
        auto item = _requests.dequeue();
        _requests_by_object.remove(item->object_ptr, item);

        // помещаем в очередь на выполнение колбека
        auto callback_item = Z_MAKE_SHARED(CallbackQueue);
        callback_item->object_ptr = item->object_ptr;
        callback_item->key = item->key;
        callback_item->data = item->data;
        callback_item->callback_slot = item->callback_slot;
        _callback_queue.enqueue(callback_item);
    }

    _requests_mutex.unlock();

    if (!_callback_queue.isEmpty()) {
        _callback_mutex.unlock();
        if (_pause_request_count == 0)
            _callback_timer->start();

    } else {
        _callback_mutex.unlock();
    }
}

QPair<CallbackManager::CallbackQueue*, int> CallbackManager::getCallbackQueueItem(const I_ObjectExtension* obj, int key) const
{
    for (int i = 0; i < _callback_queue.count(); i++) {
        auto& item = _callback_queue.at(i);
        if (item->object_ptr == obj && item->key == key)
            return {item.get(), i};
    }
    return {};
}

int CallbackManager::indexOf(const I_ObjectExtension* obj, int key) const
{
    int foundPos = -1;
    for (int i = _requests.count() - 1; i >= 0; i--) {
        const DataPtr& d = _requests.at(i);
        if (d->object_ptr == obj && d->key == key) {
            foundPos = i;
            break;
        }
    }

    return foundPos;
}

QList<int> CallbackManager::indexesOf(const I_ObjectExtension* obj, int key, bool all_requests) const
{
    QList<int> res;

    for (int i = _requests.count() - 1; i >= 0; i--) {
        const DataPtr& d = _requests.at(i);
        if (d->object_ptr == obj && (all_requests || d->key == key)) {
            res << i;
            if (!all_requests)
                break;
        }
    }

    return res;
}

CallbackManager::DataPtr CallbackManager::findData(const I_ObjectExtension* object, int key) const
{
    auto data = _requests_by_object.values(object);
    for (auto& d : qAsConst(data)) {
        if (d->key == key)
            return d;
    }
    return nullptr;
}

QList<CallbackManager::DataPtr> CallbackManager::findAllData(const I_ObjectExtension* object, int key, bool all_requests) const
{
    if (all_requests)
        return _requests_by_object.values(object);
    else
        return {findData(object, key)};
}

bool CallbackManager::removeRequestHelper(const I_ObjectExtension* obj, int key, bool all_requests)
{
    Z_CHECK_NULL(obj);

    bool found = false;
    if (all_requests) {
        QList<int> pos = indexesOf(obj, -1, true);
        found = !pos.isEmpty();

        for (auto i = pos.constBegin(); i != pos.constEnd(); ++i) {
            DataPtr d = _requests.at(*i);
            _requests_by_object.remove(obj, d);
            _requests.removeAt(*i);
        }

    } else {
        int pos = indexOf(obj, static_cast<int>(key));

        if (pos >= 0) {
            DataPtr d = _requests.at(pos);
            _requests.removeAt(pos);
            _requests_by_object.remove(obj, d);
            found = true;
        }
    }

    return found;
}

} // namespace zf
