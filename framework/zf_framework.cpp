#include "zf_framework.h"
#include "zf_core.h"
#include "zf_model_manager.h"
#include "zf_module_manager.h"
#include "zf_data_container.h"
#include "zf_logging.h"
#include "zf_core_messages.h"
#include "zf_item_delegate.h"
#include "zf_dialog_configuration.h"
#include "zf_http_headers.h"
#include "zf_exception.h"
#include "zf_catalog_info.h"
#include "zf_db_srv.h"
#include "zf_image_list.h"
#include "zf_hierarchy_list.h"
#include "zf_shared_ptr_deleter.h"
#include "zf_colors_def.h"
#include "zf_change_info.h"
#include "zf_native_event_filter.h"
#include "zf_ui_size.h"

#include "private/zf_item_selector_p.h"
#include "private/zf_accessible_view_p.h"
#include "private/zf_accessible_menu_p.h"
#include "private/zf_picture_selector_p.h"

#include <QApplication>
#include <QSslError>
#include <QDebug>
#include <QChildEvent>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "item_model/private/zf_flat_item_model_p.h"

#include "services/item_model/item_model_service.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace zf
{
//! Интервал вызова ProcessEvents
const int Framework::PROCESS_EVENTS_TIMEOUT_MS = 100;
//! Сколько моделей за один раз обрабатывает ModelKeeper
const int Framework::MODEL_KEEPER_ONE_STEP = 100;
//!  Сколько сообщений буфера обрабатывает MessageDispatcher за один раз
const int Framework::MESSAGE_DISPATCHER_ONE_STEP = 100;
//! Менеджер обратных вызовов для внутреннего использования
ObjectExtensionPtr<CallbackManager> Framework::_callback_manager;
//! Генератор последовательностей для внутреннего использования
std::unique_ptr<SequenceGenerator<quint64>> Framework::_sequence_generator;
//! Обработчик системных сообщений Qt по умолчанию
QtMessageHandler Framework::_default_message_handler = nullptr;
//! Была инициализация типов Qt
bool Framework::_metatype_initialized = false;

Framework::Framework(ModelManager* model_manager, ModuleManager* module_manager, DatabaseManager* database_manager, bool is_install_error_handlers,
    const QMap<QLocale::Language, QString>& translation)
    : QObject()
    , _model_manager(model_manager)
    , _module_manager(module_manager)
    , _database_manager(database_manager)
    , _translation(translation)
    , _language_ui(QLocale::system().language())
    , _language_workflow(QLocale::system().language())
    , _sptr_deleter(std::make_unique<SharedPtrDeleter>())
    , _object_extension(new ObjectExtension(this))

{
    Z_CHECK_NULL(_model_manager);
    Z_CHECK_NULL(_module_manager);

    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::CORE, this, "sl_message_dispatcher_inbound"));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::GENERAL));
    Z_CHECK(Core::messageDispatcher()->registerChannel(CoreChannels::INTERNAL));
    _subscribe_handle_connection_info = Core::messageDispatcher()->subscribe(CoreChannels::CONNECTION_INFORMATION, this);

    if (is_install_error_handlers)
        bootstrapErrorHandlers();

    bootstrapQtMetadata();
    bootstrapAccessibility();

    qApp->installEventFilter(this);

    loadSystemData();

    Translator::installTranslation(translation, _language_ui);

    Colors::bootstrap();

    qApp->installNativeEventFilter(new NativeEventFilter());
}

Framework::~Framework()
{
    initWindowManager(nullptr);

    _sptr_deleter.reset();

    _is_active = false;

    qInstallMessageHandler(_default_message_handler);

    spdlog::shutdown();

    if (_default_locale_ui != nullptr) {
        delete _default_locale_ui;
        _default_locale_ui = nullptr;
    }

    if (_default_locale_workflow != nullptr) {
        delete _default_locale_workflow;
        _default_locale_workflow = nullptr;
    }

    delete _object_extension;
}

bool Framework::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void Framework::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
    if (_callback_manager)
        _callback_manager->stop();
}

QVariant Framework::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool Framework::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void Framework::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void Framework::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void Framework::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void Framework::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void Framework::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

Error Framework::saveSystemData()
{
    Error error;
    if (!Core::mode().testFlag(CoreMode::Library)) {
        QFile dlg_config_file(dialogConfigFile());
        if (dlg_config_file.open(QFile::WriteOnly | QFile::Truncate)) {
            error = dialogConfiguration()->saveToDevice(&dlg_config_file);
            dlg_config_file.close();
        } else
            error = Error::fileIOError(dialogConfigFile());
    }

    return error;
}

CallbackManager* Framework::internalCallbackManager()
{
    if (_callback_manager == nullptr) {
        _callback_manager = new CallbackManager();
        _callback_manager->setObjectName("Framework");
    }
    return _callback_manager.get();
}

SequenceGenerator<quint64>* Framework::internalSequenceGenerator()
{
    if (_sequence_generator == nullptr)
        _sequence_generator = std::make_unique<SequenceGenerator<quint64>>();
    return _sequence_generator.get();
}

void Framework::installOperationsMenu(const OperationMenu& main_menu)
{
    Z_CHECK(_operation_menu_manager == nullptr);
    _operation_menu_manager = new OperationMenuManager("main_toolbar", main_menu, OperationScope::Module, ToolbarType::Main);
    connect(_operation_menu_manager.get(), &OperationMenuManager::sg_operationActivated, this, &Framework::sl_operationActionActivated);
}

