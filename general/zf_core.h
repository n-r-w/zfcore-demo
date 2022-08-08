#pragma once

#include <QCollator>
#include <QContiguousCache>
#include <QDialogButtonBox>
#include <QLocale>
#include <QMessageBox>
#include <QStandardPaths>

#include <cfloat>
#include <limits>
#include <memory>

#include "mydefs.h"
#include "zf_access_rights.h"
#include "zf_basic_types.h"
#include "zf_callback.h"
#include "zf_change_info.h"
#include "zf_colors_def.h"
#include "zf_configuration.h"
#include "zf_connection_information.h"
#include "zf_core_channels.h"
#include "zf_core_consts.h"
#include "zf_core_uids.h"
#include "zf_data_container.h"
#include "zf_data_restriction.h"
#include "zf_data_structure.h"
#include "zf_database_driver_worker.h"
#include "zf_database_manager.h"
#include "zf_database_messages.h"
#include "zf_db_srv.h"
#include "zf_defs.h"
#include "zf_error.h"
#include "zf_external_request.h"
#include "zf_feedback_timer.h"
#include "zf_global.h"
#include "zf_i_catalogs.h"
#include "zf_i_database_driver.h"
#include "zf_i_dataset_visible_info.h"
#include "zf_i_plugin.h"
#include "zf_itemview_header_item.h"
#include "zf_local_settings.h"
#include "zf_log.h"
#include "zf_message.h"
#include "zf_message_dispatcher.h"
#include "zf_mm_messages.h"
#include "zf_model.h"
#include "zf_model_keeper.h"
#include "zf_model_manager.h"
#include "zf_module_info.h"
#include "zf_numeric.h"
#include "zf_object_extension.h"
#include "zf_operation_menu_manager.h"
#include "zf_property_restriction.h"
#include "zf_row_id.h"
#include "zf_sync_ask.h"
#include "zf_table_view.h"
#include "zf_translator.h"
#include "zf_tree_view.h"
#include "zf_uid.h"
#include "zf_utils.h"
#include "zf_validators.h"
#include "zf_window_manager.h"

template <typename T>
uint qHash(const std::shared_ptr<T>& ptr, uint seed = 0)
{
    return qHash(ptr.get(), seed);
}

//! Копирование из стрима с приведением типа к int (для работы с enum class)
template <typename T>
void fromStreamInt(QDataStream& s, T& value)
{
    qint64 temp;
    s >> temp;
    value = static_cast<T>(temp);
}

//! запись в стрим с приведением типа к int (для работы с enum class)
template <typename T>
void toStreamInt(QDataStream& s, const T& value)
{
    s << static_cast<qint64>(value);
}

class QJSEngine;

namespace zf {
class ModelManager;
class ModuleManager;
class WidgetReplacer;
class Framework;
class DataContainer;
class DatabaseDriverConfig;
class CumulativeError;

class ZCORESHARED_EXPORT Core {
public:
    //! Была ли начальная инициализация
    static bool isBootstraped();
    //! Приложение открыто (главное окно открыто и не находится в состоянии закрытия)
    static bool isActive(
        //! Игнорировать состояние главного окна
        bool ignore_main_window = false);
    //! Режим работы ядра
    static CoreModes mode();
    //! Ключ ядра - строка, зависящая от приложения, котрое использует ядро
    static QString coreInstanceKey(bool halt_if_error = true);

    //! Начальная инициализация (вызывается в лаунчере при начальной инициализации ПО)
    static void bootstrap(
        //! Режимы работы ядра
        CoreModes mode,
        //! Ключ ядра - строка, зависящая от приложения, котрое использует ядро
        const QString& core_instance_key,
        //! Список БД (если ядро используется в режим, когда работа с БД не нужна, то должен быть пустым)
        const QList<DatabaseInformation>& database_info,
        //! БД умолчанию (должна входить в database_info)
        //! Если ядро используется в режим, когда работа с БД не нужна, то DatabaseID()
        const DatabaseID& default_database_id,
        //! Список файлов с переводами для разных языков. Путь к файлу - относительно директории приложения
        //! Если текущего языка в списке нет, то выбирается первый
        const QMap<QLocale::Language, QString>& translation,
        //! Параметры плагинов. Содержание зависит от реализации плагинов
        const QMap<EntityCode, QMap<QString, QVariant>>& plugins_options,
        //! Таймаут ожидания завершения работы потоков
        int terminate_timeout_ms,
        //! Параметры кэширования. Ключ - тип сущности, значение - размер кэша
        const EntityCacheConfig& cache_config = EntityCacheConfig(),
        //! Размер кэша по умолчанию
        int default_cache_size = 0,
        //! Размер буфера последних операций менеджера моделей (для отладки)
        int history_size = 0,
        //! Устанавливать обработчики ошибок
        bool is_install_error_handlers = true);

    //! Выгрузить общие ресурсы (вызывается в лаунчере при завершении работы ПО)
    static void freeResources();

    //! Версия ПО
    static Version applicationVersion();
    //! Ревизия ПО
    static QString buildVersion();
    //! Время сборки
    static QDateTime buildTime();
    //! Полная строка с версией, ревизией и временем сборки
    static QString applicationVersionFull();

    //! Идентификатор БД по умолчанию
    static DatabaseID defaultDatabase();
    //! Список доступных БД и информация по ним
    static const QHash<DatabaseID, DatabaseInformation>& databasesInformation();
    //! Информация по конкретной БД (nullptr если не найдена)
    //! По умолчанию ищет информацию для БД по умолчанию
    static const DatabaseInformation* getDatabaseInformation(const DatabaseID& database_id = {});

    //! Локаль указанного типа
    static const QLocale* locale(LocaleType type);
    //! Язык указанного типа
    static QLocale::Language language(LocaleType type);
    //! Для сравнения строк
    static const QCollator* collator(LocaleType type,
        //! Чувствительность к регистру
        Qt::CaseSensitivity sensitivity = Qt::CaseSensitive,
        //! Игнорировать знаки препинания
        bool ignore_punctuation = false,
        //! Сравнение строк с числами как значений
        bool numeric_mode = true);

    //! Язык интерфейса по умолчанию (английский). Используется при любых несоответствиях языка
    static QLocale::Language defaultLanguage();
    //! Список доступных языков пользовательского интерфейса. Определяется на основании доступных файлов перевода
    static QList<QLocale::Language> uiLanguages();

    //! Установить драйвер БД
    static Error registerDatabaseDriver(
        //! Параметры драйвера БД
        const DatabaseDriverConfig& config,
        //! Название библиотеки драйвера без указания расширения и префикса lib (для Linux)
        const QString& library_name,
        //! Путь относительно основной папки приложения
        const QString& path = QString());

    //! Текущий объект DatabaseDriverWorker. Находится в отдельном потоке
    static DatabaseDriverWorker* driverWorker();

