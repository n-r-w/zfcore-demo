#include <mydefs.h>
#include "zf_core.h"
#include "zf_data_widgets.h"
#include "zf_database_messages.h"
#include "zf_docx_report.h"
#include "zf_framework.h"
#include "zf_html_report.h"
#include "zf_html_tools.h"
#include "zf_log_dialog.h"
#include "zf_message_box.h"
#include "zf_model_keeper.h"
#include "zf_model_manager.h"
#include "zf_module_manager.h"
#include "zf_note_dialog.h"
#include "zf_translation.h"
#include "zf_widget_replacer_ds.h"
#include "zf_widget_replacer_mn.h"
#include "zf_logging.h"
#include "zf_cumulative_error.h"
#include "zf_sync_ask.h"
#include "../../../client_version.h"

#include <libxml/xmlmemory.h>
#include <libxml/xmlerror.h>

#ifdef Q_OS_WINDOWS
#include "zf_mswidget.h"
#endif

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QStandardPaths>
#include <QNetworkProxy>
#include <QtConcurrent>

#include <zf_script_view.h>

namespace zf
{
ObjectExtensionPtr<Framework> Core::_framework;
ObjectExtensionPtr<ModelManager> Core::_model_manager;
std::unique_ptr<ModuleManager> Core::_module_manager;
ObjectExtensionPtr<DatabaseManager> Core::_database_manager;
ObjectExtensionPtr<MessageDispatcher> Core::_message_dispatcher;
ObjectExtensionPtr<ModelKeeper> Core::_model_keeper;
ObjectExtensionPtr<ExternalRequester> Core::_external_requester;
ObjectExtensionPtr<ProgressObject> Core::_progress;
std::unique_ptr<CumulativeError> Core::_cumulative_error;
QString Core::_lastUsedDirectory;
DatabaseID Core::_default_database_id;
QString Core::_build_version;
Version Core::_application_version;
std::unique_ptr<QHash<DatabaseID, DatabaseInformation>> Core::_database_info;
I_Catalogs* Core::_catalogs = nullptr;
I_DataRestriction* Core::_restriction = nullptr;
QAtomicInt Core::_is_bootstraped = 0;
CoreModes Core::_mode = CoreMode::Undefinded;
QString Core::_core_instance_key;
QString Core::_default_debprint_location;
QString Core::_current_user_login;
QString Core::_changelog;

bool Core::_use_proxy = false;
Credentials Core::_proxy_credentials;
QUrl Core::_default_url;
QMutex Core::_proxy_mutex;

bool Core::isBootstraped()
{    
    return _is_bootstraped;
}

bool Core::isActive(bool ignore_main_window)
{
    if (mode().testFlag(CoreMode::Library))
        ignore_main_window = true;

    return isBootstraped() && (ignore_main_window || (_framework != nullptr && fr()->isActive()));
}

CoreModes Core::mode()
{
    return _mode;
}

QString Core::coreInstanceKey(bool halt_if_error)
{
    if (halt_if_error)
        Z_CHECK_X(!_core_instance_key.isEmpty(), "execute Core::bootstrap first");
    return _core_instance_key;
}

#ifdef Z_JEMALLOC_ACTIVATED
// функция копирования строк из libxml
char* z_xmlCharStrndup(const char* cur, int len)
{
    int i;
    char* ret;

    if ((cur == NULL) || (len < 0))
        return (NULL);
    ret = (char*)je_malloc((len + 1) * sizeof(char));
    if (ret == NULL) {
        Core::logError("z_xmlCharStrndup error");
        return (NULL);
    }
    for (i = 0; i < len; i++) {
        ret[i] = cur[i];
        if (ret[i] == 0)
            return (ret);
    }
    ret[len] = 0;
    return (ret);
}

char* z_xmlCharStrdup(const char* cur)
{
    const char* p = cur;
    if (cur == NULL)
        return (NULL);
    while (*p != '\0')
        p++; /* non input consuming */
    return (z_xmlCharStrndup(cur, p - cur));
}
#endif

void Core::bootstrap(CoreModes mode, const QString& core_instance_key, const QList<DatabaseInformation>& database_info,
                     const DatabaseID& default_database_id, const QMap<QLocale::Language, QString>& translation,
                     const QMap<EntityCode, QMap<QString, QVariant>>& plugins_options, int terminate_timeout_ms,
                     const EntityCacheConfig& cache_config, int default_cache_size, int history_size, bool is_install_error_handlers)
{
    if (isBootstraped()) {
        logInfo("zf::Core already bootstraped");
        return;
    }

    Z_CHECK(static_cast<int>(mode) != static_cast<int>(CoreMode::Undefinded));
    Z_CHECK(!core_instance_key.isEmpty());

    QNetworkProxy::setApplicationProxy(QNetworkProxy());

#ifdef GITHASH_STR
    _build_version = GITHASH_STR;
#else
    _build_version = QString::number(Utils::extractSvnRevision(BUILD_VERSION_STR));
#endif
    _application_version = Utils::getVersion();
    qApp->setApplicationVersion(_application_version.text());

    _database_info = std::make_unique<QHash<DatabaseID, DatabaseInformation>>();
    if (database_info.isEmpty()) {
        Z_CHECK(!_default_database_id.isValid());

    } else {
        bool def_found = false;
        for (auto& d : database_info) {
            Z_CHECK(d.isValid());
            Z_CHECK(!_database_info->contains(d.id()));
            if (d.id() == default_database_id)
                def_found = true;

            _database_info->insert(d.id(), d);
        }
        Z_CHECK(def_found);
    }
    _default_database_id = default_database_id;

#ifdef Z_JEMALLOC_ACTIVATED
    // переопределение функций аллокации
    xmlMemSetup(je_free, je_malloc, je_realloc, z_xmlCharStrdup);
#endif

    _mode = mode;
    _core_instance_key = core_instance_key.simplified();
    _core_instance_key.replace(' ', '_');

    Z_CHECK_X(!_framework, utf("Ядро уже инициализировано"));

    _cumulative_error = std::make_unique<CumulativeError>();

    XMLDocument::init();

    _message_dispatcher = new MessageDispatcher();
    messageDispatcher()->bootstrap();

    _module_manager = std::make_unique<ModuleManager>(plugins_options);

    _database_manager = new DatabaseManager(terminate_timeout_ms);
    databaseManager()->bootstrap();

    _model_manager = new ModelManager(cache_config, default_cache_size, history_size);

    _model_keeper = new ModelKeeper();

    _framework
        = new Framework(_model_manager.get(), _module_manager.get(), _database_manager.get(), is_install_error_handlers, translation);

    _framework->localSettings()->apply();

    _is_bootstraped = 1;
}

void Core::freeResources()
{
    if (!isBootstraped())
        return;

    Error error = _module_manager->saveModulesConfiguration();
    if (error.isError())
        logError(error);

    if (_database_manager)
        _database_manager->shutdown();
    _module_manager->shutdown();

    if (_framework)
        error = _framework->saveSystemData();
    if (error.isError())
        logError(error);

    _is_bootstraped = 0;

    _model_keeper.reset();

    _model_manager.reset();
    _database_manager.reset();
    _module_manager.reset();    
    _message_dispatcher.reset();
    XMLDocument::cleanup();
#ifdef Q_OS_WINDOWS
    MSWidget::clearEngine();
#endif

    if (mode().testFlag(CoreMode::Application))
        zf::Core::logInfo("======= application closed =======");
    _framework.reset();

#ifdef Z_DEBUG_MEMORY_LEAK
    auto leaks = DebugMemoryLeak::leaks();
    if (!leaks.isEmpty())
        qDebug() << ">>>>>>>>>> MEMORY LEAKS FOUND <<<<<<<<<<";
    else
        qDebug() << ">>>>>>>>>> NO MEMORY LEAKS <<<<<<<<<<";
    for (auto i = leaks.constBegin(); i != leaks.constEnd(); ++i) {
        qDebug() << i.key() << ":" << i.value();
    }
    if (!leaks.isEmpty())
        qDebug() << ">>>>>>>>>> MEMORY LEAKS FOUND <<<<<<<<<<";
#endif

#ifdef Z_JEMALLOC_ACTIVATED
    je_malloc_stats_print(nullptr, nullptr, nullptr);
#endif
}

Version Core::applicationVersion()
{
    return _application_version;
}

QString Core::buildVersion()
{
    return _build_version;
}

QDateTime Core::buildTime()
{
    QDate date = QLocale(QLocale::English).toDate(QString(__DATE__).simplified(), "MMM d yyyy");
    QTime time = QLocale(QLocale::English).toTime(__TIME__, "hh:mm:ss");
    return QDateTime(date, time);
}

QString Core::applicationVersionFull()
{
    return QString("%1: %2 (%3 %4), %5")
        .arg(translate(TR::ZFT_VERSION))
        .arg(_application_version.text(2))
        .arg(translate(TR::ZFT_REVISION).toLower())
        .arg(_build_version)
        .arg(Utils::variantToString(buildTime()));
}

void Core::clearEntityCache()
{
    fr(); // для проверки на инициализацию
    _model_manager->clearCache();
}

void Core::clearEntityCache(const EntityCode& entity_code)
{
    fr(); // для проверки на инициализацию
    Z_CHECK(entity_code.isValid());
    _model_manager->clearCache(entity_code.value());
}

void Core::clearEntityCache(const Uid& uid)
{
    fr(); // для проверки на инициализацию
    _model_manager->clearCache(uid);
}

int Core::entityCacheSize(const EntityCode& entity_code)
{
    fr(); // для проверки на инициализацию
    Z_CHECK(entity_code.isValid());
    return _model_manager->cacheSize(entity_code);
}

MessageDispatcher* Core::messageDispatcher(bool halt_if_null)
{
    Z_CHECK(!halt_if_null || _message_dispatcher != nullptr);
    return _message_dispatcher.get();
}

DatabaseManager* Core::databaseManager(bool halt_if_null)
{
    Z_CHECK(!halt_if_null || _database_manager != nullptr);
    return _database_manager.get();
}

ModelManager* Core::modelManager(bool halt_if_null)
{
    Z_CHECK(!halt_if_null || _model_manager != nullptr);
    return _model_manager.get();
}

const QContiguousCache<QPair<SharedObjectEventType, Uid>>& Core::modelManagerHistory()
{
    fr();
    return _model_manager->history();
}

MessageID Core::removeModels(const I_ObjectExtension* requester, const UidList& entity_uids, const QList<DataPropertySet>& properties,
                             const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& by_user)
{
    MessageID feedback_id;
    Z_CHECK(fr()->removeModelsAsync(requester, entity_uids, properties, parameters, QList<bool> {}, by_user, feedback_id));
    return feedback_id;
}

MessageID Core::removeModels(const I_ObjectExtension* requester, const UidList& entity_uids,
                             const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& by_user)
{
    zf::DBCommandRemoveEntityMessage message(entity_uids, parameters, by_user);
    Z_CHECK(messageDispatcher()->postMessage(requester, databaseManager(), message));
    return message.messageId();
}

MessageID Core::removeModel(const I_ObjectExtension* requester, const Uid& entity_uid, const QMap<QString, QVariant>& parameters,
                            const QVariant& by_user)
{
    return removeModels(requester, UidList {entity_uid}, QList<QMap<QString, QVariant>> {parameters},
                        by_user.isValid() ? QList<bool> {by_user.toBool()} : QList<bool> {});
}

Error Core::removeModelsSync(const UidList& entity_uids, int timeout_ms)
{
    zf::DBCommandRemoveEntityMessage message(entity_uids);
    auto feedback = messageDispatcher()->sendMessage(databaseManager(), message, timeout_ms);
    if (feedback.messageType() == MessageType::Error)
        return ErrorMessage(feedback).error();

    return {};
}

QMap<ModelPtr, Error> Core::waitForLoadModel(const UidList& model_uids, QList<ModelPtr>& models, int timeout_ms, const QString& wait_message)
{    
    Z_CHECK_ERROR(getModels(model_uids, models)); // тут не должно быть ошибки, т.к. нет загрузки из БД. если она есть - что-то совсем сломалось
    Z_CHECK(models.count() == model_uids.count());
    return waitForLoadModel(models, timeout_ms, wait_message);
}

QMap<ModelPtr, Error> Core::waitForLoadModel(const QList<ModelPtr>& models, int timeout_ms, const QString& wait_message)
{
    QList<ModelPtr> need_wait;
    for (auto& m : qAsConst(models)) {
        Z_CHECK_NULL(m);
        if (!m->isPersistent())
            continue;

        if (!m->isLoading()) {
            auto invalidated_props = m->data()->invalidatedProperties();
            if (!invalidated_props.isEmpty()) {
                // если данные не валидны, то запускаем загрузку
                m->load({}, invalidated_props);
            }
        }

        if (m->isLoading())
            need_wait << m;
    }

    if (need_wait.isEmpty())
        return {};

    SyncWaitWindow* wait_window = nullptr;
    if (!wait_message.isEmpty()) {
        wait_window = createSyncWaitWindow(wait_message);
        wait_window->run();
    }

    QEventLoop loop;
    QMap<ModelPtr, Error> all_errors;
    QList<QMetaObject::Connection> connections;
    int loaded = 0;

    QTimer timeout;
    if (timeout_ms > 0) {
        timeout.setSingleShot(true);
        timeout.setInterval(timeout_ms);
        Z_CHECK(timeout.connect(&timeout, &QTimer::timeout, &timeout, [&]() {
            all_errors[nullptr] = Error::timeoutError();
            loop.quit();
        }));
        timeout.start();
    }

    for (auto& m : qAsConst(need_wait)) {
        auto con
            = timeout.connect(m.get(), &Model::sg_finishLoad, m.get(),
                              [&, m, wait_window](const zf::Error& error, const zf::LoadOptions& load_options, const zf::DataPropertySet& properties) {
                                  Q_UNUSED(load_options)
                                  Q_UNUSED(properties)

                                  if (error.isError())
                                      all_errors[m] = error;
                                  loaded++;

                                  if (wait_window != nullptr)
                                      Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                                          ProgressMessage(100 * loaded / need_wait.count())));

                                  if (loaded == need_wait.count()) {
                                      if (timeout.isActive())
                                          timeout.stop();
                                      loop.quit();
                                  }
                              });
        Z_CHECK(con);
        connections << con;
    }

    loop.exec(QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents);

    for (const auto& c : qAsConst(connections)) {
        Z_CHECK(QObject::disconnect(c));
    }

    if (wait_window != nullptr)
        delete wait_window;

    return all_errors;
}

