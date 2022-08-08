#include "zf_rest_session.h"
#include "zf_utils.h"
#include "zf_rest_connection.h"
#include "zf_core.h"
#include "zf_rest_server.h"

namespace zf::http
{
SessionCacheItem::SessionCacheItem(const QString& session_id)
    : _session_id(session_id)
{    
}

SessionCacheItem::~SessionCacheItem()
{
    emit sg_deleted(_session_id); 
}

Session::Session()
{    
}

Session::Session(const QString ip_interface_address, const QString& client_info, const QString& session_id, quint64 lifetime,
                 const QDateTime& created)
    : _client_info(client_info)
    , _ip_interface_address(ip_interface_address)
    , _session_id(session_id)
    , _created(created)
    , _lifetime(lifetime)
{
    Z_CHECK(!session_id.isEmpty());
    Z_CHECK(lifetime > 0);   
}

Session::~Session()
{    
}

QString Session::clientInfo() const
{
    return _client_info;
}

void Session::closeSession()
{    
    QMutexLocker lock(&_mutex);
    if (_closed)
        return;

    _closed = true;
    emit sg_closed(_session_id);
}

bool Session::isTimeToDie() const
{
    if (_created.isNull())
        return false;

    qint64 lifetime = _created.secsTo(QDateTime::currentDateTime());
    if (lifetime < 0)
        return false; // какие то проблемы с системным временем

    return static_cast<quint64>(lifetime) > _lifetime;
}

quint64 Session::lifetime() const
{
    return _lifetime;
}

QDateTime Session::created() const
{
    return _created;
}

const QString Session::ipInterfaceAddress() const
{
    return _ip_interface_address;
}

bool Session::isSessionClosed() const
{
    QMutexLocker lock(&_mutex);
    return _closed;
}

QString Session::sessionId() const
{
    return _session_id;
}

bool Session::canRemove() const
{
    QMutexLocker lock(&_mutex);
    return (_closed || _result_taken);
}

bool Session::isTaskCompleted() const
{
    QMutexLocker lock(&_mutex);
    return _result_was_set;
}

void Session::setTaskResult(ContentType content_type, QByteArrayPtr data)
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(!_result_was_set);

    _result = Z_MAKE_SHARED(SessionResult, _session_id, content_type, data);
    _result_was_set = true;

    emit sg_resultInstalled(_session_id);
}

void Session::setTaskError(const Error& error, StatusCode error_status)
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(!_result_was_set);

    _result = Z_MAKE_SHARED(SessionResult, _session_id, error, error_status);
    _result_was_set = true;

    emit sg_resultInstalled(_session_id);
}

bool Session::isResultTaken() const
{
    QMutexLocker lock(&_mutex);
    return _result_taken;
}

static const int _session_stream_version = 3;
Error Session::saveToFile(const QString& file_name) const
{    
    QFile f(file_name);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(file_name);

    QDataStream st(&f);
    st.setVersion(Consts::DATASTREAM_VERSION);

    QMutexLocker lock(&_mutex);

    st << _session_stream_version;
    st << _session_id;
    st << _client_info;
    st << _ip_interface_address;
    st << _created;
    st << _lifetime;

    st << _result_taken;
    st << _result_was_set;
    st << _closed;

    st << (_result != nullptr);
    if (_result != nullptr)
        st << *_result.get();

    f.close();
    return Error();
}

std::shared_ptr<Session> Session::loadFromFile(const QString& file_name, Error& error)
{        
    error.clear();

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly)) {
        error = Error::fileIOError(file_name);
        return nullptr;
    }

    QDataStream st(&f);
    st.setVersion(Consts::DATASTREAM_VERSION);

    SessionPtr session = SessionPtr(new Session());
    int version;
    st >> version;
    if (version != _session_stream_version) {
        f.close();
        error = Error("wrong Session version file");
        return nullptr;
    }

    st >> session->_session_id;
    st >> session->_client_info;
    st >> session->_ip_interface_address;
    st >> session->_created;
    st >> session->_lifetime;

    st >> session->_result_taken;
    st >> session->_result_was_set;
    st >> session->_closed;

    bool has_result;
    st >> has_result;
    if (has_result) {
        session->_result = Z_MAKE_SHARED(SessionResult);
        st >> *session->_result.get();
    }

    if (st.status() != QDataStream::Ok)
        error = Error::badFileError(file_name);

    f.close();

    return error.isOk() ? session : nullptr;
}

SessionResultPtr Session::takeResult() const
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(_result_was_set);
    Z_CHECK(!_result_taken);

    auto res_copy = _result;
    _result.reset();
    _result_taken = true;

    emit const_cast<Session*>(this)->sg_resultTaken(_session_id);

    return res_copy;
}

SessionResult::SessionResult()
{
}

SessionResult::SessionResult(const SessionResult& r)
    : _session_id(r._session_id)
    , _content_type(r._content_type)
    , _data(r._data)
    , _error(r._error)
    , _error_status(r._error_status)
{
}

