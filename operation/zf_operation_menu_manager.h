#ifndef ZF_OPERATION_MANAGER_H
#define ZF_OPERATION_MANAGER_H

#include "zf.h"
#include "zf_operation.h"
#include "zf_object_extension.h"

#include <QToolBar>
#include <QMenu>
#include <QPointer>
#include <QToolButton>

namespace zf
{
//! Менеджер меню
class ZCORESHARED_EXPORT OperationMenuManager : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    OperationMenuManager(
        //! Уникальный ID для automation
        const QString& id,
        //! Меню операций. Нельзя смешивать операции с разным OperationScope
        const OperationMenu& operation_menu,
        //! Область действия меню
        OperationScope scope,
        //! Какой тип тулбара будет использован
        ToolbarType type);
    ~OperationMenuManager() override;

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
    //! Меню операций
    const OperationMenu* operationMenu() const;

    //! Toolbar
    QToolBar* toolbar() const;

    //! Экшн для операции, установленной при setOperationsMenu
    QAction* operationAction(const Operation& operation,
                             //! Остановка при ошибке
                             bool halt_if_error = true) const;
    //! Экшн для узла меню (содержит подменю с операциями)
    QAction* nodeAction(
        //! Текстовый код, заданный при создании узла
        const QString& accesible_id,
        //! Остановка при ошибке
        bool halt_if_error = true) const;

    //! Кнопка операции
    QToolButton* button(const Operation& operation,
                        //! Остановка при ошибке
                        bool halt_if_error = true) const;

    //! Все экшены
    QList<QAction*> actions() const;
    //! Вс операции
    OperationList operations() const;

signals:
    //! Сработал экшен операции
    void sg_operationActivated(const zf::Operation& operation);
    //! Изменились параметры QAction для операции
    void sg_operationActionChanged(const zf::Operation& operation, QAction* action);

private slots:
    //! Сработал экшен операции
    void sl_operationActionActivated();
    //! Изменились параметры QAction для операции
    void sl_operationActionChanged();
    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

private:
    //! Создать QMenu на основе OperationMenu
    void createToolbar();
    QMenu* createToolbarHelper(const OperationMenuNode& node, QMenu* menu);

    //! Уникальный ID для automation
    QString _id;
    OperationMenu _operation_menu;
    OperationScope _scope;    
    ToolbarType _type;
    //! Главный toolbar
    QPointer<QToolBar> _toolbar;
    //! Экшены, созданные по операциям
    QMap<Operation, QAction*> _operation_actions;
    //! Экшены, созданные родительских узлов меню. Ключ - accesibleId
    QMap<QString, QAction*> _node_actions;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf

#endif // ZF_OPERATION_MANAGER_H
