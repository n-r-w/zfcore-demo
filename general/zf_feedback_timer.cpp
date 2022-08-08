#include "zf_feedback_timer.h"

#include <QApplication>

#define TIMEOUT_EVENT (QEvent::User + 10000)

namespace zf
{
FeedbackTimer::FeedbackTimer(QObject* parent)
    : QObject(parent)
{
}

void FeedbackTimer::start()
{
    QMutexLocker lock(&_mutex);

    if (_active)
        return;

    _active = true;

    if (!_event_posted) {
        _event_posted = true;
        QApplication::postEvent(const_cast<FeedbackTimer*>(this), new QEvent(static_cast<QEvent::Type>(TIMEOUT_EVENT)));
    }
}

void FeedbackTimer::stop()
{
    QMutexLocker lock(&_mutex);
    _active = false;
}

bool FeedbackTimer::isActive() const
{
    QMutexLocker lock(&_mutex);
    return _active;
}

void FeedbackTimer::customEvent(QEvent* event)
{
    if (event->type() == TIMEOUT_EVENT) {
        _mutex.lock();

        _event_posted = false;

        if (_active) {
            _active = false;
            _mutex.unlock();

            emit timeout();

        } else {
            _mutex.unlock();
        }

        return;
    }
    QObject::customEvent(event);
}

} // namespace zf
