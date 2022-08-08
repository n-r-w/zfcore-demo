#pragma once

#include "zf_global.h"

#include <QObject>
#include <QMutex>
#include <QHostAddress>
#include <QPointer>
#include <QElapsedTimer>
#include <memory>

#include "zf_error.h"
#include "zf_http_headers.h"
#include "zf_thread_controller.h"
#include "zf_socket_operations.h"
#include "zf_rest_tcp_server.h"
#include "zf_rest_session.h"
#include "zf_rest_config.h"
#include "zf_message.h"

#define Z_REST_SERVER_DEBUG 0

class QTcpServer;
class QTcpSocket;

namespace zf
{
namespace http
{
    /*! Абстрактный класс для создания HTTP REST сервиса
     * Проверка прав доступа пользователей в настоящий момент не реализована
     * Поддерживается HTTP и HTTPS (при условии передачи в параметрах ключа RSA и сертификата)
     * Протокол:
     * 1. Клиент шлет запрос POST с body, содержащим информацию о параметрах запроса (формат зависит от реализации)
     *    Заголовок Accept должен содержать номер версии протокола в виде: version=x.x
     *    Заголовок Authorization: Basic <данные авторизации (зависит от реализации)>. Обязательность Authorization и
     * корректность данных авторизации проверяет реализация thread::Worker, созданная через RestServer::createTask
     * 2. Сервер отвечает:
     *   Если готов принять задачу, то 202 (Accepted) text/plain с body, содержащим ID сессии
     *   Если не готов принять задачу, то с  text/plain body, содержащим текст ошибки и одним из кодов 4xx, 5xx
     *   Если количество выполняемых задач превышает лимит, то 504 (GatewayTimeout) text/plain с body, содержащим текст
     * ошибки Если количество входящих соединений превышает лимит, то они закрываются без обратной связи
     * 3. Клиент периодически шлет запрос GET с text/plain body, содержащим ID сессии
     * 4. Сервер в ответ:
     *      Если результат готов, то отвечает 201 (Created) с body, содержащим результат выполнения (формат зависит от
     * реализации) Если результат не готов, то отвечает 204 (No Content) с пустым body.
     *      Если произошла ошибка или не пройдена авторизация, то отвечает с text/plain body, содержащим текст ошибки и
     * одним из кодов 4xx, 5xx */
    class ZCORESHARED_EXPORT RestServer : public QObject
    {
        Q_OBJECT
    public:
        RestServer(QObject* parent = nullptr);
        ~RestServer();

        //! Запуск
        Error start(
            //! Настройки
            const RestConfiguration& config,
            //! Порт
            quint16 port,
            //! Адрес
            const QHostAddress& address = QHostAddress::LocalHost,
            //! Файл ключа (RCA) - если задано, то будет создан SSL сервер
            const QString& key_file = QString(),
            //! Файл сертификата - если задано, то будет создан SSL сервер
            const QString& certificate_file = QString());
        //! Остановка
        void stop();
        //! Запущен ли сервер
        bool isStarted() const;

        //! Останавливает передачу новых задач на обработку (не влияет на обработку входящих соединений)
        void pause();
        //! Восстанавливает передачу новых задач на обработку (не влияет на обработку входящих соединений)
        void resume();
        //! Выполнена ли остановка на паузу
        bool isPaused() const;

        //! Количество обрабатываемых задач, включая вычисляемые и уже вычисленные
        int sessionCount() const;
        //! Количество задач, которые сейчас вычисляются либо переданы в пул потоков, но пока фактически не запущены
        int taskCount() const;
        //! Количество задач, которые сейчас вычисляются (запущены соответствующие потоки)
        int processingTaskCount() const;

        //! Результат выполнения сессии по ее id. Если сессия не найдена, то nullptr
        SessionResultPtr sessionResult(const QString& session_id) const;

        //! Адрес
        QHostAddress host() const;
        //! Порт
        quint16 port() const;

        //! Настройки
        const RestConfiguration* config() const;
        //! Параметры SSL
        const QSslConfiguration* sslConfiguration() const;