bool Framework::isOperationMenuInstalled() const
{
    return _operation_menu_manager != nullptr;
}

OperationMenuManager* Framework::operationMenuManager() const
{
    Z_CHECK_NULL(_operation_menu_manager.get());
    return _operation_menu_manager.get();
}

ModelManager* Framework::modelManager() const
{
    return _model_manager;
}

SharedPtrDeleter* Framework::sharedPtrDeleter() const
{
    return _sptr_deleter.get();
}

void Framework::setToolbarStyle(ToolbarType type, Qt::ToolButtonStyle style, int size)
{
    if (type == ToolbarType::Main) {
        _main_toolbar_style = style;
        _main_toolbar_size = size;
    } else if (type == ToolbarType::Window) {
        _module_toolbar_style = style;
        _module_toolbar_size = size;
    } else
        Z_HALT_INT;

    if (_operation_menu_manager != nullptr && _operation_menu_manager->toolbar() != nullptr && type == ToolbarType::Main) {
        _operation_menu_manager->toolbar()->setToolButtonStyle(_main_toolbar_style);
        _operation_menu_manager->toolbar()->setIconSize(QSize(_main_toolbar_size, _main_toolbar_size));
    }
}

Qt::ToolButtonStyle Framework::toolbarButtonStyle(ToolbarType type) const
{
    switch (type) {
        case ToolbarType::Main:
            return _main_toolbar_style;
        case ToolbarType::Window:
            return _module_toolbar_style;
        case ToolbarType::Internal:
            return Qt::ToolButtonStyle::ToolButtonIconOnly;
    }

    Z_HALT_INT;
    return Qt::ToolButtonStyle::ToolButtonIconOnly;
}

int Framework::toolbarButtonSize(ToolbarType type) const
{
    switch (type) {
        case ToolbarType::Main:
            return _main_toolbar_size;
        case ToolbarType::Window:
            return _module_toolbar_size;
        case ToolbarType::Internal:
            return UiSizes::defaultToolbarIconSize(type);
    }

    Z_HALT_INT;
    return 0;
}

const QMap<QLocale::Language, QString>& Framework::translation() const
{
    return _translation;
}

QLocale::Language Framework::languageUi() const
{
    return _language_ui;
}

QLocale::Language Framework::languageWorkflow() const
{
    return _language_workflow;
}

void Framework::setLanguage(QLocale::Language language_ui, QLocale::Language language_workflow)
{
    _language_ui = language_ui;
    Translator::installTranslation(_translation, language_ui);
    QLocale::setDefault(QLocale(language_ui));

    _language_workflow = language_workflow;
}

const QLocale* Framework::localeUi() const
{
    QMutexLocker lock(&locale_mutex);

    if (_default_locale_ui == nullptr) {
        if (Core::language(LocaleType::UserInterface) != QLocale::AnyLanguage)
            _default_locale_ui = new QLocale(Core::language(LocaleType::UserInterface));
        else
            _default_locale_ui = new QLocale();
        _default_locale_ui->setNumberOptions(QLocale::OmitGroupSeparator | _default_locale_ui->numberOptions());
    }
    return _default_locale_ui;
}

const QLocale* Framework::localeWorkflow() const
{
    QMutexLocker lock(&locale_mutex);

    if (_default_locale_workflow == nullptr)
        _default_locale_workflow = new QLocale(Core::language(LocaleType::Workflow));

    return _default_locale_workflow;
}

const QLocale* Framework::c_locale() const
{
    QMutexLocker lock(&locale_mutex);

    if (_default_locale_c == nullptr)
        _default_locale_c = new QLocale(QLocale::C);

    return _default_locale_c;
}

int Framework::collatorKey(bool case_sensitive, bool ignore_punctuation, bool numeric_mode)
{
    return qMin((int)case_sensitive, 1) | qMin((int)ignore_punctuation, 1) << 1 | qMin((int)numeric_mode, 1) << 2;
}

