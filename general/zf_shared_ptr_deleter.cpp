#include "zf_shared_ptr_deleter.h"
#include "zf_core.h"

namespace zf
{
SharedPtrDeleter::SharedPtrDeleter()
    : _timer(new QTimer(this))
{
    _timer->setInterval(0);
    _timer->setSingleShot(true);
    connect(_timer, &QTimer::timeout, this, &SharedPtrDeleter::sl_timeout);
}

void SharedPtrDeleter::deleteLater(const std::shared_ptr<void>& ptr)
{
    Z_CHECK_NULL(ptr);
    QMutexLocker lock(&_mutex);
    _buffer.enqueue(ptr);
    _timer->start();
}

void SharedPtrDeleter::sl_timeout()
{
    QMutexLocker lock(&_mutex);
    while (!_buffer.isEmpty()) {
        _buffer.dequeue();
    }
}

} // namespace zf
