#include "item_model_service.h"
#include "zf_core.h"
#include "zf_core_uids.h"
#include "item_model_service_translation.h"

// минимальное количество символов, с которого начинается поиск
#define REQUEST_MINIMUM_CHAR_COUNT 0

namespace ModelService
{
Service* _instance;

Service::Service()
    : zf::Plugin(zf::ModuleInfo(zf::CoreUids::UID_CODE_MODEL_SERVICE, zf::ModuleType::Unique, TR::MODULE_NAME, zf::Version(1, 0, 0)))
{
    _worker_thread = new QThread;
    _worker = new WorkerObject;
    _worker->moveToThread(_worker_thread);
    _worker_thread->start();
}

Service::~Service()
{
}

void Service::onShutdown()
{
    _worker_thread->exit();
    _worker_thread->wait();

    delete _worker;
    _worker = nullptr;

    delete _worker_thread;
    _worker_thread = nullptr;
}

Service* Service::instance()
{
    return zf::Core::getPlugin<Service>(zf::CoreUids::UID_CODE_MODEL_SERVICE);
}

WorkerObject::WorkerObject()
    : _object_extension(new zf::ObjectExtension(this))
{
    zf::Core::messageDispatcher()->registerObject(zf::CoreUids::MODEL_SERVICE, this, "sl_message_dispatcher_inbound");
}

WorkerObject::~WorkerObject()
{
    delete _object_extension;
    _models_cache.clear();
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

void WorkerObject::clearCache()
{
    _models_cache.clear();
}

void WorkerObject::registerRequest(const zf::MessageID& feedback_id, const zf::ModelServiceLookupRequestOptions& options,
    const zf::I_ObjectExtension* requester, const QString& filter_text, const QVariant& item_id)
{
#ifdef RNIKULENKOV
    if (item_id.isValid())
        zf::Core::logInfo("MODEL request: " + item_id.toString());
    else
        zf::Core::logInfo("MODEL request: " + options.lookup_entity_uid.toPrintable() + ", " + filter_text);
#endif

    InternalRequestInfoPtr r_info = Z_MAKE_SHARED(InternalRequestInfo);
    r_info->options = options;
    r_info->requester = requester;
    r_info->feedback_id = feedback_id;
    r_info->filter_text = filter_text;
    r_info->item_id = item_id;

    auto structure = zf::DataStructure::structure(options.lookup_entity_uid);

    r_info->dataset = structure->singleDataset();

    if (options.data_column_id.isValid())
        r_info->id_column = structure->property(options.data_column_id);
    else
        r_info->id_column = r_info->dataset.idColumnProperty();
    Z_CHECK(r_info->id_column.isValid());

    if (options.data_column_role > 0)
        r_info->id_column_role = options.data_column_role;

    if (options.display_column_id.isValid())
        r_info->name_column = structure->property(options.display_column_id);
    if (!r_info->name_column.isValid())
        r_info->name_column = r_info->dataset.fullNameColumn();
    if (!r_info->name_column.isValid())
        r_info->name_column = r_info->dataset.nameColumn();
    Z_CHECK(r_info->name_column.isValid());

    if (options.display_column_role > 0)
        r_info->name_column_role = options.display_column_role;

    r_info->model = _models_cache.value(options.lookup_entity_uid);
    if (r_info->model == nullptr) {
        zf::Error error;
        r_info->model = zf::Core::getModel<zf::Model>(options.lookup_entity_uid, zf::DataPropertySet {r_info->dataset}, error);
        Z_CHECK_ERROR(error);
        _models_cache[options.lookup_entity_uid] = r_info->model;
    }

    _requests[feedback_id] = r_info;

    if (r_info->model->data()->isInvalidated(r_info->dataset))
        r_info->model->load({r_info->dataset});

    if (r_info->model->isLoading())
        r_info->connection = connect(r_info->model.get(), &zf::Model::sg_finishLoad, this, &WorkerObject::sl_modelFinishLoad);
    else
        processRequest(r_info);
}

void WorkerObject::sl_modelFinishLoad(const zf::Error& error, const zf::LoadOptions& load_options, const zf::DataPropertySet& properties)
{
    Q_UNUSED(load_options);
    Q_UNUSED(properties);

    auto model = qobject_cast<zf::Model*>(sender());

    QSet<zf::MessageID> ids;
    for (auto i = _requests.constBegin(); i != _requests.constEnd(); ++i) {
        if (i.value()->model.get() == model)
            ids << i.key();
    }

    for (auto& id : qAsConst(ids)) {
        auto r_info = _requests.value(id);
        Z_CHECK_NULL(r_info);

        disconnect(r_info->connection);

        if (error.isError()) {
            _requests.remove(id);
            zf::Core::messageDispatcher()->postMessage(this, r_info->requester, zf::ErrorMessage(r_info->feedback_id, error));
            return;
        }

        processRequest(r_info);
    }
}

void WorkerObject::sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(subscribe_handle)
    Q_UNUSED(message)
    Q_UNUSED(sender)

    zf::Core::messageDispatcher()->confirmMessageDelivery(message, this);
}

void WorkerObject::processRequest(const InternalRequestInfoPtr& request)
{
    _requests.remove(request->feedback_id);

    auto ds = request->model->dataset(request->dataset);
    zf::Rows rows;
    bool all_rows = false;
    if (request->item_id.isValid()) {
        rows = request->model->findRows(request->id_column, request->item_id, false, request->options.data_column_role);

    } else {
        if (!request->filter_text.isEmpty())
            rows = ds->find(ds->index(0, request->name_column.pos()), request->name_column_role, request->filter_text, -1,
                Qt::MatchFlag::MatchContains | Qt::MatchFlag::MatchFixedString);
        else
            all_rows = true;
    }

    int column_pos = request->id_column.pos();
    int name_pos = request->name_column.pos();

    if (all_rows) {
        for (int i = 0; i < ds->rowCount(); i++) {
            QVariant id = ds->index(i, column_pos).data(request->id_column_role);
            QVariant converted;
            if (!zf::Utils::convertValue(id, request->id_column.dataType(), zf::Core::locale(zf::LocaleType::UserInterface), zf::ValueFormat::Internal,
                    zf::Core::locale(zf::LocaleType::Universal), converted)
                     .isOk())
                converted = id;

            QVariant name = ds->index(i, name_pos).data(request->name_column_role);
            request->result.append({zf::Uid::general(converted), name});
        }

    } else {
        for (auto& idx : qAsConst(rows)) {
            QVariant id = ds->index(idx.row(), column_pos, idx.parent()).data(request->id_column_role);
            QVariant converted;
            if (!zf::Utils::convertValue(id, request->id_column.dataType(), zf::Core::locale(zf::LocaleType::UserInterface), zf::ValueFormat::Internal,
                    zf::Core::locale(zf::LocaleType::Universal), converted)
                     .isOk())
                converted = id;

            QVariant name = ds->index(idx.row(), name_pos, idx.parent()).data(request->name_column_role);
            request->result.append({zf::Uid::general(converted), name});
        }
    }

    zf::Core::messageDispatcher()->postMessage(this, request->requester,
        zf::ExternalRequestMessage(
            request->feedback_id, zf::MessageCode(zf::CoreUids::UID_CODE_MODEL_SERVICE, _i_external_request_msg_code_id), request->result));
}

zf::MessageID Service::registerRequest(
    const zf::ModelServiceLookupRequestOptions& options, const zf::I_ObjectExtension* requester, const QString& filter_text, const QVariant& item_id)
{
    auto feedback_id = zf::MessageID::generate();
    Z_CHECK(QMetaObject::invokeMethod(_worker, "registerRequest", Qt::QueuedConnection, Q_ARG(zf::MessageID, feedback_id),
        Q_ARG(zf::ModelServiceLookupRequestOptions, options), Q_ARG(const zf::I_ObjectExtension*, requester), Q_ARG(QString, filter_text),
        Q_ARG(QVariant, item_id)));
    return feedback_id;
}

zf::ModelServiceLookupRequestOptions Service::getOptions(const QVariant& v)
{
    zf::ModelServiceLookupRequestOptions o = v.value<zf::ModelServiceLookupRequestOptions>();
    Z_CHECK(o.lookup_entity_uid.isValid());
    return o;
}

zf::MessageID Service::requestLookup(const zf::I_ObjectExtension* sender, const zf::Uid& key, const QString& request_type, const QVariant& options)
{
    Q_UNUSED(options);
    Q_UNUSED(request_type);
    Z_CHECK(isCorrectKey(key, options));

    return registerRequest(getOptions(options), sender, "", key.asVariant(0));
}

zf::MessageID Service::requestLookup(
    const zf::I_ObjectExtension* sender, const QString& text, const zf::UidList& parent_keys, const QString& request_type, const QVariant& options)
{
    Q_UNUSED(parent_keys);
    Q_UNUSED(options);

    Z_CHECK(checkText(text, request_type, options));

    return registerRequest(getOptions(options), sender, text, QVariant());
}

bool Service::checkText(const QString& text, const QString& request_type, const QVariant& options)
{
    Q_UNUSED(request_type);
    Q_UNUSED(options);
    return text.length() >= REQUEST_MINIMUM_CHAR_COUNT;
}

bool Service::canEdit(const zf::UidList& parent_keys, const QString& request_type, const QVariant& options)
{
    Q_UNUSED(parent_keys);
    Q_UNUSED(request_type);
    Q_UNUSED(options);

    return true;
}

bool Service::isCorrectKey(const zf::Uid& key, const QVariant& options)
{
    Q_UNUSED(options);
    if (!key.isValid() || key.type() != zf::UidType::General || key.count() != 1 || !key.asVariant(0).isValid())
        return false;
    return true;
}

QString Service::extractText(const QVariant& data, const QVariant& options)
{
    Q_UNUSED(options);
    return data.toString();
}

int Service::requestDelay(const QVariant& options)
{
    Q_UNUSED(options);
    return zf::Consts::USER_INPUT_TIMEOUT_MS;
}

void Service::invalidate()
{
    Z_CHECK(QMetaObject::invokeMethod(_worker, "clearCache", Qt::QueuedConnection));
}

} // namespace ModelService