const QCollator* Framework::collatorUi(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const
{
    return collator(languageUi(), sensitivity, ignore_punctuation, numeric_mode);
}

const QCollator* Framework::collatorWorkflow(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const
{
    return collator(languageWorkflow(), sensitivity, ignore_punctuation, numeric_mode);
}

const QCollator* Framework::c_collator(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const
{
    return collator(QLocale::C, sensitivity, ignore_punctuation, numeric_mode);
}

const QCollator* Framework::collator(QLocale::Language language, Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const
{
    QMutexLocker lock(&locale_mutex);

    int key = collatorKey(sensitivity == Qt::CaseSensitive, ignore_punctuation, numeric_mode);

    if (_collator_by_lang == nullptr)
        _collator_by_lang = new QVector<QVector<QCollator*>*>(QLocale::LastLanguage + 1);

    auto c = _collator_by_lang->at(language);
    if (c == nullptr) {
        c = new QVector<QCollator*>(collatorKey(true, true, true) + 1);
        _collator_by_lang->operator[](language) = c;
    }

    auto collator = c->at(key);
    if (collator == nullptr) {
        collator = new QCollator(QLocale(language));
        c->operator[](key) = collator;
    }

    return collator;
}

void Framework::initWindowManager(MdiArea* mdi_area)
{
    if (mdi_area != nullptr)
        _window_manager = new WindowManager(mdi_area);
    else
        _window_manager.reset();
}

WindowManager* Framework::windowManager() const
{
    return _window_manager.get();
}

bool Framework::isActive() const
{
    QMutexLocker lock(&_is_active_mutex);
    return windowManager() != nullptr && _is_active;
}

void Framework::setActive(bool b)
{
    QMutexLocker lock(&_is_active_mutex);
    _is_active = b;
}

bool Framework::getEntityValue(const Uid& source_uid, const QVariant& key_value, QVariant& result, MessageID& feedback_id, ModelPtr& source_model, Error& error,
    const DataProperty& dataset_property, const DataProperty& key_column, const DataProperty& result_column, const I_ObjectExtension* requester,
    const QWidgetList& update_on_data_loaded, int result_role) const
{
    Z_CHECK(source_uid.isValid());
    source_model.reset();

    DataProperty m_dataset_property;
    if (dataset_property.isValid()) {
        Z_CHECK(source_uid.entityCode() == dataset_property.entityCode());
        m_dataset_property = dataset_property;

    } else {
        m_dataset_property = DataStructure::structure(source_uid)->singleDataset();
    }

    DataProperty m_key_column;
    if (key_column.isValid()) {
        m_key_column = key_column;
        Z_CHECK(m_dataset_property == key_column.dataset());

    } else {
        auto props = DataProperty::datasetColumnsByOptions(source_uid, m_dataset_property.id(), PropertyOption::Id);
        Z_CHECK(!props.isEmpty());
        m_key_column = props.first();
    }

    DataProperty m_result_column;
    if (result_column.isValid()) {
        m_result_column = result_column;

    } else {
        auto props = DataProperty::datasetColumnsByOptions(source_uid, m_dataset_property.id(), PropertyOption::FullName);
        if (props.isEmpty())
            props = DataProperty::datasetColumnsByOptions(source_uid, m_dataset_property.id(), PropertyOption::Name);
        Z_CHECK(!props.isEmpty());
        m_result_column = props.first();
    }

    Z_CHECK(m_dataset_property == m_result_column.dataset());

    if (requester != nullptr) {
        source_model = Core::getModel<Model>(source_uid, {}, {m_dataset_property}, error, requester, feedback_id);
        if (error.isError())
            feedback_id.clear();
    } else {
        feedback_id.clear();
        source_model = Core::getModel<Model>(source_uid, {}, {m_dataset_property}, error);
    }

    if (error.isError()) {
        result.clear();
        return false;
    }

    if (source_model->isLoading()) {
        source_model->requestWidgetUpdate(update_on_data_loaded);
        result.clear();
        return false;
    }

    QModelIndexList rows = source_model->data()->hash()->findRows(m_key_column, key_value);
    if (!rows.isEmpty())
        result = source_model->data()->cell(rows.first().row(), m_result_column, result_role, rows.first().parent());
    else
        result.clear();

    return true;
}

Error Framework::writeToLogStorage(const QString& text, InformationType type)
{
    QMutexLocker lock(&_logger_mutex);

    QString log_file = Utils::logLocation() + QStringLiteral("/log.txt");

    if (_logger == nullptr || _log_file != log_file) {
        _log_file = log_file;
        try {
            static std::string log_file_str;
            log_file_str = log_file.toLocal8Bit().toStdString();
            _logger = spdlog::rotating_logger_mt(log_file_str, log_file_str, 1048576 * 5, 3);
        } catch (const spdlog::spdlog_ex& ex) {
            return Error(ex.what());
        }

        spdlog::set_default_logger(_logger);
        spdlog::set_pattern("[%Y/%m/%d][%T:%e][%L]  %v");
        spdlog::flush_on(spdlog::level::info);
    }

    static std::string log_str;
    log_str = text.toStdString();
    try {
        switch (type) {
            case zf::InformationType::Information:
                spdlog::info(log_str);
                break;
            case zf::InformationType::Warning:
                spdlog::warn(log_str);
                break;
            case zf::InformationType::Error:
            case zf::InformationType::Invalid:
                spdlog::error(log_str);
                break;
            case zf::InformationType::Critical:
            case zf::InformationType::Fatal:
                spdlog::critical(log_str);
                break;
            default:
                spdlog::info(log_str);
                break;
        }
    } catch (const spdlog::spdlog_ex& ex) {
        return Error(ex.what());
    }

    return Error();
}

static QString conditionFilterFileName(const QString& code, const DataProperty& dataset)
{
    return Utils::dataLocation() + QStringLiteral("/filters/") + dataset.entityCode().string() + QStringLiteral("_") + code.toLower() + QStringLiteral("_")
           + dataset.id().string();
}

Error Framework::getConditionFilter(const QString& code, const DataProperty& dataset, ComplexCondition* c) const
{
    Z_CHECK_NULL(c);
    c->clear();

    QString file_name = conditionFilterFileName(code, dataset);
    if (!QFile::exists(file_name))
        return Error();

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(file_name);

    ComplexCondition cond;
    QDataStream st(&f);
    st.setVersion(Consts::DATASTREAM_VERSION);

    st >> cond;
    Error error;
    if (st.status() != QDataStream::Ok)
        error = Error::fileIOError(file_name);
    else
        c->copyFrom(&cond, false);

    f.close();
    return error;
}

Error Framework::saveConditionFilter(const QString& code, const DataProperty& dataset, const ComplexCondition* c)
{
    Z_CHECK_NULL(c);

    QString file_name = conditionFilterFileName(code, dataset);
    QFile f(file_name);

    Error error = Utils::makeDir(QFileInfo(file_name).absolutePath());
    if (error.isError())
        return error;

    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(file_name);

    QDataStream st(&f);
    st.setVersion(Consts::DATASTREAM_VERSION);
    st << *c;

    if (st.status() != QDataStream::Ok)
        error = Error::fileIOError(file_name);
    f.close();

    if (error.isError())
        Utils::removeFile(file_name);

    return error;
}

Configuration* Framework::configuration(int version) const
{
    if (_configuration == nullptr)
        _configuration = std::make_unique<Configuration>(Consts::CORE_DEV_CODE, version);

    return _configuration.get();
}

Configuration* Framework::entityConfiguration(const EntityCode& entity_code, const QString& key, int version) const
{
    Z_CHECK(entity_code.isValid());

    QString conf_code = entity_code.string() + Consts::KEY_SEPARATOR + key;
    auto conf = _entity_configuration.value(conf_code);
    if (conf == nullptr) {
        conf = Z_MAKE_SHARED(Configuration, entity_code, key, version);
        _entity_configuration[conf_code] = conf;
    }
    return conf.get();
}

Configuration* Framework::coreConfiguration(int version) const
{
    if (_core_configuration == nullptr)
        _core_configuration = std::make_unique<Configuration>(Core::coreInstanceKey(), version);

    return _core_configuration.get();
}

LocalSettingsManager* Framework::localSettings() const
{
    if (_local_settings == nullptr)
        _local_settings = std::make_unique<LocalSettingsManager>();

    return _local_settings.get();
}

DialogConfigurationRepository* Framework::dialogConfiguration() const
{
    return _dialog_configuration.get();
}

QString Framework::dialogConfigFile() const
{
    return Utils::dataLocation() + QStringLiteral("/dialog_config");
}

Error Framework::getModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, QList<ModelPtr>& models, MessageID& feedback_message_id)
{
    return _model_manager->getModels(requester, entity_uid_list, load_options_list, properties_list, all_if_empty_list, models, feedback_message_id);
}

bool Framework::loadModels(const I_ObjectExtension* requester, const UidList& entity_uid_list, const QList<LoadOptions>& load_options_list,
    const QList<DataPropertySet>& properties_list, const QList<bool>& all_if_empty_list, MessageID& feedback_message_id)
{
    return _model_manager->loadModels(requester, entity_uid_list, load_options_list, properties_list, all_if_empty_list, feedback_message_id);
}

ModelPtr Framework::openModelSync(
    const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties, bool all_if_empty, int timeout_ms, Error& error)
{
    return _model_manager->getModelSync(entity_uid, load_options, properties, all_if_empty, timeout_ms, error);
}

ModelPtr Framework::createModel(const DatabaseID& database_id, const EntityCode& entity_code, Error& error)
{
    return _model_manager->createModel(database_id, entity_code, error);
}

ModelPtr Framework::detachModel(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties, bool all_if_empty, Error& error)
{
    return _model_manager->detachModel(entity_uid, load_options, properties, all_if_empty, error);
}

View* Framework::createView(const ModelPtr& model, const ViewStates& states, const QVariant& data, Error& error)
{
    Z_CHECK_NULL(model);

    error.clear();

#ifndef QT_DEBUG
    Utils::blockFatalHalt();
    try {
#endif
        I_Plugin* plugin = Core::getPlugin(model->entityUid().entityCode(), error);
        if (error.isError())
            return nullptr;

        View* view = plugin->createView(model, states, data);
        if (view != nullptr) {
            view->afterCreatedHelper();
        }
#ifndef QT_DEBUG
        Utils::unBlockFatalHalt();
#endif
        return view;
#ifndef QT_DEBUG
    } catch (const FatalException& e) {
        error = e.error().textDetail().isEmpty() ? e.error() : Error(e.error().textDetail());

    } catch (const std::exception& e) {
        error = Error(utf(e.what()));

    } catch (...) {
        error = Error(ZF_TR(ZFT_UNKNOWN_ERROR));
    }

    Utils::unBlockFatalHalt();
    return nullptr;
#endif
}

bool Framework::removeModelsAsync(const I_ObjectExtension* requester, const UidList& entity_uids, const QList<DataPropertySet>& properties,
    const QList<QMap<QString, QVariant>>& parameters, const QList<bool>& all_if_empty_list, const QList<bool>& by_user, MessageID& feedback_message_id)
{
    return _model_manager->removeModelsAsync(requester, entity_uids, properties, parameters, all_if_empty_list, by_user, feedback_message_id);
}

Error Framework::removeModelSync(const Uid& entity_uid, const DataPropertySet& properties, const QMap<QString, QVariant>& parameters, bool all_if_empty,
    const QVariant& by_user, int timeout_ms)
{
    return _model_manager->removeModelsSync(
        {entity_uid}, {properties}, {parameters}, {all_if_empty}, by_user.isValid() ? QList<bool> {by_user.toBool()} : QList<bool> {}, timeout_ms);
}

EntityCodeList Framework::registerModules(const QList<QPair<QString, QString>>& libraries, const QList<I_Plugin*>& plugins, Error& error)
{
    Z_CHECK(!_is_modules_registered);
    _is_modules_registered = true;

    EntityCodeList codes;
    error.clear();
    for (auto& l : libraries) {
        Error err;
        codes << _module_manager->registerModule(l.first, err, l.second);
        if (err.isError())
            error << err;
    }

    auto plugins_to_load = createCorePlugins();
    plugins_to_load << plugins;
    for (auto& p : plugins_to_load) {
        Error err;
        codes << _module_manager->registerModule(p, err);
        if (err.isError())
            error << err;
    }

    if (error.isOk()) {
        for (auto& code : qAsConst(codes)) {
            auto plugin = getPlugin(code, error);
            Z_CHECK_ERROR(error);
            plugin->afterLoadModules();
        }
        auto err = _module_manager->loadModulesConfiguration();
        if (err.isError())
            Core::logError(err);
    }

    return error.isError() ? EntityCodeList() : codes;
}

QVariant Framework::catalogValue(const EntityCode& catalog_id, const EntityID& id, const PropertyID& property_id, QLocale::Language language,
    const DatabaseID& database_id, ModelPtr& data_not_ready)
{
    auto model = Core::catalogModel(catalog_id, database_id);
    if (model->isLoading()) {
        data_not_ready = model;
        return QVariant();

    } else {
        data_not_ready.reset();
    }

    auto rows = Core::catalogsInterface()->findById(catalog_id, id, database_id);
    Z_CHECK(rows.count() <= 1);
    if (rows.isEmpty())
        return QVariant();

    return model->cell(rows.first().row(), property_id, Qt::DisplayRole, rows.first().parent(), language);
}

QString Framework::catalogItemName(const Uid& catalog_uid, const EntityID& item_id, QLocale::Language language)
{
    return Core::catalogsInterface()->catalogItemName(catalog_uid, item_id, language);
}

bool Framework::isModuleRegistered(const EntityCode& code) const
{
    return _module_manager->isModuleRegistered(code);
}

QString Framework::getModuleName(const EntityCode& code) const
{
    return _module_manager->getModuleName(code);
}

ModuleInfo Framework::getModuleInfo(const EntityCode& code, Error& error) const
{
    return _module_manager->getModuleInfo(code, error);
}

EntityCodeList Framework::getAllModules() const
{
    return _module_manager->getAllModules();
}

I_Plugin* Framework::getPlugin(const EntityCode& code, Error& error) const
{
    return _module_manager->getPlugin(code, error);
}

const QMap<EntityCode, QMap<QString, QVariant>>& Framework::pluginsOptions() const
{
    return _module_manager->pluginsOptions();
}

QList<ModelPtr> Framework::openedModels() const
{
    return _model_manager->openedModels();
}

QJSEngine* Framework::jsEngine() const
{
    if (_jsEngine == nullptr) {
        // Инициализируем движок JS
        _jsEngine = std::make_unique<QJSEngine>();
        _jsEngine->setObjectName("zf_framework_js_engine");
        _jsEngine->installExtensions(QJSEngine::ConsoleExtension);
        // инициализация глобальных функций
        const_cast<Framework*>(this)->initJsGlobalFunctions();
    }

    return _jsEngine.get();
}

void Framework::registerNonParentWidget(QWidget* w)
{
    Z_CHECK_NULL(w);
    if (_non_parent_widgets.contains(w))
        return;

    _non_parent_widgets << w;
    connect(w, &QWidget::destroyed, this, &Framework::sl_non_parent_destroyed);
}

bool Framework::isNonParentWidget(QWidget* w) const
{
    return _non_parent_widgets.contains(w);
}

I_WorkZones* Framework::workZones() const
{
    return _work_zones;
}

void Framework::installWorkZonesInterface(I_WorkZones* wz)
{
    if (_work_zones != nullptr) {
        QObject* obj = dynamic_cast<QObject*>(_work_zones);
        Z_CHECK_NULL(obj);
        disconnect(obj, SIGNAL(sg_workZoneInserted(int)), this, SIGNAL(sg_workZoneInserted(int)));
        disconnect(obj, SIGNAL(sg_workZoneRemoved(int)), this, SIGNAL(sg_workZoneRemoved(int)));
        disconnect(obj, SIGNAL(sg_currentWorkZoneChanged(int, int)), this, SIGNAL(sg_currentWorkZoneChanged(int, int)));
        disconnect(obj, SIGNAL(sg_workZoneEnabledChanged(int, bool)), this, SIGNAL(sg_workZoneEnabledChanged(int, bool)));
    }

    _work_zones = wz;

    if (_work_zones != nullptr) {
        QObject* obj = dynamic_cast<QObject*>(_work_zones);
        Z_CHECK_NULL(obj);
        connect(obj, SIGNAL(sg_workZoneInserted(int)), this, SIGNAL(sg_workZoneInserted(int)));
        connect(obj, SIGNAL(sg_workZoneRemoved(int)), this, SIGNAL(sg_workZoneRemoved(int)));
        connect(obj, SIGNAL(sg_currentWorkZoneChanged(int, int)), this, SIGNAL(sg_currentWorkZoneChanged(int, int)));
        connect(obj, SIGNAL(sg_workZoneEnabledChanged(int, bool)), this, SIGNAL(sg_workZoneEnabledChanged(int, bool)));
    }
}

QtMessageHandler Framework::defaultMessageHandler()
{
    return _default_message_handler;
}

bool Framework::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::ChildAdded) {
        // Автоматическое назначение accessibleName в objectName для виджетов
        // Для меню используется код в zf_accessible_menu_p
        QChildEvent* e = static_cast<QChildEvent*>(event);
        if (e->child()->isWidgetType()) {
            if (!e->child()->objectName().isEmpty())
                static_cast<QWidget*>(e->child())->setAccessibleName(e->child()->objectName());

            connect(e->child(), &QObject::objectNameChanged, this, &Framework::sl_accessibleNameChanged, Qt::UniqueConnection);
        }
    }

    return QObject::eventFilter(watched, event);
}

void Framework::sl_operationActionActivated(const Operation& operation)
{
    I_Plugin* plugin = Core::getPlugin(operation.entityCode());

    if (operation.options() & OperationOption::Confirmation) {
        // требуется подтверждение
        if (!plugin->confirmOperation(operation))
            return;
    }

    Error error = plugin->processOperation(operation);
    emit sg_operationProcessed(operation, error);
}

void Framework::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (message.messageType() == MessageType::General && message.messageCode() == MessageCriticalError::MSG_CODE_CRITICAL_ERROR) {
        MessageCriticalError msg(message);
        Utils::fatalErrorHelper(msg.error().fullText());
    }

    if (subscribe_handle == _subscribe_handle_connection_info) {
        DBEventConnectionInformationMessage m(message);
        emit sg_connectionInformationChanged(m.information(), m.error());
    }
}

