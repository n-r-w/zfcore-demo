#include "zf_model_keeper.h"
#include "zf_core.h"
#include "zf_core_uids.h"
#include "zf_framework.h"

namespace zf
{
ModelKeeper::ModelKeeper()
    : _object_extension(new ObjectExtension(this))
{
    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::MODEL_KEEPER, this, QStringLiteral("sl_inbound_message")));
    Framework::internalCallbackManager()->registerObject(this, QStringLiteral("sl_callbackManager"));
}

ModelKeeper::~ModelKeeper()
{
    QMutexLocker lock(&_mutex);
    _models.clear();
    delete _object_extension;
}

bool ModelKeeper::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void ModelKeeper::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant ModelKeeper::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool ModelKeeper::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void ModelKeeper::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void ModelKeeper::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void ModelKeeper::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void ModelKeeper::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void ModelKeeper::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

bool ModelKeeper::isModelKeeped(const Uid& uid) const
{
    Z_CHECK(uid.isValid() && uid.isPersistent());
    QMutexLocker lock(&_mutex);
    return _models.contains(uid);
}

void ModelKeeper::keepModels(const QList<ModelPtr>& models)
{
    QMutexLocker lock(&_mutex);

    for (auto& m : models) {
        Z_CHECK(m->isPersistent());
        _models[m->entityUid()] = m;
    }
}

void ModelKeeper::keepModels(const UidList& entity_uid_list, const QList<DataPropertySet>& properties_to_load)
{
    QMutexLocker lock(&_mutex);

    if (objectExtensionDestroyed())
        return;

    if (!properties_to_load.isEmpty())
        Z_CHECK(entity_uid_list.count() == properties_to_load.count());

    bool need_request = false;
    for (int i = 0; i < entity_uid_list.count(); i++) {
        auto u = entity_uid_list.at(i);

        if (_models.contains(u))
            continue;

        _requests.enqueue({u, properties_to_load.isEmpty() ? DataPropertySet() : properties_to_load.at(i)});
        need_request = true;
    }

    if (need_request)
        Framework::internalCallbackManager()->addRequest(this, Framework::MODEL_KEEPER_REQUEST_CALLBACK_KEY);
}

void ModelKeeper::keepModels(const Uid& entity_uid, const DataPropertySet& properties_to_load)
{
    keepModels(UidList {entity_uid}, QList<DataPropertySet> {properties_to_load});
}

void ModelKeeper::freeModels(const UidList& entity_uid_list)
{
    QMutexLocker lock(&_mutex);

    for (auto& uid : entity_uid_list) {
        _models.remove(uid);

        for (int i = _requests.count() - 1; i >= 0; i--) {
            if (_requests.at(i).entity_uid != uid)
                continue;
            _requests.removeAt(i);
        }
    }
}

void ModelKeeper::sl_finishRemove(const Error& error)
{
    QMutexLocker lock(&_mutex);

    Model* model = qobject_cast<Model*>(sender());
    Z_CHECK_NULL(model);

    if (error.isOk())
        _models.remove(model->entityUid());
}

void ModelKeeper::sl_inbound_message(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)

    QMutexLocker lock(&_mutex);

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (feedback_ids.contains(message.feedbackMessageId())) {
        feedback_ids.remove(message.feedbackMessageId());

        if (message.messageType() == MessageType::MMEventModelList) {
            MMEventModelListMessage msg(message);
            for (auto& m : msg.models()) {
                _models[m->entityUid()] = m;

                connect(m.get(), &Model::sg_finishRemove, this, &ModelKeeper::sl_finishRemove);
            }
        }
    }
}

void ModelKeeper::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (objectExtensionDestroyed())
        return;

    if (key == Framework::MODEL_KEEPER_REQUEST_CALLBACK_KEY) {
        MessagePauseHelper mpause;
        mpause.pause();

        if (!_requests.isEmpty())
            Utils::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
        Utils::blockProcessEvents();

        int count = 0;
        while (count < Framework::MODEL_KEEPER_ONE_STEP && !_requests.isEmpty()) {
            count++;
            RequestInfo request = _requests.dequeue();
            MessageID feedback_message_id;
            QList<ModelPtr> models;
            Error error = Core::getModels(this, {request.entity_uid}, QList<LoadOptions>(), {request.properties}, {false}, models,
                                           feedback_message_id);
            if (error.isOk())
                feedback_ids << feedback_message_id;
        }

        if (!_requests.isEmpty() && !objectExtensionDestroyed())
            Framework::internalCallbackManager()->addRequest(this, Framework::MODEL_KEEPER_REQUEST_CALLBACK_KEY);

        Utils::unBlockProcessEvents();
    }
}

} // namespace zf
