#include "zf_operation_menu_manager.h"
#include "zf_core.h"
#include "zf_framework.h"
#include <QDebug>

namespace zf
{
OperationMenuManager::OperationMenuManager(const QString& id, const OperationMenu& operation_menu, OperationScope scope, ToolbarType type)
    : _id(id)
    , _operation_menu(operation_menu)
    , _scope(scope)
    , _type(type)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(!_id.isEmpty());

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    createToolbar();

    for (QAction* a : qAsConst(_operation_actions)) {
        connect(a, &QAction::triggered, this, &OperationMenuManager::sl_operationActionActivated
                // тут может быть проблема для управления через UI Automation, которое может ждать отклик на нажатие кнопки, а вместо этого
                // запустится модальный диалог и пока он не закроется, отклика не будет. Добавление Qt::QueuedConnection решает эту
                // проблему, но может привести к другим нехорошим последствиям - например обработка эвента нажатия на кнопку пройдет после
                // того, как начнется обработка какого-нибудь другого эвента, связанного с GUI
                //, Qt::QueuedConnection
        );
    }
}

OperationMenuManager::~OperationMenuManager()
{    
    if (!_toolbar.isNull())
        _toolbar.clear();

    if (!_toolbar.isNull())
        delete _toolbar.data();

    delete _object_extension;
}

bool OperationMenuManager::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void OperationMenuManager::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant OperationMenuManager::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool OperationMenuManager::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void OperationMenuManager::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void OperationMenuManager::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void OperationMenuManager::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void OperationMenuManager::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void OperationMenuManager::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

const OperationMenu* OperationMenuManager::operationMenu() const
{
    return &_operation_menu;
}

QToolBar* OperationMenuManager::toolbar() const
{
    return _toolbar;
}

QAction* OperationMenuManager::operationAction(const Operation& operation, bool halt_if_error) const
{
    Z_CHECK(operation.isValid());
    Z_CHECK_X(operation.scope() == _scope,
              QString("Операция %1,%2 отлична от OperationScope::Entity").arg(operation.entityCode().value()).arg(operation.id().value()));

    QAction* action = _operation_actions.value(operation, nullptr);
    if (halt_if_error)
        Z_CHECK_X(action != nullptr, utf("Операция %1,%2 не была добавлена в меню в переопределении метода View::createOperationMenu()")
                                         .arg(operation.entityCode().value())
                                         .arg(operation.id().value()));
    return action;
}

QAction* OperationMenuManager::nodeAction(const QString& accesible_id, bool halt_if_error) const
{
    Z_CHECK(!accesible_id.isEmpty());

    QAction* action = _node_actions.value(accesible_id, nullptr);
    if (halt_if_error)
        Z_CHECK_X(action != nullptr, QString("Узел не найден %1").arg(accesible_id));
    return action;
}

QToolButton* OperationMenuManager::button(const Operation& operation, bool halt_if_error) const
{
    return qobject_cast<QToolButton*>(_toolbar->widgetForAction(operationAction(operation, halt_if_error)));
}

QList<QAction*> OperationMenuManager::actions() const
{
    return _operation_actions.values();
}

OperationList OperationMenuManager::operations() const
{
    return _operation_actions.keys();
}

void OperationMenuManager::sl_operationActionActivated()
{
    QAction* action = qobject_cast<QAction*>(sender());
    Z_CHECK_NULL(action);

    Operation operation = action->data().value<Operation>();
    Z_CHECK(operation.isValid());
    Z_CHECK(operation.scope() == _scope);

    emit sg_operationActivated(operation);
}

void OperationMenuManager::sl_operationActionChanged()
{
    if (objectExtensionDestroyed())
        return;

    QAction* action = qobject_cast<QAction*>(sender());
    Z_CHECK_NULL(action);

    Operation operation = action->data().value<Operation>();
    Z_CHECK(operation.isValid());
    Z_CHECK(operation.scope() == _scope);

    Framework::internalCallbackManager()->addRequest(this, Framework::OPERATION_MENU_MANAGER_PROCESS_TOOLBAR_KEY);

    emit sg_operationActionChanged(operation, action);
}

void OperationMenuManager::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::OPERATION_MENU_MANAGER_PROCESS_TOOLBAR_KEY) {
        Utils::compressToolbarActions(_toolbar, false);
    }
}