        //! Вывод информации в журнал. При переопределении обеспечить потокобезопасность
        virtual void log(const QString& text) const;
        //! Вывод информации в журнал (ошибка). При переопределении обеспечить потокобезопасность
        virtual void log(const zf::Error& error) const;

        //! Создание нового соединения
        virtual RestConnection* createConnectionObject(
            //! Идентификатор сокета
            qintptr socket_descriptor, QObject* parent) const;

        /*! Отправить запрос на предмет прав доступа к выполнению задачи. Возвращает message_id
         * Ответ должен поступить через вызов sg_requestAccessRightsFeedback
         * Реализация данного метода может как сразу вызвать sg_requestAccessRightsFeedback (тогда надо самому сгенерировать message_id,
         * так и отправить асинхронный запрос
         * Необходимо обеспечить потокобезопасность метода */
        virtual zf::MessageID requestAccessRights(const zf::http::HttpRequestHeader& request, const QString& login) const;
        //! Ответ на запрос о правах доступа через checkRequestAccessRights. Соединять только через Qt::QueueConnection, т.к. сигнал может
        //! быть вызван внутри requestAccessRights
        Q_SIGNAL void sg_requestAccessRightsFeedback(
            //! Идентификатор сообщения
            zf::MessageID message_id,
            //! Есть ли права
            bool accepted,
            //! Ошибка запроса прав, если она была (например сетевая ошибка)
            zf::Error error);

        //! Загрузка информации из конфигурационного файла
        static Error loadConfig(const QString& config_file, RestConfiguration& config, Credentials& credentials, QHostAddress& address,
                                quint64& port, QString& key, QString& certificate);

    protected:
        //! Начальная инициализация перез запуском. В момент вызова еще ничего не инициализировано
        virtual Error initialization();

        //! Создать обработчик данных, наследник thread::Worker
        virtual thread::Worker* createTask(const QString& session_id,
                                           //! Информация о клиенте
                                           const QString& client_info,
                                           //! Адрес сетевого интерфеса соединения
                                           const QString& ip_interface_address)
            = 0;
        //! Определить HTTP content type для результата работы обрабочика
        virtual zf::http::ContentType getTaskResultContentType(qint64 status, QByteArrayPtr data) = 0;
        //! Определить HTTP код статуса для ошибки обрабочика
        virtual StatusCode getTaskErrorStatus(qint64 status) = 0;

        //! Задача была запущена (по факту, в не помещена в очередь пула потоков)
        virtual void onTaskStarted(const QString& session_id);
        //! Ответ от обрабочика задачи
        virtual void onTaskFeedback(const QString& session_id, qint64 status, QVariantPtr data);
        //! Ошибка обрабочика задачи
        virtual void onTaskError(const QString& session_id, qint64 status, const QString& error_text);
        //! Задача обрабочика завершена
        virtual void onTaskFinished(const QString& session_id);

    signals:
        //! Поступил новый запрос
        void sg_newTask(const QString& id_session);
        //! Ошибка при обработке запроса
        void sg_taskError(const QString& id_session, const QString& error_text);
        //! Запрос еще не готов
        void sg_taskNotReady(const QString& id_session);
        //! Запрос завершен
        void sg_taskCompleted(const QString& isl_connection_accept_errord_session);

    private slots:
        //! Ошибка подтверждения начала новой сессии
        void sl_connection_accept_error(QAbstractSocket::SocketError error);
        //! Задача была запущена (по факту, в не помещена в очередь пула потоков)
        void sl_task_started(const QString& session_id);
        //! Ответ от обрабочика задачи
        void sl_task_feedback(const QString& session_id, qint64 status, zf::QVariantPtr data);
        //! Ошибка обрабочика задачи
        void sl_task_error(const QString& session_id, qint64 status, const QString& error_text);
        //! Задача обрабочика завершена
        void sl_task_finished(const QString& session_id);

        //! Слишком много сессий
        void sl_tooManySessions(
            //! Информация о клиенте
            const QString& client_info);
        //! Слишком много задач
        void sl_tooManyTasks(
            //! Информация о клиенте
            const QString& client_info);

