#pragma once

#include <QAccessibleWidget>
#include <QAccessibleTableInterface>
#include <QAccessibleObject>
#include <QFrame>
#include <QListView>
#include <QTreeView>
#include <QPointer>
#include <QStyledItemDelegate>
#include "zf_plain_text_edit.h"
#include "zf_line_edit.h"

namespace zf
{
class ItemSelector;
class ItemSelectorListView;
class ItemSelectorTreeView;

// Регистрация фабрики UI Automation в Framework::bootstrapAccessibility()

//! Интерфейс для UI Automation
class ItemSelectorAccessibilityInterface : public QAccessibleWidget
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    ItemSelectorAccessibilityInterface(QWidget* w);
    int childCount() const override;
    QAccessibleInterface* child(int index) const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QString text(QAccessible::Text t) const override;
    QStringList actionNames() const override;
    QString localizedActionDescription(const QString& action_name) const override;
    void doAction(const QString& action_name) override;
    QStringList keyBindingsForAction(const QString&) const override;
    QAccessibleInterface* childAt(int x, int y) const override;

private:
    ItemSelector* selector() const;
};

//! Внутренний редактор
class ItemSelectorEditor : public LineEdit
{
    Q_OBJECT
public:
    ItemSelectorEditor(ItemSelector* selector);
    bool event(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void initStyleOption(QStyleOptionFrame* option) const;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void onContextMenuCreated(QMenu* m) override;

    void highlight(bool b);

    ItemSelector* _selector;
    bool _highlighted = false;
    QTimer* _focus_timer;
    bool _is_edit_menu_active = false;
};

//! Внутренний контрол для отображения многострочных значений
class ItemSelectorMemo : public PlainTextEdit
{
    Q_OBJECT
public:
    ItemSelectorMemo(ItemSelector* selector);
    bool event(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    void initStyleOption(QStyleOptionFrame* option) const;
    QSize sizeHint() const override;

    void highlight(bool b);

    ItemSelector* _selector;
    bool _highlighted = false;
};

//! Контейнер для QListView. Нужен для корректной работы QListView в состоянии Qt::Popup (напрямую не
//! работает)
class ItemSelectorContainer : public QFrame
{
    Q_OBJECT
public:
    ItemSelectorContainer(ItemSelector* selector);
    ~ItemSelectorContainer();

    void paintEvent(QPaintEvent* e);

    QPointer<ItemSelectorListView> _list_view;
    QPointer<ItemSelectorTreeView> _tree_view;
    QPointer<ItemSelector> _selector;
};

class ItemSelectorListView : public QListView
{
    Q_OBJECT
public:
    ItemSelectorListView(ItemSelector* selector, QWidget* parent);
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QStyleOptionViewItem viewOptions() const override;

    ItemSelector* _selector;
};

class ItemSelectorTreeView : public QTreeView
{
    Q_OBJECT
public:
    ItemSelectorTreeView(ItemSelector* selector, QWidget* parent);
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QStyleOptionViewItem viewOptions() const override;

    ItemSelector* _selector;

    //! Указатель на приватные данные Qt для особых извращений
    QObjectData* privateData() const;
};

class ItemSelectorDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ItemSelectorDelegate(QAbstractItemView* view, ItemSelector* selector);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QAbstractItemView* _view;
    ItemSelector* _selector;
};

} // namespace zf
