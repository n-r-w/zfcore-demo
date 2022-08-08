#include "zf_work_calendar.h"
#include "zf_core.h"
#include <QCalendar>

namespace zf
{
//! Разделяемые данные для WorkCalendar
class WorkCalendar_data : public QSharedData
{
public:
    WorkCalendar_data();
    WorkCalendar_data(int year, QLocale::Country _country, const WorkCalendar& _previous, const WorkCalendar& _next);
    WorkCalendar_data(const WorkCalendar_data& d);
    ~WorkCalendar_data();

    void copyFrom(const WorkCalendar_data* d);
    void clear();

    int year = 0;
    QLocale::Country country = QLocale::AnyCountry;
    QMap<int, WorkCalendar::WorkType> data;
    QCalendar calendar; // создаем сразу чтобы потом не тратить время на создание

    std::unique_ptr<WorkCalendar> previous;
    std::unique_ptr<WorkCalendar> next;

    mutable QRecursiveMutex mutex;
};

WorkCalendar_data::WorkCalendar_data()
{
}

WorkCalendar_data::WorkCalendar_data(int _year, QLocale::Country _country, const WorkCalendar& _previous, const WorkCalendar& _next)
    : year(_year)
    , country(_country)
{
    Z_CHECK(year > 0);

    if (_previous.isValid()) {
        Z_CHECK(_previous.year() == year - 1);
        previous = std::make_unique<WorkCalendar>(_previous);
    }

    if (_next.isValid()) {
        Z_CHECK(_next.year() == year + 1);
        next = std::make_unique<WorkCalendar>(_next);
    }

    calendar = QCalendar(QCalendar::System::Gregorian);
}

WorkCalendar_data::WorkCalendar_data(const WorkCalendar_data& d)
    : QSharedData(d)
{
    copyFrom(&d);
}

WorkCalendar_data::~WorkCalendar_data()
{
}

void WorkCalendar_data::copyFrom(const WorkCalendar_data* d)
{
    QMutexLocker lock_self(&mutex);
    QMutexLocker lock_source(&d->mutex);

    year = d->year;
    country = d->country;
    data = d->data;
    previous.reset();
    next.reset();

    if (d->previous != nullptr)
        previous = std::make_unique<WorkCalendar>(*d->previous);
    if (d->next != nullptr)
        next = std::make_unique<WorkCalendar>(*d->next);
}

void WorkCalendar_data::clear()
{
    QMutexLocker lock(&mutex);

    year = 0;
    data.clear();
    previous.reset();
    next.reset();
}

WorkCalendar::WorkCalendar()
    : _d(new WorkCalendar_data())
{
}

WorkCalendar::WorkCalendar(int year, QLocale::Country country, const WorkCalendar& previous, const WorkCalendar& next)
    : _d(new WorkCalendar_data(year, country, previous, next))
{
}

WorkCalendar::WorkCalendar(const WorkCalendar& c)
    : _d(c._d)
{
}

WorkCalendar::~WorkCalendar()
{
}

WorkCalendar& WorkCalendar::operator=(const WorkCalendar& c)
{
    if (this != &c)
        _d = c._d;
    return *this;
}

bool WorkCalendar::isValid() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->year > 0;
}

void WorkCalendar::clear()
{
    _d->clear();
}

int WorkCalendar::year() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->year;
}

QLocale::Country WorkCalendar::country() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->country;
}

int WorkCalendar::daysInMonth(int month) const
{
    QMutexLocker lock(&_d->mutex);
    return _d->calendar.daysInMonth(month, _d->year);
}

int WorkCalendar::daysInMonth(int year, int month) const
{
    QMutexLocker lock(&_d->mutex);
    return findCalendar(year).daysInMonth(month);
}

int WorkCalendar::daysInYear() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->calendar.daysInYear(_d->year);
}

int WorkCalendar::daysInYear(int year)
{
    return QCalendar().daysInYear(year);
}

int WorkCalendar::dayOfYear(int month, int day_in_month) const
{
    auto date = dateFromDay(month, day_in_month);
    if (!date.isValid())
        return 0;

    return date.dayOfYear();
}

