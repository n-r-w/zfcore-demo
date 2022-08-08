#include "zf_item_selector.h"

#include "zf_core.h"
#include "zf_translation.h"
#include "private/zf_item_selector_p.h"
#include "zf_view.h"
#include "zf_select_from_model_dialog.h"
#include "zf_framework.h"
#include "zf_colors_def.h"
#include "zf_ui_size.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <QHoverEvent>
#include <QLineEdit>
#include <QScreen>
#include <QStyleOptionComboBox>
#include <QStylePainter>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolButton>
#include <QtMath>
#include <QAccessibleWidget>
#include <QAccessibleTableInterface>
#include <QClipboard>
#include <QMovie>
#include <QCompleter>
#include <qdrawutil.h>

namespace zf
{
// отступы по краям
static int margin()
{
    return 1;
}

// расстояние между кнопками
static int spacing()
{
    return 0;
}
// расстояние между полем  комбо и кнопками
static int editor_spacing()
{
    return 1;
}

static int memo_top_margin()
{
    return -1;
}

static int memo_bottom_margin()
{
    return -2;
}

#define ACCESSIBLE_ERASE 0 // кнопка Стереть
#define ACCESSIBLE_VIEW 1 // view
#define ACCESSIBLE_EDITOR 2 // редактор

QAccessibleInterface* ItemSelectorAccessibilityInterface::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("zf::ItemSelector"))
        return new ItemSelectorAccessibilityInterface(qobject_cast<QWidget*>(object));

    return nullptr;
}

ItemSelectorAccessibilityInterface::ItemSelectorAccessibilityInterface(QWidget* w)
    : QAccessibleWidget(w, QAccessible::ComboBox)
{
}

QAccessibleInterface* ItemSelectorAccessibilityInterface::child(int index) const
{
    if (index == ACCESSIBLE_ERASE)
        return QAccessible::queryAccessibleInterface(selector()->eraseButton());
    if (index == ACCESSIBLE_VIEW)
        return QAccessible::queryAccessibleInterface(selector()->view());
    if (index == ACCESSIBLE_EDITOR && selector()->isFullEnabled())
        return QAccessible::queryAccessibleInterface(selector()->editor());
    return nullptr;
}

int ItemSelectorAccessibilityInterface::childCount() const
{
    return selector()->isFullEnabled() ? ACCESSIBLE_EDITOR + 1 : ACCESSIBLE_EDITOR;
}

QAccessibleInterface* ItemSelectorAccessibilityInterface::childAt(int x, int y) const
{
    if (selector()->isFullEnabled() && selector()->editor()->rect().contains(x, y))
        return child(ACCESSIBLE_EDITOR);

    if (selector()->eraseButton()->rect().contains(x, y))
        return child(ACCESSIBLE_ERASE);

    return nullptr;
}

int ItemSelectorAccessibilityInterface::indexOfChild(const QAccessibleInterface* child) const
{
    if (selector()->eraseButton() == child->object())
        return ACCESSIBLE_ERASE;
    if (selector()->view() == child->object())
        return ACCESSIBLE_VIEW;
    if (selector()->isFullEnabled() && selector()->editor() == child->object())
        return ACCESSIBLE_EDITOR;

    return -1;
}

QString ItemSelectorAccessibilityInterface::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
        case QAccessible::Name:
#ifndef Q_OS_UNIX
            // Взято из QAccessibleComboBox::text
            // on Linux we use relations for this, name is text (fall through to Value)
            str = QAccessibleWidget::text(t);
            break;
#endif
        case QAccessible::Value:
            if (selector()->isFullEnabled())
                str = selector()->editorText();
            else
                str = selector()->currentText();
            break;
        case QAccessible::Accelerator:
            str = QKeySequence(Qt::Key_Down).toString(QKeySequence::NativeText);
            break;
        default:
            break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return str;
}

QStringList ItemSelectorAccessibilityInterface::actionNames() const
{
    return QStringList() << showMenuAction() << pressAction();
}

QString ItemSelectorAccessibilityInterface::localizedActionDescription(const QString& action_name) const
{
    if (action_name == showMenuAction() || action_name == pressAction())
        return utf("Open the ItemSelector selection popup"); // TODO translate
    return QString();
}

void ItemSelectorAccessibilityInterface::doAction(const QString& action_name)
{
    if (action_name == showMenuAction() || action_name == pressAction()) {
        if (selector()->view()->isVisible()) {
            selector()->popupHide(true);
        } else {
            selector()->popupShow();
        }
    }
}

QStringList ItemSelectorAccessibilityInterface::keyBindingsForAction(const QString&) const
{
    return QStringList();
}

ItemSelector* ItemSelectorAccessibilityInterface::selector() const
{
    return qobject_cast<ItemSelector*>(widget());
}

ItemSelectorEditor::ItemSelectorEditor(ItemSelector* selector)
    : LineEdit(selector)
    , _selector(selector)
    , _focus_timer(new QTimer(this))
{
    setStandardReadOnlyBehavior(false);

    setFrame(false);
    _focus_timer->setSingleShot(true);
    _focus_timer->setInterval(0);
    connect(_focus_timer, &QTimer::timeout, this, [&]() {
        if (hasFocus())
            Utils::selectLineEditAll(this);
    });
}

bool ItemSelectorEditor::event(QEvent* event)
{
    if (_selector->itemModel() == nullptr) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::Clipboard:
            case QEvent::MouseButtonPress:
            case QEvent::InputMethodQuery:
            case QEvent::ContextMenu:
                return true;

            default:
                break;
        }
    }

    return LineEdit::event(event);
}
void ItemSelectorEditor::focusInEvent(QFocusEvent* event)
{
    LineEdit::focusInEvent(event);

    _selector->update();
    // был установле focusProxy. При этом главный виджет почему не получает сообщения о смене фокуса
    QApplication::sendEvent(_selector, event);

    if (!_is_edit_menu_active && _selector->_select_all_on_focus && !isReadOnly())
        _focus_timer->start();
}

void ItemSelectorEditor::focusOutEvent(QFocusEvent* event)
{
    LineEdit::focusOutEvent(event);
    _selector->update();
    // был установле focusProxy. При этом главный виджет почему не получает сообщения о смене фокуса
    QApplication::sendEvent(_selector, event);

    if (!isReadOnly() && !_is_edit_menu_active) {
        deselect();
        setCursorPosition(0);
    }
}

void ItemSelectorEditor::initStyleOption(QStyleOptionFrame* option) const
{
    LineEdit::initStyleOption(option);
}

void ItemSelectorEditor::contextMenuEvent(QContextMenuEvent* event)
{
    LineEdit::contextMenuEvent(event);
}

void ItemSelectorEditor::onContextMenuCreated(QMenu* m)
{
    _is_edit_menu_active = true;
    connect(m, &QMenu::destroyed, this, [this]() { _is_edit_menu_active = false; });
}

void ItemSelectorEditor::highlight(bool b)
{
    _highlighted = b;
    Utils::updatePalette(_selector->isFullEnabled(), b, this);
}

ItemSelectorMemo::ItemSelectorMemo(ItemSelector* selector)
    : PlainTextEdit(selector)
    , _selector(selector)
{
    setReadOnly(true);
    setReadOnlyBackground(false);
    setFrameShape(QFrame::NoFrame);
    setViewportMargins(-2, memo_top_margin(), -2, memo_bottom_margin());
    setContentsMargins(0, 0, 0, 0);
    setStyleSheet(QStringLiteral("QFrame {border: none }"));
}

bool ItemSelectorMemo::event(QEvent* event)
{
    return PlainTextEdit::event(event);
}

void ItemSelectorMemo::focusInEvent(QFocusEvent* event)
{
    PlainTextEdit::focusInEvent(event);
}

void ItemSelectorMemo::focusOutEvent(QFocusEvent* event)
{
    PlainTextEdit::focusOutEvent(event);
}

void ItemSelectorMemo::paintEvent(QPaintEvent* event)
{
    PlainTextEdit::paintEvent(event);

    /* попытка отрисовки текста в строку - пусть будет
    QPainter painter(viewport());

    QString text = toPlainText().simplified();

    QStyleOption opt;
    opt.initFrom(this);

    int m = document()->documentMargin();

    QRect r = viewport()->rect();
    r.moveTopLeft(QPoint(m, m));
    r.setWidth(r.width() - m * 2);
    r.setHeight(r.height() - m * 2);

    style()->drawItemText(&painter, r, 0, opt.palette, true, text, foregroundRole());*/
}

void ItemSelectorMemo::initStyleOption(QStyleOptionFrame* option) const
{
    PlainTextEdit::initStyleOption(option);
}

QSize ItemSelectorMemo::sizeHint() const
{
    return PlainTextEdit::sizeHint();
}

void ItemSelectorMemo::highlight(bool b)
{
    _highlighted = b;
    Utils::updatePalette(_selector->isFullEnabled(), b, this);
}

ItemSelectorContainer* ItemSelector::container() const
{
    if (_destroing)
        return nullptr;

    if (_container.isNull()) {
        _container = new ItemSelectorContainer(const_cast<ItemSelector*>(this));
        _container->setObjectName(QStringLiteral("zfc"));
        _container->hide();
        _container->installEventFilter(const_cast<ItemSelector*>(this));
        view()->installEventFilter(const_cast<ItemSelector*>(this));
    }
    return _container;
}

ItemSelector::PopupMode ItemSelector::popupModeInternal() const
{
    if (_selection_mode == SelectionMode::Multi)
        return PopupMode::PopupDialog;
    return _popup_mode;
}

void ItemSelector::updateEditorReadOnly()
{
    if (popupModeInternal() == PopupMode::PopupDialog) {
        _default_editor->setReadOnly(true);
        Z_CHECK(!isPopup());
    } else if (popupModeInternal() == PopupMode::PopupComboBox) {
        _default_editor->setReadOnly(_read_only);

    } else
        Z_HALT_INT;
}

ItemSelector::EditorMode ItemSelector::editorMode() const
{
    if (_selection_mode == SelectionMode::Single)
        return EditorMode::LineEdit;

    return _multiline_editor->maximumLines() > 1 ? EditorMode::Memo : EditorMode::Label;
}

