#pragma once

#include "zf_command_processor.h"
#include "zf_database_messages.h"
#include "zf_entity_object.h"
#include "zf_message.h"
#include "zf_message_processor.h"
#include "zf_property_table.h"

#include <QEventLoop>
#include <QQueue>

namespace zf
{
class ZCORESHARED_EXPORT Model : public EntityObject
{
    Q_OBJECT
public:
    //! Структура данных определяется на основании плагина
    Model(
        //! Уникальный идентификатор сущности
        const Uid& entity_uid,
        //! Является ли отсоединенной. Такие объекты предназначены для редактирования и не доступны для
        //! получения через менеджер моделей
        bool is_detached,
        //! Параметры работы с БД
        const DatabaseObjectOptions& database_options = {DatabaseObjectOption::DataChangeBroadcasting},
        //! Параметры ModuleDataOption
        const ModuleDataOptions& data_options = ModuleDataOption::HighlightEnabled | ModuleDataOption::SimpleHighlight);
    //! Структура данных передается в конструктор
    Model(
        //! Уникальный идентификатор сущности
        const Uid& entity_uid,
        //! Является ли отсоединенной. Такие объекты предназначены для редактирования и не доступны для
        //! получения через менеджер моделей
        bool is_detached,
        //! Структура данных
        const DataStructurePtr& data_structure,
        //! Параметры работы с БД
        const DatabaseObjectOptions& database_options = {},
        //! Параметры
        const ModuleDataOptions& data_options = ModuleDataOption::HighlightEnabled | ModuleDataOption::SimpleHighlight);
    ~Model() override;

    //! Параметры работы с БД
    DatabaseObjectOptions databaseOptions() const;

    //! Постоянный идентификатор сущности (если был присвоен)
    Uid persistentUid() const;
    //! Целочисленный идентификатор в рамках типа сущности. Можно вызывать только если имеется persistentUid и сущность не является уникальной
    int id() const;
    //! Является ли модель отсоединенной. Такие объекты предназначены для редактирования и не доступны для получения
    //! через менеджер моделей
    bool isDetached() const;

    //! Если при сохранении возникла ошибка, при которой сохранение выполнено, но что-то еще пошло нет так, то она будет тут
    //! Стирается после каждого сохранения
    Error nonCriticalSaveError() const;

    //! Данные модели до начала ее изменения. Полностью перезаписываются после загрузки модели из БД
    //! Инициализировано только если isDetached() == true
    DataContainerPtr originalData() const;

    //! Есть ли несохраненные данные
    bool hasUnsavedData(
        //! Ошибка
        Error& error,
        //! Какие свойства игнорировать при приверка. Всегда игнорируются DBWriteIgnored
        const DataPropertyList& ignored_properties = {},
        //! Правило проверки изменения бинарных полей
        DataContainer::ChangedBinaryMethod changed_binary_method = DataContainer::ChangedBinaryMethod::Ignore) const;
    //! Есть ли несохраненные данные
    bool hasUnsavedData(
        //! Ошибка
        Error& error,
        //! Какие свойства игнорировать при приверка. Всегда игнорируются DBWriteIgnored
        const PropertyIDList& ignored_properties,
        //! Правило проверки изменения бинарных полей
        DataContainer::ChangedBinaryMethod changed_binary_method = DataContainer::ChangedBinaryMethod::Ignore) const;

    //! Есть ли несохраненные данные
    bool hasUnsavedData(
        //! Какие свойства игнорировать при приверка. Всегда игнорируются DBWriteIgnored
        const DataPropertyList& ignored_properties = {},
        //! Правило проверки изменения бинарных полей
        DataContainer::ChangedBinaryMethod changed_binary_method = DataContainer::ChangedBinaryMethod::Ignore) const;
    //! Есть ли несохраненные данные
    bool hasUnsavedData(
        //! Какие свойства игнорировать при приверка. Всегда игнорируются DBWriteIgnored
        const PropertyIDList& ignored_properties,
        //! Правило проверки изменения бинарных полей
        DataContainer::ChangedBinaryMethod changed_binary_method = DataContainer::ChangedBinaryMethod::Ignore) const;

