#include "zf_database_driver_config.h"

namespace zf
{
int DatabaseDriverConfig::templateCacheSize() const
{
    return _template_cache_size;
}

void DatabaseDriverConfig::setTemplateCacheSize(int template_cache_size)
{
    _template_cache_size = template_cache_size;
}

bool DatabaseDriverConfig::isBroadcasting() const
{
    return _broadcasting;
}

void DatabaseDriverConfig::setBroadcasting(bool broadcasting)
{
    _broadcasting = broadcasting;
}

int DatabaseDriverConfig::requestTimeout() const
{
    return _request_timeout;
}

void DatabaseDriverConfig::setRequestTimeout(int request_timeout)
{
    _request_timeout = request_timeout;
}

} // namespace zf
