#ifndef ZF_FRAMEWORK_H
#define ZF_FRAMEWORK_H

#include "zf_core.h"
#include "zf_model.h"
#include "zf_view.h"
#include "zf_translation.h"
#include "zf_mdi_area.h"
#include "zf_i_work_zones.h"
#include "zf_sequence_generator.h"

#include <QJSEngine>

Q_DECLARE_METATYPE(const QObject*)
Q_DECLARE_METATYPE(QList<const QObject*>)

namespace spdlog
{
class logger;
}

namespace zf
{
class ModelManager;
class ModuleManager;
class I_DatabaseDriver;
class DialogConfigurationRepository;
class SharedPtrDeleter;

class Framework : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public: // константы для тонкой настройки
    //! Интервал вызова ProcessEvents
    static const int PROCESS_EVENTS_TIMEOUT_MS;
    //! Сколько моделей за один раз обрабатывает ModelKeeper
    static const int MODEL_KEEPER_ONE_STEP;
    //!  Сколько сообщений буфера обрабатывает MessageDispatcher за один раз
    static const int MESSAGE_DISPATCHER_ONE_STEP;

    //! Ключи системного менеджера обратных вызовов (Framework::internalCallbackManager())
    //! Вынесены в одно место чтобы не было случайного пересечения ключей при наследовании
    enum InternalCallbackKeys
    {
        START_NUM = -10000,

        MODULE_OBJECT_AFTER_CREATED,
        MODULE_DATA_OBJECT_AFTER_CREATED,
        ENTITY_OBJECT_AFTER_CREATED,

        // модель
        MODEL_FINISH_CUSTOM_LOAD,
        MODEL_FINISH_CUSTOM_SAVE,
        MODEL_FINISH_CUSTOM_REMOVE,

        // представление
        VIEW_REQUEST_UPDATE_ACCESS_RIGHTS,
        VIEW_REQUEST_UPDATE_WIDGET_STATUS,
        VIEW_REQUEST_UPDATE_ALL_WIDGET_STATUS,
        VIEW_UPDATE_PROPERTY_STATUS_KEY,
        VIEW_REQUEST_UPDATE_ALL_PROPERTY_STATUS,
        VIEW_INIT_TAB_ORDER_KEY,
        VIEW_INVALIDATED_KEY,
        VIEW_UPDATE_BLOCK_UI_GEOMETRY_KEY,
        VIEW_HIDE_BLOCK_UI_KEY,
        VIEW_COMPRESS_ACTIONS_KEY,
        VIEW_LOAD_CONDITION_FILTER_KEY,
        VIEW_AFTER_LOAD_CONFIG_KEY,
        VIEW_UPDATE_PARENT_VIEW,
        VIEW_AUTO_SELECT_FIRST_ROW,
        VIEW_REQUEST_PROCESS_VISIBLE_CHANGE,
        VIEW_HIGHLIGHT_CHANGED_KEY,
        VIEW_DATA_READY,

        //! Ключ обратного вызова на обновления виджетов по списку
        DATA_WIDGETS_UPDATE_REQUESTS_CALLBACK_KEY,
        //! Ключ обратного вызова на обновления всех виджетов
        DATA_WIDGETS_UPDATE_ALL_CALLBACK_KEY,

        //! CommandProcessor
        COMMAND_PROCESSOR_COMMAND_CALLBACK_KEY,

        //! Код callback для обработки буфера
        //        MESSAGE_DISPATCHER_PROCESS_BUFFER_CALLBACK_KEY,
        //! MessageProcessor
        MESSAGE_PROCESSOR_CALLBACK_KEY,

        //! ModelKeeper
        MODEL_KEEPER_REQUEST_CALLBACK_KEY,
        //! ModelManager
        MODEL_MANAGER_SYNC_OPERATION_CALLBACK_KEY,
        //! ModuleWindow
        MODULE_WINDOW_CREATE_TOOLBAR_KEY,
        MODULE_WINDOW_RESIZE_KEY,
        MODULE_WINDOW_SAVE_CONFIG_KEY,
        //! ModalWindow
        MODAL_WINDOW_CREATE_TOOLBAR_KEY,
        MODAL_WINDOW_CHECK_ENABLED_KEY,
        //! WindowManager
        WINDOW_MANAGER_DELETE_LATER_KEY,
        //! Dialog
        DIALOG_AFTER_CREATED_KEY,
        DIALOG_CHECK_BUTTONS_KEY,
        DIALOG_INIT_TAB_ORDER_KEY,
        DIALOG_CHANGE_BOTTOM_LINE_VISIBLE,
        DIALOG_AFTER_LOAD_CONFIG_KEY,
        //! SelectFromModelDialog
        SELECT_FROM_MODEL_DIALOG_SET_FILTER_KEY,
        SELECT_FROM_MODEL_DIALOG_ENABLED_KEY,