void Framework::sl_non_parent_destroyed(QObject* obj)
{
    _non_parent_widgets.remove(obj);
}

void Framework::sl_accessibleNameChanged()
{
    if (!sender()->objectName().isEmpty())
        qobject_cast<QWidget*>(sender())->setAccessibleName(sender()->objectName());
}

void Framework::bootstrapErrorHandlers()
{
    Z_CHECK(Utils::isMainThread());
    if (_default_message_handler != nullptr)
        return;

    _default_message_handler = qInstallMessageHandler(errorHandler);

#ifdef Q_CC_MSVC
    _invalid_parameter_handler invalid_handler;
    invalid_handler = invalidParameterHandler;
    _set_invalid_parameter_handler(invalid_handler);
    _set_thread_local_invalid_parameter_handler(invalid_handler);

    ULONG stack_size = 4096 * 16;
    SetThreadStackGuarantee(&stack_size);
#endif
}

void Framework::bootstrapQtMetadata()
{
    Z_CHECK(Utils::isMainThread());

    if (_metatype_initialized)
        return;

    _metatype_initialized = true;

    qRegisterMetaType<const QObject*>();
    qRegisterMetaType<QList<const QObject*>>();
    qRegisterMetaTypeStreamOperators<QList<int>>();
    qRegisterMetaTypeStreamOperators<QList<double>>();
    qRegisterMetaTypeStreamOperators<QStringList>();

    qRegisterMetaType<zf::LoadOptions>();

    InvalidValue::_user_type = qRegisterMetaType<InvalidValue>();
    QMetaType::registerDebugStreamOperator<InvalidValue>();
    qRegisterMetaTypeStreamOperators<InvalidValue>();
    QMetaType::registerConverter<InvalidValue, QString>([](const InvalidValue& n) -> QString {
        if (!n.isValid())
            return QStringLiteral("invaid");
        return QStringLiteral("invaid: %1").arg(Utils::variantToString(n.value()));
    });

    Uid::_metaTypeNumber_uid = qRegisterMetaType<Uid>();
    qRegisterMetaTypeStreamOperators<Uid>();
    QMetaType::registerConverter<Uid, QString>([](const Uid& n) -> QString { return n.isValid() ? n.serializeToString() : QString(); });

    QMetaType::registerDebugStreamOperator<Uid>();
    QMetaType::registerDebugStreamOperator<UidList>();
    QMetaType::registerDebugStreamOperator<UidSet>();

    Uid::_metaTypeNumber_uidList = qRegisterMetaType<UidList>();
    qRegisterMetaTypeStreamOperators<UidList>();

    qRegisterMetaType<ObjectHierarchy>();
    QMetaType::registerDebugStreamOperator<ObjectHierarchy>();

    Error::_metatype_number = qRegisterMetaType<Error>();
    qRegisterMetaTypeStreamOperators<Error>();
    QMetaType::registerDebugStreamOperator<Error>();
    QMetaType::registerDebugStreamOperator<ErrorList>();
    QMetaType::registerConverter<Error, QString>([](const Error& n) -> QString {
        if (n.isOk())
            return QStringLiteral("OK");
        return n.fullText();
    });

    qRegisterMetaType<Version>();
    qRegisterMetaTypeStreamOperators<Version>();
    QMetaType::registerDebugStreamOperator<Version>();

    qRegisterMetaType<Log>();

    qRegisterMetaType<Message>();
    qRegisterMetaTypeStreamOperators<Message>();
    QMetaType::registerDebugStreamOperator<Message>();

    qRegisterMetaType<QList<Message>>();
    qRegisterMetaTypeStreamOperators<QList<Message>>();

    qRegisterMetaType<MessageChannel>();

    DataProperty::_metatype = qRegisterMetaType<DataProperty>();
    qRegisterMetaTypeStreamOperators<DataProperty>();
    qRegisterMetaType<DataPropertyPtr>();
    qRegisterMetaTypeStreamOperators<DataPropertyPtr>();
    QMetaType::registerDebugStreamOperator<DataProperty>();

    qRegisterMetaType<DataPropertySet>();
    qRegisterMetaTypeStreamOperators<DataPropertySet>();
    qRegisterMetaTypeStreamOperators<QList<DataPropertySet>>();

    qRegisterMetaType<DataPropertyList>();
    qRegisterMetaTypeStreamOperators<DataPropertyList>();

    qRegisterMetaType<PropertyTable>();
    qRegisterMetaTypeStreamOperators<PropertyTable>();

    qRegisterMetaType<IntTable>();
    qRegisterMetaTypeStreamOperators<IntTable>();

    qRegisterMetaType<DataStructure>();
    qRegisterMetaTypeStreamOperators<DataStructure>();
    qRegisterMetaType<DataStructurePtr>();
    qRegisterMetaTypeStreamOperators<DataStructurePtr>();
    QMetaType::registerDebugStreamOperator<DataStructure>();
    QMetaType::registerDebugStreamOperator<DataStructure*>();
    QMetaType::registerDebugStreamOperator<DataStructurePtr>();

    qRegisterMetaType<ProgramFeature>();
    qRegisterMetaTypeStreamOperators<ProgramFeature>();

    qRegisterMetaType<ProgramFeatures>();
    qRegisterMetaTypeStreamOperators<ProgramFeatures>();

    qRegisterMetaType<ConnectionInformation>();
    qRegisterMetaTypeStreamOperators<ConnectionInformation>();

    qRegisterMetaType<DatabaseInformation>();
    qRegisterMetaTypeStreamOperators<DatabaseInformation>();

    qRegisterMetaType<DataContainer>();
    qRegisterMetaType<DataContainerPtr>();
    qRegisterMetaType<DataContainerList>();
    qRegisterMetaTypeStreamOperators<DataContainerList>();
    qRegisterMetaType<DataContainerPtrList>();
    QMetaType::registerDebugStreamOperator<DataContainer*>();
    QMetaType::registerDebugStreamOperator<DataContainerPtr>();
    QMetaType::registerDebugStreamOperator<DataContainer>();

    qRegisterMetaType<ModuleType>();
    qRegisterMetaType<RoundOption>();
    qRegisterMetaType<ValueFormat>();
    qRegisterMetaType<LookupType>();
    qRegisterMetaType<EntityNameMode>();
    qRegisterMetaType<MathOperaton>();
    qRegisterMetaType<InformationType>();
    qRegisterMetaType<ErrorType>();
    qRegisterMetaType<SharedObjectEventType>();
    qRegisterMetaType<MessageType>();
    qRegisterMetaType<PropertyOption>();

    qRegisterMetaType<Operation>();
    QMetaType::registerDebugStreamOperator<Operation>();
    qRegisterMetaType<OperationMenuNode>();
    qRegisterMetaType<OperationMenu>();

    qRegisterMetaType<ModelPtr>();
    qRegisterMetaType<ViewPtr>();

    qRegisterMetaType<HighlightItem>();

    qRegisterMetaType<QAbstractItemDelegate::EndEditHint>();

    qRegisterMetaType<RowID>();
    qRegisterMetaTypeStreamOperators<RowID>();
    QMetaType::registerDebugStreamOperator<RowID>();

    qRegisterMetaType<AccessFlag>();

    qRegisterMetaType<AccessRight>();
    qRegisterMetaTypeStreamOperators<QList<Message>>();

    qRegisterMetaType<AccessRights>();
    qRegisterMetaTypeStreamOperators<AccessRights>();

    qRegisterMetaType<AccessRightsList>();
    qRegisterMetaTypeStreamOperators<AccessRightsList>();

    qRegisterMetaType<UidAccessRights>();
    qRegisterMetaTypeStreamOperators<UidAccessRights>();

    qRegisterMetaType<UserInformation>();
    qRegisterMetaTypeStreamOperators<UserInformation>();

    qRegisterMetaType<http::HttpRequestHeader>();
    qRegisterMetaType<http::HttpResponseHeader>();

    qRegisterMetaType<QList<QSslError>>();

    qRegisterMetaType<FileType>();
    qRegisterMetaType<CommunicationSide>();

    qRegisterMetaType<QByteArrayPtr>();
    qRegisterMetaType<QVariantPtr>();

    qRegisterMetaType<Credentials>();
    qRegisterMetaTypeStreamOperators<Credentials>();

    qRegisterMetaType<LanguageMap>();
    qRegisterMetaTypeStreamOperators<LanguageMap>();

    Numeric::_meta_type = qRegisterMetaType<Numeric>();
    qRegisterMetaTypeStreamOperators<Numeric>();
    QMetaType::registerConverter<Numeric, QString>([](const Numeric& n) -> QString { return n.toString(Core::locale(LocaleType::UserInterface)); });
    QMetaType::registerConverter<Numeric, double>([](const Numeric& n) -> double { return n.toDouble(); });
    QMetaType::registerConverter<Numeric, int>([](const Numeric& n) -> int { return n.integer(); });
    QMetaType::registerComparators<Numeric>();
    QMetaType::registerDebugStreamOperator<Numeric>();

    qRegisterMetaType<ModuleInfo>();
    QMetaType::registerDebugStreamOperator<ModuleInfo>();

    qRegisterMetaType<MessageID>();
    qRegisterMetaTypeStreamOperators<MessageID>();
    QMetaType::registerDebugStreamOperator<MessageID>();

    qRegisterMetaType<CatalogInfo>();
    qRegisterMetaTypeStreamOperators<CatalogInfo>();

    qRegisterMetaType<DatabaseID>();
    qRegisterMetaTypeStreamOperators<DatabaseID>();
    QMetaType::registerDebugStreamOperator<DatabaseID>();

    qRegisterMetaType<EntityCode>();
    qRegisterMetaTypeStreamOperators<EntityCode>();
    QMetaType::registerDebugStreamOperator<EntityCode>();

    qRegisterMetaType<EntityID>();
    qRegisterMetaTypeStreamOperators<EntityID>();
    QMetaType::registerDebugStreamOperator<EntityID>();

    qRegisterMetaType<PropertyID>();
    qRegisterMetaTypeStreamOperators<PropertyID>();
    QMetaType::registerDebugStreamOperator<PropertyID>();

    qRegisterMetaType<OperationID>();
    qRegisterMetaTypeStreamOperators<OperationID>();
    QMetaType::registerDebugStreamOperator<OperationID>();

    qRegisterMetaType<Command>();
    qRegisterMetaTypeStreamOperators<Command>();
    QMetaType::registerDebugStreamOperator<Command>();

    qRegisterMetaType<TableID>();
    qRegisterMetaTypeStreamOperators<TableID>();
    QMetaType::registerDebugStreamOperator<TableID>();

    qRegisterMetaType<FieldID>();
    qRegisterMetaTypeStreamOperators<FieldID>();
    QMetaType::registerDebugStreamOperator<FieldID>();

    qRegisterMetaType<FieldIdList>();
    qRegisterMetaTypeStreamOperators<FieldIdList>();
    QMetaType::registerDebugStreamOperator<FieldIdList>();

    qRegisterMetaType<MessageCode>();
    qRegisterMetaTypeStreamOperators<MessageCode>();
    QMetaType::registerDebugStreamOperator<MessageCode>();

    qRegisterMetaType<DataTablePtr>();
    qRegisterMetaType<DataRestrictionPtr>();

    qRegisterMetaType<SrvTableInfoPtr>();

    qRegisterMetaType<ImageCateroryID>();
    qRegisterMetaType<ImageID>();

    qRegisterMetaType<HierarchyList>();

    qRegisterMetaType<ChangeInfo>();

    PropertyRestriction::_meta_type_number = qRegisterMetaType<PropertyRestriction>();

    _FlatItemModel_data::_meta_type_number = qRegisterMetaType<_FlatItemModel_data>();

    qRegisterMetaType<zf::ModelServiceLookupRequestOptions>();
}