    //! Пометить данные как удаленные. Возвращает истину если состояние invalidate поменялось хотя бы у одного свойства
    bool invalidate(const ChangeInfo& info = {}) const override;

    //! Результат запуска команды загрузки, сохранения или удаления
    enum class CommandExecuteResult
    {
        Undefined,
        //! Выполнение команды не требуется (она уже выполняется или выполнение вообще не требуется
        Ignored,
        //! Такая команда уже есть в очереди и ее возможно объеденить с новой командой
        //! При этом вместо двух команд будет сформирована новая, включающая информацию из двух предыдущих,
        //! но в итоге в очереди как была одна команда так и останется
        Merged,
        //! Команда добавлена в очередь на выполнение
        Added,
    };

    //! Асинхронно и принудительно загрузить указанные свойства из БД, даже если они не помечены как invalidate
    //! Возвращает истину, если загрузка требуется и будет произведена
    //! По окончании загрузки генерирует сигнал sg_finishLoad
    CommandExecuteResult reload(
        //! Параметры загрузки
        const LoadOptions& load_options = LoadOptions(),
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const DataPropertySet& properties = DataPropertySet());
    CommandExecuteResult reload(
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const PropertyIDSet& properties);

    //! Асинхронно загрузить из БД. Возвращает истину, если загрузка требуется и будет произведена
    //! По окончании загрузки генерирует сигнал sg_finishLoad
    CommandExecuteResult load(
        //! Параметры загрузки
        const LoadOptions& load_options = LoadOptions(),
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const DataPropertySet& properties = DataPropertySet());
    CommandExecuteResult load(
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const PropertyIDSet& properties);
    CommandExecuteResult load(
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const DataPropertySet& properties);

    //! Синхронно загрузить из БД
    Error loadSync(
        //! Параметры загрузки
        const LoadOptions& load_options = LoadOptions(),
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const DataPropertySet& properties = DataPropertySet(),
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0);
    Error loadSync(
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const PropertyIDSet& properties,
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0);

    //! В процессе загрузки
    bool isLoading(
        //! Если истина, то учитывается не только выполнение операции загрузки, но и наличие этой операции в очереди на выполнение
        bool is_waiting = true) const;
    //! В отличие от isLoading проверяет не только сам факт нахождения в процессе загрузки, но и завершение всех операций после загрузки
    //! Таких как вызов sg_finishLoad и т.п. По хорошему так себя должна вести isLoading, но для совместимости со старым кодом мы ее не трогаем
    bool isLoadingComplete() const;