void ItemSelector::prepareEditor()
{
    updateEditorVisibility();

    if (editorMode() == EditorMode::Memo)
        setFocusProxy(_multiline_editor);
    else if (editorMode() == EditorMode::LineEdit)
        setFocusProxy(_default_editor);
    else if (editorMode() == EditorMode::Label)
        setFocusProxy(_multiline_single_editor);
    else
        Z_HALT_INT;

    updateEditorReadOnly();
    updateGeometry();
    updateControlsGeometry();
}

void ItemSelector::updateEditorVisibility()
{
    _default_editor->setVisible(_model != nullptr && editorMode() == EditorMode::LineEdit);
    _multiline_editor->setVisible(_model != nullptr && editorMode() == EditorMode::Memo);
    _multiline_single_editor->setVisible(_model != nullptr && editorMode() == EditorMode::Label);
}

void ItemSelector::beginSearch()
{
    if (_status == Searching)
        cancelSearch();

    _status = Searching;
    showSearchStatus();
    emit sg_statusChanged(_status);
}

void ItemSelector::cancelSearch()
{
    Z_CHECK(_status == Searching);
    hideSearchStatus();
    _status = Ready;
    emit sg_statusChanged(_status);
}

void ItemSelector::endSearch()
{
    Z_CHECK(_status == Searching);
    hideSearchStatus();
    _status = Ready;
    emit sg_statusChanged(_status);
}

void ItemSelector::showSearchStatus()
{
#if 0 // не используем, но пусть будет  
    Z_CHECK(_status != Searching);
    updateControlsGeometry();    
    _wait_movie_label->show();
    _wait_movie_label->movie()->start();
#endif
}

void ItemSelector::hideSearchStatus()
{
#if 0 // не используем, но пусть будет  
    Z_CHECK(_status == Searching);
    _wait_movie_label->hide();
    _wait_movie_label->movie()->stop();
#endif
}

void ItemSelector::updatePopupGeometry()
{
    if (container() == nullptr || !_popup)
        return;

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QRect screen_geometry = Utils::screenRect(this, Qt::AlignLeft | Qt::AlignBottom);
    if (screen_geometry.isNull())
        return;

    QRect list_rect(style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxListBoxPopup, this));

    QPoint below = mapToGlobal(list_rect.bottomLeft());
    int below_height = screen_geometry.bottom() - below.y();
    QPoint above = mapToGlobal(list_rect.topLeft());
    int above_height = above.y() - screen_geometry.y();

    int height_count = 1;
    if (view()->model() != nullptr) {
        if (view()->model()->rowCount() > 0)
            list_rect.setHeight(view()->visualRect(view()->model()->index(0, _display_column)).height() + Utils::scaleUI(1));

        if (container()->_list_view != nullptr)
            height_count = qMax(1, qMin(view()->model()->rowCount(), _view_height_count));
        else
            height_count = _view_height_count;
    }

    list_rect.setHeight(list_rect.height() * height_count);
    if (height_count > 1) {
        int spacing = 0;
        if (container()->_list_view != nullptr)
            spacing = container()->_list_view->spacing();
        list_rect.setHeight(list_rect.height() + (height_count - 1) * spacing);
    }
    list_rect.setHeight(list_rect.height() + 2);

    if (list_rect.height() <= below_height) {
        list_rect.moveTopLeft(below);
    } else if (list_rect.height() <= above_height) {
        list_rect.moveBottomLeft(above);
    } else if (below_height >= above_height) {
        list_rect.setHeight(below_height);
        list_rect.moveTopLeft(below);
    } else {
        list_rect.setHeight(above_height);
        list_rect.moveBottomLeft(above);
    }

    if (!fuzzyCompare(_view_width_count, 1)) {
        int old_width = list_rect.width();
        list_rect.setWidth(static_cast<int>(list_rect.width() * _view_width_count));
        list_rect.moveLeft(list_rect.left() - (list_rect.width() - old_width));
    }

    container()->setGeometry(list_rect);
}

ItemSelectorListView::ItemSelectorListView(ItemSelector* selector, QWidget* parent)
    : QListView(parent)
    , _selector(selector)
{
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFocusPolicy(Qt::NoFocus);
}

void ItemSelectorListView::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);
}

void ItemSelectorListView::mousePressEvent(QMouseEvent* event)
{
    QListView::mousePressEvent(event);

    if (indexAt(event->pos()).isValid())
        _selector->popupHide(true);
}

void ItemSelectorListView::resizeEvent(QResizeEvent* event)
{
    resizeContents(viewport()->width(), contentsSize().height());
    QListView::resizeEvent(event);
}

void ItemSelectorListView::hideEvent(QHideEvent* event)
{
    QListView::hideEvent(event);
    _selector->popupHide(false);
}

QSize ItemSelectorListView::sizeHint() const
{
    return {};
}

QSize ItemSelectorListView::minimumSizeHint() const
{
    return sizeHint();
}

QStyleOptionViewItem ItemSelectorListView::viewOptions() const
{
    QStyleOptionViewItem option = QListView::viewOptions();
    option.showDecorationSelected = true;
    option.font = parentWidget()->font();
    return option;
}

ItemSelectorTreeView::ItemSelectorTreeView(ItemSelector* selector, QWidget* parent)
    : QTreeView(parent)
    , _selector(selector)
{
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(false);
    setFocusPolicy(Qt::NoFocus);
    setHeaderHidden(true);
    //    header()->setStretchLastSection(true);
}

void ItemSelectorTreeView::keyPressEvent(QKeyEvent* event)
{
    QTreeView::keyPressEvent(event);
}

void ItemSelectorTreeView::mousePressEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    bool expanded = index.isValid() ? isExpanded(index) : false;

    QTreeView::mousePressEvent(event);

    if (index.isValid()) {
        if (isExpanded(index) == expanded)
            _selector->popupHide(true);
    }
}

void ItemSelectorTreeView::resizeEvent(QResizeEvent* event)
{
    QTreeView::resizeEvent(event);
}

void ItemSelectorTreeView::hideEvent(QHideEvent* event)
{
    QTreeView::hideEvent(event);
    _selector->popupHide(false);
}

QSize ItemSelectorTreeView::sizeHint() const
{
    return {};
}

QSize ItemSelectorTreeView::minimumSizeHint() const
{
    return sizeHint();
}

QStyleOptionViewItem ItemSelectorTreeView::viewOptions() const
{
    QStyleOptionViewItem option = QTreeView::viewOptions();
    option.showDecorationSelected = true;
    option.font = parentWidget()->font();
    return option;
}

QObjectData* ItemSelectorTreeView::privateData() const
{
    return d_ptr.data();
}

ItemSelectorContainer::ItemSelectorContainer(ItemSelector* selector)
    : QFrame(selector, Qt::Popup)
    , _selector(selector)
{
    setFrameShape(StyledPanel);
    setAttribute(Qt::WA_WindowPropagation);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setObjectName("zfla");
    layout->setSpacing(0);
    layout->setMargin(0);

    if (selector->datasetProperty().isValid() && selector->datasetProperty().dataType() == DataType::Tree) {
        _tree_view = new ItemSelectorTreeView(selector, this);
        _tree_view->setObjectName(QStringLiteral("zf_tree"));
        _tree_view->setItemDelegate(new ItemSelectorDelegate(_tree_view, selector));
        layout->addWidget(_tree_view);
    } else {
        _list_view = new ItemSelectorListView(selector, this);
        _list_view->setObjectName(QStringLiteral("zf_list"));
        _list_view->setItemDelegate(new ItemSelectorDelegate(_list_view, selector));
        layout->addWidget(_list_view);
    }
}

ItemSelectorContainer::~ItemSelectorContainer()
{
}

void ItemSelectorContainer::paintEvent(QPaintEvent* e)
{
    QFrame::paintEvent(e);
}

ItemSelector::ItemSelector(QWidget* parent)
    : QWidget(parent)

{
    init();
}

ItemSelector::ItemSelector(QAbstractItemModel* model, QWidget* parent)
    : QWidget(parent)
{
    init();
    setItemModel(model);
}

ItemSelector::ItemSelector(const ModelPtr& model, const DataProperty& dataset, const DataFilterPtr& data_filter, QWidget* parent)
    : QWidget(parent)
{
    init();
    setModel(model, dataset, data_filter);
}

ItemSelector::ItemSelector(const Uid& entity_uid, const PropertyID& dataset_id, QWidget* parent)
    : QWidget(parent)
{
    init();
    setModel(entity_uid, dataset_id);
}

ItemSelector::~ItemSelector()
{
    _destroing = true;

    if (_multiline_editor)
        _multiline_editor->objectExtensionDestroy();

    if (_container != nullptr)
        delete _container;

    delete _object_extension;
}

bool ItemSelector::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void ItemSelector::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant ItemSelector::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool ItemSelector::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void ItemSelector::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void ItemSelector::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void ItemSelector::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void ItemSelector::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void ItemSelector::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

ItemSelector::PopupMode ItemSelector::popupMode() const
{
    return _popup_mode;
}

void ItemSelector::setPopupMode(ItemSelector::PopupMode m)
{
    if (_popup_mode == m)
        return;

    _popup_mode = m;

    updateEditorReadOnly();

    emit sg_popupModeChanged();
}

SelectionMode::Enum ItemSelector::selectionMode() const
{
    return _selection_mode;
}

void ItemSelector::setSelectionMode(SelectionMode::Enum m)
{
    if (_selection_mode == m)
        return;

    _selection_mode = m;

    if (_selection_mode == SelectionMode::Single)
        setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Minimum);
    else
        setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);

    prepareEditor();

    emit sg_selectionModeChanged();
}

bool ItemSelector::onlyBottomItem() const
{
    return _only_bottom_item;
}

void ItemSelector::setOnlyBottomItem(bool b)
{
    _only_bottom_item = b;
}

ItemSelector::Status ItemSelector::status() const
{
    return _status;
}

QAbstractItemModel* ItemSelector::itemModel() const
{
    return _model;
}

void ItemSelector::setItemModel(QAbstractItemModel* model)
{
    if (_destroing)
        return;

    setItemModelHelper(model);
}

ModelPtr ItemSelector::model() const
{
    return _model_ptr;
}

DataFilterPtr ItemSelector::filter() const
{
    return _data_filter;
}

