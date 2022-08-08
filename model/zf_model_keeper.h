#pragma once

#include <QHash>
#include <QObject>
#include <QQueue>

#include "zf_error.h"
#include "zf_model.h"
#include "zf_uid.h"

namespace zf
{
/*! Класс предназначен для централизованного хранения списков моделей, чтобы они не сбрасывались когда счетчик указателя
 * падает до нуля. При каждом запросе keepModels, загруженные модели сохраняются и хранятся до тех пор, пока объект не
 * будет удален или не будет вызов freeModels */
// TODO добавить возможность сохранять только заданое время, а потом удалять если не используется
class ZCORESHARED_EXPORT ModelKeeper : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    ModelKeeper();
    ~ModelKeeper();

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
    //! Хранится ли такая модель
    bool isModelKeeped(const Uid& uid) const;

    //! Хранить указанные модели
    void keepModels(const QList<ModelPtr>& models);

    //! Хранить указанные модели
    void keepModels(
        //! Список идентификаторов моделей
        const UidList& entity_uid_list,
        //! Какие свойства грузить. Если не задано, то все
        const QList<DataPropertySet>& properties_to_load = {});
    void keepModels(
        //! Идентификатор модели
        const Uid& entity_uid,
        //! Какие свойства грузить. Если не задано, то все
        const DataPropertySet& properties_to_load = {});

    //! Не хранить указанные модели
    void freeModels(
        //! Список идентификаторов моделей
        const UidList& entity_uid_list);

private slots:
    //! Завершено удаление модели
    void sl_finishRemove(const zf::Error& error);

    //! Для приема сообщений через messageDispatcher
    void sl_inbound_message(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Менеджер обратных вызово
    void sl_callbackManager(int key, const QVariant& data);

private:
    struct RequestInfo
    {
        Uid entity_uid;
        DataPropertySet properties;
    };

    QQueue<RequestInfo> _requests;
    QSet<MessageID> feedback_ids;
    QHash<Uid, ModelPtr> _models;

    mutable QMutex _mutex;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
