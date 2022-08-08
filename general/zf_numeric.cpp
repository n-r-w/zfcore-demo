#include "zf_numeric.h"
#include "zf_core.h"
#include "zf_utils.h"

#include <QtGlobal>

namespace zf
{
int Numeric::_meta_type = 0;

template <class T> int numDigits(T number)
{
    int digits = 0;
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}

Numeric::Numeric()
{
}

Numeric::Numeric(const Numeric& n)
    : _value(n._value)
    , _fract_count(n._fract_count)
{
}

Numeric::Numeric(qint64 value, quint8 fract_count)
    : _value(value)
    , _fract_count(fract_count)
{
}

qint64 Numeric::value() const
{
    return _value;
}

quint8 Numeric::fractCount() const
{
    return _fract_count;
}

long double Numeric::toDouble() const
{
    return _fract_count == 0 ? static_cast<long double>(_value) : static_cast<long double>(_value) / static_cast<long double>(pow(10, _fract_count));
}

QString Numeric::toString(const QLocale* locale, bool show_zero_decimal, bool divide_digits_groups) const
{
    if (_value == 0)
        return QStringLiteral("0");

    if (locale == nullptr)
        locale = Core::locale(LocaleType::UserInterface);

    QString v = QString::number(qAbs(_value));
    if (_fract_count > 0) {
        int pos = v.length() - _fract_count;
        if (pos < 0) {
            v = QString(-pos, '0') + v;
            pos = 0;
        }

        long double check = static_cast<long double>(qAbs(_value)) / static_cast<long double>(pow(10, _fract_count));
        if (fuzzyCompare(check, static_cast<long double>(static_cast<qint64>(check)))) {
            // после запятой одни нули
            v.remove(pos, v.length() - pos);

        } else {
            v.insert(pos, locale->decimalPoint());
            if (pos == 0)
                v = QStringLiteral("0") + v;

            if (pos >= 0) {
                // убираем лишние нули после запятой
                for (int pos = v.length() - 1; pos >= 0; pos--) {
                    if (v.at(pos) == '0')
                        continue;
                    v.remove(pos + 1, v.length() - pos);
                    break;
                }
            }
        }
    }

    if (divide_digits_groups) {
        int pos = v.indexOf(locale->decimalPoint());
        if (pos == -1)
            pos = v.size();
        for (int i = pos - _digits_group_size; i > 0; i -= _digits_group_size) {
            v.insert(i, ' ');
        }
    }

    if (_value < 0)
        v = QStringLiteral("-") + v;

    if (show_zero_decimal) {
        if (v.isEmpty()) {
            v = '0' + locale->decimalPoint() + QString(_fract_count, '0');

        } else {
            int pos = v.indexOf(locale->decimalPoint());
            if (pos < 0) {
                pos = v.length();
                v += locale->decimalPoint();
            }
            int f_add = 3 - (v.length() - pos);
            Z_CHECK(f_add >= 0);
            v += QString(f_add, '0');
        }
    }

    return v;
}

QVariant Numeric::toVariant() const
{
    return QVariant::fromValue(*this);
}

zf::Numeric::operator long double() const
{
    if (_value == 0 || _fract_count == 0)
        return _value;

    return static_cast<long double>(_value) / static_cast<long double>(pow(10, _fract_count));
}

zf::Numeric::operator double() const
{
    return (double)operator long double();
}

Numeric& Numeric::operator=(const Numeric& n)
{
    _value = n._value;
    _fract_count = n._fract_count;
    return *this;
}

bool Numeric::operator==(int n) const
{
    return operator==(qint64(n));
}

bool Numeric::operator!=(int n) const
{
    return operator!=(qint64(n));
}

bool Numeric::operator<(int n) const
{
    return operator<(qint64(n));
}

bool Numeric::operator<=(int n) const
{
    return operator<=(qint64(n));
}

bool Numeric::operator>(int n) const
{
    return operator>(qint64(n));
}

bool Numeric::operator>=(int n) const
{
    return operator>=(qint64(n));
}

Numeric Numeric::operator+(const Numeric& n) const
{
    if (_fract_count == n._fract_count)
        return Numeric(_value + n._value, _fract_count);

    quint8 max_fract;
    qint64 v1, v2;
    alignment(*this, n, v1, v2, max_fract);

    return Numeric(v1 + v2, max_fract);
}

Numeric Numeric::operator+(double n) const
{
    if (_fract_count == 0)
        return Numeric(_value + static_cast<qint64>(n), _fract_count);

    return Numeric(_value + static_cast<qint64>(n * pow(10, _fract_count)), _fract_count);
}

Numeric& Numeric::operator++()
{
    _value += pow(10, _fract_count);
    return *this;
}

Numeric& Numeric::operator+=(const Numeric& n)
{
    *this = operator+(n);
    return *this;
}

Numeric& Numeric::operator+=(double n)
{
    *this = operator+(n);
    return *this;
}

Numeric Numeric::operator-(const Numeric& n) const
{
    return operator+(-n);
}

Numeric Numeric::operator-(double n) const
{
    return operator+(-n);
}

Numeric& Numeric::operator--()
{
    _value -= pow(10, _fract_count);
    return *this;
}

Numeric& Numeric::operator-=(const Numeric& n)
{
    *this = operator-(n);
    return *this;
}

Numeric Numeric::operator*(int n) const
{
    return operator*(qint64(n));
}

Numeric& Numeric::operator*=(int n)
{
    return operator*=(qint64(n));
}

Numeric Numeric::operator/(int n) const
{
    return operator/(qint64(n));
}

Numeric& Numeric::operator/=(int n)
{
    return operator/=(qint64(n));
}

Numeric Numeric::operator*(double n) const
{
    return Numeric(zf::round((double)_value * n, RoundOption::Nearest), _fract_count);
}

Numeric& Numeric::operator*=(double n)
{
    *this = operator*(n);
    return *this;
}

Numeric Numeric::operator/(double n) const
{
    Z_CHECK(!zf::fuzzyIsNull(n));
    return Numeric(zf::round((double)_value / n, RoundOption::Nearest), _fract_count);
}

Numeric& Numeric::operator/=(double n)
{
    *this = operator/(n);
    return *this;
}

Numeric& Numeric::operator-=(double n)
{
    *this = operator-(n);
    return *this;
}

Numeric Numeric::operator*(qint64 n) const
{
    return Numeric(_value * n, _fract_count);
}

Numeric& Numeric::operator*=(qint64 n)
{
    *this = operator*(n);
    return *this;
}

Numeric Numeric::operator/(qint64 n) const
{
    Z_CHECK(n != 0);
    return Numeric(_value / n, _fract_count);
}

Numeric& Numeric::operator/=(qint64 n)
{
    _value = _value / n;
    return *this;
}

bool Numeric::operator==(const Numeric& n) const
{
    if (n._fract_count == _fract_count)
        return n._value == _value;

    quint8 max_fract;
    qint64 v1, v2;
    alignment(*this, n, v1, v2, max_fract);

    return v1 == v2;
}

bool Numeric::operator==(double n) const
{
    return fuzzyCompare(toDouble(), (long double)n);
}

bool Numeric::operator==(qint64 n) const
{
    return operator==((double)n);
}

bool Numeric::operator!=(const Numeric& n) const
{
    return !operator==(n);
}

bool Numeric::operator!=(double n) const
{
    return !operator==(n);
}

bool Numeric::operator!=(qint64 n) const
{
    return !operator==(n);
}

bool Numeric::operator<(qint64 n) const
{
    return operator<(double(n));
}

bool Numeric::operator<=(qint64 n) const
{
    return operator<=(double(n));
}

bool Numeric::operator>(const Numeric& n) const
{
    return !operator<=(n);
}

bool Numeric::operator>=(const Numeric& n) const
{
    return !operator<(n);
}

bool Numeric::operator>(double n) const
{
    return !operator<=(n);
}

bool Numeric::operator>=(double n) const
{
    return !operator<(n);
}

bool Numeric::operator>(qint64 n) const
{
    return !operator<=(n);
}

bool Numeric::operator>=(qint64 n) const
{
    return !operator<(n);
}

bool Numeric::operator<(const Numeric& n) const
{
    if (n._fract_count == _fract_count)
        return _value < n._value;

    quint8 max_fract;
    qint64 v1, v2;
    alignment(*this, n, v1, v2, max_fract);

    return v1 < v2;
}

bool Numeric::operator<=(const Numeric& n) const
{
    if (operator==(n))
        return true;
    return operator<(n);
}

bool Numeric::operator<(double n) const
{
    return toDouble() < n;
}

bool Numeric::operator<=(double n) const
{
    if (operator==(n))
        return true;
    return operator<(n);
}

Numeric Numeric::operator-() const
{
    return Numeric(-_value, _fract_count);
}

Numeric Numeric::fromString(const QString& value, bool* ok, const QLocale* locale, quint8 fract_count, RoundOption options)
{
    if (ok != nullptr)
        *ok = true;

    if (value.isEmpty())
        return Numeric(0, fract_count);

    if (locale == nullptr)
        locale = Core::locale(LocaleType::UserInterface);

    QString v = value.trimmed();
    v.replace(' ', QString());
    v.replace(locale->groupSeparator(), QString());

    if (!v.isEmpty() && v.at(0) == '+')
        v.remove(0, 1);

    bool sign = false;
    if (!v.isEmpty() && v.at(0) == '-') {
        sign = true;
        v.remove(0, 1);
    }

    if (v.isEmpty())
        return Numeric(0, fract_count);

    QString int_part;
    QString fract_part;

    int pos = v.indexOf(locale->decimalPoint());
    if (pos >= 0) {
        int_part = v.left(pos);
        // Если требуется округление, то оставляем в дробной части на один знак больше fract_count для последующего округления
        fract_part
            = (options != RoundOption::Undefined) ? v.right(v.length() - pos - 1).left(fract_count + 1) : v.right(v.length() - pos - 1).left(fract_count);

    } else {
        int_part = v;
    }

    bool b;
    qint64 int_v = locale->toLong(int_part, &b);
    if (!b) {
        if (ok != nullptr)
            *ok = false;
        return Numeric(0, fract_count);
    }

    qint64 fract_v = 0;
    // Если требуется округление, то оставляем в дробной части на один знак больше fract_count для последующего округления
    QStringRef fract_str = (options != RoundOption::Undefined) ? fract_part.leftRef(fract_count + 1) : fract_part.leftRef(fract_count);

    if (!fract_str.isEmpty()) {
        fract_v = locale->toLong(fract_str, &b);
        if (!b) {
            if (ok != nullptr)
                *ok = false;
            return Numeric(0, fract_count);
        }

        if (fract_part.length() < fract_count)
            fract_v *= pow(10, fract_count - fract_part.length());
    }

    // при хранении int64 с учетом дробной части может быть переполнение
    qint64 mult = pow(10, fract_count);
    // Если требуется округление
    if (fract_str.count() > fract_count && options != RoundOption::Undefined)
        fract_v = static_cast<qint64>(roundPrecision(fract_v / 10.0, 0, options));
    qint64 target = mult * int_v + fract_v;
    if (mult != 0 && ((target - fract_v) / mult) != int_v) {
        // переполнение
        if (ok != nullptr)
            *ok = false;
        return Numeric(0, fract_count);
    }

    return Numeric((sign ? -1 : 1) * target, fract_count);
}

Numeric Numeric::fromDouble(double value, quint8 fract_count, RoundOption options)
{
    if (options != RoundOption::Undefined)
        value = roundPrecision(value, fract_count, options);
    double int_part;
    double fract_part;
    double mult = pow(10, fract_count);
    fract_part = modf(value, &int_part) * mult;

    if (fract_count == 0) //-V1051
        return Numeric(static_cast<qint64>(int_part), fract_count);

    fract_part = roundPrecision(fract_part, 0, RoundOption::Nearest);
    return Numeric(static_cast<qint64>(int_part * mult + fract_part), fract_count);
}

bool Numeric::isNumeric(const QVariant& v)
{
    Z_CHECK(_meta_type > 0);
    return v.userType() == _meta_type;
}

Numeric Numeric::fromVariant(const QVariant& v)
{
    if (isNumeric(v))
        return v.value<Numeric>();

    switch (v.type()) {
        case QVariant::String:
            return fromString(v.toString());
        case QVariant::Double:
            return fromDouble(v.toDouble());
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
            return Numeric(v.toLongLong());
        case QVariant::ULongLong:
            return Numeric(v.toULongLong());

        default:
            return Numeric();
    }
}

int Numeric::metaType()
{
    return _meta_type;
}

qint64 Numeric::integer() const
{
    return _fract_count == 0 ? value() : value() / pow(10, _fract_count);
}

qint64 Numeric::fractional() const
{
    if (_fract_count == 0)
        return 0;

    int mult = pow(10, _fract_count);
    return value() - (int)(value() / mult) * mult;
}

void Numeric::alignment(const Numeric& n1, const Numeric& n2, qint64& v1, qint64& v2, quint8& max_fract)
{
    max_fract = qMax(n1._fract_count, n2._fract_count);
    v1 = max_fract > n1._fract_count ? n1._value * pow(10, max_fract - n1._fract_count) : n1._value;
    v2 = max_fract > n2._fract_count ? n2._value * pow(10, max_fract - n2._fract_count) : n2._value;
}

QDataStream& operator<<(QDataStream& out, const Numeric& data)
{
    return out << data._value << data._fract_count;
}

QDataStream& operator>>(QDataStream& in, Numeric& data)
{
    return in >> data._value >> data._fract_count;
}

Money::Money()
    : Numeric()
{
}

Money::Money(const Numeric& n)
    : Numeric(n)
{
    Z_CHECK(fractCount() == 2);
}

Money::Money(const Money& n)
    : Numeric(n)
{
}

Money::Money(qint64 integer, qint64 fractional)
    : Numeric(integer * 100 + fractional, 2)
{
}

Money::Money(qint64 value)
    : Numeric(value, 2)
{
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::Numeric& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << c.toString(zf::Core::locale(zf::LocaleType::Universal));

    return debug;
}