        //! OperationMenuManager
        OPERATION_MENU_MANAGER_PROCESS_TOOLBAR_KEY,

        //! MainWindow
        MAIN_WINDOW_SPLITTER_KEY,

        //! HightlightController
        HIGHLIGHT_CONTROLLER_SYNCHRONIZE,
        HIGHLIGHT_CONTROLLER_SET_FOCUS,

        //! HightlightView
        HIGHLIGHT_VIEW_UPDATE_KEY,
        HIGHLIGHT_VIEW_RESIZE_KEY,

        //! CumulativeError
        CUMULATIVE_ERROR_KEY,

        //! ItemSelector
        ITEM_SELECTOR_LOAD_MODEL_KEY,

        //! HighlightDialog
        HIGHLIGHT_DIALIG_CREATE_HIGHLIGHT_KEY,

        //! HighlightProcessor
        HIGHLIGHT_PROCESSOR_CHECK_CALLBACK_KEY,

        // WidgetHighligterController
        WIDGET_HIGHLIGHT_CONTROLLER_UPDATE_HIGHLIGHT_KEY,

        //! PlainTextEdit
        PLAIN_TEXT_EDIT_UPDATE_STYLE,
    };

    //! Коды, передаваемые в HighlightInfo::insert/empty/set
    enum HighlightID
    {
        //! уникальность
        HIGHLIGHT_ID_UNIQUE = 1,
        //! требуется значение
        HIGHLIGHT_ID_REQUIRED = 2,
        //! максимальная длина текста
        HIGHLIGHT_ID_MAX_LENGTH_TEXT = 3,
        //! Данные содержат значение, которого нет в лукап
        HIGHLIGHT_ID_BAD_LOOKUP_VALUE = 4,
        //! Данные содержат значение InvalidValue
        HIGHLIGHT_ID_INVALID_VALUE = 5,
        //! Регулярное выражение
        HIGHLIGHT_ID_REGEXP = 6,
        //! QValidator
        HIGHLIGHT_ID_VALIDATOR = 7,
    };

public:
    Framework(ModelManager* model_manager, ModuleManager* module_manager, DatabaseManager* database_manager,
              //! Устанавливать свои обработчики ошибок
              bool is_install_error_handlers,
              //! Список файлов с переводами для разных языков. Путь к файлу - относительно директории приложения
              //! Если текущего языка в списке нет, то выбирается первый
              const QMap<QLocale::Language, QString>& translation);
    ~Framework();

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
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Загрузка кэша и т.п.
    void loadSystemData();
    //! Сохранение кэша и т.п.
    Error saveSystemData();

    //! Менеджер обратных вызовов для внутреннего использования
    static CallbackManager* internalCallbackManager();
    //! Генератор последовательностей для внутреннего использования
    static SequenceGenerator<quint64>* internalSequenceGenerator();

    //! Установить общее меню операций. Допустимы операции OperationScope::Module
    //! Вызывать после регистрации всех модулей, т.к. при формировании меню у модулей будет запрашиваться информация об
    //! операции
    void installOperationsMenu(const OperationMenu& main_menu);
    //! Установлено ли меню операций
    bool isOperationMenuInstalled() const;
    //! Меню операций
    OperationMenuManager* operationMenuManager() const;
    //! Менеджер моделей
    ModelManager* modelManager() const;

    //! Отложенное удаление умных указателей
    SharedPtrDeleter* sharedPtrDeleter() const;

    //! Задать стиль и размер кнопок тулбара
    void setToolbarStyle(
            ToolbarType type,
        //! Стиль
        Qt::ToolButtonStyle style,
        //! Размер иконок
        int size);


    //! Стиль тулбара
    Qt::ToolButtonStyle toolbarButtonStyle(ToolbarType type) const;
    //! Размер иконок
    int toolbarButtonSize(ToolbarType type) const;

    //! Список файлов с переводами для разных языков. Путь к файлу - относительно директории приложения
    //! Если текущего языка в списке нет, то выбирается первый
    const QMap<QLocale::Language, QString>& translation() const;

    //! Текущий язык интерфейса
    QLocale::Language languageUi() const;
    //! Текущий язык документооборота
    QLocale::Language languageWorkflow() const;
    void setLanguage(QLocale::Language language_ui, QLocale::Language language_workflow);

