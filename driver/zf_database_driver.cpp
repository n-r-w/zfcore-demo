#include "zf_database_driver.h"
#include "zf_core.h"

#include <QDebug>

namespace zf
{
DatabaseDriver::DatabaseDriver()
    : QObject()
    , _object_extension(new ObjectExtension(this, false))

{
}

DatabaseDriver::~DatabaseDriver()
{
    delete _object_extension;
}

void DatabaseDriver::shutdown()
{
    _shutdowned = 1;
}

bool DatabaseDriver::isShutdowned() const
{
    return _shutdowned;
}

bool DatabaseDriver::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void DatabaseDriver::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant DatabaseDriver::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool DatabaseDriver::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void DatabaseDriver::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void DatabaseDriver::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void DatabaseDriver::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void DatabaseDriver::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void DatabaseDriver::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

const DatabaseDriverConfig* DatabaseDriver::config() const
{
    return &_config;
}

Error DatabaseDriver::bootstrap(const DatabaseDriverConfig& config)
{
    _config = config;

    zf::Core::messageDispatcher()->registerObject(CoreUids::DRIVER, this, "sl_message_dispatcher_inbound");

    return Error();
}

void DatabaseDriver::onMessageDispatcherInbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(message)
    Q_UNUSED(subscribe_handle)
}

void DatabaseDriver::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Core::messageDispatcher()->confirmMessageDelivery(message, this);
    onMessageDispatcherInbound(sender, message, subscribe_handle);
}

} // namespace zf
