#include "zf_module_object.h"
#include "zf_core.h"
#include "zf_framework.h"

namespace zf
{
ModuleObject::ModuleObject(const EntityCode& module_code)
    : _entity_code(module_code)
{
    internalCallbackManager()->registerObject(this, "sl_callbackManagerInternal");
    callbackManager()->registerObject(this, "sl_callbackManager");

    setObjectName(_entity_code.string());
    internalCallbackManager()->addRequest(this, Framework::MODULE_OBJECT_AFTER_CREATED);
}

ModuleObject::~ModuleObject()
{        
    _is_destroying = true;

    _callback_manager->objectExtensionDestroy();
    _internal_callback_manager->objectExtensionDestroy();
}

I_Plugin* ModuleObject::plugin() const
{
    if (_plugin == nullptr)
        _plugin = Core::getPlugin(entityCode());

    return _plugin;
}

EntityCode ModuleObject::entityCode() const
{
    return _entity_code;
}

OperationList ModuleObject::operations() const
{
    return plugin()->operations();
}

ModuleInfo ModuleObject::moduleInfo() const
{
    Error error;
    return Core::getModuleInfo(_entity_code, error);
}

Operation ModuleObject::operation(const OperationID& operation_id) const
{
    return plugin()->operation(operation_id);
}

Operation ModuleObject::operation(const EntityCode& module_code, const OperationID& operation_id)
{
    return Operation::getOperation(module_code, operation_id);
}

AccessRights ModuleObject::directAccessRights() const
{
    return Core::directPermissions()->rights(entityCode());
}

bool ModuleObject::hasDirectAccessRight(const AccessRights& right) const
{
    return directAccessRights().contains(right);
}

bool ModuleObject::hasDirectAccessRight(ObjectActionType action_type) const
{
    return hasDirectAccessRight(Utils::accessForAction(action_type));
}

AccessRights ModuleObject::relationAccessRights() const
{
    return Core::relationPermissions()->rights(entityCode());
}

bool ModuleObject::hasRelationAccessRight(const AccessRights& right) const
{
    return relationAccessRights().contains(right);
}

bool ModuleObject::hasRelationAccessRight(ObjectActionType action_type) const
{
    return hasRelationAccessRight(Utils::accessForAction(action_type));
}

QString ModuleObject::customConfigurationCode() const
{
    return _custom_configuration_code;
}

void ModuleObject::setCustomConfigurationCode(const QString& s)
{
    _custom_configuration_code = s;
}

QString ModuleObject::configurationCode() const
{
    return customConfigurationCodeInternal();
}

void ModuleObject::objectExtensionDestroy()
{
    ProgressObject::objectExtensionDestroy();
    _callback_manager->stop();
    _internal_callback_manager->stop();
}

QString ModuleObject::getDebugName() const
{
    return QStringLiteral("%1, %2").arg(metaObject()->className()).arg(entityCode().string());
}

QString ModuleObject::customConfigurationCodeInternal() const
{
    QString class_name = metaObject()->className();
    class_name.replace("::", "_");

    QString c_code = customConfigurationCode();
    if (!c_code.isEmpty())
        c_code += "_";

    return c_code + class_name.toLower();
}

Configuration* ModuleObject::activeConfiguration(int version) const
{
    return Core::entityConfiguration(entityCode(), customConfigurationCodeInternal(), version);
}

CallbackManager* ModuleObject::callbackManager() const
{
    if (_callback_manager == nullptr) {
        _callback_manager = new CallbackManager();
        _callback_manager->setObjectName("ModuleObject::callbackManager");
    }
    return _callback_manager;
}

void ModuleObject::processCallback(int key, const QVariant& data)
{
    Q_UNUSED(key)
    Q_UNUSED(data)
}

QString ModuleObject::moduleDataLocation() const
{
    return plugin()->moduleDataLocation();
}

CallbackManager* ModuleObject::internalCallbackManager() const
{
    if (_internal_callback_manager == nullptr) {
        _internal_callback_manager = new CallbackManager(const_cast<ModuleObject*>(this));
        _internal_callback_manager->setObjectName("ModuleObject::internalCallbackManager");
    }
    return _internal_callback_manager;
}

void ModuleObject::processCallbackInternal(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (key == Framework::MODULE_OBJECT_AFTER_CREATED) {
        setObjectName(QString("%1:%2").arg(metaObject()->className()).arg(entityCode().string()));
    }
}

void ModuleObject::sl_callbackManager(int key, const QVariant& data)
{
    if (_is_destroying || objectExtensionDestroyed())
        return;

    processCallback(key, data);
}

void ModuleObject::sl_callbackManagerInternal(int key, const QVariant& data)
{
    if (_is_destroying || Utils::isAppHalted() || objectExtensionDestroyed())
        return;

    processCallbackInternal(key, data);
}

} // namespace zf
