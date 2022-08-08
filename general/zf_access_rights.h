#pragma once

#include "zf.h"
#include <QSet>

namespace zf
{
class AccessRight;
class AccessRights;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::AccessRight& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::AccessRight& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::AccessRights& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::AccessRights& c);

namespace zf
{
//! Право доступа
class ZCORESHARED_EXPORT AccessRight
{
public:
    AccessRight();
    AccessRight(const AccessRight& r);
    AccessRight(const AccessFlag& f);
    //! Для нестандартных AccessFlag
    AccessRight(const quint64& f);
    AccessRight(const qint64& f);

    bool isValid() const;

    AccessFlag flag() const;
    operator quint64() const;

    bool operator<(const AccessRight& r) const;
    bool operator==(const AccessRight& r) const;
    bool operator!=(const AccessRight& r) const;
    AccessRight& operator=(const AccessRight& r);

private:
    union Flag
    {
        Flag() {}
        Flag(quint64 num)
            : num(num)
        {
        }
        quint64 num = 0;
        AccessFlag flag;
    };

    Flag _data;

    friend uint qHash(const AccessRight& r);
    friend QDataStream& ::operator<<(QDataStream& s, const zf::AccessRight& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::AccessRight& c);
};
typedef QList<AccessRight> AccessRightList;
typedef QSet<AccessRight> AccessRightSet;
inline uint qHash(const AccessRight& r)
{
    return ::qHash(r._data.num);
}

/*! Набор прав доступа. Полностью "immutable". Контролируется логическая целостность прав.
 * Например при установке флага Modify должен появиться View и т.п. */
class ZCORESHARED_EXPORT AccessRights
{
public:
    AccessRights();
    AccessRights(const AccessRights& r);
    AccessRights(const AccessRight& r,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});
    AccessRights(const AccessFlag& r,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});
    AccessRights(const qint64& f,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});
    AccessRights(const quint64& f,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});
    AccessRights(const AccessRightList& r,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});
    AccessRights(const AccessRightSet& r,
                 //! Дополнительная произвольная информация о правах. Пары ключ-значение
                 const QMap<QString, QVariant>& additional_rights = {});

    bool operator==(const AccessRights& r) const;
    AccessRights& operator=(const AccessRights& r);

    void clear();

    //! Содержит ли какие либо права
    bool isEmpty() const;

    //! Замена одного флага доступа. Может выставить или снять другие флаги в зависимости от логики прав
    AccessRights set(const AccessRight& flag, bool on) const;
    //! Замена одного флага доступа. Может выставить или снять другие флаги в зависимости от логики прав
    AccessRights set(AccessFlag flag, bool on) const;

    //! Проверка наличия флага доступа
    bool contains(const AccessRight& flag) const;
    //! Проверка наличия флага доступа
    bool contains(AccessFlag flag) const;
    //! Проверка наличия набора флагов доступа
    bool contains(const AccessRights& rights) const;
    //! Проверка наличия набора флагов доступа
    bool contains(const QList<AccessFlag>& flags) const;

    //! Список прав
    const AccessRightSet& rights() const;

    //! Дополнительная произвольная информация о правах. Пары ключ-значение
    QMap<QString, QVariant> additionalRights() const;
    //! Дополнительная произвольная информация о правах
    QVariant additional(const QString& key) const;
    //! Содержит ли дополнительную информацию о правах
    bool contains(const QString& key) const;
    //! Изъятие дополнительной информации о правах
    AccessRights remove(const QString& key) const;

    //! Слияние прав
    AccessRights merge(const AccessRights& r) const;
    //! Изъятие прав
    AccessRights remove(const AccessRights& r) const;
    //! Только совпадающие права
    AccessRights intersect(const AccessRights& r) const;

private:
    //! Проверка при инициализации
    void check();
    //! Какие права надо установить вместе с указанным правом
    static AccessRightSet onSet(const AccessRight& f);
    static AccessRightSet onSet(const AccessRightSet& f);
    //! Какие права надо снять вместе с указанным правом
    static AccessRightSet onRemove(const AccessRight& f);

    //! Набор прав
    AccessRightSet _rights;
    //! Дополнительная произвольная информация о правах. Пары ключ-значение
    QMap<QString, QVariant> _additional_rights;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::AccessRights& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::AccessRights& c);
};
typedef QList<AccessRights> AccessRightsList;

inline AccessRights operator+(const AccessRights& r1, const AccessRights& r2)
{
    return r1.merge(r2);
}
inline AccessRights operator|(const AccessRights& r1, const AccessRights& r2)
{
    return operator+(r1, r2);
}
inline AccessRights operator-(const AccessRights& r1, const AccessRights& r2)
{
    return r1.remove(r2);
}
inline AccessRights operator&(const AccessRights& r1, const AccessRights& r2)
{
    return r1.intersect(r2);
}
inline AccessRights operator|(AccessFlag r1, AccessFlag r2)
{
    return operator+(AccessRights(r1), AccessRights(r2));
}

} // namespace zf

Q_DECLARE_METATYPE(zf::AccessRight)
Q_DECLARE_METATYPE(zf::AccessRights)
Q_DECLARE_METATYPE(zf::AccessRightsList)