SessionResult::SessionResult(const QString& session_id, ContentType content_type, QByteArrayPtr data)
    : _session_id(session_id)
    , _content_type(content_type)
    , _data(data)
{
    Z_CHECK(content_type != ContentType::Unknown);
}

SessionResult::SessionResult(const QString& session_id, const Error& error, StatusCode error_status)
    : _session_id(session_id)
    , _data(Z_MAKE_SHARED(QByteArray))
    , _error(error)
    , _error_status(error_status)
{
    Z_CHECK(_error.isError());
}

SessionResult::~SessionResult()
{
}

bool SessionResult::isValid() const
{
    return !_session_id.isEmpty();
}

bool SessionResult::isError() const
{
    return _error.isError();
}

QString SessionResult::sessionId() const
{
    return _session_id;
}

ContentType SessionResult::contentType() const
{
    return _content_type;
}

QByteArrayPtr SessionResult::data() const
{
    return _data;
}

Error SessionResult::error() const
{
    return _error;
}

StatusCode SessionResult::errorStatus() const
{
    return _error_status;
}

static const int _session_result_stream_version = 1;
QDataStream& operator<<(QDataStream& s, const SessionResult& res)
{
    s << _session_result_stream_version;
    s << res._session_id;
    toStreamInt(s, res._content_type);
    s << (res._data == nullptr ? QByteArray() : *res.data());
    s << res._error;
    toStreamInt(s, res._error_status);
    return s;
}

QDataStream& operator>>(QDataStream& s, SessionResult& res)
{
    res = SessionResult();

    int version;
    s >> version;
    if (version != _session_result_stream_version) {
        if (s.status() == QDataStream::Ok)
            s.setStatus(QDataStream::ReadCorruptData);
        return s;
    }

    s >> res._session_id;
    fromStreamInt(s, res._content_type);

    QByteArray data;
    s >> data;
    if (data.isNull())
        res._data.reset();
    else
        res._data = Z_MAKE_SHARED(QByteArray, data);

    s >> res._error;
    fromStreamInt(s, res._error_status);

    if (s.status() != QDataStream::Ok)
        res = SessionResult();

    return s;
}

SessionManager::SessionManager(RestServer* server)
    : _server(server)
{
    Z_CHECK_NULL(server);

    connect(&_collect_garbage_timer, &QTimer::timeout, this, [&]() { collectGarbage(); });
    _collect_garbage_timer.setSingleShot(true);

    connect(&_flush_timer, &QTimer::timeout, this, [&]() {
        auto error = flush();
        if (error.isError())
            _server->log(error);            
    });
    _flush_timer.setSingleShot(true);

    updateConfig();
}

SessionManager::~SessionManager()
{
    flush();
}

void SessionManager::updateConfig()
{
    _session_cache.setMaxCost(_server->config()->maxSessionResultCached());

    _collect_garbage_timer.setInterval(_server->config()->collectGarbagePeriod() * 1000);
    _collect_garbage_timer.start();

    _flush_timer.setInterval(_server->config()->flushTimeout() * 1000);
}

SessionPtr SessionManager::getSession(const QString& session_id)
{
    QMutexLocker lock(&_mutex);

    auto info = getSessions()->value(session_id);
    if (info == nullptr)
        return nullptr;

    if (info->session == nullptr) {
        Error error;
        auto session = Session::loadFromFile(sessionFileName(session_id), error);
        if (error.isError()) {
            _server->log(error);
            return nullptr;
        }

        info->session = session;
        registerInCache(session_id);
    }

    return info->session;
}

SessionPtr SessionManager::registerNewSession(const QString& ip_interface_address, const QString& client_info, const QString& session_id,
                                              quint64 lifetime)
{    
    QMutexLocker lock(&_mutex);

    Z_CHECK(!getSessions()->contains(session_id));

    SessionPtr session = SessionPtr(new Session());
    session->_client_info = client_info;
    session->_ip_interface_address = ip_interface_address;
    session->_session_id = session_id;    
    session->_lifetime = lifetime;
    session->_created = QDateTime::currentDateTime();

    return registerNewSessionHelper(session);
}

SessionPtr SessionManager::registerNewSessionHelper(const SessionPtr& session)
{
    connect(session.get(), &Session::sg_resultInstalled, this, &SessionManager::sl_sessionResultInstalled);

    auto session_info = Z_MAKE_SHARED(SessionInfo);

    Z_CHECK(session_info->session == nullptr);
    session_info->session = session;
    session_info->session_id = session->sessionId();
    session_info->created = session->created();
    session_info->lifetime = session->lifetime();

    getSessions()->insert(session->sessionId(), session_info);
    registerInCache(session->sessionId());

    return session;
}

