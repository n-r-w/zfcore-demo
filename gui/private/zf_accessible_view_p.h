#pragma once

#include <QPointer>
#include <QAccessible>
#include <QAccessibleWidget>
#include <QAbstractItemView>
#include <QHeaderView>

class QTreeViewPrivate;

/*
Весь код ниже скопирован из Src/qtbase/src/widgets/accessible/itemviews_p.h, itemviews.cpp
Классы: QAccessibleTable, QAccessibleTree, QAccessibleTableCell, AccessibleTableHeaderCell,
QAccessibleTableCornerButton
Регистрация фабрики UI Automation в Framework::bootstrapAccessibility()
*/

namespace zf
{
class AccessibleTableCell;
class AccessibleTableHeaderCell;
class ItemSelectorTreeView;

//! Реализация UI Automation для табличных ItemView
class AccessibleTable : public QAccessibleTableInterface, public QAccessibleObject
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    explicit AccessibleTable(QWidget* w);
    bool isValid() const override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QString text(QAccessible::Text t) const override;
    QRect rect() const override;

    QAccessibleInterface *childAt(int x, int y) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *) const override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // table interface
    virtual QAccessibleInterface *cellAt(int row, int column) const override;
    virtual QAccessibleInterface *caption() const override;
    virtual QAccessibleInterface *summary() const override;
    virtual QString columnDescription(int column) const override;
    virtual QString rowDescription(int row) const override;
    virtual int columnCount() const override;
    virtual int rowCount() const override;

    // selection
    virtual int selectedCellCount() const override;
    virtual int selectedColumnCount() const override;
    virtual int selectedRowCount() const override;
    virtual QList<QAccessibleInterface*> selectedCells() const override;
    virtual QList<int> selectedColumns() const override;
    virtual QList<int> selectedRows() const override;
    virtual bool isColumnSelected(int column) const override;
    virtual bool isRowSelected(int row) const override;
    virtual bool selectRow(int row) override;
    virtual bool selectColumn(int column) override;
    virtual bool unselectRow(int row) override;
    virtual bool unselectColumn(int column) override;

    QAbstractItemView *view() const;

    void modelChange(QAccessibleTableModelChangeEvent *event) override;

protected:
    QAccessible::Role cellRole() const
    {
        switch (m_role) {
        case QAccessible::List:
            return QAccessible::ListItem;
        case QAccessible::Table:
            return QAccessible::Cell;
        case QAccessible::Tree:
            return QAccessible::TreeItem;
        default:
            Q_ASSERT(0);
        }
        return QAccessible::NoRole;
    }

    QHeaderView *horizontalHeader() const;
    QHeaderView *verticalHeader() const;

    // maybe vector
    typedef QHash<int, QAccessible::Id> ChildCache;
    mutable ChildCache childToId;

    virtual ~AccessibleTable();

private:
    // the child index for a model index
    int logicalIndex(const QModelIndex& index) const;
    QAccessible::Role m_role;
};

//! Реализация UI Automation для древовидных ItemView
class AccessibleTree : public AccessibleTable
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    explicit AccessibleTree(QWidget* w)
        : AccessibleTable(w)
    {}


    QAccessibleInterface *childAt(int x, int y) const override;
    int childCount() const override;
    QAccessibleInterface *child(int index) const override;

    int indexOfChild(const QAccessibleInterface *) const override;

    int rowCount() const override;

    // table interface
    QAccessibleInterface *cellAt(int row, int column) const override;
    QString rowDescription(int row) const override;
    bool isRowSelected(int row) const override;
    bool selectRow(int row) override;

    // Грязный, пухучий хак для доступа к приватному d_func
    static QTreeViewPrivate* call_d_func(const QObject* obj);

private:
    QModelIndex indexFromLogical(int row, int column = 0) const;
    int logicalIndex(const QModelIndex& index) const;
};

//! Реализация UI Automation для ячейки ItemView
class AccessibleTableCell : public QAccessibleInterface, public QAccessibleTableCellInterface, public QAccessibleActionInterface
{
public:        
    AccessibleTableCell(QAbstractItemView* view, const QModelIndex& m_index, QAccessible::Role role);

    void *interface_cast(QAccessible::InterfaceType t) override;
    QObject *object() const override { return nullptr; }
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    bool isValid() const override;

    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int) const override;

    // cell interface
    virtual int columnExtent() const override;
    virtual QList<QAccessibleInterface*> columnHeaderCells() const override;
    virtual int columnIndex() const override;
    virtual int rowExtent() const override;
    virtual QList<QAccessibleInterface*> rowHeaderCells() const override;
    virtual int rowIndex() const override;
    virtual bool isSelected() const override;
    virtual QAccessibleInterface* table() const override;

    //action interface
    virtual QStringList actionNames() const override;
    virtual void doAction(const QString &actionName) override;
    virtual QStringList keyBindingsForAction(const QString &actionName) const override;

private:
    QHeaderView *verticalHeader() const;
    QHeaderView *horizontalHeader() const;
    QPointer<QAbstractItemView > view;
    QPersistentModelIndex m_index;
    QAccessible::Role m_role;

    void selectCell();
    void unselectCell();

    friend class AccessibleTable;
    friend class AccessibleTree;
};

//! Реализация UI Automation для заголовка ItemView
class AccessibleTableHeaderCell : public QAccessibleInterface
{
public:
    // For header cells, pass the header view in addition
    AccessibleTableHeaderCell(QAbstractItemView* view, int index, Qt::Orientation orientation);

    QObject *object() const override { return nullptr; }
    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;
    bool isValid() const override;

    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }

    QString text(QAccessible::Text t) const override;
    void setText(QAccessible::Text t, const QString &text) override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;

private:
    QHeaderView *headerView() const;

    QPointer<QAbstractItemView> view;
    int index;
    Qt::Orientation orientation;

    friend class AccessibleTable;
    friend class AccessibleTree;
};

//! Реализация UI Automation для углового элемента ItemView
class AccessibleTableCornerButton : public QAccessibleInterface
{
public:
    AccessibleTableCornerButton(QAbstractItemView* view_)
        : view(view_)
    {}

    QObject *object() const override { return nullptr; }
    QAccessible::Role role() const override { return QAccessible::Pane; }
    QAccessible::State state() const override { return QAccessible::State(); }
    QRect rect() const override { return QRect(); }
    bool isValid() const override { return true; }

    QAccessibleInterface *childAt(int, int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface *) const override { return -1; }

    QString text(QAccessible::Text) const override { return QString(); }
    void setText(QAccessible::Text, const QString &) override {}

    QAccessibleInterface *parent() const override {
        return QAccessible::queryAccessibleInterface(view);
    }
    QAccessibleInterface *child(int) const override {
        return nullptr;
    }

private:
    QPointer<QAbstractItemView> view;
};

} // namespace zf