    //! Отправить стандартную команду на загрузку данных модели (отправка через postMessageCommand)
    bool postLoadMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Какие свойства надо сохранять
        const DataPropertySet& properties) const;
    //! Отправить стандартную команду на загрузку данных модели (отправка через postMessageCommand)
    bool postLoadMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Какие свойства надо сохранять
        const PropertyIDSet& properties) const;

    //! Асинхронно сохранить в БД
    //! ВАЖНО: Объект с временным ID (isTemporary) при каждом сохранении будет создавать в БД объект с новым постоянным
    //! ID, поэтому такие объекты надо сохранять только один раз
    //! По окончании сохранения генерирует сигнал sg_finishSave
    CommandExecuteResult save(
        //! Атрибуты, которые надо сохранить. Должны быть подмножеством загруженных атрибутов. Если пусто, то все
        //! загруженные
        const DataPropertySet& properties,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = QVariant());
    //! Асинхронно сохранить в БД
    //! ВАЖНО: Объект с временным ID (isTemporary) при каждом сохранении будет создавать в БД объект с новым постоянным
    //! ID, поэтому такие объекты надо сохранять только один раз
    //! По окончании сохранения генерирует сигнал sg_finishSave
    CommandExecuteResult save(
        //! Атрибуты, которые надо сохранить. Должны быть подмножеством загруженных атрибутов. Если пусто, то все
        //! загруженные
        const PropertyIDSet& properties,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = QVariant());
    //! Асинхронно сохранить в БД
    //! ВАЖНО: Объект с временным ID (isTemporary) при каждом сохранении будет создавать в БД объект с новым постоянным
    //! ID, поэтому такие объекты надо сохранять только один раз
    //! По окончании сохранения генерирует сигнал sg_finishSave
    //! Сохраняет все свойства (атрибуты)
    CommandExecuteResult save(
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = QVariant());

    //! Синхронно Сохранить в БД
    Error saveSync(
        //! Атрибуты, которые надо сохранить. Должны быть подмножеством загруженных атрибутов. Если пусто, то все
        //! загруженные
        const DataPropertySet& properties,
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект уже
         * имеет постоянный идентификатор, то возвращается он */
        Uid& persistent_uid,
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = {});
    //! Синхронно Сохранить в БД
    Error saveSync(
        //! Атрибуты, которые надо сохранить. Должны быть подмножеством загруженных атрибутов. Если пусто, то все
        //! загруженные
        const PropertyIDSet& properties,
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект уже
         * имеет постоянный идентификатор, то возвращается он */
        Uid& persistent_uid,
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = {});
    //! Синхронно Сохранить в БД. Сохраняет все свойства
    Error saveSync(
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект уже
         * имеет постоянный идентификатор, то возвращается он */
        Uid& persistent_uid,
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = {});
    //! Синхронно Сохранить в БД. Сохраняет все свойства. Использовать если не нужно получать persistent_uid
    Error saveSync(
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Действие пользователя. true, false или пусто (неопределено)
        const QVariant& by_user = {});

    //! В процессе сохранения
    bool isSaving(
        //! Если истина, то учитывается не только выполнение операции сохранения, но и наличие этой операции в очереди на выполнение
        bool is_waiting = true) const;

    //! Отправить стандартную команду на сохранение данных модели (отправка через postMessageCommand)
    bool postSaveMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Какие свойства надо сохранять
        const DataPropertySet& properties,
        //! Действие пользователя
        const QVariant& by_user = false) const;
    //! Отправить стандартную команду на сохранение данных модели (отправка через postMessageCommand)
    bool postSaveMessageCommand(
        //! Ключ сообщения
        const Command& message_key,
        //! Какие свойства надо сохранять
        const PropertyIDSet& properties,
        //! Действие пользователя
        const QVariant& by_user = false) const;

    //! Асинхронно Удалить из БД
    CommandExecuteResult remove(
        //! Действие пользователя
        const QVariant& by_user = false);
    //! Синхронно удалить из БД
    Error removeSync(
        //! Ожидание в мс. По умолчанию - бесконечно
        int timeout_ms = 0,
        //! Действие пользователя
        const QVariant& by_user = false);
    //! В процессе удаления
    bool isRemoving(
        //! Если истина, то учитывается не только выполнение операции удаления, но и наличие этой операции в очереди на выполнение
        bool is_waiting = true) const;

    //! Выполнить операцию
    Error executeOperation(const Operation& op);
    Error executeOperation(const OperationID& operation_id);

    //! Запросить обновление виджета по окончании загрузки
    void requestWidgetUpdate(QWidget* widget);
    void requestWidgetUpdate(const QWidgetList& widgets);

    //! Клонировать модель со всеми данными
    ModelPtr clone( //! Если не задано, то новая модель будет с временным идентификатором
        const Uid& new_uid = {},
        //! Сохранение признака detach для detach-модели
        bool keep_detached = false) const;
    //! Клонировать модель со всеми данными. Новая модель будет с временным идентификатором
    template <class T>
    std::shared_ptr<T> clone(
        //! Если не задано, то новая модель будет с временным идентификатором
        const Uid& new_uid = {},
        //! Сохранение признака detach для detach-модели
        bool keep_detached = false)
    {
        auto m = clone(new_uid, keep_detached);
        auto res = std::dynamic_pointer_cast<T>(m);
        Z_CHECK_NULL(res);
        return res;
    }

    //! Прямые права доступа к данной сущности
    AccessRights directAccessRights() const override;
    //! Косвенные права доступа к данному типу сущности
    AccessRights relationAccessRights() const override;

    //! Имя сущности
    QString entityName() const final;

    //! Модель не будет удалена при падении на нее ссылок до нуля
    bool isKeeped() const;

    using EntityObject::copyFrom;
    void copyFrom(const ModuleDataObject* source, bool save_original_data);

