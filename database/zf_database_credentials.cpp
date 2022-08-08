#include "zf_database_credentials.h"
#include "zf_core.h"
#include <QCryptographicHash>

namespace zf
{
Credentials::Credentials()
{
}

Credentials::Credentials(const QString& user, const QString& password)
    : _login(user.trimmed())    
{
    _password = qCompress(password.toUtf8(), 9);

    Z_CHECK(!_login.isEmpty());
    createId();
}

Credentials::Credentials(const QString& hash)
    : _hash(hash)
{
    Z_CHECK(!hash.isEmpty());
    createId();
}

Credentials::Credentials(const Credentials& c)
    : _login(c._login)
    , _password(c._password)
    , _hash(c._hash)
    , _unique_id(c._unique_id)
{
}

Credentials& Credentials::operator=(const Credentials& c)
{
    _login = c._login;
    _password = c._password;
    _hash = c._hash;
    _unique_id = c._unique_id;
    return *this;
}

bool Credentials::isValid() const
{
    return !_login.isEmpty() || !_hash.isEmpty();
}

bool Credentials::isHashBased() const
{
    return !_hash.isEmpty();
}

QString Credentials::login() const
{
    return _login;
}

QString Credentials::password() const
{
    return _password.isEmpty() ? QString() : QString::fromUtf8(qUncompress(_password));
}

QString Credentials::hash() const
{
    return _hash;
}

QString Credentials::uniqueId() const
{
    return _unique_id;
}

void Credentials::createId()
{
    _unique_id = _login + Consts::KEY_SEPARATOR + _password + Consts::KEY_SEPARATOR + _hash;
}
} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::Credentials& c)
{
    return s << c._login << c._password << c._hash << c._unique_id;
}

QDataStream& operator>>(QDataStream& s, zf::Credentials& c)
{
    return s >> c._login >> c._password >> c._hash >> c._unique_id;
}
