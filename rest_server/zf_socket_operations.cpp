#include "zf_socket_operations.h"
#include "zf_core.h"

namespace zf::http
{
SocketOperation::SocketOperation(const QString& host_info, QAbstractSocket* socket, quint16 buffer_size, quint32 disconnect_timeout,
                                 bool close_on_finish, bool delete_on_finish, QObject* parent)
    : QObject(parent)
    , _socket(socket)
    , _id(Utils::generateUniqueString())
    , _host_info(host_info)
    , _disconnect_timeout(disconnect_timeout)
    , _close_on_finish(close_on_finish)
    , _delete_on_finish(delete_on_finish)
    , _buffer_size(buffer_size)
{
    Z_CHECK_NULL(socket);
    Z_CHECK(!host_info.isEmpty());
}

SocketOperation::~SocketOperation()
{
    if (_close_on_finish && !_socket.isNull())
        _socket->abort();
}

QString SocketOperation::uniqueId() const
{
    return _id;
}

QAbstractSocket* SocketOperation::socket() const
{
    return _socket;
}

QString SocketOperation::hostInfo() const
{
    return _host_info;
}

quint32 SocketOperation::disconnectTimeout() const
{
    return _disconnect_timeout;
}

bool SocketOperation::hasError() const
{
    return _error.isError();
}

Error SocketOperation::error() const
{
    return _error;
}

void SocketOperation::start()
{
    Z_CHECK(!_started);
    Z_CHECK(!_socket.isNull() && (_socket->isOpen() || _socket->state() == QAbstractSocket::ConnectingState));

    _started = true;

    if (!_signals_connected)
        connectToSocket();

    QTimer* timeout = new QTimer(this);
    timeout->setSingleShot(true);
    timeout->setInterval(_disconnect_timeout * 2);
    connect(timeout, &QTimer::timeout, this, [this]() {
        if (_socket != nullptr && _close_on_finish) {
            _socket->abort();
        }
        setError(Error("timeout"));
        if (_loop != nullptr)
            _loop->quit();

        sender()->deleteLater();
    });
    timeout->start();
}

Error SocketOperation::wait()
{
    if (_finished)
        return _error;

    if (!_started)
        start();

    QTimer wait_timer;
    wait_timer.setSingleShot(true);
    wait_timer.setInterval(_disconnect_timeout * 2);
    connect(&wait_timer, &QTimer::timeout, this, [this]() {
        if (_socket != nullptr && _close_on_finish) {
            _socket->abort();
        }
        setError(Error("timeout"));
        if (_loop != nullptr)
            _loop->quit();
    });
    wait_timer.start();

    _loop = new QEventLoop(this);
    _loop->exec();
    delete _loop;
    _loop = nullptr;

    if (_delete_on_finish)
        deleteLater();

    return _error;
}

void SocketOperation::onConnected()
{
}

void SocketOperation::onDisconnected()
{
    if (_delete_on_finish)
        deleteLater();
}

void SocketOperation::onError()
{
    if (!_started || _finished)
        return;

    setError(Error(_socket->errorString()));
}

void SocketOperation::connectToSocket()
{
    _signals_connected = true;
    connect(_socket, &QAbstractSocket::connected, this, &SocketOperation::onConnected);
    connect(_socket, &QAbstractSocket::disconnected, this, &SocketOperation::onDisconnected);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this, &SocketOperation::onError);
#else
    connect(_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::errorOccurred), this, &SocketOperation::onError);
#endif
}

void SocketOperation::disconnectFromSocket()
{
    _signals_connected = false;
    disconnect(_socket, &QAbstractSocket::connected, this, &SocketOperation::onConnected);
    disconnect(_socket, &QAbstractSocket::disconnected, this, &SocketOperation::onDisconnected);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    disconnect(_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this,
               &SocketOperation::onError);
#else
    disconnect(_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::errorOccurred), this,
               &SocketOperation::onError);
#endif
}

void SocketOperation::finish()
{
    if (_finished)
        return;

    _finished = true;

    if (_signals_connected)
        disconnectFromSocket();

    if (_close_on_finish) {
        if (_socket != nullptr && _socket->state() == QAbstractSocket::ConnectedState) {
            connect(_socket, &QAbstractSocket::destroyed, this, &SocketOperation::sl_socket_closed, Qt::DirectConnection);
            connect(_socket, &QAbstractSocket::disconnected, this, &SocketOperation::sl_socket_closed, Qt::DirectConnection);

            _socket->disconnectFromHost();

            _disconnect_timer = new QTimer(this);
            connect(_disconnect_timer, &QTimer::timeout, this, &SocketOperation::sl_socket_closed, Qt::DirectConnection);
            _disconnect_timer->setSingleShot(true);
            _disconnect_timer->setInterval(_disconnect_timeout);
            _disconnect_timer->start();
            return;
        }
    }

    if (_loop != nullptr)
        _loop->quit();
    else if (_delete_on_finish)
        deleteLater();
}