void ItemSelector::setModel(const ModelPtr& model, const DataProperty& dataset, const DataFilterPtr& data_filter)
{
    Z_CHECK_NULL(model);
    Z_CHECK(dataset.propertyType() == PropertyType::Dataset);

    if (_model_ptr == model && _dataset == dataset && _data_filter == data_filter)
        return;

    if (data_filter != nullptr)
        Z_CHECK(data_filter->source() == model.get());

    if (!dataset.isValid())
        _dataset = model->structure()->singleDataset();
    else
        _dataset = dataset;

    _model_ptr = model;
    _entity_uid = model->entityUid();
    _dataset_id = dataset.id();
    _goto_entity.reset();
    _goto_button->setToolTip("");

    if (!_container.isNull()) {
        delete _container;
        _container = nullptr;
    }

    _model_ptr->data()->initDataset(_dataset, 0);

    if (_data_filter != nullptr) {
        disconnect(_data_filter.get(), &DataFilter::sg_refiltered, this, &ItemSelector::sl_refiltered);
    }

    if (data_filter != nullptr) {
        connect(data_filter.get(), &DataFilter::sg_refiltered, this, &ItemSelector::sl_refiltered);
        setItemModelHelper(data_filter->proxyDataset(dataset));
    } else {
        setItemModelHelper(_model_ptr->data()->dataset(_dataset));
    }

    _data_filter = data_filter;

    connect(_model_ptr.get(), &Model::sg_finishLoad, this, &ItemSelector::sl_model_finishLoad);
    connect(_model_ptr.get(), &Model::sg_invalidateChanged, this, &ItemSelector::sl_model_invalidateChanged);

    if (_model_ptr->isLoading())
        _async_loading = true;

    auto id_columns = _dataset.columnsByOptions(PropertyOption::Id);
    if (!id_columns.isEmpty())
        _id_column = id_columns.constFirst().pos();

    auto names_columns = _dataset.columnsByOptions(PropertyOption::FullName);
    if (names_columns.isEmpty())
        names_columns = _dataset.columnsByOptions(PropertyOption::Name);
    if (!names_columns.isEmpty())
        setDisplayColumn(names_columns.constFirst().pos());
}

void ItemSelector::setModel(const Uid& entity_uid, const PropertyID& dataset_id)
{
    _entity_uid = entity_uid;
    if (!_entity_uid.isValid()) {
        Z_CHECK(!dataset_id.isValid());
        setItemModel(nullptr);
        return;
    }

    // Создаем модель
    if (!dataset_id.isValid())
        _dataset_id = DataStructure::structure(_entity_uid)->singleDatasetId();
    else
        _dataset_id = dataset_id;

    auto model = Core::getModel<Model>(_entity_uid, {_dataset_id});
    setModel(model, model->structure()->property(_dataset_id));

    connect(_model_ptr.get(), &Model::sg_finishLoad, this, &ItemSelector::sl_model_finishLoad);
    connect(_model_ptr.get(), &Model::sg_invalidateChanged, this, &ItemSelector::sl_model_invalidateChanged);
}

DataProperty ItemSelector::datasetProperty() const
{
    return _dataset;
}

bool ItemSelector::hasFilteredValue(const QVariant& v) const
{
    DataContainerPtr c;
    DataHashed* h;
    if (_data_filter == nullptr) {
        Z_CHECK_NULL(_model_ptr);
        c = _model_ptr->data();
        h = c->hash();
    } else {
        c = _data_filter->data();
        h = _data_filter->hash();
    }
    Z_CHECK(c->contains(_dataset));
    Z_CHECK(_id_column >= 0);
    Z_CHECK(_dataset.columns().count() > _id_column);
    auto col = _dataset.columns().at(_id_column);
    return !h->findRows(col, v).isEmpty();
}

void ItemSelector::setDisplayColumn(int c)
{
    Z_CHECK(c >= 0);

    bool changed = (_display_column == c);

    _display_column = c;

    if (container()->_list_view) {
        container()->_list_view->setModelColumn(c);
    } else {
        container()->_tree_view->setTreePosition(c);
    }

    if (changed) {
        updateTreeViewColumns();
        updateText(false);
    }

    _completer->setCompletionColumn(c);
}

void ItemSelector::setDisplayColumn(const DataProperty& column)
{
    setDisplayColumn(column.pos());
}

int ItemSelector::displayColumn() const
{
    return _display_column;
}

int ItemSelector::displayRole() const
{
    return _display_role;
}

void ItemSelector::setDisplayRole(int role)
{
    if (_display_role == role)
        return;

    _display_role = role;
    _completer->setCompletionRole(role);
    updateText(false);
}

QString ItemSelector::displayText() const
{
    if (_editable)
        return _editable_text;

    return calcDisplayText();
}

void ItemSelector::setIdColumn(int c)
{
    Z_CHECK(c >= 0);
    if (_id_column == c)
        return;

    _id_column = c;
}

void ItemSelector::setIdColumn(const DataProperty& column)
{
    setIdColumn(column.pos());
}

int ItemSelector::idColumn() const
{
    return _id_column >= 0 ? _id_column : displayColumn();
}

int ItemSelector::idRole() const
{
    return _id_role >= 0 ? _id_role : displayRole();
}

void ItemSelector::setIdRole(int role)
{
    if (_id_role == role)
        return;

    _id_role = role;
}

bool ItemSelector::isEditable() const
{
    return _editable;
}

void ItemSelector::setEditable(bool b)
{
    if (_editable == b)
        return;

    _editable = b;
}

void ItemSelector::setCurrentText(const QString& s)
{
    Z_CHECK(_editable);
    if (_editable_text == s)
        return;

    _editable_text = s;
    _default_editor->setText(s);
    emit sg_changed(s);
}

QString ItemSelector::currentText() const
{
    QString lookup_text = Utils::variantToString(currentIndex().data(_display_role));

    if (_editable && !comp(_editable_text, lookup_text))
        return _editable_text;

    return lookup_text;
}

void ItemSelector::setMultiSelectionLevels(const QList<int>& levels)
{
    _multi_selection_levels = levels;
}

QList<int> ItemSelector::multiSelectionLevels() const
{
    return _multi_selection_levels;
}

QModelIndex ItemSelector::currentIndex() const
{
    Z_CHECK(_selection_mode == SelectionMode::Single);

    QModelIndex idx = view()->currentIndex();
    if (idx.isValid())
        return view()->model()->index(idx.row(), 0, idx.parent());
    return idx;
}

int ItemSelector::currentRow() const
{
    return currentIndex().row();
}

QVariant ItemSelector::currentValue() const
{
    auto index = currentIndex();
    if (!index.isValid())
        return QVariant();

    return index.model()->index(index.row(), idColumn(), index.parent()).data(idRole());
}

QVariantList ItemSelector::currentValues() const
{
    Z_CHECK(_selection_mode == SelectionMode::Multi);
    return _current_values;
}

bool ItemSelector::setCurrentValues(const QVariantList& values)
{
    Z_CHECK(_selection_mode == SelectionMode::Multi);
    if (_current_values.count() == values.count()) {
        QSet<QString> old_values;
        for (auto& v : qAsConst(_current_values)) {
            old_values << Utils::variantToString(v);
        }

        QSet<QString> new_values;
        for (auto& v : qAsConst(values)) {
            new_values << Utils::variantToString(v);
        }

        if (old_values == new_values)
            return false;
    }

    _current_values = values;
    _current_values_text.clear();
    lookupToExactValue(false);

    emit sg_changedMulti(_current_values);
    return true;
}

bool ItemSelector::setCurrentIndex(const QModelIndex& index, bool by_user, bool by_keys, bool clear)
{
    Z_CHECK(_selection_mode == SelectionMode::Single);

    if (_model == nullptr)
        return false;

    QModelIndex current = _before_popup_index != nullptr ? *_before_popup_index : currentIndex();
    QModelIndex real_index;
    if (index.isValid())
        real_index = _model->index(index.row(), _display_column, index.parent());

    if (current.row() != real_index.row()) {
        view()->setCurrentIndex(real_index);
        view()->scrollTo(real_index, QAbstractItemView::PositionAtTop);

        if (!clear) {
            updateExactValue(by_user);
            if (_exact_value != nullptr)
                _exact_value_found = view()->currentIndex() == real_index ? ExactValueStatus::Found : ExactValueStatus::NotFound;
        }

        emit sg_changed(real_index);
        if (by_user)
            emit sg_edited(real_index, by_keys);

        _last_selected = real_index;

        updateText(by_user);

    } else {
        _last_selected = real_index;
    }

    if (clear) {
        _exact_value.reset();
        _exact_value_found = ExactValueStatus::Undefined;
        updateText(by_user);
    }

    return current.row() != real_index.row();
}

void ItemSelector::setCurrentValue(const QVariant& value, int column, int role)
{
    setCurrentValueHelper(value, column, role, false);
}

void ItemSelector::setCurrentValueHelper(const QVariant& value, int column, int role, bool by_user)
{
    Z_CHECK(_selection_mode == SelectionMode::Single);

    _exact_value = Z_MAKE_SHARED(QVariant, value);
    _exact_value_column = column;
    _exact_value_role = role;
    _exact_value_by_user = by_user;

    lookupToExactValue(by_user);
}
void ItemSelector::setCurrentValue(const QVariant& value, const DataProperty& column, int role)
{
    Z_CHECK(_model_ptr != nullptr);
    int column_pos = _dataset.columnPos(column);
    setCurrentValue(value, column_pos, role);
}

bool ItemSelector::isMatchContains() const
{
    return _match & Qt::MatchContains;
}

void ItemSelector::setMatchContains(bool b)
{
    Qt::MatchFlags new_match = b ? Qt::MatchContains : Qt::MatchStartsWith;
    if (_match == new_match)
        return;

    _match = new_match;
}

bool ItemSelector::isCurrentValueFound() const
{
    Z_CHECK(_selection_mode == SelectionMode::Single);
    return _exact_value_found == ExactValueStatus::Found;
}

bool ItemSelector::hasCurrentValue() const
{
    return _exact_value != nullptr;
}

void ItemSelector::clear()
{
    clearHelper(false);
}

void ItemSelector::clearHelper(bool by_user)
{
    if (_selection_mode == SelectionMode::Single) {
        setCurrentIndex(QModelIndex(), by_user, false, true);
        if (isEditable())
            setCurrentText(QString());

    } else {
        setCurrentValues({});
    }
}

QString ItemSelector::placeholderText() const
{
    return _default_editor->placeholderText();
}

void ItemSelector::setPlaceholderText(const QString& s)
{
    _default_editor->setPlaceholderText(s);
    _multiline_editor->setPlaceholderText(s);
}

void ItemSelector::setOverrideText(const QString& text)
{
    if (_override_text == text)
        return;

    _override_text = text;

    updateText(false);
}

