#pragma once

#include <QMutexLocker>
#include <QDateTime>
#include <QCache>
#include <QTimer>

#include "zf_error.h"
#include "zf_http_headers.h"

namespace zf
{
namespace http
{
class RestServer;

//! Результат выполнения задачи
class SessionResult
{
public:
    SessionResult();
    SessionResult(const SessionResult& r);
    SessionResult(const QString& session_id, ContentType content_type, QByteArrayPtr data);
    SessionResult(const QString& session_id, const Error& error, StatusCode error_status);
    ~SessionResult();

    bool isValid() const;
    //! Ошибка при расчете
    bool isError() const;

    //! Идентификатор сессии
    QString sessionId() const;

    //! HTTP content type для task_result_data
    ContentType contentType() const;
    //! Данные, которые подготовил обработчик
    QByteArrayPtr data() const;

    //! Ошибка, которую сгенерировал обработчик
    Error error() const;
    //! Статус ошибки
    StatusCode errorStatus() const;

private:
    //! Идентификатор сессии
    QString _session_id;
    //! HTTP content type для task_result_data
    ContentType _content_type = ContentType::Unknown;
    //! Данные, которые подготовил обработчик
    QByteArrayPtr _data;
    //! Ошибка, которую сгенерировал обработчик
    Error _error;
    //! Статус ошибки
    StatusCode _error_status = StatusCode::Ok;

    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::http::SessionResult& res);
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::http::SessionResult& res);
};
typedef std::shared_ptr<SessionResult> SessionResultPtr;

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::http::SessionResult& res);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::http::SessionResult& res);

/*! Информация о сессии. Потокобезопасна
 * Хранится до момента запроса клиентом результатов или по истечении таймаута */
class Session : public QObject
{
    Q_OBJECT
public:
    ~Session();

    //! Уникальный номер сессии
    QString sessionId() const;

    //! Можно ли удалить сессию
    bool canRemove() const;

    //! Сессия закрыта и будет удалена
    bool isSessionClosed() const;
    //! Установить флаг закрытия сессии
    void closeSession();

    //! Закончилось ли время жизни
    bool isTimeToDie() const;
    //! Время жизни расчитанной сессии в секундах
    quint64 lifetime() const;

    //! Время создания
    QDateTime created() const;

    //! Адрес сетевого интерфейса на котором произошло подключение
    const QString ipInterfaceAddress() const;
    //! Клиент
    QString clientInfo() const;

    //! Обработчик задачи завершен и становлен результат или ошибка
    bool isTaskCompleted() const;

    //! Установить результат выполнения задачи. Для корректной работы с потоками можно выполнить только один раз
    void setTaskResult(ContentType content_type, QByteArrayPtr data);
    //! Установить ошибку результата выполнения задачи. Для корректной работы с потоками можно выполнить только один раз
    void setTaskError(const Error& error, StatusCode error_status);

    /*! Результат выполнения задачи. Для корректной работы с потоками, результат можно забрать только один раз,
     * после этого он удаляется из сессии */
    SessionResultPtr takeResult() const;
    //! Был ли забран результат выполнения задачи
    bool isResultTaken() const;

signals:
    //! Установлен результат расчета
    void sg_resultInstalled(const QString& session_id);
    //! Результат расчета изъят
    void sg_resultTaken(const QString& session_id);
    //! Сессия закрыта и должна быть удалена
    void sg_closed(const QString& session_id);

private:
    Session();
    Session(
        //! Адрес сетевого интерфейса на котором произошло подключение
        const QString ip_interface_address,
        //! Клиент
        const QString& client_info,
        //! id сессии
        const QString& session_id,
        //! Время жизни расчитанной сессии в секундах
        quint64 lifetime,
        //! Когда была создана
        const QDateTime& created);

    //! Сохранить в файл
    Error saveToFile(const QString& file_name) const;
    //! Создать объект из файла
    static std::shared_ptr<Session> loadFromFile(const QString& file_name, Error& error);

    mutable QMutex _mutex;

    //! Клиент
    QString _client_info;
    //! Адрес сетевого интерфейса на котором произошло подключение
    QString _ip_interface_address;

    //! Уникальный номер сессии
    QString _session_id;
    //! Сессия закрыта и будет удалена
    bool _closed = false;