Error Core::waitForLoadModel(const ModelPtr& model, int timeout_ms, const QString& wait_message)
{
    auto error = waitForLoadModel(QList<ModelPtr> {model}, timeout_ms, wait_message);
    return error.isEmpty() ? Error() : error.constBegin().value();
}

QMap<ModelPtr, Error> Core::waitForSaveModel(const QList<ModelPtr>& models, int timeout_ms, const QString& wait_message, bool collect_non_critical_error)
{
    QList<ModelPtr> need_wait;
    for (auto& m : qAsConst(models)) {
        Z_CHECK_NULL(m);
        if (m->isSaving())
            need_wait << m;
    }

    if (need_wait.isEmpty())
        return {};

    SyncWaitWindow* wait_window = nullptr;
    if (!wait_message.isEmpty()) {
        wait_window = createSyncWaitWindow(wait_message);
        wait_window->run();
    }

    QEventLoop loop;
    QMap<ModelPtr, Error> all_errors;
    QList<QMetaObject::Connection> connections;
    int saved = 0;

    QTimer timeout;
    if (timeout_ms > 0) {
        timeout.setSingleShot(true);
        timeout.setInterval(timeout_ms);
        Z_CHECK(timeout.connect(&timeout, &QTimer::timeout, &timeout, [&]() {
            all_errors[nullptr] = Error::timeoutError();
            loop.quit();
        }));
        timeout.start();
    }

    for (auto& m : qAsConst(need_wait)) {
        auto con = timeout.connect(m.get(), &Model::sg_finishSave, m.get(),
            [&, m, collect_non_critical_error, wait_window](const zf::Error& error, const zf::DataPropertySet& requested_properties,
                const zf::DataPropertySet& saved_properties, const zf::Uid& persistent_uid) {
                Q_UNUSED(requested_properties)
                Q_UNUSED(saved_properties)
                Q_UNUSED(persistent_uid)

                if (error.isError())
                    all_errors[m] = error;
                else if (collect_non_critical_error && m->nonCriticalSaveError().isError())
                    all_errors[m] = m->nonCriticalSaveError();
                saved++;

                if (wait_window != nullptr)
                    Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                        ProgressMessage(100 * saved / need_wait.count())));

                if (saved == need_wait.count()) {
                    if (timeout.isActive())
                        timeout.stop();
                    loop.quit();
                }
            });
        Z_CHECK(con);
        connections << con;
    }

    loop.exec(QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents);

    for (const auto& c : qAsConst(connections)) {
        Z_CHECK(QObject::disconnect(c));
    }

    if (wait_window != nullptr)
        delete wait_window;

    return all_errors;
}

Error Core::waitForSaveModel(const ModelPtr& model, int timeout_ms, const QString& wait_message)
{
    auto error = waitForSaveModel(QList<ModelPtr> {model}, timeout_ms, wait_message);
    return error.isEmpty() ? Error() : error.constBegin().value();
}

QMap<ModelPtr, Error> Core::waitForRemoveModel(const QList<ModelPtr>& models, int timeout_ms, const QString& wait_message)
{
    QList<ModelPtr> need_wait;
    for (auto& m : qAsConst(models)) {
        Z_CHECK_NULL(m);
        if (m->isRemoving() && m->isPersistent())
            need_wait << m;
    }

    if (need_wait.isEmpty())
        return {};

    SyncWaitWindow* wait_window = nullptr;
    if (!wait_message.isEmpty()) {
        wait_window = createSyncWaitWindow(wait_message);
        wait_window->run();
    }

    QEventLoop loop;
    QMap<ModelPtr, Error> all_errors;
    QList<QMetaObject::Connection> connections;
    int removed = 0;

    QTimer timeout;
    if (timeout_ms > 0) {
        timeout.setSingleShot(true);
        timeout.setInterval(timeout_ms);
        Z_CHECK(timeout.connect(&timeout, &QTimer::timeout, &timeout, [&]() {
            all_errors[nullptr] = Error::timeoutError();
            loop.quit();
        }));
        timeout.start();
    }

    for (auto& m : qAsConst(need_wait)) {
        auto con = timeout.connect(m.get(), &Model::sg_finishRemove, m.get(), [&, m](const zf::Error& error) {
            if (error.isError())
                all_errors[m] = error;
            removed++;

            if (wait_window != nullptr)
                Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                    ProgressMessage(100 * removed / need_wait.count())));

            if (removed == need_wait.count()) {
                if (timeout.isActive())
                    timeout.stop();
                loop.quit();
            }
        });
        Z_CHECK(con);
        connections << con;
    }

    loop.exec(QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents);

    for (const auto& c : qAsConst(connections)) {
        Z_CHECK(QObject::disconnect(c));
    }

    if (wait_window != nullptr)
        delete wait_window;

    return all_errors;
}

Error Core::waitForRemoveModel(const ModelPtr& model, int timeout_ms, const QString& wait_message)
{
    auto error = waitForRemoveModel(QList<ModelPtr> {model}, timeout_ms, wait_message);
    return error.isEmpty() ? Error() : error.constBegin().value();
}

