#pragma once

#include "zf_global.h"

namespace zf
{
//! Информация о версии
class ZCORESHARED_EXPORT Version
{
    //! Выгрузка в стрим
    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const Version& data);
    //! Загрузка из стрима
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, Version& data);

public:
    Version();
    //! Из строки типа "1.0.0"
    Version(const QString& v);
    //! Из строки типа "1.0.0"
    Version(const char* v);

    Version(const Version& v);
    Version(int majorNumber, int minorNumber = 0, int buildNumber = 0);

    bool operator==(const Version& v) const;
    bool operator!=(const Version& v) const;
    bool operator>(const Version& v) const;
    bool operator<(const Version& v) const;
    bool operator>=(const Version& v) const;
    bool operator<=(const Version& v) const;
    Version& operator=(const Version& v);

    int majorNumber() const;
    int minorNumber() const;
    int buildNumber() const;
    QString text(
        //! Размерность 1-3
        int size = 3) const;

private:
    void fromString(const QString& v);

    int _major;
    int _minor;
    int _build;
};

//! Выгрузка в стрим
ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const Version& data);
//! Загрузка из стрима
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, Version& data);

} // namespace zf

Q_DECLARE_METATYPE(zf::Version)
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Version& c);
