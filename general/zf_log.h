#ifndef ZF_LOG_H
#define ZF_LOG_H

#include "zf_global.h"
#include "zf.h"
#include "zf_error.h"
#include <QString>
#include <QVariant>
#include <QSharedDataPointer>

class QIODevice;

namespace zf
{
class LogRecord_data;

//! Запись в журнале
class ZCORESHARED_EXPORT LogRecord
{
public:
    LogRecord();
    LogRecord(const LogRecord& record);
    LogRecord(int num, InformationType type, const QString& info, const QString& detail = QString());
    ~LogRecord();
    LogRecord& operator=(const LogRecord& r);

    bool isValid() const;
    //! Порядковый номер
    int num() const;
    //! Тип
    InformationType type() const;
    QString typeText() const;
    //! Информация
    QString info() const;
    //! Детальная информация
    QString detail() const;

private:
    //! Данные
    QSharedDataPointer<LogRecord_data> _d;
};

typedef QList<LogRecord> LogRecordList;

class Log_data;

//! Информация о каком либо действии, состоящем из набора операций
class ZCORESHARED_EXPORT Log : public QObject
{
    Q_OBJECT
public:
    Log();
    Log(const QString& name, const QString& detail = QString());
    Log(const Log& log);
    ~Log();

    //! Добавить запись в лог
    Log& add(
            //! Тип записи
            InformationType type,
            //! Основной текст
            const QString& info,
            //! Дополнительный текст
            const QString& detail = QString());
    Log& add(
            //! Ошибка
            const QString& error,
            //! Тип записи
            InformationType type = InformationType::Error);
    Log& add(
            //! Ошибка
            const QStringList& error,
            //! Тип записи
            InformationType type = InformationType::Error);
    Log& add(
            //! Ошибка
            const char* error,
            //! Тип записи
            InformationType type = InformationType::Error);
    Log& add(
            //! Ошибка
            const Error& error,
            //! Тип записи
            InformationType type = InformationType::Error);
    Log& add(
            //! Ошибка
            const ErrorList& error,
            //! Тип записи
            InformationType type = InformationType::Error);
    Log& add(const LogRecord& r);

    Log& add(const LogRecordList& recs,
            //! Добавлять текст к началу каждой записи
            const QString& prefix = QString());
    Log& add(const Log& log,
            //! Добавлять текст к началу каждой записи
            const QString& prefix = QString());

    Log& operator<<(const LogRecord& r);
    Log& operator<<(const Log& log);
    Log& operator<<(const Error& error);
    Log& operator=(const Log& log);

    LogRecordList records() const;
    LogRecordList records(InformationType type) const;

    bool isValid() const;
    bool isEmpty() const;

    //! Содержит записи определенного типа
    bool contains(InformationType type) const;

    //! Количество различных типов записей
    int typesCount() const;

    //! Количество записей в логе
    int count() const;
    int count(InformationType type) const;
    //! Запись по номеру
    LogRecord record(int i) const;
    LogRecord record(int i, InformationType type) const;
    //! Очистить лог
    void clear();
    //! Название лога
    QString name() const;
    //! Дополнительная информация
    QString detail() const;

    //! Преобразовать в текстовую строку
    QString toString(InformationType type = InformationType::Invalid) const;

    //! Сохранить в файл в формате csv
    Error saveCSV(QIODevice* device, InformationType type = InformationType::Invalid) const;

    //! Преобразовать в QVariant
    QVariant variant() const { return QVariant::fromValue(*this); }
    //! Восстановить из QVariant
    static Log fromVariant(const QVariant& v) { return v.value<Log>(); }

signals:
    //! Запись была добавлена
    void sg_recordAdded(const zf::LogRecord& record);
    //! Лог был очищен
    void sg_cleared();

private:
    //! Данные
    QSharedDataPointer<Log_data> _d;

    friend class Framework;
};

inline Log operator+(const Log& log1, const Log& log2)
{
    if (!log1.isValid() && !log2.isValid())
        return Log();
    if (log1.isValid() && !log2.isValid())
        return log1;
    if (!log1.isValid() && log2.isValid())
        return log2;
    return Log(log1).add(log2);
}

} // namespace zf

Q_DECLARE_METATYPE(zf::LogRecord);
Q_DECLARE_METATYPE(zf::Log);

#endif // ZF_LOG_H
