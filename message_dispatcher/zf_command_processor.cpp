#include "zf_command_processor.h"
#include "zf_core.h"
#include "zf_database_messages.h"
#include "zf_framework.h"

#include <QDebug>

namespace zf
{
CommandProcessor::CommandProcessor(QObject* object, CallbackManager* callback_manager)
    : QObject()
    , _object(object)
    , _callback_manager(callback_manager)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_object);
    Z_CHECK_NULL(callback_manager);
    _callback_manager->registerObject(this, "sl_callback");
}

CommandProcessor::~CommandProcessor()
{
    delete _object_extension;
}

bool CommandProcessor::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void CommandProcessor::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant CommandProcessor::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool CommandProcessor::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void CommandProcessor::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void CommandProcessor::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void CommandProcessor::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void CommandProcessor::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void CommandProcessor::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void CommandProcessor::addCommandRequest(const Command& command_key, const std::shared_ptr<void>& custom_data,
                                         CommandProcessorCallbackFunc callback)
{
    Z_CHECK(command_key.isValid());

    auto q_info = Z_MAKE_SHARED(CommandQueueInfo);
    q_info->command_key = command_key;
    q_info->data = custom_data;
    q_info->callback = callback;

    _command_queue.enqueue(q_info);

    _callback_manager->addRequest(this, Framework::COMMAND_PROCESSOR_COMMAND_CALLBACK_KEY);

    emit sg_commandAdded(command_key, custom_data);
}

bool CommandProcessor::isWaitingCommandExecute(const Command& command_key) const
{        
    for (auto& c : _command_queue) {
        if (c->command_key == command_key)
            return true;
    }
    return false;
}

int CommandProcessor::removeCommandRequests(const Command& command_key, const std::shared_ptr<void>& custom_data)
{
    Z_CHECK(command_key.isValid());

    int count = 0;
    for (int i = _command_queue.count() - 1; i >= 0; i--) {
        if (_command_queue.at(i)->command_key != command_key)
            continue;

        if (custom_data != nullptr && _command_queue.at(i)->data != custom_data)
            continue;

        auto data = _command_queue.at(i);
        _command_queue.removeAt(i);
        emit sg_commandRemoved(command_key, data);
        count++;
    }

    return count;
}

QList<std::shared_ptr<void>> CommandProcessor::commandData(const Command& command_key) const
{
    QList<std::shared_ptr<void>> res;

    for (auto& c : _command_queue) {
        if (c->command_key == command_key && c->data != nullptr)
            res << c->data;
    }
    return res;
}

bool CommandProcessor::isExecuting() const
{
    return _current_command.isValid();
}

Command CommandProcessor::currentCommand() const
{
    return _current_command;
}

std::shared_ptr<void> CommandProcessor::currentData() const
{
    return _current_data;
}

bool CommandProcessor::finishCommand()
{
    if (!isExecuting())
        return false;

    //    qDebug() << "CommandProcessor finish command:" << _object->objectName() << _current_command;

    auto current_command = _current_command;
    auto current_data = _current_data;

    _current_command.clear();
    _current_data.reset();

    if (!_command_queue.isEmpty())
        _callback_manager->addRequest(this, Framework::COMMAND_PROCESSOR_COMMAND_CALLBACK_KEY);

    emit sg_commandRemoved(current_command, current_data);

    return true;
}

void CommandProcessor::sl_callback(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (_object_extension->objectExtensionDestroyed())
        return;

    if (key == Framework::COMMAND_PROCESSOR_COMMAND_CALLBACK_KEY)
        processCommandQueue();
}

void CommandProcessor::processCommandQueue()
{
    if (!Core::isBootstraped() || isExecuting() || _command_queue.isEmpty())
        return;

    auto command = _command_queue.dequeue();
    _current_command = command->command_key;
    _current_data = command->data;

    //    qDebug() << "CommandProcessor start command:" << _object->objectName() << _current_command;

    command->callback(command->command_key, command->data);
}

} // namespace zf
