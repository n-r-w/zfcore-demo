#pragma once
#include <QObject>
#include "zf_uid.h"
#include "zf_error.h"
#include "zf_object_extension.h"

namespace zf
{
/*! Базовый абстрактный класс для менеджера разделяемых объектов */
class ZCORESHARED_EXPORT SharedObjectManager : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    SharedObjectManager(
        //! Какие типы объектов надо кэшировать. Ключ - тип объекта, значение - количество объектов в кэше
        const QMap<int, int>& cache_config,
        //! Размер кэша по умолчанию
        int default_cache_size,
        //! Размер буфера последних операций
        int history_size);
    ~SharedObjectManager();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;    
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    /*! Получить объект по его идентификатору
     * Если объект используется, то сразу возвращается указатель. Если есть в кэше, то копируется из кэша. 
     * Иначе создается новый */
    QObjectPtr getObject(
        const Uid& uid,
        //! Объект должен быть отсоединен. Т.е. если такой уже используется, то надо сделать его копию. Если не используется - загрузить
        bool is_detached,
        //! Взято из кэша
        bool& is_from_cache,
        /*! Клонировано. Данные клонируются при условии: 
         * - Запрашивается отсоединенный объект
         * - Объект с таким идентификатором уже используется */
        bool& is_cloned,
        //! Создан новый объект
        bool& is_new,
        //! Если ошибка, то метод возвращает nullptr
        Error& error) const;

    //! Получить список используемых объектов
    QList<QObjectPtr> activeObjects() const;

    //! Очистить кэш полностью
    void clearCache();
    //! Очистить кэш от объектов определенного типа
    void clearCache(int object_type);
    //! Очистить кэш от конкретного объекта
    void clearCache(const Uid& uid);
    //! Очистить кэш от конкретного объекта
    void clearCache(const UidList& uids);

    //! Текущий размер кэша для объектов определенного типа
    int cacheSize(int object_type) const;
    //! Запретить кэширование для данного типа сущностей
    void disableCache(int object_type);

    //! История последних операций для отладки
    const QContiguousCache<QPair<SharedObjectEventType, Uid>>& history() const;

protected:            
    //! Создать новый объект
    virtual QObject* createObject(const Uid& uid,
                                  //! Является ли объект отсоединенным. Такие объекты предназначены для редактирования и не доступны для
                                  //! получения через менеджер
                                  bool is_detached, Error& error) const = 0;
    //! Обработка объекта после создания shared_ptr
    virtual void prepareSharedPointerObject(const std::shared_ptr<QObject>& object) const;

    //! Извлечь тип из идентификатора
    virtual int extractType(const Uid& uid) const = 0;
    //! Извлечь идентификатор из объекта
    virtual Uid extractUid(QObject* object) const = 0;
    //! Является ли объект отсоединенным (например, взятым на редактирование)
    virtual bool isDetached(QObject* object) const = 0;
    //! Можно ли копировать данные из объекта
    virtual bool canCloneData(QObject* object) const = 0;
    //! Копирование данные из одного объекта в другой
    virtual void cloneData(QObject* source, QObject* destination) const = 0;
    //! Пометить объект как временный (если надо)
    virtual void markAsTemporary(QObject* object) const;
    //! Можно ли разделять объект между потоками
    virtual bool isShareBetweenThreads(const Uid& uid) const = 0;

    //! Удалить объект из хэша и кэша
    void removeObject(const Uid& uid);

private:
    //! Создать SharedPtr на основании сырого объекта
    std::shared_ptr<QObject> createObjectSharedPointer(QObject* object) const;

    //! вызывается когда счетчик использования объекта упал до 0
    void noReferenceObject(QObject* object) const;
    //! Записать операцию в журнал
    void writeHistory(SharedObjectEventType type, const Uid& uid1, const Uid& uid2 = {}) const;

    //! Информация по используемым моделям
    struct ObjectHashInfo
    {
        ~ObjectHashInfo();
        void clear();
        void setObject(const QObjectPtr& obj);
        QObjectPtr object() const;
        bool isNull() const;
        bool isExpired() const;

    private:
        std::weak_ptr<QObject>* _object = nullptr;
    };
    typedef std::shared_ptr<SharedObjectManager::ObjectHashInfo> ObjectHashInfoPtr;

    //! Информация по кэшируемым моделям
    struct ObjectCacheInfo
    {
        ObjectCacheInfo();
        ~ObjectCacheInfo();

        void clear();
        QObjectPtr* object() const;
        void setObject(QObjectPtr* obj);

    private:
        QObjectPtr* _object = nullptr;
    };
    typedef std::shared_ptr<SharedObjectManager::ObjectCacheInfo> ObjectCacheInfoPtr;
    typedef QCache<Uid, SharedObjectManager::ObjectCacheInfoPtr> ObjectCache;

    //! Кэш по типу сущности
    ObjectCache* getCache(int type) const;

    //! Получить указатель на корректный (не перемещенный частично в кэш) элемент хэша
    ObjectHashInfo* validHashItem(const Uid& uid) const;
    //! Получить указатель на корректный (перемещенный полнстью в кэш) элемент кэша
    ObjectCacheInfo* validCacheItem(const Uid& uid) const;

    //! Блокировка помещения в кэш
    void blockCache() const;
    //! Разблокировка помещения в кэш
    void unBlockCache() const;
    bool isCacheBlocked() const;
    //! Забрать объект из кэша. Если нет в кэше, то возвращает nullptr
    QObjectPtr takeFromCache(const Uid& uid) const;

    //! Используемые в настоящий момент объекты
    mutable QHash<Uid, ObjectHashInfoPtr> _hash;
    //! Какие типы объектов надо кэшировать. Ключ - тип объекта, значение - количество объектов в кэше
    QMap<int, int> _cache_config;
    //! Размер кэша по умолчанию
    int _default_cache_size;
    //! Кэш по типу объекта
    mutable QMap<int, std::shared_ptr<ObjectCache>> _cache;
    //! Блокировка помещения в кэш - счетчик
    mutable int _block_cache = 0;

    //! История последних операций для отладки
    mutable QContiguousCache<QPair<SharedObjectEventType, Uid>> _history;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