protected:    
    //! Скопировать данные из source. Копирование доступно только для ModuleDataObject одного типа
    void doCopyFrom(const ModuleDataObject* source) override;

    //! Обработчик операции для OperationScope::Entity. Вызов происходит если операция не обработана во view
    virtual Error processOperation(const Operation& op);

    //! Вызывается ядром до загрузки из БД. Метод синхронный
    virtual Error beforeLoad(
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Набор свойств, которые надо загрузить. Если не задано, то все
        const DataPropertySet& properties);
    //! Вызывается ядром после загрузки из БД. Метод синхронный
    //! Необходимо вернуть входящую ошибку и при необходимости дополнить ее своей
    virtual Error afterLoad(const LoadOptions& load_options, const DataPropertySet& properties,
        //! Если при загрузке была ошибка, то она будет здесь
        const Error& error);

    //! Вызывается ядром до сохранения в БД. Метод синхронный
    virtual Error beforeSave(
        //! Свойства, которые сохраняются
        const DataPropertySet& properties);
    //! Вызывается ядром после сохранения в БД. Метод синхронный
    virtual void afterSave(
        //! Какие свойства сохранялись
        const zf::DataPropertySet& requested_properties,
        //! Какие свойства сохранялись фактически. Перечень не может быть шире чем requested_properties, т.к. сервер может сохранять свойства группами
        const zf::DataPropertySet& saved_properties,
        //! Если при сохранении была ошибка, то она будет здесь
        Error& error);

    //! Вызывается ядром до удаления из БД. Метод синхронный
    virtual Error beforeRemove();
    //! Вызывается ядром после удаления из БД. Метод синхронный
    virtual void afterRemove(
        //! Если при удалении была ошибка, то она будет здесь
        Error& error);

    //! Вызывается ядром при необходимости загрузить конфигурацию сущности. Каталог для хранения файлов: moduleDataLocation
    virtual Error onLoadConfiguration();
    //! Вызывается ядром при необходимости сохранить конфигурацию сущности. Каталог для хранения файлов: moduleDataLocation
    virtual Error onSaveConfiguration() const;

    //! Запрет менеджеру моделейц на удаление этой модели при падении на нее ссылок до нуля
    //! ВЫЗЫВАТЬ ТОЛЬКО В КОНСТРУКТОРЕ
    void keep();

protected:
    //! Дополнить запрашиваемые к загрузке свойства
    virtual DataPropertySet extendLoadProperties(const DataPropertySet& requested);

    //! Переопределять при необходимости собственной реализации загрузки из БД
    //! (DatabaseObjectOption::CustomLoad, DatabaseObjectOption::StandardLoadExtension)
    virtual Error onStartCustomLoad(
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Набор свойств, которые надо загрузить. Если не задано, то все
        const DataPropertySet& properties);
    //! Вызывать кода закончена собственная реализация загрузки
    void finishCustomLoad(
        //! Ошибка загрузки (если была)
        const Error& error,
        //! Прямые права доступа
        const AccessRights& direct_rights = AccessRights(AccessFlag::Administrate),
        //! Косвенные права доступа (на связаные объекты)
        const AccessRights& relation_rights = AccessRights(AccessFlag::Administrate)) const;

    //! Переопределять при необходимости собственной реализации сохранения в БД
    //! (DatabaseObjectOption::CustomSave)
    virtual Error onStartCustomSave(
        //! Набор свойств, которые надо сохранить. Если не задано, то все
        const DataPropertySet& properties);
    //! Вызывать кода закончена собственная реализация сохранения
    void finishCustomSave(
        //! В случае ошибки указать ее тут
        const Error& error,
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект уже
         * имеет постоянный идентификатор, то указать пустой. При наличии ошибки - не задавать!
         * Если используется DatabaseObjectOption::StandardSaveExtension, то тоже не задавать */
        const Uid& persistent_uid,
        //! Некритическая ошибка. Означает что сохранение выполнено, но что-то еще пошло не так
        const Error& non_critical_error = {}) const;

    //! Переопределять при необходимости собственной реализации удаления из БД
    //! (DatabaseObjectOption::CustomRemove)
    virtual Error onStartCustomRemove();
    //! Вызывать кода закончена собственная реализация удаления
    void finishCustomRemove(const Error& error) const;

    //! Вызывается при необходимости сгенерировать сигнал об ошибке
    void registerError(const Error& error, ObjectActionType type = ObjectActionType::Undefined) final;

