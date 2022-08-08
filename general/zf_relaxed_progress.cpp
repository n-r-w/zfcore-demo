#include "zf_relaxed_progress.h"
#include "zf_core.h"
#include "zf_feedback_timer.h"

namespace zf
{
RelaxedProgress::RelaxedProgress(ProgressObject* progress, QObject* parent)
    : QObject(parent)
    , _progress(progress)
    , _timer(new QTimer(this))
{
    Z_CHECK_NULL(progress);

    _timer->setSingleShot(true);
    _timer->setInterval(_relaxed_timeout);
    connect(_timer, &QTimer::timeout, this, &RelaxedProgress::sl_timeout);

    connect(_progress, &ProgressObject::destroyed, this, &RelaxedProgress::sl_progressDestroyed);
    connect(_progress, &ProgressObject::sg_progressBegin, this, &RelaxedProgress::sl_progressBegin);
    connect(_progress, &ProgressObject::sg_progressEnd, this, &RelaxedProgress::sl_progressEnd);
    connect(_progress, &ProgressObject::sg_progressChange, this, &RelaxedProgress::sl_progressChange);
    connect(_progress, &ProgressObject::sg_cancelSupportActivated, this, &RelaxedProgress::sl_cancelSupportActivated);
}

RelaxedProgress::~RelaxedProgress()
{
    _timer->stop();
}

ProgressObject* RelaxedProgress::progress() const
{
    return _progress;
}

bool RelaxedProgress::isRelaxedProgress() const
{
    return _active;
}

void RelaxedProgress::setRelaxedTimeout(int ms)
{
    Z_CHECK(ms >= 0);

    if (_relaxed_timeout == ms)
        return;

    _relaxed_timeout = ms;
    _timer->setInterval(_relaxed_timeout);
}

int RelaxedProgress::relaxedTimeout() const
{
    return _relaxed_timeout;
}

void RelaxedProgress::force()
{
    if (!_progress->isProgress() || _active)
        return;

    _timer->stop();
    sl_timeout();
}

void RelaxedProgress::sl_timeout()
{
    Z_CHECK(!_active);
    Z_CHECK(_progress->isProgress());

    _active = true;
    emit sg_startProgress(
        _progress->progressText(), _progress->progressPercent(), _progress->isCancelProgressSupported());
}

void RelaxedProgress::sl_progressBegin()
{
    Z_CHECK(!_active);

    if (!_timer->isActive())
        _timer->start(_relaxed_timeout);
}

void RelaxedProgress::sl_progressEnd()
{
    if (_timer->isActive())
        _timer->stop();

    if (_active) {
        _active = false;
        emit sg_finishProgress();
    }
}

void RelaxedProgress::sl_progressChange(int percent, const QString& text)
{
    if (_active) {
        emit sg_progressChange(percent, text);
        return;
    }

    Utils::processEvents();
}

void RelaxedProgress::sl_cancelSupportActivated()
{
    if (_active)
        emit sg_cancelSupportActivated();
}

void RelaxedProgress::sl_progressDestroyed()
{
    // если прогресс удаляется не при завершении работы, то это ошибка
    Z_CHECK(!Core::isActive());

    if (_timer->isActive())
        _timer->stop();
}

} // namespace zf
