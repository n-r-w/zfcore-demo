#pragma once
#include "zf_plugin.h"
#include "zf_external_request.h"
#include "zf_object_extension.h"
#include "zf_uid.h"
#include "zf_thread_worker.h"
#include "zf_data_structure.h"

namespace ModelService
{
//! Инофрмация о запросах
struct InternalRequestInfo
{
    zf::ModelServiceLookupRequestOptions options;
    const zf::I_ObjectExtension* requester;
    zf::MessageID feedback_id;

    // либо текст, либо item_id
    QString filter_text;
    QVariant item_id;

    zf::ModelPtr model;
    QMetaObject::Connection connection;
    zf::DataProperty dataset;

    zf::DataProperty id_column;
    int id_column_role = Qt::DisplayRole;

    zf::DataProperty name_column;
    int name_column_role = Qt::DisplayRole;

    QList<QPair<zf::Uid, QVariant>> result;
};
typedef std::shared_ptr<InternalRequestInfo> InternalRequestInfoPtr;

//! Класс для взаимодействия с Worker через сигналы/слоты
class WorkerObject : public QObject, public zf::I_ObjectExtension
{
    Q_OBJECT
public:
    WorkerObject();
    ~WorkerObject();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;

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
    //! Обработать запрос
    Q_SLOT void registerRequest(const zf::MessageID& feedback_id, const zf::ModelServiceLookupRequestOptions& options, const zf::I_ObjectExtension* requester,
        const QString& filter_text, const QVariant& item_id);

    //! Очистка кэша
    Q_SLOT void clearCache();

private slots:
    //! Завершена загрузка модели
    void sl_modelFinishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    void processRequest(const InternalRequestInfoPtr& request);

    //! Запросы
    QMap<zf::MessageID, InternalRequestInfoPtr> _requests;
    //! Код сообщения для интерфейса I_ExternalRequest
    const int _i_external_request_msg_code_id = 1;
    //! Кэш моделей
    QMap<zf::Uid, zf::ModelPtr> _models_cache;

    //! Потокобезопасное расширение возможностей QObject
    zf::ObjectExtension* _object_extension = nullptr;
};

/*! Управление запросами к моделям
 * Асинхронные методы возвращают идентификатор сообщения. Он будет в качестве feedback_id в ответном сообщении от сервиса */
class ZCORESHARED_EXPORT Service : public zf::Plugin, public zf::I_ExternalRequest
{
    Q_OBJECT
public:
    Service();
    ~Service();

    //! Экземпляр сервиса
    static Service* instance();

public: // *** реализация I_ExternalRequest
    /*! Выполнить запрос к внешнему сервису на основании конкретного идентификатора. Метод должен быть асинхронным
     * Ответ ExternalRequestMessage должен содержать один элемент с идентификатором, совпадающим с запросом          
     * Тип ответного сообщения ExternalRequestMessage если все ОК, либо ErrorMessage при ошибке */
    zf::MessageID requestLookup(
        //! Отправитель
        const zf::I_ObjectExtension* sender,
        //! Идентификатор для поиска
        const zf::Uid& key,
        //! Тип запроса. В данном сервисе - сериализованное значение идентифкатора сущности
        const QString& request_type,
        //! Параметры
        const QVariant& options) override;
    /*! Выполнить запрос к внешнему сервису на основании произвольной строки. Метод должен быть асинхронным     
     * Ответ ExternalRequestMessage может содержать произвольное количество строк     
     * Тип ответного сообщения ExternalRequestMessage если все ОК, либо ErrorMessage при ошибке */
    zf::MessageID requestLookup(
        //! Отправитель
        const zf::I_ObjectExtension* sender,
        //! Текстовые значение для поиска
        const QString& text,
        //! Идентификатор родительского объекта
        const zf::UidList& parent_keys,
        //! Тип запроса. В данном сервисе - сериализованное значение идентифкатора сущности
        const QString& request_type,
        //! Параметры
        const QVariant& options) override;

    //! Проверка допустимости поиска по указанной строке
    bool checkText(const QString& text,
        //! Тип запроса. В данном сервисе - сериализованное значение идентифкатора сущности
        const QString& request_type,
        //! Параметры
        const QVariant& options) override;

    //! Проверка допустимости редактирования
    bool canEdit(
        //! Идентификаторы родительских объектов по порядку от головного к подчиненному
        const zf::UidList& parent_keys,
        //! Тип запроса. В данном сервисе не используется
        const QString& request_type,
        //! Параметры
        const QVariant& options) override;

    //! Проверить идентификатор на корректность
    bool isCorrectKey(
        //! Идентификатор для поиска
        const zf::Uid& key,
        //! Параметры
        const QVariant& options) override;

    //! Извлечь текстовое значение из ExternalRequestMessage::result
    QString extractText(const QVariant& data,
        //! Параметры
        const QVariant& options) override;

    //! Задержка перед началом поиска в мс.
    int requestDelay(
        //! Параметры
        const QVariant& options) override;

    //! Сбросить кэш и т.п.
    void invalidate() override;

protected:
    //! Вызывается ядром перед завершением работы
    void onShutdown() override;

private:
    //! Запрос на обработку запроса
    zf::MessageID registerRequest(
        const zf::ModelServiceLookupRequestOptions& options, const zf::I_ObjectExtension* requester, const QString& filter_text, const QVariant& item_id);

    //! Получить параметры из QVariant
    static zf::ModelServiceLookupRequestOptions getOptions(const QVariant& v);

    WorkerObject* _worker = nullptr;
    QThread* _worker_thread = nullptr;
};

} // namespace ModelService
