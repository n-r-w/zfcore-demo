#include "zf_progress_object.h"
#include "zf_core.h"
#include "zf_exception.h"
#include "zf_translation.h"

#define DEBUG_NAME_PROPERTY "_ZF_DEBUG_NAME_"

namespace zf
{
ProgressHelper::ProgressHelper(const ProgressObject* object)
{
    if (object != nullptr)
        reset(object);
}

ProgressHelper::~ProgressHelper()
{
    if (_object && _object->isProgress()) {
        Error error;
        try {
            _object->finishProgress();
        } catch (const std::bad_alloc&) {
            error = Error(ZF_TR(ZFT_NO_MEMORY));
        } catch (const FatalException& e) {
            error = e.error();
        } catch (const std::exception& e) {
            error = Error(utf(e.what()));
        } catch (...) {
            error = Error(ZF_TR(ZFT_UNKNOWN_ERROR));
        }
        if (error.isError())
            Core::logError(error);
    }
}

void ProgressHelper::reset(const ProgressObject* object)
{
    _object = object;
}

ProgressObject::ProgressObject(QObject* parent)
    : QObject(parent)
    , _object_extension(new ObjectExtension(this))
{
}

ProgressObject::~ProgressObject()
{
    while (isProgress()) {
        finishProgress();
    }
    delete _object_extension;
}

QString ProgressObject::debugName() const
{
    QString dname = property(DEBUG_NAME_PROPERTY).toString();

    if (dname.isEmpty()) {
        dname = getDebugName();
        const_cast<ProgressObject*>(this)->setProperty(DEBUG_NAME_PROPERTY, dname);
    }

    return dname;
}

QString ProgressObject::getDebugName() const
{
    QString object_name = objectName();

    if (object_name.isEmpty() || comp(object_name, metaObject()->className()))
        return metaObject()->className();

    return QStringLiteral("%1, %2").arg(metaObject()->className()).arg(object_name);
}

bool ProgressObject::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void ProgressObject::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant ProgressObject::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool ProgressObject::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void ProgressObject::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void ProgressObject::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void ProgressObject::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void ProgressObject::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void ProgressObject::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void ProgressObject::requestDelete() const
{
    _is_delete_requested = true;
    if (!isProgress())
        const_cast<ProgressObject*>(this)->objectExtensionDestroy();
}

bool ProgressObject::isDeleteRequested() const
{
    return _is_delete_requested;
}

bool ProgressObject::isProgress() const
{
    return !_progress_stack.isEmpty();
}

int ProgressObject::progressPercent() const
{
    Z_CHECK_X(isProgress(), "Попытка вызвать progressPercent без startProgress");
    return _progress_stack.last()->percent;
}

QString ProgressObject::progressText() const
{
    Z_CHECK_X(isProgress(), "Попытка вызвать progressText без startProgress");
    return _progress_stack.last()->text;
}

int ProgressObject::progressDepth() const
{
    return _progress_stack.count();
}

void ProgressObject::cancelProgress() const
{
    if (Utils::isAppHalted())
        return;

    Z_CHECK_X(_is_cancel_support,
            utf("Вызов ZProgressObject::cancelProgress без его запуска через "
                "ZProgressObject::startProgressWithCancel"));

    if (!isProgress())
        return;

    _is_progress_to_cancel = true;

    emit const_cast<ProgressObject*>(this)->sg_cancelProgress();
}

bool ProgressObject::isCancelProgressSupported() const
{
    if (!isProgress())
        return _is_cancel_support;

    return _progress_stack.last()->cancel_support;
}

void ProgressObject::setLongOperation() const
{
    if (Utils::isAppHalted())
        return;

    if (_is_long_operation)
        return;

    _is_long_operation = true;
    if (isProgress()) {
        const_cast<ProgressObject*>(this)->onProgressChange(progressPercent(), progressText());

        emit const_cast<ProgressObject*>(this)->sg_progressChange(progressPercent(), progressText());
    }
}

bool ProgressObject::isLongOperation() const
{
    return _is_long_operation;
}

void ProgressObject::startProgress(const QString& text, int start_persent) const
{
    if (Utils::isAppHalted())
        return;

    const_cast<ProgressObject*>(this)->startProgressOperation(text, start_persent, false);
}

void ProgressObject::startProgressWithCancel(const QString& text, int start_persent) const
{
    if (Utils::isAppHalted())
        return;

    const_cast<ProgressObject*>(this)->startProgressOperation(text, start_persent, true);
}

void ProgressObject::startProgressOperation(const QString& text, int start_persent, bool cancel_support)
{
    if (Utils::isAppHalted())
        return;

    auto p = Z_MAKE_SHARED(Progress);
    p->owner = this;
    p->key = Utils::generateUniqueStringDefault();

    p->text = text.simplified();
    if (p->text.isEmpty())
        p->text = defaultProgressText();

    p->percent = start_persent;
    p->cancel_support = cancel_support;
    p->start_time = QDateTime::currentDateTime();

    bool cancel_activated = (_progress_stack.isEmpty() && cancel_support)
        || (!_progress_stack.isEmpty() && _progress_stack.last()->cancel_support != cancel_support && cancel_support);
    _is_cancel_support = _is_cancel_support || cancel_support;
    _progress_stack << p;

    if (_progress_stack.count() == 1)
        const_cast<ProgressObject*>(this)->onProgressBegin(p->text, p->percent);

    const_cast<ProgressObject*>(this)->onProgressChange(p->percent, p->text);

    emit sg_startProgress(p->text, p->percent, cancel_support);

    if (_progress_stack.count() == 1) {
#ifdef RNIKULENKOV
//        qDebug() << "start progress:" << objectName();
#endif
        emit sg_progressBegin(p->text, p->percent, cancel_support);
    }

    emit sg_progressChange(p->percent, p->text);
    if (cancel_activated)
        emit sg_cancelSupportActivated();
}

void ProgressObject::setProgressPercent(int percent, const QString& text) const
{
    if (Utils::isAppHalted())
        return;

    Z_CHECK_X(percent >= 0 && percent <= 100,
            utf("Процент выполнения должен быть в диапазоне 0-100. "
                "Задано: %1. Объект: %2 (%3)")
                    .arg(percent)
                    .arg(objectName(), metaObject()->className()));

    if (!isProgress()) {
        if (percent < 100)
            startProgress(text, percent);
        return;
    }

    Progress* p = _progress_stack.last().get();
    QString s = text.trimmed();
    if (s.isEmpty())
        s = !p->text.isEmpty() ? p->text : defaultProgressText();
    p->percent = percent;
    p->text = s;

    const_cast<ProgressObject*>(this)->onProgressChange(p->percent, p->text);

    emit const_cast<ProgressObject*>(this)->sg_progressChange(p->percent, p->text);
}

void ProgressObject::updateProgressInfo() const
{
    if (Utils::isAppHalted())
        return;

    emit const_cast<ProgressObject*>(this)->sg_updateProgressInfo();
}

void ProgressObject::finishProgress() const
{
    if (Utils::isAppHalted())
        return;

    if (!isProgress())
        return;

    if (progressPercent() != 100)
        setProgressPercent(100);

    QString key = _progress_stack.last()->key;
    _progress_stack.pop();

    if (_progress_stack.isEmpty()) {
        _is_progress_to_cancel = false;
        _is_cancel_support = false;
        _is_long_operation = false;

        const_cast<ProgressObject*>(this)->onProgressEnd();

#ifdef RNIKULENKOV
//        qDebug() << "finish progress:" << objectName();
#endif
        emit const_cast<ProgressObject*>(this)->sg_finishProgress(key);
        emit const_cast<ProgressObject*>(this)->sg_progressEnd();

        if (_is_delete_requested)
            const_cast<ProgressObject*>(this)->objectExtensionDestroy();

    } else {
        const_cast<ProgressObject*>(this)->onProgressChange(
                _progress_stack.last()->percent, _progress_stack.last()->text);

        emit const_cast<ProgressObject*>(this)->sg_progressChange(_progress_stack.last()->percent, _progress_stack.last()->text);
        emit const_cast<ProgressObject*>(this)->sg_finishProgress(key);
    }
}

QVariant ProgressObject::progressData() const
{
    return _progress_data;
}

void ProgressObject::setProgressData(const QVariant& v)
{
    if (Utils::isAppHalted())
        return;

    if (_progress_data == v)
        return;

    _progress_data = v;

    onProgressDataChange(_progress_data);
    emit sg_progressDataChange(_progress_data);
}

void ProgressObject::processUserEvents() const
{
    emit const_cast<ProgressObject*>(this)->sg_processUserEvents();
}

void ProgressObject::setDefaultProgressText(const QString& text)
{
    _default_progress_text = text;
}

QString ProgressObject::defaultProgressText() const
{
    return _default_progress_text.isEmpty() ? ZF_TR(ZFT_DATA_PROCESSING) : _default_progress_text;
}

bool ProgressObject::isCanceled() const
{
    return _is_progress_to_cancel;
}

void ProgressObject::followProgress(ProgressObject* obj, bool full_follow, Qt::ConnectionType c_type)
{
    Z_CHECK_NULL(obj);

    // встроен ли такой ранее
    auto info = _follow_info.value(obj);
    if (info != nullptr) {
        info->count++;
        return;
    }

    bool need_sg_startProgress = true;
    bool need_sg_finishProgress = true;
    bool need_sg_progressChange = full_follow;
    bool need_sg_progressDataChange = full_follow;
    bool need_sg_cancelProgress = full_follow;
    bool need_sg_processUserEvents = full_follow;
    bool need_sg_updateProgressInfo = full_follow;

    for (int i = 0; i < obj->progressDepth(); i++) {
        startProgress();
    }

    info = Z_MAKE_SHARED(FollowInfo);
    _follow_info[obj] = info;
    info->c_type = c_type;

    if (need_sg_startProgress) //-V547
        info->sg_startProgress
            = connect(obj, &ProgressObject::sg_startProgress, this, &ProgressObject::sl_followObjectStartProgress, c_type);
    if (need_sg_finishProgress) //-V547
        info->sg_finishProgress
            = connect(obj, &ProgressObject::sg_finishProgress, this, &ProgressObject::sl_followObjectFinishProgress, c_type);
    if (need_sg_progressChange)
        info->sg_progressChange
            = connect(obj, &ProgressObject::sg_progressChange, this, &ProgressObject::sl_followObjectProgressChange, c_type);
    if (need_sg_progressDataChange)
        info->sg_progressDataChange
            = connect(obj, &ProgressObject::sg_progressDataChange, this, &ProgressObject::sl_followObjectProgressDataChange, c_type);
    if (need_sg_cancelProgress)
        info->sg_cancelProgress
            = connect(obj, &ProgressObject::sg_cancelProgress, this, &ProgressObject::sl_followObjectCancelProgress, c_type);
    if (need_sg_processUserEvents)
        info->sg_processUserEvents
            = connect(obj, &ProgressObject::sg_processUserEvents, this, &ProgressObject::sl_followObjectProcessUserEvents, c_type);
    if (need_sg_updateProgressInfo)
        info->sg_updateProgressInfo
            = connect(obj, &ProgressObject::sg_updateProgressInfo, this, &ProgressObject::sl_followObjectUpdateProgressInfo, c_type);

    info->sg_destroyed = connect(obj, &QObject::destroyed, this, &ProgressObject::sl_followObjectDestroyed);
}

void ProgressObject::followProgress(const std::shared_ptr<ProgressObject>& obj, bool full_follow, Qt::ConnectionType c_type)
{
    followProgress(obj.get(), full_follow, c_type);
}

void ProgressObject::unFollowProgress(ProgressObject* obj)
{
    auto info = _follow_info.value(obj);
    Z_CHECK_X(info != nullptr && info->count > 0, "ProgressObject::unFollowProgress counter error");

    info->count--;
    if (info->count > 0)
        return;

    for (int i = 0; i < obj->progressDepth(); i++) {
        Z_CHECK(progressDepth() > 0);
        finishProgress();
    }

    if (info->sg_startProgress)
        disconnect(info->sg_startProgress);
    if (info->sg_finishProgress)
        disconnect(info->sg_finishProgress);
    if (info->sg_progressChange)
        disconnect(info->sg_progressChange);
    if (info->sg_progressDataChange)
        disconnect(info->sg_progressDataChange);
    if (info->sg_cancelProgress)
        disconnect(info->sg_cancelProgress);
    if (info->sg_processUserEvents)
        disconnect(info->sg_processUserEvents);
    if (info->sg_updateProgressInfo)
        disconnect(info->sg_updateProgressInfo);

    disconnect(info->sg_destroyed);

    _follow_info.remove(obj);
}

void ProgressObject::unFollowProgress(const std::shared_ptr<ProgressObject>& obj)
{
    unFollowProgress(obj.get());
}

bool ProgressObject::isFollowProgress(ProgressObject* object) const
{
    Z_CHECK_NULL(object);
    return _follow_info.contains(object);
}

bool ProgressObject::isFollowProgress(const std::shared_ptr<ProgressObject>& obj) const
{
    return isFollowProgress(obj.get());
}

QList<ProgressObject*> ProgressObject::allFollowProgressObjects() const
{
    QList<ProgressObject*> res;
    for (auto i = _follow_info.constBegin(); i != _follow_info.constEnd(); ++i) {
        res << qobject_cast<ProgressObject*>(i.key());
    }
    return res;
}

const ProgressObject::Progress* ProgressObject::progressByKey(const QString& key) const
{
    for (const auto& p : qAsConst(_progress_stack)) {
        if (p->key == key)
            return p.get();
    }

    return nullptr;
}

QString ProgressObject::progressKey() const
{
    Z_CHECK(!_progress_stack.isEmpty());
    return _progress_stack.last()->key;
}

const QStack<std::shared_ptr<ProgressObject::Progress>>& ProgressObject::progressStack() const
{
    return _progress_stack;
}

void ProgressObject::onProgressBegin(const QString& text, int start_persent)
{
    Q_UNUSED(text)
    Q_UNUSED(start_persent)
}

void ProgressObject::onProgressChange(int percent, const QString& text)
{
    Q_UNUSED(text)
    Q_UNUSED(percent)
}

void ProgressObject::onProgressEnd()
{    
}

void ProgressObject::onProgressDataChange(const QVariant& v)
{
    Q_UNUSED(v)
}

void ProgressObject::sl_followObjectDestroyed(QObject* obj)
{
    Z_CHECK(_follow_info.remove(obj));
}

void ProgressObject::sl_followObjectStartProgress(const QString& text, int start_persent, bool cancel_support)
{
    startProgressOperation(text, start_persent, cancel_support);
}

void ProgressObject::sl_followObjectFinishProgress(const QString& key)
{
    Q_UNUSED(key)
    finishProgress();
}

void ProgressObject::sl_followObjectProgressChange(int percent, const QString& text)
{
    setProgressPercent(percent, text);
}

void ProgressObject::sl_followObjectProgressDataChange(const QVariant& v)
{
    setProgressData(v);
}

void ProgressObject::sl_followObjectCancelProgress()
{
    cancelProgress();
}

void ProgressObject::sl_followObjectProcessUserEvents()
{
    processUserEvents();
}

void ProgressObject::sl_followObjectUpdateProgressInfo()
{
    updateProgressInfo();
}

ProgressObject::Progress::Progress()
    : owner(nullptr)
    , percent(0)
{
}

bool ProgressObject::Progress::isLongOperation() const
{
    return owner->_is_long_operation;
}

bool ProgressObject::FollowInfo::isEqual(ProgressObject::FollowInfo* i) const
{
    return (c_type == i->c_type && sg_startProgress == i->sg_startProgress && sg_finishProgress == i->sg_finishProgress
            && sg_progressChange == i->sg_progressChange && sg_progressDataChange == i->sg_progressDataChange
            && sg_cancelProgress == i->sg_cancelProgress && sg_processUserEvents == i->sg_processUserEvents
            && sg_updateProgressInfo == i->sg_updateProgressInfo);
}

} // namespace zf
