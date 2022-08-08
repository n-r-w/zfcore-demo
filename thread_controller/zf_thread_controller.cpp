#include "zf_thread_controller.h"
#include "zf_core.h"
#include <exception>
#include <QDebug>
#include <QMetaObject>
#include <QApplication>

namespace zf::thread
{
Controller::Controller(int terminate_timeout_ms, int max_thread_count, int thread_timeout, QObject* parent)
    : QObject(parent)
    , _pool(new QThreadPool(this))
    , _terminate_timeout_ms(terminate_timeout_ms)
    , _thread_timeout(thread_timeout)
{
    if (max_thread_count > 0)
        _pool->setMaxThreadCount(max_thread_count);
}

Controller::~Controller()
{    
    cancelAll(); 
}

void Controller::setMaxThreadCount(int count)
{
    _pool->setMaxThreadCount(count);
}

int Controller::maxThreadCount() const
{
    return _pool->maxThreadCount();
}

int Controller::threadTimeout() const
{
    QMutexLocker lock(&_thread_timeout_mutex);
    return _thread_timeout;
}

void Controller::setThreadTimeout(int thread_timeout_ms)
{
    QMutexLocker lock(&_thread_timeout_mutex);
    _thread_timeout = thread_timeout_ms;
}

int Controller::count() const
{
    return _workers.count();
}

int Controller::activeCount() const
{
    return _pool->activeThreadCount();
}

QString Controller::bestTask() const
{
    int count = INT_MAX;
    QString best_id;        
    for (auto it = _workers.constBegin(); it != _workers.constEnd(); ++it) {
        int c = it.value()->requestCount();
        if (count > c) {
            best_id = it.key();
            count = c;            
        }        
    }

    return best_id;
}

bool Controller::containts(const QString& id) const
{
    return _workers.contains(id);
}

Worker* Controller::worker(const QString& id) const
{
    return _workers.value(id);
}

QStringList Controller::id() const
{
    return _workers.keys();
}

QString Controller::startWorker(Worker* worker)
{
    Z_CHECK_NULL(worker);
    QString id = worker->id();
    Z_CHECK(!_workers.contains(id));

    worker->_controller = this;

    Z_THREAD_CONTROLLER_DEBUG_LOG(id, "Controller::startWorker 1");

    connect(worker->object(), &WorkerObject::sg_started, this, &Controller::sg_started, Qt::QueuedConnection);
    connect(worker->object(), &WorkerObject::sg_progress, this, &Controller::sg_progress, Qt::QueuedConnection);
    connect(worker->object(), &WorkerObject::sg_cancelled, this, &Controller::sg_cancelled, Qt::QueuedConnection);
    connect(worker->object(), &WorkerObject::sg_finished, this, &Controller::sl_worker_finished, Qt::QueuedConnection);
    connect(worker->object(), &WorkerObject::sg_data, this, &Controller::sg_data, Qt::QueuedConnection);
    connect(worker->object(), &WorkerObject::sg_error, this, &Controller::sg_error, Qt::QueuedConnection);

    emit sg_created(worker);

    try {
        _pool->start(worker);
    } catch (std::exception& e) {
        delete worker;
        Core::logError(e.what());
        Z_THREAD_CONTROLLER_DEBUG_LOG(id, "Controller::startWorker 2");
        return QString();

    } catch (...) {
#if defined(Q_CC_GNU)
        auto e = std::current_exception();
        if (e != nullptr)
            Core::logError(e.__cxa_exception_type()->name());
#endif
        delete worker;
        Z_THREAD_CONTROLLER_DEBUG_LOG(id, "Controller::startWorker 3");
        return QString();
    }

    _workers[id] = worker;

    Z_THREAD_CONTROLLER_DEBUG_LOG(id, "Controller::startWorker 4");

    return id;
}

void Controller::cancellWorker(const QString& id)
{
    Worker* worker = _workers.value(id);
    Z_CHECK_NULL(worker);
    if (!worker->isFinished()) {
        QMetaObject::invokeMethod(worker->object(), "cancel", Qt::QueuedConnection);
        _waiting_to_cancel << id;
    }
}

void Controller::cancelAll()
{
    if (_cancell_requested)
        return;

    _cancell_requested = true;

    _pool->clear();
    for (auto it = _workers.constBegin(); it != _workers.constEnd(); ++it) {
        cancellWorker(it.key());
    }

    QStringList not_cancelled = _workers.keys();
    QSet<QString> cancelled;
    bool need_more = true;
    while (need_more) {
        need_more = false;
        while (!not_cancelled.isEmpty() && !_workers.isEmpty()) {
            auto w = _workers.constBegin().value();
            QString id = w->id();

            not_cancelled.removeOne(id);
            if (cancelled.contains(id))
                continue;

            if (w->wait(_terminate_timeout_ms))
                cancelled << id;
            else {
                Utils::processEvents();
                need_more = true;
            }
        }
    }

    _pool->waitForDone();

    while (!_workers.isEmpty()) {
        auto w = _workers.constBegin().value();
        QString id = w->id();
        Z_CHECK(!w->isStarted() || w->isFinished());
        delete w;
        _workers.remove(id);
    }

    _waiting_to_cancel.clear();
    _cancell_requested = false;
}

bool Controller::isCancellRequested() const
{
    return _cancell_requested;
}

void Controller::request(const QString& id, QVariantPtr request)
{
    Worker* worker = _workers.value(id);
    Z_CHECK_NULL(worker);
    Z_CHECK(QMetaObject::invokeMethod(worker->object(), "request", Qt::QueuedConnection, Q_ARG(zf::QVariantPtr, request)));
}

void Controller::requestAll(QVariantPtr request)
{
    for (auto it = _workers.constBegin(); it != _workers.constEnd(); ++it) {
        Z_CHECK(QMetaObject::invokeMethod(it.value()->object(), "request", Qt::QueuedConnection, Q_ARG(zf::QVariantPtr, request)));
    }
}

void Controller::putExternalFeedback(const QString& id, const Uid& sender, const Message& message)
{
    Worker* worker = _workers.value(id);
    Z_CHECK_NULL(worker);
    Z_CHECK(QMetaObject::invokeMethod(worker->object(), "onExternalFeedback", Qt::QueuedConnection, Q_ARG(zf::Uid, sender),
                                      Q_ARG(zf::Message, message)));
}

void Controller::putExternalFeedbackAll(const Uid& sender, const Message& message)
{
    for (auto it = _workers.constBegin(); it != _workers.constEnd(); ++it) {
        Z_CHECK(QMetaObject::invokeMethod(it.value()->object(), "onExternalFeedback", Qt::QueuedConnection, Q_ARG(zf::Uid, sender),
                                          Q_ARG(zf::Message, message)));
    }
}

void Controller::sl_worker_finished(QString id)
{
    _waiting_to_cancel.remove(id);

    auto w = _workers.value(id);
    if (w == nullptr)
        return;

    _workers.remove(id);
    emit sg_finished(id);

    w->wait(_terminate_timeout_ms);

    delete w;
}

void Controller::requestMoveToThread(QObject* object, QThread* worker_thread, QString worker_id)
{
    Z_CHECK_NULL(object);

    if (_waiting_to_cancel.contains(worker_id)) {
        emit sg_objectMovedToThread(worker_id, false);
        return;
    }

    Z_CHECK_X(object->parent() == nullptr, object->parent()->metaObject()->className());
    Z_CHECK_X(object->thread() == QThread::currentThread(), object->metaObject()->className());
    object->moveToThread(worker_thread);
    Z_CHECK_X(object->thread() == worker_thread, object->metaObject()->className());

    emit sg_objectMovedToThread(worker_id, true);
}

} // namespace zf::thread