int ItemSelector::popupHeightCount() const
{
    return _view_height_count;
}

void ItemSelector::setPopupHeightCount(int count)
{
    _view_height_count = qBound(3, count, 20);
}

bool ItemSelector::isFullEnabled() const
{
    if (popupModeInternal() == PopupMode::PopupDialog)
        return _model != nullptr && !isReadOnly() && !isAsyncLoading();
    else
        return _model != nullptr && !isReadOnly() && isEnabled() && !isAsyncLoading();
}

void ItemSelector::setEraseEnabled(bool b)
{
    if (_is_erase_enabled == b)
        return;

    _is_erase_enabled = b;
    _update_enabled_timer->start();
}

bool ItemSelector::isEraseEnabled() const
{
    return _is_erase_enabled;
}

Uid ItemSelector::gotoEntity() const
{
    if (_model_ptr == nullptr)
        return {};

#if false // отключаем функционал
    if (_goto_entity == nullptr) {
        if (Core::isCatalogUid(_model_ptr->entityUid())) {
            auto catalog_id = Core::catalogId(_model_ptr->entityUid());
            auto m = Core::catalogModel(catalog_id);
            if (m->isInvalidated() || m->isLoading())
                return {}; // не запоминаем _goto_entity

            _goto_entity = std::make_unique<Uid>(Core::catalogMainEntityUid(catalog_id));
        }

        if (_goto_entity == nullptr) {
            _goto_entity = std::make_unique<Uid>();
            if (Core::connectionInformation()->directPermissions().hasRight(_model_ptr->entityUid().code(), AccessFlag::View))
                *_goto_entity = _model_ptr->entityUid();
        }

        if (_goto_entity != nullptr && _goto_entity->isValid())
            _goto_button->setToolTip(ZF_TR(ZFT_GOTO).arg(Core::getPlugin(_goto_entity->code())->getModuleInfo().name()));
    }

    return *_goto_entity;
#else
    return Uid();
#endif
}

bool ItemSelector::isReadOnly() const
{
    return _read_only;
}

void ItemSelector::setReadOnly(bool b)
{
    if (_read_only == b)
        return;

    _read_only = b;

    // для режима Dialog: _editor всегда находимся в режиме только для чтения
    if (popupModeInternal() == PopupMode::PopupComboBox)
        _default_editor->setReadOnly(b);

    sl_updateEnabled();
    updateGeometry();
    updateControlsGeometry();

    emit sg_readOnlyChanged();
}

bool ItemSelector::isSelectAllOnFocus() const
{
    return _select_all_on_focus;
}

void ItemSelector::setSelectAllOnFocus(bool b)
{
    _select_all_on_focus = b;
}

bool ItemSelector::isPopupEnter() const
{
    return _is_popup_enter;
}

void ItemSelector::setPopupEnter(bool b)
{
    _is_popup_enter = b;
}

void ItemSelector::setFrame(bool b)
{
    if (_has_frame == b)
        return;

    _has_frame = b;
    _default_editor->update();
    _multiline_editor->update();
    update();
}

bool ItemSelector::hasFrame() const
{
    return _has_frame;
}

int ItemSelector::maximumButtonsSize() const
{
    return _maximum_buttons_size;
}

void ItemSelector::setMaximumButtonsSize(int n)
{
    if (_maximum_buttons_size == n)
        return;

    _maximum_buttons_size = n;
    updateControlsGeometry();
}

int ItemSelector::maximumLines() const
{
    return _multiline_editor->maximumLines();
}

void ItemSelector::setMaximumLines(int n)
{
    Z_CHECK(_selection_mode == SelectionMode::Multi);

    if (n == _multiline_editor->maximumLines())
        return;

    _multiline_editor->setMaximumLines(n);
    updateControlsGeometry();
    _update_enabled_timer->start();
}

bool ItemSelector::allowEmptySelection() const
{
    return _allow_empty_selection;
}

void ItemSelector::setAllowEmptySelection(bool b)
{
    if (b == _allow_empty_selection)
        return;

    Z_CHECK(_selection_mode == SelectionMode::Multi);
    _allow_empty_selection = b;

    _update_enabled_timer->start();
}

bool ItemSelector::isAsyncLoading() const
{
    return _async_loading;
}

LineEdit* ItemSelector::editorSingle() const
{
    return _default_editor;
}

PlainTextEdit* ItemSelector::editorMulti() const
{
    return _multiline_editor;
}

QWidget* ItemSelector::editor() const
{
    if (_selection_mode == SelectionMode::Single)
        return _default_editor;

    if (_multiline_editor->maximumLines() == 1)
        return _multiline_single_editor;
    else
        return _multiline_editor;
}

QString ItemSelector::editorText() const
{
    if (_selection_mode == SelectionMode::Single)
        return _default_editor->text();

    if (_multiline_editor->maximumLines() == 1)
        return _multiline_single_editor->text();
    else
        return _multiline_editor->toPlainText();
}

QToolButton* ItemSelector::eraseButton() const
{
    return _erase_button;
}

QToolButton* ItemSelector::gotoButton() const
{
    return _goto_button;
}

QSize ItemSelector::sizeHint() const
{
    QSize size = editorSizeHint();
    if (gotoEntity().isValid())
        size.setWidth(size.width() + gotoSizeHint().width());
    if (!isReadOnly())
        size.setWidth(size.width() + arrowSizeHint().width());

    size.setWidth(size.width() + margin() * 2);
    size.setHeight(size.height() + margin() * 2);

    return size;
}

QSize ItemSelector::minimumSizeHint() const
{
    return sizeHint();
}

bool ItemSelector::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateControlsGeometry();
            break;

        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
            if (const QHoverEvent* he = static_cast<const QHoverEvent*>(event))
                updateHoverControl(he->pos());
            break;
        default:
            break;
    }

    if (_model_ptr != nullptr && (event->type() == QEvent::Show || event->type() == QEvent::ParentChange) && Utils::isVisible(this)) {
        //! Поменялась видимость, надо загрузить данные если не загружены
        Framework::internalCallbackManager()->addRequest(this, Framework::ITEM_SELECTOR_LOAD_MODEL_KEY);
    }

    return QWidget::event(event);
}

void ItemSelector::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::StyleChange:
        case QEvent::FontChange:
            updateControlsGeometry();
            break;
        default:
            break;
    }
    QWidget::changeEvent(event);
}

void ItemSelector::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateControlsGeometry();
    updateGeometry();
}

