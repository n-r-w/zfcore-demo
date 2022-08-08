#pragma once

#include <QTcpServer>
#include <QSslConfiguration>

#include "zf_error.h"
#include "zf_thread_controller.h"
#include "zf_http_headers.h"

namespace zf
{
namespace http
{
class RestServer;
class RestConnection;

class RestTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    RestTcpServer(
        //! Основной сервер
        RestServer* rest_server, QObject* parent = nullptr);

    void stop();

    static QString clientName(const QAbstractSocket* socket);

protected:        
    //! Основной сервер
    RestServer* restServer() const;    

signals:
    //! Слишком много сессий
    void sg_tooManySessions(
        //! Информация о клиенте
        const QString& client_info);
    //! Слишком много задач
    void sg_tooManyTasks(
        //! Информация о клиенте
        const QString& client_info);

    //! Ошибка работы с сокетом
    void sg_connection_socketError(const QString& session_id,
                                   //! Информация о клиенте
                                   const QString& client_info, zf::Error error);
    //! Принят запрос на выполнение
    void sg_connection_taskCreated(
        //! id новой сессии, который надо дальше использовать при создании ее объекта
        const QString& session_id,
        //! запрос, на основании которого создана новая сессия
        const zf::http::HttpRequestHeader& request,
        //! Адрес сетевого интерфейса на котором произошло подключение
        const QString& ip_interface_address,
        //! Информация о клиенте
        const QString& client_info);
    //! Некорректный запрос от клиента
    void sg_connection_badRequest(
        //! Номер сессии для которой был некорректный запрос. Может быть пустым, если это первичный запрос
        const QString& session_id,
        //! Информация о клиенте
        const QString& client_info, zf::Error error);
    //! Клиент прислал запрос на получение результата, то такой сессии нет
    void sg_connection_sessionNotFound(const QString& session_id,
        //! Информация о клиенте
        const QString& client_info);
    //! Результат выполнения задачи отправлен клиенту. После этого сессию можно закрывать
    void sg_connection_resultDataDelivered(const QString& session_id,
                                           //! Информация о клиенте
                                           const QString& client_info);
    //! Ошибка выполнения задачи отправлена клиенту. После этого сессию можно закрывать
    void sg_connection_resultErrorDelivered(const QString& session_id,
                                            //! Информация о клиенте
                                            const QString& client_info);

    //! Соединение закрыто
    void sg_connectionClosed(const QString& session_id);

private slots:
    //! Создан новый RestConnectionWorker, но пока не запущен в потоке
    void sl_connectionWorkerCreated(thread::Worker* worker);
    //! Соединение закрыто
    void sl_connectionClosed(QString session_id);
    //! Ошибка создания сокета
    void sl_socketCreationError(zf::Error error);

private:
    void incomingConnection(qintptr handle) override;

    //! Менеджер потоков для обработки входящих соединений
    thread::Controller* _connection_controller;
    //! Основной сервер
    RestServer* _rest_server;
};

class RestNonSslTcpServer : public RestTcpServer
{
    Q_OBJECT
public:
    RestNonSslTcpServer(
        //! Основной сервер
        RestServer* rest_server, QObject* parent = nullptr);
};

class RestSslTcpServer : public RestTcpServer
{
    Q_OBJECT
public:
    RestSslTcpServer(
        //! Основной сервер
        RestServer* rest_server, QObject* parent = nullptr);
};

} // namespace http
} // namespace zf
