#include "zf_rest_tcp_server.h"
#include "zf_core.h"
#include "zf_rest_connection.h"
#include "zf_rest_server.h"
#include "zf_socket_operations.h"
#include "zf_rest_server.h"

#include <QSslKey>
#include <QSslCertificate>

namespace zf::http
{
RestTcpServer::RestTcpServer(RestServer* rest_server, QObject* parent)
    : QTcpServer(parent)
    , _rest_server(rest_server)
{
    Z_CHECK_NULL(_rest_server);

    setMaxPendingConnections(rest_server->config()->maxConnectionCount());
    _connection_controller = new thread::Controller(0, _rest_server->config()->connectionsThreadCount(),
                                                    _rest_server->config()->threadInternalTimeout(), this);
    connect(_connection_controller, &thread::Controller::sg_created, this, &RestTcpServer::sl_connectionWorkerCreated,
            Qt::DirectConnection);

    connect(this, &RestTcpServer::newConnection, this, [&]() {
        // имитация для контроля количества входящих соединений
        auto fake_soket = nextPendingConnection();
        if (fake_soket != nullptr)
            delete fake_soket;
    });

    // инициализируем пул обработчиков соединений
    for (int i = 0; i < rest_server->config()->connectionsThreadCount(); i++) {
        _connection_controller->startWorker(new RestConnectionWorker(rest_server));
    }
}

void RestTcpServer::stop()
{
    close();
    _connection_controller->cancelAll();
}

RestServer* RestTcpServer::restServer() const
{
    return _rest_server;
}

void RestTcpServer::sl_connectionWorkerCreated(thread::Worker* worker)
{
    auto connection = qobject_cast<RestConnectionWorkerObject*>(worker->object());
    Z_CHECK_NULL(connection);

    connect(connection, &RestConnectionWorkerObject::sg_socketError, this, &RestTcpServer::sg_connection_socketError, Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_taskCreated, this, &RestTcpServer::sg_connection_taskCreated, Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_badRequest, this, &RestTcpServer::sg_connection_badRequest, Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_sessionNotFound, this, &RestTcpServer::sg_connection_sessionNotFound,
            Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_resultDataDelivered, this, &RestTcpServer::sg_connection_resultDataDelivered,
            Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_resultErrorDelivered, this, &RestTcpServer::sg_connection_resultErrorDelivered,
            Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_socketCreationError, this, &RestTcpServer::sl_socketCreationError,
            Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_connectionClosed, this, &RestTcpServer::sl_connectionClosed, Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_tooManyTasks, this, &RestTcpServer::sg_tooManyTasks, Qt::QueuedConnection);
    connect(connection, &RestConnectionWorkerObject::sg_tooManySessions, this, &RestTcpServer::sg_tooManySessions, Qt::QueuedConnection);
}

void RestTcpServer::sl_connectionClosed(QString session_id)
{
    emit sg_connectionClosed(session_id);
}

void RestTcpServer::sl_socketCreationError(Error error)
{
    emit sg_connection_socketError(QString(), QString(), error);
}

void RestTcpServer::incomingConnection(qintptr handle)
{
    if (_connection_controller->isCancellRequested())
        return;

    RestConnection::InitialStatus initial_status;

    if (restServer()->taskCount() >= _rest_server->config()->maxTaskCount()) {
        initial_status = RestConnection::InitialStatus::TasksLimitError;
    } else if (restServer()->sessionCount() >= _rest_server->config()->maxSessionsCount()) {
        initial_status = RestConnection::InitialStatus::SessionsLimitError;
    } else {
        initial_status = RestConnection::InitialStatus::OK;
    }

    _rest_server->taskCountIncreased();

    // выбираем поток с наименьшей нагрузкой
    QString best_task_id = _connection_controller->bestTask();
    Z_CHECK(!best_task_id.isEmpty());

    // шлем ему запрос
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setVersion(Consts::DATASTREAM_VERSION);

    toStreamInt(ds, handle);
    toStreamInt(ds, initial_status);
    _connection_controller->request(best_task_id, Z_MAKE_SHARED(QVariant, data));

    /* делаем вид что добавляем сокет, чтобы задействовать механизм ограничения входящих соединений
     * но реальный сокет добавить нельзя, т.к. он должен быть создан внутри потока RestConnection */
    addPendingConnection(new QTcpSocket);
}

QString RestTcpServer::clientName(const QAbstractSocket* socket)
{
    QString peer_name = socket->peerName();
    QString peer_address = socket->peerAddress().toString();
    if (peer_name.isEmpty())
        return peer_address;

    return QStringLiteral("%1 (%2)").arg(peer_address, peer_name);
}

RestSslTcpServer::RestSslTcpServer(RestServer* rest_server, QObject* parent)
    : RestTcpServer(rest_server, parent)
{    
}

RestNonSslTcpServer::RestNonSslTcpServer(RestServer* rest_server, QObject* parent)
    : RestTcpServer(rest_server, parent)
{
}

} // namespace zf::http