    //! Идентификатор каталога по его коду сущности
    static Uid catalogUid(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Структура данных каталога
    static DataStructurePtr catalogStructure(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Модель каталога по его коду сущности
    static ModelPtr catalogModel(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Модель каталога по его коду сущности
    template <class T>
    static std::shared_ptr<T> catalogModel(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase())
    {
        auto m = catalogModel(catalog_id, database_id);
        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(m);
        if (converted == nullptr)
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(catalog_id.value()));
        return converted;
    }

    //! Модель каталога по его коду сущности. Синхронно грузит модель
    static ModelPtr catalogModelSync(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Модель каталога по его коду сущности. Синхронно грузит модель
    template <class T>
    static std::shared_ptr<T> catalogModelSync(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase())
    {
        auto m = catalogModelSync(catalog_id, database_id);
        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(m);
        if (converted == nullptr)
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(catalog_id.value()));
        return converted;
    }

    //! Свойство каталога
    static DataProperty catalogProperty(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код свойства каталога
        const PropertyID& property_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Проверка на идентификатор каталога
    static bool isCatalogUid(const Uid& entity_uid);
    //! Идентификатор основной сущности каталога. Используется если каталог является упрощенным видом для полноценной списочной сущности
    static Uid catalogMainEntityUid(const zf::EntityCode& catalog_id,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Идентификатор каталога, который содержит список сущностей данного типа
    static zf::Uid catalogEntityUid(
        //! Код сущности
        const EntityCode& entity_code,
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Извлечь ID каталога из идентификатора каталога
    static zf::EntityCode catalogId(const zf::Uid& catalog_uid);

    /*! Значение в колонке с названием в каталоге по ID строки.
     * НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО! Иначе это вызовет постоянную подрузку модели и метод будет возвращать пустые значения
     * Исключение только для фейковых справочников - для них можно использовать везде
     * Если на момент вызова каталог не загружен, то вернет пустое значение. Если надо обрабатывать подобные случаи,
     * то можно подписаться на сигнал Model::sg_finishLoad для модели каталога, полученной через Core::catalogModel
     * Пустое значение также вернется если такой строки не существует
     * Работает быстро, рекомендуется для массовых вызовов (например в таблицах) */
    static QVariant catalogValue(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        int id,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    /*! Значение в каталоге по ID строки.
     * НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО! Иначе это вызовет постоянную подрузку модели и метод будет возвращать пустые значения
     * Исключение только для фейковых справочников - для них можно использовать везде
     * Если на момент вызова каталог не загружен, то вернет пустое значение. Если надо обрабатывать подобные случаи,
     * то можно подписаться на сигнал Model::sg_finishLoad для модели каталога, полученной через Core::catalogModel
     * Пустое значение также вернется если такой строки не существует
     * Работает быстро, рекомендуется для массовых вызовов (например в таблицах) */
    static QVariant catalogValue(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    /*! Значение в каталоге по ID строки.
     *  НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО! Иначе это вызовет постоянную подрузку модели и метод будет возвращать пустые значения
     * Исключение только для фейковых справочников - для них можно использовать везде
     * Если на момент вызова каталог не загружен, то вернет пустое значение. Если надо обрабатывать подобные случаи,
     * то можно подписаться на сигнал Model::sg_finishLoad для модели каталога, полученной через Core::catalogModel
     * Пустое значение также вернется если такой строки не существует
     * Работает быстро, рекомендуется для массовых вызовов (например в таблицах) */
    static QVariant catalogValue(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Будет инициализировано если модель каталога находится в состоянии загрузки
        ModelPtr& data_not_ready,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Значение в каталоге по ID строки. Работает через сообщения (синхронно)
    //! Не использовать для массовых вызовов (например в таблицах)
    //! НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО!
    //! Исключение только для фейковых справочников - для них можно использовать везде
    static QVariant catalogValueSync(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Ошибка
        Error& error,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms = 0,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Значение в каталоге по ID строки. Работает через сообщения (синхронно)
    //! Не использовать для массовых вызовов (например в таблицах)
    //! Если ошибка, то она логируется но пользователю не показывается
    //! НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО!
    //! Исключение только для фейковых справочников - для них можно использовать везде
    static QVariant catalogValueSync(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms = 0,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Значение в каталоге по ID строки. Асинхронно
    //! Возвращает ID сообщения на которое надо ждать ответ. Ответ поступит от CoreUids::MODEL_MANAGER
    //! НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО!
    //! Исключение только для фейковых справочников - для них можно использовать везде
    static MessageID catalogValueAsync(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Идентификатор свойства каталога для которого надо получить значение
        const PropertyID& property_id,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Имя в каталоге по ID строки. Синхронно
    //! НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО!
    //! Исключение только для фейковых справочников - для них можно использовать везде
    static QVariant catalogItemNameSync(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Ошибка
        Error& error,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms = 0,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());
    //! Имя в каталоге по ID строки. Асинхронно
    //! Возвращает ID сообщения на которое надо ждать ответ. Ответ поступит от CoreUids::MODEL_MANAGER
    //! НЕ ИСПОЛЬЗОВАТЬ В ДРАЙВЕРЕ И В ЦЕЛОМ В ПОТОКАХ, КРОМЕ ГЛАВНОГО!
    //! Исключение только для фейковых справочников - для них можно использовать везде
    static MessageID catalogItemNameAsync(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! База данных
        const zf::DatabaseID& database_id = defaultDatabase());

    //! Задать интерфейс по работе с каталогами
    static void installCatalogsInterface(I_Catalogs* catalogs);
    //! Интерфейс по работе с каталогами
    static I_Catalogs* catalogsInterface();

    //! Логин текушего пользователя
    static QString currentUserLogin();
    //! Задать логин текушего пользователя. В принципе он автоматически задается при setDatabaseCredentials
    //! но имеет смысл его задать раньше, чтобы логи писались в каталог пользователя
    static void setCurrentUserLogin(const QString& login);

    //! Задать интерфейс генерации объектов DataRestriction
    //! За генерацию таких объектов отвечает драйвер
    static void installDataRestrictionInterface(I_DataRestriction* restriction);
    //! Создать новый пустой объект DataRestriction
    static DataRestrictionPtr createDataRestriction(const TableID& table_id);

    //! Задать параметры соединения с БД (предварительно должен быть установлен драйвер БД)
    static void setDatabaseCredentials(const DatabaseID& database_id, const Credentials& credentials);

    //! Вызывать после того, как будет готова работа с базой данных (установлен драйвер, логин и пароль)
    static Error databaseInitialized();

    //! Информация о подключении к БД
    static const ConnectionInformation* connectionInformation(bool halt_if_null = true);
    //! Локальные настройки приложения
    static const LocalSettings* settings();

    //! Изменения
    static QString changelog();
    static void setChangelog(const QString& s);

    //! Права доступа к приложению в целом
    static AccessRights applicationAccessRights();
    //! Права доступа для типа сущностей или конкретной сущности
    static const Permissions* directPermissions();
    /*! Права доступа для объектов(документов), связанных с какой либо сущностью.
     * Например: если даны права на просмотр для проекта, то это значит права доступа на просмотр для документов по данному проекту, но
     * не дает прав на просмотр карточки самого проекта. Для просмотра карточки проекта надо задать direct_permissions */
    static const Permissions* relationPermissions();

    /*! Асинхронный запрос прав доступа к перечню идентификаторов сущностей. Если список пустой, то запрашивается информация по всем
     * доступным сущностям. Ответ придет отправителю с Message::feedbackMessageId равному возвращаемому значению.
     * Будет отправлено сообщение DBCommandGetAccessRightsMessage для DatabaseManager, ответом будет DBEventAccessRightsMessage или
     * ErrorMessage. Если права доступа лежат в кэше, то функция возвращает пустую строку и заполняет параметры direct_access_rights,
     * relation_access_rights */
    static MessageID requestAccessRights(
        //! Объект, которому придет ответ ReportTemplateMessage
        const I_ObjectExtension* sender,
        //! Идентификаторы сущностей, по которым нужны права
        const UidList& entity_uids,
        //! Прямые права (если они лежат в кэше и можно получить сразу)
        AccessRightsList& direct_access_rights,
        //! Косвенные права (если они лежат в кэше и можно получить сразу)
        AccessRightsList& relation_access_rights,
        //! Логин. Если не задано, то для текущего
        const QString& login = QString());

    //! Запросить генерацию отчета. Возвращает код Message::feedbackMessageId для ответного сообщения
    //! Пока не поддерживается
    static MessageID requestReport(
        //! Объект, которому придет ответ ReportTemplateMessage
        const I_ObjectExtension* sender,
        //! id шаблона
        const QString& template_id,
        //! Данные для генерации отчета
        const DataContainer& data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне
        const QMap<DataProperty, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<DataProperty, QLocale::Language>& field_languages = {});

    //! Зарегистрировать модули
    static EntityCodeList registerModules(
        //! Список пар:
        //! [Название библиотеки модуля без указания расширения и префикса lib (для Linux),
        //!  Путь относительно основной папки приложения (может быть пустым)]
        const QList<QPair<QString, QString>>& libraries,
        //! Список плагинов
        const QList<I_Plugin*>& plugins,
        //! Ошибка
        Error& error);
    //! Зарегистрировать модули
    static EntityCodeList registerModules(
        //! Список названий библиотек модуля без указания расширения и префикса lib (для Linux)
        const QStringList& libraries,
        //! Список плагинов
        const QList<I_Plugin*>& plugins,
        //! Ошибка
        Error& error);

    //! Установить общее меню операций. Допустимы операции OperationScope::Module
    //! Вызывать после регистрации всех модулей, т.к. при формировании меню у модулей будет запрашиваться информация об
    //! операции (вызывается в лаунчере при начальной инициализации ПО)
    static void installOperationsMenu(const OperationMenu& main_menu);

    //! Стиль тулбара
    static Qt::ToolButtonStyle toolbarButtonStyle(ToolbarType type);
    //! Размер иконок тулбара
    static int toolbarButtonSize(ToolbarType type);

    //! Диспетчер сообщений
    static MessageDispatcher* messageDispatcher(bool halt_if_null = true);
    //! Менеджер базы данных
    static DatabaseManager* databaseManager(bool halt_if_null = true);
    //! Менеджер моделей
    static ModelManager* modelManager(bool halt_if_null = true);
    //! Оконный менержер
    static WindowManager* windowManager(bool halt_if_null = true);
    //! Меню операций
    static OperationMenuManager* operationMenuManager(bool halt_if_null = true);
    //! Хранитель моделей
    static ModelKeeper* modelKeeper(bool halt_if_null = true);
    //! Для управления запросами к внешним сервисам на уровне сигналов/слотов Qt
    static ExternalRequester* externalRequester();

    //! Глобальный прогресс. Вызывает блокировку UI всего приложения
    static ProgressObject* progress();

    //! Зарегистрирован ли модуль с указанным кодом
    static bool isModuleRegistered(const EntityCode& entity_code);
    //! Возвращает название модуля с указанным кодом. Если такого модуля не найдено, то возвращает значение кода
    static QString getModuleName(const EntityCode& entity_code);

    //! Возвращает общую информацию по зарегистрированному модулю
    static ModuleInfo getModuleInfo(const EntityCode& entity_code, Error& error);
    //! Возвращает общую информацию по зарегистрированному модулю. Если модуль не найден - критическая ошибка
    static ModuleInfo getModuleInfo(const EntityCode& entity_code);

    //! Возвращает структуру данных модуля
    static DataStructurePtr getModuleDataStructure(const EntityCode& entity_code, Error& error);
    //! Возвращает структуру данных модуля. Если модуль не найден - остановка
    static DataStructurePtr getModuleDataStructure(const EntityCode& entity_code);
    //! Возвращает структуру данных модуля
    static DataStructurePtr getModuleDataStructure(const Uid& entity_uid, Error& error);
    //! Возвращает структуру данных модуля. Если модуль не найден - остановка
    static DataStructurePtr getModuleDataStructure(const Uid& entity_uid);

    //! Замена виджета на новый. Метод удаляет виджет source и ставит на его место target
    //! source должен быть заранее размещен на форме (иметь parentWidget)
    static void replaceWidget(QWidget* source, QWidget* target,
        //! Копировать ли свойства
        bool copy_properties = false,
        //! Какое имя надо присвоить target. По умолчанию берет имя из source
        const QString& name = QString());

    //! Выполнить автозамену виджетов. Только для независимых структур данных, созданных вручную
    static void replaceWidgets(const DataWidgets* widgets, QWidget* body);

    //! Очистить кэш сущностей полностью
    static void clearEntityCache();
    //! Очистить кэш от сущностей определенного типа
    static void clearEntityCache(const EntityCode& entity_code);
    //! Очистить кэш от конкретной сущности
    static void clearEntityCache(const Uid& uid);
    //! Текущий размер кэша для сущностей определенного типа
    static int entityCacheSize(const EntityCode& entity_code);

    //! Получить БД, в которой находятся объекты указанного типа
    static DatabaseID entityCodeDatabase(
        //! Владелец объекта (идентификатор клиента и т.п.)
        const Uid& owner_uid,
        //! Код сущности
        const EntityCode& entity_code);

    //! Есть ли в БД сущность с указанным идентификатором (синхронный метод)
    static bool isEntityExists(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms, Error& error);
    //! Есть ли в БД сущности с указанными идентификатором (синхронный метод)
    static QList<bool> isEntitiesExists(
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms,
        //! Ошибка
        Error& error);

    //! Получить название сущности по ее идентификатору (синхронно)
    static QString getEntityNameSync(
        //! Идентификатор
        const Uid& uid,
        //! Ошибка
        Error& error,
        //! Код свойства, содержащего имя. Если < 0, то ищется по PropertyOption::FullName(Name)
        const PropertyID& name_property_id = {},
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms = 0);
    //! Получить название сущности по ее идентификатору (синхронно)
    static QStringList getEntityNamesSync(
        //! Идентификатор
        const UidList& uids,
        //! Ошибка
        Error& error,
        //! Код свойства, содержащего имя. Должно совпадать по количеству в uids или быть пустым.
        //! Если пусто, то ищется по PropertyOption::FullName(Name)
        const PropertyIDList& name_property_ids = {},
        //! Язык
        QLocale::Language language = defaultLanguage(),
        //! Время ожидания ответа сервера (0 - бесконечно)
        int timeout_ms = 0);

    //! Получить название сущности по ее идентификатору (асинхронно)
    //! Возвращает ID сообщения на которое надо ждать ответ. В ответ поступит MMEventEntityNamesMessage или ErrorMessage от CoreUids::MODEL_MANAGER
    static MessageID getEntityNames(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Идентификаторы
        const UidList& uids,
        //! Коды свойств, содержащего имя. Должно совпадать по количеству в uids или быть пустым.
        //! Если пусто, то ищется по PropertyOption::FullName(Name)
        const PropertyIDList& name_property_ids = {},
        //! Язык
        QLocale::Language language = defaultLanguage());
    //! Получить название сущности по ее идентификатору (асинхронно)
    //! Возвращает ID сообщения на которое надо ждать ответ. В ответ поступит MMEventEntityNamesMessage или ErrorMessage от CoreUids::MODEL_MANAGER
    static MessageID getEntityName(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Идентификатор
        const Uid& uid,
        //! Код свойства, содержащего имя
        //! Если пусто, то ищется по PropertyOption::FullName(Name)
        const PropertyID& name_property_ids = {},
        //! Язык
        QLocale::Language language = defaultLanguage());

    //! Создать идентификатор плагина
    static Uid pluginUid(const EntityCode& code);

    //! Получить указатель на плагин указанной сущности. Плагин существует в единственном экземпляре для указанного кода сущности. За
    //! удаление отвечает ядро
    template <class T>
    static inline T* getPlugin(const EntityCode& entity_code, Error& error)
    {
        I_Plugin* plugin = getPluginHelper(entity_code, error);
        if (error.isError())
            return nullptr;

        T* converted = dynamic_cast<T*>(plugin);
        if (converted == nullptr) {
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(entity_code.value()));
        }
        return converted;
    }
    //! Получить указатель на плагин указанной сущности. Если модуль не найден - критическая ошибка
    template <class T>
    static inline T* getPlugin(const EntityCode& entity_code)
    {
        Error error;
        T* plugin = getPlugin<T>(entity_code, error);
        Z_CHECK_ERROR(error);
        return plugin;
    }
    //! Получить базовый интерфейс плагина. Плагин существует в единственном экземпляре для указанного кода сущности. За
    //! удаление отвечает ядро
    static I_Plugin* getPlugin(const EntityCode& entity_code, Error& error);
    //! Получить базовый интерфейс плагина. Если модуль не найден - критическая ошибка
    static I_Plugin* getPlugin(const EntityCode& entity_code);

    //! Коды всех зарегистрированных модулей
    static EntityCodeList getAllModules();

    //! Сделать загруженные модели с указанным кодом сущности невалидными
    static bool invalidateModels(const EntityCode& entity_code);

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id
     * Список не загруженных моделей выдается сразу
     * В отличие от loadModels метод может тормозить на запросе слишком большого количества моделей за один раз */
    static Error getModels(
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
        //! Модели (данные пока не загружены)
        QList<ModelPtr>& models,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);
    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id
     * Список не загруженных моделей выдается сразу
     * В отличие от loadModels метод может тормозить на запросе слишком большого количества моделей за один раз */
    static Error getModels(
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Загружать все свойства, если properties_list пустой
        const QList<bool>& all_if_empty_list,
        //! Модели (данные пока не загружены)
        QList<ModelPtr>& models);
    /*! Асинхронно запросить создание экземпляров моделей со всеми свойствами. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id
     * Список не загруженных моделей выдается сразу
     * В отличие от loadModels метод может тормозить на запросе слишком большого количества моделей за один раз */
    static Error getModels(
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Модели (данные пока не загружены)
        QList<ModelPtr>& models);

    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id */
    static bool loadModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<DataPropertySet>& properties_list,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);
    /*! Асинхронно запросить создание экземпляров моделей. По завершении создания requester получает сообщение вида
     * MMEventModelList или ErrorMessage с id=feedback_message_id */
    static bool loadModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Параметры загрузки
        const QList<LoadOptions>& load_options_list,
        //! Что загружать
        const QList<PropertyIDSet>& properties_list,
        //! Message::feedbackMessageId для сообщения с ответом
        MessageID& feedback_message_id);

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить
        const DataPropertySet& properties,
        //! Ошибка
        Error& error)
    {
        MessageID feedback_message_id;
        return getModel<T>(entity_uid, load_options, properties, error, nullptr, feedback_message_id);
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить
        const DataPropertySet& properties,
        //! Ошибка
        Error& error,
        //! Кому направить сообщение об окончании асинхронной загрузки
        const I_ObjectExtension* requester,
        //! Message::feedbackId для сообщения, отправляемого requester
        MessageID& feedback_message_id)
    {
        QList<ModelPtr> models;
        error = getModels(requester, { entity_uid }, { load_options }, { properties }, { false }, models, feedback_message_id);
        if (error.isError())
            return std::shared_ptr<T>();

        Z_CHECK(models.count() == 1 && models.first() != nullptr);

        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(models.first());
        if (converted == nullptr) {
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(entity_uid.code()));
        }
        return converted;
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод без указания LoadOptions
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const DataPropertySet& properties,
        //! Ошибка
        Error& error)
    {
        MessageID feedback_message_id;
        return getModel<T>(entity_uid, {}, properties, error, nullptr, feedback_message_id);
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод без указания LoadOptions
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const PropertyIDSet& properties,
        //! Ошибка
        Error& error)
    {
        DataPropertySet props;
        for (auto& p : properties) {
            props << DataProperty::property(entity_uid, p);
        }

        return getModel<T>(entity_uid, props, error);
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод с остановкой при ошибке
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const PropertyIDSet& properties)
    {
        Error error;
        auto res = getModel<T>(entity_uid, properties, error);
        if (error.isError())
            Z_HALT(error);
        return res;
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод без указания LoadOptions. Не загружает никакие аттрибуты(свойства).
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Ошибка
        Error& error)
    {
        MessageID feedback_message_id;
        return getModel<T>(entity_uid, {}, {}, error, nullptr, feedback_message_id);
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод. В случае ошибки - остановка программы. Не загружает никакие аттрибуты(свойства).
    template <class T>
    static inline std::shared_ptr<T> getModel(
        //! Идентификатор сущности
        const Uid& entity_uid)
    {
        Error error;
        auto res = getModel<T>(entity_uid, {}, {}, error);
        if (error.isError())
            Z_HALT(error);
        return res;
    }

    //! Синхронно загрузить модель. Использовать только в модальных диалогах
    template <class T>
    static inline std::shared_ptr<T> getModelSync(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const PropertyIDSet& properties,
        //! Ошибка
        Error& error,
        //! Время ожидания. По умолчанию - бесконечно
        int timeout_ms = 0)
    {
        auto model = getModel<T>(entity_uid, properties, error);
        if (error.isError())
            return nullptr;

        error = waitForLoadModel(model, timeout_ms);
        if (error.isError())
            return nullptr;

        return model;
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод. Модель загружается со всеми доступными свойствами. В случае ошибки - остановка программы
    template <class T>
    static inline std::shared_ptr<T> getModelFull(
        //! Идентификатор сущности
        const Uid& entity_uid)
    {
        Error error;
        auto res = getModelFull<T>(entity_uid, error);
        if (error.isError())
            Z_HALT(error);
        return res;
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод. Модель загружается со всеми доступными свойствами. В случае ошибки - остановка программы
    template <class T>
    static inline std::shared_ptr<T> getModelFull(
        //! Идентификатор сущности
        const EntityCode& entity_code,
        //! Идентификатор базы данных
        const DatabaseID& database = defaultDatabase())
    {
        Error error;
        auto res = getModelFull<T>(Uid::uniqueEntity(entity_code, database), error);
        if (error.isError())
            Z_HALT(error);
        return res;
    }

    //! Асинхронно запросить создание экземпляра модели указанным идентификатором и привести указатель к указанному типу
    //! Указатель на модель доступен сразу, но она может быть не инициализирована
    //! Упрощенный метод. Модель загружается со всеми доступными свойствами. В случае ошибки - остановка программы
    template <class T>
    static inline std::shared_ptr<T> getModelFull(
        //! Идентификатор сущности
        const Uid& entity_uid, Error& error)
    {
        auto res = getModel<T>(entity_uid, {}, DataStructure::structure(entity_uid)->propertiesMain(), error);
        if (error.isError())
            return nullptr;
        return res;
    }

    //! Синхронно загрузить модель со всеми доступными свойствами. Использовать только в модальных диалогах
    template <class T>
    static inline std::shared_ptr<T> getModelFullSync(
        //! Идентификатор сущности
        const Uid& entity_uid, Error& error,
        //! Время ожидания. По умолчанию - бесконечно
        int timeout_ms = 0)
    {
        auto model = getModel<T>(entity_uid, {}, DataStructure::structure(entity_uid)->propertiesMain(), error);
        if (error.isError())
            return nullptr;

        error = waitForLoadModel(model, timeout_ms);
        if (error.isError())
            return nullptr;

        return model;
    }

    /*! Создать новый экземпляр модели указанным кодом сущности. Модель будет иметь временный
     * идентификатор */
    template <class T>
    static inline std::shared_ptr<T> createModel(
        //! Идентификатор БД
        const DatabaseID& database_id,
        //! Код сущности
        const EntityCode& entity_code, Error& error)
    {
        ModelPtr m = createModelHelper(database_id, entity_code, error);
        if (error.isError())
            return std::shared_ptr<T>();

        Z_CHECK_NULL(m);
        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(m);
        if (converted == nullptr) {
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(entity_code.value()));
        }
        return converted;
    }
    /*! Создать новый экземпляр модели указанным кодом сущности в БД по умолчанию.
     * Модель будет иметь временный идентификатор */
    template <class T>
    static inline std::shared_ptr<T> createModel(
        //! Код сущности
        const EntityCode& entity_code, Error& error)
    {
        ModelPtr m = createModelHelper(defaultDatabase(), entity_code, error);
        if (error.isError())
            return std::shared_ptr<T>();

        Z_CHECK_NULL(m);
        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(m);
        if (converted == nullptr) {
            Z_HALT(utf("Ошибка преобразования типа для %1").arg(entity_code.value()));
        }
        return converted;
    }

    /*! Создать новый экземпляр модели указанным кодом сущности в БД по умолчанию.
     * Модель будет иметь временный идентификатор */
    template <class T>
    static inline std::shared_ptr<T> createModel(
        //! Код сущности
        const EntityCode& entity_code)
    {
        Error error;
        auto res = createModel<T>(entity_code, error);
        Z_CHECK_ERROR(error);
        return res;
    }

    //! Создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить
        const DataPropertySet& properties,
        //! Ошибка
        Error& error)
    {
        ModelPtr m = detachModelHelper(entity_uid, load_options, properties, error);
        if (error.isError())
            return std::shared_ptr<T>();

        Z_CHECK_NULL(m);
        std::shared_ptr<T> converted = std::dynamic_pointer_cast<T>(m);
        if (converted == nullptr) {
            Z_HALT(utf("casting error для %1").arg(entity_uid.code()));
        }
        return converted;
    }

    //! Создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить
        const PropertyIDSet& properties,
        //! Ошибка
        Error& error)
    {
        DataPropertySet props;
        for (auto& p : properties) {
            props << DataProperty::property(entity_uid, p);
        }

        return detachModel<T>(entity_uid, load_options, props, error);
    }

    //! Создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const DataPropertySet& properties)
    {
        Error error;
        auto m = detachModel<T>(entity_uid, LoadOptions(), properties, error);
        Z_CHECK_ERROR(error);
        return m;
    }

    //! Создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Атрибуты, которые надо загрузить
        const PropertyIDSet& properties)
    {
        Error error;
        auto m = detachModel<T>(entity_uid, LoadOptions(), properties, error);
        Z_CHECK_ERROR(error);
        return m;
    }

    //! Создать копию модели с указанным идентификатором для редактирования. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModel(
        //! Идентификатор сущности
        const Uid& entity_uid)
    {
        return detachModel<T>(entity_uid, DataPropertySet());
    }

    //! Создать копию модели с указанным идентификатором для редактирования со всеми доступными свойствами. Копия будет доступна только
    //! вызывающему. После получения модели, она может быть еще не загружена.
    template <class T>
    static inline std::shared_ptr<T> detachModelFull(
        //! Идентификатор сущности
        const Uid& entity_uid)
    {
        return detachModel<T>(entity_uid, DataStructure::structure(entity_uid)->propertiesMain());
    }

    //! Асинхронно удалить модели из базы данных
    //! Метод сначала загружает модель, чтобы она могла контролировать возможность своего удаления
    static MessageID removeModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы для удаления
        const UidList& entity_uids,
        //! Атрибуты, которые надо загрузить
        const QList<DataPropertySet>& properties,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {},
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user = {});

    //! Асинхронно удалить модели из базы данных
    //! Метод сразу отправляет запрос на удаление серверу без загрузки моделей
    static MessageID removeModels(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы для удаления
        const UidList& entity_uids,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters = {},
        //! Действие пользователя. Количество равно entity_uids или пусто (не определено)
        const QList<bool>& by_user = {});

    //! Асинхронно удалить модели из базы данных
    //! Метод сразу отправляет запрос на удаление серверу без загрузки моделей
    static MessageID removeModel(
        //! Кому отправить ответ
        const I_ObjectExtension* requester,
        //! Идентификаторы для удаления
        const Uid& entity_uid,
        //! Произвольные параметры
        const QMap<QString, QVariant>& parameters = {},
        //! Действие пользователя. Если пусто - не определено
        const QVariant& by_user = QVariant());

    //! Синхронное удаление моделей
    //! Метод сразу отправляет запрос на удаление серверу без загрузки моделей
    static Error removeModelsSync(
        //! Идентификаторы для удаления
        const UidList& entity_uids,
        //! Время ожидания в миллисекундах (0 - бесконечно)
        int timeout_ms = 0);

    //! Синхронное ожидание загрузки моделей из БД
    //! Возвращает ошибки для моделей если они есть. Если была ошибка таймаута, то в QMap будет значение [nullptr, timeout error]
    static QMap<ModelPtr, Error> waitForLoadModel(const UidList& model_uids,
        //! Созданные модели
        QList<ModelPtr>& models,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());

    //! Синхронное ожидание загрузки моделей из БД. Если модель не в состоянии isLoading(), то игнорируется
    //! Возвращает ошибки для моделей если они есть. Если была ошибка таймаута, то в QMap будет значение [nullptr, timeout error]
    static QMap<ModelPtr, Error>
    waitForLoadModel(const QList<ModelPtr>& models,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());
    //! Синхронное ожидание загрузки моделей из БД. Если модель не в состоянии isLoading(), то игнорируется
    static Error waitForLoadModel(const ModelPtr& model,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());

    //! Синхронное ожидание сохранения моделей в БД. Если модель не в состоянии isSaving(), то игнорируется
    //! Возвращает ошибки для моделей если они есть. Если была ошибка таймаута, то в QMap будет значение [nullptr, timeout error]
    static QMap<ModelPtr, Error>
    waitForSaveModel(const QList<ModelPtr>& models,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString(),
        //! Возвращать некритические ошибки сохранения Model::nonCriticalSaveError()
        bool collect_non_critical_error = false);
    //! Синхронное ожидание сохранения моделей в БД. Если модель не в состоянии isSaving(), то игнорируется
    static Error waitForSaveModel(const ModelPtr& model,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());

    //! Синхронное ожидание удаления моделей из БД. Если модель не в состоянии isRemoving(), то игнорируется
    //! Возвращает ошибки для моделей если они есть. Если была ошибка таймаута, то в QMap будет значение [nullptr, timeout error]
    static QMap<ModelPtr, Error>
    waitForRemoveModel(const QList<ModelPtr>& models,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());
    //! Синхронное ожидание удаления моделей в БД. Если модель не в состоянии isRemoving(), то игнорируется
    static Error waitForRemoveModel(const ModelPtr& model,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString());
    //! Синхронное ожидания операций загрузки, удаления или сохранения
    static QMap<ModelPtr, Error> waitForModelOperation(
        //! Модели и операции, которые надо выполнить
        const QList<QPair<
            ModelPtr,
            //! Требуемое действие: ObjectActionType::View (загрузка), ObjectActionType::Remove (удаление), ObjectActionType::Modify (сохранение)
            ObjectActionType>>& models,
        //! Таймаут. Если 0, то бесконечно
        int timeout_ms = 0,
        //! Сообщение для пользователя. Если задано, то будет выведено модальное окно с данным текстом
        const QString& wait_message = QString(),
        //! Возвращать некритические ошибки сохранения Model::nonCriticalSaveError()
        bool collect_non_critical_error = false);

    /*! Создать новый экземпляр view для указанной модели */
    template <class T>
    static inline std::shared_ptr<T> createView(const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Произвольные данные. Для возможности создания разных представлений по запросу
        const QVariant& data,
        //! Ошибка
        Error& error,
        //! Критическая ошибка если представление отсутствует
        bool halt_if_null = true)
    {
        Z_CHECK_NULL(model);

        View* view = createViewHelper(model, states, data, error);
        if (error.isError())
            return std::shared_ptr<T>();

        if (view == nullptr) {
            if (halt_if_null)
                Z_HALT(QString::number(model->entityCode().value()));
            return nullptr;
        }

        T* converted = dynamic_cast<T*>(view);
        if (converted == nullptr) {
            Z_HALT(utf("casting error %1").arg(model->entityUid().code()));
        }

        return std::dynamic_pointer_cast<T>(createViewSharedPointer(view));
    }
    /*! Создать новый экземпляр view для указанной модели
     * Упрощенный метод */
    template <class T>
    static inline std::shared_ptr<T> createView(const ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Критическая ошибка если представление отсутствует
        bool halt_if_null = true)
    {
        return createView<T>(model, {}, {}, error, halt_if_null);
    }
    /*! Создать новый экземпляр view для указанной модели
     * Упрощенный метод. Ошибка вызывает остановку программы */
    template <class T>
    static inline std::shared_ptr<T> createView(const ModelPtr& model)
    {
        zf::Error error;
        auto v = createView<T>(model, {}, {}, error, true);
        Z_CHECK_ERROR(error);
        return v;
    }

    /*! Создать новый экземпляр view для указанной модели
     * Упрощенный метод. Ошибка вызывает остановку программы */
    template <class T>
    static inline std::shared_ptr<T> createView(const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states)
    {
        zf::Error error;
        auto v = createView<T>(model, states, {}, error, true);
        Z_CHECK_ERROR(error);
        return v;
    }

    /*! Создать новый экземпляр view для указанного идентификатора */
    template <class T>
    static inline std::shared_ptr<T> createView(const Uid& entity_uid,
        //! Ошибка
        Error& error,
        //! Критическая ошибка если представление отсутствует
        bool halt_if_null = true,
        //! Грузить все свойства сразу
        bool load_all_properties = true)
    {
        ModelPtr model;
        if (load_all_properties)
            model = getModelFull<Model>(entity_uid, error);
        else
            model = getModel<Model>(entity_uid, error);
        if (error.isError())
            return nullptr;

        return createView<T>(model, error, halt_if_null);
    }

public:
    //! Асинхронно отправить сообщение менеджеру БД (CoreUids::DATABASE_MANAGER)
    static bool postDatabaseMessage(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Сообщение
        const Message& message);
    /*! Синхронно отправить сообщение менеджеру БД (CoreUids::DATABASE_MANAGER) и получить ответ.
     * Если ответ не получен, то результат isValid() == false */
    static Message sendDatabaseMessage(
        //! Сообщение
        const Message& message,
        //! Время ожидания ответа. По умолчанию - бесконечно
        int timeout_ms = 0);

public:
    //! Разослать уведомление об изменении сущностей
    void entityChanged(const Uid& sender, const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});
    //! Разослать уведомление об изменении сущностей
    void entityChanged(const Uid& sender, const Uid& entity_uid);
    //! Разослать уведомление об изменении сущностей
    void entityChanged(const Uid& sender, const Uid& entity_uid,
        //! Действие пользователя
        bool by_user);
    //! Разослать уведомление об изменении сущностей по всем загруженным моделям указанного вида
    void entityChanged(const Uid& sender, const EntityCode& entity_code);

    //! Разослать уведомление об удалении сущностей
    void entityRemoved(const Uid& sender, const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});
    //! Разослать уведомление об удалении сущностей
    void entityRemoved(const Uid& sender, const Uid& entity_uid);
    //! Разослать уведомление об удалении сущностей
    void entityRemoved(const Uid& sender, const Uid& entity_uid,
        //! Действие пользователя
        bool by_user);

    //! Разослать уведомление о создании сущностей
    void entityCreated(const Uid& sender, const UidList& entity_uids,
        //! Действие пользователя. Количество равно entityUids или нулю
        const QList<bool>& by_user = {});
    //! Разослать уведомление о создании сущностей
    void entityCreated(const Uid& sender, const Uid& entity_uid);
    //! Разослать уведомление о создании сущностей
    void entityCreated(const Uid& sender, const Uid& entity_uid,
        //! Действие пользователя
        bool by_user);

public:
    //! Асинхронный метод запроса данных из серверной таблицы с использованием ограничений
    //! Назад придет ответ либо zf::DBEventDataTableMessage, либо zf::ErrorMessage
    static MessageID getDbTableAsync(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Код БД
        const DatabaseID& database_id,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});
    //! Асинхронный метод запроса данных из серверной таблицы с использованием ограничений
    //! Назад придет ответ либо zf::DBEventDataTableMessage, либо zf::ErrorMessage
    static MessageID getDbTableAsync(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {});

    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием ограничений
    static DataTablePtr getDbTable(
        //! Ошибка
        Error& error,
        //! Код БД
        const DatabaseID& database_id,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);
    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием ограничений
    static DataTablePtr getDbTable(Error& error,
        //! Серверный код таблицы
        const TableID& table_id,
        //! Ограничения
        DataRestrictionPtr restriction = nullptr,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);

    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием отбора по ключам
    static DataTablePtr getDbTable(
        //! Ошибка
        Error& error,
        //! Код БД
        const DatabaseID& database_id,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Набор ключей
        const QList<int>& keys,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);
    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием отбора по ключам
    static DataTablePtr getDbTable(
        //! Ошибка
        Error& error,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Набор ключей
        const QList<int>& keys,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);

    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием отбора по одному ключу
    static DataTablePtr getDbTable(
        //! Ошибка
        Error& error,
        //! Код БД
        const DatabaseID& database_id,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Ключ
        int key,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);
    //! Упрощенный синхронный метод запроса данных из серверной таблицы.
    //! С использованием отбора по одному ключу
    static DataTablePtr getDbTable(
        //! Ошибка
        Error& error,
        //! Код колонки для поиска
        const FieldID& field_id,
        //! Ключ
        int key,
        //! Маска. Если не пусто, то только эти колонки
        const FieldIdList& mask = {},
        //! Таймаут. По умолчанию бесконечно
        int timeout_ms = 0);

public:
    /*! Получить значение из набора данных по ключевому значению.
     * Возвращает false, если данные еще не загружены. В этом случает result не инициализирован */
    static bool getEntityValue(
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
        //! загружены
        const QWidgetList& update_on_data_loaded = QWidgetList(),
        //! Роль для выборки result
        int result_role = Qt::DisplayRole);
    /*! Получить значение из набора данных по ключевому значению. Упрощенный метод со значениями по умолчанию.
     * Возвращает false, если данные еще не загружены. В этом случает result не инициализирован */
    static bool getEntityValue(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Значение из колонки result_column. Инициализировано, если функция вернула true
        QVariant& result,
        //! Модель, сформированная на основании source_uid
        ModelPtr& source_model,
        //! Ошибка при попытке загрузить модель с данными
        Error& error);
    /*! Получить значение из набора данных по ключевому значению. Данные о колонках извлекаются из lookup
     * Возвращает false, если данные еще не загружены. В этом случает result не инициализирован */
    static bool getEntityValue(
        //! lookup для выборки
        const PropertyLookupPtr& lookup,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Значение из колонки result_column. Инициализировано, если функция вернула true
        QVariant& result,
        //! Модель, сформированная на основании source_uid
        ModelPtr& source_model,
        //! Ошибка при попытке загрузить модель с данными
        Error& error,
        //! Список виджетов, которые должны быть обновлены, после окончания загрузки, если на момент запроса данные не
        //! загружены
        const QWidgetList& update_on_data_loaded = QWidgetList());
    /*! Получить значение из набора данных по ключевому значению. Совсем упрощенный метод со значениями по умолчанию.
     * Возвращает QVariant(), если данные еще не загружены */
    static QVariant getEntityValue(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Модель, сформированная на основании source_uid
        ModelPtr& source_model);
    /*! Получить значение из набора данных по ключевому значению. Совсем упрощенный метод со значениями по умолчанию.
     * Возвращает QVariant(), если данные еще не загружены */
    static QVariant getEntityValue(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value);

