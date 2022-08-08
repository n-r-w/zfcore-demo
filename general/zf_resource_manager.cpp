#include "zf_resource_manager.h"
#include <QTimer>

namespace zf
{
ResourceManager::ResourceManager(int free_resource_period)
    : _free_resource_period(free_resource_period)
{
    _free_resources_timer = new QTimer(this);
    _free_resources_timer->setSingleShot(true);
    _free_resources_timer->setInterval(_free_resource_period * 1000);
    connect(_free_resources_timer, &QTimer::timeout, this, &ResourceManager::sl_freeResourceTimer);
}

void ResourceManager::resourceUsed() const
{
    // реагировать не чаше раз в секунду
    if (_free_resource_period > 0 && _free_resources_timer->isActive()
        && (1000 * _free_resource_period - _free_resources_timer->remainingTime() > 1000)) {
        _free_resources_timer->start();
    }
}

void ResourceManager::resourceAdded() const
{
    if (_free_resource_period > 0 && !_free_resources_timer->isActive()) {
        _free_resources_timer->start();
    }
}

void ResourceManager::setFreeResourcePeriod(int n)
{
    if (_free_resource_period == n)
        return;

    _free_resource_period = n;

    if (n <= 0) {
        _free_resources_timer->stop();

    } else {
        _free_resources_timer->setInterval(_free_resource_period * 1000);
        _free_resources_timer->start();
    }
}

int ResourceManager::freeResourcePeriod() const
{
    return _free_resource_period;
}

void ResourceManager::sl_freeResourceTimer()
{
    emit sg_freeResources();
}

} // namespace zf
