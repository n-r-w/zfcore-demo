#pragma once

#include "zf_global.h"
#include "zf_error.h"
#include "zf_http_headers.h"
#include "zf_http_parser.h"

#include <QSslSocket>
#include <QByteArray>
#include <QBuffer>
#include <QPointer>
#include <QEventLoop>

namespace zf
{
namespace http
{
class ZCORESHARED_EXPORT TcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    TcpSocket(QObject* parent = nullptr);
    ~TcpSocket();
};

class ZCORESHARED_EXPORT SslSocket : public QSslSocket
{
    Q_OBJECT
public:
    SslSocket(QObject* parent = nullptr);
    ~SslSocket();
};

//! Запись в сокет по частям
class ZCORESHARED_EXPORT SocketOperation : public QObject
{
    Q_OBJECT
public:
    SocketOperation(
        //! Информация о хосте
        const QString& host_info,
        //! Сокет
        QAbstractSocket* socket,
        //! Размер буфера (байт)
        quint16 buffer_size,
        //! Время ожидания отключения сокета (мс). Имеет смысл только при close_on_finish
        quint32 disconnect_timeout = 5000,
        //! Закрыть сокет по окончании записи/чтения или при ошибке
        bool close_on_finish = false,
        //! Удалить объект по окончании записи/чтения или при ошибке
        bool delete_on_finish = false, QObject* parent = 0);
    ~SocketOperation();

    //! Уникальный код операции
    QString uniqueId() const;

    QAbstractSocket* socket() const;
    //! Информация о хосте
    QString hostInfo() const;

    quint32 disconnectTimeout() const;

    bool hasError() const;
    Error error() const;

    //! Запуск (асинхронно)
    virtual void start();

protected:
    //! Запустить и ждать до окончания записи/чтения или ошибки
    Error wait();

    Q_SLOT virtual void onConnected();
    Q_SLOT virtual void onDisconnected();
    Q_SLOT virtual void onError();

    //! Подключиться к сокету
    virtual void connectToSocket();
    //! Отключить сигналы от сокета
    virtual void disconnectFromSocket();
    //! Завершение работы с сокетом
    virtual void finish();

    quint16 bufferSize() const;
    bool isStarted() const;
    bool isFinished() const;

    void setError(const Error& error);
    void setDone();

signals:
    void sg_done();
    void sg_error(const zf::Error& error);
    void sg_progressed(int percent);

private slots:
    void sl_socket_closed();

private:
    QPointer<QAbstractSocket> _socket;

    QString _id;
    QString _host_info;

    quint32 _disconnect_timeout;
    bool _close_on_finish = false;
    bool _delete_on_finish = false;
    quint16 _buffer_size;

    bool _signals_connected = false;
    bool _started = false;
    bool _finished = false;
    bool _disconnected = false;
    Error _error;
    bool _ok_done = false;

    QEventLoop* _loop = nullptr;
    QTimer* _disconnect_timer = nullptr;
};

class ZCORESHARED_EXPORT SocketHttpReader : public SocketOperation
{
    Q_OBJECT
public:
    explicit SocketHttpReader(
        //! Информация о хосте
        const QString& host_info,
        //! Сокет
        QAbstractSocket* socket,
        //! Размер буфера чтения
        qint64 read_buffer_size,
        //! Время ожидания отключения сокета (мс). Имеет смысл только при close_on_finish
        quint32 disconnect_timeout = 5000,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false,
        //! Удалить объект по окончании записи или при ошибке
        bool delete_on_finish = false, QObject* parent = 0);
    ~SocketHttpReader();

    //! Запуск (асинхронно)
    void start() override;
    //! Запустить и ждать до окончания чтения или ошибки
    Error wait(HttpRequestHeader& request, HttpResponseHeader& response);

    //! Синхронно прочитать входящий HTTP запрос
    static Error read(
        //! HTTP запрос
        HttpRequestHeader& request,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет
        QAbstractSocket* socket,
        //! Размер буфера чтения
        qint64 read_buffer_size,
        //! Время ожидания отключения сокета (мс)
        quint32 disconnect_timeout,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false);

    //! Синхронно прочитать входящий HTTP ответ
    static Error read(
        //! HTTP ответ
        HttpResponseHeader& response,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет
        QAbstractSocket* socket,
        //! Размер буфера чтения
        qint64 read_buffer_size,
        //! Время ожидания отключения сокета (мс)
        quint32 disconnect_timeout,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false);

protected:
    void connectToSocket() override;
    void disconnectFromSocket() override;
    void onConnected() override;
    void onError() override;

signals:
    void sg_request(const zf::http::HttpRequestHeader& request);
    void sg_response(const zf::http::HttpResponseHeader& response);

private slots:
    void sl_readyRead();

private:
    std::shared_ptr<HttpParser> _parcer;
    HttpRequestHeader _request;
    HttpResponseHeader _response;
};

//! Запись в сокет по частям
class ZCORESHARED_EXPORT SocketWriter : public SocketOperation
{
    Q_OBJECT
public:
    explicit SocketWriter(
        //! Данные
        const QByteArray& data,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет для записи (должен быть открыт)
        QAbstractSocket* socket,
        //! Размер буфера записи данных в сокет (байт)
        quint16 write_buffer_size,
        //! Время ожидания отключения сокета (мс). Имеет смысл только при close_on_finish
        quint32 disconnect_timeout = 5000,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false,
        //! Удалить объект по окончании записи или при ошибке
        bool delete_on_finish = false, QObject* parent = 0);
    explicit SocketWriter(
        //! Данные
        const HttpHeader& header,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет для записи (должен быть открыт)
        QAbstractSocket* socket,
        //! Размер буфера записи данных в сокет (байт)
        quint16 write_buffer_size,
        //! Время ожидания отключения сокета (мс). Имеет смысл только при close_on_finish
        quint32 disconnect_timeout = 5000,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false,
        //! Удалить объект по окончании записи или при ошибке
        bool delete_on_finish = false, QObject* parent = 0);
    ~SocketWriter();

    QByteArray data() const;

    //! Запуск (асинхронно)
    void start() override;

    using SocketOperation::wait;

    //! Синхронная запись
    static Error write(
        //! Данные
        const QByteArray& data,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет для записи (должен быть открыт)
        QAbstractSocket* socket,
        //! Размер буфера записи данных в сокет (байт)
        quint16 write_buffer_size,
        //! Время ожидания отключения сокета (мс)
        quint32 disconnect_timeout,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false);

    //! Синхронная запись HTTP заголовка
    static Error write(
        //! Данные
        const HttpHeader& data,
        //! Информация о хосте
        const QString& host_info,
        //! Сокет для записи (должен быть открыт)
        QAbstractSocket* socket,
        //! Размер буфера записи данных в сокет (байт)
        quint16 write_buffer_size,
        //! Время ожидания отключения сокета (мс)
        quint32 disconnect_timeout,
        //! Закрыть сокет по окончании записи или при ошибке
        bool close_on_finish = false);

protected:
    void connectToSocket() override;
    void disconnectFromSocket() override;
    void onConnected() override;
    void onError() override;
    void finish() override;

private slots:
    void sl_send();
    void sl_bytesWritten(qint64 len);

private:
    bool isAllWriten() const;

    QByteArray _data;
    QBuffer* _data_buffer = nullptr;
    QByteArray _write_buffer;

    qint64 _has_read = 0;
    qint64 _has_written = 0;
    qint64 _data_size = 0;
};

} // namespace http
} // namespace zf
