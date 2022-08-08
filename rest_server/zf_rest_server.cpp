#include "zf_rest_server.h"
#include "zf_rest_connection.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCertificate>
#include <QSslCipher>
#include <QTimer>

#include <QUuid>
#include <QElapsedTimer>
#include <QApplication>
#include <QFile>
#include <QTextCodec>
#include <QApplication>

#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_utils.h"

namespace zf::http
{
RestServer::RestServer(QObject* parent)
    : QObject(parent)
    , _session_manager(std::make_unique<SessionManager>(this))
{
}

RestServer::~RestServer()
{    
    stop();
}

Error RestServer::start(
    const RestConfiguration& config, quint16 port, const QHostAddress& address, const QString& key_file, const QString& certificate_file)
{
    stop();

    Error error = initialization();
    if (error.isError())
        return error;

    _config = config;

    Z_CHECK(_tcp_server == nullptr);
    Z_CHECK(_task_controller == nullptr);

    initSslConfuguration(key_file, certificate_file);

    if (key_file.isEmpty())
        _tcp_server = new RestNonSslTcpServer(this);
    else
        _tcp_server = new RestSslTcpServer(this);

    connect(_tcp_server, &RestTcpServer::acceptError, this, &RestServer::sl_connection_accept_error, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_tooManySessions, this, &RestServer::sl_tooManySessions, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_tooManyTasks, this, &RestServer::sl_tooManyTasks, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_socketError, this, &RestServer::sl_connection_socketError, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_taskCreated, this, &RestServer::sl_connection_taskCreated, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_badRequest, this, &RestServer::sl_connection_badRequest, Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_sessionNotFound, this, &RestServer::sl_connection_sessionNotFound,
            Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_resultDataDelivered, this, &RestServer::sl_connection_resultDataDelivered,
            Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connection_resultErrorDelivered, this, &RestServer::sl_connection_resultErrorDelivered,
            Qt::DirectConnection);
    connect(_tcp_server, &RestTcpServer::sg_connectionClosed, this, &RestServer::sl_connectionClosed, Qt::DirectConnection);

    _task_controller = new thread::Controller(0, _config.maxThreadCount(), _config.threadInternalTimeout(), this);

    connect(_task_controller, &thread::Controller::sg_started, this, &RestServer::sl_task_started, Qt::DirectConnection);
    connect(_task_controller, &thread::Controller::sg_data, this, &RestServer::sl_task_feedback, Qt::DirectConnection);
    connect(_task_controller, &thread::Controller::sg_cancelled, this, &RestServer::sl_task_finished, Qt::DirectConnection);
    connect(_task_controller, &thread::Controller::sg_error, this, &RestServer::sl_task_error, Qt::DirectConnection);
    connect(_task_controller, &thread::Controller::sg_finished, this, &RestServer::sl_task_finished, Qt::DirectConnection);

    _session_manager->updateConfig();
    _session_manager->loadSessionsFromDisk();

    bool res = _tcp_server->listen(address, port);
    if (!res) {
        auto error = _tcp_server->errorString();
        stop();
        return Error(error);
    }

    return Error();
}

void RestServer::stop()
{
    _session_manager->flush();

    if (_tcp_server != nullptr) {
        _tcp_server->stop();
        delete _tcp_server;
        _tcp_server = nullptr;
    }

    if (_task_controller != nullptr) {
        _task_controller->cancelAll();
        delete _task_controller;
        _task_controller = nullptr;
    }
}

bool RestServer::isStarted() const
{
    return _tcp_server != nullptr && _tcp_server->isListening();
}

void RestServer::pause()
{
    _pause_counter++;
}

void RestServer::resume()
{
    if (_pause_counter == 0) {
        Core::logError(" RestServer::resume - counter error");
        return;
    }
    _pause_counter--;

    if (_pause_counter > 0)
        return;

    while (!_inbound_task_request_queue.isEmpty()) {
        auto request = _inbound_task_request_queue.dequeue();
        processTaskRequest(request->session_id, request->request, request->ip_interface_address, request->client_info);
    }
}

bool RestServer::isPaused() const
{
    return _pause_counter > 0;
}

int RestServer::sessionCount() const
{
    return _session_manager->sessionCount();
}

int RestServer::taskCount() const
{
    return _task_controller->count()
           + qMax(0, _posted_new_task_count.
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                     loadRelaxed()
#else
                     load()
#endif
           );
}

int RestServer::processingTaskCount() const
{
    return _task_controller->count();
}

SessionResultPtr RestServer::sessionResult(const QString& session_id) const
{
    auto session = getSessionById(session_id);
    if (session == nullptr)
        return nullptr;

    return session->isTaskCompleted() ? session->takeResult() : Z_MAKE_SHARED(SessionResult);
}

QHostAddress RestServer::host() const
{
    Z_CHECK_NULL(_tcp_server);
    return _tcp_server->serverAddress();
}

quint16 RestServer::port() const
{
    Z_CHECK_NULL(_tcp_server);
    return _tcp_server->serverPort();
}

bool RestServer::removeSession(const QString& session_id) const
{
    auto session = _session_manager->getSession(session_id);
    if (session != nullptr && !session->isSessionClosed()) {
        session->closeSession();        
        _session_manager->removeSession(session_id);
        return true;
    }

    return false;
}

void RestServer::processTaskRequest(const QString& session_id, const HttpRequestHeader& request, const QString& ip_interface_address,
                                    const QString& client_info)
{
    // счетчик был увеличен при создании нового соединения
    taskCountDecreased();

    auto task = createTask(session_id, client_info, ip_interface_address);
    Z_CHECK_NULL(task);

    if (_task_controller->startWorker(task).isEmpty()) {
        delete task;
        logSession(session_id, "out of system resources (too many threads)");
        return;
    }
    _pending_requests[task->id()] = Z_MAKE_SHARED(QVariant, QVariant::fromValue(request));

    _session_manager->registerNewSession(ip_interface_address, client_info, session_id, _config.keepDataTimout());
    _session_manager->requestFlush();

    logSession(session_id, "new task");
    emit sg_newTask(session_id);
}

SessionPtr RestServer::getSessionById(const QString& session_id) const
{    
    auto session = _session_manager->getSession(session_id);
    if (session == nullptr)
        return nullptr;
    return session->isSessionClosed() ? nullptr : session;
}

const RestConfiguration* RestServer::config() const
{
    return &_config;
}

const QSslConfiguration* RestServer::sslConfiguration() const
{
    return _ssl_config.get();
}

void RestServer::log(const QString& text) const
{
    Core::logInfo(text);
}

void RestServer::log(const Error& error) const
{
    Core::logError(error);
}

RestConnection* RestServer::createConnectionObject(qintptr socket_descriptor, QObject* parent) const
{
    return new RestConnection(this, socket_descriptor, parent);
}

zf::MessageID RestServer::requestAccessRights(const HttpRequestHeader& request, const QString& login) const
{
    Q_UNUSED(request)
    Q_UNUSED(login)

    // по умолчанию ничего не проверяем
    MessageID fake_id = MessageID::generate();
    emit const_cast<RestServer*>(this)->sg_requestAccessRightsFeedback(fake_id, true, {});
    return fake_id;
}

Error RestServer::loadConfig(const QString& config_file, RestConfiguration& config, Credentials& credentials, QHostAddress& address,
                             quint64& port, QString& key, QString& certificate)
{
    if (!QFile::exists(config_file))
        return Error::fileNotFoundError(config_file);

    QSettings settings(config_file, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    if (settings.status() != QSettings::NoError) {
        if (settings.status() == QSettings::AccessError)
            return Error(QDir::toNativeSeparators(config_file) + " not accessible");
        else if (settings.status() == QSettings::FormatError)
            return Error(QDir::toNativeSeparators(config_file) + " format error");
        else
            return Error(QDir::toNativeSeparators(config_file) + " unknown error");
    }

    QString log_path_relative = settings.value("log/log_path_relative").toString();
    if (log_path_relative.left(1) == "/")
        log_path_relative.remove(0, 1);
    if (log_path_relative.right(1) == "/")
        log_path_relative.remove(log_path_relative.length() - 1, 1);

    QString log_path_absolute = settings.value("log/log_path_absolute").toString();
    if (log_path_absolute.right(1) == "/")
        log_path_absolute.remove(log_path_absolute.length() - 1, 1);

    if (log_path_absolute.isEmpty() && !log_path_relative.isEmpty())
        log_path_absolute = QApplication::instance()->applicationDirPath() + "/" + log_path_relative;

    if (log_path_absolute.isEmpty())
        QApplication::instance()->applicationDirPath() + "/logs";

    Utils::setLogLocation(log_path_absolute);

    QString host_arg = settings.value("address/host").toString();
    QString port_arg = settings.value("address/port").toString();

    QString login_arg = settings.value("database/login").toString();
    QString password_arg = settings.value("database/password").toString();

    key = settings.value("ssl/key").toString();
    certificate = settings.value("ssl/certificate").toString();

    QString threads_arg = settings.value("tuning/threads").toString();
    QString tasks_arg = settings.value("tuning/tasks").toString();
    QString sessions_arg = settings.value("tuning/sessions").toString();
    QString connections_arg = settings.value("tuning/connections").toString();
    QString connection_threads_arg = settings.value("tuning/c-threads").toString();
    QString timeout_arg = settings.value("tuning/timeout").toString();
    QString keep_data_timeout_arg = settings.value("tuning/keep").toString();
    QString collect_garbage_arg = settings.value("tuning/collect").toString();
    QString cache_size_arg = settings.value("tuning/cache").toString();
    QString max_size_arg = settings.value("tuning/maxsize").toString();

    port = port_arg.toUInt();
    if (port <= 0)
        port = 8080;

    if (host_arg.toLower() == "*")
        address = QHostAddress::Any;
    else
        address = QHostAddress(host_arg);

    if (address.isNull())
        address = QHostAddress::Any;

    quint64 threads = threads_arg.toUInt();    

    quint64 tasks = tasks_arg.toUInt();
    if (tasks <= 0)
        tasks = 100;

    quint64 sessions = sessions_arg.toUInt();
    if (sessions <= 0)
        sessions = 2000;

    quint64 connections = connections_arg.toUInt();
    if (connections <= 0)
        connections = 1000;

    quint64 connection_threads = connection_threads_arg.toUInt();

    quint64 timeout = timeout_arg.toUInt();
    if (timeout <= 0)
        timeout = 10000;

    quint64 keep_data_timeout = keep_data_timeout_arg.toUInt();
    if (keep_data_timeout <= 0)
        keep_data_timeout = 600;

    quint64 collect_garbage = collect_garbage_arg.toUInt();
    if (collect_garbage <= 0)
        collect_garbage = 30;

    quint64 cache_size = cache_size_arg.toUInt();
    if (cache_size <= 0)
        cache_size = 1000;

    quint64 max_size = max_size_arg.toUInt();
    if (max_size <= 0)
        max_size = 10;

    if (key.isEmpty() != certificate.isEmpty()) {
        if (key.isEmpty())
            return Error("ssl key file not defined");
        else
            return Error("ssl certificate file not defined");
    }

    if (login_arg.isNull() && !password_arg.isEmpty())
        return Error("login not defined");

    //! Параметры доступа к БД
    if (!login_arg.isEmpty())
        credentials = Credentials(login_arg, password_arg);
    else
        credentials = Credentials();

    config = http::RestConfiguration();
    config.setMaxThreadCount(threads);
    config.setConnectionsThreadCount(connection_threads);
    config.setMaxTaskCount(tasks);
    config.setMaxSessionsCount(sessions);
    config.setMaxConnectionCount(connections);
    config.setDisconnectTimeout(timeout);
    config.setKeepDataTimout(keep_data_timeout);
    config.setMaxSessionResultCached(cache_size);
    config.setCollectGarbagePeriod(collect_garbage);
    config.setMaxRequestContentSize(max_size);
    config.setAcceptedVersions({Version(1, 0, 0)});

    return Error();
}

Error RestServer::initialization()
{
    return Error();
}

void RestServer::onTaskStarted(const QString& session_id)
{
    Q_UNUSED(session_id)
}

void RestServer::onTaskFeedback(const QString& session_id, qint64 status, QVariantPtr data)
{
    Q_UNUSED(session_id)
    Q_UNUSED(status)
    Q_UNUSED(data)
}

void RestServer::onTaskError(const QString& session_id, qint64 status, const QString& error_text)
{
    Q_UNUSED(session_id)
    Q_UNUSED(status)
    Q_UNUSED(error_text)
}

void RestServer::onTaskFinished(const QString& session_id)
{
    Q_UNUSED(session_id);
}

void RestServer::sl_connection_accept_error(QAbstractSocket::SocketError socketError)
{
    log(Error("socket error " + QString::number(socketError)));
}

void RestServer::sl_task_started(const QString& session_id)
{
    auto request = _pending_requests.value(session_id);
    Z_CHECK_NULL(request);
    _pending_requests.remove(session_id);
    // отправляем запрос на обработку
    _task_controller->request(session_id, request);

    onTaskStarted(session_id);
}

void RestServer::sl_task_feedback(const QString& session_id, qint64 status, QVariantPtr data)
{
    Q_UNUSED(status)    
    auto session = getSessionById(session_id);
    if (session != nullptr) {
        auto byte_array_data = Z_MAKE_SHARED(QByteArray, data->toByteArray());
        session->setTaskResult(getTaskResultContentType(status, byte_array_data), byte_array_data);

        logSession(session->sessionId(), "task finished");
    }

    onTaskFeedback(session_id, status, data);
}

void RestServer::sl_tooManySessions(const QString& client_info)
{
    if (!_too_many_session_info.isValid() || _too_many_session_info.elapsed() > 1000) {
        // не чаще чем раз в секунду
        _too_many_session_info.restart();
        logClient(client_info, "too many sessions");
    }
}

void RestServer::sl_tooManyTasks(const QString& client_info)
{
    if (!_too_many_session_info.isValid() || _too_many_session_info.elapsed() > 1000) {
        // не чаще чем раз в секунду
        _too_many_session_info.restart();
        logClient(client_info, "too many tasks");
    }
}

void RestServer::sl_connection_socketError(const QString& session_id, const QString& client_info, const Error& error)
{    
    if (session_id.isEmpty())
        logClient(client_info, error.fullText());
    else
        logSession(session_id, error.fullText());
}

void RestServer::sl_connection_taskCreated(const QString& session_id, const HttpRequestHeader& request, const QString& ip_interface_address,
                                           const QString& client_info)
{    
    if (_task_controller == nullptr)
        return;

    if (isPaused()) {
        auto new_request = Z_MAKE_SHARED(TaskRequestInfo);
        new_request->session_id = session_id;
        new_request->request = request;
        new_request->client_info = client_info;
        new_request->ip_interface_address = ip_interface_address;
        _inbound_task_request_queue.enqueue(new_request);

        return;
    }

    processTaskRequest(session_id, request, ip_interface_address, client_info);
}

void RestServer::sl_connection_badRequest(const QString& session_id, const QString& client_info, const Error& error)
{
    if (_session_manager->getSession(session_id) == nullptr)
        logClient(client_info, error.fullText());
    else
        logSession(session_id, error.fullText());

    if (!session_id.isEmpty())
        removeSession(session_id);
}

void RestServer::sl_connection_sessionNotFound(const QString& session_id, const QString& client_info)
{
    logClient(client_info, QStringLiteral("session %1 not found").arg(session_id));
}

void RestServer::sl_connection_resultDataDelivered(const QString& session_id, const QString& client_info)
{    
    Q_UNUSED(client_info);
    logSession(session_id, "data delivered");
    removeSession(session_id);
}

void RestServer::sl_connection_resultErrorDelivered(const QString& session_id, const QString& client_info)
{    
    Q_UNUSED(client_info);
    logSession(session_id, "error delivered");
    removeSession(session_id);
}

void RestServer::sl_connectionClosed(const QString& session_id)
{    
    SessionPtr session = _session_manager->getSession(session_id);
    if (session == nullptr)
        return;

    if (session->canRemove())
        removeSession(session_id);
}

void RestServer::taskCountIncreased()
{
    _posted_new_task_count++;
}

void RestServer::taskCountDecreased()
{
    _posted_new_task_count--;    
}

void RestServer::initSslConfuguration(const QString& key_file, const QString& certificate_file)
{
    Z_CHECK(key_file.isEmpty() == certificate_file.isEmpty());

    if (key_file.isEmpty() && certificate_file.isEmpty())
        return;

    // генерация: openssl req -x509 -newkey rsa:2048 -keyout server.key -nodes -days 9999 -out server.crt

    QByteArray key;
    QByteArray certificate;

    QFile f_sert(certificate_file);
    if (!f_sert.open(QFile::ReadOnly)) {
        log(Error("error loading certificate file " + f_sert.fileName()));
    } else {
        certificate = f_sert.readAll();
        f_sert.close();
    }

    QFile f_key(key_file);
    if (!f_key.open(QFile::ReadOnly)) {
        log(Error("error loading key file " + f_key.fileName()));
    } else {
        key = f_key.readAll();
        f_key.close();
    }

    QSslKey sslKey(key, QSsl::Rsa);

    _ssl_config = std::make_unique<QSslConfiguration>();
    _ssl_config->setPrivateKey(sslKey);

    QSslCertificate sslCertificate(certificate);
    _ssl_config->setCaCertificates({sslCertificate});
    _ssl_config->setLocalCertificate(sslCertificate);
    _ssl_config->setProtocol(QSsl::AnyProtocol);
}

void RestServer::sl_task_error(const QString& session_id, qint64 status, const QString& error_text)
{
    Q_UNUSED(status)
    Z_CHECK(!error_text.isEmpty());

    auto session = getSessionById(session_id);
    if (session != nullptr) {
        StatusCode error_status = getTaskErrorStatus(status);
        QString erro_n = QString::number(static_cast<int>(error_status)).left(1);
        Z_CHECK(erro_n == "4" || erro_n == "5");

        session->setTaskError(Error(error_text), error_status);

        logSession(session->sessionId(),
                   "task error: " + error_text + " (" + HttpHeader::statusToString(error_status) + ")");
    }

    onTaskError(session_id, status, error_text);
}

void RestServer::sl_task_finished(const QString& session_id)
{
    auto session = getSessionById(session_id);
    if (session != nullptr) {
        if (!session->isTaskCompleted())
            removeSession(session_id);
    }

    onTaskFinished(session_id);
}

void RestServer::logSession(const QString& session_id, const QString& message) const
{
    if (message.isEmpty())
        return;

    QString client_info;
    auto session = _session_manager->getSession(session_id);
    if (session != nullptr)
        client_info = session->clientInfo();

    if (client_info.isEmpty() && session_id.isEmpty())
        log(message);
    else
        log(QString("[%1][%2] %3").arg(client_info, session_id, message));
}

void RestServer::logClient(const QString& client_info, const QString& message) const
{    
    if (client_info.isEmpty())
        log(message);
    else
        log(QString("[%1] %2").arg(client_info, message));
}

} // namespace zf::http
