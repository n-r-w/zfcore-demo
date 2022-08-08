#include "zf_shared_object_manager.h"
#include "zf_core.h"

namespace zf
{
SharedObjectManager::SharedObjectManager(const QMap<int, int>& cache_config, int default_cache_size, int history_size)
    : _cache_config(cache_config)
    , _default_cache_size(default_cache_size)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(_default_cache_size >= 0);
    _history.setCapacity(history_size);
}

SharedObjectManager::~SharedObjectManager()
{
    blockCache();

    //   отключено, т.к. во первых не нужно т.к. и так все очистится при завершении программы, а во вторых вызывает виртуальный метод, который уже вызвать нельзя
    //    _hash.clear();
    //    clearCache();

    delete _object_extension;
}

bool SharedObjectManager::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void SharedObjectManager::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant SharedObjectManager::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool SharedObjectManager::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void SharedObjectManager::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void SharedObjectManager::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void SharedObjectManager::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void SharedObjectManager::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void SharedObjectManager::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

QList<QObjectPtr> SharedObjectManager::activeObjects() const
{
    QList<QObjectPtr> res;
    for (auto& h : _hash) {
        res << h.get()->object();
    }
    return res;
}

void SharedObjectManager::clearCache()
{
    for (auto i = _cache.constBegin(); i != _cache.constEnd(); ++i) {
        clearCache(i.key());
    }
}

void SharedObjectManager::clearCache(int object_type)
{
    blockCache();

    auto cache = _cache.value(object_type);
    if (cache) {
        QList<Uid> keys = cache->keys();
        for (Uid& uid : keys) {
            Z_CHECK(extractType(uid) == object_type);
            clearCache(uid);
        }
    }

    unBlockCache();
}

void SharedObjectManager::clearCache(const Uid& uid)
{
    Z_CHECK(uid.isValid());

    blockCache();
    auto cache = _cache.value(extractType(uid));
    if (cache) {
        ObjectCacheInfoPtr* info = cache->object(uid);
        if (info != nullptr) {
            if ((*info)->object() != nullptr) {
                writeHistory(SharedObjectEventType::RemoveFromCache, uid);
                (*info)->clear();
            }
        }
        cache->remove(uid);
    }

    unBlockCache();
}

void SharedObjectManager::clearCache(const UidList& uids)
{
    for (auto& uid : uids)
        clearCache(uid);
}

int SharedObjectManager::cacheSize(int object_type) const
{
    auto cache = _cache.value(object_type);
    return cache ? cache->size() : 0;
}

void SharedObjectManager::disableCache(int object_type)
{
    _cache.remove(object_type);
    _cache_config[object_type] = 0;
}

const QContiguousCache<QPair<SharedObjectEventType, Uid>>& SharedObjectManager::history() const
{
    return _history;
}

void SharedObjectManager::prepareSharedPointerObject(const std::shared_ptr<QObject>& object) const
{
    Q_UNUSED(object);
}

void SharedObjectManager::removeObject(const Uid& uid)
{
    blockCache();
    _hash.remove(uid);
    clearCache(uid);
    writeHistory(SharedObjectEventType::RemoveFromStorage, uid);
    unBlockCache();
}

QObjectPtr SharedObjectManager::getObject(const Uid& uid, bool is_detached, bool& is_from_cache, bool& is_cloned, bool& is_new,
                                          Error& error) const
{
    bool is_main_thread = Utils::isMainThread();
    bool is_shared = isShareBetweenThreads(uid);
    is_from_cache = false;
    is_cloned = false;
    is_new = false;

    QObjectPtr result_object;

    // надо ли создавать временный объект
    bool is_temporary = uid.isTemporary();

    ObjectHashInfo* hash_item = (is_main_thread || is_shared) ? validHashItem(uid) : nullptr;
    if (hash_item != nullptr) {
        // проверка на криворукость
        Z_CHECK(!hash_item->isNull() && !hash_item->isExpired());
    }

    if (hash_item != nullptr && !is_detached) {
        // используется и не надо создавать отсоединенную копию - возвращаем
        writeHistory(SharedObjectEventType::TakeFromStorage, uid);
        result_object = hash_item->object();

    } else {
        // создаем элемент хэша
        ObjectHashInfoPtr new_hash_item = (is_detached || !is_main_thread) ? nullptr : Z_MAKE_SHARED(ObjectHashInfo);
        // ищем в кэше
        ObjectCacheInfo* cache_item = is_temporary || is_detached || !is_main_thread ? nullptr : validCacheItem(uid);
        if (cache_item != nullptr) {
            // забираем из кэша как есть
            result_object = takeFromCache(uid);

        } else {
            // создаем новый объект
            is_new = true;
            QObject* object = createObject(uid, is_detached, error);
            if (error.isError())
                return nullptr;

            result_object = createObjectSharedPointer(object);
            prepareSharedPointerObject(result_object);

            if (is_detached && hash_item != nullptr && canCloneData(hash_item->object().get())) {
                // можно скопировать данные из хэша
                cloneData(hash_item->object().get(), result_object.get());
                writeHistory(SharedObjectEventType::DetachedCloned, uid);
                is_cloned = true;

            } else if (!is_temporary) {
                // если это не временная модель, то ее надо загрузить из БД
                writeHistory(SharedObjectEventType::LoadedFromDatabase, uid);
            }

            if (is_temporary)
                markAsTemporary(result_object.get());

            writeHistory(is_detached ? SharedObjectEventType::NewDetached : SharedObjectEventType::New, uid);
        }

        if (new_hash_item != nullptr) {
            // помещаем в хэш
            new_hash_item->setObject(result_object);
            _hash[uid] = new_hash_item;
        }
    }

    return result_object;
}