public:
    //! Метод вызывается перед отправкой сообщения DBCommandGetEntityMessage для стандартной загрузки объекта
    //! (когда не используется CustomLoad). Необходимо заполнить произвольный набор параметров, который должен понимать драйвер
    //! Параметры будут переданы в конструктор DBCommandGetEntityMessage
    virtual void getLoadParameters(QMap<QString, QVariant>& parameters) const;
    //! Задать параметры, возвращаемые методом getLoadParameters (если он не был переопределен)
    void setLoadParameters(const QMap<QString, QVariant>& parameters);
    //! Метод вызывается перед отправкой сообщения DBCommandWriteEntityMessage для стандартного сохранения объекта
    //! (когда не используется CustomSave). Необходимо заполнить произвольный набор параметров, который должен понимать драйвер
    //! Параметры будут переданы в конструктор DBCommandWriteEntityMessage
    virtual void getSaveParameters(QMap<QString, QVariant>& parameters) const;
    //! Задать параметры, возвращаемые методом getSaveParameters (если он не был переопределен)
    void setSaveParameters(const QMap<QString, QVariant>& parameters);
    //! Метод вызывается перед отправкой сообщения DBCommandRemoveEntityMessage для стандартного удаления объекта
    //! (когда не используется CustomRemove). Необходимо заполнить произвольный набор параметров, который должен понимать драйвер
    //! Параметры будут переданы в конструктор DBCommandRemoveEntityMessage
    virtual void getRemoveParameters(QMap<QString, QVariant>& parameters) const;
    //! Задать параметры, возвращаемые методом getRemoveParameters (если он не был переопределен)
    void setRemoveParameters(const QMap<QString, QVariant>& parameters);

protected:
    //! Обработка команды
    void processCommand(const Command& command_key, const std::shared_ptr<void>& custom_data) override;

    //! Вызывается при получении ответа на postMessageCommand
    Error onMessageCommandFeedback(
        const Command& message_key, const std::shared_ptr<void>& custom_data, const Message& source_message, const Message& feedback_message) override;
    //! Вызывается при добавлении сообщения через EntityObject::postMessage
    void onStartWaitingMessageCommandFeedback(const Command& message_key) override;
    //! Вызывается после обработки запроса через EntityObject::onMessageFeedback
    void onFinishWaitingMessageCommandFeedback(const Command& message_key) override;

    //! Обработчик входящих сообщений от диспетчера сообщений
    zf::Error processInboundMessage(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle) override;

    //! Требуется ли проверка на ошибки (требуется только для моделей, которые не сущестувуют в БД(новые) и для
    //! отсоедененных (detached))
    bool isHighlightViewIsCheckRequired() const override;

protected:
    //! Обработчик менеджера обратных вызовов
    void processCallbackInternal(int key, const QVariant& data) final;

signals:
    //! Перед началом загрузки. ВАЖНО: это только запрос на загрузку, она может и не начаться если все данные уже есть или загрузка их уже в очереди
    void sg_beforeLoad(
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const zf::DataPropertySet& properties);
    //! Начата загрузка
    void sg_startLoad();
    //! Начато сохранение
    void sg_startSave();
    //! Начато удаление
    void sg_startRemove();

    //! Завершена загрузка
    void sg_finishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);
    //! Завершено сохранение
    void sg_finishSave(const zf::Error& error,
        //! Какие свойства сохранялись
        const zf::DataPropertySet& requested_properties,
        //! Какие свойства сохранялись фактически. Перечень не может быть шире чем requested_properties, т.к. сервер может сохранять свойства группами
        const zf::DataPropertySet& saved_properties,
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект
                        * уже имеет постоянный идентификатор, то возвращается пустой */
        const zf::Uid& persistent_uid);
    //! Завершено удаление
    void sg_finishRemove(const zf::Error& error);

    //! Первая попытка загрузки
    void sg_onFirstLoadRequest();

    //! Перед началом запуска операции на выполнение
    void sg_operationBeforeProcess(const zf::Operation& operation);
    //! Завершилась обработка операции
    void sg_operationProcessed(const zf::Operation& operation, const zf::Error& error);

    //! Изменились права доступа
    void sg_accessRightsChanged(
        //! Прямые права доступа
        const zf::AccessRights& direct_rights,
        //! Косвенные права доступа (на связаные объекты)
        const zf::AccessRights& relation_rights);

