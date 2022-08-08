#pragma once

#include "zf.h"
#include <QDebug>

namespace zf
{
class ID_Wrapper;
class FieldID;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::ID_Wrapper& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::ID_Wrapper& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::FieldID& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::FieldID& c);

namespace zf
{
//! Базовый класс для всех целочисленных идентификаторов
class ZCORESHARED_EXPORT ID_Wrapper
{
public:
    ID_Wrapper();
    explicit ID_Wrapper(int id);
    ID_Wrapper(const ID_Wrapper& d);
    virtual ~ID_Wrapper();

    ID_Wrapper& operator=(const ID_Wrapper& d);
    bool operator==(const ID_Wrapper& d) const;    
    bool operator!=(const ID_Wrapper& d) const;
    bool operator<(const ID_Wrapper& d) const;
    bool operator<=(const ID_Wrapper& d) const;
    bool operator>(const ID_Wrapper& d) const;
    bool operator>=(const ID_Wrapper& d) const;
    operator QVariant() const;

    //! Значение идентификатора
    int value() const;
    //! Преобразовать в строку
    QString string() const;

    virtual bool isValid() const;
    //! Сброс в isValid() == false
    virtual void clear();

protected:
    int _id;

private:
    friend QDataStream& ::operator<<(QDataStream& s, const zf::ID_Wrapper& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::ID_Wrapper& c);
};

bool ZCORESHARED_EXPORT operator==(const ID_Wrapper& w1, int w2);
bool ZCORESHARED_EXPORT operator!=(const ID_Wrapper& w1, int w2);
bool ZCORESHARED_EXPORT operator==(int w1, const ID_Wrapper& w2);
bool ZCORESHARED_EXPORT operator!=(int w1, const ID_Wrapper& w2);

//! Идентификатор БД
class ZCORESHARED_EXPORT DatabaseID : public ID_Wrapper
{
public:
    DatabaseID();
    explicit DatabaseID(int id);    

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static DatabaseID fromVariant(const QVariant& v);
};

//! Код сущности
class ZCORESHARED_EXPORT EntityCode : public ID_Wrapper
{
public:
    EntityCode();
    explicit EntityCode(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static EntityCode fromVariant(const QVariant& v);
};

//! Идентификатор сущности в рамках ее кода
class ZCORESHARED_EXPORT EntityID : public ID_Wrapper
{
public:
    EntityID();
    explicit EntityID(int id);    

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static EntityID fromVariant(const QVariant& v);
};

//! Идентификатор свойства данных
class ZCORESHARED_EXPORT PropertyID : public ID_Wrapper
{
public:
    PropertyID();
    explicit PropertyID(int id);    

    //! Идентификатор по умолчанию (используется там, где не принципиально какое именно свойство нужно.
    //! Например при создании структур данных не связанных с сущностями)
    static PropertyID def();

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static PropertyID fromVariant(const QVariant& v);
};

//! Идентификатор операции
class ZCORESHARED_EXPORT OperationID : public ID_Wrapper
{
public:
    OperationID();
    explicit OperationID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static OperationID fromVariant(const QVariant& v);
};

//! Команда
class ZCORESHARED_EXPORT Command : public ID_Wrapper
{
public:
    Command();
    explicit Command(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static Command fromVariant(const QVariant& v);
};

//! Шаблонный класс для создания идентификаторов с постоянно возрастающим id
template <class T> class Handle
{
public:
    Handle()
        : _id(std::numeric_limits<T>::min())
    {
    }
    Handle(const Handle& m)
        : _id(m._id)
    {
    }
    explicit Handle(T id)
        : _id(id)
    {
    }

    Handle& operator=(const Handle& m)
    {
        _id = m._id;
        return *this;
    }
    bool operator==(const Handle& m) const { return _id == m._id; }
    bool operator==(T m) const { return _id == m; }
    bool operator!=(const Handle& m) const { return _id != m._id; }
    bool operator!=(T m) const { return _id != m; }
    bool operator<(const Handle& m) const { return _id < m._id; }
    bool operator<(T m) const { return _id < m; }

    bool isValid() const { return _id > std::numeric_limits<T>::min(); }
    //! Представить в виде строки
    QString toString() const { return QString::number(_id); }
    //! Сделать невалидным
    void clear() { _id = std::numeric_limits<T>::min(); }

    //! Для функции qHash
    uint hashValue() const { return ::qHash(_id); }

    T id() const { return _id; }

    //! Создать новый идентификатор
    static Handle generate() { return Handle(++_max_id); }

private:
    T _id;
    inline static QAtomicInteger<T> _max_id;
};

//! Зарезервированное значение для хранения информации что данные некорректны
class ZCORESHARED_EXPORT InvalidValue
{
public:
    InvalidValue();
    InvalidValue(const InvalidValue& v);
    InvalidValue(const QVariant& value);
    ~InvalidValue();

