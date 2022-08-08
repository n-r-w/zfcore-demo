#pragma once

#include "zf_global.h"
#include <QMutex>
#include <QSet>

namespace zf
{
ZCORESHARED_EXPORT bool _isShutdown();

//! Интерфейс для потокобезопасного расширения возможностей QObject
class I_ObjectExtension
{
public:
    //! Удален ли объект
    virtual bool objectExtensionDestroyed() const = 0;
    //! Безопасно удалить объект
    virtual void objectExtensionDestroy() = 0;

    //! Получить данные
    virtual QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const = 0;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    virtual bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const = 0;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    virtual void objectExtensionRegisterUseExternal(I_ObjectExtension* i) = 0;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    virtual void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) = 0;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    virtual void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) = 0;

    //! Этот объект начинает использовать другой объект
    virtual void objectExtensionRegisterUseInternal(I_ObjectExtension* i) = 0;
    //! Этот объект прекращает использовать другой объект
    virtual void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) = 0;
};

//! Базовая реализация I_ObjectExtension для хранения внутри наследников
//! Это не реализация интерфейса в понятии наследования! Этот объект  должен лежать в наследнике I_ObjectExtension,
//! который вызывает методы этого объекта
class ZCORESHARED_EXPORT ObjectExtension
{
public:
    ObjectExtension(I_ObjectExtension* owner,
                    //! Контролировать корректное удаление объекта
                    bool control_delete = true);
    ~ObjectExtension();

    //! Удален ли объект
    bool objectExtensionDestroyed() const;

    //! Безопасно удалить владельца
    void objectExtensionDestroy();

    //! Получить данные. Потокобезопасно
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const;
    //! Записать данные. Потокобезопасно
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace);

    //! Начало использования внутренних ссылок (блокировка изменения их из других потоков)
    void startUseInternal();
    //! Окончание использования внутренних ссылок (разблокировка изменения их из других потоков)
    void finishUseInternal();

    //! Этот объект начинает использовать другой объект
    //! При этом у другого объекта будет вызван objectExtensionRegisterUseExternal
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i);
    //! Этот объект прекращает использовать другой объект
    //! При этом у другого объекта будет вызван objectExtensionRegisterUseExternal
    //! Возвращает истину, если удаление информации фактически произошло
    bool objectExtensionUnregisterUseInternal(I_ObjectExtension* i);

    //! Другой объект X сообщает этому, что будет хранить указатель на него
    //! Следовательно при удалении этого объекта надо информировать X через objectExtensionDeleteInfo
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i);
    //! Другой объект X сообщает этому, что перестает хранить указатель на него
    //! Следовательно при удалении этого объекта больше не надо информировать X через objectExtensionDeleteInfo
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i);

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i);

private:
    mutable QMutex _data_mutex;
    mutable QMutex _internal_mutex;
    mutable QMutex _external_mutex;

    //! Счетчик блокировки использования внутренних ссылок
    int _use_internal_counter = 0;

    //! Владелец
    I_ObjectExtension* _owner;
    //! Владелец, приведенный к QObject
    QObject* _owner_object;
    //! Владелец удален
    QAtomicInt _destroyed = false;
    //! Данные
    QMap<QString, QVariant> _data;

    //! Кто использует этот объект
    QSet<I_ObjectExtension*> _external_objects;
    //! Кто использует этот объект и количество использований
    QMap<I_ObjectExtension*, int> _external_objects_count;

    //! Кого использует этот объект и количество использований
    QSet<I_ObjectExtension*> _internal_objects;
    //! Кого использует этот объект и количество использований
    QMap<I_ObjectExtension*, int> _internal_objects_count;

    //! Контролировать корректное удаление объекта
    bool _control_delete;
};

//! Объект для корректного удаления наследников I_ObjectExtension
template <class T> class ObjectExtensionPtr
{
public:
    ObjectExtensionPtr()
        : _object(nullptr)
    {
    }
    ObjectExtensionPtr(T* object)
        : _object(object)
    {
    }

    ~ObjectExtensionPtr() { reset(); }

    T* operator->() const { return _object; }    
    bool operator==(std::nullptr_t) const { return _object == nullptr; }
    bool operator!=(std::nullptr_t) const { return _object != nullptr; }
    bool operator==(T* i) const { return _object == i; }
    bool operator!=(T* i) const { return _object != i; }
    bool operator!() const { return _object == nullptr; }
    operator bool() const { return _object != nullptr; }
    ObjectExtensionPtr& operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }
    ObjectExtensionPtr& operator=(T* p)
    {
        if (p == _object)
            return *this;
        reset();
        _object = p;
        return *this;
    }

    T* get() const { return _object; }

    void reset()
    {
        if (_object == nullptr)
            return;

        if (_isShutdown())
            delete _object;
        else
            _object->objectExtensionDestroy();
        _object = nullptr;
    }

private:
    QPointer<T> _object;
};

} // namespace zf

Q_DECLARE_METATYPE(const zf::I_ObjectExtension*)
Q_DECLARE_METATYPE(zf::I_ObjectExtension*)