    /*! Получить значение из набора данных по ключевому значению. Совсем упрощенный метод со значениями по умолчанию.
     * Если данные еще не загружены - ждет */
    static QVariant getEntityValueSync(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Время ожидания в миллисекундах (0 - бесконечно)
        int timeout_ms = 0);
    /*! Получить значение из набора данных по ключевому значению.
     * Возвращает false, если данные еще не загружены. В этом случает result не инициализирован */
    static QVariant getEntityValueSync(
        //! Идентификатор сущности, в которой идет поиск
        const Uid& source_uid,
        //! Ключевое значение по которому ищется строка в dataset_property
        const QVariant& key_value,
        //! Ошибка при попытке загрузить модель с данными
        Error& error,
        //! Набор данных. Если у сущности всего один набор данных, то может быть не задан
        const PropertyID& dataset_property = PropertyID(),
        //! Колонка, по которой идет поиск. Если не задана, то набор данных должен иметь колонку с PropertyOption::Id
        const PropertyID& key_column = PropertyID(),
        //! Колонка из которой извлекается результат. Если не задана, то набор данных должен иметь колонку с
        //! PropertyOption::Name
        const PropertyID& result_column = PropertyID(),
        //! Роль для выборки result
        int result_role = Qt::DisplayRole);
    //! Движок JavaScript
    static QJSEngine* jsEngine();

public:
    //! Глобальная пользовательская конфигурация приложения (информация хранится на компьютере пользователя)
    static Configuration* configuration(int version = 0);
    //! Конфигурация по коду сущности (информация хранится на компьютере пользователя)
    static Configuration* entityConfiguration(
        //! Код сущности
        const EntityCode& entity_code,
        //! Произвольный код
        const QString& key = QString(),
        //! Версия
        int version = 0);