void Framework::bootstrapAccessibility()
{
#if false // отключаем т.к. пока не востребовано, доделано не везде и вообще похоже приводит к крешам на специфическом железе типа asus zenbook с тачпадом-экраном   
    if (Core::mode().testFlag(CoreMode::Application)) {
        QAccessible::installFactory(ItemSelectorAccessibilityInterface::accessibleFactory);
        QAccessible::installFactory(AccessibleTable::accessibleFactory);
        QAccessible::installFactory(AccessibleTree::accessibleFactory);
        QAccessible::installFactory(AccessibleMenu::accessibleFactory);
        QAccessible::installFactory(AccessibleMenuBar::accessibleFactory);
        QAccessible::installFactory(AccessibleMenuItem::accessibleFactory);
        QAccessible::installFactory(PictureSelectorAccessibilityInterface::accessibleFactory);
    }
#endif
}

void Framework::loadSystemData()
{
    _dialog_configuration = std::make_unique<DialogConfigurationRepository>();
    if (!Core::mode().testFlag(CoreMode::Library) && QFile::exists(dialogConfigFile())) {
        QFile f(dialogConfigFile());
        if (f.open(QFile::ReadOnly)) {
            Error error = _dialog_configuration->loadFromDevice(&f);
            f.close();

            if (error.isError())
                Core::logError(error);
        } else
            Core::logError(Error::fileIOError(dialogConfigFile()));
    }
}

void Framework::initJsGlobalFunctions()
{
    // выбор первого не NULL параметра
    jsEngine()->evaluate(R"(
                function coalesce() {
                    var len = arguments.length;
                    for (var i=0; i<len; i++) {
                        if (arguments[i] !== null && arguments[i] !== undefined) {
                            return arguments[i];
                        }
                    }
                    return null;
                }
    )");

    // приведение к числу. NULL -> 0
    jsEngine()->evaluate(R"(
        function to_num(p) {
            return Number(coalesce(p,0));
        }
    )");

    // сравнение двух double
    jsEngine()->evaluate(R"(
        function fuzzyEqual(a, b) {
            var a1 = to_num(a);
            var b1 = to_num(b);
            if (Math.abs(a1) < Math.abs(b1))
                return Math.abs(a1 - b1) <= Math.abs(b1) * 0.00001;
            else
                return Math.abs(a1 - b1) <= Math.abs(a1) * 0.00001;
        }
                         )");
}

QList<I_Plugin*> Framework::createCorePlugins()
{
    QList<I_Plugin*> plugins;

    plugins << new ModelService::Service;

    return plugins;
}

} // namespace zf