bool ItemSelector::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _default_editor) {
        switch (event->type()) {
            case QEvent::EnabledChange: {
                if (_default_editor->testAttribute(Qt::WA_Disabled)) {
                    _default_editor->setAttribute(Qt::WA_Disabled, false);
                    if (popupModeInternal() == PopupMode::PopupComboBox)
                        _default_editor->setReadOnly(!isEnabled());
                }

                sl_updateEnabled();
                return true;
            }
            default:
                break;
        }
    }

    if (isFullEnabled()) {
        if (watched == _default_editor || watched == _multiline_editor->viewport() || watched == _multiline_editor || watched == _multiline_single_editor) {
            if (_eat_focus_out && event->type() == QEvent::FocusOut) {
                if (view()->isVisible())
                    return true;
            }

            switch (event->type()) {
                case QEvent::HoverEnter:
                case QEvent::HoverLeave:
                case QEvent::HoverMove: {
                    if (const QHoverEvent* he = static_cast<const QHoverEvent*>(event))
                        updateHoverControl(he->pos());
                    break;
                }

                case QEvent::MouseButtonDblClick: {
                    if (popupModeInternal() == PopupMode::PopupDialog) {
                        dialogShow("");
                        return true;
                    }

                    if (currentText().isEmpty()) {
                        popupShow();
                        return true;
                    }
                    break;
                }

                case QEvent::KeyPress: {
                    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                    const int key = ke->key();

                    if (popupModeInternal() == PopupMode::PopupDialog) {
                        QString clipboard_text;
                        if (ke->matches(QKeySequence::Paste)) {
                            QClipboard* clipboard = QGuiApplication::clipboard();
                            clipboard_text = clipboard->text();
                        }

                        if (ke->key() != Qt::Key_Escape) {
                            if (ke->key() == Qt::Key_Delete && ((ke->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier)) {
                                // Очистка
                                clearHelper(true);
                                return true;
                            }

                            if (ke->key() == Qt::Key_Backspace) {
                                return true;
                            }

                            if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                                Dialog* a_dialog = qobject_cast<Dialog*>(QApplication::activeWindow());
                                if (a_dialog == nullptr || a_dialog->isDisableDefaultAction()) {
                                    dialogShow("");
                                    return true;
                                }
                            } else if (ke->key() == Qt::Key_Space && !_filter_timer->isActive()) {
                                dialogShow("");
                                return true;

                            } else {
                                if (!clipboard_text.isEmpty()) {
                                    dialogShow(clipboard_text);
                                    return true;

                                } else if (((ke->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier) || ke->modifiers() == Qt::ShiftModifier) {
                                    QString text = ke->text().simplified().trimmed();
                                    if (!text.isEmpty()) {
                                        if (!_filter_timer->isActive())
                                            _filter_text.clear();

                                        _filter_text += text;
                                        _filter_timer->start();
                                    }
                                }
                            }
                        }

                    } else {
                        switch (key) {
                            case Qt::Key_Escape:
                                if (ke->modifiers() == Qt::NoModifier) {
                                    popupHide(false);
                                    return true;
                                }
                                break;

                            case Qt::Key_Up:
                                if (ke->modifiers() == Qt::NoModifier) {
                                    itemUp();
                                    return true;
                                }
                                break;

                            case Qt::Key_Down:
                                if (ke->modifiers() == Qt::NoModifier) {
                                    itemDown();
                                    return true;
                                }
                                break;

                            case Qt::Key_Enter:
                            case Qt::Key_Return:
                            case Qt::Key_PageUp:
                            case Qt::Key_PageDown: {
                                if (ke->modifiers() == Qt::NoModifier && !isPopup()) {
                                    if (!_is_popup_enter && (key == Qt::Key_Enter || key == Qt::Key_Return)) {
                                        break;
                                    } else {
                                        popupShow();
                                        return true;
                                    }
                                }
                                break;
                            }
                            case Qt::Key_Backspace:
                                if (ke->modifiers() == Qt::ControlModifier && _erase_button->isEnabled()) {
                                    sl_erase();
                                    return true;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }

        } else if (watched == container()) {
            switch (event->type()) {
                case QEvent::KeyRelease:
                case QEvent::KeyPress: {
                    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                    const int key = ke->key();
                    const int row_count = view()->model()->rowCount();
                    const bool no_modifier = ke->modifiers() == Qt::NoModifier;

                    // qt почему-то пожирает QEvent::KeyPress для Qt::Key_Escape
                    if (event->type() == QEvent::KeyPress || key == Qt::Key_Escape) {
                        switch (key) {
                            case Qt::Key_Escape:
                                if (ke->modifiers() == Qt::NoModifier) {
                                    popupHide(false);
                                    return true;
                                }
                                break;

                            case Qt::Key_Backspace:
                                if (ke->modifiers() == Qt::ControlModifier && _erase_button->isEnabled()) {
                                    sl_erase();
                                    return true;
                                }
                                break;

                            case Qt::Key_Tab:
                                if (no_modifier)
                                    popupHide(true);
                                break;

                            case Qt::Key_Enter:
                            case Qt::Key_Return:
                                if (no_modifier) {
                                    popupHide(true);
                                    return true;
                                }
                                return false;

                            case Qt::Key_End:
                            case Qt::Key_Home:
                                if (ke->modifiers() & Qt::ControlModifier)
                                    return false;
                                break;

                            case Qt::Key_Up:
                                if (no_modifier) {
                                    itemUp();
                                    return true;
                                }
                                return false;

                            case Qt::Key_Down:
                                if (no_modifier) {
                                    itemDown();
                                    return true;
                                }

                                return false;

                            case Qt::Key_PageUp:
                                if (ke->modifiers() == Qt::ControlModifier) {
                                    scrollUp();
                                    return false;
                                }

                                if (row_count > 0 && no_modifier) {
                                    scrollPageUp();
                                    return true;
                                }
                                return false;

                            case Qt::Key_PageDown:
                                if (ke->modifiers() == Qt::ControlModifier) {
                                    scrollDown();
                                    return false;
                                }

                                if (row_count > 0 && no_modifier) {
                                    scrollPageDown();
                                    return true;
                                }
                                return false;
                        }

                        _eat_focus_out = false;
                        (static_cast<QObject*>(_default_editor))->event(ke);
                        _eat_focus_out = true;
                        if (event->isAccepted() || !view()->isVisible()) {
                            // widget lost focus, hide the popup
                            if (_default_editor && (!_default_editor->hasFocus()))
                                popupHide(false);
                            if (event->isAccepted())
                                return true;
                        }

                        if (ke->matches(QKeySequence::Cancel)) {
                            popupHide(false);
                            return true;
                        }
                    }

                    break;
                }

                case QEvent::MouseButtonPress: {
                    QMouseEvent* me = static_cast<QMouseEvent*>(event);
                    if (isPopup() && !container()->underMouse()) {
                        // нажатие за пределами popup при открытом popup
                        auto pos = mapFromGlobal(container()->mapToGlobal(me->pos()));
                        if (isErasePos(pos)) {
                            clearHelper(true);
                            popupHide(false);

                        } else if (isArrowPos(pos)) {
                            // Нажата кнопка комбобокса при открытом view. Надо проигнорировать нажатие, чтобы не было
                            // открытия view сразу после закрытия
                            _ignore_popup_click_timer->start();
                            popupHide(view()->currentIndex().isValid());
                        } else {
                            popupHide(false);
                        }
                        return true;
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void ItemSelector::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);

    painter.fillRect(rect(), Qt::white);
    if (_has_frame)
        Utils::paintBorder(&painter, rect());

    if (isFullEnabled() || (isAsyncLoading() && !isReadOnly())) {
        painter.setPen(palette().color(QPalette::Text));
        QStyleOptionComboBox opt_combo;
        initStyleOption(&opt_combo);
        painter.drawComplexControl(QStyle::CC_ComboBox, opt_combo);
    }
}

void ItemSelector::mousePressEvent(QMouseEvent* event)
{
    if (isFullEnabled()) {
        if (isArrowPos(event->pos())) {
            popupClicked();
        }
    }

    QWidget::mousePressEvent(event);
}

void ItemSelector::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
}

void ItemSelector::wheelEvent(QWheelEvent* e)
{
    e->ignore();
}

void ItemSelector::sl_erase()
{
    if (isPopup()) {
        popupHide(false);
    } else {
        clearHelper(true);
    }

    if (_selection_mode == SelectionMode::Single) {
        updateText(true);
        emit sg_edited(QModelIndex(), false);

    } else {
        emit sg_editedMulti({});
    }
}

void ItemSelector::sl_goto()
{
    Z_CHECK_NULL(_goto_entity);
    auto error = Core::windowManager()->openWindow(*_goto_entity);
    if (error.isError())
        Core::error(error);
}

void ItemSelector::sl_editTextEdited(const QString& s)
{
    popupShow();
    QModelIndex index = search(s, _display_column, _display_role, false);

    if (_editable) {
        if (index.isValid() && comp(s, Utils::variantToString(index.data(_display_role)))) {
            _editable_text.clear();
            emit sg_edited(QString());
            emit sg_changed(QString());

        } else {
            _block_update_text++;
            setCurrentIndex(QModelIndex(), true, false, false);
            _block_update_text--;

            _editable_text = s;
            emit sg_edited(_editable_text);
            emit sg_changed(_editable_text);
            return;
        }
    }

    _block_update_text++;
    setCurrentIndex(index, true, true, false);
    _block_update_text--;
}

void ItemSelector::sl_modelDestroyed()
{
    if (_destroing)
        return;

    setItemModel(nullptr);
}

void ItemSelector::sl_modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if (_selection_mode == SelectionMode::Multi) {
        lookupToExactValue(false);

    } else if (!currentIndex().isValid()
               || (currentRow() >= topLeft.row() && currentRow() <= bottomRight.row() && _display_column >= topLeft.column()
                   && _display_column <= bottomRight.column() && roles.contains(_display_role))) {
        lookupToExactValue(false);
    }

    emit sg_lookupModelChanged();
}

void ItemSelector::sl_modelRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)

    if (_selection_mode == SelectionMode::Multi) {
        lookupToExactValue(false);

    } else if (_selection_mode == SelectionMode::Single && (!currentIndex().isValid() || (currentRow() >= first && currentRow() <= last))) {
        lookupToExactValue(false);
    }

    emit sg_lookupModelChanged();
}

void ItemSelector::sl_modelColumnsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)

    if (_selection_mode == SelectionMode::Multi) {
        lookupToExactValue(false);

    } else if (!currentIndex().isValid() || (_display_column >= first && _display_column <= last)) {
        lookupToExactValue(false);
    }

    emit sg_lookupModelChanged();
}

void ItemSelector::sl_modelReset()
{
    lookupToExactValue(false);
    emit sg_lookupModelChanged();
}

void ItemSelector::sl_modelRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(row)
    lookupToExactValue(false);
    emit sg_lookupModelChanged();
}

void ItemSelector::sl_modelColumnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)
    Q_UNUSED(column)

    lookupToExactValue(false);
    emit sg_lookupModelChanged();
}

void ItemSelector::sl_model_finishLoad(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error)
    Q_UNUSED(load_options)
    Q_UNUSED(properties)
    _async_loading = false;
    _error_loading = error;

    lookupToExactValue(false);

    sl_updateEnabled();
    updateGeometry();
    updateControlsGeometry();
    updatePopupGeometry();

    emit sg_lookupModelLoaded();
    emit sg_lookupModelChanged();
}

void ItemSelector::sl_model_invalidateChanged(const DataProperty& p, bool invalidate)
{
    if (!invalidate || p != _dataset || objectExtensionDestroyed())
        return;

    Framework::internalCallbackManager()->addRequest(this, Framework::ITEM_SELECTOR_LOAD_MODEL_KEY);
}

QAbstractItemView* ItemSelector::view() const
{
    if (container()->_list_view)
        return container()->_list_view;
    return container()->_tree_view;
}

void ItemSelector::setItemModelHelper(QAbstractItemModel* model)
{
    if (model == _model)
        return;

    if (_model != nullptr) {
        disconnect(_model, &QAbstractItemModel::destroyed, this, &ItemSelector::sl_modelDestroyed);
        disconnect(_model, &QAbstractItemModel::dataChanged, this, &ItemSelector::sl_modelDataChanged);
        disconnect(_model, &QAbstractItemModel::rowsRemoved, this, &ItemSelector::sl_modelRowsRemoved);
        disconnect(_model, &QAbstractItemModel::columnsRemoved, this, &ItemSelector::sl_modelColumnsRemoved);
        disconnect(_model, &QAbstractItemModel::modelReset, this, &ItemSelector::sl_modelReset);
        disconnect(_model, &QAbstractItemModel::rowsMoved, this, &ItemSelector::sl_modelRowsMoved);
        disconnect(_model, &QAbstractItemModel::columnsMoved, this, &ItemSelector::sl_modelColumnsMoved);
        if (view() != nullptr && _selection_model_handle)
            disconnect(_selection_model_handle);

        _completer->setModel(nullptr);
    }

    if (_model_ptr != nullptr) {
        disconnect(_model_ptr.get(), &Model::sg_finishLoad, this, &ItemSelector::sl_model_finishLoad);
        disconnect(_model_ptr.get(), &Model::sg_invalidateChanged, this, &ItemSelector::sl_model_invalidateChanged);
    }

    _model = model;

    setView(model);

    if (model != nullptr) {
        connect(model, &QAbstractItemModel::destroyed, this, &ItemSelector::sl_modelDestroyed);
        connect(model, &QAbstractItemModel::dataChanged, this, &ItemSelector::sl_modelDataChanged);
        connect(model, &QAbstractItemModel::rowsRemoved, this, &ItemSelector::sl_modelRowsRemoved);
        connect(model, &QAbstractItemModel::columnsRemoved, this, &ItemSelector::sl_modelColumnsRemoved);
        connect(model, &QAbstractItemModel::modelReset, this, &ItemSelector::sl_modelReset);
        connect(model, &QAbstractItemModel::rowsMoved, this, &ItemSelector::sl_modelRowsMoved);
        connect(model, &QAbstractItemModel::columnsMoved, this, &ItemSelector::sl_modelColumnsMoved);
        if (view() != nullptr)
            _selection_model_handle = connect(view()->selectionModel(), &QItemSelectionModel::currentChanged, this, &ItemSelector::sl_updateEnabled);

        _completer->setModel(model);
        _completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
        _completer->setCompletionMode(QCompleter::InlineCompletion);
        _completer->setCompletionColumn(_display_column);
        _completer->setCompletionRole(_display_role);
        _default_editor->setCompleter(_completer);
        _completer->setWidget(this);
    }

    // значение может сбросится внутри ListView или TreeView
    setDisplayColumn(_display_column);

    updateText(false);
    sl_updateEnabled();
    updateGeometry();
    updateControlsGeometry();
}