void SocketOperation::sl_socket_closed()
{
    if (_disconnected)
        return;

    _disconnected = true;

    if (_socket != nullptr)
        _socket->abort();

    if (_disconnect_timer != nullptr)
        _disconnect_timer->stop();

    if (_loop == nullptr) {
        if (_delete_on_finish) {
            // откладываем удаление самого себя
            deleteLater();
        }

    } else {
        _loop->quit();
    }
}

quint16 SocketOperation::bufferSize() const
{
    return _buffer_size;
}

bool SocketOperation::isStarted() const
{
    return _started;
}

bool SocketOperation::isFinished() const
{
    return _finished;
}

void SocketOperation::setError(const Error& error)
{
    Z_CHECK(error.isError());

    if (_error.isError()) {
        _error << error;
        return;
    }

    if (_signals_connected)
        disconnectFromSocket();

    if (_socket != nullptr)
        _socket->abort();

    finish();

    _error = Error(QStringLiteral("[%1] %2").arg(_host_info, error.fullText()));
    emit sg_error(_error);
}

void SocketOperation::setDone()
{
    if (_ok_done)
        return;

    if (_signals_connected)
        disconnectFromSocket();
    finish();

    _ok_done = true;
    emit sg_done();
}

SocketHttpReader::SocketHttpReader(const QString& host_info, QAbstractSocket* socket, qint64 read_buffer_size, quint32 disconnect_timeout,
                                   bool close_on_finish, bool delete_on_finish, QObject* parent)
    : SocketOperation(host_info, socket, read_buffer_size, disconnect_timeout, close_on_finish, delete_on_finish, parent)
    , _parcer(Z_MAKE_SHARED(HttpParser, HttpParser::Type::HTTP_BOTH))
{
    if (read_buffer_size > 0)
        socket->setReadBufferSize(read_buffer_size);

    Z_DEBUG_NEW("SocketHttpReader");
}

SocketHttpReader::~SocketHttpReader()
{
    Z_DEBUG_DELETE("SocketHttpReader");    
}

void SocketHttpReader::start()
{
    SocketOperation::start();   

    Z_CHECK(QMetaObject::invokeMethod(this, "sl_readyRead", Qt::QueuedConnection));
}

Error SocketHttpReader::wait(HttpRequestHeader& request, HttpResponseHeader& response)
{
    auto res = SocketOperation::wait();
    request = _request;
    response = _response;
    return res;
}

Error SocketHttpReader::read(HttpRequestHeader& request, const QString& host_info, QAbstractSocket* socket, qint64 read_buffer_size,
                             quint32 disconnect_timeout, bool close_on_finish)
{
    SocketHttpReader reader(host_info, socket, read_buffer_size, disconnect_timeout, close_on_finish);
    HttpResponseHeader response;
    Error error = reader.wait(request, response);
    if (error.isOk() && !request.isValid())
        error = Error("invalid request");
    return error;
}

Error SocketHttpReader::read(HttpResponseHeader& response, const QString& host_info, QAbstractSocket* socket, qint64 read_buffer_size,
                             quint32 disconnect_timeout, bool close_on_finish)
{
    SocketHttpReader reader(host_info, socket, read_buffer_size, disconnect_timeout, close_on_finish);
    HttpRequestHeader request;
    Error error = reader.wait(request, response);
    if (error.isOk() && !response.isValid())
        error = Error("invalid response");
    return error;
}

void SocketHttpReader::connectToSocket()
{
    SocketOperation::connectToSocket();
    connect(socket(), &QAbstractSocket::readyRead, this, &SocketHttpReader::sl_readyRead);
    connect(socket(), &QAbstractSocket::readChannelFinished, this, &SocketHttpReader::sl_readyRead);
}

void SocketHttpReader::disconnectFromSocket()
{
    SocketOperation::disconnectFromSocket();
    disconnect(socket(), &QAbstractSocket::readyRead, this, &SocketHttpReader::sl_readyRead);
    disconnect(socket(), &QAbstractSocket::readChannelFinished, this, &SocketHttpReader::sl_readyRead);
}

void SocketHttpReader::onConnected()
{
    if (isStarted())
        sl_readyRead();
}

void SocketHttpReader::onError()
{
    if (isStarted() && !isFinished())
        _parcer->clear();
    SocketOperation::onError();
}

void SocketHttpReader::sl_readyRead()
{
    if (socket() == nullptr || hasError() || isFinished())
        return;

    if (socket()->state() != QAbstractSocket::ConnectedState) {
        if (!isFinished())
            setError(Error("disconnected"));
        else
            setDone();
        return;
    }

    QString error = _parcer->parse(socket());
    if (!error.isEmpty()) {
        _parcer->clear();
        setError(Error(error));
        return;
    }

    if (!_parcer->isCompleted())
        return;

    if (_parcer->type() == HttpParser::Type::HTTP_REQUEST) {
        _request = HttpRequestHeader(*_parcer);
        emit sg_request(_request);
        setDone();

    } else if (_parcer->type() == HttpParser::Type::HTTP_RESPONSE) {
        _response = HttpResponseHeader(*_parcer);
        emit sg_response(_response);
        setDone();

    } else {
        _parcer->clear();
        setError(Error("SocketReader::sl_readyRead - internal parsing error"));
        return;
    }

    _parcer->clear();
}