QMap<ModelPtr, Error> Core::waitForModelOperation(const QList<QPair<ModelPtr, ObjectActionType>>& models, int timeout_ms,
    const QString& wait_message, bool collect_non_critical_error)
{
    QList<ModelPtr> need_wait_load;
    QList<ModelPtr> need_wait_remove;
    QList<ModelPtr> need_wait_save;

    for (auto& m : qAsConst(models)) {
        Z_CHECK_NULL(m.first);
        Z_CHECK(m.second == ObjectActionType::View || m.second == ObjectActionType::Remove || m.second == ObjectActionType::Modify);

        if (m.second == ObjectActionType::View && m.first->isPersistent() && m.first->isLoading())
            need_wait_load << m.first;
        if (m.second == ObjectActionType::Remove && m.first->isPersistent() && m.first->isRemoving())
            need_wait_remove << m.first;
        if (m.second == ObjectActionType::Modify && m.first->isSaving())
            need_wait_save << m.first;
    }

    if (need_wait_load.isEmpty() && need_wait_remove.isEmpty() && need_wait_save.isEmpty())
        return {};

    SyncWaitWindow* wait_window = nullptr;
    if (!wait_message.isEmpty()) {
        wait_window = createSyncWaitWindow(wait_message);
        wait_window->run();
    }

    QEventLoop loop;
    QMap<ModelPtr, Error> all_errors;
    QList<QMetaObject::Connection> connections;
    int loaded = 0;
    int removed = 0;
    int saved = 0;

    QTimer timeout;
    if (timeout_ms > 0) {
        timeout.setSingleShot(true);
        timeout.setInterval(timeout_ms);
        Z_CHECK(timeout.connect(&timeout, &QTimer::timeout, &timeout, [&]() {
            all_errors[nullptr] = Error::timeoutError();
            loop.quit();
        }));
        timeout.start();
    }

    for (auto& m : qAsConst(need_wait_load)) {
        auto con = timeout.connect(
            m.get(), &Model::sg_finishLoad, m.get(),
            [&, m](const zf::Error& error, const zf::LoadOptions& load_options, const zf::DataPropertySet& properties) {
                Q_UNUSED(load_options)
                Q_UNUSED(properties)

                if (error.isError())
                    all_errors[m] = error;
                loaded++;

                if (wait_window != nullptr)
                    Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                        ProgressMessage(100 * (loaded + removed + saved) / (need_wait_load.count() + need_wait_remove.count() + need_wait_save.count()))));

                if (loaded == need_wait_load.count() && removed == need_wait_remove.count() && saved == need_wait_save.count()) {
                    if (timeout.isActive())
                        timeout.stop();
                    loop.quit();
                }
            });
        Z_CHECK(con);
        connections << con;
    }

    for (auto& m : qAsConst(need_wait_remove)) {
        auto con = timeout.connect(m.get(), &Model::sg_finishRemove, m.get(), [&, m](const zf::Error& error) {
            if (error.isError())
                all_errors[m] = error;
            removed++;

            if (wait_window != nullptr)
                Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                    ProgressMessage(100 * (loaded + removed + saved) / (need_wait_load.count() + need_wait_remove.count() + need_wait_save.count()))));

            if (loaded == need_wait_load.count() && removed == need_wait_remove.count() && saved == need_wait_save.count()) {
                if (timeout.isActive())
                    timeout.stop();
                loop.quit();
            }
        });
        Z_CHECK(con);
        connections << con;
    }

    for (auto& m : qAsConst(need_wait_save)) {
        auto con = timeout.connect(m.get(), &Model::sg_finishSave, m.get(),
            [&, m, collect_non_critical_error](const zf::Error& error, const zf::DataPropertySet& requested_properties,
                const zf::DataPropertySet& saved_properties, const zf::Uid& persistent_uid) {
                Q_UNUSED(requested_properties)
                Q_UNUSED(saved_properties)
                Q_UNUSED(persistent_uid)
                if (error.isError())
                    all_errors[m] = error;
                else if (collect_non_critical_error && m->nonCriticalSaveError().isError())
                    all_errors[m] = m->nonCriticalSaveError();
                saved++;

                if (wait_window != nullptr)
                    Z_CHECK(messageDispatcher()->postMessage(fr(), wait_window->progressUid(),
                        ProgressMessage(100 * (loaded + removed + saved) / (need_wait_load.count() + need_wait_remove.count() + need_wait_save.count()))));

                if (loaded == need_wait_load.count() && removed == need_wait_remove.count()
                    && saved == need_wait_save.count()) {
                    if (timeout.isActive())
                        timeout.stop();
                    loop.quit();
                }
            });
        Z_CHECK(con);
        connections << con;
    }

    loop.exec(QEventLoop::ExcludeSocketNotifiers | QEventLoop::ExcludeUserInputEvents);

    for (const auto& c : qAsConst(connections)) {
        Z_CHECK(QObject::disconnect(c));
    }

    if (wait_window != nullptr)
        delete wait_window;

    return all_errors;
}

DataTablePtr Core::getDbTable(Error& error, const DatabaseID& database_id, const TableID& table_id, DataRestrictionPtr restriction,
                              const FieldIdList& mask, int timeout_ms)
{
    error.clear();
    auto feedback = zf::Core::messageDispatcher()->sendMessage(
        databaseManager(), DBCommandGetDataTableMessage(database_id, table_id, restriction, mask), timeout_ms);
    if (feedback.messageType() == zf::MessageType::Error) {
        error = zf::ErrorMessage(feedback).error();
        Z_CHECK(error.isError());
        return nullptr;
    }

    auto res = zf::DBEventDataTableMessage(feedback).result();
    Z_CHECK_NULL(res);
    return res;
}

DataTablePtr Core::getDbTable(Error& error, const TableID& table_id, DataRestrictionPtr restriction, const FieldIdList& mask,
                              int timeout_ms)
{
    return getDbTable(error, Core::defaultDatabase(), table_id, restriction, mask, timeout_ms);
}

DataTablePtr Core::getDbTable(Error& error, const DatabaseID& database_id, const FieldID& field_id, const QList<int>& keys,
                              const FieldIdList& mask, int timeout_ms)
{
    Z_CHECK(field_id.isValid());

    error.clear();
    auto feedback = zf::Core::messageDispatcher()->sendMessage(databaseManager(),
                                                               DBCommandGetDataTableMessage(database_id, field_id, keys, mask), timeout_ms);
    if (feedback.messageType() == zf::MessageType::Error) {
        error = zf::ErrorMessage(feedback).error();
        Z_CHECK(error.isError());
        return nullptr;
    }

    auto res = zf::DBEventDataTableMessage(feedback).result();
    Z_CHECK_NULL(res);
    return res;
}

DataTablePtr Core::getDbTable(Error& error, const FieldID& field_id, const QList<int>& keys, const FieldIdList& mask, int timeout_ms)
{
    return getDbTable(error, Core::defaultDatabase(), field_id, keys, mask, timeout_ms);
}

DataTablePtr Core::getDbTable(Error& error, const DatabaseID& database_id, const FieldID& field_id, int key, const FieldIdList& mask,
                              int timeout_ms)
{
    return getDbTable(error, database_id, field_id, QList<int> {key}, mask, timeout_ms);
}

DataTablePtr Core::getDbTable(Error& error, const FieldID& field_id, int key, const FieldIdList& mask, int timeout_ms)
{
    return getDbTable(error, Core::defaultDatabase(), field_id, key, mask, timeout_ms);
}

bool Core::postDatabaseMessage(const I_ObjectExtension* sender, const Message& message)
{
    return messageDispatcher()->postMessage(sender, databaseManager(), message);
}

Message Core::sendDatabaseMessage(const Message& message, int timeout_ms)
{
    return messageDispatcher()->sendMessage(databaseManager(), message, timeout_ms);
}

void Core::entityChanged(const Uid &sender, const UidList &entity_uids, const QList<bool> &by_user)
{
    databaseManager()->entityChanged(sender, entity_uids, by_user);
}

void Core::entityChanged(const Uid &sender, const Uid &entity_uid)
{
    databaseManager()->entityChanged(sender, entity_uid);
}

void Core::entityChanged(const Uid &sender, const Uid &entity_uid, bool by_user)
{
    databaseManager()->entityChanged(sender, entity_uid, by_user);
}

void Core::entityChanged(const Uid &sender, const EntityCode &entity_code)
{
    databaseManager()->entityChanged(sender, entity_code);
}

void Core::entityRemoved(const Uid &sender, const UidList &entity_uids, const QList<bool> &by_user)
{
    databaseManager()->entityRemoved(sender, entity_uids, by_user);
}

void Core::entityRemoved(const Uid &sender, const Uid &entity_uid)
{
    databaseManager()->entityRemoved(sender, entity_uid);
}

void Core::entityRemoved(const Uid &sender, const Uid &entity_uid, bool by_user)
{
    databaseManager()->entityRemoved(sender, entity_uid, by_user);
}

void Core::entityCreated(const Uid &sender, const UidList &entity_uids, const QList<bool> &by_user)
{
    databaseManager()->entityCreated(sender, entity_uids, by_user);
}

void Core::entityCreated(const Uid &sender, const Uid &entity_uid)
{
    databaseManager()->entityCreated(sender, entity_uid);
}

void Core::entityCreated(const Uid &sender, const Uid &entity_uid, bool by_user)
{
    databaseManager()->entityCreated(sender, entity_uid, by_user);
}

MessageID Core::getDbTableAsync(const I_ObjectExtension* sender, const DatabaseID& database_id, const TableID& table_id,
                                DataRestrictionPtr restriction, const FieldIdList& mask)
{
    DBCommandGetDataTableMessage msg(database_id, table_id, restriction, mask);
    Z_CHECK(zf::Core::messageDispatcher()->postMessage(sender, databaseManager(), msg));
    return msg.messageId();
}

