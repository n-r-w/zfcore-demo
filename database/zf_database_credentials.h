#pragma once

#include "zf_global.h"

namespace zf
{
class Credentials;
}
ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::Credentials& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::Credentials& c);

namespace zf {

//! Параметры подключения к серверу БД
class ZCORESHARED_EXPORT Credentials
{
public:
    Credentials();
    //! На основании логина и пароль
    Credentials(const QString& login, const QString& password);
    //! На основании хэша логина и пароль
    Credentials(const QString& hash);
    Credentials(const Credentials& c);

    Credentials& operator=(const Credentials& c);

    bool isValid() const;

    //! Логин и пароль не заданы, вместо них хэш
    bool isHashBased() const;

    //! Логин
    QString login() const;
    //! Пароль
    QString password() const;

    //! Хэш логина и пароля
    QString hash() const;

    //! Уникальный код, на основе логина пароля или хэша
    QString uniqueId() const;

private:
    void createId();

    QString _login;
    QByteArray _password;
    QString _hash;
    QString _unique_id;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::Credentials& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::Credentials& c);
};

}

Q_DECLARE_METATYPE(zf::Credentials)
