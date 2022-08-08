#include "zf_console_utils.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#include <QWinEventNotifier>
#else
#include <locale.h>
#include <QSocketNotifier>
#endif

#include <iostream>

namespace zf
{
ConsoleTextStream::ConsoleTextStream()
    : QTextStream(stdout, QIODevice::WriteOnly)
{
}

ConsoleTextStream& ConsoleTextStream::operator<<(const QString& string)
{
#ifdef Q_OS_WIN32
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), string.utf16(), string.size(), NULL, NULL);
#else
    QTextStream::operator<<(string);
#endif
    return *this;
}

ConsoleInput::ConsoleInput()
    : QTextStream(stdin, QIODevice::ReadOnly)
{
}

QString ConsoleInput::readLine()
{
#ifdef Q_OS_WIN32
    const int bufsize = 512;
    wchar_t buf[bufsize];
    DWORD read;
    QString res;
    do {
        ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), buf, bufsize, &read, NULL);
        res += QString::fromWCharArray(buf, read);
    } while (read > 0 && res[res.length() - 1] != '\n');
    // could just do res.truncate(res.length() - 2), but better be safe
    while (res.length() > 0 && (res[res.length() - 1] == '\r' || res[res.length() - 1] == '\n'))
        res.truncate(res.length() - 1);
    return res;
#else
    return QTextStream::readLine();
#endif
}

ConsoleListener::ConsoleListener(QObject* parent)
    : QObject(parent)
{
    QObject::connect(this, &ConsoleListener::sg_finishedGetLine, this, &ConsoleListener::sl_finishedGetLine, Qt::QueuedConnection);
#ifdef Q_OS_WIN
    m_notifier = new QWinEventNotifier(GetStdHandle(STD_INPUT_HANDLE));
#else
    m_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read);
#endif
    // NOTE : move to thread because std::getline blocks,
    //        then we sync with main thread using a QueuedConnection with finishedGetLine
    m_notifier->moveToThread(&m_thread);
    QObject::connect(&m_thread, &QThread::finished, m_notifier, &QObject::deleteLater);
#ifdef Q_OS_WIN
    QObject::connect(m_notifier, &QWinEventNotifier::activated,
#else
    QObject::connect(m_notifier, &QSocketNotifier::activated,
#endif
                     m_notifier, [this]() {
                         std::string line;
                         std::getline(std::cin, line);
                         QString strLine = QString::fromStdString(line);
                         emit this->sg_finishedGetLine(strLine);
                     });
    m_thread.start();
}

void ConsoleListener::sl_finishedGetLine(const QString& strNewLine)
{
    emit this->sg_newLine(strNewLine);
}

ConsoleListener::~ConsoleListener()
{
    m_thread.quit();
    m_thread.wait();
}

} // namespace zf