MessageID Core::getDbTableAsync(const I_ObjectExtension* sender, const TableID& table_id, DataRestrictionPtr restriction,
                                const FieldIdList& mask)
{
    return getDbTableAsync(sender, defaultDatabase(), table_id, restriction, mask);
}

bool Core::getEntityValue(const Uid& source_uid, const QVariant& key_value, QVariant& result, MessageID& feedback_id,
                          ModelPtr& source_model, Error& error, const DataProperty& dataset_property, const DataProperty& key_column,
                          const DataProperty& result_column, const I_ObjectExtension* requester, const QWidgetList& update_on_data_loaded,
                          int result_role)
{
    return fr()->getEntityValue(source_uid, key_value, result, feedback_id, source_model, error, dataset_property, key_column,
                                result_column, requester, update_on_data_loaded, result_role);
}

bool Core::getEntityValue(const Uid& source_uid, const QVariant& key_value, QVariant& result, ModelPtr& source_model,
                          Error& error)
{
    MessageID feedback_id;
    return getEntityValue(source_uid, key_value, result, feedback_id, source_model, error);
}

bool Core::getEntityValue(const PropertyLookupPtr& lookup, const QVariant& key_value, QVariant& result,
                          ModelPtr& source_model, Error& error, const QWidgetList& update_on_data_loaded)
{
    Z_CHECK_NULL(lookup);
    Z_CHECK(lookup->listEntity().isValid());
    MessageID feedback_id;
    return getEntityValue(lookup->listEntity(), key_value, result, feedback_id, source_model, error, lookup->datasetProperty(),
                          lookup->dataColumn(), lookup->displayColumn(), nullptr, update_on_data_loaded, lookup->displayColumnRole());
}

QVariant Core::getEntityValue(const Uid& source_uid, const QVariant& key_value, ModelPtr& source_model)
{
    QVariant res;
    Error error;
    if (getEntityValue(source_uid, key_value, res, source_model, error))
        return res;
    return QVariant();
}

QVariant Core::getEntityValue(const Uid& source_uid, const QVariant& key_value)
{    
    ModelPtr source_model;
    return getEntityValue(source_uid, key_value, source_model);
}

QVariant Core::getEntityValueSync(const Uid& source_uid, const QVariant& key_value, Error& error, const PropertyID& dataset_property_id,
                                  const PropertyID& key_column_id, const PropertyID& result_column_id, int result_role)
{
    auto model = getModel<Model>(source_uid, {dataset_property_id}, error);
    if (error.isOk())
        error = waitForLoadModel(model);
    if (error.isError())
        return QVariant();
    QVariant value;
    MessageID feedback_id;

    DataProperty dataset_property = model->property(dataset_property_id);
    DataProperty key_column = model->property(key_column_id);
    DataProperty result_column = model->property(result_column_id);

    getEntityValue(source_uid, key_value, value, feedback_id, model, error, dataset_property, key_column, result_column, nullptr,
                   QWidgetList(), result_role);
    if (error.isError())
        return QVariant();
    return value;
}

QVariant Core::getEntityValueSync(const Uid& source_uid, const QVariant& key_value, int timeout_sec)
{
    auto dataset_property = DataStructure::structure(source_uid)->singleDataset();
    Z_CHECK(dataset_property.isValid());

    Error error;
    auto model = getModel<Model>(source_uid, {dataset_property}, error);
    if (error.isOk())
        error = waitForLoadModel(model, timeout_sec);
    Z_CHECK_ERROR(error);
    return getEntityValue(source_uid, key_value);
}

QJSEngine* Core::jsEngine()
{
    return fr()->jsEngine();
}

Configuration* Core::configuration(int version)
{
    return fr()->configuration(version);
}

Configuration* Core::entityConfiguration(const EntityCode& entity_code, const QString& key, int version)
{
    return fr()->entityConfiguration(entity_code, key, version);
}

QString Core::lastUsedDirectory()
{
    if (_lastUsedDirectory.isEmpty())
        _lastUsedDirectory = fr()->coreConfiguration()->value("LAST_USED_DIR").toString();

    if (!_lastUsedDirectory.isEmpty() && !QDir(_lastUsedDirectory).exists())
        _lastUsedDirectory.clear();

    return _lastUsedDirectory.isEmpty() ? Utils::documentsLocation() : _lastUsedDirectory;
}

void Core::updateLastUsedDirectory(const QString& folder)
{
    QFileInfo fi(folder);

    if (fi.isDir())
        _lastUsedDirectory = QDir(folder).absolutePath();
    else
        _lastUsedDirectory = fi.absolutePath();

    fr()->coreConfiguration()->setValue("LAST_USED_DIR", _lastUsedDirectory);
}

I_Plugin* Core::getPluginHelper(const EntityCode& entity_code, Error& error)
{
    return fr()->getPlugin(entity_code, error);
}

I_Plugin* Core::getPlugin(const EntityCode& entity_code)
{
    return getPlugin<I_Plugin>(entity_code);
}

EntityCodeList Core::getAllModules()
{
    return fr()->getAllModules();
}

WindowManager* Core::windowManager(bool halt_if_null)
{
    Z_CHECK(!halt_if_null || fr()->windowManager() != nullptr);
    return fr()->windowManager();
}

QString Core::getEntityNameSync(const Uid& uid, Error& error, const PropertyID& name_property_id, QLocale::Language language,
                                int timeout_ms)
{
    QStringList res = getEntityNamesSync({uid}, error, name_property_id.isValid() ? PropertyIDList {name_property_id} : PropertyIDList {},
                                         language, timeout_ms);
    return res.isEmpty() ? QString() : res.at(0);
}

QStringList Core::getEntityNamesSync(const UidList& uids, Error& error, const PropertyIDList& name_property_ids, QLocale::Language language,
                                     int timeout_ms)
{
    Z_CHECK(!uids.isEmpty());
    DataPropertyList p;
    if (!name_property_ids.isEmpty()) {
        Z_CHECK(uids.count() == name_property_ids.count());
        for (int i = 0; i < name_property_ids.count(); i++) {
            p << DataProperty::property(uids.at(i).entityCode(), name_property_ids.at(i));
        }
    }

    auto result = messageDispatcher()->sendMessage(_model_manager.get(), MMCommandGetEntityNamesMessage(uids, p, language), timeout_ms);
    if (result.messageType() == MessageType::Error) {
        error = ErrorMessage(result).error();
        return QStringList();
    }

    MMEventEntityNamesMessage msg(result);

    // порядок имен может не совпадать
    auto names = msg.names();
    auto entity_uids = msg.entityUids();
    auto errors = msg.errors();
    QStringList res;
    error.clear();
    for (auto& u : uids) {
        int pos = entity_uids.indexOf(u);
        Z_CHECK(pos >= 0);
        res << names.at(pos);
        error << errors.at(pos);
    }

    return res;
}

MessageID Core::getEntityNames(const I_ObjectExtension* sender, const UidList& uids, const PropertyIDList& name_property_ids,
                               QLocale::Language language)
{
    Z_CHECK(!uids.isEmpty());
    DataPropertyList p;
    if (!name_property_ids.isEmpty()) {
        Z_CHECK(uids.count() == name_property_ids.count());
        for (int i = 0; i < name_property_ids.count(); i++) {
            p << DataProperty::property(uids.at(i).entityCode(), name_property_ids.at(i));
        }
    }

    MMCommandGetEntityNamesMessage message(uids, p, language);
    Z_CHECK(messageDispatcher()->postMessage(sender, _model_manager.get(), message));

    return message.messageId();
}

MessageID Core::getEntityName(const I_ObjectExtension* sender, const Uid& uid, const PropertyID& name_property_ids,
                              QLocale::Language language)
{
    return getEntityNames(sender, {uid}, name_property_ids.isValid() ? PropertyIDList {name_property_ids} : PropertyIDList(), language);
}

Uid Core::pluginUid(const EntityCode& code)
{
    return Uid::uniqueEntity(code);
}

I_Plugin* Core::getPlugin(const EntityCode& entity_code, Error& error)
{
    return getPlugin<I_Plugin>(entity_code, error);
}

bool Core::invalidateModels(const EntityCode& entity_code)
{
    bool res = false;
    for (auto& m : fr()->openedModels()) {
        if (m->entityUid().entityCode() != entity_code)
            continue;
        res = true;
        m->invalidate();
    }
    return res;
}

Error Core::getModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
                      const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, QList<ModelPtr>& models,
                      MessageID& feedback_message_id)
{
    return fr()->getModels(requester, entity_uid_list, load_options_list, properties_list, all_if_empty_list, models, feedback_message_id);
}

Error Core::getModels(const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
                      const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, QList<ModelPtr>& models)
{
    MessageID feedback_message_id;
    return getModels(fr(), entity_uid_list, load_options_list, properties_list, all_if_empty_list, models, feedback_message_id);
}

Error Core::getModels(const UidList& entity_uid_list, QList<ModelPtr>& models)
{
    QList<LoadOptions> load_options_list;
    return getModels(entity_uid_list, load_options_list, {}, Utils::makeList(entity_uid_list.count(), true), models);
}

bool Core::loadModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
                      const QList<DataPropertySet>& properties_list, MessageID& feedback_message_id)
{
    return fr()->loadModels(requester, entity_uid_list, load_options_list, properties_list, {}, feedback_message_id);
}

