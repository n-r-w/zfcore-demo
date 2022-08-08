#pragma once

#include "zf_global.h"
#include <QThread>
#include <QTextStream>

#include <QObject>
#include <QThread>
#include <iostream>

#ifdef Q_OS_WIN32
class QWinEventNotifier;
#else
class QSocketNotifier;
#endif

namespace zf
{
//! Вывод текста в консоль
class ZCORESHARED_EXPORT ConsoleTextStream : public QTextStream
{
public:
    ConsoleTextStream();
    ConsoleTextStream& operator<<(const QString& string);
};

//! Ввод текста из консоли
class ZCORESHARED_EXPORT ConsoleInput : public QTextStream
{
public:
    ConsoleInput();
    QString readLine();
};

//! Перехват ввода в консоль (источник: https://github.com/juangburgos/QConsoleListener)
class ZCORESHARED_EXPORT ConsoleListener : public QObject
{
    Q_OBJECT

public:
    ConsoleListener(QObject* parent = nullptr);
    ~ConsoleListener();

Q_SIGNALS:
    // connect to "newLine" to receive console input
    void sg_newLine(const QString& strNewLine);
    // finishedGetLine if for internal use
    void sg_finishedGetLine(const QString& strNewLine);

private:
#ifdef Q_OS_WIN
    QWinEventNotifier* m_notifier;
#else
    QSocketNotifier* m_notifier;
#endif

private Q_SLOTS:
    void sl_finishedGetLine(const QString& strNewLine);

private:
    QThread m_thread;
};

} // namespace zf