void ItemSelector::initStyleOption(QStyleOptionComboBox* option) const
{
    if (option == nullptr)
        return;

    option->initFrom(this);
    option->editable = isFullEnabled();
    option->frame = _has_frame;
    if (hasFocus() && !isFullEnabled())
        option->state |= QStyle::State_Selected;

    option->subControls = QStyle::SC_All;
    option->subControls &= ~(QStyle::SC_ComboBoxListBoxPopup);

    if (!_has_frame) {
        option->subControls &= ~(QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField);
        option->state &= ~(QStyle::State_HasFocus | QStyle::State_FocusAtBorder);
    }

    if (_arrow_state == QStyle::State_Sunken) {
        option->activeSubControls = QStyle::SC_ComboBoxArrow;
        option->state |= _arrow_state;
    } else {
        option->activeSubControls = _hover_control;
    }

    if (isPopup())
        option->state |= QStyle::State_On;
}

void ItemSelector::updateControlsGeometry()
{
    _default_editor->setGeometry(editorRect());
    _multiline_editor->setGeometry(editorRect());
    _multiline_single_editor->setGeometry(editorRect());

    auto erase_rect = eraseRect();
    _erase_button->setGeometry(erase_rect);
    UiSizes::prepareEditorToolButton(font(), _erase_button, false);

    auto goto_rect = gotoRect();
    _goto_button->setGeometry(goto_rect);
    UiSizes::prepareEditorToolButton(font(), _goto_button, false);

#if 0 // не используем, но пусть будет  
    _wait_movie_label->setGeometry(waitMovieRect());
    _wait_movie_label->movie()->setScaledSize(waitMovieRect().size());
#endif

    update();
}

void ItemSelector::updateText(bool by_user)
{
    if (_model == nullptr || _block_update_text > 0 || isPopup())
        return;

    _update_text_by_user = _update_text_by_user | by_user;
    _update_text_timer->start();
}

void ItemSelector::updateTextHelper(bool by_user)
{
    if (_model == nullptr || _block_update_text > 0 || isPopup() || _block_update_text_helper)
        return;

    _block_update_text_helper = true;

    if (!_update_text_timer->isActive())
        _update_text_by_user = false;

    // какой текст должен быть виден с точки зрения пользователя
    QString text;
    if (_error_loading.isError()) {
        text = _error_loading.fullText();

    } else {
        if (_selection_mode == SelectionMode::Single) {
            if (!_override_text.isEmpty()) {
                text = _override_text;

            } else if (_model_ptr != nullptr && _model_ptr->data()->isInvalidated(_dataset)) {
                text = ZF_TR(ZFT_LOADING).toLower();

            } else if (_exact_value != nullptr && _exact_value_found == ExactValueStatus::NotFound) {
                if (isAsyncLoading()) {
                    text = ZF_TR(ZFT_LOADING).toLower();

                } else {
                    // ищем значение без учета фильтрации через прокси
                    auto source_model = Utils::getTopSourceModel(_model);
                    bool found = false;
                    if (source_model != _model) {
                        QModelIndex index = search(Utils::variantToString(*_exact_value).trimmed(), idColumn(), idRole(), true, source_model);

                        if (index.isValid()) {
                            // нашли отфильтрованное значение
                            found = true;
                            text = Utils::variantToString(source_model->index(index.row(), displayColumn(), index.parent()).data(_display_role));
                        }
                    }

                    if (!found) {
                        if (isReadOnly() || popupModeInternal() == PopupMode::PopupDialog)
                            text = ZF_TR(ZFT_ENTITY_NOT_FOUND) + " (" + Utils::variantToString(*_exact_value) + ")";
                        else
                            text = Utils::variantToString(*_exact_value);
                    }
                }

            } else {
                if (_editable && !view()->currentIndex().isValid())
                    text = _editable_text;
                else
                    text = Utils::variantToString(_model->data(view()->currentIndex(), _display_role));
            }

        } else if (_selection_mode == SelectionMode::Multi) {
            Z_CHECK(_current_values.count() == _current_values_text.count());

            if (!_override_text.isEmpty()) {
                text = _override_text;

            } else if (_model_ptr != nullptr && _model_ptr->data()->isInvalidated(_dataset)) {
                text = ZF_TR(ZFT_LOADING).toLower();

            } else {
                QStringList text_items;
                for (int i = 0; i < _current_values.count(); i++) {
                    if (_current_values_text.at(i) == nullptr) {
                        if (isAsyncLoading()) {
                            text_items.clear();
                            break;
                        } else {
                            text_items << ZF_TR(ZFT_ENTITY_NOT_FOUND) + " (" + Utils::variantToString(_current_values.at(i)) + ")";
                        }
                    } else {
                        text_items << *_current_values_text.at(i);
                    }
                }

                if (text_items.isEmpty() && isAsyncLoading()) {
                    text = ZF_TR(ZFT_LOADING).toLower();

                } else {
                    std::sort(text_items.begin(), text_items.end());
                    text = text_items.join(", ");
                }
            }

        } else {
            Z_HALT_INT;
        }
    }

    if (_default_editor->text() != text) {
        _default_editor->setText(text);
        _default_editor->setCursorPosition(0);
    }

    _multiline_single_editor->setText(text);
    _multiline_single_editor->setSelection(0, 0);

    _multiline_editor->setPlainText(text);

    QString display_text = calcDisplayText();
    if (display_text != _display_text) {
        _display_text = display_text;
        emit sg_displayTextChanged(_display_text);

        if (by_user)
            emit sg_displayTextEdited(_display_text);
    }

    QAccessibleValueChangeEvent event(this, text);
    QAccessible::updateAccessibility(&event);

    _block_update_text_helper = false;
}

QString ItemSelector::calcDisplayText() const
{
    QString display_text;
    if (_selection_mode == SelectionMode::Single) {
        if (_model_ptr != nullptr && _model_ptr->data()->isInvalidated(_dataset)) {
            // в процессе загрузки нет текста

        } else if (_exact_value != nullptr && (_exact_value_found == ExactValueStatus::NotFound || _exact_value_found == ExactValueStatus::Error)) {
            if (isAsyncLoading()) {
                // в процессе загрузки нет текста
            } else if (isReadOnly() || popupModeInternal() == PopupMode::PopupDialog) {
                // ничего не найдено
            } else {
                // ищем значение без учета фильтрации через прокси
                auto source_model = Utils::getTopSourceModel(_model);
                bool found = false;
                if (source_model != _model) {
                    QModelIndex index = search(Utils::variantToString(*_exact_value).trimmed(), idColumn(), idRole(), true, source_model);

                    if (index.isValid()) {
                        // нашли отфильтрованное значение
                        found = true;
                        display_text = Utils::variantToString(source_model->index(index.row(), displayColumn(), index.parent()).data(_display_role));
                    }
                }

                if (!found)
                    display_text = Utils::variantToString(*_exact_value);
            }

        } else {
            if (_editable && !view()->currentIndex().isValid())
                display_text = _editable_text;
            else
                display_text = Utils::variantToString(_model->data(view()->currentIndex(), _display_role));
        }

    } else if (_selection_mode == SelectionMode::Multi) {
        Z_CHECK(_current_values.count() == _current_values_text.count());

        if (_model_ptr != nullptr && _model_ptr->data()->isInvalidated(_dataset)) {
            // в процессе загрузки нет текста

        } else {
            QStringList text_items;
            for (int i = 0; i < _current_values.count(); i++) {
                if (_current_values_text.at(i) == nullptr) {
                    text_items.clear();
                    break;

                } else {
                    text_items << *_current_values_text.at(i);
                }
            }

            if (text_items.isEmpty() && isAsyncLoading()) {
                // в процессе загрузки нет текста

            } else {
                std::sort(text_items.begin(), text_items.end());
                display_text = text_items.join(", ");
            }
        }

    } else {
        Z_HALT_INT;
    }

    return display_text;
}

void ItemSelector::updateEnabledHelper()
{
    updateEditorVisibility();

    QColor background = isFullEnabled() ? Colors::background() : Colors::readOnlyBackground();

    _goto_button->setHidden(!gotoEntity().isValid());
    if (!_goto_button->isHidden()) {
        _goto_button->setEnabled(currentIndex().isValid());
        _goto_button->setStyleSheet(QStringLiteral("background-color: %1").arg(background.name()));
    }

    _erase_button->setHidden(isReadOnly() || !_allow_empty_selection);
    if (!_erase_button->isHidden()) {
        if (selectionMode() == SelectionMode::Single)
            _erase_button->setEnabled(!_default_editor->text().trimmed().isEmpty() || currentIndex().isValid() || _last_selected.isValid());
        else
            _erase_button->setEnabled(!_default_editor->text().trimmed().isEmpty());
        _erase_button->setStyleSheet(QStringLiteral("background-color: %1").arg(background.name()));
    }

    // через палитру не работает. почему - загадка
    _default_editor->setStyleSheet(QStringLiteral("QLineEdit {border: none; background-color: %1}").arg(background.name()));
    _multiline_editor->setStyleSheet(QStringLiteral("QPlainTextEdit {border: none; background-color: %1}").arg(background.name()));
}

void ItemSelector::updateExactValue(bool by_user)
{
    Z_CHECK(_selection_mode == SelectionMode::Single);

    QModelIndex idx = currentIndex();
    QVariant value;
    if (idx.isValid()) {
        value = idx.model()
                    ->index(idx.row(), _exact_value_column >= 0 ? _exact_value_column : idColumn(), idx.parent())
                    .data(_exact_value_role >= 0 ? _exact_value_role : idRole());
    }
    _exact_value = Z_MAKE_SHARED(QVariant, value);
    _exact_value_found = ExactValueStatus::Found;

    updateText(by_user);
}