bool Core::loadModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
                      const QList<PropertyIDSet>& properties_list, MessageID& feedback_message_id)
{
    QList<DataPropertySet> props;
    Z_CHECK(properties_list.isEmpty() || properties_list.count() == entity_uid_list.count());
    for (int i = 0; i < entity_uid_list.count(); i++) {
        auto structure = getModuleDataStructure(entity_uid_list.at(i));
        if (properties_list.isEmpty() || properties_list.at(i).isEmpty()) {
            props << structure->propertiesMain();
        } else {
            DataPropertySet p_set;
            for (auto& p : properties_list.at(i)) {
                p_set << structure->property(p);
            }
            props << p_set;
        }
    }

    return loadModels(requester, entity_uid_list, load_options_list, props, feedback_message_id);
}

bool Core::ask(const QString& text, InformationType type, const QString& detail, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());
    MessageBox::MessageType msg_type;
    if (type == InformationType::Critical || type == InformationType::Error || type == InformationType::Fatal)
        msg_type = MessageBox::MessageType::AskError;
    else if (type == InformationType::Warning)
        msg_type = MessageBox::MessageType::AskWarning;
    else
        msg_type = MessageBox::MessageType::Ask;

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(msg_type, text, detail, entity_uid);
    return static_cast<QDialogButtonBox::StandardButton>(msgBox->exec()) == QDialogButtonBox::Yes;
}

bool Core::ask(const QString& text, const Error& detail, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());
    Z_CHECK_X(detail.isError(), "Попытка вызвать Core::ask для аргумента Error, который не содержит ошибку");

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(MessageBox::MessageType::AskError, text, detail.fullText(0, true), entity_uid);
    return static_cast<QDialogButtonBox::StandardButton>(msgBox->exec()) == QDialogButtonBox::Yes;
}

bool Core::ask(const Error& error, InformationType type)
{
    Z_CHECK(Utils::isMainThread());
    Z_CHECK_X(error.isError(), "Попытка вызвать ZCore::ask для аргумента Error, который не содержит ошибку");

    MessageBox::MessageType msg_type;
    if (type == InformationType::Critical || type == InformationType::Error || type == InformationType::Fatal)
        msg_type = MessageBox::MessageType::AskError;
    else if (type == InformationType::Warning)
        msg_type = MessageBox::MessageType::AskWarning;
    else
        msg_type = MessageBox::MessageType::Ask;

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(msg_type, error.text(), error.textDetailFull());
    return static_cast<QDialogButtonBox::StandardButton>(msgBox->exec()) == QDialogButtonBox::Yes;
}

QMessageBox::StandardButton Core::choice(const QString& text, QMessageBox::StandardButton button1,
    QMessageBox::StandardButton button2, QMessageBox::StandardButton button3, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());

    QList<int> buttons;
    buttons << button1 << button2 << button3;

    ObjectExtensionPtr<MessageBox> msgBox
        = new MessageBox(MessageBox::MessageType::Choice, buttons, QStringList(), QList<int>(), text, "", entity_uid);
    return static_cast<QMessageBox::StandardButton>(msgBox->exec());
}

QMessageBox::StandardButton Core::choice(const QString& text, const Uid& entity_uid)
{
    QMessageBox::StandardButton res = choice(text, QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel, entity_uid);
    if (res == QMessageBox::NoButton)
        return QMessageBox::Cancel; // Нажат escape
    else
        return res;
}

void Core::info(const QString& text, const QString& detail, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(MessageBox::MessageType ::Info, text, detail, entity_uid);
    msgBox->exec();
}

void Core::info(const Log& log)
{
    logDialogHelper(log, Utils::resizeSvg(":/share_icons/blue/info.svg", 24));
}

void Core::warning(const QString& text, const QString& detail, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(MessageBox::MessageType::Warning, text, detail, entity_uid);
    msgBox->exec();
}

void Core::warning(const Log& log)
{
    logDialogHelper(log, Utils::resizeSvg(":/share_icons/warning.svg", 24));
}

void Core::error(const QString& text, const QString& detail, const Uid& entity_uid)
{
    Z_CHECK(Utils::isMainThread());

    logError(text);
    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(MessageBox::MessageType::Error, text, detail, entity_uid);
    msgBox->exec();
}

void Core::error(const QString& text, const Error& detail, const Uid& entity_uid)
{
    error(text, HtmlTools::table(detail.toStringList()), entity_uid);
}

void Core::error(const QString& text, const ErrorList& detail, const Uid& entity_uid)
{
    error(text, Error(detail), entity_uid);
}

static Error entityNotFoundErrorHelper(const Error& error, const ErrorList& child_errors)
{
    if (error.isEntityNotFoundError()) {
        Uid entity_uid = Uid::fromVariant(error.data());
        Z_CHECK(entity_uid.isValid());

        Error err;
        QString entity_name = Core::getEntityNameSync(entity_uid, err);
        if (err.isError()) {
            Core::logError(err.text() + "; object not found: " + entity_uid.toPrintable());
            Core::logError(err);

        } else {
            if (entity_name.isEmpty()) {
                Error error_local;
                auto plugin = Core::getPlugin(entity_uid.entityCode(), error_local);
                if (plugin != nullptr && !plugin->getModuleInfo().name().isEmpty()) {
                    if (entity_uid.isUniqueEntity())
                        entity_name = plugin->getModuleInfo().name();
                    else if (entity_uid.isEntity())
                        entity_name = plugin->getModuleInfo().name() + " (id: " + entity_uid.id().string() + ")";
                }
            }

            if (entity_name.isEmpty()) {
                Core::logError(QString("Undefined entity name for %1").arg(entity_uid.toPrintable()));

            } else {
                err = error;
                err.setData(QVariant());
                err.setType(ErrorType::Custom);
                err.setText(QStringLiteral("%1:<br><b>%2</b>").arg(ZF_TR(ZFT_ENTITY_NOT_FOUND)).arg(entity_name));
                err.removeChildErrors();
                err.addChildError(child_errors);
                Core::logError(err.text() + "; object not found: " + entity_uid.toPrintable());
                return err;
            }
        }

        return Error(error).setType(ErrorType::Custom);
    }
    return error;
}

void Core::error(const Error& error)
{
    Z_CHECK(Utils::isMainThread());

    Z_CHECK_X(error.isError(), "Попытка вызвать ZCore::error для аргумента Error, который не содержит ошибку");

    if (error.isCancelError())
        return;

    if (error.isEntityNotFoundError()) {
        Uid entity_uid = Uid::fromVariant(error.data());
        Z_CHECK(entity_uid.isValid());

        // проверяем дочерние ошибки
        ErrorList child_errors;
        for (auto& err : error.childErrors()) {
            if (err.isEntityNotFoundError()) {
                if (entity_uid == Uid::fromVariant(err.data()))
                    continue;
                else
                    child_errors << entityNotFoundErrorHelper(err, {});

            } else {
                child_errors << err;
            }
        }

        Core::error(entityNotFoundErrorHelper(error, child_errors));
        return;
    }

    if (!error.isFileDebug())
        logError(error);

    if (error.isHiddenError() || error.isHidden())
        return;

    ErrorList log_errors = error.childErrors(ErrorType::LogItems);

    if (!log_errors.isEmpty()) {
        if (log_errors.count() > 1)
            Z_HALT(utf("Ошибка содержит более одной дочерней ошибки типа LogItemsError"));

        Error not_log_errs;
        for (Error c_err : error.childErrors()) {
            if (c_err.type() != ErrorType::LogItems)
                not_log_errs << c_err.setModalWindow(error.isModalWindow());
        }
        if (not_log_errs.isError())
            // сначала показываем ошибку без журнала
            Core::error(not_log_errs);

        Z_CHECK(error.isModalWindow());
        // Для ошибок с журналом, он содержится в data
        Core::error(Log::fromVariant(log_errors.first().data()));
        return;
    }

    if (error.isFileDebug()) { // вывод в файл вместо отображения
        // получаем список файлов в каталоге
        QString folder = Utils::logLocation() + "/error_debug";
        auto err = Utils::makeDir(folder);

        QStringList files;
        if (err.isOk())
            err = Utils::getDirContent(folder, files, false, false);

        if (err.isOk()) {
            // ищем файлы по номерам
            const int file_max_count = 50;
            // первое значение - файл, второе - имя без номера
            QVector<QPair<QString, QString>> files_chain(file_max_count);
            for (auto& f : qAsConst(files)) {
                QFileInfo fi(f);
                QString file_name = fi.completeBaseName();
                int pos = file_name.lastIndexOf("_");
                if (pos < 0)
                    continue;

                int num = file_name.rightRef(file_name.length() - pos - 1).toInt() - 1;
                if (num < 0 || num >= file_max_count)
                    continue;

                QString base_name = file_name.left(pos);
                if (base_name.isEmpty())
                    continue;

                files_chain[num] = {f, base_name};
            }

            // сдвигаем
            for (int i = file_max_count - 1; i >= 0; i--) {
                const QPair<QString, QString>& f = files_chain.at(i);
                if (f.first.isEmpty())
                    continue;

                if (i == file_max_count - 1) {
                    Utils::removeFile(f.first);
                    continue;
                }

                QFileInfo fi(f.first);
                QString new_name = fi.absolutePath() + "/" + f.second + "_"
                                   + QString::number(i + 2).rightJustified(QString::number(file_max_count).length(), '0') + ".txt";
                Utils::renameFile(f.first, new_name);
            }

            QFile file(folder + "/" + error.fileDebugModifier() + "_"
                       + QString("1").rightJustified(QString::number(file_max_count).length(), '0') + ".txt");
            if (!file.open(QFile::Truncate | QFile::WriteOnly))
                err = Error::fileIOError(&file);

            if (err.isOk()) {
                QTextStream st(&file);
                st.setCodec("UTF-8");
                st << error.fullText();
                file.close();
            }
        }

        if (err.isError()) {
            logError(err);
            Core::error(Error(error).setFileDebug(false));
        }
        return;
    }

    Uid entity_uid;
    if (error.isDocumentError()) {
        entity_uid = Uid::fromVariant(error.data());
        Z_CHECK(entity_uid.isValid());
    }

    InformationType info_type = error.informationType();
    MessageBox::MessageType message_type;
    if (static_cast<int>(info_type) >= static_cast<int>(InformationType::Error))
        message_type = MessageBox::MessageType::Error;
    else if (static_cast<int>(info_type) >= static_cast<int>(InformationType::Warning))
        message_type = MessageBox::MessageType::Warning;
    else
        message_type = MessageBox::MessageType::Info;

    Dialog* dlg;

    if (error.isNote()) {
        NoteDialog* msgBox = new NoteDialog();
        dlg = msgBox;

        QString text = HtmlTools::table(error);

        if (HtmlTools::isHtml(text))
            msgBox->setHtml(HtmlTools::correct(text));
        else
            msgBox->setPlainText(text);

    } else {
        QString text;
        QString text_detail;

        if (error.childErrors().count() == 0) {
            text = error.text();
            text_detail = error.textDetail();

        } else {
            Error main_error = error.extractMainError();

            if (main_error.isError()) {
                text = main_error.text();
                text_detail = HtmlTools::table(error.extractNotMainError());
            } else {
                Error c_error = error.compressChildErrors();
                if (c_error.childErrors().count() == 0) {
                    Core::error(c_error);
                    return;

                } else {
                    text = HtmlTools::color(QString("<b>%1:</b>").arg(ZF_TR(ZFT_ERROR)), Qt::darkRed);
                    text_detail = HtmlTools::table(error);
                }
            }
        }

        MessageBox* msgBox = new MessageBox(message_type, text, text_detail, entity_uid);
        dlg = msgBox;
    }

    if (error.isModalWindow()) {
        dlg->exec();
        dlg->objectExtensionDestroy();

        if (error.isConnectionClosedError())
            qApp->quit();

    } else {
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->open();
    }
}

