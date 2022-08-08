#pragma once

#include "zf_core_consts.h"
#include <QDebug>

namespace zf
{
//! Число с фиксированной точкой
class ZCORESHARED_EXPORT Numeric
{
    //! Выгрузка в стрим
    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const Numeric& data);
    //! Загрузка из стрима
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, Numeric& data);

public:
    Numeric();
    Numeric(const Numeric& n);
    Numeric(
        //! Значение
        qint64 value,
        //! Количество знаков после запятой
        quint8 fract_count = Consts::DOUBLE_DECIMALS);

    //! Значение
    qint64 value() const;
    //! Количество знаков после запятой
    quint8 fractCount() const;

    //! Целая часть
    qint64 integer() const;
    //! Дробная часть
    qint64 fractional() const;

    //! Преобразовать в double
    long double toDouble() const;
    //! Преобразовать в строку
    QString toString(
        //! Если не задано, то используется Core::locale(LocaleType::UserInterface)
        const QLocale* locale = nullptr,
        //! Показывать нули после запятой
        bool show_zero_decimal = true,
        //! Разделить число на группы цифр
        bool divide_digits_groups = true) const;
    //! Преобразовать в QVariant
    QVariant toVariant() const;

    operator double() const;
    operator long double() const;

    Numeric& operator=(const Numeric& n);
    bool operator==(const Numeric& n) const;
    bool operator==(double n) const;
    bool operator==(qint64 n) const;
    bool operator==(int n) const;

    bool operator!=(const Numeric& n) const;
    bool operator!=(double n) const;
    bool operator!=(qint64 n) const;
    bool operator!=(int n) const;

    bool operator<(const Numeric& n) const;
    bool operator<=(const Numeric& n) const;
    bool operator<(double n) const;
    bool operator<=(double n) const;
    //! Считаем что n - целая часть
    bool operator<(qint64 n) const;
    bool operator<=(qint64 n) const;
    bool operator<(int n) const;
    bool operator<=(int n) const;

    bool operator>(const Numeric& n) const;
    bool operator>=(const Numeric& n) const;
    bool operator>(double n) const;
    bool operator>=(double n) const;
    //! Считаем что n - целая часть
    bool operator>(qint64 n) const;
    bool operator>=(qint64 n) const;
    bool operator>(int n) const;
    bool operator>=(int n) const;

    Numeric operator-() const;

    Numeric operator+(const Numeric& n) const;
    Numeric operator+(double n) const;
    Numeric& operator++();
    Numeric& operator+=(const Numeric& n);
    Numeric& operator+=(double n);

    Numeric operator-(const Numeric& n) const;
    Numeric operator-(double n) const;
    Numeric& operator--();
    Numeric& operator-=(const Numeric& n);
    Numeric& operator-=(double n);

    Numeric operator*(qint64 n) const;
    Numeric& operator*=(qint64 n);
    Numeric operator*(int n) const;
    Numeric& operator*=(int n);
    Numeric operator*(double n) const;
    Numeric& operator*=(double n);

    Numeric operator/(qint64 n) const;
    Numeric& operator/=(qint64 n);
    Numeric operator/(int n) const;
    Numeric& operator/=(int n);
    Numeric operator/(double n) const;
    Numeric& operator/=(double n);

    static Numeric fromString(const QString& value, bool* ok = nullptr,
        //! Если не задано, то используется Core::locale(LocaleType::UserInterface)
        const QLocale* locale = nullptr, quint8 fract_count = Consts::DOUBLE_DECIMALS, RoundOption options = RoundOption::Undefined);
    static Numeric fromDouble(double value, quint8 fract_count = Consts::DOUBLE_DECIMALS, RoundOption options = RoundOption::Undefined);
    //! Является ли QVariant типом Numeric
    static bool isNumeric(const QVariant& v);
    //! Получить Numeric из QVariant
    static Numeric fromVariant(const QVariant& v);

    //! Тип QVariant
    static int metaType();

private:
    static double toDouble(qint64 int_part, quint8 fract_count);
    static void alignment(const Numeric& n1, const Numeric& n2, qint64& v1, qint64& v2, quint8& max_fract);

    //! Значение
    qint64 _value = 0;
    //! Количество знаков после запятой
    quint8 _fract_count = Consts::DOUBLE_DECIMALS;
    //! Размер групп цифр для деления на классы (Пример: 1 000 000,00)
    int _digits_group_size = Consts::DIGITS_GROUP_SIZE;

    //! Тип QVariant
    static int _meta_type;

    friend class Framework;
};

inline Numeric operator+(double d, const Numeric& n)
{
    return n + d;
}
inline Numeric operator-(double d, const Numeric& n)
{
    return n - d;
}
inline Numeric operator*(qint64 d, const Numeric& n)
{
    return n * d;
}
inline Numeric operator*(int d, const Numeric& n)
{
    return operator*(qint64(d), n);
}

//! Выгрузка в стрим
ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const Numeric& data);
//! Загрузка из стрима
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, Numeric& data);

//! Деньги
class ZCORESHARED_EXPORT Money : public Numeric
{
public:
    Money();
    Money(const Numeric& n);
    Money(const Money& n);
    Money(
        //! Целая часть
        qint64 integer,
        //! Дробная часть
        qint64 fractional);
    Money(
        //! Рубли/Центы
        qint64 value);
};

} // namespace zf

Q_DECLARE_METATYPE(zf::Numeric)

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Numeric& c);
