#include "zf_accessible_menu_p.h"

#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStyle>
#include <private/qwidget_p.h>

namespace zf
{
static int qt_accAmpIndex(const QString& text)
{
    if (text.isEmpty())
        return -1;

    int fa = 0;
    while ((fa = text.indexOf(QLatin1Char('&'), fa)) != -1) {
        ++fa;
        if (fa < text.length()) {
            // ignore "&&"
            if (text.at(fa) == QLatin1Char('&')) {
                ++fa;
                continue;
            } else {
                return fa - 1;
                break;
            }
        }
    }

    return -1;
}

QString qt_accStripAmp(const QString& text)
{
    QString newText(text);
    int ampIndex = qt_accAmpIndex(newText);
    if (ampIndex != -1)
        newText.remove(ampIndex, 1);

    return newText.replace(QLatin1String("&&"), QLatin1String("&"));
}
QString qt_accHotKey(const QString& text)
{
    int ampIndex = qt_accAmpIndex(text);
    if (ampIndex != -1)
        return QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + text.at(ampIndex + 1);

    return QString();
}

QAccessibleInterface *getOrCreateMenu(QWidget *menu, QAction *action)
{
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(action);
    if (!iface) {
        iface = new AccessibleMenuItem(menu, action);
        QAccessible::registerAccessibleInterface(iface);
    }
    return iface;
}

QAccessibleInterface* AccessibleMenu::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("QMenu"))
        return new AccessibleMenu(qobject_cast<QWidget*>(object));

    return nullptr;
}

AccessibleMenu::AccessibleMenu(QWidget* w)
    : QAccessibleWidget(w)
{
    Q_ASSERT(menu());
}

QMenu* AccessibleMenu::menu() const
{
    return qobject_cast<QMenu*>(object());
}

int AccessibleMenu::childCount() const
{
    return menu()->actions().count();
}

QAccessibleInterface* AccessibleMenu::childAt(int x, int y) const
{
    QAction *act = menu()->actionAt(menu()->mapFromGlobal(QPoint(x,y)));
    if(act && act->isSeparator())
        act = 0;
    return act ? getOrCreateMenu(menu(), act) : 0;
}

QString AccessibleMenu::text(QAccessible::Text t) const
{
    QString tx = QAccessibleWidget::text(t);
    if (!tx.isEmpty())
        return tx;

    if (t == QAccessible::Name)
        return menu()->objectName().isEmpty() ? menu()->windowTitle() : menu()->objectName();
    return tx;
}

QAccessible::Role AccessibleMenu::role() const
{
    return QAccessible::PopupMenu;
}

QAccessibleInterface* AccessibleMenu::child(int index) const
{
    if (index < childCount())
        return getOrCreateMenu(menu(), menu()->actions().at(index));
    return 0;
}

QAccessibleInterface* AccessibleMenu::parent() const
{
    if (QAction *menuAction = menu()->menuAction()) {
        const QList<QWidget*> parentCandidates =
                QList<QWidget*>() << menu()->parentWidget() << menuAction->associatedWidgets();
        for (QWidget *w : parentCandidates) {
            if (qobject_cast<QMenu*>(w) || qobject_cast<QMenuBar*>(w)) {
                if (w->actions().indexOf(menuAction) != -1)
                    return getOrCreateMenu(w, menuAction);
            }
        }
    }
    return QAccessibleWidget::parent();
}

int AccessibleMenu::indexOfChild(const QAccessibleInterface* child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menu()) {
        return menu()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

AccessibleMenuBar::AccessibleMenuBar(QWidget* w)
    : QAccessibleWidget(w, QAccessible::MenuBar)
{
    Q_ASSERT(menuBar());
}

QMenuBar* AccessibleMenuBar::menuBar() const
{
    return qobject_cast<QMenuBar*>(object());
}

int AccessibleMenuBar::childCount() const
{
    return menuBar()->actions().count();
}

QAccessibleInterface* AccessibleMenuBar::child(int index) const
{
    if (index < childCount()) {
        return getOrCreateMenu(menuBar(), menuBar()->actions().at(index));
    }
    return 0;
}

int AccessibleMenuBar::indexOfChild(const QAccessibleInterface* child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menuBar()) {
        return menuBar()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

QAccessibleInterface* AccessibleMenuItem::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr)
        return nullptr;

    if (class_name == QLatin1String("QAction") && object->parent() != nullptr
        && object->parent()->metaObject()->className() == QLatin1String("QMenu"))
        return new AccessibleMenuItem(qobject_cast<QWidget*>(object->parent()), qobject_cast<QAction*>(object));

    return nullptr;
}

AccessibleMenuItem::AccessibleMenuItem(QWidget* owner, QAction* action)
    : m_action(action)
    , m_owner(owner)
{
}

AccessibleMenuItem::~AccessibleMenuItem()
{}

