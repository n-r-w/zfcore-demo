#ifndef ZF_MODULE_OBJECT_H
#define ZF_MODULE_OBJECT_H

#include "zf_configuration.h"
#include "zf_i_plugin.h"
#include "zf_module_info.h"
#include "zf_progress_object.h"
#include "zf_connection_information.h"

namespace zf
{
class CallbackManager;

//! Базовый класс для классов, которые входят в модуль
class ZCORESHARED_EXPORT ModuleObject : public ProgressObject
{
    Q_OBJECT

    Q_PROPERTY(zf::ModuleInfo moduleInfo READ moduleInfo)
public:
    ModuleObject(
        //! Уникальный код модуля
        const EntityCode& module_code);
    ~ModuleObject() override;

    //! Информация о модуле
    virtual ModuleInfo moduleInfo() const;
    //! Плагин модуля
    I_Plugin* plugin() const;
    //! Код сущности (вид сущности)
    EntityCode entityCode() const;

    //! Вернуть полный список поддерживаемых операций
    OperationList operations() const;
    //! Найти операцию по ее коду
    Operation operation(const OperationID& operation_id) const;
    //! Операция по коду модуля и идентификатору. Нельзя использовать в конструкторе плагина
    static Operation operation(const EntityCode& module_code, const OperationID& operation_id);

    //! Прямые права доступа к данному типу сущности
    virtual AccessRights directAccessRights() const;
    //! Проверка наличия прямого права доступа для данного типа сущности
    bool hasDirectAccessRight(const AccessRights& right) const;
    //! Проверка наличия прямого права по виду действия для данного типа сущности
    bool hasDirectAccessRight(ObjectActionType action_type) const;

    //! Косвенные права доступа к данному типу сущности
    virtual AccessRights relationAccessRights() const;
    //! Проверка наличия косвенного права доступа для данного типа сущности
    bool hasRelationAccessRight(const AccessRights& right) const;
    //! Проверка наличия косвенного права по виду действия для данного типа сущности
    bool hasRelationAccessRight(ObjectActionType action_type) const;

    //! Нестандартное имя конфигурации.
    QString customConfigurationCode() const;
    //! Нестандартное имя конфигурации. Устанавливать если необходимо разделить хранение
    //! параметров (ширина и положение колонок, фильтры и т.д.)
    //! Вызывать в конструкторе или сразу после создания объекта
    void setCustomConfigurationCode(const QString& s);

    //! Имя конфигурации
    QString configurationCode() const;

    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

protected:
    //! Нестандартное имя конфигурации (служебный)
    virtual QString customConfigurationCodeInternal() const;

    //! Какую конфигурацию использовать по умолчанию для автоматического сохранения настроек
    Configuration* activeConfiguration(
        //! Версия
        int version = 0) const;

    //! Менеджер обратных вызовов
    CallbackManager* callbackManager() const;
    //! Обработчик менеджера обратных вызовов
    virtual void processCallback(int key, const QVariant& data);

    //! Каталог, где можно хранить разные данные
    QString moduleDataLocation() const;

    //! Имя объекта для отладки
    QString getDebugName() const override;

protected:
    //! Менеджер обратных вызовов для нужд ядра. Не использовать!
    CallbackManager* internalCallbackManager() const;
    //! Обработчик менеджера обратных вызовов
    virtual void processCallbackInternal(int key, const QVariant& data);

private slots:
    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);
    //! Менеджер обратных вызовов
    void sl_callbackManagerInternal(int key, const QVariant& data);

private:
    EntityCode _entity_code;
    bool _is_destroying = false;
    mutable I_Plugin* _plugin = nullptr;
    //! Имя нестандартной конфигурации
    QString _custom_configuration_code;

    //! Менеджер обратных вызовов
    mutable CallbackManager* _callback_manager = nullptr;
    mutable CallbackManager* _internal_callback_manager = nullptr;
};
} // namespace zf

#endif // ZF_MODULE_OBJECT_H
