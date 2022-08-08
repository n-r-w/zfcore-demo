#pragma once

#include <QAccessibleWidget>
#include <QAccessibleTableInterface>
#include <QAccessibleObject>
#include <QLineEdit>
#include <QFrame>
#include <QListView>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QFutureWatcher>

#include "zf_plain_text_edit.h"
#include "zf_uid.h"

namespace zf
{
class RequestSelector;
class RequestSelectorListView;

//! Обертка вокруг данных RequestSelector. Одна колонка
class RequestSelectorItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    RequestSelectorItemModel(RequestSelector* s);

    struct RowData
    {
        int row = -1;
        //! Идентификатор строки
        Uid uid;
        //! Имя
        QString name;
        //! Данные (структура зависит от конкретного сервиса)
        QVariant data;
    };
    typedef std::shared_ptr<RowData> RowDataPtr;

    //! Возвращает текущую, выделенную строку
    //! Истина, если начинает обновление в отдельном потоке
    bool setInternalData(const QList<QPair<Uid, QVariant>>& msg_data);

    //! Поиск по идентификатору
    int findUid(const Uid& uid) const;
    //! Поиск по произвольной строке
    int findString(const QString s) const;

    //! Имя строки
    QString rowName(int row) const;
    //! Идентификатор строки
    Uid rowUid(int row) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex&) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex&, const QVariant&, int) override;

    RequestSelector* _selector;
    QVector<RowDataPtr> _data;
    mutable QHash<Uid, RowDataPtr> _data_hash;
    mutable QHash<QString, RowDataPtr> _string_hash;

    QFutureWatcher<int> _watcher_key_row;
    bool _in_update_data = false;
    mutable QMutex _mutex;

signals:
    void sg_onBeginUpdateData();
    void sg_onEndUpdateData(
        //! Текущая, выделенная строки
        int key_row);
};

//! Контейнер для QListView. Нужен для корректной работы QListView в состоянии Qt::Popup (напрямую не
//! работает)
class RequestSelectorContainer : public QFrame
{
    Q_OBJECT
public:
    RequestSelectorContainer(RequestSelector* selector, RequestSelectorItemModel* model);
    ~RequestSelectorContainer();

    void paintEvent(QPaintEvent* e) override;
    void hideEvent(QHideEvent* e) override;

    QPointer<RequestSelectorListView> _list_view;
    QPointer<RequestSelector> _selector;    
};

//! Внутренний редактор
class RequestSelectorEditor : public QLineEdit
{
    Q_OBJECT
public:
    RequestSelectorEditor(RequestSelector* selector);
    bool event(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void initStyleOption(QStyleOptionFrame* option) const;

    void highlight(bool b);

    RequestSelector* _selector;
    bool _highlighted = false;
    QTimer* _focus_timer;
};

class RequestSelectorListView : public QListView
{
    Q_OBJECT
public:
    RequestSelectorListView(RequestSelector* selector, QWidget* parent);
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    QStyleOptionViewItem viewOptions() const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    RequestSelector* _selector;
};

class RequestSelectorDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    RequestSelectorDelegate(QAbstractItemView* view, RequestSelector* selector);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QAbstractItemView* _view;
    RequestSelector* _selector;
};

} // namespace zf