void Core::error(const ErrorList& err, bool auto_main)
{
    error(Error::fromList(err, auto_main));
}

void Core::error(const Log& log)
{
    logDialogHelper(log, Utils::resizeSvg(":/share_icons/red/important.svg", 24));
}

void Core::errorIf(const Error& err)
{
    if (err.isError())
        error(err);
}

void Core::errorIf(const ErrorList& err, bool auto_main)
{
    auto e = Error::fromList(err, auto_main);
    if (e.isOk())
        return;
    error(e);
}

void Core::systemError(const QString& text, const QString& detail)
{
    error(text, detail);
}

void Core::systemError(const Error& error, const QString& detail)
{
    Core::error(error.fullText(), detail);
}

bool Core::askFunction(const QString& question, const QString& wait_info, SyncAskDialogFunction sync_function, Error& error,
                       const QString& result_info, bool show_error, bool run_in_thread, const zf::Uid& progress_uid)
{
    return SyncAskDialog::run(question, wait_info, sync_function, error, result_info, show_error, run_in_thread, progress_uid);
}

Error Core::waitFunction(const QString& wait_info, SyncAskDialogFunction sync_function, const QString& result_info, bool show_error,
                         bool run_in_thread, const zf::Uid& progress_uid)
{
    Z_CHECK_NULL(sync_function);

    SyncWaitWindow wait_dlg(wait_info, progress_uid);
    wait_dlg.run();

    QElapsedTimer timer;
    timer.start();

    Error error;

    if (run_in_thread) {
        QEventLoop loop;
        QFutureWatcher<Error> watcher;
        watcher.connect(&watcher, &QFutureWatcher<int>::finished, &watcher, [&loop]() { loop.quit(); });
        QFuture<Error> future = QtConcurrent::run(sync_function);
        watcher.setFuture(future);
        loop.exec();
        error = future.result();

    } else {
        error = sync_function();
    }

    if (timer.elapsed() < SyncWaitWindow::MINIMUM_TIME_MS)
        Utils::delay(SyncWaitWindow::MINIMUM_TIME_MS - timer.elapsed());

    wait_dlg.hide();

    if (error.isError()) {
        if (show_error)
            zf::Core::error(error);

    } else if (!result_info.isEmpty()) {
        zf::Core::info(result_info);
    }

    return error;
}

SyncWaitWindow* Core::createSyncWaitWindow(const QString& text, const zf::Uid& progress_uid)
{
    return SyncWaitWindow::run(text, progress_uid);
}

void Core::postError(const Error& error)
{
    Z_CHECK_NULL(_cumulative_error);
    _cumulative_error->addError(error);
}

void Core::postError(const QString& error)
{
    postError(Error(error));
}

void Core::postInfo(const QString& info)
{
    postError(Error(info).setInformationType(InformationType::Information));
}

void Core::postNote(const QString& info)
{
    postError(Error(info).setInformationType(InformationType::Information).setNote(true));
}

void Core::note(const QString& text, const QString& caption)
{
    ObjectExtensionPtr<NoteDialog> msgBox = new NoteDialog(caption);

    if (HtmlTools::isHtml(text))
        msgBox->setHtml(HtmlTools::correct(text));
    else
        msgBox->setPlainText(text);

    msgBox->exec();
}

bool Core::askParameters(const DatabaseID& database_id, const QString& caption, const QString& text, const DataContainerPtr& data)
{
    Z_CHECK_NULL(data);

    DataWidgets mw(database_id, data);

    ObjectExtensionPtr<MessageBox> msgBox = new MessageBox(MessageBox::MessageType::Ask, text);
    if (!caption.isEmpty())
        msgBox->setWindowTitle(caption);

    msgBox->setBottomLineVisible(false);

    QVBoxLayout* la = new QVBoxLayout;
    la->setObjectName(QStringLiteral("zfla"));
    la->addWidget(mw.content());
    msgBox->insertLayout(la);

    return static_cast<QDialogButtonBox::StandardButton>(msgBox->exec()) == QDialogButtonBox::Yes;
}

bool Core::askText(const QString& name, bool value_required, QString& value, bool hide, const QString& placeholder,
                   const QString& input_mask, const QString& caption)
{
    ObjectExtensionPtr<Dialog> dlg = new Dialog(zf::Dialog::Type::Edit);
    dlg->setWindowTitle(caption);

    auto la = new QHBoxLayout;

    QLabel* label = nullptr;
    if (!name.isEmpty())
        label = new QLabel(name);

    auto editor = new zf::LineEdit;
    editor->setText(value);
    editor->setPlaceholderText(placeholder);
    editor->setInputMask(input_mask);
    if (hide)
        editor->setEchoMode(QLineEdit::Password);

    dlg->workspace()->setLayout(la);
    if (label != nullptr)
        la->addWidget(label);
    la->addWidget(editor);
    la->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    if (value_required) {
        dlg->setOkButtonEnabled(false);
        dlg->connect(editor, &QLineEdit::textEdited, dlg.get(),
                     [dlg, editor]() { dlg->setOkButtonEnabled(!editor->text().trimmed().isEmpty()); });
    }

    if (dlg->exec() != QDialogButtonBox::Ok)
        return false;

    value = editor->text().trimmed();
    return true;
}

void Core::logInfo(const QString& text, const QString& detail, bool plain_text)
{
    log(InformationType::Information, text, detail, plain_text);
}

void Core::logError(const QString& text, const QString& detail, bool plain_text)
{
    Z_CHECK_X(!text.trimmed().isEmpty(), "Попытка выполнить Core::logError без текста ошибки");
    log(InformationType::Error, text.trimmed(), detail.trimmed(), plain_text);
}

void Core::logError(const Error& error, bool plain_text)
{
    logError(error.text(), error.textDetailFull(), plain_text);
}

void Core::logCriticalError(const QString& text, const QString& detail, bool plain_text)
{
    Z_CHECK_X(!text.trimmed().isEmpty(), "Попытка выполнить Core::logCriticalError без текста ошибки");
    log(InformationType::Critical, text.trimmed(), detail.trimmed(), plain_text);
}

void Core::logCriticalError(const Error& error, bool plain_text)
{
    logCriticalError(error.text(), error.textDetailFull(), plain_text);
}

Error Core::debPrint(const QObject* object, QStandardPaths::StandardLocation location, bool open)
{
    return Logging::debugFile(object, location, open);
}

Error Core::debPrint(const QObject* object, const QDir& folder, bool open)
{
    return Logging::debugFile(object, folder, open);
}

Error Core::debPrint(const QObject* object)
{
    if (_default_debprint_location.isEmpty())
        return debPrint(object, QStandardPaths::DownloadLocation, true);
    else
        return debPrint(object, _default_debprint_location, true);
}

Error Core::debPrint(const QVariant& value, const QDir& folder, bool open)
{
    return Logging::debugFile(value, folder, open);
}

Error Core::debPrint(const QVariant& value, QStandardPaths::StandardLocation location, bool open)
{
    return Logging::debugFile(value, location, open);
}

Error Core::debPrint(const QVariant& value)
{
    return debPrint(value, QStandardPaths::DownloadLocation, true);
}

void Core::setDefaultDebPrintLocation(QStandardPaths::StandardLocation location)
{
    _default_debprint_location = QStandardPaths::writableLocation(location);
}

void Core::setDefaultDebPrintLocation(const QDir& folder)
{
    _default_debprint_location = folder.absolutePath();
}

void Core::beginDebugOutput()
{
    Logging::beginDebugOutput();
}

void Core::endDebugOutput()
{
    Logging::endDebugOutput();
}

int Core::debugLevel()
{
    return Logging::debugLevel();
}

QString Core::debugIndent()
{
    return Logging::debugIndent();
}

QNetworkProxy Core::getProxy(const QUrl& url)
{
    QMutexLocker lock(&_proxy_mutex);

    if (!_use_proxy)
        return QNetworkProxy();

    QNetworkProxy p = Utils::proxyForUrl(url.isValid() ? url : _default_url);
    if (p.type() == QNetworkProxy::ProxyType::NoProxy)
        return p;

    p.setUser(_proxy_credentials.login());
    p.setPassword(_proxy_credentials.password());

    return p;
}