void SharedObjectManager::markAsTemporary(QObject* object) const
{
    Q_UNUSED(object)
}

void SharedObjectManager::writeHistory(SharedObjectEventType type, const Uid& uid1, const Uid& uid2) const
{
    if (_history.capacity() == 0)
        return;

    _history.append(QPair<SharedObjectEventType, Uid>(type, uid1));

    QString message = uid1.toPrintable();

    if (uid2.isValid())
        message += QStringLiteral(" | ") + uid2.toPrintable();
}

SharedObjectManager::ObjectCache* SharedObjectManager::getCache(int type) const
{
    int size = _cache_config.value(type, _default_cache_size);
    if (size <= 0)
        return nullptr;
    auto cache = _cache.value(type);
    if (!cache) {
        cache = Z_MAKE_SHARED(ObjectCache, size);
        _cache[type] = cache;
    }

    return cache.get();
}

std::shared_ptr<QObject> SharedObjectManager::createObjectSharedPointer(QObject* object) const
{
    Z_CHECK_NULL(object);
    return QObjectPtr(object,
                      // нестандартный метод удаления объекта
                      [&](QObject* object) {
                          if (!Core::isBootstraped() || !Core::isActive() || !Utils::isMainThread(object)) {
                              Utils::deleteLater(object);
#ifdef RNIKULENKOV
                              qDebug() << "Free model (no main thread):" << object->objectName();
#endif

                          } else
                              noReferenceObject(object);
                      });
}

void SharedObjectManager::noReferenceObject(QObject* object) const
{
    Z_CHECK_NULL(object);

    // сохраняем конфигурацию моделей
    Model* model = qobject_cast<Model*>(object);
    if (model != nullptr) {
        auto error = model->onSaveConfiguration();
        if (error.isError())
            Core::logError(error);
    }

    Uid uid = extractUid(object);
    bool is_detached = isDetached(object);

    ObjectHashInfo* hash_info
        // отсоединенные удаляем сразу
        = is_detached ? nullptr : _hash.value(uid).get();
    if (hash_info == nullptr) {
        writeHistory(is_detached ? SharedObjectEventType::DestroyDetached : SharedObjectEventType::Destroy, uid);
        Utils::deleteLater(object);

#ifdef RNIKULENKOV
        qDebug() << "Free model (detached):" << object->objectName();
#endif

        return;
    }

    // было перемещено в кэш
    bool model_moved_to_cache = hash_info->isNull();

    ObjectCache* cache = getCache(extractType(uid));
    ObjectCacheInfoPtr* cache_item = (cache != nullptr) ? cache->object(uid) : nullptr;

    // существует в кэше
    bool model_exists_in_cache = (cache_item != nullptr && cache_item->get()->object() != nullptr);

    Z_CHECK(!model_moved_to_cache && !model_exists_in_cache);

    if (cache == nullptr || isCacheBlocked() || uid.isTemporary()) {
        // нет кэша, кэш отключен или это временный объект - удаляем
        blockCache();

        hash_info->clear();
        Utils::deleteLater(object);

#ifdef RNIKULENKOV
        if (uid.isTemporary())
            qDebug() << "Free model (temporary):" << object->objectName();
        else if (cache == nullptr)
            qDebug() << "Free model (no cache):" << object->objectName();
        else
            qDebug() << "Free model (cache blocked):" << object->objectName();
#endif

        writeHistory(SharedObjectEventType::Remove, uid);

        _hash.remove(uid);

        if (cache_item != nullptr)
            cache->remove(uid);

        unBlockCache();
        return;
    }

    blockCache();

    // помещаем в кэш
    if (cache_item == nullptr) {
        cache_item = new ObjectCacheInfoPtr(Z_MAKE_SHARED(ObjectCacheInfo));

#ifdef RNIKULENKOV
        if (cache->count() == cache->maxCost() && !cache->contains(uid))
            qDebug() << "Cache full:" << object->objectName();
#endif

        if (!cache->contains(uid)) {
#ifdef RNIKULENKOV
            qDebug() << "Cache model:" << object->objectName();
#endif
        }

        cache->insert(uid, cache_item);
    }

    Z_CHECK((*cache_item)->object() == nullptr);
    (*cache_item)->setObject(new QObjectPtr(createObjectSharedPointer(object)));
    writeHistory(SharedObjectEventType::ToCache, uid);

    if ((*cache_item)->object() != nullptr) {
        // полностью перемещено в кэш
        _hash.remove(uid);
    }

    unBlockCache();
}