    bool isValid() const;
    void clear();

    //! Некорректное значение
    QVariant value() const;
    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Преобразовать в строку
    QString string() const;

    //! Восстановить из QVariant
    static InvalidValue fromVariant(const QVariant& v);
    //! Проверка хранит ли данный QVariant InvalidValue
    static bool isInvalidValueVariant(const QVariant& v);

    bool operator==(const InvalidValue& v) const;
    bool operator!=(const InvalidValue& v) const;

    InvalidValue& operator=(const InvalidValue& v);

private:
    static int _user_type;
    std::unique_ptr<QVariant> _value;

    friend class Framework;
};

typedef QList<DatabaseID> DatabaseIDList;
typedef QList<EntityCode> EntityCodeList;
typedef QList<EntityID> EntityIDList;
typedef QList<PropertyID> PropertyIDList;
typedef QSet<PropertyID> PropertyIDSet;
typedef QList<OperationID> OperationIDList;
typedef QList<Command> CommandList;

inline uint qHash(const zf::DatabaseID& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::EntityCode& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::EntityID& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::PropertyID& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::OperationID& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::Command& d)
{
    return ::qHash(d.value());
}

} // namespace zf

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::ID_Wrapper& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::InvalidValue& c);

Q_DECLARE_METATYPE(zf::DatabaseID)
Q_DECLARE_METATYPE(zf::EntityCode)
Q_DECLARE_METATYPE(zf::EntityID)
Q_DECLARE_METATYPE(zf::PropertyID)
Q_DECLARE_METATYPE(zf::OperationID)
Q_DECLARE_METATYPE(zf::Command)
Q_DECLARE_METATYPE(zf::InvalidValue)

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::InvalidValue& v);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::InvalidValue& v);

//! Макро для создания операторов для наследников zf::Handle
#ifdef Q_OS_LINUX
#define Z_HANDLE_OPERATORS(X)                                                                                                              \
    inline QDataStream& operator<<(QDataStream& out, const X& obj) { return out << obj.id(); }                                             \
    inline QDataStream& operator>>(QDataStream& in, X& obj)                                                                                \
    {                                                                                                                                      \
        auto id = obj.id();                                                                                                                \
        in >> id;                                                                                                                          \
        obj = X(id);                                                                                                                       \
        return in;                                                                                                                         \
    }                                                                                                                                      \
    inline QDebug operator<<(QDebug debug, const X& c)                                                                                     \
    {                                                                                                                                      \
        QDebugStateSaver saver(debug);                                                                                                     \
        return debug.nospace().noquote() << c.id();                                                                                        \
    }
#else
#define Z_HANDLE_OPERATORS(X)                                                                                                              \
    inline QDataStream& operator<<(QDataStream& out, const X& obj) { return out << obj.id(); }                                             \
    inline QDataStream& operator>>(QDataStream& in, X& obj)                                                                                \
    {                                                                                                                                      \
        auto id = obj.id();                                                                                                                \
        in >> id;                                                                                                                          \
        obj = X(id);                                                                                                                       \
        return in;                                                                                                                         \
    }                                                                                                                                      \
    inline QDebug(operator<<(QDebug debug, const X& c))                                                                                    \
    {                                                                                                                                      \
        QDebugStateSaver saver(debug);                                                                                                     \
        return debug.nospace().noquote() << c.id();                                                                                        \
    }
#endif
