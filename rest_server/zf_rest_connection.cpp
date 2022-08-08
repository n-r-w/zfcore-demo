#include "zf_rest_connection.h"
#include "zf_socket_operations.h"
#include "zf_rest_server.h"
#include "zf_utils.h"
#include "zf_core.h"

#include <QSslSocket>

namespace zf::http
{
RestConnection::RestConnection(const RestServer* rest_server, qintptr socket_descriptor, QObject* parent)
    : QObject(parent)
    , _rest_server(const_cast<RestServer*>(rest_server))
    , _socket_descriptor(socket_descriptor)
{
    Z_CHECK_NULL(rest_server);
    Z_CHECK(socket_descriptor > 0);
}

RestConnection::~RestConnection()
{    
}

void RestConnection::setInitialStatus(RestConnection::InitialStatus s)
{
    Z_CHECK(Utils::isMainThread());
    _initial_status = s;
}

RestConnection::InitialStatus RestConnection::initialStatus() const
{
    return _initial_status;
}

void RestConnection::process()
{
    QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

void RestConnection::cancell()
{
    _cancelled = true;

    if (_socket != nullptr)
        _socket->abort();

    _rest_server->taskCountDecreased();
}

void RestConnection::sl_sslErrors(const QList<QSslError>& errors)
{
    if (_cancelled || _socket == nullptr)
        return;

    auto socket = qobject_cast<QSslSocket*>(_socket);
    Z_CHECK_NULL(socket);

    QStringList err;
    for (auto& e : errors) {
        err << e.errorString();
    }

    socket->ignoreSslErrors(errors);
    emit sg_socketError(_session_id, _client_info, zf::Error(err.join("/r/n")));
}

void RestConnection::start()
{
    QSslSocket* ssl_socket = nullptr;

    if (_rest_server->sslConfiguration() == nullptr) {
        _socket = new TcpSocket(this);
    } else {
        _socket = new SslSocket(this);
        ssl_socket = qobject_cast<QSslSocket*>(_socket);
    }

    if (!_socket->setSocketDescriptor(_socket_descriptor)) {
        _rest_server->taskCountDecreased();
        _socket->abort();
        emit sg_socketCreationError(Error(_socket->errorString()));
        onConnectionClosed();
        return;
    }

    if (ssl_socket != nullptr) {
        connect(ssl_socket, qOverload<const QList<QSslError>&>(&QSslSocket::sslErrors), this, &RestConnection::sl_sslErrors,
                Qt::DirectConnection);
        ssl_socket->setSslConfiguration(*_rest_server->sslConfiguration());
        ssl_socket->startServerEncryption();
    }

    _client_info = RestTcpServer::clientName(_socket);

    if (_socket->state() == QTcpSocket::ConnectedState) {
        _ip_interface_address = QStringLiteral("%1:%2").arg(_socket->localAddress().toString()).arg(_socket->localPort());
    } else {
        connect(_socket, &QTcpSocket::connected, this, [this]() {
            _ip_interface_address = QStringLiteral("%1:%2").arg(_socket->localAddress().toString()).arg(_socket->localPort());
        });
    }

    if (_initial_status != InitialStatus::OK) {
        // сокет изначально был передан со статусом ошибки, чтобы отправить ответ клиенту
        QString error_text;
        if (_initial_status == InitialStatus::SessionsLimitError) {
            emit sg_tooManySessions(_client_info);
            error_text = "too many sessions";

        } else if (_initial_status == InitialStatus::TasksLimitError) {
            emit sg_tooManyTasks(_client_info);
            error_text = "too many tasks";

        } else
            Z_HALT_INT;

        // счетчик задач всегда увеличивается при создании нового соединения, поэтому уменьшаем, т.к. тут новая задача
        // не создается
        _rest_server->taskCountDecreased();

        HttpResponseHeader response(StatusCode::TooManyRequests);
        response.setContent(error_text);
        SocketWriter* writer = new SocketWriter(response, _client_info, _socket, _rest_server->config()->writeBufferSize(),
                                                _rest_server->config()->disconnectTimeout(), true, true);
        connect(writer, &SocketWriter::sg_done, this, [this]() { onConnectionClosed(); });
        connect(writer, &SocketWriter::sg_error, this, [this]() { onConnectionClosed(); });
        writer->start();
        return;
    }

    // читаем входящий запрос
    SocketHttpReader* request_reader = new SocketHttpReader(_client_info, _socket, _rest_server->config()->readBufferSize(),
                                                            _rest_server->config()->disconnectTimeout(), false, true);
    connect(request_reader, &SocketHttpReader::sg_request, this, [this](const HttpRequestHeader& request) {
        // проверяем права доступа
        checkAccessRights(request);
    });
    connect(request_reader, &SocketHttpReader::sg_response, this, [this](const HttpResponseHeader&) {
        // какая то странная ошибка
        onBadRequest(Error("invalid request"));
    });
    connect(request_reader, &SocketHttpReader::sg_error, this, [this](const Error& error) {
        // ошибка чтения
        onBadRequest(error);
    });
    request_reader->start();
}

void RestConnection::checkAccessRights(const HttpRequestHeader& request)
{
    // асинхронно запрашиваем права доступа
    connect(
        _rest_server, &RestServer::sg_requestAccessRightsFeedback, this,
        [this, request](MessageID message_id, bool accepted, zf::Error error) {
            if (message_id != _access_rights_feedback_id)
                return;

            if (accepted) {
                Z_CHECK(error.isOk());
                if (request.method() == Method::Post) {
                    processInitialRequest(request);

                } else if (request.method() == Method::Get) {
                    processGetResultRequest(request);

                } else {
                    onBadRequest(Error(QStringLiteral("invalid request method: %1").arg(request.methodString())));
                }
            } else {
                if (error.isError())
                    onBadRequest(error);
                else
                    onBadRequest(zf::Error("no access rights"));
            }
        },
        Qt::QueuedConnection);

    _access_rights_feedback_id = _rest_server->requestAccessRights(request, request.cridentials().login());
    Z_CHECK(_access_rights_feedback_id.isValid());
}

void RestConnection::processInitialRequest(const HttpRequestHeader& request)
{
    qint64 maximum_size = static_cast<qint64>(_rest_server->config()->maxRequestContentSize()) * 1024 * 1024;

    if (request.hasContentType() && request.hasContentLength() && request.contentLength() > 0 && request.contentLength() <= maximum_size) {
        Version version = request.protocolVersion();

        if (!_rest_server->config()->acceptedVersions().isEmpty() && !_rest_server->config()->acceptedVersions().contains(version)) {
            onBadRequest(Error(QStringLiteral("protocol version not accepted: %1 (supported %2)")
                                   .arg(version.text(), _rest_server->config()->acceptedVersionsString())));
        } else {
            // ответ - запрос принят
            onRequestAccept(request);
        }

    } else {
        onBadRequest(Error(QStringLiteral("bad content size: %1 (maximum %2)").arg(request.contentLength()).arg(maximum_size)));
    }
}

void RestConnection::processGetResultRequest(const HttpRequestHeader& request)
{
    // счетчик задач всегда увеличивается при создании нового соединения, поэтому уменьшаем, т.к. тут новая задача не создается
    _rest_server->taskCountDecreased();

    if (request.contentLength() == 0) {
        onBadRequest(Error(QStringLiteral("no session id")));

    } else if (request.contentLength() > _rest_server->config()->maxSessionIdLength()) {
        onBadRequest(Error(QStringLiteral("session id too long: %1")
                               .arg(QString::fromUtf8(request.content().left(_rest_server->config()->maxSessionIdLength())))));

    } else {
        _session_id = QString::fromUtf8(request.content());

        auto result = _rest_server->sessionResult(_session_id);
        if (result == nullptr) {
            // клиент проверяет готовность данных, но его сессии нет
            onSessionNotFound();
        } else {
            onResult(result);
        }
    }
}

void RestConnection::onBadRequest(const Error& error)
{
    // счетчик задач всегда увеличивается при создании нового соединения, поэтому уменьшаем, т.к. тут новая задача не создается
    _rest_server->taskCountDecreased();

    emit sg_badRequest(_session_id, _client_info, error);

    if (_socket != nullptr && _socket->isOpen()) {
        HttpResponseHeader response(StatusCode::BadRequest);
        response.setContent(error.fullText());
        response.setContentType(ContentType::TextPlain);

        SocketWriter* writer = new SocketWriter(response, _client_info, _socket, _rest_server->config()->writeBufferSize(),
                                                _rest_server->config()->disconnectTimeout(), true, true);
        connect(writer, &SocketWriter::sg_done, this, [this]() { onConnectionClosed(); });
        connect(writer, &SocketWriter::sg_error, this, [this, session_id = _session_id](const Error& error) {
            emit sg_socketError(session_id, _client_info, error);
            onConnectionClosed();
        });
        writer->start();

    } else {
        emit sg_socketError(_session_id, _client_info, error);
        onConnectionClosed();
    }
}

void RestConnection::onRequestAccept(const HttpRequestHeader& request)
{
    if (_socket != nullptr && _socket->isOpen()) {
        // создаем номер новой сессии. сам объект-сессия будет создан в RestServer
        _session_id = Utils::generateUniqueString();

        HttpResponseHeader response(StatusCode::Accepted);
        response.setContent(_session_id);
        response.setContentType(ContentType::TextPlain);

        SocketWriter* writer = new SocketWriter(response, _client_info, _socket, _rest_server->config()->writeBufferSize(),
                                                _rest_server->config()->disconnectTimeout(), true, true);
        connect(writer, &SocketWriter::sg_done, this, [this, session_id = _session_id, request]() {
            // информируем сервер о необходимости создания новой сессии
            emit sg_taskCreated(session_id, request, _ip_interface_address, _client_info);
            onConnectionClosed();
        });
        connect(writer, &SocketWriter::sg_error, this, [this](const Error& error) {
            // если ошибка при отправке ответа, то новую сессию создавать смысла нет
            emit sg_socketError(_session_id, _client_info, error);
            onConnectionClosed();
        });
        writer->start();

    } else {
        // счетчик задач всегда увеличивается при создании нового соединения, поэтому уменьшаем, т.к. тут новая задача не создается
        _rest_server->taskCountDecreased();
        emit sg_socketError(QString(), _client_info, Error(QStringLiteral("onRequestAccept - connection closed")));
        onConnectionClosed();
    }
}

void RestConnection::onSessionNotFound()
{
    emit sg_sessionNotFound(_session_id, _client_info);

    if (_socket != nullptr && _socket->isOpen()) {
        HttpResponseHeader response(StatusCode::NotFound);
        response.setContent(_session_id);
        response.setContentType(ContentType::TextPlain);

        SocketWriter* writer = new SocketWriter(response, _client_info, _socket, _rest_server->config()->writeBufferSize(),
                                                _rest_server->config()->disconnectTimeout(), true, true);
        connect(writer, &SocketWriter::sg_done, this, [this]() { onConnectionClosed(); });
        connect(writer, &SocketWriter::sg_error, this, [this](const Error& error) {
            emit sg_socketError(_session_id, _client_info, error);
            onConnectionClosed();
        });
        writer->start();

    } else {
        onConnectionClosed();
    }
}

void RestConnection::onResult(const SessionResultPtr& result)
{
    QScopedPointer<HttpResponseHeader> response;

    if (!result->isValid()) {
        // данные пока не готовы
        response.reset(new HttpResponseHeader(StatusCode::NoContent));
        response->setContentType(ContentType::TextPlain);
        response->setContent(result->sessionId());

    } else if (result->isError()) {
        // ошибка при расчете
        response.reset(new HttpResponseHeader(result->errorStatus()));
        response->setContent(QStringLiteral("%1\n%2").arg(result->sessionId(), result->error().fullText()));
        response->setContentType(ContentType::TextPlain);

    } else {
        // расчет готов
        response.reset(new HttpResponseHeader(StatusCode::Created));
        response->setContentType(result->contentType());
        response->setContent(*result->data());
    }

    if (_socket != nullptr && _socket->isOpen()) {
        SocketWriter* writer = new SocketWriter(*response.data(), _client_info, _socket, _rest_server->config()->writeBufferSize(),
                                                _rest_server->config()->disconnectTimeout(), true, true);
        connect(writer, &SocketWriter::sg_done, this, [this, result]() {
            // сообщаем о фактическом окончании сессии, т.к. клиент проинформирован о результате
            if (result->isError())
                emit sg_resultErrorDelivered(result->sessionId(), _client_info);
            else if (result->isValid())
                emit sg_resultDataDelivered(result->sessionId(), _client_info);
            onConnectionClosed();
        });
        connect(writer, &SocketWriter::sg_error, this, [this, result](const Error& error) {
            emit sg_socketError(result->sessionId(), _client_info, error);
            onConnectionClosed();
        });
        writer->start();

    } else {
        emit sg_socketError(QString(), _client_info, Error(QStringLiteral("onResult - connection closed")));
        onConnectionClosed();
    }
}

void RestConnection::onConnectionClosed()
{
    Z_CHECK(_socket == nullptr || _socket->state() == QAbstractSocket::UnconnectedState);
    emit sg_connectionClosed(_session_id);
    deleteLater();
}

RestConnectionWorker::RestConnectionWorker(RestServer* rest_server)
    : zf::thread::Worker(true, new RestConnectionWorkerObject)
    , _rest_server(rest_server)
{
}

RestConnectionWorker::~RestConnectionWorker()
{
}

RestConnectionWorkerObject* RestConnectionWorker::object() const
{
    return qobject_cast<RestConnectionWorkerObject*>(zf::thread::Worker::object());
}

void RestConnectionWorker::onCancell()
{
    for (auto c : qAsConst(_connections)) {
        RestConnection* con = qobject_cast<RestConnection*>(c);
        Z_CHECK_NULL(con);
        con->cancell();
    }
}

void RestConnectionWorker::onRequest()
{
    auto request = getRequest();
    Z_CHECK_NULL(request);

    qintptr socket_descriptor;
    RestConnection::InitialStatus initial_status;

    QDataStream ds(request->toByteArray());
    ds.setVersion(Consts::DATASTREAM_VERSION);

    fromStreamInt(ds, socket_descriptor);
    fromStreamInt(ds, initial_status);

    auto connection = _rest_server->createConnectionObject(socket_descriptor, object());
    object()->setupConnection(connection);

    _connections << connection;
    connection->process();
}

void RestConnectionWorker::onConnectionDestroyed(QObject* connection)
{
    Z_CHECK(_connections.remove(connection));
    sendDone();
}

RestConnectionWorkerObject::RestConnectionWorkerObject()
{
}

RestConnectionWorker* RestConnectionWorkerObject::worker() const
{
    return static_cast<RestConnectionWorker*>(zf::thread::WorkerObject::worker());
}

void RestConnectionWorkerObject::setupConnection(RestConnection* connection)
{
    Z_CHECK_NULL(connection);
    connect(connection, &RestConnection::sg_tooManySessions, this, &RestConnectionWorkerObject::sg_tooManySessions, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_tooManyTasks, this, &RestConnectionWorkerObject::sg_tooManyTasks, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_socketCreationError, this, &RestConnectionWorkerObject::sg_socketCreationError,
            Qt::DirectConnection);
    connect(connection, &RestConnection::sg_connectionClosed, this, &RestConnectionWorkerObject::sg_connectionClosed, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_socketError, this, &RestConnectionWorkerObject::sg_socketError, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_taskCreated, this, &RestConnectionWorkerObject::sg_taskCreated, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_badRequest, this, &RestConnectionWorkerObject::sg_badRequest, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_sessionNotFound, this, &RestConnectionWorkerObject::sg_sessionNotFound, Qt::DirectConnection);
    connect(connection, &RestConnection::sg_resultDataDelivered, this, &RestConnectionWorkerObject::sg_resultDataDelivered,
            Qt::DirectConnection);
    connect(connection, &RestConnection::sg_resultErrorDelivered, this, &RestConnectionWorkerObject::sg_resultErrorDelivered,
            Qt::DirectConnection);
    connect(
        connection, &RestConnection::destroyed, this, [this]() { worker()->onConnectionDestroyed(sender()); }, Qt::DirectConnection);
}

} // namespace zf::http