void OperationMenuManager::createToolbar()
{
    _toolbar = new QToolBar;
    Utils::prepareToolbar(_toolbar, _type);
    _toolbar->setObjectName(_id + +"tb");
    for (auto& node : _operation_menu.root()) {        
        if (node.isSeparator()) {
            _toolbar->addSeparator();

        } else if (node.operation().isValid()) {
            Z_CHECK_X(
                node.operation().scope() == _scope,
                QString("wrong operation scope: %1,%2").arg(node.operation().entityCode().value()).arg(node.operation().id().value()));

            Qt::ToolButtonStyle button_style;
            if (node.options().testFlag(OperationOption::ShowButtonText)) {
                if (Core::toolbarButtonStyle(_type) == Qt::ToolButtonTextUnderIcon)
                    button_style = Qt::ToolButtonTextUnderIcon;
                else if (Core::toolbarButtonStyle(_type) == Qt::ToolButtonTextBesideIcon)
                    button_style = Qt::ToolButtonTextBesideIcon;
                else if (Core::toolbarButtonStyle(_type) == Qt::ToolButtonIconOnly)
                    button_style = (_type == ToolbarType::Main) ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon;
                else
                    button_style = Qt::ToolButtonIconOnly;

            } else {
                button_style = Core::toolbarButtonStyle(_type);
            }

            QAction* action;
            if (node.options().testFlag(OperationOption::NoToolbar)) {
                action = Utils::createToolbarMenuAction(button_style, node.shortText(), node.icon(), nullptr, nullptr, nullptr,
                                                        node.accesibleId() + "a", "");

            } else {
                action = Utils::createToolbarMenuAction(button_style, node.shortText(), node.icon(), _toolbar, nullptr, nullptr,
                                                        node.accesibleId() + "a", node.accesibleId() + "b");
            }
            action->setData(QVariant::fromValue(node.operation()));
            _operation_actions[node.operation()] = action;

            connect(action, &QAction::changed, this, &OperationMenuManager::sl_operationActionChanged);

        } else if (!node.children().isEmpty()) {
            QMenu* menu = createToolbarHelper(node, nullptr);
            Z_CHECK_NULL(menu);
            menu->setObjectName(QStringLiteral("menu_%1").arg(node.accesibleId()));
            _node_actions[node.accesibleId()] = Utils::createToolbarMenuAction(
                node.options().testFlag(OperationOption::ShowButtonText) ? Qt::ToolButtonTextBesideIcon : Core::toolbarButtonStyle(_type),
                node.shortText(), node.icon(), _toolbar, nullptr, menu, node.accesibleId() + "a", node.accesibleId() + "b");
        }
    }
}

QMenu* OperationMenuManager::createToolbarHelper(const OperationMenuNode& node, QMenu* menu)
{
    QString automation_code = QStringLiteral("%1_%2").arg(node.operation().entityCode().value()).arg(node.operation().id().value());

    if (node.children().isEmpty()) {
        Z_CHECK_NULL(menu);
        QAction* action;
        if (node.operation().isValid()) {
            Z_CHECK_X(
                node.operation().scope() == _scope,
                QString("wrong operation scope: %1,%2").arg(node.operation().entityCode().value()).arg(node.operation().id().value()));

            action = menu->addAction(node.operation().icon(), node.shortText());
            action->setObjectName(QStringLiteral("action_%1").arg(automation_code));
            action->setData(QVariant::fromValue(node.operation()));
            _operation_actions[node.operation()] = action;

            connect(action, &QAction::changed, this, &OperationMenuManager::sl_operationActionChanged);
        }

        return nullptr;

    } else {
        Z_CHECK(!node.accesibleId().isEmpty());
        Z_CHECK_X(!_node_actions.contains(node.accesibleId()), QString("Menu node duplication: %1").arg(node.accesibleId()));

        QMenu* sub_menu = (menu == nullptr ? new QMenu(node.shortText()) : menu->addMenu(node.shortText()));
        _node_actions[node.accesibleId()] = sub_menu->menuAction();
        sub_menu->setObjectName(QStringLiteral("menu_%1").arg(automation_code));
        sub_menu->setIcon(node.icon());
        for (auto& child : node.children()) {
            createToolbarHelper(child, sub_menu);
        }

        return sub_menu;
    }
}

} // namespace zf
