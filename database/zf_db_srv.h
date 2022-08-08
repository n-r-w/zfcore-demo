#pragma once

#include "zf.h"
#include "zf_core_consts.h"

namespace zf
{
class TableID;
class FieldID;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::TableID& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::TableID& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::FieldID& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::FieldID& c);

namespace zf
{
//! Базовый класс идентификатора таблиц. Не завязан на конкретные id таблиц, т.к. ядро ничего не должно знать о специфике сервера
class ZCORESHARED_EXPORT TableID
{
public:
    TableID();
    TableID(const TableID& t);

    //! Инициализировано
    bool isValid() const;

    //! Фактическое значение id таблицы
    int value() const;

    //! Преобразование в QVariant
    QVariant variant() const;
    //! Восстановление из QVariant
    static TableID fromVariant(const QVariant& v);

    bool operator==(const TableID& t) const;
    TableID& operator=(const TableID& t);
    bool operator<(const TableID& t) const; // Для использования в QMap

    //! Служебный метод создания id таблицы. Не использовать.
    static TableID createTableID(int id);

protected:
    TableID(int id);

private:
    int _id;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::TableID& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::TableID& c);
};
// Для использования в QHash
inline uint qHash(const TableID& t)
{
    return ::qHash(t.value());
}

// Базовый класс всех полей. Не завязан на конкретные id полей, т.к. ядро ничего не должно знать о специфике сервера
class ZCORESHARED_EXPORT FieldID
{
public:
    FieldID();

    //! Инициализировано
    bool isValid() const;

    //! Таблица
    TableID table() const;
    //! Фактическое значение id поля
    int value() const;

    //! Преобразование в QVariant
    QVariant variant() const;
    //! Восстановление из QVariant
    static FieldID fromVariant(const QVariant& v);

    bool operator==(const FieldID& f) const;
    bool operator<(const FieldID& f) const;

    //! Служебный метод создания id поля. Не использовать.
    static FieldID createFieldID(const TableID& table, int field);

protected:
    FieldID(const TableID& table, int field);

private:
    TableID _table;
    int _field;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::FieldID& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::FieldID& c);
};
// Для использования в QHash
inline uint qHash(const FieldID& f)
{
    return ::qHash(QString::number(f.table().value()) + zf::Consts::KEY_SEPARATOR + QString::number(f.value()));
}

//! Список полей одной таблицы с проверкой на корректность
class ZCORESHARED_EXPORT FieldIdList : public QList<FieldID>
{
public:
    FieldIdList();    
    FieldIdList(const std::initializer_list<FieldID>& fields);
    FieldIdList(const QList<FieldID>& fields);
    FieldIdList(const QSet<FieldID>& fields);

    // Переопределение методов QList для проверки корректности
    FieldIdList& insert(int pos, const FieldID& f);
    FieldIdList& append(const FieldID& f);
    FieldIdList& prepend(const FieldID& f);
    FieldIdList& operator+=(const FieldID& f);
    FieldIdList& operator+=(const FieldIdList& fl);
    FieldIdList& operator<<(const FieldID& f);
    FieldIdList& operator<<(const FieldIdList& f);

private:
    //! Проверка поля на корректность
    void check(const FieldID& f) const;
};

ZCORESHARED_EXPORT FieldIdList operator+(const FieldID& f1, const FieldID& f2);
ZCORESHARED_EXPORT FieldIdList operator+(const FieldIdList& f1, const FieldID& f2);

ZCORESHARED_EXPORT FieldIdList operator|(const FieldID& f1, const FieldID& f2);
ZCORESHARED_EXPORT FieldIdList operator|(const FieldIdList& f1, const FieldID& f2);

ZCORESHARED_EXPORT FieldIdList operator<<(const FieldID& f1, const FieldID& f2);
ZCORESHARED_EXPORT FieldIdList operator<<(const FieldIdList& f1, const FieldID& f2);

} // namespace zf

Q_DECLARE_METATYPE(zf::TableID)
Q_DECLARE_METATYPE(zf::FieldID)
Q_DECLARE_METATYPE(zf::FieldIdList)

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::TableID& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::FieldID& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::FieldIdList& c);
