#pragma once

#include <QAccessibleWidget>
#include <QPointer>

/* Код выдран из
Src\qtbase\src\widgets\accessible\qaccessiblewidget.cpp
Src\qtbase\src\widgets\accessible\qaccessiblemenu_p.h
Src\qtbase\src\widgets\accessible\qaccessiblemenu.cpp

Изменение:
- возврат objectName вместо содержимого для QAccessible::Name (метод QAccessibleInterface::text)
- добавление методов accessibleFactory

Регистрация фабрики в Framework::bootstrapAccessibility()
*/

class QMenu;
class QMenuBar;
class QAction;

namespace zf
{
class AccessibleMenu : public QAccessibleWidget
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    explicit AccessibleMenu(QWidget* w);

    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;

    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;
    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *parent() const override;
    int indexOfChild( const QAccessibleInterface *child ) const override;

protected:
    QMenu *menu() const;
};

class AccessibleMenuBar : public QAccessibleWidget
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    explicit AccessibleMenuBar(QWidget* w);

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;

    int indexOfChild(const QAccessibleInterface *child) const override;

protected:
    QMenuBar *menuBar() const;
};

class AccessibleMenuItem : public QAccessibleInterface, public QAccessibleActionInterface
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    explicit AccessibleMenuItem(QWidget* owner, QAction* w);

    ~AccessibleMenuItem();
    void *interface_cast(QAccessible::InterfaceType t) override;

    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    bool isValid() const override;
    int indexOfChild(const QAccessibleInterface * child) const override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    QObject * object() const override;
    QWindow *window() const override;

    QRect rect() const override;
    QAccessible::Role role() const override;
    void setText(QAccessible::Text t, const QString & text) override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;

    // QAccessibleActionInterface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

    QWidget *owner() const;
protected:
    QAction *action() const;
private:
    QAction *m_action;
    QPointer<QWidget> m_owner; // can hold either QMenu or the QMenuBar that contains the action
};

} // namespace zf
