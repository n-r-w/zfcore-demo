#pragma once

#include "zf.h"
#include "zf_message.h"
#include "zf_object_extension.h"

#include <QObject>
#include <QRunnable>
#include <QMutex>
#include <QQueue>
#include <QEventLoop>

namespace zf::thread
{
class Worker;
class Controller;

//! Класс для взаимодействия с Worker через сигналы/слоты
class ZCORESHARED_EXPORT WorkerObject : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    WorkerObject();
    ~WorkerObject();

    //! Worker, который относится к данному объекту
    Worker* worker() const;

    //! Отложенное удаление
    void deleteLater();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) override;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

signals: // не вызывать в классах наследниках
    //! Задача была запущена (по факту, в не помещена в очередь пула потоков)
    void sg_started(QString id);
    //! Процент выполнения задачи
    void sg_progress(QString id, int percent);
    //! Задача была остановлена до ее выполнения
    void sg_cancelled(QString id);
    //! Задача завершена
    void sg_finished(QString id);
    //! Ответ на запрос
    void sg_data(QString id, qint64 status, zf::QVariantPtr data);
    //! Ошибка
    void sg_error(QString id, qint64 status, QString error_text);

protected:
    //! Запросить остановку задачи
    //! Вызывать только через QMetaObject::invokeMethod Qt::QueueConnection (protected для защиты от дурака)
    Q_SLOT void cancel();
    //! Запросить ответ на некий запрос. Запросы ставятся в очереди на обработку и могут быть получены через getRequest
    //! Вызывать только через QMetaObject::invokeMethod Qt::QueueConnection (protected для защиты от дурака)
    Q_SLOT void request(zf::QVariantPtr request);

    //! Получен ответ на запрос
    Q_SLOT virtual void onData(QString id, qint64 status, zf::QVariantPtr data);

private:
    //! Получено внешнее сообщение
    Q_SLOT void onExternalFeedback(const zf::Uid& sender, const zf::Message& message);

private:
    Worker* _worker = nullptr;
    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
    QMetaObject::Connection _data_connection;

    friend class Worker;
};

/*! Класс задачи (выполняет некий расчет)
 * В процессе расчета (или до его начала) может принимать запросы на обработку данных и позволяет отвечать на них */
class ZCORESHARED_EXPORT Worker : public QRunnable
{    
public:
    Worker(
        //! Запускать QEventLoop. Если используется QEventLoop, то остановка через cancel
        bool use_event_loop,
        //! Объект для взаимодействия через сигналы/слоты. Если не задан, то создается объект по умолчанию
        //! Владение переходит к Worker
        WorkerObject* object = nullptr,
        //! Уникальный идентификатор задачи. Если не задан, то генерируется случайным образом
        const QString& id = QString());
    ~Worker() override;

    //! Идентификатор задачи
    QString id() const;

    //! Задача была запущена на вполнение
    bool isStarted() const;
    //! Задача завершена
    bool isFinished() const;
    //! Задача отменена
    bool isCancelled() const;

    //! Объект для взаимодействия через сигналы/слоты. Все подключения через Qt::QueueConnection
    WorkerObject* object() const;

    //! Количество запросов на обработку, включая забранные на обработку, но еще не обработанные
    //! Увеличивается при вызове request и уменьшается при sendData/sendError/sendDone
    int requestCount() const;

    //! Ждать до окончания выполнения
    bool wait(
        //! Время ожидания (0 - бесконечно)
        int wait_ms = 0) const;

    //! Ждать получения внешнего сообщения. Сначала вызывает регистрацию через registerExternalFeedback, а потом ждет получения через getWaitingExternalFeedback
    zf::Message waitForExternalFeedback(const zf::MessageID& feedback_id,
                                        //! Сколько всего ждать (0 - бесконечно)
                                        int total_msec = 0,
                                        //! Какие делать интервалы между проверками получения отчета
                                        int pause_ms = 50);

    //! Зарегистрироватаь ожидание внешнего сообщения
    void registerExternalFeedback(const zf::MessageID& feedback_id);
    //! Получен ли ответ на внешнее сообщение
    zf::Message getWaitingExternalFeedback(const zf::MessageID& feedback_id) const;

protected:
    //! Расчет задачи (методы вызывается только если в конструкторе не задано use_event_loop)
    virtual void process() {};
    //! Вызывается при запросе остановки
    virtual void onCancell();
    //! Вызывается при запросе на обработку данных.
    //! В начала обязательно вызвать getRequest() для получения данных на обработку
    virtual void onRequest();

    //! Была ли запрошена остановка задачи
    bool isCancelRequested() const;

    //! Получить следующий запрос на обработку. Если запроса нет, то возвращает nullptr
    QVariantPtr getRequest();

    //! Запросить остановку задачи. Вызывать в наследнике Worker, если он работает через eventLoop (use_event_loop ==
    //! true)
    void cancel();

    //! Отправить пустой ответ
    void sendDone() const;
    //! Отправить ответ
    void sendData(
        //! Произвольное значение
        qint64 status = 0,
        //! Данные
        QVariantPtr data = nullptr) const;
    //! Отправить ответ об ошибке
    void sendError(
        //! Произвольное значение
        qint64 status,
        //! Данные
        QString error_text) const;

    //! Указать процент выполнения задачи
    void setProgress(int percent);
    //! Указать, что задача была остановлена до ее выполнения
    void cancelled();

private:
    //! Запуск потока на выполнение
    void run() override;

    void requestHelper(zf::QVariantPtr request);
    void onExternalFeedbackHelper(const zf::Uid& sender, const zf::Message& message);

    //! Контроллер
    Controller* _controller = nullptr;

    mutable int _request_count = 0;
    // Запросы на обработу
    QQueue<QVariantPtr> _data_request;
    //! Блокировка _data_request
    mutable Z_RECURSIVE_MUTEX _data_request_locker;

    // Идентификатор задачи
    QString _id;
    //! Блокировка _id
    mutable QMutex _id_locker;

    //! Объект для взаимодействия через сигналы/слоты
    QPointer<WorkerObject> _object;

    //! Работа через QEventLoop
    bool _use_loop;
    QEventLoop* _main_loop = nullptr;

    // Запрошена остановка задачи
    bool _cancel_requested = false;
    // Задача запущена на выполнение
    bool _started = false;
    // Задача выполнена
    bool _finished = false;
    //! Блокировка _cancel_requested, _started, _finished
    mutable QMutex _status_locker;
    //! Мьютекс для ожидания завершения потока
    mutable QMutex _wait_mutex;

    mutable QMutex _waiting_feedback_mutex;
    //! Идентификаторы ожидаетых сообщений
    QSet<zf::MessageID> _waiting_feedback;
    //! Полученные ответы
    QMap<zf::MessageID, zf::Message> _collected_feedback;

    friend class WorkerObject;
    friend class Controller;
};
} // namespace zf::thread
