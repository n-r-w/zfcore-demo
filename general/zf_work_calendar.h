#pragma once

#include "zf.h"
#include <QSharedDataPointer>

namespace zf
{
class WorkCalendar_data;

//! Производственный календарь
class ZCORESHARED_EXPORT WorkCalendar
{
public:
    //! Тип дня
    enum class WorkType
    {
        Undefined = 0,
        //! Рабочий день
        WorkDay = 1,
        //! Сокращенный рабочий день
        ReducedWorkDay = 2,
        //! Выходной день
        Weekend = 3,
        //! Праздничный день
        Holiday = 4,
        //! Перенесенный выходной день
        MovedWeekend = 5,
    };

    //! День недели
    enum class DayOfWeek
    {
        Undefined = 0,
        //! Понедельник
        Monday = 1,
        //! Вторник
        Tuesday = 2,
        //! Среда
        Wednesday = 3,
        //! Четверг
        Thursday = 4,
        //! Пятница
        Friday = 5,
        //! Суббота
        Saturday = 6,
        //! Воскресенье
        Sunday = 7,
    };

    //! Не валидный календарь
    WorkCalendar();
    //! Инициализирован выходными по умолчанию
    WorkCalendar(
        //! Год
        int year,
        //! Страна
        QLocale::Country country = QLocale::AnyCountry,
        //! За предыдущий год
        const WorkCalendar& previous = {},
        //! На следующий год
        const WorkCalendar& next = {});
    WorkCalendar(const WorkCalendar& c);
    ~WorkCalendar();

    WorkCalendar& operator=(const WorkCalendar& c);

    bool isValid() const;
    void clear();

    //! Год
    int year() const;
    //! Страна
    QLocale::Country country() const;

    //! Количество дней в месяце. Если неверный месяц или календарь не инициализирован - 0
    int daysInMonth(int month) const;
    int daysInMonth(int year, int month) const;

    //! Количество дней в году
    int daysInYear() const;
    //! Количество дней в году для произвольного года
    static int daysInYear(int year);

    //! День с начала года. Если ошибка - 0
    int dayOfYear(int month, int day_in_month) const;
    //! День с начала года для произвольного года. Если ошибка - 0
    static int dayOfYear(int year, int month, int day_in_month);

    //! День недели
    DayOfWeek dayOfWeek(
        //! День с начала года (начало с единицы)
        int day) const;
    //! День недели
    DayOfWeek dayOfWeek(int month, int day_in_month) const;
    //! День недели
    static DayOfWeek dayOfWeek(const QDate& date);

    //! Тип дня. Undefined - неверный день
    WorkType dayType(
        //! День с начала года (начало с единицы)
        int day) const;
    //! Тип дня. Undefined - неверный день
    WorkType dayType(int month, int day_in_month) const;
    //! Тип дня
    WorkType dayType(const QDate& date) const;

    //! Задать тип дня (изменить данные по умолчанию). Возвращает истину если день корректный
    bool setDayType(int day, WorkType type);
    //! Задать тип дня (изменить данные по умолчанию). Возвращает истину если день корректный
    bool setDayType(int month, int day_in_month, WorkType type);
    //! Задать тип дня (изменить данные по умолчанию). Возвращает истину если день корректный
    bool setDayType(const QDate& date, WorkType type);

    //! Дата из дня с начала года (начало с единицы)
    QDate dateFromDay(int day) const;
    //! Дата из месяца и дня в месяце
    QDate dateFromDay(int month, int day_in_month) const;

    //! Следующая дата по типу. Сама date не учитывается
    QDate nextDateByTypes(const QDate& date,
                          //! Какие типы дней нас интересуют
                          const QList<WorkType>& types,
                          //! Если 1, то следующая подходящая. 2 - через одну и т.д. Отрицательные значение - поиск в прошлом
                          int shift) const;
    //! Следующая дата по типу. Сама date не учитывается
    QDate nextDateByTypes(const QDate& date,
                          //! Какие типы дней нас интересуют
                          WorkType type,
                          //! Если 1, то следующая подходящая. 2 - через одну и т.д. Отрицательные значение - поиск в прошлом
                          int shift) const;

    //! Следующая дата по дню недели. Сама date не учитывается
    QDate nextDateByDaysOfWeek(const QDate& date,
                               //! Дни недели
                               const QList<DayOfWeek>& days_of_week,
                               //! На сколько дней вперед
                               int shift) const;
    //! Следующая дата по дню недели. Сама date не учитывается
    QDate nextDateByDaysOfWeek(const QDate& date,
                               //! День недели (1-понедельник, 7-воскресенье)
                               DayOfWeek day_of_week,
                               //! На сколько дней вперед
                               int shift) const;

    //! Календарь за предыдущий год
    WorkCalendar previousCalendar() const;
    //! Задать календарь за предыдущий год
    void setPreviousCalendar(const WorkCalendar& c);

    //! Календарь за следующий год
    WorkCalendar nextCalendar() const;
    //! Задать календарь за следующий год
    void setNextCalendar(const WorkCalendar& c);

    //! Найти подходяший календарь по году. Ищет в цепочке nextCalendar/previousCalendar
    WorkCalendar findCalendar(int year) const;

private:
    //! Следующая дата по типу. Сама date не учитывается
    QDate nextDate_helper(const QDate& date,
                          //! Какие типы дней нас интересуют
                          const QList<WorkType>& types,
                          //! Дни недели (1-понедельник, 7-воскресенье)
                          const QList<DayOfWeek>& days_of_week,
                          //! Если 1, то следующая подходящая. 2 - через одну и т.д. Отрицательные значение - поиск в прошлом
                          int shift) const;

    //! Данные
    QSharedDataPointer<WorkCalendar_data> _d;
};

} // namespace zf
