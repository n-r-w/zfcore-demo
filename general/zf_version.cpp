#include "zf_version.h"
#include "zf_core.h"
#include <QDataStream>

namespace zf
{
static int _ZVersionVersion = 1;
QDataStream& operator<<(QDataStream& out, const Version& data)
{
    // Версия
    out << _ZVersionVersion;
    out << data._major;
    out << data._minor;
    out << data._build;
    return out;
}

QDataStream& operator>>(QDataStream& in, Version& data)
{
    data = Version();

    // Версия
    int version;
    in >> version;
    if (version != _ZVersionVersion) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    in >> data._major;
    in >> data._minor;
    in >> data._build;

    if (in.status() != QDataStream::Ok) {
        data = Version();
        Core::logError("ZVersion: error loading from stream");
    }

    return in;
}

Version::Version()
    : _major(0)
    , _minor(0)
    , _build(0)
{
}

Version::Version(const QString& v)
{
    fromString(v);
}

Version::Version(const char* v)
{
    fromString(QString::fromUtf8(v));
}

Version::Version(const Version& v)
    : _major(v._major)
    , _minor(v._minor)
    , _build(v._build)
{
}

Version::Version(int major, int minor, int build)
    : _major(major <= 0 ? 1 : major)
    , _minor(minor)
    , _build(build)
{
}

bool Version::operator==(const Version& v) const
{
    return v._major == _major && v._minor == _minor && v._build == _build;
}

bool Version::operator!=(const Version& v) const
{
    return !operator==(v);
}

bool Version::operator>(const Version& v) const
{
    if (_major > v._major)
        return true;
    if (_major == v._major && _minor > v._minor)
        return true;
    if (_major == v._major && _minor == v._minor && _build > v._build)
        return true;
    return false;
}

bool Version::operator<(const Version& v) const
{
    return !operator==(v) && !operator>(v);
}

bool Version::operator>=(const Version& v) const
{
    return operator==(v) || operator>(v);
}

bool Version::operator<=(const Version& v) const
{
    return operator==(v) || operator<(v);
}

Version& Version::operator=(const Version& v)
{
    _major = v._major;
    _minor = v._minor;
    _build = v._build;
    return *this;
}

int Version::majorNumber() const
{
    return _major;
}

int Version::minorNumber() const
{
    return _minor;
}

int Version::buildNumber() const
{
    return _build;
}

QString Version::text(int size) const
{
    if (size == 3)
        return QString("%1.%2.%3").arg(_major).arg(_minor).arg(_build);
    else if (size == 2)
        return QString("%1.%2").arg(_major).arg(_minor);
    else if (size == 1)
        return QString("%1").arg(_major);
    else
        Z_HALT_INT;

    return {};
}

void Version::fromString(const QString& v)
{
    _major = 0;
    _minor = 0;
    _build = 0;
    QStringList l = v.simplified().split(".", QString::KeepEmptyParts);
    if (l.count() > 2)
        _build = l.at(2).toInt();
    if (l.count() > 1)
        _minor = l.at(1).toInt();
    if (l.count() > 0)
        _major = l.at(0).toInt();
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::Version& c)
{
    QDebugStateSaver saver(debug);
    debug.noquote().nospace();
    debug << c.text();

    return debug;
}
