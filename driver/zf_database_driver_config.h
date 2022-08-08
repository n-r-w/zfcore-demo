#pragma once

#include "zf_global.h"

namespace zf
{
//! Параметры драйвера БД
class ZCORESHARED_EXPORT DatabaseDriverConfig
{
public:
    DatabaseDriverConfig() = default;
    DatabaseDriverConfig(const DatabaseDriverConfig& c) = default;
    DatabaseDriverConfig& operator=(const DatabaseDriverConfig& c) = default;

    //! Размера кэша файлов шаблонов отчета
    int templateCacheSize() const;
    //! Размера кэша файлов шаблонов отчета
    void setTemplateCacheSize(int template_cache_size);

    //! Рассылать ли сообщения об изменении данных
    bool isBroadcasting() const;
    //! Рассылать ли сообщения об изменении данных
    void setBroadcasting(bool broadcasting);

    //! Время ожидания ответа от сервера (мс)
    int requestTimeout() const;
    //! Задать время ожидания ответа от сервера (мс)
    void setRequestTimeout(int request_timeout);

private:
    //! Размера кэша файлов шаблонов отчета
    int _template_cache_size = 100;
    //! Рассылать ли сообщения об изменении данных
    bool _broadcasting = false;

    //! Время ожидания ответа от сервера (мс)
    int _request_timeout = 2000;
};
} // namespace zf