void ItemSelector::lookupToExactValue(bool by_user)
{
    if (_selection_mode == SelectionMode::Single) {
        if (_exact_value == nullptr) {
            updateText(false);
            return;
        }

        if (_model == nullptr || _error_loading.isError()) {
            _exact_value_found = ExactValueStatus::Error;

        } else {
            QModelIndex index = search(Utils::variantToString(*_exact_value).trimmed(), _exact_value_column < 0 ? idColumn() : _exact_value_column,
                _exact_value_role < 0 ? idRole() : _exact_value_role, true);
            _exact_value_found = index.isValid() ? ExactValueStatus::Found : ExactValueStatus::NotFound;
            setCurrentIndex(index, by_user, false, false);
        }

    } else if (_selection_mode == SelectionMode::Multi) {
        _current_values_text.clear();
        for (auto& value : _current_values) {
            if (_model == nullptr || _error_loading.isError()) {
                _current_values_text << std::make_shared<QString>(Utils::variantToString(value));
                continue;
            }

            QModelIndex index = search(Utils::variantToString(value).trimmed(), idColumn(), idRole(), true);
            if (!index.isValid())
                _current_values_text << nullptr;
            else
                _current_values_text << std::make_shared<QString>(
                    Utils::variantToString(_model->data(_model->index(index.row(), displayColumn(), index.parent()), _display_role)));
        }
    } else
        Z_HALT_INT;

    updateText(false);
}

void ItemSelector::sl_updateEnabled()
{
    _update_enabled_timer->start();
}

void ItemSelector::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)
    Core::messageDispatcher()->confirmMessageDelivery(message, this);
}

void ItemSelector::sl_filter_timeout()
{
    dialogShow(_filter_text);
    _filter_text.clear();
}

void ItemSelector::sl_callback(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (objectExtensionDestroyed())
        return;

    if (key == Framework::ITEM_SELECTOR_LOAD_MODEL_KEY) {
        Z_CHECK_NULL(_model_ptr);
        _model_ptr->load({}, {_dataset});
        if (_model_ptr->isLoading()) {
            _async_loading = true;
            updateText(false);
        }
    }
}

void ItemSelector::sl_refiltered(const DataProperty& dataset_property)
{
    Q_UNUSED(dataset_property);
    emit sg_lookupModelChanged();
}

void ItemSelector::init()
{
    _object_extension = new ObjectExtension(this);

    Framework::internalCallbackManager()->registerObject(this, "sl_callback");

    _default_editor = new ItemSelectorEditor(this);
    _default_editor->setHidden(true);
    _default_editor->setObjectName(QStringLiteral("zfed"));
    _default_editor->installEventFilter(this);
    _completer = new QCompleter(_default_editor);
    connect(_completer, qOverload<const QString&>(&QCompleter::highlighted), this, [this](const QString& s) {
        if (_editable) {
            sl_editTextEdited(s);
        }
    });
    connect(_default_editor, &QLineEdit::textEdited, this, &ItemSelector::sl_editTextEdited);
    connect(_default_editor, &QLineEdit::textChanged, this, [this]() { sl_updateEnabled(); });

    _multiline_editor = new ItemSelectorMemo(this);
    _multiline_editor->setHidden(true);
    _multiline_editor->setSizePolicy(_multiline_editor->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    _multiline_editor->setObjectName(QStringLiteral("zfme"));
    _multiline_editor->installEventFilter(this);
    _multiline_editor->viewport()->installEventFilter(this);
    connect(_multiline_editor, &PlainTextEdit::textChanged, this, [&]() {
        if (_selection_mode == SelectionMode::Multi)
            updateGeometry();
    });

    _multiline_single_editor = new SqueezedTextLabel(this);
    _multiline_single_editor->setHidden(true);
    _multiline_single_editor->setMargin(2);
    _multiline_single_editor->setObjectName(QStringLiteral("zfmes"));
    _multiline_single_editor->installEventFilter(this);
    _multiline_single_editor->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _multiline_single_editor->setCursor(QCursor(Qt::IBeamCursor));

    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_KeyCompression);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    _ignore_popup_click_timer = new QTimer(this);
    _ignore_popup_click_timer->setSingleShot(true);
    _ignore_popup_click_timer->setInterval(qApp->doubleClickInterval());

    _erase_button = new QToolButton(this);
    _erase_button->setToolTip(ZF_TR(ZFT_CLEAR));
    _erase_button->setObjectName(QStringLiteral("zfer"));
    _erase_button->installEventFilter(this);
    _erase_button->show();
    _erase_button->setAutoRaise(true);
    _erase_button->setFocusPolicy(Qt::NoFocus);
    _erase_button->setIcon(QIcon(":/share_icons/cancel-bw.svg"));
    connect(_erase_button, &QToolButton::clicked, this, &ItemSelector::sl_erase);

    _goto_button = new QToolButton(this);
    _goto_button->setObjectName(QStringLiteral("zfgt"));
    _goto_button->installEventFilter(this);
    _goto_button->show();
    _goto_button->setAutoRaise(true);
    _goto_button->setFocusPolicy(Qt::NoFocus);
    _goto_button->setIcon(QIcon(":/share_icons/goto-bw.svg"));
    connect(_goto_button, &QToolButton::clicked, this, &ItemSelector::sl_goto, Qt::QueuedConnection);

    _update_enabled_timer = new QTimer(this);
    _update_enabled_timer->setSingleShot(true);
    _update_enabled_timer->setInterval(0);
    connect(_update_enabled_timer, &QTimer::timeout, this, [&]() { updateEnabledHelper(); });
    _update_enabled_timer->start();

    _update_text_timer = new QTimer(this);
    _update_text_timer->setSingleShot(true);
    _update_text_timer->setInterval(0);
    connect(
        _update_text_timer, &QTimer::timeout, this, [&]() { updateTextHelper(_update_text_by_user); }, Qt::QueuedConnection);

    _filter_timer = new QTimer(this);
    _filter_timer->setSingleShot(true);
    _filter_timer->setInterval(300);
    connect(_filter_timer, &QTimer::timeout, this, &ItemSelector::sl_filter_timeout);

#if 0 // не используем, но пусть будет  
    _wait_movie_label = Utils::createWaitingMovie(this);
    _wait_movie_label->setHidden(true);
#endif

    prepareEditor();
}

QSize ItemSelector::editorSizeHint() const
{
    ensurePolished();

    QSize size = UiSizes::defaultLineEditSize(font());

    if (!isReadOnly())
        size.setWidth(size.width() - arrowWidth());

    if (editorMode() == EditorMode::Memo)
        size.setHeight(_multiline_editor->sizeHint().height() - margin() * 2);
    else
        size.setHeight(size.height() - margin() * 2);

    return size;
}

QSize ItemSelector::eraseSizeHint() const
{
    int s = UiSizes::defaultEditorToolButtonSize(font());
    return {s, s};
}

QSize ItemSelector::gotoSizeHint() const
{
    int s = UiSizes::defaultEditorToolButtonSize(font());
    return {s, s};
}

int ItemSelector::arrowWidth() const
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    return style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this).width();
}

QSize ItemSelector::arrowSizeHint() const
{
    int x = editorSizeHint().height();

    return {arrowWidth(), x};
}

QRect ItemSelector::editorRect() const
{
    QRect rect(this->rect());
    rect.adjust(margin(), margin(), -margin(), -margin());

    if (!isReadOnly())
        rect.setRight(rect.right() - arrowSizeHint().width() - eraseSizeHint().width() - editor_spacing());

    if (gotoEntity().isValid())
        rect.setRight(rect.right() - gotoSizeHint().width() - spacing());

    return rect;
}

QRect ItemSelector::eraseRect() const
{
    QRect editor_rect = editorRect();

    QRect rect = editor_rect;
    rect.setSize(eraseSizeHint());

    rect.moveCenter({rect.center().x(), editor_rect.center().y()});
    rect.moveLeft(editor_rect.right() + editor_spacing());

    return rect;
}

QRect ItemSelector::gotoRect() const
{
    QRect editor_rect = editorRect();

    QRect rect = editor_rect;
    rect.setSize(gotoSizeHint());

    rect.moveCenter({rect.center().x(), editor_rect.center().y()});
    rect.moveLeft(editor_rect.right() + editor_spacing());

    if (isFullEnabled())
        rect.moveLeft(eraseRect().right() + spacing());

    return rect;
}

QRect ItemSelector::waitMovieRect() const
{
    QRect rect = eraseRect();

    if (isReadOnly()) {
        rect.moveLeft(editorRect().right() - rect.width() - 1);
    } else {
        rect.moveLeft(rect.left() - rect.width() - 1);
    }
    rect.adjust(1, 1, -1, -1);

    return rect;
}

QRect ItemSelector::arrowRect() const
{
    QRect rect = this->rect();
    rect.setSize(arrowSizeHint());

    rect.moveCenter({rect.center().x(), rect.center().y()});
    rect.moveRight(this->rect().right() - 1);

    return rect;
}

bool ItemSelector::isArrowPos(const QPoint& pos) const
{
    return arrowRect().contains(pos);
}

bool ItemSelector::isErasePos(const QPoint& pos) const
{
    return eraseRect().contains(pos);
}

bool ItemSelector::isGotoPos(const QPoint& pos) const
{
    return gotoRect().contains(pos);
}

void ItemSelector::updateArrow(QStyle::StateFlag state)
{
    if (_arrow_state == state)
        return;
    _arrow_state = state;
    update(arrowRect());
}

