#ifndef ZF_I_PLUGIN_H
#define ZF_I_PLUGIN_H

#include "zf.h"
#include "zf_operation.h"
#include "zf_uid.h"
#include "zf_object_extension.h"

namespace zf
{
class View;
class Model;
typedef std::shared_ptr<Model> ModelPtr;
typedef std::shared_ptr<View> ViewPtr;
class ModuleInfo;
class DataStructure;
typedef std::shared_ptr<DataStructure> DataStructurePtr;
class Uid;
class CallbackManager;
class MMCommandGetEntityNamesMessage;
class MessageID;
class DataProperty;

/*! Интерфейс плагина
 * Плагин реализуется в модуле.
 * Отвечает за общую информацию о модуле и выступает в качестве фабрики для представлений, моделей и сервисов модуля
 */
class I_Plugin
{
public:
    virtual ~I_Plugin() {}

    //! Структура данных сущности
    virtual DataStructurePtr dataStructure(
        //! Для какого экземпляра сущности запрашивается структура.
        //! Если модуль поддерживает общую структуру для всех
        //! сущностей, то можно указать пустой Uid()
        const Uid& entity_uid,
        //! Ошибка
        Error& error) const = 0;

    //! Созданные ранее структуры
    virtual QHash<Uid, DataStructurePtr> createdStructure() const = 0;

    //! Вернуть полный список поддерживаемых операций
    virtual OperationList operations() const = 0;
    //! Найти операцию по ее коду
    virtual Operation operation(const OperationID& operation_id) const = 0;

    //! Общая информация о модуле
    virtual const ModuleInfo& getModuleInfo() const = 0;
    //! Код сущности
    virtual EntityCode entityCode() const = 0;

    //! Создать новое view для модели
    virtual View* createView(
        //! Модель
        const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Произвольные данные. Для возможности создания разных представлений по запросу
        const QVariant& data)
        = 0;

    //! Создать новую модель
    virtual Model* createModel(
        //! Идентификатор сущности
        const Uid& entity_uid,
        //! Является ли модель отсоединенной. Такие модели предназначены для редактирования и не доступны для
        //! получения через менеджер моделей
        bool is_detached)
        = 0;

    //! Асинхронное получение имени сущностей.
    //! В ответ необходимо послать сообщение MMEventEntityNamesMessage для sender.
    //! По умолчанию возвращает имя модуля для уникальных сущностей и грузит модель с данными для получения именя для остальных (медленно)
    //! Если имеется возможность получить расшифровку имени другими быстрыми способами (например через справочники) то надо переопределить
    //! этот метод
    virtual void getEntityName(
        //! Кому отправить ответное сообщение MMEventEntityNamesMessage
        const I_ObjectExtension* requester,
        //! Идентификатор сообщения, который надо передать в ответное MMEventEntityNamesMessage
        const MessageID& feedback_message_id,
        //! Идентификаторы сущностей (все относятся к данному плагину)
        const UidList& entity_uids,
        //! Какое свойство отвечают за хранение имени. Если какое свойство не валидно , то получить свойство с именем на основании
        //! zf::PropertyOption:FullName, а затем zf::PropertyOption:Name
        const DataProperty& name_property,
        //! Язык. Если QLocale::AnyLanguage, то используется язык интерфейса
        QLocale::Language language)
        = 0;

    //! Вызывается ядром после загрузки всех модулей
    virtual void afterLoadModules() = 0;

    //! Вызывается ядром при необходимости загрузить конфигурацию модуля. Каталог для хранения файлов: moduleDataLocation
    virtual Error onLoadConfiguration() = 0;
    //! Вызывается ядром при необходимости сохранить конфигурацию модуля. Каталог для хранения файлов: moduleDataLocation
    virtual Error onSaveConfiguration() const = 0;

    //! Начальная загрузка данных из БД (если надо). Вызывается ядром один раз после инициализации плагина либо программистом при
    //! необходимости перегрузить данные из БД по какому то событию
    virtual Error loadDataFromDatabase() = 0;

    //! Каталог, где можно хранить разные данные
    virtual QString moduleDataLocation() const = 0;
    //! Модель с данным идентификатором можно использовать в разных потоках одновременно
    virtual bool isModelSharedBetweenThreads(const Uid& entity_uid) const = 0;

    //! Вызывается ядром перед завершением работы
    virtual void onShutdown() = 0;

protected:
    //! Запрос на подтверждение операции. Вызывается автоматически перед processOperation для
    //! OperationOption::Confirmation
    virtual bool confirmOperation(const Operation& operation) = 0;
    //! Обработчик операции OperationScope::Module. Вызывается автоматически при активации экшена операции
    virtual Error processOperation(const Operation& operation) = 0;

    friend class Framework;
};

} // namespace zf

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(zf::I_Plugin, "ru.ancor.metastaff.I_Plugin/1.0")
QT_END_NAMESPACE

#endif // ZF_I_PLUGIN_H
