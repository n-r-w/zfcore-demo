#include "zf_rest_config.h"
#include "zf_core.h"

namespace zf::http
{
quint8 RestConfiguration::maxThreadCount() const
{
    return _max_thread_count;
}

void RestConfiguration::setMaxThreadCount(quint8 max_thread_count)
{
    _max_thread_count = max_thread_count == 0 ? QThread::idealThreadCount() : max_thread_count;
}

quint8 RestConfiguration::connectionsThreadCount() const
{
    return _connections_thread_count;
}

void RestConfiguration::setConnectionsThreadCount(quint8 connections_thread_count)
{
    _connections_thread_count = connections_thread_count == 0 ? QThread::idealThreadCount() : connections_thread_count;
}

quint16 RestConfiguration::maxConnectionCount() const
{
    return _max_connection_count;
}

void RestConfiguration::setMaxConnectionCount(quint16 max_connection_count)
{
    Z_CHECK(max_connection_count > 0);
    _max_connection_count = max_connection_count;
}

quint16 RestConfiguration::maxTaskCount() const
{
    return _max_task_count;
}

void RestConfiguration::setMaxTaskCount(quint16 max_task_count)
{
    Z_CHECK(max_task_count > 0);
    _max_task_count = max_task_count;
}

quint16 RestConfiguration::maxSessionsCount() const
{
    return _max_sessions_count;
}

void RestConfiguration::setMaxSessionsCount(quint16 max_sessions_count)
{
    Z_CHECK(max_sessions_count > 0);
    _max_sessions_count = max_sessions_count;
}

quint16 RestConfiguration::readBufferSize() const
{
    return _read_buffer_size;
}

void RestConfiguration::setReadBufferSize(quint16 read_buffer_size)
{    
    _read_buffer_size = read_buffer_size;
}

quint16 RestConfiguration::writeBufferSize() const
{
    return _write_buffer_size;
}

void RestConfiguration::setWriteBufferSize(quint16 write_buffer_size)
{
    Z_CHECK(write_buffer_size > 0);
    _write_buffer_size = write_buffer_size;
}

quint32 RestConfiguration::maxRequestContentSize() const
{
    return _max_request_content_size;
}

void RestConfiguration::setMaxRequestContentSize(quint32 max_request_content_size)
{
    Z_CHECK(max_request_content_size > 0);
    _max_request_content_size = max_request_content_size;
}

quint64 RestConfiguration::keepDataTimout() const
{
    return _keep_data_timout;
}

void RestConfiguration::setKeepDataTimout(quint32 keep_data_timout)
{
    Z_CHECK(keep_data_timout > 0);
    _keep_data_timout = keep_data_timout;
}

quint32 RestConfiguration::disconnectTimeout() const
{
    return _disconnect_timeout;
}

void RestConfiguration::setDisconnectTimeout(quint32 disconnect_timeout)
{
    Z_CHECK(disconnect_timeout > 0);
    _disconnect_timeout = disconnect_timeout;
}

quint8 RestConfiguration::maxSessionIdLength() const
{
    return _max_session_id_length;
}

void RestConfiguration::setMaxSessionIdLength(quint8 max_session_id_length)
{
    Z_CHECK(max_session_id_length > 0);
    _max_session_id_length = max_session_id_length;
}

quint16 RestConfiguration::maxSessionResultCached() const
{
    return _max_session_result_cached;
}

void RestConfiguration::setMaxSessionResultCached(quint16 max_session_result_cached)
{
    Z_CHECK(max_session_result_cached > 0);
    _max_session_result_cached = max_session_result_cached;
}

quint16 RestConfiguration::collectGarbagePeriod() const
{
    return _collect_garbage_period;
}

void RestConfiguration::setCollectGarbagePeriod(quint16 collect_garbage_period)
{
    Z_CHECK(collect_garbage_period > 0);
    _collect_garbage_period = collect_garbage_period;
}

quint16 RestConfiguration::flushTimeout() const
{
    return _flush_timeout;
}

void RestConfiguration::setFlushTimeout(quint32 flush_timeout)
{
    _flush_timeout = flush_timeout;
}

const QList<Version>& RestConfiguration::acceptedVersions() const
{
    return _accepted_versions;
}

void RestConfiguration::setAcceptedVersions(const QList<Version>& accepted_versions)
{
    _accepted_versions = accepted_versions;

    _accepted_versions_string.clear();
    for (auto& v : _accepted_versions) {
        if (!_accepted_versions_string.isEmpty())
            _accepted_versions_string += ", ";

        _accepted_versions_string += v.text();
    }
}

const QString& RestConfiguration::acceptedVersionsString() const
{
    return _accepted_versions_string;
}

quint16 RestConfiguration::threadInternalTimeout() const
{
    return _thread_internal_timeout;
}

void RestConfiguration::setThreadInternalTimeout(const quint16& thread_internal_timeout)
{
    _thread_internal_timeout = thread_internal_timeout;
}
} // namespace zf::http
