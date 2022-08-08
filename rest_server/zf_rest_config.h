#pragma once

#include "zf_global.h"
#include "zf_version.h"

namespace zf
{
namespace http
{
class ZCORESHARED_EXPORT RestConfiguration
{
public:
    RestConfiguration() = default;
    RestConfiguration(const RestConfiguration& r) = default;
    RestConfiguration& operator=(const RestConfiguration& r) = default;

    //! Максимальное количество потоков одновременной обработки данных. Если 0, то равно количеству логических CPU
    quint8 maxThreadCount() const;
    void setMaxThreadCount(quint8 max_thread_count);

    //! Количество потоков под входящие соединения. Если 0, то равно количеству логических CPU
    quint8 connectionsThreadCount() const;
    void setConnectionsThreadCount(quint8 connections_thread_count);

    //! Максимальное количество входящих соединений
    quint16 maxConnectionCount() const;
    void setMaxConnectionCount(quint16 max_connection_count);

    //! Максимальное количество расчитываемых задач
    quint16 maxTaskCount() const;
    void setMaxTaskCount(quint16 max_task_count);

    //! Максимальное количество сессий. Сессия начинается приемом заявки на расчет и закрывается выдачей результата или
    //! по таймауту
    quint16 maxSessionsCount() const;
    void setMaxSessionsCount(quint16 max_sessions_count);

    //! Размер буфера приема данных из сокета (байт). Если 0, то по умолчанию для QTcpSocket
    quint16 readBufferSize() const;
    void setReadBufferSize(quint16 read_buffer_size);

    //! Размер буфера записи данных в сокет (байт)
    quint16 writeBufferSize() const;
    void setWriteBufferSize(quint16 write_buffer_size);

    //! Максимально допустимый размер входящго пакета данных в МБ
    quint32 maxRequestContentSize() const;
    void setMaxRequestContentSize(quint32 max_request_content_size);

    //! Время ожидания отключения сокета (мс)
    quint32 disconnectTimeout() const;
    void setDisconnectTimeout(quint32 disconnect_timeout);

    //! Сколько хранить результат расчета в секундах
    quint64 keepDataTimout() const;
    void setKeepDataTimout(quint32 keep_data_timout);

    //! Максимальная длина иденфтификатора сессии
    quint8 maxSessionIdLength() const;
    void setMaxSessionIdLength(quint8 max_session_id_length);

    //! Максимальное количество результатов расчета сессии в памяти
    quint16 maxSessionResultCached() const;
    void setMaxSessionResultCached(quint16 max_session_result_cached);

    //! Периодичность сбора мусора в секундах
    quint16 collectGarbagePeriod() const;
    void setCollectGarbagePeriod(quint16 collect_garbage_period);

    //! Время бездействия для периодического сброса кэша на диск (с)
    quint16 flushTimeout() const;
    void setFlushTimeout(quint32 flush_timeout);

    //! Список допустимых версий протокола
    const QList<Version>& acceptedVersions() const;
    void setAcceptedVersions(const QList<Version>& accepted_versions);
    //! Список допустимых версий протокола одной строкой через запятую
    const QString& acceptedVersionsString() const;

    //! Максимальная задержка внутренних операций с потоками
    //! Может потребоваться увеличение для слабых CPU
    quint16 threadInternalTimeout() const;
    void setThreadInternalTimeout(const quint16& threadInternalTimeout);

private:
    //! Максимальное количество потоков одновременной обработки данных. Если 0, то равно количеству логических CPU
    quint8 _max_thread_count = 0;
    //! Максимальное количество потоков под входящие соединения
    quint8 _connections_thread_count = 100;
    //! Максимальное количество входящих соединений
    quint16 _max_connection_count = 300;
    //! Максимальное количество расчитываемых задач
    quint16 _max_task_count = 100;
    //! Максимальное количество сессий. Сессия начинается приемом заявки на расчет и закрывается выдачей результата или
    //! по таймауту
    quint16 _max_sessions_count = 2000;
    //! Размер буфера приема данных из сокета (байт)
    quint16 _read_buffer_size = 0;
    //! Размер буфера записи данных в сокет (байт)
    quint16 _write_buffer_size = 2048;
    //! Максимально допустимый размер входящго пакета данных в МБ
    quint32 _max_request_content_size = 10;
    //! Сколько хранить результат расчета в секундах
    quint64 _keep_data_timout = 60;
    //! Время ожидания отключения сокета (мс)
    quint32 _disconnect_timeout = 5000;
    //! Максимальная длина иденфтификатора сессии
    quint8 _max_session_id_length = 100;
    //! Максимальное количество результатов расчета сессии в памяти
    quint16 _max_session_result_cached = 100;
    //! Периодичность сбора мусора в секундах
    quint16 _collect_garbage_period = 30;
    //! Время бездействия для периодического сброса кэша на диск (с)
    quint16 _flush_timeout = 20;
    //! Список допустимых версий протокола
    QList<Version> _accepted_versions;
    //! Список допустимых версий протокола одной строкой через запятую
    QString _accepted_versions_string;
    //! Максимальная задержка внутренних операций с потоками
    //! Может потребоваться увеличение для слабых CPU
    quint16 _thread_internal_timeout = 10000;
};

} // namespace http
} // namespace zf