    //! Когда была создана
    QDateTime _created;
    //! Время жизни расчитанной сессии в секундах
    quint64 _lifetime = 0;

    //! Результат выполнения задачи
    mutable SessionResultPtr _result;
    //! Результат был установлен
    bool _result_was_set = false;
    //! Результат был изьят
    mutable bool _result_taken = false;

    friend class SessionManager;
};
typedef std::shared_ptr<Session> SessionPtr;

//! Механизм кэширования сессий. Отслеживается sg_deleted и на основании этого очищается SessionInfo::session
class SessionCacheItem : public QObject
{
    Q_OBJECT
public:
    SessionCacheItem(const QString& session_id);
    ~SessionCacheItem();

    Q_SIGNAL void sg_deleted(const QString& session_id);

private:
    QString _session_id;
};

//! Управление сессиями
class SessionManager : public QObject
{
    Q_OBJECT
public:
    SessionManager(RestServer* server);
    ~SessionManager();

    //! Обновить конфигурацию на основании сервера
    void updateConfig();

    //! Получить сессию. Если такой нет, то nullptr
    SessionPtr getSession(const QString& session_id);

    //! Зарегистрировать новую сессию
    SessionPtr registerNewSession(
        //! Адрес сетевого интерфейса на котором произошло подключение
        const QString& ip_interface_address,
        //! Клиент
        const QString& client_info,
        //! id сессии
        const QString& session_id,
        //! Время жизни расчитанной сессии в секундах
        quint64 lifetime);

    //! Удалить сессию
    Error removeSession(const QString& session_id);

    /*! Проверить какие сессии устарели и удалить их */
    Error collectGarbage();
    //! Сохранить информацию о сессиях на диск
    Error flush();
    //! Активировать таймер сброса сессий на диск
    void requestFlush();

    //! Папка, где хранятся файла сессий
    static QString sessionManagerFolder();

    //! Общее количество сессий
    int sessionCount() const;
    //! Количество сессий, результаты расчета которых загружены в память
    int sessionInMemoryCount() const;

    //! Загрузить все сессии с диска (начальная инициализация)
    Error loadSessionsFromDisk();

private slots:
    //! Установлен результат расчета сессии
    void sl_sessionResultInstalled(const QString& session_id);

    //! Был удален элемент кэша
    void sl_cacheItemDeleted(const QString& session_id);

private:
    //! Имя файла с путем для сессии
    static QString sessionFileName(const QString& session_id);
    //! Код сессии по имени файла
    static QString sessionIdFromFileName(const QString& file_name);

    //! Зарегистрировать новую сессию
    SessionPtr registerNewSessionHelper(const SessionPtr& session);

    //! Сохранить сессию на диск и убрать из памяти
    Error flushSession(const QString& session_id);
    //! Загрузить сессию с диска
    Error loadSession(const QString& session_id);

    mutable Z_RECURSIVE_MUTEX _mutex;

    //! Сервер
    RestServer* _server;

    //! Информация о сессии
    struct SessionInfo
    {
        //! Уникальный номер сессии
        QString session_id;
        //! Когда была создана
        QDateTime created;
        //! Время жизни расчитанной сессии в секундах
        quint64 lifetime = 0;
        //! Указатель на объект сессию (может быть nullptr, если saved == true)
        SessionPtr session;

        //! Закончилось ли время жизни
        bool isTimeToDie() const;
    };
    typedef std::shared_ptr<SessionInfo> SessionInfoPtr;

    //! Получить информацию о сессии
    SessionInfoPtr sessionInfo(const QString& session_id) const;
    //! Указатель на список сессий (ленивая инициализация). При первом обращении считывает все сессии, сохраненные на диске
    QHash<QString, SessionInfoPtr>* getSessions() const;

    //! Информация о сессиях
    mutable std::unique_ptr<QHash<QString, SessionInfoPtr>> _sessions;

    //! Зарегистрировать сессию в кэше
    Error registerInCache(const QString& session_id);
    //! Удалить из кэша
    void removeFromCache(const QString& session_id);

    //! Ключ - session id
    QCache<QString, SessionCacheItem> _session_cache;

    //! Ошибки при удалении из кэша и сохранении на диск
    Error _last_remove_cache_errors;

    QTimer _collect_garbage_timer;
    QTimer _flush_timer;
};

} // namespace http

} // namespace zf
