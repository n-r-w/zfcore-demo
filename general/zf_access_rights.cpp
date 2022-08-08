#include "zf_access_rights.h"
#include "zf_core.h"

namespace zf
{
AccessRight::AccessRight()
{
}

AccessRight::AccessRight(const AccessRight& r)
    : _data(r._data)
{
}

AccessRight::AccessRight(const AccessFlag& f)
    : AccessRight(static_cast<quint64>(f))
{
}

AccessRight::AccessRight(const quint64& f)
    : _data(f)
{
    Z_CHECK(isValid());
}

AccessRight::AccessRight(const qint64& f)
    : AccessRight(static_cast<quint64>(f))
{
}

bool AccessRight::isValid() const
{
    return _data.num > 0;
}

AccessFlag AccessRight::flag() const
{
    return _data.flag;
}

zf::AccessRight::operator quint64() const
{
    return _data.num;
}

bool AccessRight::operator<(const AccessRight& r) const
{
    return _data.num < r._data.num;
}

bool AccessRight::operator==(const AccessRight& r) const
{
    return _data.num == r._data.num;
}

bool AccessRight::operator!=(const AccessRight& r) const
{
    return _data.num != r._data.num;
}

AccessRight& AccessRight::operator=(const AccessRight& r)
{
    _data = r._data;
    return *this;
}

AccessRights::AccessRights()
{
}

AccessRights::AccessRights(const AccessRights& r)
    : _rights(r._rights)
    , _additional_rights(r._additional_rights)
{
}

AccessRights::AccessRights(const qint64& f, const QMap<QString, QVariant>& additional_rights)
    : AccessRights(AccessRight(f), additional_rights)
{
}

zf::AccessRights::AccessRights(const zf::AccessRight& r, const QMap<QString, QVariant>& additional_rights)
    : AccessRights(onSet(r), additional_rights)
{
}

AccessRights::AccessRights(const AccessFlag& r, const QMap<QString, QVariant>& additional_rights)
    : AccessRights(AccessRight(r), additional_rights)
{
}

AccessRights::AccessRights(const quint64& f, const QMap<QString, QVariant>& additional_rights)
    : AccessRights(AccessRight(f), additional_rights)
{
}

AccessRights::AccessRights(const AccessRightList& r, const QMap<QString, QVariant>& additional_rights)
    : _rights(onSet(AccessRightSet(Utils::toSet(r))))
    , _additional_rights(additional_rights)
{
    check();
}

AccessRights::AccessRights(const AccessRightSet& r, const QMap<QString, QVariant>& additional_rights)
    : _rights(onSet(r))
    , _additional_rights(additional_rights)
{
    check();
}

bool AccessRights::operator==(const AccessRights& r) const
{
    if (_rights != r._rights || _additional_rights.count() != r._additional_rights.count())
        return false;

    for (auto i = _additional_rights.constBegin(); i != _additional_rights.constEnd(); ++i) {
        auto x = r._additional_rights.constFind(i.key());
        if (x == r._additional_rights.constEnd())
            return false;

        if (!Utils::compareVariant(x.value(), i.value()))
            return false;
    }
    return true;
}

AccessRights& AccessRights::operator=(const AccessRights& r)
{
    _rights = r._rights;
    _additional_rights = r._additional_rights;
    return *this;
}

void AccessRights::clear()
{
    *this = AccessRights();
}

bool AccessRights::isEmpty() const
{
    return _rights.isEmpty();
}

AccessRights AccessRights::set(const AccessRight& flag, bool on) const
{
    Z_CHECK(flag.isValid());
    AccessRights res(*this);
    if (on)
        res._rights.unite(onSet(flag));
    else
        res._rights.subtract(onRemove(flag));

    return res;
}

AccessRights AccessRights::set(AccessFlag flag, bool on) const
{
    return set(AccessRight(flag), on);
}

bool AccessRights::contains(const AccessRight& flag) const
{
    return !flag.isValid() || _rights.contains(flag);
}

bool AccessRights::contains(AccessFlag flag) const
{
    return contains(AccessRight(flag));
}

bool AccessRights::contains(const AccessRights& rights) const
{
    for (auto& f : rights.rights()) {
        if (!contains(f))
            return false;
    }
    return true;
}

bool AccessRights::contains(const QList<AccessFlag>& flags) const
{
    for (auto f : flags) {
        if (!contains(f))
            return false;
    }
    return true;
}

const AccessRightSet& AccessRights::rights() const
{
    return _rights;
}

QMap<QString, QVariant> AccessRights::additionalRights() const
{
    return _additional_rights;
}

QVariant AccessRights::additional(const QString& key) const
{
    return _additional_rights.value(key);
}

bool AccessRights::contains(const QString& key) const
{
    return _additional_rights.contains(key);
}

AccessRights AccessRights::remove(const QString& key) const
{
    AccessRights res(*this);
    res._additional_rights.remove(key);
    return res;
}

AccessRights AccessRights::merge(const AccessRights& r) const
{
    AccessRights res(*this);
    for (auto& f : r.rights()) {
        res = res.set(f, true);
    }

    // надо добавить в res доп. права, которых нет тут
    for (auto i = r._additional_rights.constBegin(); i != r._additional_rights.constEnd(); ++i) {
        if (!res._additional_rights.contains(i.key()))
            res._additional_rights[i.key()] = i.value();
    }

    return res;
}

AccessRights AccessRights::remove(const AccessRights& r) const
{
    AccessRights res(*this);
    for (auto& f : r.rights()) {
        res = res.set(f, false);
    }

    auto keys = Utils::toSet(_additional_rights.keys()).intersect(Utils::toSet(r._additional_rights.keys()));
    for (auto& key : qAsConst(keys)) {
        res._additional_rights.remove(key);
    }

    return res;
}

AccessRights AccessRights::intersect(const AccessRights& r) const
{
    AccessRightSet inter(_rights);
    inter.intersect(r._rights);

    AccessRights res;
    for (auto& f : inter) {
        res = res.set(f, false);
    }

    auto keys = Utils::toSet(_additional_rights.keys()).intersect(Utils::toSet(r._additional_rights.keys())).toList();
    for (int i = keys.count() - 1; i >= 0; i++) {
        QVariant v = _additional_rights.value(keys.at(i));
        if (Utils::compareVariant(v, r._additional_rights.value(keys.at(i))))
            res._additional_rights[keys.at(i)] = v;
    }

    return res;
}

void AccessRights::check()
{
    for (auto& f : qAsConst(_rights)) {
        Z_CHECK(f.isValid());
    }
}

AccessRightSet AccessRights::onSet(const AccessRight& f)
{
    switch (f.flag()) {
        case AccessFlag::Undefined:
            Z_HALT_INT;
            return {};
        case AccessFlag::Blocked:
            return AccessRightSet {f};
        case AccessFlag::View:
            return AccessRightSet {f};
        case AccessFlag::Modify:
            return onSet(AccessRight(AccessFlag::View)) + AccessRightSet {f};
        case AccessFlag::Create:
            return onSet(AccessRight(AccessFlag::Modify)) + AccessRightSet {f};
        case AccessFlag::Remove:
            return onSet(AccessRight(AccessFlag::View)) + AccessRightSet {f};
        case AccessFlag::Print:
            return onSet(AccessRight(AccessFlag::View)) + AccessRightSet {f};
        case AccessFlag::Export:
            return onSet(AccessRight(AccessFlag::View)) + AccessRightSet {f};
        case AccessFlag::Owner:
            return onSet(AccessRight(AccessFlag::Modify)) + onSet(AccessRight(AccessFlag::Create)) + onSet(AccessRight(AccessFlag::Remove))
                   + onSet(AccessRight(AccessFlag::Print)) + onSet(AccessRight(AccessFlag::Export)) + AccessRightSet {f};
        case AccessFlag::Configurate:
            return onSet(AccessRight(AccessFlag::View)) + AccessRightSet {f};
        case AccessFlag::Administrate:
            return onSet(AccessRight(AccessFlag::Modify)) + onSet(AccessRight(AccessFlag::Create)) + onSet(AccessRight(AccessFlag::Remove))
                   + onSet(AccessRight(AccessFlag::Print)) + onSet(AccessRight(AccessFlag::Export))
                   + onSet(AccessRight(AccessFlag::Configurate)) + onSet(AccessRight(AccessFlag::Owner)) + AccessRightSet {f};
        case AccessFlag::SystemAdministrate:
            return AccessRightSet {f};
        case AccessFlag::Debug:
            return AccessRightSet {f};
        case AccessFlag::Developer:
            return onSet(AccessRight(AccessFlag::Modify)) + onSet(AccessRight(AccessFlag::Create)) + onSet(AccessRight(AccessFlag::Remove))
                   + onSet(AccessRight(AccessFlag::Print)) + onSet(AccessRight(AccessFlag::Export))
                   + onSet(AccessRight(AccessFlag::Configurate)) + onSet(AccessRight(AccessFlag::Owner))
                   + onSet(AccessRight(AccessFlag::SystemAdministrate)) + AccessRightSet {f};

            return AccessRightSet {f};
        case AccessFlag::RegularUser:
            return onSet(AccessRight(AccessFlag::Modify)) + onSet(AccessRight(AccessFlag::Create)) + onSet(AccessRight(AccessFlag::Remove))
                   + onSet(AccessRight(AccessFlag::Print)) + onSet(AccessRight(AccessFlag::Export)) + AccessRightSet {f};
        default:
            return AccessRightSet {f};
    }
}

AccessRightSet AccessRights::onSet(const AccessRightSet& f)
{
    AccessRightSet res;
    for (auto& r : f) {
        res = res.unite(onSet(r));
    }
    return res;
}

AccessRightSet AccessRights::onRemove(const AccessRight& f)
{
    switch (f.flag()) {
        case AccessFlag::Undefined:
            Z_HALT_INT;
            return {};
        case AccessFlag::Blocked:
            return AccessRightSet {f};
        case AccessFlag::View:
            return AccessRightSet {f};
        case AccessFlag::Modify:
            return AccessRightSet {f};
        case AccessFlag::Create:
            return AccessRightSet {f};
        case AccessFlag::Remove:
            return AccessRightSet {f};
        case AccessFlag::Print:
            return AccessRightSet {f};
        case AccessFlag::Export:
            return AccessRightSet {f};
        case AccessFlag::Owner:
            return AccessRightSet {f};
        case AccessFlag::Configurate:
            return AccessRightSet {f};
        case AccessFlag::Administrate:
            return AccessRightSet {f};
        case AccessFlag::SystemAdministrate:
            return AccessRightSet {f};
        case AccessFlag::Debug:
            return AccessRightSet {f};
        case AccessFlag::Developer:
            return AccessRightSet {f};
        case AccessFlag::RegularUser:
            return AccessRightSet {f};
        default:
            return AccessRightSet {f};
    }
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::AccessRight& c)
{
    return s << c._data.num;
}

QDataStream& operator>>(QDataStream& s, zf::AccessRight& c)
{
    return s >> c._data.num;
}

QDataStream& operator<<(QDataStream& s, const zf::AccessRights& c)
{
    return s << c._rights << c._additional_rights;
}

QDataStream& operator>>(QDataStream& s, zf::AccessRights& c)
{
    return s >> c._rights >> c._additional_rights;
}