int WorkCalendar::dayOfYear(int year, int month, int day_in_month)
{
    return QDate(year, month, day_in_month).dayOfYear();
}

WorkCalendar::DayOfWeek WorkCalendar::dayOfWeek(int day) const
{
    QMutexLocker lock(&_d->mutex);

    auto date = dateFromDay(day);
    if (!date.isValid())
        return DayOfWeek::Undefined;

    return static_cast<DayOfWeek>(_d->calendar.dayOfWeek(date));
}

WorkCalendar::DayOfWeek WorkCalendar::dayOfWeek(int month, int day_in_month) const
{
    return dayOfWeek(dayOfYear(month, day_in_month));
}

WorkCalendar::DayOfWeek WorkCalendar::dayOfWeek(const QDate& date)
{
    return static_cast<DayOfWeek>(date.dayOfWeek());
}

WorkCalendar::WorkType WorkCalendar::dayType(int day) const
{
    QMutexLocker lock(&_d->mutex);

    if (day <= 0 || day > daysInYear())
        return WorkType::Undefined;

    auto info = _d->data.constFind(day);
    if (info == _d->data.constEnd()) {
        DayOfWeek dw = dayOfWeek(day);
        if (dw == DayOfWeek::Saturday || dw == DayOfWeek::Sunday)
            return WorkType::Weekend;
        return WorkType::WorkDay;
    }

    return info.value();
}

WorkCalendar::WorkType WorkCalendar::dayType(int month, int day_in_month) const
{
    return dayType(dayOfYear(month, day_in_month));
}

WorkCalendar::WorkType WorkCalendar::dayType(const QDate& date) const
{
    Z_CHECK(date.isValid());
    QMutexLocker lock(&_d->mutex);
    return findCalendar(date.year()).dayType(date.dayOfYear());
}

bool WorkCalendar::setDayType(int day, WorkType type)
{
    QMutexLocker lock(&_d->mutex);
    Z_CHECK(isValid());
    Z_CHECK(type != WorkType::Undefined);
    if (day <= 0 || day >= daysInYear())
        return false;

    if (dayType(day) == type)
        return true;

    _d->data[day] = type;
    return true;
}

bool WorkCalendar::setDayType(int month, int day_in_month, WorkType type)
{
    return setDayType(dayOfYear(month, day_in_month), type);
}

bool WorkCalendar::setDayType(const QDate& date, WorkType type)
{
    QMutexLocker lock(&_d->mutex);
    Z_CHECK(date.isValid());
    if (date.year() != year())
        return false;

    return setDayType(date.dayOfYear(), type);
}

QDate WorkCalendar::dateFromDay(int day) const
{
    QMutexLocker lock(&_d->mutex);
    if (day <= 0 || day > _d->calendar.daysInYear(_d->year))
        return QDate();

    return QDate(_d->year, 1, 1).addDays(day - 1);
}

QDate WorkCalendar::dateFromDay(int month, int day_in_month) const
{
    QMutexLocker lock(&_d->mutex);
    if (month <= 0 || month > 12 || day_in_month <= 0 || day_in_month > _d->calendar.daysInMonth(month, _d->year))
        return QDate();

    return QDate(_d->year, month, day_in_month);
}

QDate WorkCalendar::nextDateByTypes(const QDate& date, const QList<WorkType>& types, int shift) const
{
    QMutexLocker lock(&_d->mutex);
    Z_CHECK(date.isValid());
    Z_CHECK(!types.isEmpty());

    return nextDate_helper(date, types, {}, shift);
}

QDate WorkCalendar::nextDateByTypes(const QDate& date, WorkType type, int shift) const
{       
    return nextDateByTypes(date, QList<WorkType> {type}, shift);
}

QDate WorkCalendar::nextDateByDaysOfWeek(const QDate& date, const QList<DayOfWeek>& days_of_week, int shift) const
{
    QMutexLocker lock(&_d->mutex);
    Z_CHECK(date.isValid());
    Z_CHECK(!days_of_week.isEmpty());

    return nextDate_helper(date, {}, days_of_week, shift);
}

