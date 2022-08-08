#pragma once

#include "zf_global.h"

#include <QObject>
#include <QThreadPool>
#include <QMap>
#include <QSet>

#include "zf_thread_worker.h"

#define Z_THREAD_CONTROLLER_DEBUG 0

namespace zf::thread
{
//! Менеджер выполняемых задач
class ZCORESHARED_EXPORT Controller : public QObject
{
    Q_OBJECT
public:
    Controller(
        //! Таймаут ожидания завершения работы потоков. Если <=0 то бесконечно
        int terminate_timeout_ms,
        /*! Максимальное количество одновременно выполняемых задач (размер пула потоков)
         * Если 0, то определяется по количеству CPU */
        int max_thread_count /*= 0*/,
        //! Время ожидания для внутренних операций потока (используется для контроля максимального времени перевода
        //! объектов из одного потока в другой
        int thread_timeout_ms /*= 10000*/, QObject* parent /*= nullptr*/);
    ~Controller();

    //! Задать максимальное количество одновременно выполняемых задач
    void setMaxThreadCount(int count);
    //! Максимальное количество одновременно выполняемых задач
    int maxThreadCount() const;

    //! Время ожидания для внутренних операций потока (используется для контроля максимального времени перевода
    //! объектов из одного потока в другой
    int threadTimeout() const;
    void setThreadTimeout(int thread_timeout_ms);

    //! Общее количество выполняемых задач
    int count() const;
    //! Сколько задач реально выполняется, а не просто висит в ожидании свободного потока в пуле
    int activeCount() const;

    //! Возвращает id задачи с наименьшим количеством входящих запросов
    QString bestTask() const;

    //! Содержит задачу с таким id
    bool containts(const QString& id) const;
    //! Получить указатель на задачу с указанным id. Использовать осторожно, т.к. совершенно не факт что задача не будет удалена в процессе обращения к указателю
    Worker* worker(const QString& id) const;

    //! Идентификаторы всех задач
    QStringList id() const;

    //! Запустить задачу. Возвращает Worker::id либо пустую строку если поток запустить не удалось (например нет ресурсов)
    QString startWorker(Worker* worker);
    //! Остановить задачу по id
    void cancellWorker(const QString& id);
    //! Остановить все задачи
    void cancelAll();

    //! Запрошена остановка всех потоков
    bool isCancellRequested() const;

    //! Запросить ответ от задачи
    void request(const QString& id, QVariantPtr request);
    //! Запросить ответ от всех задач
    void requestAll(QVariantPtr request);

    //! Поместить входящее сообщение
    void putExternalFeedback(const QString& id, const zf::Uid& sender, const zf::Message& message);
    //! Поместить входящее сообщение для всех задач
    void putExternalFeedbackAll(const zf::Uid& sender, const zf::Message& message);

signals:
    //! Создан новый Worker, но пока не запущен в потоке. При connect к worker использовать Qt::QueueConnection, т.к. в этот момент он еще
    //! не запущен в отдельном потоке
    void sg_created(zf::thread::Worker* worker);
    //! Задача была запущена (по факту, в не помещена в очередь пула потоков)
    void sg_started(const QString& id);
    //! Процент выполнения задачи
    void sg_progress(const QString& id, int percent);
    //! Задача была остановлена до ее выполнения
    void sg_cancelled(const QString& id);
    //! Задача завершена
    void sg_finished(const QString& id);
    //! Ответ на запрос
    void sg_data(const QString& id, qint64 status, zf::QVariantPtr data);
    //! Ошибка
    void sg_error(const QString& id, qint64 status, const QString& error_text);
    //! Информирует поток, о том что объект переведен в поток или была ошибка. Ответ на запрос requestMoveToThread
    void sg_objectMovedToThread(QString worker_id, bool is_success);

private slots:
    //! Задача завершена
    void sl_worker_finished(QString id);

private:
    //! Запросить перевод объекта в поток воркера
    Q_SLOT void requestMoveToThread(QObject* object, QThread* worker_thread, QString worker_id);

    //! Пул потоков
    QThreadPool* _pool;
    //! Таймаут ожидания завершения работы потоков
    int _terminate_timeout_ms;
    //! Время ожидания в мс для внутренних операций потока (используется для контроля максимального времени перевода
    //! объектов из одного потока в другой
    int _thread_timeout;
    mutable QMutex _thread_timeout_mutex;

    //! Список выполняемых задач
    QMap<QString, Worker*> _workers;
    //! Ожидают остановки
    QSet<QString> _waiting_to_cancel;

    //! Запрошена остановка всех потоков
    bool _cancell_requested = false;
};

} // namespace zf::thread

#if Z_THREAD_CONTROLLER_DEBUG > 0
#define Z_THREAD_CONTROLLER_DEBUG_LOG(_S_, _I_) (zf::Utils::infoToConsole(QStringLiteral("THREAD DEBUG [%1][%2]").arg(_S_).arg(_I_)))
#else
#define Z_THREAD_CONTROLLER_DEBUG_LOG(_S_, _I_) while (false)
#endif