SharedObjectManager::ObjectHashInfo* SharedObjectManager::validHashItem(const Uid& uid) const
{
    auto item = _hash.value(uid).get();
    if (item == nullptr || item->isNull())
        return nullptr;

    return item;
}

SharedObjectManager::ObjectCacheInfo* SharedObjectManager::validCacheItem(const Uid& uid) const
{
    auto cache = _cache.value(extractType(uid));
    if (cache == nullptr)
        return nullptr;

    auto item = cache->object(uid);
    if (item == nullptr)
        return nullptr;

    auto ptr = item->get();
    if (ptr->object() == nullptr)
        return nullptr;

    return ptr;
}

void SharedObjectManager::blockCache() const
{
    _block_cache++;
}

void SharedObjectManager::unBlockCache() const
{
    Z_CHECK(_block_cache > 0);
    _block_cache--;
}

bool SharedObjectManager::isCacheBlocked() const
{
    return _block_cache > 0;
}

QObjectPtr SharedObjectManager::takeFromCache(const Uid& uid) const
{
    QObjectPtr object;
    ObjectCacheInfo* cache_item = validCacheItem(uid);
    if (cache_item != nullptr) {
        Z_CHECK_NULL(cache_item->object());
        object = *(cache_item->object());
        _cache.value(extractType(uid))->remove(uid);
        writeHistory(SharedObjectEventType::TakeFromCache, uid);

#ifdef RNIKULENKOV
        qDebug() << "Take from cache:" << object->objectName();
#endif
    }

    return object;
}

SharedObjectManager::ObjectCacheInfo::ObjectCacheInfo()
{
}

SharedObjectManager::ObjectCacheInfo::~ObjectCacheInfo()
{
    clear();
}

void SharedObjectManager::ObjectCacheInfo::clear()
{
    if (_object != nullptr) {
        delete _object;
        _object = nullptr;
    }
}

QObjectPtr* SharedObjectManager::ObjectCacheInfo::object() const
{
    return _object;
}

void SharedObjectManager::ObjectCacheInfo::setObject(QObjectPtr* obj)
{
    Z_CHECK_NULL(obj);
    Z_CHECK_NULL(*obj);
    Z_CHECK(Utils::isMainThread((*obj).get()));

    clear();
    _object = obj;
}

SharedObjectManager::ObjectHashInfo::~ObjectHashInfo()
{
    clear();
}

void SharedObjectManager::ObjectHashInfo::clear()
{
    if (_object != nullptr) {
        delete _object;
        _object = nullptr;
    }
}

void SharedObjectManager::ObjectHashInfo::setObject(const QObjectPtr& obj)
{
    Z_CHECK_NULL(obj);
    clear();
    Z_CHECK(Utils::isMainThread(obj->thread()));
    _object = new std::weak_ptr<QObject>(obj);
}

QObjectPtr SharedObjectManager::ObjectHashInfo::object() const
{
    Z_CHECK_NULL(_object);
    return _object->lock();
}

bool SharedObjectManager::ObjectHashInfo::isNull() const
{
    return _object == nullptr;
}

bool SharedObjectManager::ObjectHashInfo::isExpired() const
{
    Z_CHECK_NULL(_object);
    return _object->expired();
}

} // namespace zf