    //! Последняя использованная папка
    static QString lastUsedDirectory();
    static void updateLastUsedDirectory(const QString& folder);

    //! Диалог с вопросом
    static bool ask(const QString& text, InformationType type = InformationType::Information, const QString& detail = QString(),
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    static bool ask(const QString& text, const Error& detail,
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    static bool ask(const Error& error, InformationType type = InformationType::Error);

    //! Диалог выбора из трех вариантов
    static QMessageBox::StandardButton choice(const QString& text, QMessageBox::StandardButton button1,
        QMessageBox::StandardButton button2, QMessageBox::StandardButton button3,
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    //! Выбор из Yes/No/Cancel
    static QMessageBox::StandardButton choice(const QString& text,
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());

    //! Диалог с информацией
    static void info(const QString& text, const QString& detail = QString(),
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    static void info(const Log& log);

    //! Диалог с предупреждением
    static void warning(const QString& text, const QString& detail = QString(),
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    static void warning(const Log& log);

    //! Диалог с ошибкой
    static void error(const QString& text, const QString& detail = QString(),
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    //! Диалог с ошибкой
    static void error(const QString& text, const Error& detail,
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    //! Диалог с ошибкой
    static void error(const QString& text, const ErrorList& detail,
        //! Идентификатор модели для открытия
        const Uid& entity_uid = Uid());
    //! Диалог с ошибкой
    static void error(const Error& err);
    //! Диалог с ошибкой
    static void error(const ErrorList& err,
        //! Ищет в списке ошибок первую с признаком isMain и делает ее главной. Иначе делает главной первую в списке
        bool auto_main = false);
    //! Диалог с ошибкой в виде журнала
    static void error(const Log& log);
    //! Если передана ошибка, то отобразить. Иначе проигнорировать
    static void errorIf(const Error& err);
    //! Если передана ошибка, то отобразить. Иначе проигнорировать
    static void errorIf(const ErrorList& err,
        //! Ищет в списке ошибок первую с признаком isMain и делает ее главной. Иначе делает главной первую в списке
        bool auto_main = false);

    //! Диалог с критической ошибкой
    static void systemError(const QString& text, const QString& detail = QString());
    //! Диалог с критической ошибкой
    static void systemError(const Error& error, const QString& detail = QString());

    //! Запуск диалога с вопросом и выполнением синхронной функции при подтверждении
    //! Возвращает true, если на вопрос ответили ДА. При ошибке выполнения функции тоже возвращает true!
    static bool askFunction(
        //! Текст вопроса. Обязательно
        const QString& question,
        //! Текст ожидания. Обязательно
        const QString& wait_info,
        //! Синхронная функция
        SyncAskDialogFunction sync_function,
        //! Ошибка по итогам вызова функции. Помимо того, что она возвращается тут - сообщение об ошибке показывается пользователю при show_error true
        Error& error,
        //! Текст с положительным результатом. Если задано, то будет показан по итогам вызова функции
        const QString& result_info = {},
        //! Показывать ошибку по итогам выполнения функции
        bool show_error = false,
        //! Запускать функцию в отдельном потоке. Имеет смысл использовать, если функция не использует QEventLoop и может подвесить UI
        bool run_in_thread = true,
        //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
        const zf::Uid& progress_uid = {});
    //! Запуск диалога с выполнением синхронной функции. Если была ошибка, то она отобразится и потом вернется на выход
    static Error waitFunction(
        //! Текст ожидания. Обязательно
        const QString& wait_info,
        //! Синхронная функция
        SyncAskDialogFunction sync_function,
        //! Текст с положительным результатом. Если задано, то будет показан по итогам вызова функции при show_error true
        const QString& result_info = {},
        //! Показывать ошибку по итогам выполнения функции
        bool show_error = false,
        //! Запускать функцию в отдельном потоке. Имеет смысл использовать, если функция не использует QEventLoop и может подвесить UI
        bool run_in_thread = true,
        //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
        const zf::Uid& progress_uid = {});
    //! Создает окно ожидания. Окно не модальное (метод завершается сразу), но обладает модальным поведением (блокирует все действия пользователя)
    //! За удаление отвечает вызывающий
    static SyncWaitWindow* createSyncWaitWindow(const QString& text,
        //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
        const zf::Uid& progress_uid = {});

    /*! Не отображать ошибку сразу. Вместо этого положить ее в буфер и отобразить при первом входе в eventloop
     * Использовать при необходимости отобразить ошибку внутри обрабочиков, критичных к скорости выхода из метода или
     * в потоках, отличных от главного. Несколько ошибок отправленных подряд, будут показаны совместно */
    static void postError(const Error& error);
    /*! Не отображать ошибку сразу. Вместо этого положить ее в буфер и отобразить при первом входе в eventloop
     * Использовать при необходимости отобразить ошибку внутри обрабочиков, критичных к скорости выхода из метода или
     * в потоках, отличных от главного. Несколько ошибок отправленных подряд, будут показаны совместно */
    static void postError(const QString& error);
    /*! Не отображать сообщение сразу. Вместо этого положить его в буфер и отобразить при первом входе в eventloop
     * Использовать при необходимости отобразить сообщение внутри обрабочиков, критичных к скорости выхода из метода или
     * в потоках, отличных от главного. Несколько сообщений отправленных подряд, будут показаны совместно */
    static void postInfo(const QString& info);
    /*! Для показа обширной информации. Не отображать сообщение сразу. Вместо этого положить его в буфер и отобразить при первом входе в eventloop
     * Использовать при необходимости отобразить сообщение внутри обрабочиков, критичных к скорости выхода из метода или
     * в потоках, отличных от главного. Несколько сообщений отправленных подряд, будут показаны совместно */
    static void postNote(const QString& info);

    //! Диалог с обширной информацией
    static void note(const QString& text,
        //! Заголовок
        const QString& caption = QString());

    //! Запросить произвольные параметры
    static bool askParameters(
        //! Идентификатор БД
        const zf::DatabaseID& database_id,
        //! Заголовок окна
        const QString& caption,
        //! Информационный текст
        const QString& text,
        //! Данные для заполнения
        const DataContainerPtr& data);

    //! Диалог OK/Cancel с одним полем ввода
    static bool askText(
        //! Называние поля ввода
        const QString& name,
        //! Требовать ли ввод значения
        bool value_required,
        //! Значение
        QString& value,
        //! Скрывать ли ввод (например для паролей)
        bool hide = false,
        //! Текст при отсутствии значения в поле ввода
        const QString& placeholder = {},
        //! Маска ввода
        const QString& input_mask = {},
        //! Заголовок окна
        const QString& caption = {});

    //! Журналирование информационное
    static void logInfo(const QString& text, const QString& detail = QString(),
        //! Убрать из текста html форматирование
        bool plain_text = true);
    //! Журналирование ошибок
    static void logError(const QString& text, const QString& detail = QString(),
        //! Убрать из текста html форматирование
        bool plain_text = true);
    static void logError(const Error& error,
        //! Убрать из текста html форматирование
        bool plain_text = true);
    //! Журналирование ошибок
    static void logCriticalError(const QString& text, const QString& detail = QString(),
        //! Убрать из текста html форматирование
        bool plain_text = true);
    static void logCriticalError(const Error& error,
        //! Убрать из текста html форматирование
        bool plain_text = true);

    //! Виджет не будет выбираться в качестве parent при создании Dialog и его наследников
    static void registerNonParentWidget(QWidget* w);

    //! Framework (для внутреннего использования в ядре)
    static Framework* fr();
    //! История последних операций менеджера моделей для отладки
    static const QContiguousCache<QPair<SharedObjectEventType, Uid>>& modelManagerHistory();
    //! Записать информацию в журнал приложения
    static Error writeToLogStorage(const QString& text, zf::InformationType type);

    //! Инициализирована ли переменная Framework (для внутреннего использования в ядре)
    static bool isCoreInitialized();

public:
    /*! Вывести информацию об объекте в файл. По умолчанию выводятся все свойства, зарегистрированные через Q_PROPERTY.
     * Если требуется собственная реализация алгоритма вывода для класса, то необходимо:
     *  1. Создать обработчик: QDebug operator<<(QDebug debug, const MyClass* c);
     *  2. Зарегистрировать обработчик QMetaType::registerDebugStreamOperator<MyClass*>(); */
    static Error debPrint(const QObject* object,
        //! Куда сохранить файл
        QStandardPaths::StandardLocation location,
        //! Открывать ли файл после вывода
        bool open = true);
    /*! Вывести информацию об объекте в файл. По умолчанию выводятся все свойства, зарегистрированные через Q_PROPERTY.
     * Если требуется собственная реализация алгоритма вывода для класса, то необходимо:
     *  1. Создать обработчик: QDebug operator<<(QDebug debug, const MyClass* c);
     *  2. Зарегистрировать обработчик QMetaType::registerDebugStreamOperator<MyClass*>(); */
    static Error debPrint(const QObject* object,
        //! Куда сохранить файл
        const QDir& folder,
        //! Открывать ли файл после вывода
        bool open = true);
    /*! Вывести информацию об объекте в файл. По умолчанию выводятся все свойства, зарегистрированные через Q_PROPERTY.
     * Если требуется собственная реализация алгоритма вывода для класса, то необходимо:
     *  1. Создать обработчик: QDebug operator<<(QDebug debug, const MyClass* c);
     *  2. Зарегистрировать обработчик QMetaType::registerDebugStreamOperator<MyClass*>(); */
    static Error debPrint(const QObject* object);

    //! Вывести информацию о QVariant в файл
    static Error debPrint(const QVariant& value, const QDir& folder, bool open = true);
    //! Вывести информацию об объекте в файл
    static Error debPrint(const QVariant& value, QStandardPaths::StandardLocation location, bool open = true);
    //! Вывести информацию о QVariant в файл
    static Error debPrint(const QVariant& value);

    //! Установить каталог по умолчанию, куда будет выводится информация через debPrint
    static void setDefaultDebPrintLocation(QStandardPaths::StandardLocation location);
    //! Установить каталог по умолчанию, куда будет выводится информация через debPrint
    static void setDefaultDebPrintLocation(const QDir& folder);

    //! Начало вывода объекта в debug
    static void beginDebugOutput();
    //! Окончание вывода объекта в debug
    static void endDebugOutput();
    //! Текущий уровень вложенности вывода объектов в debug
    static int debugLevel();
    //! Текущий уровень вложенности вывода объектов в debug в виде строки сдвига
    static QString debugIndent();

    //! Получить прокси на основе заданных парамеров
    static QNetworkProxy getProxy(const QUrl& url = {});
    static QNetworkProxy getProxy(const QString& url);
    //! Задать параметры прокси
    static void setProxySettings(
        //! Использовать ли прокси
        bool use,
        //! Логин и пароль
        const Credentials& сredentials,
        //! Адрес для проверки работы прокси
        const QUrl& default_url);

private:
    //! Журналирование
    static void log(InformationType type, const QString& text, const QString& detail, bool toPlainText);

    static ModelPtr createModelHelper(const DatabaseID& database_id, const EntityCode& entity_code, Error& error);
    static ModelPtr detachModelHelper(const Uid& entity_uid, const LoadOptions& load_options, const DataPropertySet& properties,
        Error& error);

    static I_Plugin* getPluginHelper(const EntityCode& entity_code, Error& error);

    static View* createViewHelper(const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Произвольные данные. Для возможности создания разных представлений по запросу
        const QVariant& data, Error& error);

    static void logDialogHelper(const Log& log, const QIcon& icon);

    //! Создание shared_ptr для представлений
    static ViewPtr createViewSharedPointer(View* view);

    static ObjectExtensionPtr<Framework> _framework;
    static ObjectExtensionPtr<ModelManager> _model_manager;
    static std::unique_ptr<ModuleManager> _module_manager;
    static ObjectExtensionPtr<DatabaseManager> _database_manager;
    static ObjectExtensionPtr<MessageDispatcher> _message_dispatcher;
    static ObjectExtensionPtr<ModelKeeper> _model_keeper;
    static ObjectExtensionPtr<ExternalRequester> _external_requester;
    static ObjectExtensionPtr<ProgressObject> _progress;
    static std::unique_ptr<CumulativeError> _cumulative_error;

    //! Ревизия
    static QString _build_version;
    //! Версия ПО
    static Version _application_version;
    //! Идентификатор БД по умолчанию
    static DatabaseID _default_database_id;
    //! Список БД
    static std::unique_ptr<QHash<DatabaseID, DatabaseInformation>> _database_info;
    //! Последняя использованная папка
    static QString _lastUsedDirectory;
    //! Интерфейс по работе с каталогами
    static I_Catalogs* _catalogs;
    //! Интерфейс генерации ограничений DataRestriction
    static I_DataRestriction* _restriction;

    //! Была ли начальная инициализация
    static QAtomicInt _is_bootstraped;

    //! Режим работы ядра
    static CoreModes _mode;
    //! Ключ ядра - строка, зависящая от приложения, котрое использует ядро
    static QString _core_instance_key;
    //! Каталог по умолчанию, куда будет выводится информация через debPrint
    static QString _default_debprint_location;

    //! Логин текушего пользователя
    static QString _current_user_login;

    //! Изменения
    static QString _changelog;

    //! Параметры прокси
    static bool _use_proxy;
    static Credentials _proxy_credentials;
    static QUrl _default_url;
    static QMutex _proxy_mutex;
};
} // namespace zf

//! Для совместимости версий Qt
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
inline QDataStream& operator<<(QDataStream& out, const QLocale::Language& obj)
{
    return out << static_cast<int>(obj);
}
inline QDataStream& operator>>(QDataStream& in, QLocale::Language& obj)
{
    return in >> reinterpret_cast<int&>(obj);
}
#endif
