#pragma once

#include <QObject>
#include <QPointer>
#include <QQueue>

#include "zf_basic_types.h"
#include "zf_callback.h"
#include "zf_message.h"
#include "zf_uid.h"

namespace zf
{
typedef std::function<void(const zf::Command& command_key, const std::shared_ptr<void>& data)> CommandProcessorCallbackFunc;

/*! Класс для упрощения работы с очередями команд: ModuleCommands::CommandKey */
class ZCORESHARED_EXPORT CommandProcessor : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    explicit CommandProcessor(
        //! Кому будут направляться CommandProcessorCallbackFunc
        QObject* object,
        //! Менеджер обратных вызовов
        CallbackManager* callback_manager);
    ~CommandProcessor() override;

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    ;
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
    //! Добавить запрос на выполнение команды
    void addCommandRequest(
        //! Ключ команды ModuleCommands::CommandKey
        const Command& command_key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data,
        //! Что будет вызвано при выполнении команды. Делать bind к объекту, переданному в конструктор
        CommandProcessorCallbackFunc callback);
    //! Ждем ли выполнение команды ModuleCommands::CommandKey
    bool isWaitingCommandExecute(const Command& command_key) const;

    //! Удаляет все команды с данным ключом. Возвращает количество удаленных
    //! Не действует на команду, выполняетмую в настоящий момент
    int removeCommandRequests(
        //! Ключ команды ModuleCommands::CommandKey
        const Command& command_key,
        //! Если не nullptr, то удаляет только команды с парой command_key/custom_data
        const std::shared_ptr<void>& custom_data = std::shared_ptr<void>());

    //! Данные для команды при условии что она находится в очереди на выполнение. Если нет, то ошибка
    QList<std::shared_ptr<void>> commandData(const Command& command_key) const;

    //! Выполняется ли сейчас команда
    bool isExecuting() const;

    //! Текущая выполнямая команда (если нет, то 0)
    Command currentCommand() const;
    //! Текущие данные
    std::shared_ptr<void> currentData() const;

    //! Закончить обработку текущей команды. Команда находится в очереди до момента вызова этого метода
    bool finishCommand();

public slots:
    //! Обратный вызов от CallbackManager
    void sl_callback(int key, const QVariant& data);

signals:
    //! Команда добавлена
    void sg_commandAdded(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& command_key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);
    //! Команда удалена или завершилась
    void sg_commandRemoved(
        //! Ключ команды ModuleCommands::CommandKey
        const zf::Command& command_key,
        //! Произвольные данные
        const std::shared_ptr<void>& custom_data);

private:
    //! Обработка очереди команд
    void processCommandQueue();

    QObject* _object;
    //! Менеджер обратных вызовов
    CallbackManager* _callback_manager;

    struct CommandQueueInfo
    {
        Command command_key;
        std::shared_ptr<void> data;
        CommandProcessorCallbackFunc callback;        
    };
    QQueue<std::shared_ptr<CommandQueueInfo>> _command_queue;
    //! Текущая команда
    Command _current_command;
    std::shared_ptr<void> _current_data;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
