#ifndef ZF_MODULE_MANAGER_H
#define ZF_MODULE_MANAGER_H

#include "zf.h"
#include "zf_module_info.h"
#include "zf_error.h"
#include "zf_i_plugin.h"

namespace zf
{
//! Отвечает за загрузку и хранение информации о модулях
class ModuleManager
{
public:
    ModuleManager(
        //! Параметры плагинов. Содержание зависит от реализации плагинов
        const QMap<EntityCode, QMap<QString, QVariant>>& plugins_options);
    ~ModuleManager();

    //! Параметры плагинов. Содержание зависит от реализации плагинов
    const QMap<EntityCode, QMap<QString, QVariant>>& pluginsOptions() const;

    //! Зарегистрировать модуль из файла библиотеки
    EntityCode registerModule(
        //! Название библиотеки модуля без указания расширения и префикса lib (для Linux)
        const QString& library_name,
        //! Ошибка
        Error& error,
        //! Путь относительно основной папки приложения
        const QString& path);

    //! Зарегистрировать модуль из заранее созданного объекта
    EntityCode registerModule(
        //! Указатель на плагин. Владение переходит к ModuleManager
        I_Plugin* plugin, Error& error);

    //! Зарегистрирован ли модуль с указанным кодом
    bool isModuleRegistered(const EntityCode& code) const;
    //! Возвращает название модуля с указанным кодом. Если такого модуля не найдено, то возвращает значение кода
    QString getModuleName(const EntityCode& code) const;
    //! Возвращает общую информацию по зарегистрированному модулю
    ModuleInfo getModuleInfo(const EntityCode& code, Error& error) const;
    //! Получить интерфейс плагина. Плагие существует в единственном экземпляре для указанного кода сущности. За
    //! удаление отвечает ядро
    I_Plugin* getPlugin(const EntityCode& code, Error& error) const;

    //! Коды всех зарегистрированных модулей
    EntityCodeList getAllModules() const;

    //! Загрузить конфигурацию всех модулей
    Error loadModulesConfiguration();
    //! Сохранить конфигурацию всех модулей
    Error saveModulesConfiguration();

    //! Завершение работы
    void shutdown();

private:
    struct MInfo
    {
        //! Информация о модуле
        ModuleInfo info;
        //! Точка входа
        I_Plugin* plugin = nullptr;
    };

    //! Загрузить модуль и получить точку входа в плагин
    static I_Plugin* loadModule(const QString& file, Error& error);

    //! Все модули
    QHash<EntityCode, std::shared_ptr<MInfo>> _modules;
    //! Список модулей в порядке регистрации
    EntityCodeList _modules_order;

    //! Параметры плагинов. Содержание зависит от реализации плагинов
    QMap<EntityCode, QMap<QString, QVariant>> _plugins_options;

    mutable Z_RECURSIVE_MUTEX _mutex;
};

} // namespace zf

#endif // ZF_MODULE_MANAGER_H
