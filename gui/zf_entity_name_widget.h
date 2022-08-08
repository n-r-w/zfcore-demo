#pragma once

#include "zf_uid.h"
#include "zf_message.h"
#include "zf_object_extension.h"

class QHBoxLayout;

namespace zf
{
class EntityNameWidgetLabel;
class EntityNameWidgetEditor;

//! Виджет, который отображает имя объекта
class ZCORESHARED_EXPORT EntityNameWidget : public QWidget, public I_ObjectExtension
{
    Q_OBJECT
public:
    //! Внешний вид
    enum Type
    {
        //! Аналог QLabel
        LabelType,
        //! Аналог QLineEdit
        LineEditType
    };

    EntityNameWidget(QWidget* parent = nullptr);
    ~EntityNameWidget();

    //! Идентификатор
    Uid entityUid() const;
    //! Установить идентификатор
    void setEntityUid(const Uid& entity_uid,
                      //! Поле, содержащее имя
                      const PropertyID& name_property = {});

    //! Имя
    QString name() const;
    //! Поле, содержащее имя
    PropertyID nameProperty() const;

    //! Задать текст, который будет отображен вне зависимости от данных
    void setOverrideText(const QString& s);

    //! Задать внешний вид
    void setType(Type type);
    //! Внешний вид
    Type type() const;

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

signals:
    //! Имя получено
    void sg_nameReceived();

private slots:
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    void requestUpdateName();
    void setName(const QString& name);
    void initEditor();
    void setEditorText(const QString& s);

    Type _type = LineEditType;
    Uid _entity_uid;
    PropertyID _name_property;
    QString _name;
    QString _override_text;

    QHBoxLayout* _layout = nullptr;
    EntityNameWidgetEditor* _editor = nullptr;
    EntityNameWidgetLabel* _label = nullptr;

    MessageID _message_id;

    SubscribeHandle _subscribe_handle_entity_changed;
    SubscribeHandle _subscribe_handle_entity_removed;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
