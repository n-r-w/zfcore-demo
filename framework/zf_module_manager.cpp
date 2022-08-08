#include "zf_module_manager.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_translation.h"
#include <QDebug>
#include <QPluginLoader>

namespace zf
{
ModuleManager::ModuleManager(const QMap<EntityCode, QMap<QString, QVariant>>& plugins_options)
    : _plugins_options(plugins_options)
{
}

ModuleManager::~ModuleManager()
{
}

const QMap<EntityCode, QMap<QString, QVariant>>& ModuleManager::pluginsOptions() const
{
    return _plugins_options;
}

EntityCode ModuleManager::registerModule(const QString& library_name, Error& error, const QString& path)
{
    Z_CHECK(Utils::isMainThread());

    error.clear();

    QString app_path = qApp->applicationDirPath();
    QString sub_path = QDir::fromNativeSeparators(path);
    if (sub_path.left(1) == "/")
        sub_path = sub_path.right(sub_path.length() - 1);
    QString lib_path = app_path + "/";
    if (!sub_path.isEmpty())
        lib_path += sub_path + "/";
    lib_path += Utils::libraryName(library_name);

    if (!QFile::exists(lib_path)) {
        error = Error::moduleError(ZF_TR(ZFT_MODULE_NOT_FOUND).arg(library_name));
        return {};
    }

    I_Plugin* plugin = loadModule(lib_path, error);
    if (error.isError())
        return {};

    return registerModule(plugin, error);
}

EntityCode ModuleManager::registerModule(I_Plugin* plugin, Error& error)
{
    Z_CHECK(Utils::isMainThread());

    Z_CHECK_NULL(plugin);
    auto info = Z_MAKE_SHARED(MInfo);
    info->plugin = plugin;
    info->info = ModuleInfo(plugin->getModuleInfo());
    Z_CHECK(info->info.isValid());

    for (const auto& i : qAsConst(_modules)) {
        if (i->info.code() == info->info.code()) {
            error = Error::moduleError(QString("Module %1 already registered").arg(info->info.code().value()));
            return {};
        }
    }

    _modules[info->info.code()] = info;
    _modules_order << info->info.code();
    return info->info.code();
}

bool ModuleManager::isModuleRegistered(const EntityCode& code) const
{
    QMutexLocker lock(&_mutex);
    return _modules.contains(code);
}

QString ModuleManager::getModuleName(const EntityCode& code) const
{
    QMutexLocker lock(&_mutex);
    if (!isModuleRegistered(code))
        return QString::number(code.value());
    return _modules.value(code)->info.name();
}

ModuleInfo ModuleManager::getModuleInfo(const EntityCode& code, Error& error) const
{
    QMutexLocker lock(&_mutex);
    error.clear();
    if (!isModuleRegistered(code)) {
        error = Error::moduleNotFoundError(code);
        return ModuleInfo();
    }
    return _modules.value(code)->info;
}

I_Plugin* ModuleManager::getPlugin(const EntityCode& code, Error& error) const
{
    QMutexLocker lock(&_mutex);
    error.clear();
    if (!isModuleRegistered(code)) {
        error = Error::moduleNotFoundError(code);
        return nullptr;
    }
    return _modules.value(code)->plugin;
}

EntityCodeList ModuleManager::getAllModules() const
{
    QMutexLocker lock(&_mutex);
    return _modules_order;
}

Error ModuleManager::loadModulesConfiguration()
{
    Error error;
    for (auto& i : qAsConst(_modules)) {
        error << i->plugin->onLoadConfiguration();
    }
    return error;
}

Error ModuleManager::saveModulesConfiguration()
{
    Error error;
    for (auto& i : qAsConst(_modules)) {
        error << i->plugin->onSaveConfiguration();
    }
    return error;
}

void ModuleManager::shutdown()
{
    for (auto& i : qAsConst(_modules)) {
        i->plugin->onShutdown();
    }
}

I_Plugin* ModuleManager::loadModule(const QString& file, Error& error)
{
    error.clear();

    QPluginLoader loader(file);
    if (!loader.load()) {
        error = Error::moduleError(loader.errorString());
        return nullptr;
    }
    I_Plugin* entrance = qobject_cast<I_Plugin*>(loader.instance());
    if (!entrance) {
        error = Error::moduleError(ZF_TR(ZFT_MODULE_LOAD_ERROR));
        return nullptr;
    }

    return entrance;
}

} // namespace zf