bool ItemSelector::updateHoverControl(const QPoint& pos)
{
    // взято из QComboBoxPrivate
    QRect lastHoverRect = _hover_rect;
    QStyle::SubControl lastHoverControl = _hover_control;
    bool doesHover = testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        update(lastHoverRect);
        update(_hover_rect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl ItemSelector::newHoverControl(const QPoint& pos)
{
    // взято из QComboBoxPrivate
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    _hover_control = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, pos);
    _hover_rect = (_hover_control != QStyle::SC_None) ? style()->subControlRect(QStyle::CC_ComboBox, &opt, _hover_control) : QRect();
    return _hover_control;
}

void ItemSelector::popupClicked()
{
    if (popupModeInternal() == PopupMode::PopupDialog) {
        dialogShow("");

    } else {
        if (!isPopup() && _ignore_popup_click_timer->isActive()) {
            _ignore_popup_click_timer->stop();
            return;
        }

        if (isPopup())
            popupHide(true);
        else
            popupShow();
    }
}

QModelIndex ItemSelector::search(const QString& s, int column, int role, bool exact, QAbstractItemModel* model) const
{
    if (model == nullptr)
        model = _model;

    if (model == nullptr || _error_loading.isError())
        return {};

    QModelIndex index;
    QString text = s.trimmed();
    if (!text.isEmpty()) {
        QModelIndexList found = model->match(
            model->index(0, column), role, text, 1, exact ? (Qt::MatchFixedString | Qt::MatchRecursive) : (Qt::MatchStartsWith | Qt::MatchRecursive));
        index = found.isEmpty() ? QModelIndex() : found.first();
    }

    return index;
}

bool ItemSelector::isPopup() const
{
    return _popup;
}

void ItemSelector::popupShow()
{
    Z_CHECK(popupMode() != PopupMode::PopupDialog);

    if (_popup || _popup_visible_chinging || _error_loading.isError() || isReadOnly())
        return;

    _popup = true;
    _popup_visible_chinging = true;

    if (_container->_tree_view != nullptr)
        _container->_tree_view->expandAll();

    QRect screen_geometry = Utils::screenRect(mapToGlobal({0, 0}));
    if (screen_geometry.isNull()) {
        _popup_visible_chinging = false;
        return;
    }

    updateArrow(QStyle::State_Sunken);

    updatePopupGeometry();

    _default_editor->deselect();
    _default_editor->setFocus();

    _before_popup_index = std::make_shared<QModelIndex>(currentIndex());
    container()->show();

    if (container()->_tree_view != nullptr) {
        if (container()->_tree_view->currentIndex().isValid())
            container()->_tree_view->expand(container()->_tree_view->currentIndex().parent());
    }

    _update_enabled_timer->start();

    _popup_visible_chinging = false;

    emit sg_popupOpened();
}

void ItemSelector::popupHide(bool apply)
{
    Z_CHECK(popupMode() != PopupMode::PopupDialog);

    if (!_popup || _popup_visible_chinging || container() == nullptr)
        return;

    _popup = false;
    _popup_visible_chinging = true;

    container()->hide();

    updateArrow(QStyle::State_None);

    if (apply)
        setCurrentIndex(view()->currentIndex(), true, false, false);

    _before_popup_index.reset();

    _update_enabled_timer->start();
    setFocus();
    _popup_visible_chinging = false;

    if (apply)
        updateExactValue(true);
    else
        updateText(false);

    Utils::selectLineEditAll(_default_editor);

    emit sg_popupClosed(apply);
}

void ItemSelector::dialogShow(const QString& filter_text)
{
    Z_CHECK(popupModeInternal() == PopupMode::PopupDialog);

    if (isReadOnly())
        return;

    QScopedPointer<SelectFromModelDialog> dlg;
    if (_model_ptr != nullptr) {
        //TODO пока фильтруем по всем видимым колонкам
        dlg.reset(new SelectFromModelDialog(_selection_mode, _model_ptr, _dataset, DataProperty(), Qt::DisplayRole, _dataset.columns().at(displayColumn()),
            multiSelectionLevels(), _data_filter));
    } else {
        Z_CHECK_NULL(_model);
        dlg.reset(new SelectFromModelDialog(_selection_mode, !objectName().isEmpty() ? objectName() : "item_selector", qApp->applicationName(),
            qApp->windowIcon(), _model, idColumn(), idRole(), displayColumn(), displayRole(), multiSelectionLevels(), QList<int> {displayColumn()}));
    }

    dlg->setOnlyBottomItem(_only_bottom_item);
    dlg->setFilterText(filter_text);
    dlg->setAllowEmptySelection(isEraseEnabled());

    if (_selection_mode == SelectionMode::Single) {
        if (!dlg->select({currentValue()}))
            return;

        auto selected = dlg->selectedValues();
        Z_CHECK(selected.count() < 2);
        setCurrentValueHelper(selected.isEmpty() ? QVariant() : selected.first(), -1, -1, true);
        emit sg_edited(view()->currentIndex(), false);

    } else {
        dlg->setAllowEmptySelection(_allow_empty_selection);
        if (!dlg->select(currentValues()))
            return;

        setCurrentValues(dlg->selectedValues());
        emit sg_editedMulti(_current_values);
    }
}

QAbstractItemView* ItemSelector::popupView() const
{
    if (_container->_list_view != nullptr)
        return _container->_list_view;
    else if (_container->_tree_view != nullptr)
        return _container->_tree_view;
    Z_HALT_INT;
    return nullptr;
}

int ItemSelector::pageStep() const
{
    int row_count = view()->model()->rowCount();
    QModelIndex top_index = view()->indexAt(QPoint(0, 0));
    QModelIndex bottom_index = view()->indexAt(QPoint(0, view()->height()));
    return qMax(0, (bottom_index.isValid() ? bottom_index.row() : row_count - 1) - top_index.row() - 1);
}

void ItemSelector::scrollPageUp()
{
    if (isReadOnly())
        return;

    int step = pageStep();
    if (step <= 0)
        return;

    QModelIndex cur_index = view()->currentIndex();
    int next_row = qBound(0, cur_index.row() - step, view()->model()->rowCount(cur_index.parent()) - 1);
    QModelIndex new_index = view()->model()->index(next_row, _display_column, cur_index.parent());
    view()->setCurrentIndex(new_index);
    updateText(false);

    if (!isPopup()) {
        updateExactValue(true);
        emit sg_edited(new_index, true);
        emit sg_changed(new_index);
    }
}

void ItemSelector::scrollPageDown()
{
    if (isReadOnly())
        return;

    int step = pageStep();
    if (step <= 0)
        return;

    QModelIndex cur_index = view()->currentIndex();
    int next_row = qBound(0, cur_index.row() + step, view()->model()->rowCount(cur_index.parent()) - 1);
    QModelIndex new_index = view()->model()->index(next_row, _display_column, cur_index.parent());
    view()->setCurrentIndex(new_index);
    updateText(false);

    if (!isPopup()) {
        updateExactValue(true);
        emit sg_edited(new_index, true);
        emit sg_changed(new_index);
    }
}

void ItemSelector::scrollUp()
{
    if (isReadOnly())
        return;

    int row_count = view()->model()->rowCount();
    if (row_count > 0) {
        QModelIndex new_index = view()->model()->index(0, _display_column);
        view()->setCurrentIndex(new_index);
        updateText(false);

        if (!isPopup()) {
            updateExactValue(true);
            emit sg_edited(new_index, true);
            emit sg_changed(new_index);
        }
    }
}

void ItemSelector::scrollDown()
{
    if (isReadOnly())
        return;

    int row_count = view()->model()->rowCount();
    if (row_count > 0) {
        QModelIndex new_index = view()->model()->index(row_count - 1, _display_column);
        view()->setCurrentIndex(new_index);
        updateText(false);

        if (!isPopup()) {
            updateExactValue(true);
            emit sg_edited(new_index, true);
            emit sg_changed(new_index);
        }
    }
}

void ItemSelector::itemUp()
{
    if (isReadOnly())
        return;

    QModelIndex cur_index = view()->currentIndex();
    const int row_count = view()->model()->rowCount();

    bool changed = false;
    QModelIndex new_index;
    if (cur_index.isValid()) {
        QModelIndex next_index = Utils::getNextItemModelIndex(cur_index, false);
        if (next_index.isValid()) {
            new_index = view()->model()->index(next_index.row(), _display_column, next_index.parent());
            view()->setCurrentIndex(new_index);
            updateText(false);
            changed = true;
        }
    } else if (row_count > 0) {
        new_index = view()->model()->index(0, _display_column);
        view()->setCurrentIndex(new_index);
        updateText(false);
        changed = true;
    }

    if (changed && !isPopup()) {
        updateExactValue(true);
        emit sg_edited(new_index, true);
        emit sg_changed(new_index);
    }
}

void ItemSelector::itemDown()
{
    if (isReadOnly())
        return;

    QModelIndex cur_index = view()->currentIndex();
    const int row_count = view()->model()->rowCount();

    bool changed = false;
    QModelIndex new_index;
    if (cur_index.isValid()) {
        QModelIndex next_index = Utils::getNextItemModelIndex(cur_index, true);
        if (next_index.isValid()) {
            new_index = view()->model()->index(next_index.row(), _display_column, next_index.parent());
            view()->setCurrentIndex(new_index);
            updateText(false);
            changed = true;
        }
    } else if (row_count > 0) {
        new_index = view()->model()->index(0, _display_column);
        view()->setCurrentIndex(new_index);
        updateText(false);
        changed = true;
    }

    if (changed && !isPopup()) {
        updateExactValue(true);
        emit sg_edited(new_index, true);
        emit sg_changed(new_index);
    }
}

void ItemSelector::updateTreeViewColumns()
{
    QTreeView* tree = qobject_cast<QTreeView*>(view());
    if (tree != nullptr) {
        // если вызвать сразу, то treeview клинит и он не показывает вообще никакие колонки
        QTimer::singleShot(0, tree, [tree]() {
            for (int i = 0; i < tree->header()->count(); i++) {
                if (i == qMax(0, tree->treePosition())) {
                    tree->showColumn(i);
                    tree->header()->setSectionResizeMode(QHeaderView::Stretch);
                } else {
                    tree->hideColumn(i);
                }
            }
        });
    };
}

void ItemSelector::setView(QAbstractItemModel* model)
{
    view()->setModel(model);
    updateTreeViewColumns();
}

ItemSelectorDelegate::ItemSelectorDelegate(QAbstractItemView* view, ItemSelector* selector)
    : QStyledItemDelegate(view)
    , _view(view)
    , _selector(selector)
{
}

void ItemSelectorDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    opt.state = opt.state & ~QStyle::State_MouseOver;

    if (_view->currentIndex() == index) {
        opt.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Active | QStyle::State_Window;

        opt.backgroundBrush = opt.palette.brush(QPalette::Active, QPalette::Highlight);
        opt.palette.setBrush(QPalette::Text, opt.palette.brush(QPalette::Active, QPalette::HighlightedText));
        opt.palette.setBrush(QPalette::Window, opt.palette.brush(QPalette::Active, QPalette::Highlight));
    }

    if (_selector->displayRole() != Qt::DisplayRole && _selector->displayRole() != Qt::EditRole) {
        opt.text = displayText(Utils::variantToString(index.data(_selector->displayRole())), QLocale::system());
    }

    painter->save();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    painter->restore();

    painter->save();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    painter->restore();
}

} // namespace zf
