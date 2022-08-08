#include "zf_thread_worker.h"
#include "zf_thread_controller.h"
#include <QDebug>
#include <QTime>
#include <QThread>
#include <QWaitCondition>
#include <QUuid>
#include <QApplication>
#include "zf_core.h"

namespace zf::thread
{
WorkerObject::WorkerObject()
    : QObject()
    , _object_extension(new ObjectExtension(this))
{
}

WorkerObject::~WorkerObject()
{
    delete _object_extension;
}

Worker* WorkerObject::worker() const
{
    return _worker;
}

void WorkerObject::deleteLater()
{
    _object_extension->objectExtensionDestroy();
}

void WorkerObject::cancel()
{
    if (_object_extension->objectExtensionDestroyed())
        return;
    _worker->cancel();
}

void WorkerObject::request(QVariantPtr request)
{
    if (_object_extension->objectExtensionDestroyed())
        return;

    if (!_data_connection)
        _data_connection = connect(this, &WorkerObject::sg_data, this, &WorkerObject::onData);

    _worker->requestHelper(request);
}

void WorkerObject::onData(QString id, qint64 status, QVariantPtr data)
{
    Q_UNUSED(id);
    Q_UNUSED(status);
    Q_UNUSED(data);
}

void WorkerObject::onExternalFeedback(const Uid& sender, const Message& message)
{
    if (_object_extension->objectExtensionDestroyed())
        return;
    _worker->onExternalFeedbackHelper(sender, message);
}

bool WorkerObject::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void WorkerObject::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant WorkerObject::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool WorkerObject::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void WorkerObject::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void WorkerObject::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void WorkerObject::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void WorkerObject::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void WorkerObject::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

Worker::Worker(bool use_event_loop, WorkerObject* object, const QString& id)
    : QRunnable()
    , _id(id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id)
    , _object(object == nullptr ? new WorkerObject() : object)
    , _use_loop(use_event_loop)
{
    _object->setParent(nullptr);
    _object->_worker = this;
    setAutoDelete(false);
    Z_THREAD_CONTROLLER_DEBUG_LOG(_id, "Worker::Worker");
}

Worker::~Worker()
{
    if (_object != nullptr)
        _object->deleteLater();
    Z_THREAD_CONTROLLER_DEBUG_LOG(_id, "Worker::~Worker");
}

void Worker::run()
{
    Z_THREAD_CONTROLLER_DEBUG_LOG(id(), "Worker::run 1");

    QMutexLocker lock(&_wait_mutex);

    _status_locker.lock();
    Z_CHECK(!_started);
    _started = true;
    _status_locker.unlock();

    Z_CHECK_NULL(_controller);
    Z_CHECK_NULL(_object);
    Z_CHECK(_object->thread() != QThread::currentThread());

    // мы не можем просто взять и переместить обьект в поток здесь, т.к. перевод объекта в поток должен осуществляться в
    // том потоке, в котором его создавали (а создавали его в одном потоке с контроллером). Вместо этого мы шлем
    // сообщение контроллеру с запросом на перевод в поток и ждем ответа
    bool object_moved = false;
    QEventLoop* move_object_to_thread_loop = new QEventLoop;

    // выход из ожидания по таймеру чтобы не подвисло
    QTimer* move_object_to_thread_loop_timer = new QTimer;
    move_object_to_thread_loop_timer->setSingleShot(true);
    if (_controller->threadTimeout() > 0)
        move_object_to_thread_loop_timer->start(_controller->threadTimeout());
    auto connection_timeout
        = QObject::connect(move_object_to_thread_loop_timer, &QTimer::timeout, move_object_to_thread_loop,
                           [move_object_to_thread_loop]() { QMetaObject::invokeMethod(move_object_to_thread_loop, "quit"); });

    auto connection_move = QObject::connect(_controller, &Controller::sg_objectMovedToThread, _object,
                                            [move_object_to_thread_loop, move_object_to_thread_loop_timer,
                                             &object_moved, id = id()](QString worker_id, bool is_success) {
                                                if (worker_id != id)
                                                    return;
                                                object_moved = is_success;

                                                QMetaObject::invokeMethod(move_object_to_thread_loop, "quit");
                                                QMetaObject::invokeMethod(move_object_to_thread_loop_timer, "stop");
                                            });
    QMetaObject::invokeMethod(_controller, "requestMoveToThread", Qt::QueuedConnection, Q_ARG(QObject*, _object),
                              Q_ARG(QThread*, QThread::currentThread()), Q_ARG(QString, id()));

    move_object_to_thread_loop->exec();
    QObject::disconnect(connection_timeout);
    QObject::disconnect(connection_move);

    delete move_object_to_thread_loop_timer;
    move_object_to_thread_loop_timer = nullptr;

    delete move_object_to_thread_loop;
    move_object_to_thread_loop = nullptr;

    _status_locker.lock();
    if (!object_moved || _cancel_requested ||
        // не удалось переместить в этот поток
        _object->thread() != QThread::currentThread()) {
        _status_locker.unlock();

        emit _object->sg_started(id());

        _status_locker.lock();
        _finished = true;
        _status_locker.unlock();

        emit _object->emit sg_finished(id());

        Z_THREAD_CONTROLLER_DEBUG_LOG(id(), "Worker::run 2");
        return;
    }

    _status_locker.unlock();

    if (_use_loop) {
        _main_loop = new QEventLoop;

        emit _object->sg_started(id());
        if (!_data_request.isEmpty())
            onRequest();

        _main_loop->exec();
        delete _main_loop;
        _main_loop = nullptr;

        _status_locker.lock();
        _finished = true;
        _status_locker.unlock();

        emit _object->sg_finished(id());

        _main_loop = nullptr;

    } else {
        emit _object->sg_started(id());

        if (!_data_request.isEmpty())
            onRequest();
        process();

        _status_locker.lock();
        _finished = true;
        _status_locker.unlock();

        emit _object->sg_finished(id());
    }
    Z_THREAD_CONTROLLER_DEBUG_LOG(id(), "Worker::run 3");    
}

QString Worker::id() const
{
    QMutexLocker lock(&_id_locker);
    return _id;
}

bool Worker::isStarted() const
{
    return _started;
}

bool Worker::isFinished() const
{
    QMutexLocker lock(&_status_locker);
    return _finished;
}

bool Worker::isCancelled() const
{
    QMutexLocker lock(&_status_locker);
    return _cancel_requested;
}

WorkerObject* Worker::object() const
{
    QMutexLocker lock(&_status_locker);
    Z_CHECK_NULL(_object);
    return _object;
}

void Worker::requestHelper(QVariantPtr request)
{
    _data_request_locker.lock();
    _data_request.enqueue(request);
    _request_count++;
    _data_request_locker.unlock();

    _status_locker.lock();
    bool is_finished = _finished;
    _status_locker.unlock();

    if (is_finished) {
        Z_CHECK(!Utils::isMainThread());
        onRequest();
    }
}

void Worker::onExternalFeedbackHelper(const Uid& sender, const Message& message)
{
    Q_UNUSED(sender)

    QMutexLocker lock(&_waiting_feedback_mutex);

    if (_waiting_feedback.remove(message.feedbackMessageId())) {
        _collected_feedback[message.feedbackMessageId()] = message;        
    }
}

QVariantPtr Worker::getRequest()
{
    QMutexLocker lock(&_data_request_locker);
    bool has_request = !_data_request.isEmpty();
    QVariantPtr request;
    if (has_request)
        request = _data_request.dequeue();

    return request;
}

void Worker::cancel()
{
    QMutexLocker lock(&_status_locker);

    if (_cancel_requested || !_started || _finished)
        return;

    _cancel_requested = true;
    onCancell();

    if (_main_loop != nullptr)
        _main_loop->quit();
}

void Worker::sendDone() const
{
    sendData();
}

void Worker::sendData(qint64 status, QVariantPtr data) const
{
    emit _object->sg_data(id(), status, data);

    QMutexLocker lock(&_data_request_locker);
    Z_CHECK(_request_count > 0);
    _request_count--;
}

void Worker::sendError(qint64 status, QString error_text) const
{
    emit _object->sg_error(id(), status, error_text);
    QMutexLocker lock(&_data_request_locker);
    Z_CHECK(_request_count > 0);
    _request_count--;
}

void Worker::setProgress(int percent)
{
    Z_CHECK(percent >= 0 && percent <= 100);
    emit _object->sg_progress(id(), percent);
}

void Worker::cancelled()
{
    emit _object->sg_cancelled(id());
}

void Worker::onCancell()
{
}

void Worker::onRequest()
{
}

int Worker::requestCount() const
{
    QMutexLocker lock(&_data_request_locker);
    return _request_count;
}

bool Worker::wait(int wait_ms) const
{
    if (wait_ms <= 0) {
        _wait_mutex.lock();
        _wait_mutex.unlock();
        return true;
    } else {
        bool res = _wait_mutex.tryLock(wait_ms);
        if (res)
            _wait_mutex.unlock();
        return res;
    }
}

bool Worker::isCancelRequested() const
{
    QMutexLocker lock(&_status_locker);
    return _cancel_requested;
}

void Worker::registerExternalFeedback(const zf::MessageID& feedback_id)
{
    QMutexLocker lock(&_waiting_feedback_mutex);

    Z_CHECK(feedback_id.isValid());
    Z_CHECK(!_waiting_feedback.contains(feedback_id));
    _waiting_feedback << feedback_id;
}

Message Worker::waitForExternalFeedback(const MessageID& feedback_id, int total_msec, int pause_ms)
{
    registerExternalFeedback(feedback_id);

    Message message;
    QElapsedTimer* total_timer = nullptr;
    if (total_msec > 0) {
        total_timer = new QElapsedTimer;
        total_timer->start();
    }

    QEventLoop* loop = new QEventLoop;
    QTimer timer;
    loop->connect(&timer, &QTimer::timeout, loop, [loop]() { loop->quit(); });
    while (true) {
        timer.start(pause_ms);
        loop->exec(QEventLoop::ExcludeUserInputEvents);
        message = getWaitingExternalFeedback(feedback_id);
        if (message.isValid())
            break;

        if (total_timer != nullptr && total_timer->elapsed() > total_msec)
            break;
    }
    delete loop;

    if (total_timer != nullptr)
        delete total_timer;

    return message;
}

Message Worker::getWaitingExternalFeedback(const zf::MessageID& feedback_id) const
{
    QMutexLocker lock(&_waiting_feedback_mutex);

    return _collected_feedback.value(feedback_id);
}

} // namespace zf::thread
