#include "zf_single_app.h"
#include <QLocalServer>
#include <QLocalSocket>

namespace zf
{
SingleAppInstance::SingleAppInstance()
{
}

SingleAppInstance::~SingleAppInstance()
{
    clear();
}

bool SingleAppInstance::start(const QString& app_code, Command command, int wait_period)
{
    clear();

    QLocalSocket socket;
    socket.connectToServer(app_code);
    if (socket.waitForConnected(wait_period)) {
        socket.write(commandToString(command).toUtf8());
        socket.waitForBytesWritten(wait_period * 2);
        if (command == Activate || command == Request)
            return false;
    }

    _app_code = app_code;
    _wait_period = wait_period;
    _localServer = new QLocalServer;
    connect(_localServer, &QLocalServer::newConnection, this, &SingleAppInstance::sl_newLocalConnection);

    if (!_localServer->listen(_app_code)) {
        if (_localServer->serverError() == QAbstractSocket::AddressInUseError) {
            QLocalServer::removeServer(_app_code);
            _localServer->listen(_app_code);
        }
    }

    return true;
}

bool SingleAppInstance::isStarted() const
{
    return _localServer != nullptr;
}

QString SingleAppInstance::code() const
{
    return _app_code;
}

void SingleAppInstance::sl_newLocalConnection()
{
    QLocalSocket* socket = _localServer->nextPendingConnection();
    if (socket) {
        bool read_ok = socket->waitForReadyRead(2 * _wait_period);
        if (read_ok)
            socket->waitForBytesWritten(2 * _wait_period);

        QByteArray message;
        if (read_ok)
            message = socket->readAll();
        delete socket;

        if (!message.isEmpty()) {
            qDebug() << QString::fromUtf8(message);
            emit sg_request(stringToCommand(QString::fromUtf8(message)));
        }
    }
}

void SingleAppInstance::clear()
{
    if (_localServer) {
        _localServer->close();
        disconnect(_localServer, &QLocalServer::newConnection, this, &SingleAppInstance::sl_newLocalConnection);
        delete _localServer;
        _localServer = nullptr;
    }
    _app_code.clear();
    _wait_period = -1;
}

#define TERMINATE_STRING QStringLiteral("TERMINATE")
#define ACTIVATE_STRING QStringLiteral("ACTIVATE")
#define REQUEST_STRING QStringLiteral("REQUEST")

QString SingleAppInstance::commandToString(SingleAppInstance::Command c)
{
    if (c == Request)
        return REQUEST_STRING;
    else if (c == Activate)
        return ACTIVATE_STRING;
    else if (c == Terminate)
        return TERMINATE_STRING;
    else {
        Q_ASSERT(false);
        return QString();
    }
}

SingleAppInstance::Command SingleAppInstance::stringToCommand(const QString& s)
{
    if (s == REQUEST_STRING)
        return Request;
    else if (s == ACTIVATE_STRING)
        return Activate;
    else if (s == TERMINATE_STRING)
        return Terminate;
    else {
        Q_ASSERT(false);
        return Terminate;
    }
}

} // namespace zf