Error SessionManager::removeSession(const QString& session_id)
{
    QMutexLocker lock(&_mutex);

    auto info = sessionInfo(session_id);
    Z_CHECK_NULL(info);

    if (info->session != nullptr)
        info->session->closeSession();

    Error error = Utils::removeFile(sessionFileName(session_id));
    if (error.isOk()) {
        _sessions->remove(session_id);
        removeFromCache(session_id);

    } else {
        _server->log(error);
    }

    return error;
}

Error SessionManager::collectGarbage()
{
    QMutexLocker lock(&_mutex);

    Error error;
    QStringList to_remove;
    for (auto& s : *getSessions()) {
        if (!s->isTimeToDie())
            continue;
        to_remove << s->session_id;
    }

    for (auto& id : to_remove) {
        error << removeSession(id);
    }

    _collect_garbage_timer.start();
    return error;
}

Error SessionManager::flush()
{
    QMutexLocker lock(&_mutex);

    _last_remove_cache_errors.clear();
    // при удалении объектов из кэша будет автоматическое скидывание их на диск по сигналу SessionCacheItem::sg_deleted
    _session_cache.clear();
    return _last_remove_cache_errors;
}

void SessionManager::requestFlush()
{
    _flush_timer.start();
}

QString SessionManager::sessionManagerFolder()
{
    return Utils::dataLocation() + QStringLiteral("/sessions");
}

int SessionManager::sessionCount() const
{
    QMutexLocker lock(&_mutex);
    return getSessions()->count();
}

int SessionManager::sessionInMemoryCount() const
{
    QMutexLocker lock(&_mutex);
    return _session_cache.count();
}

void SessionManager::sl_sessionResultInstalled(const QString& session_id)
{    
    QMutexLocker lock(&_mutex);
    registerInCache(session_id);
}

void SessionManager::sl_cacheItemDeleted(const QString& session_id)
{
    QMutexLocker lock(&_mutex);
    // надо сохранить эту сессию на диск, если конечно она уже не там
    _last_remove_cache_errors << flushSession(session_id);
}

QString SessionManager::sessionFileName(const QString& session_id)
{
    Z_CHECK(!session_id.isEmpty());
    return sessionManagerFolder() + "/" + session_id;
}

QString SessionManager::sessionIdFromFileName(const QString& file_name)
{
    return file_name;
}

Error SessionManager::flushSession(const QString& session_id)
{
    auto info = sessionInfo(session_id);
    if (info == nullptr || info->session == nullptr || info->session->isSessionClosed())
        return Error();

    Error error = Utils::makeDir(sessionManagerFolder());
    if (error.isError())
        return error;

    error = info->session->saveToFile(sessionFileName(session_id));
    if (error.isError())
        return error;

    info->session.reset();

    return Error();
}

Error SessionManager::loadSession(const QString& session_id)
{
    auto info = sessionInfo(session_id);
    if (info != nullptr && info->session != nullptr)
        return Error(); // уже все загружено

    Error error;
    SessionPtr session = Session::loadFromFile(sessionFileName(session_id), error);
    if (error.isError())
        return error;

    if (info == nullptr) {
        registerNewSessionHelper(session);

    } else {
        Z_CHECK(info->session == nullptr);
        info->session = session;
    }

    registerInCache(session_id);

    return Error();
}

SessionManager::SessionInfoPtr SessionManager::sessionInfo(const QString& session_id) const
{
    return getSessions()->value(session_id);
}

Error SessionManager::loadSessionsFromDisk()
{
    Z_CHECK(_session_cache.isEmpty() && getSessions()->isEmpty());

    QDir dir(sessionManagerFolder());
    Error error;
    for (auto& file_name : dir.entryList(QDir::Files)) {
        error << loadSession(sessionIdFromFileName(QFileInfo(file_name).fileName()));
    }

    return error;
}

QHash<QString, SessionManager::SessionInfoPtr>* SessionManager::getSessions() const
{
    if (_sessions == nullptr)
        _sessions = std::make_unique<QHash<QString, SessionInfoPtr>>();

    return _sessions.get();
}

Error SessionManager::registerInCache(const QString& session_id)
{    
    if (_session_cache.contains(session_id))
        return Error();

    SessionCacheItem* cache_item = new SessionCacheItem(session_id);
    connect(cache_item, &SessionCacheItem::sg_deleted, this, &SessionManager::sl_cacheItemDeleted);

    _last_remove_cache_errors.clear();
    // при удалении из кэша _last_remove_cache_errors заполнится само
    _session_cache.insert(session_id, cache_item);
    return _last_remove_cache_errors;
}

void SessionManager::removeFromCache(const QString& session_id)
{
    _session_cache.remove(session_id);
}

bool SessionManager::SessionInfo::isTimeToDie() const
{
    if (created.isNull())
        return false;

    qint64 lt = created.secsTo(QDateTime::currentDateTime());
    if (lt < 0)
        return false; // какие то проблемы с системным временем

    return static_cast<quint64>(lt) > lifetime;
}

} // namespace zf::http