QNetworkProxy Core::getProxy(const QString& url)
{
    return getProxy(QUrl::fromUserInput(url));
}

void Core::setProxySettings(bool use, const Credentials& redentials, const QUrl& default_url)
{
    QMutexLocker lock(&_proxy_mutex);
    _use_proxy = use;
    _proxy_credentials = redentials;
    _default_url = default_url;
}

void Core::registerNonParentWidget(QWidget* w)
{
    fr()->registerNonParentWidget(w);
}

Error Core::registerDatabaseDriver(const DatabaseDriverConfig& config, const QString& library_name, const QString& path)
{
    fr();
    return _database_manager->registerDatabaseDriver(config, library_name, path);
}

DatabaseDriverWorker* Core::driverWorker()
{
    fr();
    return _database_manager->worker();
}

void Core::installCatalogsInterface(I_Catalogs* catalogs)
{
    Z_CHECK(_catalogs == nullptr);
    _catalogs = catalogs;
}

I_Catalogs* Core::catalogsInterface()
{
    return _catalogs;
}

QString Core::currentUserLogin()
{
    return _current_user_login;
}

void Core::setCurrentUserLogin(const QString& login)
{
    QString s = login.trimmed();
    if (s.isEmpty() || _current_user_login == s)
        return;
    _current_user_login = s;

    fr()->loadSystemData();
    fr()->localSettings()->apply();
}

void Core::installDataRestrictionInterface(I_DataRestriction* restriction)
{
    _restriction = restriction;
}

DataRestrictionPtr Core::createDataRestriction(const TableID& table_id)
{
    Z_CHECK_NULL(_restriction);
    Z_CHECK(table_id.isValid());
    return DataRestrictionPtr(_restriction->createDataRestriction(table_id));
}

Uid Core::catalogUid(const zf::EntityCode& catalog_id, const DatabaseID& database_id)
{
    Z_CHECK_NULL(_catalogs);
    return _catalogs->catalogUid(catalog_id, database_id);
}

DataStructurePtr Core::catalogStructure(const zf::EntityCode& catalog_id, const DatabaseID& database_id)
{
    Error error;
    auto structure = getModuleDataStructure(catalogUid(catalog_id, database_id), error);
    Z_CHECK_ERROR(error);
    Z_CHECK_NULL(structure);
    return structure;
}

ModelPtr Core::catalogModel(const zf::EntityCode& catalog_id, const DatabaseID& database_id)
{
    Z_CHECK_NULL(_catalogs);
    Error error;
    ModelPtr model = _catalogs->catalogModel(catalog_id, database_id, error);
    Z_CHECK_ERROR(error);
    return model;
}

ModelPtr Core::catalogModelSync(const EntityCode& catalog_id, const DatabaseID& database_id)
{
    auto m = catalogModel(catalog_id, database_id);
    waitForLoadModel(m);
    return m;
}

DataProperty Core::catalogProperty(const zf::EntityCode& catalog_id, const PropertyID& property_id, const DatabaseID& database_id)
{
    return catalogStructure(catalog_id, database_id)->property(property_id);
}

bool Core::isCatalogUid(const Uid& entity_uid)
{
    Z_CHECK_NULL(_catalogs);
    return _catalogs->isCatalogUid(entity_uid);
}

Uid Core::catalogMainEntityUid(const EntityCode& catalog_id, const DatabaseID& database_id)
{
    Z_CHECK_NULL(_catalogs);
    return _catalogs->mainEntityUid(catalog_id, database_id);
}

Uid Core::catalogEntityUid(const EntityCode& entity_code, const DatabaseID& database_id)
{
    Z_CHECK_NULL(_catalogs);
    return _catalogs->catalogEntityUid(entity_code, database_id);
}

zf::EntityCode Core::catalogId(const Uid& catalog_uid)
{
    Z_CHECK_NULL(_catalogs);
    return _catalogs->catalogId(catalog_uid);
}

QVariant Core::catalogValue(const EntityCode& catalog_id, int id, QLocale::Language language, const DatabaseID& database_id)
{
    auto structure = catalogStructure(catalog_id, database_id);

    return catalogValue(catalog_id, EntityID(id), structure->nameColumn(structure->singleDataset()).id(), language, database_id);
}

QVariant Core::catalogValue(const EntityCode& catalog_id, const EntityID& id, const PropertyID& property_id, QLocale::Language language,
                            const DatabaseID& database_id)
{
    ModelPtr data_not_ready;
    return fr()->catalogValue(catalog_id, id, property_id, language, database_id, data_not_ready);
}

QVariant Core::catalogValue(const EntityCode& catalog_id, const EntityID& id, const PropertyID& property_id, ModelPtr& data_not_ready,
                            QLocale::Language language, const DatabaseID& database_id)
{
    return fr()->catalogValue(catalog_id, id, property_id, language, database_id, data_not_ready);
}

QVariant Core::catalogValueSync(const zf::EntityCode& catalog_id, const zf::EntityID& id, const PropertyID& property_id, Error& error,
                                int timeout_ms, QLocale::Language language, const DatabaseID& database_id)
{
    auto result = messageDispatcher()->sendMessage(
        _model_manager.get(), MMCommandGetCatalogValueMessage(catalog_id, id, property_id, language, database_id), timeout_ms);

    if (result.messageType() == MessageType::Error) {
        error = ErrorMessage(result).error();
        return QVariant();
    }

    return MMEventCatalogValueMessage(result).value();
}

QVariant Core::catalogValueSync(const EntityCode& catalog_id, const EntityID& id, const PropertyID& property_id, int timeout_ms,
                                QLocale::Language language, const DatabaseID& database_id)
{
    Error error;
    QVariant res = catalogValueSync(catalog_id, id, property_id, error, timeout_ms, language, database_id);
    if (error.isError())
        logError(error);
    return res;
}

MessageID Core::catalogValueAsync(const I_ObjectExtension* sender, const zf::EntityCode& catalog_id, const EntityID& id,
                                  const PropertyID& property_id, QLocale::Language language, const DatabaseID& database_id)
{
    MMCommandGetCatalogValueMessage message(catalog_id, id, property_id, language, database_id);
    Z_CHECK(messageDispatcher()->postMessage(sender, _model_manager.get(), message));
    return message.messageId();
}

QVariant Core::catalogItemNameSync(const zf::EntityCode& catalog_id, const zf::EntityID& id, Error& error, int timeout_ms,
                                   QLocale::Language language, const DatabaseID& database_id)
{
    Z_CHECK_NULL(catalogsInterface());
    return catalogValueSync(catalog_id, id, catalogsInterface()->namePropertyId(catalogUid(catalog_id, database_id)), error, timeout_ms,
                            language, database_id);
}

MessageID Core::catalogItemNameAsync(const I_ObjectExtension* sender, const zf::EntityCode& catalog_id, const zf::EntityID& id,
                                     QLocale::Language language, const DatabaseID& database_id)
{
    Z_CHECK_NULL(catalogsInterface());
    return catalogValueAsync(sender, catalog_id, id, catalogsInterface()->namePropertyId(catalogUid(catalog_id, database_id)), language,
                            database_id);
}

void Core::setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials)
{
    setCurrentUserLogin(credentials.login());
    fr();
    _database_manager->setDatabaseCredentials(database_id, credentials);
}

Error Core::databaseInitialized()
{
    fr();
    return _database_manager->databaseInitialized();
}

bool Core::isModuleRegistered(const EntityCode& entity_code)
{
    return fr()->isModuleRegistered(entity_code);
}

QString Core::getModuleName(const EntityCode& entity_code)
{
    return fr()->getModuleName(entity_code);
}

ModuleInfo Core::getModuleInfo(const EntityCode& entity_code, Error& error)
{
    return fr()->getModuleInfo(entity_code, error);
}

ModuleInfo Core::getModuleInfo(const EntityCode& entity_code)
{
    Error error;
    ModuleInfo info = getModuleInfo(entity_code, error);
    if (error.isError())
        Z_HALT(error);

    return info;
}

DataStructurePtr Core::getModuleDataStructure(const EntityCode& entity_code, Error& error)
{
    auto plugin = getPlugin(entity_code, error);
    if (error.isError())
        return nullptr;
    return plugin->dataStructure(Uid(), error);
}

DataStructurePtr Core::getModuleDataStructure(const EntityCode& entity_code)
{
    Error error;
    DataStructurePtr res = getModuleDataStructure(entity_code, error);
    if (error.isError())
        Z_HALT(error);

    return res;
}

DataStructurePtr Core::getModuleDataStructure(const Uid& entity_uid, Error& error)
{
    Z_CHECK(entity_uid.isValid());
    auto plugin = getPlugin(entity_uid.entityCode(), error);
    if (error.isError())
        return nullptr;

    return plugin->dataStructure(entity_uid, error);
}

DataStructurePtr Core::getModuleDataStructure(const Uid& entity_uid)
{
    Error error;
    DataStructurePtr res = getModuleDataStructure(entity_uid, error);
    if (error.isError())
        Z_HALT(error);

    return res;
}

void Core::replaceWidget(QWidget* source, QWidget* target, bool copy_properties, const QString& name)
{
    if (name.isEmpty())
        Z_CHECK(!source->objectName().isEmpty());

    WidgetReplacerManual::replaceWidget(source, target, name.isEmpty() ? source->objectName() : name, copy_properties);
}

void Core::replaceWidgets(const DataWidgets* widgets, QWidget* body)
{
    Z_CHECK_NULL(widgets);
    Z_CHECK_NULL(body);
    auto wr = Z_MAKE_SHARED(zf::WidgetReplacerDataStructure, widgets, nullptr);
    wr->replace(body);
}