    //! Локаль по умолчанию для UI
    const QLocale* localeUi() const;
    //! Локаль по умолчанию для документооборота
    const QLocale* localeWorkflow() const;
    //! Локаль С
    const QLocale* c_locale() const;

    //! QCollator по умолчанию для UI
    const QCollator* collatorUi(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const;
    //! QCollator по умолчанию для документооборота
    const QCollator* collatorWorkflow(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const;
    //! QCollator С
    const QCollator* c_collator(Qt::CaseSensitivity sensitivity, bool ignore_punctuation, bool numeric_mode) const;
    //! QCollator для конкретной локали
    const QCollator* collator(QLocale::Language language, Qt::CaseSensitivity sensitivity, bool ignore_punctuation,
                              bool numeric_mode) const;

    //! Инициализация оконного менеджера
    void initWindowManager(MdiArea* mdi_area);
    //! Оконный менеджер
    WindowManager* windowManager() const;

    //! Приложение открыто (главное окно открыто и не находится в состоянии закрытия)
    bool isActive() const;
    void setActive(bool b);

    /*! Получить значение из набора данных по ключевому значению.
     * Возвращает false, если данные еще не загружены. В этом случает result не инициализирован */
    bool getEntityValue(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Значение из колонки result_column. Инициализировано, если функция вернула true
        QVariant& result,
        //! feedbackId сообщения об окончании загрузки, если на момент запроса данные не загружены и задан requester
        MessageID& feedback_id,
        //! Модель, сформированная на основании source_uid
        ModelPtr& source_model,
        //! Ошибка при попытке загрузить модель с данными
        Error& error,
        //! Набор данных. Если у сущности всего один набор данных, то может быть не задан
        const DataProperty& dataset_property = DataProperty(),
        //! Колонка, по которой идет поиск. Если не задана, то набор данных должен иметь колонку с PropertyOption::Id
        const DataProperty& key_column = DataProperty(),
        //! Колонка из которой извлекается результат. Если не задана, то набор данных должен иметь колонку с
        //! PropertyOption::Name
        const DataProperty& result_column = DataProperty(),
        //! Кому отправлять сообщение об окончании загрузки данных, если они не загружены на момент запроса. Если не
        //! задан, то сообщение не оправляется
        const I_ObjectExtension* requester = nullptr,
        //! Список виджетов, которые должны быть обновлены, после окончания загрузки, если на момент запроса данные не
        //! загружены. Может быть пустым
        const QWidgetList& update_on_data_loaded = QWidgetList(),
        //! Роль для выборки result
        int result_role = Qt::DisplayRole) const;

    //! Записать информацию в журнал приложения
    Error writeToLogStorage(const QString& text, zf::InformationType type);

    //! Загрузить фильтр по набору данных из файла
    Error getConditionFilter(const QString& code, const DataProperty& dataset, ComplexCondition* c) const;
    //! Сохранить фильтр по набору данных в файл
    Error saveConditionFilter(const QString& code, const DataProperty& dataset, const ComplexCondition* c);

    //! Глобальная конфигурация приложения
    Configuration* configuration(int version = 0) const;
    //! Конфигурация по коду сущности
    Configuration* entityConfiguration(
        //! Код сущности
        const EntityCode& entity_code,
        //! Произвольный код
        const QString& key = QString(),
        //! Версия
        int version = 0) const;
    //! Внутренняя конфигурация ядра
    Configuration* coreConfiguration(int version = 0) const;

    //! Локальные параметры приложения
    LocalSettingsManager* localSettings() const;

    //! Хранилище со свойствами диалогов
    DialogConfigurationRepository* dialogConfiguration() const;
    //! Файл настройки диалогов
    QString dialogConfigFile() const;

    //! Значение в каталоге по ID строки. Если каталог не загружен или в процессе загрузки, то пустое значение
    QVariant catalogValue(
        //! Код сущности каталога
        const EntityCode& catalog_id,
        //! Код строки каталога
        const EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Язык
        QLocale::Language language,
        //! База данных
        const zf::DatabaseID& database_id,
        //! Будет инициализировано если модель каталога находится в состоянии загрузки
        ModelPtr& data_not_ready);
    //! Получить расшифровку кода элемента каталога. Если каталог не загружен или в процессе загрузки, то пустое значение
    QString catalogItemName(const Uid& catalog_uid, const EntityID& item_id, QLocale::Language language);

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * ModelListMessage или ErrorMessage с id=feedback_message_id */
    Error getModels( //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Модели (данные пока не загружены)
        QList<ModelPtr>& models,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id */
    bool loadModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Открыть существую в БД модель
    ModelPtr openModelSync(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties,
                           //! Загружать все свойства, если properties_list пустой
                           bool all_if_empty, int timeout_ms, Error& error);
    //! Создать временную модель
    ModelPtr createModel(const DatabaseID& database_id, const EntityCode& entity_code, Error& error);
    ModelPtr detachModel(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties,
                         //! Загружать все свойства, если properties_list пустой
                         bool all_if_empty, Error& error);

    //! Создать представление для указанной модели
    View* createView(const ModelPtr& model,
                     //! Состояние представления
                     const ViewStates& states,
                     //! Произвольные данные. Для возможности создания разных представлений по запросу
                     const QVariant& data, Error& error);

    //! Асинхронно удалить модели из базы данных
    bool removeModelsAsync(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы для удаления
        const UidList& entity_uids,
        //! Атрибуты, которые надо загрузить
        //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
        const QList<DataPropertySet>& properties,
        //! Дополнительные параметры
        const QList<QMap<QString, QVariant>>& parameters,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user,
        //! Message::feedbaFramework::ckMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Синхронно удалить модель из базы данных
    Error removeModelSync(const Uid& entity_uid,
                          //! Атрибуты, которые надо загрузить
                          //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
                          const DataPropertySet& properties,
                          //! Дополнительные параметры
                          const QMap<QString, QVariant>& parameters,
                          //! Загружать все свойства, если properties_list пустой
                          bool all_if_empty,
                          //! Действие пользователя
                          const QVariant& by_user,
                          //! Время ожидания ответа сервера (0 - бесконечно)
                          int timeout_ms);

    //! Зарегистрировать модули
    EntityCodeList registerModules(
        //! Список пар:
        //! [Название библиотеки модуля без указания расширения и префикса lib (для Linux),
        //!  Путь относительно основной папки приложения (может быть пустым)]
        const QList<QPair<QString, QString>>& libraries,
        //! Список заранее созданных плагинов
        const QList<I_Plugin*>& plugins,
        //! Ошибка
        Error& error);

    //! Зарегистрирован ли модуль с указанным кодом
    bool isModuleRegistered(const EntityCode& code) const;
    //! Возвращает название модуля с указанным кодом. Если такого модуля не найдено, то возвращает значение кода
    QString getModuleName(const EntityCode& code) const;
    //! Возвращает общую информацию по зарегистрированному модулю
    ModuleInfo getModuleInfo(const EntityCode& code, Error& error) const;
    //! Коды всех зарегистрированных модулей
    EntityCodeList getAllModules() const;

    //! Получить интерфейс плагина. Плагие существует в единственном экземпляре для указанного кода сущности. За
    //! удаление отвечает ядро
    I_Plugin* getPlugin(const EntityCode& code, Error& error) const;
    //! Параметры плагинов. Содержание зависит от реализации плагинов
    const QMap<EntityCode, QMap<QString, QVariant>>& pluginsOptions() const;

    //! Получить список открытых моделей
    QList<ModelPtr> openedModels() const;

    //! Движок JavaScript
    QJSEngine* jsEngine() const;

    //! Виджет не будет выбираться в качестве parent при создании Dialog и его наследников
    void registerNonParentWidget(QWidget* w);
    //! Указано ли что виджет не будет выбираться в качестве parent при создании Dialog и его наследников
    bool isNonParentWidget(QWidget* w) const;

    //! Рабочие зоны
    I_WorkZones* workZones() const;
    //! Устиановить интерфейс рабочих зон
    void installWorkZonesInterface(I_WorkZones* wz);

    //! Обработчик системных сообщений Qt по умолчанию
    static QtMessageHandler defaultMessageHandler();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    //! Завершилась обработка операции
    void sg_operationProcessed(const zf::Operation& operation, const zf::Error& error);

    //! Сигнал - добавилась рабочая зона
    void sg_workZoneInserted(int id);
    //! Сигнал - удалена рабочая зона
    void sg_workZoneRemoved(int id);
    //! Сигнал - сменилась текущая рабочая зона
    void sg_currentWorkZoneChanged(int previous_id, int current_id);
    //! Сигнал - изменилась доступность рабочей зоны
    void sg_workZoneEnabledChanged(int id, bool enabled);
    //! Изменилась информация о подкючении к серверу
    void sg_connectionInformationChanged(
        //! Информация о подключении к БД. Если не валидно, то значит подключение было потеряно
        const zf::ConnectionInformation& info,
        //! Ошибка подключения
        const zf::Error& error);

private slots:
    //! Сработал экшен операции
    void sl_operationActionActivated(const zf::Operation& operation);
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Был удален виджет из registerNonParentWidget
    void sl_non_parent_destroyed(QObject* obj);
    //! Автоматическое назначение accessibleName в objectName для виджетов
    void sl_accessibleNameChanged();

private:
    //! Установка обработчиков ошибок
    void bootstrapErrorHandlers();
    //! Регистрация нестандартных классов в Qt metadata
    void bootstrapQtMetadata();
    //! Инициализация UI Automation
    void bootstrapAccessibility();    
    //! Инциализация голобальных функций java script
    void initJsGlobalFunctions();

    //! Создать плагины, встроенные в ядро
    QList<I_Plugin*> createCorePlugins();

    ModelManager* _model_manager = nullptr;
    ModuleManager* _module_manager = nullptr;
    DatabaseManager* _database_manager = nullptr;

    ObjectExtensionPtr<OperationMenuManager> _operation_menu_manager;
    ObjectExtensionPtr<WindowManager> _window_manager;

    //! Глобальная конфигурация приложения
    mutable std::unique_ptr<Configuration> _configuration;
    //! Настройки сущностей. Ключ - <код сущности>$<произвольный текстовый код>
    mutable QHash<QString, std::shared_ptr<Configuration>> _entity_configuration;
    //! Внутренняя конфигурация ядра
    mutable std::unique_ptr<Configuration> _core_configuration;

    //! Локальные параметры приложения
    mutable std::unique_ptr<LocalSettingsManager> _local_settings;
    //! Хранилище информации о свойствах диалогов
    mutable std::unique_ptr<DialogConfigurationRepository> _dialog_configuration;

    //! Приложение открыто (главное окно открыто и не находится в состоянии закрытия)
    bool _is_active = false;
    mutable QMutex _is_active_mutex;
    //! Модули были зарегистрированы
    bool _is_modules_registered = false;

    //! Список виджетов, которые не будут выбираться в качестве parent при создании Dialog и его наследников
    QSet<QObject*> _non_parent_widgets;

    //! Стиль главного тулбара
    Qt::ToolButtonStyle _main_toolbar_style = Qt::ToolButtonTextUnderIcon;
    //! Размер иконок главного тулбара
    int _main_toolbar_size = 24;
    //! Стиль тулбара модулей
    Qt::ToolButtonStyle _module_toolbar_style = Qt::ToolButtonIconOnly;
    //! Размер иконок тулбара модулей
    int _module_toolbar_size = 24;

    //! Вывод журнала в файл
    std::shared_ptr<spdlog::logger> _logger;
    QString _log_file;
    QMutex _logger_mutex;

    //! Менеджер обратных вызовов для внутреннего использования
    static ObjectExtensionPtr<CallbackManager> _callback_manager;
    //! Генератор последовательностей для внутреннего использования
    static std::unique_ptr<SequenceGenerator<quint64>> _sequence_generator;
    //! Обработчик системных сообщений Qt по умолчанию
    static QtMessageHandler _default_message_handler;

    //! Список файлов с переводами для разных языков. Путь к файлу - относительно директории приложения
    //! Если текущего языка в списке нет, то выбирается первый
    QMap<QLocale::Language, QString> _translation;

    //! Текущий язык интерфейса
    QLocale::Language _language_ui = QLocale::AnyLanguage;
    //! Текущий язык документооборота
    QLocale::Language _language_workflow = QLocale::AnyLanguage;

    mutable QMutex locale_mutex;

    //! Локаль по умолчанию для UI
    mutable QLocale* _default_locale_ui = nullptr;
    //! Локаль по умолчанию для документооборота
    mutable QLocale* _default_locale_workflow = nullptr;
    //! Локаль C
    mutable QLocale* _default_locale_c = nullptr;

    //! QCollator для языка. Вектор языков, затем вектор из collatorKey
    mutable QVector<QVector<QCollator*>*>* _collator_by_lang = nullptr;
    static int collatorKey(bool case_sensitive, bool ignore_punctuation, bool numeric_mode);

    //! Движок JavaScript
    mutable std::unique_ptr<QJSEngine> _jsEngine;

    //! Рабочие зоны
    I_WorkZones* _work_zones = nullptr;

    //! Отложенное удаление умных указателей
    std::unique_ptr<SharedPtrDeleter> _sptr_deleter;

    //! Подписка на канал CONNECTION_INFORMATION
    SubscribeHandle _subscribe_handle_connection_info;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    //! Была инициализация типов Qt
    static bool _metatype_initialized;
};

} // namespace zf

#endif // ZF_FRAMEWORK_H
