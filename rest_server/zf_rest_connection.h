#pragma once

#include <QEventLoop>
#include <QSslConfiguration>

#include "zf_error.h"
#include "zf_thread_worker.h"
#include "zf_http_headers.h"
#include "zf_rest_session.h"
#include "zf_version.h"
#include "zf_message.h"

namespace zf::http
{
class RestServer;
class RestConnection;
class RestConnectionWorker;

//! Класс для взаимодействия с RestConnectionWorker через сигналы/слоты
class RestConnectionWorkerObject : public zf::thread::WorkerObject
{
    Q_OBJECT
public:
    RestConnectionWorkerObject();

    //! Worker, который относится к данному объекту
    RestConnectionWorker* worker() const;

    //! Подключиться к сигналам соединения
    void setupConnection(RestConnection* connection);

signals:
    //! Слишком много сессий
    void sg_tooManySessions(
        //! Информация о клиенте
        const QString& client_info);
    //! Слишком много задач
    void sg_tooManyTasks(
        //! Информация о клиенте
        const QString& client_info);

    //! Ошибка создания сокета
    void sg_socketCreationError(zf::Error error);
    //! Соединение закрыто
    void sg_connectionClosed(
        //! id сессии - может быть пустым, если сессия не создавалась
        QString session_id);
    //! Ошибка работы с сокетом
    void sg_socketError(
        //! id сессии
        QString session_id,
        //! Информация о клиенте
        const QString& client_info, zf::Error error);
    //! Принят запрос на выполнение
    void sg_taskCreated(
        //! Генерируется ID новой сессии, который надо дальше использовать при создании ее объекта
        QString session_id,
        //! запрос, на основании которого создана новая сессия
        zf::http::HttpRequestHeader request,
        //! Адрес сетевого интерфейса на котором произошло подключение
        const QString& ip_interface_address,
        //! Информация о клиенте
        const QString& client_info);
    //! Некорректный запрос от клиента
    void sg_badRequest(
        //! Номер сессии для которой был некорректный запрос. Может быть пустым, если это первичный запрос
        QString session_id,
        //! Информация о клиенте
        const QString& client_info, zf::Error error);
    //! Клиент запросил результат для сессии, но она не найдена
    void sg_sessionNotFound(QString session_id,
                            //! Информация о клиенте
                            const QString& client_info);
    //! Результат выполнения задачи отправлен клиенту. После этого сессию можно закрывать
    void sg_resultDataDelivered(QString session_id,
                                //! Информация о клиенте
                                const QString& client_info);
    //! Ошибка выполнения задачи отправлена клиенту. После этого сессию можно закрывать
    void sg_resultErrorDelivered(QString session_id,
                                 //! Информация о клиенте
                                 const QString& client_info);

    friend class RestConnectionWorker;
};

//! Соединение с клиентом. Активно только на время открытго сокета
class RestConnectionWorker : public zf::thread::Worker
{    
public:
    RestConnectionWorker(
        //! Основной сервер
        RestServer* rest_server);
    ~RestConnectionWorker();

    RestConnectionWorkerObject* object() const;

protected:    
    //! Вызывается при запросе остановки
    void onCancell() override;
    //! Вызывается при запросе на обработку данных
    void onRequest() override;

private:
    //! Объект-соединение был уничтожен
    void onConnectionDestroyed(QObject* connection);

    //! Основной сервер
    RestServer* _rest_server;
    //! Соединения
    QSet<QObject*> _connections;

    friend class RestConnectionWorkerObject;
};

//! Соединение с клиентом. Активно только на время открытго сокета
class ZCORESHARED_EXPORT RestConnection : public QObject
{
    Q_OBJECT
public:
    RestConnection(
        //! Основной сервер
        const RestServer* rest_server,
        //! Идентификатор сокета
        qintptr socket_descriptor, QObject* parent);
    ~RestConnection();

    enum class InitialStatus
    {
        //! Нормальная обработка
        OK,
        //! Превышен лимит сессий
        SessionsLimitError,
        //! Превышен лимит задач
        TasksLimitError,
    };

    //! Вызывается до передачи в пул потоков. Состояние соединения
    void setInitialStatus(InitialStatus s);

    //! Запуск
    void process();
    //! Запрос остановки
    void cancell();

protected:
    //! Состояние соединения
    InitialStatus initialStatus() const;

signals:
    //! Слишком много сессий
    void sg_tooManySessions(
        //! Информация о клиенте
        const QString& client_info);
    //! Слишком много задач
    void sg_tooManyTasks(
        //! Информация о клиенте
        const QString& client_info);

    //! Ошибка создания сокета
    void sg_socketCreationError(zf::Error error);
    //! Соединение закрыто
    void sg_connectionClosed(
        //! id сессии - может быть пустым, если сессия не создавалась
        QString session_id);    
    //! Ошибка работы с сокетом
    void sg_socketError(
        //! id сессии
        QString session_id,
        //! Информация о клиенте
        const QString& client_info, zf::Error error);
    //! Принят запрос на выполнение
    void sg_taskCreated(
        //! Генерируется ID новой сессии, который надо дальше использовать при создании ее объекта
        QString session_id,
        //! запрос, на основании которого создана новая сессия
        zf::http::HttpRequestHeader request,
        //! Адрес сетевого интерфейса на котором произошло подключение
        const QString& ip_interface_address,
        //! Информация о клиенте
        const QString& client_info);
    //! Некорректный запрос от клиента
    void sg_badRequest(
        //! Номер сессии для которой был некорректный запрос. Может быть пустым, если это первичный запрос
        QString session_id,
        //! Информация о клиенте
        const QString& client_info, zf::Error error);
    //! Клиент запросил результат для сессии, но она не найдена
    void sg_sessionNotFound(QString session_id,
        //! Информация о клиенте
        const QString& client_info);
    //! Результат выполнения задачи отправлен клиенту. После этого сессию можно закрывать
    void sg_resultDataDelivered(QString session_id,
        //! Информация о клиенте
        const QString& client_info);
    //! Ошибка выполнения задачи отправлена клиенту. После этого сессию можно закрывать
    void sg_resultErrorDelivered(QString session_id,
        //! Информация о клиенте
        const QString& client_info);

private slots:
    void sl_sslErrors(const QList<QSslError>& errors);

private:
    Q_SLOT void start();

    //! Проверка прав доступа
    void checkAccessRights(const HttpRequestHeader& request);
    //! Обработать начальный запрос на обработку данных
    void processInitialRequest(const HttpRequestHeader& request);
    //! Обработать запрос на получение результата
    void processGetResultRequest(const HttpRequestHeader& request);

    //! Некорректный запрос от клиента
    void onBadRequest(const Error& error);
    //! Принят первичный запрос от клиента
    void onRequestAccept(const HttpRequestHeader& request);
    //! Клиент прислал запрос на получение результата, то такой сессии нет
    void onSessionNotFound();
    //! Клиент прислал запрос на получение результата, и ему надо его отправить
    void onResult(const SessionResultPtr& result);

    //! Соединение было закрыто
    void onConnectionClosed();

    //! Основной сервер
    RestServer* _rest_server;
    QPointer<QAbstractSocket> _socket;

    //! id сокета
    qintptr _socket_descriptor;

    QString _session_id;
    QString _client_info;
    //! Адрес сетевого интерфейса на котором произошло подключение
    QString _ip_interface_address;
    zf::MessageID _access_rights_feedback_id;

    //! Вызывается до передачи в пул потоков. Состояние соединения
    InitialStatus _initial_status = InitialStatus::OK;

    bool _cancelled = false;
};
} // namespace zf::http