SocketWriter::SocketWriter(const QByteArray& data, const QString& host_info, QAbstractSocket* socket, quint16 write_buffer_size,
                           quint32 disconnect_timeout, bool close_on_finish, bool delete_on_finish, QObject* parent)
    : SocketOperation(host_info, socket, write_buffer_size, disconnect_timeout, close_on_finish, delete_on_finish, parent)
    , _data(data)
    , _data_buffer(new QBuffer(&_data))
    , _write_buffer(write_buffer_size, Qt::Uninitialized)
{
    _data_buffer->open(QIODevice::ReadOnly);
    _data_size = _data_buffer->size();

    Z_DEBUG_NEW("SocketWriter");
}

SocketWriter::SocketWriter(const HttpHeader& header, const QString& host_info, QAbstractSocket* socket, quint16 write_buffer_size,
                           quint32 disconnect_timeout, bool close_on_finish, bool delete_on_finish, QObject* parent)
    : SocketWriter(header.toByteArray(), host_info, socket, write_buffer_size, disconnect_timeout, close_on_finish, delete_on_finish,
                   parent)
{
}

SocketWriter::~SocketWriter()
{
    delete _data_buffer;
    Z_DEBUG_DELETE("SocketWriter");
}

QByteArray SocketWriter::data() const
{
    return _data;
}

void SocketWriter::start()
{
    SocketOperation::start();    
    QMetaObject::invokeMethod(this, "sl_send", Qt::QueuedConnection);
}

Error SocketWriter::write(const QByteArray& data, const QString& host_info, QAbstractSocket* socket, quint16 write_buffer_size,
                          quint32 disconnect_timeout, bool close_on_finish)
{
    SocketWriter writer(data, host_info, socket, write_buffer_size, disconnect_timeout, close_on_finish);
    return writer.wait();
}

Error SocketWriter::write(const HttpHeader& data, const QString& host_info, QAbstractSocket* socket, quint16 write_buffer_size,
                          quint32 disconnect_timeout, bool close_on_finish)
{
    Z_CHECK(data.isValid());
    return write(data.toByteArray(), host_info, socket, write_buffer_size, disconnect_timeout, close_on_finish);
}

void SocketWriter::connectToSocket()
{
    SocketOperation::connectToSocket();
    connect(socket(), &QAbstractSocket::bytesWritten, this, &SocketWriter::sl_bytesWritten);
}

void SocketWriter::disconnectFromSocket()
{
    SocketOperation::disconnectFromSocket();
    disconnect(socket(), &QAbstractSocket::bytesWritten, this, &SocketWriter::sl_bytesWritten);
}

void SocketWriter::onConnected()
{
    if (isStarted())
        sl_send();
}

void SocketWriter::onError()
{
    SocketOperation::onError();
}

void SocketWriter::finish()
{
    if (socket() != nullptr && socket()->state() != QAbstractSocket::UnconnectedState)
        socket()->waitForBytesWritten(disconnectTimeout());
    SocketOperation::finish();
}

void SocketWriter::sl_send()
{
    if (socket() == nullptr || isFinished() || socket()->state() != QAbstractSocket::ConnectedState)
        return;

    if (isAllWriten() && !isFinished()) {
        setDone();
        return;
    }

    if (socket()->state() != QAbstractSocket::ConnectedState) {
        setDone();
        return;
    }

    qint64 read = _data_buffer->read(_write_buffer.data(), _write_buffer.size());
    Z_CHECK(read >= 0);

    _has_read += read;
    qint64 written = socket()->write(_write_buffer.constData(), read);
    if (written < 0) {
        if (!socket()->errorString().isEmpty())
            setError(Error(socket()->errorString()));
        else
            setError(Error("Unable to send data"));
        return;
    }
    if (written != read) {
        if (!socket()->errorString().isEmpty())
            setError(Error(socket()->errorString()));
        else
            setError(Error("Internal error while filling write buffer"));
        return;
    }
}

void SocketWriter::sl_bytesWritten(qint64 len)
{
    if (!isStarted() || isFinished())
        return;

    if (socket()->bytesToWrite() < _write_buffer.size() / 2) {
        // the transmit buffer is running low, refill
        sl_send();
    }
    _has_written += len;
    emit sg_progressed((_has_written * 100) / _data_size);

    if (!isFinished() && isAllWriten()) {
        setDone();
    }
}

bool SocketWriter::isAllWriten() const
{
    return _data_size == 0 || _has_written == _data_size || _data_buffer->atEnd();
}

TcpSocket::TcpSocket(QObject* parent)
    : QTcpSocket(parent)
{
    Z_DEBUG_NEW("TcpSocket");
}

TcpSocket::~TcpSocket()
{
    Z_DEBUG_DELETE("TcpSocket");
}

SslSocket::SslSocket(QObject* parent)
    : QSslSocket(parent)
{
}

SslSocket::~SslSocket()
{
}

} // namespace zf::http