QAccessibleInterface* AccessibleMenuItem::childAt(int x, int y) const
{
    for (int i = childCount() - 1; i >= 0; --i) {
        QAccessibleInterface *childInterface = child(i);
        if (childInterface->rect().contains(x,y)) {
            return childInterface;
        }
    }
    return 0;
}

int AccessibleMenuItem::childCount() const
{
    return m_action->menu() ? 1 : 0;
}

int AccessibleMenuItem::indexOfChild(const QAccessibleInterface* child) const
{
    if (child && child->role() == QAccessible::PopupMenu && child->object() == m_action->menu())
        return 0;
    return -1;
}

bool AccessibleMenuItem::isValid() const
{
    return m_action && m_owner;
}

QAccessibleInterface* AccessibleMenuItem::parent() const
{
    return QAccessible::queryAccessibleInterface(owner());
}

QAccessibleInterface* AccessibleMenuItem::child(int index) const
{
    if (index == 0 && action()->menu())
        return QAccessible::queryAccessibleInterface(action()->menu());
    return 0;
}

void* AccessibleMenuItem::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

QObject* AccessibleMenuItem::object() const
{
    return m_action;
}

/*! \reimp */
QWindow* AccessibleMenuItem::window() const
{
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    QWindow* result = nullptr;
    if (!m_owner.isNull()) {
        result = m_owner->windowHandle();
        if (!result) {
            if (const QWidget* nativeParent = m_owner->nativeParentWidget())
                result = nativeParent->windowHandle();
        }
    }
    return result;
#else
    return m_owner.isNull()
        ? nullptr
        : qt_widget_private(m_owner.data())->windowHandle(QWidgetPrivate::WindowHandleMode::Closest);
#endif
}

QRect AccessibleMenuItem::rect() const
{
    QRect rect;
    QWidget *own = owner();

    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        rect = menuBar->actionGeometry(m_action);
        QPoint globalPos = menuBar->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    } else if (QMenu* menu = qobject_cast<QMenu*>(own)) {
        rect = menu->actionGeometry(m_action);
        QPoint globalPos = menu->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    }
    return rect;
}

QAccessible::Role AccessibleMenuItem::role() const
{
    return m_action->isSeparator() ? QAccessible::Separator : QAccessible::MenuItem;
}

void AccessibleMenuItem::setText(QAccessible::Text /*t*/, const QString& /*text */)
{
}

QAccessible::State AccessibleMenuItem::state() const
{
    QAccessible::State s;
    QWidget *own = owner();

    if (own && (own->testAttribute(Qt::WA_WState_Visible) == false || m_action->isVisible() == false)) {
        s.invisible = true;
    }

    if (QMenu *menu = qobject_cast<QMenu*>(own)) {
        if (menu->activeAction() == m_action)
            s.focused = true;
    } else if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        if (menuBar->activeAction() == m_action)
            s.focused = true;
    }
    if (own && own->style()->styleHint(QStyle::SH_Menu_MouseTracking))
        s.hotTracked = true;
    if (m_action->isSeparator() || !m_action->isEnabled())
        s.disabled = true;
    if (m_action->isChecked())
        s.checked = true;
    if (m_action->isCheckable())
        s.checkable = true;

    return s;
}

QString AccessibleMenuItem::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
        case QAccessible::Name: {
            if (!m_action->objectName().isEmpty())
                str = m_action->objectName();            
            else
                str = qt_accStripAmp(m_action->text());
            break;
        }
        case QAccessible::Accelerator: {
            QKeySequence key = m_action->shortcut();
            if (!key.isEmpty()) {
                str = key.toString();
            } else {
                str = qt_accHotKey(m_action->text());
            }
            break;
        }
        default:
            break;
    }
    return str;
}

QStringList AccessibleMenuItem::actionNames() const
{
    QStringList actions;
    if (!m_action || m_action->isSeparator())
        return actions;

    if (m_action->menu()) {
        actions << showMenuAction();
    } else {
        actions << pressAction();
    }
    return actions;
}

void AccessibleMenuItem::doAction(const QString& actionName)
{
    if (!m_action->isEnabled())
        return;

    if (actionName == pressAction()) {
        m_action->trigger();
    } else if (actionName == showMenuAction()) {
        if (QMenuBar *bar = qobject_cast<QMenuBar*>(owner())) {
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                bar->setActiveAction(m_action);
            }
        } else if (QMenu* menu = qobject_cast<QMenu*>(owner())) {
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                menu->setActiveAction(m_action);
            }
        }
    }
}

QStringList AccessibleMenuItem::keyBindingsForAction(const QString&) const
{
    return QStringList();
}

QAction* AccessibleMenuItem::action() const
{
    return m_action;
}

QWidget* AccessibleMenuItem::owner() const
{
    return m_owner;
}

QAccessibleInterface* AccessibleMenuBar::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("QMenuBar"))
        return new AccessibleMenu(qobject_cast<QWidget*>(object));

    return nullptr;
}

} // namespace zf