void Core::installOperationsMenu(const OperationMenu& main_menu)
{
    fr()->installOperationsMenu(main_menu);
}

Qt::ToolButtonStyle Core::toolbarButtonStyle(ToolbarType type)
{
    return fr()->toolbarButtonStyle(type);
}

int Core::toolbarButtonSize(ToolbarType type)
{
    return fr()->toolbarButtonSize(type);
}

const ConnectionInformation* Core::connectionInformation(bool halt_if_null)
{
    auto dm = databaseManager(halt_if_null);
    return dm ? dm->connectionInformation() : nullptr;
}

const LocalSettings* Core::settings()
{
    return fr()->localSettings()->currentSettings();
}

QString Core::changelog()
{
    return _changelog;
}

void Core::setChangelog(const QString &s)
{
    _changelog = s;
}

AccessRights Core::applicationAccessRights()
{
    return connectionInformation()->directPermissions().rights();
}

const Permissions* Core::directPermissions()
{
    return &connectionInformation()->directPermissions();
}

const Permissions* Core::relationPermissions()
{
    return &connectionInformation()->relationPermissions();
}

MessageID Core::requestAccessRights(const I_ObjectExtension* sender, const UidList& entity_uids, AccessRightsList& direct_access_rights,
                                    AccessRightsList& relation_access_rights, const QString& login)
{
    if (databaseManager()->requestCachedAccessRights(entity_uids, direct_access_rights, relation_access_rights, login))
        return MessageID();

    auto msg = DBCommandGetAccessRightsMessage(entity_uids, login);
    messageDispatcher()->postMessage(sender, databaseManager(), msg);
    return msg.messageId();
}

MessageID Core::requestReport(const I_ObjectExtension* sender, const QString& template_id, const DataContainer& data,
                              const QMap<DataProperty, QString>& field_names, bool auto_map, QLocale::Language language,
                              const QMap<DataProperty, QLocale::Language>& field_languages)
{
    auto msg = DBCommandGenerateReportMessage(template_id, data, field_names, auto_map, language, field_languages);
    messageDispatcher()->postMessage(sender, databaseManager(), msg);
    return msg.messageId();
}

EntityCodeList Core::registerModules(const QStringList& libraries, const QList<I_Plugin*>& plugins, Error& error)
{
    QList<QPair<QString, QString>> l;
    for (auto& s : libraries) {
        l << QPair<QString, QString>(s, "");
    }
    return registerModules(l, plugins, error);
}

EntityCodeList Core::registerModules(const QList<QPair<QString, QString>>& libraries, const QList<I_Plugin*>& plugins, Error& error)
{
    return fr()->registerModules(libraries, plugins, error);
}

DatabaseID Core::defaultDatabase()
{
    return _default_database_id;
}

const QHash<DatabaseID, DatabaseInformation>& Core::databasesInformation()
{
    Z_CHECK_NULL(_database_info);
    return *_database_info.get();
}

const DatabaseInformation* Core::getDatabaseInformation(const DatabaseID& database_id)
{
    Z_CHECK_NULL(_database_info);

    auto db = _database_info->find(database_id.isValid() ? database_id : defaultDatabase());
    if (db == _database_info->end())
        return nullptr;

    return &db.value();
}

const QLocale* Core::locale(LocaleType type)
{
    switch (type) {
        case LocaleType::Universal:
            return fr()->c_locale();
        case LocaleType::UserInterface:
            return fr()->localeUi();
        case LocaleType::Workflow:
            return fr()->localeWorkflow();
        default:
            Z_HALT_INT;
            return nullptr;
    }
}

QLocale::Language Core::language(LocaleType type)
{
    switch (type) {
        case LocaleType::Universal:
            return QLocale::C;
        case LocaleType::UserInterface:
            return fr()->languageUi();
        case LocaleType::Workflow:
            return fr()->languageWorkflow();
        default:
            Z_HALT_INT;
            return QLocale::AnyLanguage;
    }
}

const QCollator* Core::collator(LocaleType type, Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode)
{
    switch (type) {
        case LocaleType::Universal:
            return fr()->c_collator(sensitivity, ignore_punctuation, numeric_mode);
        case LocaleType::UserInterface:
            return fr()->collatorUi(sensitivity, ignore_punctuation, numeric_mode);
        case LocaleType::Workflow:
            return fr()->collatorWorkflow(sensitivity, ignore_punctuation, numeric_mode);
        default:
            Z_HALT_INT;
            return nullptr;
    }
}

QLocale::Language Core::defaultLanguage()
{
    return QLocale::English;
}

QList<QLocale::Language> Core::uiLanguages()
{
    return fr()->translation().keys();
}

DatabaseID Core::entityCodeDatabase(const Uid& owner_uid, const EntityCode& entity_code)
{
    return databaseManager()->entityCodeDatabase(owner_uid, entity_code);
}

OperationMenuManager* Core::operationMenuManager(bool halt_if_null)
{
    Z_CHECK(!halt_if_null || fr()->operationMenuManager() != nullptr);
    return fr()->operationMenuManager();
}

ModelKeeper* Core::modelKeeper(bool halt_if_null)
{
    fr();
    Z_CHECK(!halt_if_null || _model_keeper != nullptr);
    return _model_keeper.get();
}

ExternalRequester* Core::externalRequester()
{
    fr();

    if (_external_requester == nullptr)
        _external_requester = new ExternalRequester();

    return _external_requester.get();
}

ProgressObject* Core::progress()
{
    if (_progress == nullptr)
        _progress = new ProgressObject();
    return _progress.get();
}

bool Core::isEntityExists(const Uid& entity_uid, int timeout_ms, Error& error)
{
    auto res = isEntitiesExists({entity_uid}, timeout_ms, error);
    return res.isEmpty() ? false : res.first();
}

QList<bool> Core::isEntitiesExists(const UidList& entity_uids, int timeout_ms, Error& error)
{
    fr();
    return databaseManager()->isEntityExistsSync(entity_uids, timeout_ms, error);
}

void Core::log(InformationType type, const QString& text, const QString& detail, bool toPlainText)
{
    Z_CHECK_X(!text.trimmed().isEmpty(), utf("Попытка выполнить ZCore::log без текста ошибки"));
    QString s;

    QString t = toPlainText ? HtmlTools::plain(text, false) : text;
    QString d = toPlainText ? HtmlTools::plain(detail, false) : detail;

    if (d.isEmpty())
        s = t;
    else
        s = QString("%1 (%2)").arg(t, d);

    if (!s.isEmpty()) {
        switch (type) {
            case zf::InformationType::Information: {
#ifdef Q_OS_LINUX
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
                qDebug().noquote() << s;
#else
                qDebug() << s;
#endif
#else

                qInfo().noquote() << s;
#endif

                // Уровень ошибки qDebug не вызовет автоматической записи в файл в методе
                // ZFramework.cpp::qtErrorHandler, поэтому пишем вручную
#ifdef QT_DEBUG
                auto error =
#endif
                    writeToLogStorage(s, zf::InformationType::Information);
#ifdef QT_DEBUG
                if (error.isError())
                    error = Error(error);
#endif

                break;
            }
            case zf::InformationType::Warning:
            case zf::InformationType::Error:
            case zf::InformationType::Invalid:

                qWarning().noquote() << s;
                break;
            case zf::InformationType::Critical:
            case zf::InformationType::Fatal:

                qCritical().noquote() << s;
                break;
            default: {
                qDebug().noquote() << s;
                // Уровень ошибки qDebug не вызовет автоматической записи в файл в методе
                // ZFramework.cpp::qtErrorHandler, поэтому пишем вручную
                writeToLogStorage(s, zf::InformationType::Information);
                break;
            }
        }
    }
}

Error Core::writeToLogStorage(const QString& text, InformationType type)
{
    if (_framework != nullptr)
        return fr()->writeToLogStorage(text, type);
    return Error();
}

bool Core::isCoreInitialized()
{
    return _framework != nullptr;
}

ModelPtr Core::createModelHelper(const DatabaseID& database_id, const EntityCode& entity_code, Error& error)
{
    return fr()->createModel(database_id, entity_code, error);
}

ModelPtr Core::detachModelHelper(const Uid& entity_uid, const LoadOptions& load_options,
                                 const DataPropertySet& properties, Error& error)
{
    return fr()->detachModel(entity_uid, load_options, properties, {}, error);
}

View* Core::createViewHelper(const ModelPtr& model, const ViewStates& states, const QVariant& data, Error& error)
{
    return fr()->createView(model, states, data, error);
}

void Core::logDialogHelper(const Log& log, const QIcon& icon)
{
    QIcon i;
    if (log.contains(InformationType::Error) || log.contains(InformationType::Fatal)
        || log.contains(InformationType::Critical) || log.contains(InformationType::Required))
        i = QIcon(":/share_icons/red/important.svg");
    else if (log.contains(InformationType::Warning))
        i = QIcon(":/share_icons/warning.svg");
    else
        i = icon;

    ObjectExtensionPtr<LogDialog> dlg = new LogDialog(&log, i, "");
    dlg->exec();
}

ViewPtr Core::createViewSharedPointer(View* view)
{
    Z_CHECK_NULL(view);
    return ViewPtr(view,
                   // нестандартный метод удаления объекта
                   [&](View* object) { object->objectExtensionDestroy(); });
}

Framework* Core::fr()
{
    Z_CHECK_X(_framework != nullptr, utf("Не вызван метод Core::bootstrap"));
    return _framework.get();
}

struct core_initializer
{
    core_initializer() { }
    ~core_initializer()
    {
        if (!Core::mode().testFlag(CoreMode::QtDesigner)) {
            // должен быть вызван метод Core::freeResources
            assert(!Core::isCoreInitialized());
        }
    }
};
static core_initializer initializer;

} // namespace zf