private:
    using EntityObject::setEntity;

    //! Информация о команде загрузки
    struct LoadInfo
    {
        DataPropertySet properties;
        LoadOptions options;
        QMap<QString, QVariant> parameters;

        bool contains(const LoadInfo* i) const;
        bool equal(const LoadInfo* i) const;
    };
    //! Информация о команде сохранения
    struct SaveInfo
    {
        DataPropertySet properties;
        QMap<QString, QVariant> parameters;
        QVariant by_user;

        bool contains(const SaveInfo* i) const;
        bool equal(const SaveInfo* i) const;
    };
    //! Информация о команде удаления
    struct RemoveInfo
    {
        QMap<QString, QVariant> parameters;
        QVariant by_user;

        bool contains(const RemoveInfo* i) const;
        bool equal(const RemoveInfo* i) const;
    };

    void bootstrap();
    //! Вызывается ядром после создания объекта
    void afterCreated();

    //! Изменилось имя сущности
    void onEntityNameChangedHelper() final;

    Error processOperationHelper(const Operation& op);

    //! Проверить на вхождение command_info_2 в command_info_1
    static bool commandContains(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2);
    //! Проверить на совпадение command_info_1 с command_info_2
    static bool commandEqual(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2);

    //! Слить две команды в одну. Если нельзя - возвращает false
    static bool commandMerge(const Command& command_key, const std::shared_ptr<void>& command_info_1, const std::shared_ptr<void>& command_info_2,
        //! Результат слияния
        std::shared_ptr<void>& target_command_info);

    //! Проверить есть ли уже такие команды в очереди и выполнить при необходимости
    CommandExecuteResult tryExecuteCommand(const Command& command_key, const std::shared_ptr<void>& custom_data);

    //! Задать ошибку синхронной операции
    void setSyncError(const Error& error) const;

    void startLoadHelper(const std::shared_ptr<LoadInfo>& info) const;
    Error finishLoadHelper(const std::shared_ptr<void>& custom_data, const Message& message, const Error& error,
        //! Прямые права доступа (при CustomLoad)
        const AccessRights& custom_direct_rights,
        //! Косвенные права доступа (на связаные объекты) (при CustomLoad)
        const AccessRights& custom_relation_rights) const;
    Error finishSaveHelper(const std::shared_ptr<void>& custom_data, const Message& message, const Error& error, const Uid& persistent_uid,
        //! Некритическая ошибка. Означает что сохранение выполнено, но что-то еще пошло не так
        const Error& non_critical_error) const;
    Error finishRemoveHelper(const std::shared_ptr<void>& custom_data, const Message& message, const Error& error) const;

    //! Закончена собственная реализация загрузки
    //! На вход передается ошибка загрузки (если была)
    void finishCustomLoadHelper(const Error& error,
        //! Прямые права доступа
        const AccessRights& direct_rights = AccessRights(AccessFlag::Administrate),
        //! Косвенные права доступа (на связаные объекты)
        const AccessRights& relation_rights = AccessRights(AccessFlag::Administrate));
    //! Закончена собственная реализация сохранения
    void finishCustomSaveHelper(
        //! В случае ошибки указать ее тут
        const Error& error,
        /*! Постоянный идентификатор, полученный после сохранения с isTemporary() == true. Если объект уже
         * имеет постоянный идентификатор, то указать пустой. При наличии ошибки - не задавать!
         * Если используется DatabaseObjectOption::StandardSaveExtension, то тоже не задавать */
        const Uid& persistent_uid,
        //! Некритическая ошибка. Означает что сохранение выполнено, но что-то еще пошло не так
        const Error& non_critical_error);
    //! Закончена собственная реализация удаления
    void finishCustomRemoveHelper(const Error& error) const;

    //! Сохранить копию данных до начала их изменения
    void saveOriginalData() const;
    //! Запросить обновление виджета по окончании загрузки
    void updateRequestedWidgets() const;
    //! Задать права доступа
    void setAccessRights(
        //! Прямые права доступа
        const AccessRights& direct_rights,
        //! Косвенные права доступа (на связаные объекты)
        const AccessRights& relation_rights) const;

    //! Очистка идентификаторов строк
    void clearDatasetRowIDs(const DataProperty& dataset_property, ItemModel* dataset, const QModelIndex& parent) const;

    //! Вызвать sg_startLoad при необходимости
    void emitStartLoad();

    static DataStructurePtr getDataStructureHelper(const Uid& entity_uid);

    //! Постоянный идентификатор сущности (если был присвоен)
    mutable Uid _persistent_uid;
    //! Некритическая ошибка при сохранении
    mutable Error _non_critical_save_error;

    //! Является ли отсоединенной. Такие объекты предназначены для редактирования и не доступны для получения
    //! через менеджер моделей
    bool _is_detached = false;

    //! Последняя ошибка синхронной операции
    mutable Error _sync_error;

    //! Особые настройки
    DatabaseObjectOptions _options;

    //! параметры, возвращаемые методом getLoadParameters (если он не был переопределен)
    QMap<QString, QVariant> _load_parameters;
    //! параметры, возвращаемые методом getSaveParameters (если он не был переопределен)
    QMap<QString, QVariant> _save_parameters;
    //! параметры, возвращаемые методом getRemoveParameters (если он не был переопределен)
    QMap<QString, QVariant> _remove_parameters;

    //! event loop для ожидания ответа синхронных сообщений
    mutable QEventLoop _send_message_loop;
    //! Таймер для ожидания ответа синхронных сообщений
    mutable QTimer* _send_message_timer = nullptr;

    //! Выполняется custom сохранение, загрузка или удаление
    mutable bool _is_custom_db_operation = false;
    mutable std::shared_ptr<void> _custom_db_operation_data;

    //! Информация об ошибке окончания CustomLoad, CustomSave, CustomRemove
    mutable Error _custom_error;

    //! Информация об окончании CustomLoad
    mutable AccessRights _custom_load_direct_rights;
    mutable AccessRights _custom_load_relation_rights;

    //! Информация об окончании CustomSave
    mutable Uid _custom_save_persistent_uid;
    //! Некритическая ошибка при CustomSave
    mutable Error _custom_save_non_critical_error;

    //! Выполняется загрузка DatabaseObjectOption::StandardLoadExtension
    mutable bool _is_standard_load_extension = false;
    //! Выполняется загрузка DatabaseObjectOption::StandardAfterSaveExtension
    mutable bool _is_standard_after_save_extension = false;

    //! Список виджетов по которым был запрос на обновление по окончании загрузки
    mutable QSet<QPointer<QWidget>> _widget_update_requested;

    //! Данные модели до начала ее изменения
    mutable DataContainerPtr _original_data;

    //! Прямые права доступа к данной сущности
    mutable std::unique_ptr<AccessRights> _direct_rights;
    //! Косвенные права доступа к данному типу сущности
    mutable std::unique_ptr<AccessRights> _relation_rights;

    //! Вызваны все конструкторы
    bool _is_constructed = false;
    //! Модель не будет удалена при падении на нее ссылок до нуля
    bool _is_model_keeped = false;

    //! Была хотя бы раз попытка загрузки
    bool _was_load_request = false;

    //! Завершена полная загрузка
    mutable bool _is_loading_complete = true;

    //! Был ли сигнал на начало загрузки. Для исключения двойных вызовов сигнала
    mutable bool _is_start_load_emmited = false;

    friend class View;
    friend class SharedPtrDeleter;
    friend class SharedObjectManager;
    friend class ModelManager;
};

//! Умный указатель на базовый класс модели
typedef std::shared_ptr<Model> ModelPtr;

} // namespace zf

Q_DECLARE_METATYPE(zf::ModelPtr)