        //! Ошибка работы с сокетом
        void sl_connection_socketError(const QString& session_id,
                                       //! Информация о клиенте
                                       const QString& client_info, const zf::Error& error);
        //! Принят запрос на выполнение
        void sl_connection_taskCreated(
            //! id новой сессии, который надо дальше использовать при создании ее объекта
            const QString& session_id,
            //! запрос, на основании которого создана новая сессия
            const zf::http::HttpRequestHeader& request,
            //! Адрес сетевого интерфейса на котором произошло подключение
            const QString& ip_interface_address,
            //! Информация о клиенте
            const QString& client_info);
        //! Некорректный запрос от клиента
        void sl_connection_badRequest(
            //! Номер сессии для которой был некорректный запрос. Может быть пустым, если это первичный запрос
            const QString& session_id,
            //! Информация о клиенте
            const QString& client_info, const zf::Error& error);
        //! Клиент прислал запрос на получение результата, то такой сессии нет
        void sl_connection_sessionNotFound(const QString& session_id,
                                           //! Информация о клиенте
                                           const QString& client_info);
        //! Результат выполнения задачи отправлен клиенту. После этого сессию можно закрывать
        void sl_connection_resultDataDelivered(const QString& session_id,
                                               //! Информация о клиенте
                                               const QString& client_info);
        //! Ошибка выполнения задачи отправлена клиенту. После этого сессию можно закрывать
        void sl_connection_resultErrorDelivered(const QString& session_id,
                                                //! Информация о клиенте
                                                const QString& client_info);
        //! Соединение закрыто
        void sl_connectionClosed(const QString& session_id);

    private:
        //! Вызывается для увеличения счетчика задач не дожидаясь поступления сигнала
        void taskCountIncreased();
        //! Вызывается для уменьшения счетчика задач не дожидаясь поступления сигнала
        void taskCountDecreased();

        void initSslConfuguration(const QString& key_file, const QString& certificate_file);

        //! Вывод в журнал
        void logSession(const QString& session_id, const QString& message) const;

        //! Вывод в журнал
        void logClient(const QString& client_info, const QString& message) const;

        //! Удалить сессию
        bool removeSession(const QString& session_id) const;

        //! Передать запрос на обработку
        void processTaskRequest(
            //! id новой сессии, который надо дальше использовать при создании ее объекта
            const QString& session_id,
            //! запрос, на основании которого создана новая сессия
            const HttpRequestHeader& request,
            //! Адрес сетевого интерфейса на котором произошло подключение
            const QString& ip_interface_address,
            //! Информация о клиенте
            const QString& client_info);

        SessionPtr getSessionById(const QString& session_id) const;

        //! Очередь запросов на обработку задач
        struct TaskRequestInfo
        {
            QString session_id;
            HttpRequestHeader request;
            QString ip_interface_address;
            QString client_info;
        };
        QQueue<std::shared_ptr<TaskRequestInfo>> _inbound_task_request_queue;

        //! Настройки
        RestConfiguration _config;

        //! Параметры SSL
        std::unique_ptr<QSslConfiguration> _ssl_config;

        //! TCP сервер
        RestTcpServer* _tcp_server = nullptr;
        //! Менеджер потоков для обработки задач расчета
        thread::Controller* _task_controller = nullptr;
        //! Запросы, которые надо передать на обработку. Ключ - id задачи
        QHash<QString, QVariantPtr> _pending_requests;

        //! Управление сессиями
        std::unique_ptr<SessionManager> _session_manager;

        //! Вывод в консоль информации о превышении количества сессий не чаще чем указано
        QElapsedTimer _too_many_session_info;

        //! Количество новых задач, которые были отправлены на обработку, но пока не получены в sl_task_created, т.к. идут через
        //! Qt::QueueConnection
        QAtomicInt _posted_new_task_count = 0;

        //! Счетчик запросов постановки на паузу (не влияет на обработку входящих соединений)
        int _pause_counter = 0;

        friend class RestConnection;
        friend class RestTcpServer;
    };

} // namespace http
} // namespace zf
