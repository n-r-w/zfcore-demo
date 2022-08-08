#include "zf_cumulative_error.h"
#include "zf_core.h"
#include "zf_framework.h"

namespace zf
{
QMutex CumulativeError::_block_count_mutex;
int CumulativeError::_block_count = 0;

CumulativeError::CumulativeError()
    : _timer(new QTimer(this))
{
    _timer->setInterval(200);
    _timer->setSingleShot(false);
    _timer->start();
    connect(_timer, &QTimer::timeout, this, &CumulativeError::sl_timeout);
}

CumulativeError::~CumulativeError()
{
}

void CumulativeError::addError(const Error& error)
{
    QMutexLocker lock(&_mutex);
    if (Utils::isAppHalted() || !Core::isBootstraped())
        return;

    _errors << error;
}

void CumulativeError::block()
{
    QMutexLocker lock(&_block_count_mutex);
    _block_count++;
}

void CumulativeError::unblock()
{
    QMutexLocker lock(&_block_count_mutex);
    Z_CHECK(_block_count > 0);
    _block_count--;
}

void CumulativeError::sl_timeout()
{
    QMutexLocker block_count_mutex_lock(&_block_count_mutex);
    if (_block_count > 0)
        return;
    block_count_mutex_lock.unlock();

    _mutex.lock();
    if (_errors.isEmpty() || _show_on) {
        _mutex.unlock();
        return;
    }

    if (Core::mode().testFlag(CoreMode::Library)) {
        _errors.clear();
        _mutex.unlock();
        return;
    }

    if (Core::mode().testFlag(CoreMode::Application) && !Core::isActive()) {
        // надо дождаться открытия главного окна
        _mutex.unlock();
        return;
    }

    auto errors_copy = _errors;
    _errors.clear();
    _show_on = true;
    _mutex.unlock();

    // надо выделить ошибки типа fileDebug
    ErrorList regular_errors;
    for (auto& e : qAsConst(errors_copy)) {
        if (e.isFileDebug())
            Core::error(e); // запись в файл
        else
            regular_errors << e;
    }

    if (!regular_errors.isEmpty()) {
        Error full_error;
        full_error.addChildError(regular_errors);
        if (full_error.isError())
            Core::error(full_error);
    }

    _mutex.lock();
    _show_on = false;
    _mutex.unlock();
}

} // namespace zf