QDate WorkCalendar::nextDateByDaysOfWeek(const QDate& date, DayOfWeek day_of_week, int shift) const
{
    return nextDateByDaysOfWeek(date, QList<DayOfWeek> {day_of_week}, shift);
}

WorkCalendar WorkCalendar::previousCalendar() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->previous == nullptr ? WorkCalendar() : *_d->previous;
}

void WorkCalendar::setPreviousCalendar(const WorkCalendar& c)
{
    QMutexLocker lock(&_d->mutex);
    if (c.isValid())
        Z_CHECK(c.year() == year() - 1);

    if (c.isValid())
        _d->previous = std::make_unique<WorkCalendar>(c);
    else
        _d->previous.reset();
}

WorkCalendar WorkCalendar::nextCalendar() const
{
    QMutexLocker lock(&_d->mutex);
    return _d->next == nullptr ? WorkCalendar() : *_d->next;
}

void WorkCalendar::setNextCalendar(const WorkCalendar& c)
{
    QMutexLocker lock(&_d->mutex);
    if (c.isValid())
        Z_CHECK(c.year() == year() + 1);

    if (c.isValid())
        _d->next = std::make_unique<WorkCalendar>(c);
    else
        _d->next.reset();
}

WorkCalendar WorkCalendar::findCalendar(int year) const
{
    QMutexLocker lock(&_d->mutex);
    if (year <= 0)
        return WorkCalendar();

    if (year == _d->year)
        return *this;

    if (year > _d->year) {
        if (nextCalendar().isValid())
            return nextCalendar().findCalendar(year);
        return WorkCalendar();
    }

    if (previousCalendar().isValid())
        return previousCalendar().findCalendar(year);

    return WorkCalendar();
}

QDate WorkCalendar::nextDate_helper(const QDate& date, const QList<WorkType>& types, const QList<DayOfWeek>& days_of_week, int shift) const
{
    WorkCalendar c;

    if (!days_of_week.isEmpty() && date.year() != year())
        c = WorkCalendar(date.year()); // для дней недели нам не нужены доп. данные
    else
        c = findCalendar(date.year());

    if (!c.isValid())
        return QDate();

    int sign = shift >= 0 ? +1 : -1;
    int days_in_year = c.daysInYear();
    int day = date.dayOfYear() + sign;

    while (true) {
        if (sign > 0 && day >= days_in_year)
            break;
        if (sign < 0 && day <= 0)
            break;

        auto type = c.dayType(day);
        DayOfWeek day_of_week = c.dayOfWeek(day);

        if (types.contains(type) || days_of_week.contains(day_of_week)) {
            if (shift == sign)
                return c.dateFromDay(day);
            shift -= sign;
        }

        day += sign;
    }

    if (sign > 0) {
        auto next = c.nextCalendar();
        // дошли до конца года и ничего не нашли
        if (!next.isValid()) {
            if (!days_of_week.isEmpty())
                next = WorkCalendar(c.year() + 1); // для дней недели нам не нужены доп. данные
            else
                return QDate(); // увы - календари кончились
        }

        auto next_date = QDate(next.year(), 1, 1);
        if (types.contains(next.dayType(next_date)) || days_of_week.contains(next.dayOfWeek(next_date))) {
            if (shift == 1)
                return next_date;
            shift -= sign;
        }

        return next.nextDate_helper(next_date, types, days_of_week, shift);
    }

    auto prev = c.previousCalendar();
    // дошли до начала года и ничего не нашли
    if (!prev.isValid()) {
        if (!days_of_week.isEmpty())
            prev = WorkCalendar(c.year() - 1); // для дней недели нам не нужены доп. данные
        else
            return QDate(); // увы - календари кончились
    }

    auto next_date = QDate(c.year(), 1, 1).addDays(-1);
    if (types.contains(prev.dayType(next_date)) || days_of_week.contains(prev.dayOfWeek(next_date))) {
        if (shift == -1)
            return next_date;
        shift -= sign;
    }

    return prev.nextDate_helper(next_date, types, days_of_week, shift);
}

} // namespace zf
