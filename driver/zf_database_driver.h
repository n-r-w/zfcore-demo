#pragma once

#include "zf_core.h"
#include "zf_i_database_driver.h"
#include "zf_database_driver_config.h"

namespace zf
{
class ZCORESHARED_EXPORT DatabaseDriver : public QObject, public zf::I_DatabaseDriver, public I_ObjectExtension
{
    Q_OBJECT
    Q_INTERFACES(zf::I_DatabaseDriver)
public:
    DatabaseDriver();
    ~DatabaseDriver();

    //! Завершение работы
    void shutdown() override;
    //! Остановлен ли драйвер
    bool isShutdowned() const final;

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
    //! Параметры драйвера
    const DatabaseDriverConfig* config() const final;

    //! Начальная инициализация. Вызывается в методе Core::registerDatabaseDriver
    zf::Error bootstrap(const DatabaseDriverConfig& config) override;

protected:
    //! Входящие сообщения
    virtual void onMessageDispatcherInbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private slots:
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    DatabaseDriverConfig _config;
    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
    //! Признак того, что драйвер завершил работу
    QAtomicInt _shutdowned = 0;
};

} // namespace zf
