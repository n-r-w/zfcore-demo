#pragma once

#include "zf_message.h"
#include "zf_data_container.h"
#include "zf_object_extension.h"

namespace zf
{
//! Сообщение для отправки результата запроса через I_ExternalRequest::requestLookup
class ZCORESHARED_EXPORT ExternalRequestMessage : public zf::Message
{
public:
    ExternalRequestMessage(const zf::Message& m);
    explicit ExternalRequestMessage(const zf::ExternalRequestMessage& m);
    explicit ExternalRequestMessage(const MessageID& feedback_id,
        //! Код сообщения сервиса, к которому идет запрос
        const zf::MessageCode& service_message_code,
        //! Результат запроса
        //! Пара: идентификатор строки, значение (зависит от конкретого сервиса, к которому идет запрос)
        const QList<QPair<zf::Uid, QVariant>>& result,
        //! Атрибуты. Количество совпадает с result. Содержит произвольный набор пар ключ/значение для каждой строки в result
        const QList<QMap<QString, QVariant>>& attributes = {});

    //! Результат запроса
    //! Пара: идентификатор, значение (зависит от конкретого сервиса, к которому идет запрос)
    QList<QPair<zf::Uid, QVariant>> result() const;
    //! Атрибуты. Количество и порядок совпадает с result. Содержит произвольный набор пар ключ/значение для каждой строки в result
    QList<QMap<QString, QVariant>> attributes() const;
};

/*! Интерфейс для установки делегата со спецификой внешнего сервиса
 * Класс, который реализует данный интерфейс, должен быть наследником QObject */
class I_ExternalRequest
{
public:
    virtual ~I_ExternalRequest() { }

    /*! Выполнить запрос к внешнему сервису на основании конкретного идентификатора. Метод должен быть асинхронным
     * Ответ ExternalRequestMessage должен содержать один элемент с идентификатором, совпадающим с запросом          
     * Тип ответного сообщения ExternalRequestMessage если все ОК, либо ErrorMessage при ошибке */
    virtual MessageID requestLookup(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Идентификатор для поиска
        const Uid& key,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type,
        //! Параметры
        const QVariant& options = {})
        = 0;

    /*! Выполнить запрос к внешнему сервису на основании произвольной строки. Метод должен быть асинхронным     
     * Ответ ExternalRequestMessage может содержать произвольное количество строк     
     * Тип ответного сообщения ExternalRequestMessage если все ОК, либо ErrorMessage при ошибке */
    virtual MessageID requestLookup(
        //! Отправитель
        const I_ObjectExtension* sender,
        //! Текстовое значение для поиска
        const QString& text,
        //! Идентификаторы родительских объектов
        const UidList& parent_keys,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type,
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Проверка допустимости поиска по указанной строке
    virtual bool checkText(const QString& text,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type,
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Проверка допустимости редактирования
    virtual bool canEdit(
        //! Идентификаторы родительских объектов по порядку от головного к подчиненному
        const UidList& parent_keys,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type,
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Извлечь текстовое значение из ExternalRequestMessage::result
    virtual QString extractText(const QVariant& data,
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Проверить идентификатор на корректность
    virtual bool isCorrectKey(
        //! Идентификатор для поиска
        const Uid& key,
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Задержка перед началом поиска в мс.
    virtual int requestDelay(
        //! Параметры
        const QVariant& options = {})
        = 0;

    //! Сбросить кэш и т.п.
    virtual void invalidate() = 0;
};

//! Класс для управления запросами к внешним сервисам на уровне сигналов/слотов Qt
class ZCORESHARED_EXPORT ExternalRequester : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    ExternalRequester(QObject* parent = nullptr);
    ~ExternalRequester();

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
    /*! Выполнить запрос к внешнему сервису на основании конкретного идентификатора
     *  MessageID содержится в сигнале sg_feedback */
    MessageID requestLookup(
        //! Интерфейс для запроса
        I_ExternalRequest* request_interface,
        //! Идентификатор для поиска
        const Uid& key,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type);
    /*! Выполнить запрос к внешнему сервису на основании произвольной строки 
     *  MessageID содержится в сигнале sg_feedback */
    MessageID requestLookup(
        //! Интерфейс для запроса
        I_ExternalRequest* request_interface,
        //! Текстовые значение для поиска
        const QString& text,
        //! Идентификаторы родительских объектов
        const UidList& parent_keys,
        //! Тип запроса (зависит от специфики внешнего сервиса:
        //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
        const QString& request_type);

signals:
    //! Сигнал вызывается при получении ответа от сервиса
    void sg_feedback(
        //! Идентификатор сообщения
        const zf::MessageID& message_id,
        //! Результат запроса
        //! Пара: идентификатор, значение (зависит от конкретого сервиса, к которому идет запрос)
        const QList<QPair<zf::Uid, QVariant>> result,
        //! Атрибуты. Количество совпадает с result. Содержит произвольный набор пар ключ/значение для каждой строки в result
        const QList<QMap<QString, QVariant>>& attributes,
        //! Ошибка (если есть)
        const zf::Error& error);

private slots:
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    QSet<MessageID> _requests;
    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

}; // namespace zf
