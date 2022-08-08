#include "zf_dbg_leaks.h"
#include <QThread>
#include <QDebug>
#include <zf_core.h>

namespace zf
{
void DebugMemoryLeak::new_object(const QString& key, bool print)
{
    QMutexLocker lock(&_mutex);

    auto n = counter()->value(key, 0) + 1;
    counter()->insert(key, n);
    if (print) {
        QString text = QStringLiteral("DebugMemoryLeak: %1 (>> %2, T:%3)")
                           .arg(key)
                           .arg(n)
                           .arg(reinterpret_cast<quint64>(QThread::currentThreadId()));
        qDebug() << text;
        Core::logInfo(text);
    }
}

void DebugMemoryLeak::delete_object(const QString& key, bool print)
{
    QMutexLocker lock(&_mutex);

    auto n = counter()->value(key, 0) - 1;
    counter()->insert(key, n);
    if (print) {
        QString text = QStringLiteral("DebugMemoryLeak: %1(<< %2, T:%3)")
                           .arg(key)
                           .arg(n)
                           .arg(reinterpret_cast<quint64>(QThread::currentThreadId()));
        qDebug() << text;
        Core::logInfo(text);
    }
}

QMap<QString, qint64> DebugMemoryLeak::leaks()
{
    QMutexLocker lock(&_mutex);

    QMap<QString, qint64> l;
    for (auto i = counter()->constBegin(); i != counter()->constEnd(); ++i) {
        if (i.value() > 0)
            l[i.key()] = i.value();
    }

    return l;
}

QHash<QString, qint64>* DebugMemoryLeak::counter()
{
    if (_counter == nullptr)
        _counter = std::make_unique<QHash<QString, qint64>>();
    return _counter.get();
}

} // namespace zf
