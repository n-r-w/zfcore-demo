#include "zf_item_delegate.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTableView>
#include <QTextDocument>
#include <QTimer>
#include <QToolTip>
#include <QTreeView>
#include <float.h>
#include <QTextOption>
#include <QTextLayout>

#include "zf_core.h"
#include "zf_picture_selector.h"
#include "zf_request_selector.h"
#include "zf_view.h"
#include "zf_html_tools.h"
#include "zf_ui_size.h"
#include "zf_highlight_mapping.h"

namespace zf
{
HintItemDelegate::HintItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QString HintItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

bool HintItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* v, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    if (!opt.text.isEmpty()) {
        QRect rect = v->visualRect(index);
        rect.setHeight(qMax(rect.height(), Z_PM(PM_HeaderDefaultSectionSizeVertical)));
        QSize real_size = cellRealSize(opt, index);
        bool is_html = HtmlTools::correctIfHtml(opt.text);

        opt.features = (opt.features | QStyleOptionViewItem::HasDisplay);
        if (!is_html && real_size.height() > rect.height()) {
            // Активен перенос строк, но высота строки этого не позволяет, поэтому убираем перенос
            opt.features = (opt.features & (~QStyleOptionViewItem::WrapText));
            real_size = sizeHint(opt, index);
        }

        if (rect.width() < real_size.width() || rect.height() < real_size.height() - Utils::scaleUI(2)) {
            QString text;
            if (is_html)
                text = opt.text;
            else
                text = QStringLiteral("<div>%1</div>").arg(opt.text.toHtmlEscaped());
            QToolTip::showText(event->globalPos(), text, v);
            return true;
        }
    }
    if (!QStyledItemDelegate::helpEvent(event, v, opt, index))
        QToolTip::hideText();
    return true;
}

void HintItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->textElideMode = Qt::ElideRight;
}

QSize HintItemDelegate::cellRealSize(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return sizeHint(option, index);
}

ItemDelegate::ItemDelegate(QAbstractItemView* item_view, QAbstractItemView* main_item_view, I_ItemDelegateCellProperty* cell_property_interface,
    I_ItemDelegateCheckInfo* check_info_interface, const DataStructurePtr& structure, const DataProperty& dataset_property, QObject* parent)
    : HintItemDelegate(parent)
    , _item_view(item_view)
    , _main_item_view(main_item_view)
    , _dataset_property(dataset_property)
    , _structure(structure)
    , _cell_property_interface(cell_property_interface)
    , _check_info_interface(check_info_interface)
{
    Z_CHECK_NULL(_item_view);
    Z_CHECK((_structure != nullptr) == _dataset_property.isValid());
    if (_structure != nullptr)
        Z_CHECK(_structure->isValid());
}

ItemDelegate::ItemDelegate(
    const View* view, const DataProperty& dataset_property, QAbstractItemView* main_item_view, I_ItemDelegateCheckInfo* check_info_interface, QObject* parent)
    : HintItemDelegate(parent)
    , _view(const_cast<View*>(view))
    , _main_item_view(main_item_view)
    , _dataset_property(dataset_property)
    , _check_info_interface(check_info_interface)
{
    Z_CHECK_NULL(_view);
    _structure = _view->structure();
    Z_CHECK(dataset_property.isValid() && dataset_property.propertyType() == PropertyType::Dataset);
    _widgets = _view->widgets();
    _highlight = _view->highlightProcessor();    
}

ItemDelegate::ItemDelegate(const DataWidgets* widgets, const DataProperty& dataset_property, const HighlightProcessor* highlight,
    QAbstractItemView* main_item_view, I_ItemDelegateCheckInfo* check_info_interface, QObject* parent)
    : HintItemDelegate(parent)
    , _widgets(widgets)
    , _highlight(highlight)
    , _main_item_view(main_item_view)
    , _dataset_property(dataset_property)
    , _check_info_interface(check_info_interface)
{
    Z_CHECK_NULL(_widgets);
    _structure = _widgets->structure();
    Z_CHECK(dataset_property.isValid() && dataset_property.propertyType() == PropertyType::Dataset);
}

void ItemDelegate::setUseHtml(bool b)
{
    if (b == _use_html)
        return;
    _use_html = b;
}

bool ItemDelegate::isUseHtml() const
{
    return _use_html;
}

void ItemDelegate::setFormatBottomItems(bool b)
{
    _format_bottom_items = b;
}

bool ItemDelegate::isFormatBottomItems() const
{
    return _format_bottom_items;
}

QAbstractItemView* ItemDelegate::itemView() const
{
    lazyInit();
    return _item_view;
}

QAbstractItemView* ItemDelegate::mainItemView() const
{
    lazyInit();
    return _main_item_view;
}

DataProperty ItemDelegate::datasetProperty() const
{
    return _dataset_property;
}

QWidget* ItemDelegate::currentEditor() const
{
    return _current_editor;
}

View* ItemDelegate::view() const
{
    return _view;
}

const DataWidgets* ItemDelegate::widgets() const
{
    return _widgets;
}

QRect ItemDelegate::checkBoxRect(const QModelIndex& index, bool expand) const
{
    QRect rect;

    if (_check_info_interface != nullptr) {
        bool show_check = false;
        bool checked = false;
        _check_info_interface->delegateGetCheckInfo(_item_view, index, show_check, checked);
        if (show_check) {
            const int shift = 2;

            QStyle* style = _item_view->style() ? _item_view->style() : QApplication::style();
            QRect item_rect = _item_view->visualRect(index);

            int check_width = style->pixelMetric(QStyle::PM_IndicatorWidth);
            int check_height = style->pixelMetric(QStyle::PM_IndicatorHeight);

            if (expand)
                rect = {item_rect.left(), item_rect.top(), check_width + shift * 3, item_rect.height()};
            else
                rect = {item_rect.left() + shift, item_rect.top() + shift, check_width, check_height};

            rect.moveCenter({rect.center().x(), item_rect.center().y()});
        }
    }

    return rect;
}

QString ItemDelegate::getDisplayText(const QModelIndex& index, QStyleOptionViewItem* option) const
{
    Q_UNUSED(option)

    if (Utils::isAppHalted() || !index.isValid() || _item_view == nullptr)
        return QString();

    QString text;

    QModelIndex source_index = sourceIndex(index);

    if (source_index.isValid()) {
        // Преобразовываем текст
        DataProperty column_property = cellDataProperty(index);
        QVariant value = source_index.data(Qt::DisplayRole);

        QList<ModelPtr> data_not_ready;
        if (_view != nullptr)
            _view->getDatasetCellVisibleValue(
                source_index.row(), column_property, source_index.parent(), value, VisibleValueOption::Application, text, data_not_ready);
        else
            View::getDatasetCellVisibleValue(_item_view, column_property, source_index, value, VisibleValueOption::Application, text, data_not_ready);

        if (column_property.dataType() == DataType::Bool) {
            QVariant converted;
            Utils::convertValue(
                value, column_property.dataType(), Core::locale(LocaleType::Universal), ValueFormat::Edit, Core::locale(LocaleType::UserInterface), converted);
            option->checkState = converted.toBool() ? Qt::Checked : Qt::Unchecked;
            option->features |= QStyleOptionViewItem::HasCheckIndicator;
        }

        // ждем загрузки data_not_ready чтобы обновить отображение
        // подписываемся на окончание загрузки lookup
        for (auto& dnr : qAsConst(data_not_ready)) {
            Z_CHECK(dnr->isLoading());
            bool is_new = true;
            for (auto& info : qAsConst(_data_not_ready_info)) {
                if (info.model == dnr) {
                    is_new = false;
                    break;
                }
            }
            if (is_new) {
                auto connection = connect(dnr.get(), &Model::sg_finishLoad, this, &ItemDelegate::sl_lookupModelLoaded);
                _data_not_ready_info << DataNotReadyInfo {dnr, connection};
            }
        }
    }

    // не надо отображать текст true/false если задан CheckStateRole
    if (index.model()->itemData(index).contains(Qt::CheckStateRole) && (text == QStringLiteral("true") || text == QStringLiteral("false")))
        text.clear();

    if (!HtmlTools::correctIfHtml(text)) {
        // заменяем переносы строк на пробелы
        text = Utils::prepareMemoString(text);
        text.replace('\n', ' ');
        const int MAX_SIZE = 250;
        if (text.length() >= MAX_SIZE) {
            text.resize(MAX_SIZE);
            text.replace(MAX_SIZE - 3, 3, QStringLiteral("..."));
        }
    }

    return text;
}

void ItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    lazyInit();

    Q_UNUSED(option)

    if (_item_view == nullptr) {
        HintItemDelegate::updateEditorGeometry(editor, option, index);
        return;
    }

    // позиционируем Memo поля
    QPlainTextEdit* pe = qobject_cast<QPlainTextEdit*>(editor);
    if (pe != nullptr) {
        HintItemDelegate::updateEditorGeometry(editor, option, index);

        QRect r = pe->geometry();
        r.setHeight(pe->geometry().height() * 4); // высота 4 строки

        // Преобразуем координаты, т.к. QPlainTextEdit имеет окно в качестве родителя
        QWidget* base = Utils::getTopWindow();
        QPoint p = _item_view->viewport()->mapTo(base, r.topLeft());
        r.moveTopLeft(p);
        // Ограничение по размеру
        r.setWidth(qMin(r.width(), base->width()));
        r.setHeight(qMin(r.height(), base->height()));
        // Ограничение по положению
        if (r.right() > base->width())
            r.moveRight(base->width());
        if (r.left() < 0)
            r.moveLeft(0);
        if (r.bottom() > base->height())
            r.moveBottom(base->height());
        if (r.top() < 0)
            r.moveTop(0);

        pe->setGeometry(r);

    } else {
        QRect rect = _item_view->visualRect(index);
        rect.setRight(qMin(rect.right(), _item_view->viewport()->geometry().right() - 1));
        editor->setGeometry(rect);

        if (_main_item_view != nullptr && editor->parent() != _main_item_view) {
            // избавляемся от обрезания ширины виджета при редактировании фиксированных колонок
            editor->setParent(_main_item_view);
            QPoint pos = editor->pos();
            pos.setX(pos.x() + _main_item_view->viewport()->pos().x());
            pos.setY(pos.y() + _main_item_view->viewport()->pos().y());
            editor->move(pos);
        }
    }
}

QWidget* ItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    lazyInit();

    if (!_current_editor.isNull())
        Core::logCriticalError("ItemDelegate::createEditor - current editor is not null");

    DataProperty property;
    _current_editor = createEditorInternal(parent, option, index, property);

    if (_current_editor != nullptr) {
        bool custom_prepared = false;
        if (_view != nullptr) {
            QModelIndex source_index = sourceIndex(index);
            custom_prepared = _view->prepareDatasetEditor(_current_editor, source_index.row(), property, source_index.column(), source_index.parent());
        }

        if (auto w = qobject_cast<QCheckBox*>(_current_editor)) {
            connect(w, &QCheckBox::stateChanged, this, &ItemDelegate::sl_checkboxChanged, Qt::QueuedConnection);

        } else if (auto w = qobject_cast<ComboBox*>(_current_editor)) {
            connect(w, &ComboBox::sg_popupClosed, this, qOverload<>(&ItemDelegate::sl_popupClosed), Qt::QueuedConnection);

        } else if (auto w = qobject_cast<FormattedEdit*>(_current_editor)) {
            connect(w, &DateEdit::sg_popupClosed, this, qOverload<>(&ItemDelegate::sl_popupClosed), Qt::QueuedConnection);
            w->setPopupOnEnter(false);
            w->setMaximumButtonsSize(UiSizes::defaultTableRowHeight() - 2);

        } else if (auto w = qobject_cast<ItemSelector*>(_current_editor)) {
            w->setSelectAllOnFocus(false);
            w->setPopupEnter(false);
            w->setPopupMode(ItemSelector::PopupMode::PopupComboBox);
            connect(w, &ItemSelector::sg_popupClosed, this, qOverload<>(&ItemDelegate::sl_popupClosed), Qt::QueuedConnection);

            if (w->currentText().isEmpty() && !w->isPopup()) {
                // вызывать напрямую нельзя, т.к. виджет еще не показан на экране
                QMetaObject::invokeMethod(w, "popupShow", Qt::QueuedConnection);
            }

        } else if (auto w = qobject_cast<RequestSelector*>(_current_editor)) {
            connect(w, &RequestSelector::sg_popupClosed, this, qOverload<bool>(&ItemDelegate::sl_popupClosed), Qt::QueuedConnection);
            w->setMaximumControlsSize(UiSizes::defaultTableRowHeight() - 2);
            w->editor()->installEventFilter(const_cast<ItemDelegate*>(this));
            if (!custom_prepared && property.isValid() && !property.lookup()->parentKeyProperties().isEmpty()) {
                // установка родительского идентификатора
                Z_CHECK_NULL(_widgets);

                QVariantList values;
                for (auto& p : property.lookup()->parentKeyProperties()) {
                    auto parent_key_property = _widgets->property(p);
                    values << index.model()->index(index.row(), parent_key_property.pos(), index.parent());
                }
                w->setParentKeys(values);
            }
        }
    }

    if (_current_editor != nullptr && _current_editor->parent() == nullptr)
        _current_editor->setParent(parent);

    return _current_editor;
}

void ItemDelegate::destroyEditor(QWidget* editor, const QModelIndex& index) const
{
    lazyInit();
    HintItemDelegate::destroyEditor(editor, index);
}

void ItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    lazyInit();

    setEditorDataInternal(editor, index);

    if (QComboBox* w = qobject_cast<QComboBox*>(editor))
        w->showPopup();
}

void ItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    lazyInit();

    setModelDataInternal(editor, model, index);
}

bool ItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* v, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (!index.isValid() || _view == nullptr)
        return HintItemDelegate::helpEvent(event, v, option, index);

    QModelIndex source_index = sourceIndex(index);
    QList<HighlightItem> errors;
    if (source_index.isValid())
        errors = _view->model()->cellHighlight(source_index, false, false);
    if (errors.isEmpty())
        return HintItemDelegate::helpEvent(event, v, option, index);

    // ошибка вместо текста
    QStringList text;
    for (auto& i : errors) {
        text << i.text();
    }

    QToolTip::showText(event->globalPos(), QStringLiteral("<div>%1</div>").arg(text.join("<br>")), v);
    return true;
}

void ItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (Utils::isAppHalted() || !index.isValid()) {
        HintItemDelegate::paint(painter, option, index);
        return;
    }

    lazyInit();

    if (_item_view->indexWidget(index) != nullptr && index == _item_view->currentIndex()) {
        // чтобы не было видно выделения по краям редактора
        painter->fillRect(option.rect, _item_view->palette().base());
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    // отрисовка ошибки
    QRect highlight_rect;
    QRect opt_rect;
    QColor highlight_color;
    getHighlightInfo(&opt, index, highlight_rect, opt_rect, highlight_color);
    if (highlight_rect.isValid())
        opt.rect = opt_rect;

    // отрисовка дополнительного чекбокса
    int check_shift = 0;
    if (_check_info_interface != nullptr) {
        bool show_check = false;
        bool checked = false;
        _check_info_interface->delegateGetCheckInfo(_item_view, index, show_check, checked);
        if (show_check) {
            QRect check_rect = checkBoxRect(index, false);
            QRect check_rect_expanded = checkBoxRect(index, true);

            painter->save();
            QStyleOptionButton check_option;

            check_option.rect = check_rect;
            check_option.state = QStyle::State_Enabled;
            check_option.state |= checked ? QStyle::State_On : QStyle::State_Off;

            QRect item_rect = _item_view->visualRect(index);
            bool mouse_over = false;
            if (item_rect.isValid()) {
                QPoint cursor_screen_pos = QCursor::pos();
                QPoint checkbox_screen_top_left = _item_view->viewport()->mapToGlobal(check_rect.topLeft());
                QRect checkbox_screen_rect = {checkbox_screen_top_left.x(), checkbox_screen_top_left.y(), check_rect.width(), check_rect.height()};
                mouse_over = checkbox_screen_rect.contains(cursor_screen_pos);
            }

            check_option.state.setFlag(QStyle::State_MouseOver, mouse_over);

            painter->save();
            painter->fillRect(opt.rect, opt.backgroundBrush);
            painter->restore();

            style->drawControl(QStyle::ControlElement::CE_CheckBox, &check_option, painter, opt.widget);
            painter->restore();

            check_shift += check_rect_expanded.width();
        }
    }

    painter->save();
    opt.rect.setLeft(opt.rect.left() + check_shift);
    highlight_rect.setLeft(highlight_rect.left() + check_shift);

    paintCellContent(style, painter, &opt, index, opt.widget);

    painter->restore();

    if (highlight_rect.isValid()) {
        QStyleOptionViewItem opt_rect = opt;
        opt_rect.rect = highlight_rect;
        paintErrorBox(painter, opt_rect, index, highlight_color, 2);
    }
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    lazyInit();

    if (!index.isValid() || qobject_cast<QTreeView*>(_item_view) != nullptr)
        return HintItemDelegate::sizeHint(option, index);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QSize size;
    if (!opt.icon.isNull() && opt.text.isEmpty()) {
        size = iconSize(opt.icon);

    } else {
        // криворукие программисты Qt не учитывают span при расчете sizeHint
        if (auto table_view = qobject_cast<QTableView*>(_item_view)) {
            int span = table_view->columnSpan(index.row(), index.column());
            if (span > 1) {
                int span_col = index.column() + span - 1;
                if (span_col > table_view->horizontalHeader()->count() - 1)
                    span_col = table_view->horizontalHeader()->count() - 1;

                int real_width = 0;
                for (int col = index.column(); col < span_col; col++) {
                    if (!table_view->horizontalHeader()->isSectionHidden(col))
                        real_width += table_view->horizontalHeader()->sectionSize(col);
                }
                if (real_width > 0)
                    opt.rect.setWidth(real_width);
            }

            span = table_view->rowSpan(index.row(), index.column());
            if (span > 1)
                opt.rect.setWidth(9999);
        }

        if (_use_html && HtmlTools::correctIfHtml(opt.text)) {
            size = cellRealSizeHelper(opt, index, true);
            // если соотношение сторон слишком вытянуто в высоту, то надо поправить
            double ratio = 2;
            if (size.width() > 0 && (double)size.height() / (double)size.width() > ratio)
                size.setHeight(size.width() * ratio);

        } else {
            size = HintItemDelegate::sizeHint(opt, index);
        }
    }

    size.setHeight(size.height() + Utils::scaleUI(2));

    // не меньше минимального размера
    size.setHeight(qMax(size.height(), UiSizes::defaultTableRowHeight()));

    return size;
}

bool ItemDelegate::eventFilter(QObject* object, QEvent* event)
{
    lazyInit();

    if (object == _item_view) {
        if (event->type() == QEvent::Resize) {
            if (!_close_widget.isNull())
                _close_editor_timer->start();
            return false;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (_current_editor == nullptr && keyEvent->matches(QKeySequence::Cancel))
            return false; // борьба с багом при котором базовый делегат Qt жрет нажатия клавишь

        if (_current_editor == nullptr && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            // Борьба с багом, при котором Qt вызывает сигнал закрытия редактора при открытии его через enter
            _fix_enter_key = true;
        }

        if (_current_editor != nullptr && (_current_editor == object || Utils::hasParent(object, _current_editor))) {
            // Для QPlainTextEdit необходима обработка закрытия
            if (auto w = qobject_cast<QPlainTextEdit*>(_current_editor)) {
                if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) && keyEvent->modifiers() == Qt::ControlModifier) {
                    // Ctrl + Enter закрывает QPlainTextEdit
                    _close_widget = w;
                    _close_editor_timer->start();
                    return false;
                }
            }

            // Закрываем RequestSelector по ESC
            if (auto w = qobject_cast<RequestSelector*>(_current_editor); w != nullptr && keyEvent->matches(QKeySequence::Cancel)) {
                _close_widget = w;
                _close_editor_timer->start();
                return false;
            }
        }
    }

    return HintItemDelegate::eventFilter(object, event);
}

QWidget* ItemDelegate::createEditorInternal(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index, DataProperty& property) const
{
    QWidget* result = nullptr;

    if (_structure != nullptr) {
        Z_CHECK(_dataset_property.isValid());
        if (_dataset_property.options().testFlag(PropertyOption::ReadOnly))
            return nullptr;

        if (_dataset_property.columns().count() > index.column()) {
            property = _dataset_property.columns().at(index.column());
            if (property.options().testFlag(PropertyOption::ReadOnly))
                return nullptr;
        }
    }

    if (_view != nullptr) {
        QModelIndex source_index = sourceIndex(index);
        if (_dataset_property.columns().count() > source_index.column())
            result = _view->createDatasetEditor(parent, source_index.row(), property, property.pos(), source_index.parent());
        else
            result = _view->createDatasetEditor(parent, source_index.row(), DataProperty(), source_index.column(), source_index.parent());
    }

    if (result == nullptr && _widgets != nullptr) {
        result = _widgets->generateWidget(property, false, false, false)->field;

        QPlainTextEdit* pe = qobject_cast<QPlainTextEdit*>(result);

        if (pe != nullptr) {
            pe->setParent(Utils::getTopWindow()); // для мемо полей нельзя обрезать размерами таблицы
            pe->setWordWrapMode(QTextOption::WrapAnywhere);
            pe->setFrameShape(QFrame::Box);

        } else {
            result->setParent(parent);
        }
    }

    if (result == nullptr) {
        result = HintItemDelegate::createEditor(parent, option, index);
        if (result->parent() == nullptr)
            result->setParent(parent);
    }

    return result;
}

void ItemDelegate::setEditorDataInternal(QWidget* editor, const QModelIndex& index) const
{
    QModelIndex source_index = sourceIndex(index);
    DataProperty column_property = cellDataProperty(source_index);
    if (_view != nullptr) {
        if (_view->setDatasetEditorData(editor, source_index.row(), column_property, source_index.column(), source_index.parent()))
            return;
    }

    if (!column_property.isValid()) {
        HintItemDelegate::setEditorData(editor, index);
        return;
    }

    Z_CHECK(source_index.isValid());

    QMap<PropertyID, QVariant> values;
    values[column_property.id()] = source_index.data(Qt::DisplayRole);

    auto links = column_property.links(PropertyLinkType::LookupFreeText);
    if (!links.isEmpty()) {
        Z_CHECK_NULL(_structure);
        auto free_text_property = _structure->property(links.constFirst()->linkedPropertyId());
        QModelIndex free_text_index = source_index.model()->index(source_index.row(), free_text_property.pos(), source_index.parent());
        values[free_text_property.id()] = free_text_index.data(Qt::DisplayRole);
    }

    DataWidgets::updateWidget(editor, column_property, values, DataWidgets::UpdateReason::NoReason);
}

void ItemDelegate::setModelDataInternal(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (_view == nullptr && _widgets == nullptr) {
        HintItemDelegate::setModelData(editor, model, index);
        return;
    }

    QModelIndex source_index = sourceIndex(index);
    DataProperty column_property = cellDataProperty(source_index);

    if (_view != nullptr && _view->setDatasetEditorModelData(editor, source_index.row(), column_property, source_index.column(), source_index.parent()))
        return;

    if (!column_property.isValid()) {
        HintItemDelegate::setModelData(editor, model, index);
        return;
    }

    Z_CHECK(source_index.isValid());
    Z_CHECK(column_property.isValid());

    auto new_values = DataWidgets::extractWidgetValues(column_property, editor, _view);
    Z_CHECK(new_values.contains(column_property.id()));
    for (auto it = new_values.constBegin(); it != new_values.constEnd(); ++it) {
        QVariant new_value = it.value();
        Z_CHECK_NULL(_structure);
        DataProperty property = _structure->property(it.key());
        Z_CHECK(property.propertyType() == PropertyType::ColumnFull);

        QVariant new_value_converted;
        Utils::convertValue(new_value, property.dataType(), Core::locale(LocaleType::UserInterface), ValueFormat::Internal, Core::locale(LocaleType::Universal),
            new_value_converted);

        QVariant current_value = _widgets->data()->cell(source_index.row(), property, Qt::DisplayRole, source_index.parent());

        if (!Utils::compareVariant(new_value_converted, current_value)) {
            _widgets->data()->setCell(source_index.row(), property, new_value_converted, Qt::DisplayRole, source_index.parent());

            // Установка значения в колонку расшифровки для LookupRequest
            if (property.lookup() != nullptr && property.lookup()->type() == LookupType::Request) {
                auto links = property.links(PropertyLinkType::LookupFreeText);
                Z_CHECK(links.count() == 1);
                if (links.at(0)->linkedPropertyId() != property) { // совпадения колонок быть не должно, но на всякий случай
                    RequestSelector* rs = qobject_cast<RequestSelector*>(editor);
                    Z_CHECK_NULL(rs);
                    Z_CHECK_ERROR(_widgets->data()->setCell(
                        source_index.row(), links.at(0)->linkedPropertyId(), rs->currentText(), Qt::DisplayRole, source_index.parent()));
                }
            }
        }
    }
}

void ItemDelegate::paintErrorBox(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QColor& color, int line_width) const
{
    Q_UNUSED(index)
    painter->save();
    painter->setPen(QPen(color, line_width, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    painter->drawRect(option.rect);
    painter->restore();
}

QSize ItemDelegate::cellRealSizeHelper(const QStyleOptionViewItem& option, const QModelIndex& index, bool is_html) const
{
    lazyInit();

    QSize size;
    if (is_html) {
        QTextDocument doc;
        initTextDocument(&option, index, doc);

        int width = 0;
        if (auto table_view = qobject_cast<QTableView*>(_item_view))
            width = table_view->horizontalHeader()->sectionSize(index.column());
        else if (auto tree_view = qobject_cast<QTreeView*>(_item_view))
            width = tree_view->header()->sectionSize(index.column());
        else
            Z_HALT_INT;

        size = QSize(width, doc.size().height());

    } else {
        size = sizeHint(option, index);
    }

    return size;
}

QModelIndex ItemDelegate::sourceIndex(const QModelIndex& index) const
{
    lazyInit();

    if (_view != nullptr)
        return _view->sourceIndex(index);

    if (_widgets != nullptr && _widgets->filter() != nullptr)
        return _widgets->filter()->sourceIndex(index);

    if (_item_view != nullptr)
        return Utils::alignIndexToModel(index, _item_view->model());

    return index;
}

QSize ItemDelegate::cellRealSize(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    return cellRealSizeHelper(opt, index, _use_html && HtmlTools::correctIfHtml(opt.text));
}

void ItemDelegate::sl_closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    // Борьба с багом, при котором Qt вызывает сигнал закрытия редактора при открытии его через enter
    if (_fix_enter_key) {
        _fix_enter_key = false;
        return;
    }

    lazyInit();

    Q_UNUSED(editor)
    Q_UNUSED(hint)

    _current_editor = nullptr;

    if (_close_widget.isNull())
        return;

    emit commitData(_close_widget.data());

    _close_widget = nullptr;
}

void ItemDelegate::sl_currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    lazyInit();

    if (current.isValid() || previous.isValid()) {
        if (qobject_cast<QTreeView*>(_item_view) != nullptr) {
            _item_view->update();

        } else {
            for (int i = 0; i < _item_view->model()->columnCount(); i++) {
                if (current.isValid())
                    _item_view->update(_item_view->model()->index(current.row(), i, current.parent()));
                if (previous.isValid())
                    _item_view->update(_item_view->model()->index(previous.row(), i, previous.parent()));
            }
        }
    }
}

void ItemDelegate::popupClosedInternal(bool applied)
{
    if (Utils::isAppHalted())
        return;

    auto w = qobject_cast<QWidget*>(sender());
    if (w == nullptr)
        return;

    if (applied)
        emit commitData(w);

    emit closeEditor(w);
    if (_item_view)
        _item_view->setFocus();
}

void ItemDelegate::paintCellContent(QStyle* style, QPainter* p, const QStyleOptionViewItem* opt, const QModelIndex& index, const QWidget* widget) const
{
    Q_UNUSED(index)

    QStyleOptionViewItem option(*opt);

    if (_use_html && HtmlTools::isHtml(option.text)) {
        // код выдран из QCommonStyle::drawControl

        p->setClipRect(option.rect);

        QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, widget);
        QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &option, widget);
        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option, widget);

        // draw the background
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, p, widget);

        // draw the check mark
        if (option.features & QStyleOptionViewItem::HasCheckIndicator) {
            option.rect = checkRect;
            option.state = option.state & ~QStyle::State_HasFocus;

            switch (option.checkState) {
                case Qt::Unchecked:
                    option.state |= QStyle::State_Off;
                    break;
                case Qt::PartiallyChecked:
                    option.state |= QStyle::State_NoChange;
                    break;
                case Qt::Checked:
                    option.state |= QStyle::State_On;
                    break;
            }
            style->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, p, widget);
        }

        // draw the icon
        QIcon::Mode mode = QIcon::Normal;
        if (!(option.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if (option.state & QStyle::State_Selected)
            mode = QIcon::Selected;
        QIcon::State state = option.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
        option.icon.paint(p, iconRect, option.decorationAlignment, mode, state);

        // draw the text
        if (!option.text.isEmpty()) {
            QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
                cg = QPalette::Inactive;

            if (option.state & QStyle::State_Selected) {
                p->setPen(option.palette.color(cg, QPalette::HighlightedText));
            } else {
                p->setPen(option.palette.color(cg, QPalette::Text));
            }
            if (option.state & QStyle::State_Editing) {
                p->setPen(option.palette.color(cg, QPalette::Text));
                p->drawRect(textRect.adjusted(0, 0, -1, -1));
            }

            viewItemDrawText(style, p, &option, textRect);
        }

        // draw the focus rect
        if (option.state & QStyle::State_HasFocus) {
            QStyleOptionFocusRect o;
            o.QStyleOption::operator=(option);
            o.rect = style->subElementRect(QStyle::SE_ItemViewItemFocusRect, &option, widget);
            o.state |= QStyle::State_KeyboardFocusChange;
            o.state |= QStyle::State_Item;
            QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
            o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window);
            style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, p, widget);
        }

    } else {
        style->drawControl(QStyle::CE_ItemViewItem, &option, p, option.widget);
    }
}

void ItemDelegate::viewItemDrawText(QStyle* style, QPainter* p, const QStyleOptionViewItem* option, const QRect& rect) const
{
    const QWidget* widget = option->widget;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;

    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding

    // Рисуем иконку
    if (!option->icon.isNull()) {
        QStyleOptionViewItem optIcon = *option;
        optIcon.text = QString();
        optIcon.state = option->state & ~(QStyle::State_HasFocus | QStyle::State_Active | QStyle::State_Selected);
        style->drawControl(QStyle::CE_ItemViewItem, &optIcon, p, option->widget);
    }

    p->save();
    p->translate(textRect.topLeft());
    QRect clip = textRect.translated(-textRect.topLeft());
    p->setClipRect(clip);

    QTextDocument doc;
    initTextDocument(option, option->index, doc);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = clip;
    if (option->state & QStyle::State_Selected) {
        // Рисуем обводную линию вокруг текста
        QTextCharFormat format;
        // #fafafa
        format.setTextOutline(QPen(QColor(250, 250, 250), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        doc.documentLayout()->draw(p, ctx);

        format.setTextOutline(QPen(Qt::NoPen));
        cursor.mergeCharFormat(format);
    }
    // Рисуем текст без обводной линии
    ctx.palette.setColor(QPalette::Text, option->palette.color(QPalette::Text));
    doc.documentLayout()->draw(p, ctx);
    p->restore();

    if (option->rect.width() < doc.size().width() || option->rect.height() < doc.size().height()) {
        // отрисовка многоточия
        //        qDebug() << option->rect.width() << doc.size().width() << option->rect.height() << doc.size().height();
        p->drawText(option->rect.adjusted(0, 0, -1, 2), Qt::AlignRight | Qt::AlignBottom, QStringLiteral("..."));
    }
}

void ItemDelegate::initTextDocument(const QStyleOptionViewItem* option, const QModelIndex& index, QTextDocument& doc) const
{
    if (_item_view == nullptr)
        return;

    if (auto w = qobject_cast<QTableView*>(_item_view))
        doc.setTextWidth(w->horizontalHeader()->sectionSize(index.column()) - 4);
    else if (auto w = qobject_cast<QTreeView*>(_item_view))
        doc.setTextWidth(w->header()->sectionSize(index.column()) - 4);
    else
        Z_HALT_INT;

    doc.setHtml(option->text);
}

QSizeF ItemDelegate::viewItemTextLayout(QTextLayout& textLayout, int lineWidth, int maxHeight, int* lastVisibleLine)
{
    if (lastVisibleLine)
        *lastVisibleLine = -1;
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    int i = 0;
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
        // we assume that the height of the next line is the same as the current one
        if (maxHeight > 0 && lastVisibleLine && height + line.height() > maxHeight) {
            const QTextLine nextLine = textLayout.createLine();
            *lastVisibleLine = nextLine.isValid() ? i : -1;
            break;
        }
        ++i;
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

void ItemDelegate::sl_popupClosed()
{
    popupClosedInternal(true);
}

void ItemDelegate::sl_popupClosed(bool applied)
{
    popupClosedInternal(applied);
}

void ItemDelegate::sl_checkboxChanged(int)
{
    if (Utils::isAppHalted())
        return;

    auto w = qobject_cast<QWidget*>(sender());
    if (w == nullptr)
        return;

    emit commitData(w);
    emit closeEditor(w);
}

void ItemDelegate::sl_lookupModelLoaded(const Error& error, const LoadOptions& load_options, const DataPropertySet& properties)
{
    Q_UNUSED(error)
    Q_UNUSED(load_options)
    Q_UNUSED(properties)

    Model* m_finished = qobject_cast<Model*>(sender());

    Z_CHECK(!_data_not_ready_info.isEmpty());

    bool found = false;
    for (int i = _data_not_ready_info.count() - 1; i >= 0; i--) {
        auto& wi = _data_not_ready_info.at(i);
        if (wi.model.get() == m_finished) {
            disconnect(wi.connection);
            _data_not_ready_info.removeAt(i);
            found = true;
            break;
        }
    }
    Z_CHECK(found);

    if (_data_not_ready_info.isEmpty()) {
        // все загрузилось
        _item_view->viewport()->update();
    }
}

void ItemDelegate::lazyInit() const
{
    if (_initialized)
        return;
    _initialized = true;

    ItemDelegate* self = const_cast<ItemDelegate*>(this);

    self->_close_editor_timer = new FeedbackTimer(self);

    if (_item_view == nullptr) {
        Z_CHECK_NULL(_widgets);
        Z_CHECK(_dataset_property.isValid() && _dataset_property.propertyType() == PropertyType::Dataset);
        self->_item_view = _widgets->object<QAbstractItemView>(_dataset_property);
    }

    if (_main_item_view != nullptr)
        connect(_main_item_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &ItemDelegate::sl_currentChanged);
    else
        connect(_item_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &ItemDelegate::sl_currentChanged);

    _item_view->installEventFilter(self);

    connect(_close_editor_timer, &FeedbackTimer::timeout, this, [&]() {
        if (_close_widget.isNull())
            return;

        emit const_cast<ItemDelegate*>(this)->commitData(_close_widget.data());
        emit const_cast<ItemDelegate*>(this)->closeEditor(_close_widget.data(), QAbstractItemDelegate::SubmitModelCache);

        _close_widget = nullptr;
        _current_editor = nullptr;
    });

    connect(this, &ItemDelegate::closeEditor, this, &ItemDelegate::sl_closeEditor);
}

DataProperty ItemDelegate::cellDataProperty(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());

    if (_dataset_property.isValid()) {
        if (_dataset_property.columns().count() <= index.column())
            return DataProperty();

        return _dataset_property.columns().at(index.column());
    }

    return _cell_property_interface != nullptr ? _cell_property_interface->delegateGetCellDataProperty(_item_view, index) : DataProperty();
}

QSize ItemDelegate::iconSize(const QIcon& icon)
{
    int size = Z_PM(PM_SmallIconSize);

    QSize a_size = icon.actualSize(QSize(size, size));
    if (a_size.width() > size)
        a_size.setWidth(size);
    if (a_size.height() > size)
        a_size.setHeight(size);
    return a_size;
}

void ItemDelegate::getHighlightInfo(QStyleOptionViewItem* option, const QModelIndex& index, QRect& rect, QRect& opt_rect, QColor& color) const
{
    rect = QRect();

    if (_highlight == nullptr || _item_view == nullptr || Utils::isAppHalted() || !index.isValid() ||
        // активен редактор ячейки
        _item_view->indexWidget(index) != nullptr)
        return;

    QModelIndex source_index;

    if (_view != nullptr) {
        auto mapped_prop = _view->highligtMapping()->getSourceHighlightProperty(_dataset_property);
        if (mapped_prop != _dataset_property) {
            // замапили один датасет на другой
            Z_CHECK(mapped_prop.isDataset());
            // берем ошибку из оригинального датасета
            auto ds = _view->dataset(mapped_prop);
            // тут по хорошему надо искать с учетом иерархии, но пока не надо и возможно вообще не понадобится
            if (index.parent().isValid() || ds->rowCount() <= index.row() || ds->columnCount() <= index.column())
                return;

            source_index = _view->sourceIndex(ds->index(index.row(), index.column()));
        }
    }

    if (!source_index.isValid())
        source_index = sourceIndex(index);

    QList<HighlightItem> errors = _highlight->cellHighlight(source_index, false, false);
    if (errors.isEmpty())
        return;

    QStyleOptionViewItem opt = *option;

    QRect itemRect = _item_view->visualRect(index);
    TableView* table_view = qobject_cast<TableView*>(_item_view);

    if ((table_view && table_view->frozenGroupCount() > 0 && index.column() == 0) || // Первая фиксированная ячейка
        ((!table_view || table_view->frozenGroupCount() == 0) && itemRect.left() == 0)) { // Первая видимая ячейка и нет фиксированных
        rect = opt.rect.adjusted(2, 1, -1, -1);
        opt_rect = opt.rect.adjusted(1, 0, 0, 0); // Иначе с левой стороны ячейки рамка не влезает
    } else {
        rect = opt.rect.adjusted(1, 1, -1, -1);
        opt_rect = opt.rect;
    }

    if (itemRect.top() == 0)
        rect.adjust(0, 1, 0, 0); // Иначе рамка налезет на заголовок

    color = Utils::informationTypeColor(errors.first().type());
}

void ItemDelegate::getCellProperties(const QModelIndex& index, QStyleOptionViewItem* option) const
{
    if (!index.isValid())
        return;

    if (_view != nullptr && _item_view != nullptr) {
        QModelIndex source_index = sourceIndex(index);
        DataProperty column_property = cellDataProperty(source_index);
        if (column_property.isValid()) {
            _view->getDatasetCellPropertiesHelper(
                column_property, source_index, option->state, option->font, option->backgroundBrush, option->palette, option->icon, option->displayAlignment);
        }
        option->decorationAlignment = option->displayAlignment;
    }

    if (_format_bottom_items && _item_view != nullptr) {
        if (qobject_cast<QTreeView*>(_item_view) != nullptr) {
            if (index.model()->rowCount(index.model()->index(index.row(), 0, index.parent())) == 0) {
                option->font = Utils::fontBold(option->font);
            }
        }
    }
}

void ItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const
{
    HintItemDelegate::initStyleOption(option, index);

    if (Utils::isAppHalted() || !index.isValid())
        return;

    lazyInit();

    QAbstractItemView* item_view = _main_item_view != nullptr ? _main_item_view : _item_view;
    QModelIndex current_index;
    DataProperty column_property;
    QModelIndex source_index = sourceIndex(index);
    // текущая ячейка
    bool is_current_cell = false;
    // строка выделена
    bool is_selected_row = false;
    // текущая строка
    bool is_current_row = false;

    if (item_view != nullptr) {
        // определение текущего индекса
        current_index = item_view->currentIndex();
        auto tree = qobject_cast<QTreeView*>(item_view);
        if (tree != nullptr && current_index.column() != 0 && tree->isFirstColumnSpanned(current_index.row(), current_index.parent())) {
            current_index = tree->model()->index(current_index.row(), 0, current_index.parent());
        }

// цвет фона текущей строки при условии что фокус не на таблице
#define COLOR_CURRENT_LINE_BACKGROUND_NOT_FOCUSED QColor(QStringLiteral("#ebf5ff"))
// цвет фона текущей строки при условии что фокус на таблице
#define COLOR_CURRENT_LINE_BACKGROUND_FOCUSED QColor(QStringLiteral("#ebf5ff"))
// цвет фона выделенной ячейки при условии что фокус не на таблице
#define COLOR_SELECTED_BACKGROUND_NOT_FOCUSED QColor(QStringLiteral("#bfdfff"))
// цвет фона выделенной ячейки при условии что фокус на таблице
#define COLOR_SELECTED_BACKGROUND_FOCUSED QColor(QStringLiteral("#04aa6d"))
// Надо ли менять цвет текст для текущей ячейки, на которой нет фокуса ввода
#define CHANGE_FONT_COLOR_FOR_NOT_FOCUSED false

        bool has_selection = false;
        // находится ли таблица в фокусе
        bool is_focused = item_view->hasFocus();

        // для оптимизации не используем selectedRows
        const QItemSelection selection = item_view->selectionModel()->selection();
        // выделено больше одной строки
        bool selected_more_one = selection.count() > 1 || (selection.count() == 1 && selection.at(0).bottom() > selection.at(0).top());
        QModelIndex first_selected;
        if (!selection.isEmpty())
            first_selected = item_view->model()->index(selection.at(0).top(), 0, selection.at(0).parent());

        if (!selection.isEmpty() && !selected_more_one) {
            bool is_select_current_row = first_selected.row() == current_index.row() && first_selected.parent() == current_index.parent();
            if (is_select_current_row) {
                // выделена одна строка и она же является текущей. делаем вид что вообще ничего не выделено
                is_selected_row = false;
                is_current_cell = index == current_index;

            } else {
                // выделена не текущая строка. выделяем ее и не показываем текущую
                is_selected_row = first_selected.row() == index.row() && first_selected.parent() == index.parent();
                is_current_cell = false;
                has_selection = true;
            }

        } else if (selected_more_one) {
            // выделено более одной строки
            is_selected_row = item_view->selectionModel()->isRowSelected(index.row(), index.parent());
            is_current_cell = false;
            has_selection = true;

        } else {
            // выделения нет
            is_selected_row = false;
            is_current_cell = current_index == index;
        }

        // текущая строка
        if (!has_selection && current_index.isValid() && item_view->selectionBehavior() == QAbstractItemView::SelectRows
            && current_index.parent() == index.parent()) {
            // Текущая активная строка в режиме QAbstractItemView::SelectRows
            // Проверяем span
            QTableView* tw = qobject_cast<QTableView*>(item_view);
            if ((!tw && current_index.row() == index.row())
                || (tw && index.row() >= current_index.row() && index.row() < current_index.row() + tw->rowSpan(current_index.row(), current_index.column()))) {
                is_current_row = true;
            }
        }

        if (is_current_cell || is_selected_row)
            option->backgroundBrush = is_focused ? COLOR_SELECTED_BACKGROUND_FOCUSED : COLOR_SELECTED_BACKGROUND_NOT_FOCUSED;
        else if (is_current_row)
            option->backgroundBrush = is_focused ? COLOR_CURRENT_LINE_BACKGROUND_FOCUSED : COLOR_CURRENT_LINE_BACKGROUND_NOT_FOCUSED;
        else
            option->backgroundBrush = QColor(Qt::white);

        QColor font_color;
        if ((is_current_cell || is_selected_row) && !index.data(Qt::CheckStateRole).isValid() && (CHANGE_FONT_COLOR_FOR_NOT_FOCUSED || is_focused))
            font_color = QColor(Qt::white);
        else
            font_color = QColor(Qt::black);
        option->palette.setColor(QPalette::Text, font_color);
    }

    // переопределяем цвета
    QBrush background_brush = option->backgroundBrush;
    getCellProperties(source_index, option);
    // Защита от дурака. Текущую строку переопределять не даем (шрифт однако менять разрешаем)
    if (is_current_cell && is_current_row) {
        option->backgroundBrush = background_brush;
    }

    // Иконка могла быть переназначена внутри getDatasetCellProperties
    if (!option->icon.isNull()) {
        option->features |= QStyleOptionViewItem::HasDecoration;
        option->decorationSize = iconSize(option->icon);
        option->decorationAlignment = Qt::AlignCenter;
    } else {
        option->decorationSize = {Z_PM(PM_SmallIconSize), Z_PM(PM_SmallIconSize)};
    }

    // Убираем флаги выделения, чтобы Qt не перерисовывал фокус
    option->state = option->state & (~(QStyle::State_HasFocus | QStyle::State_Selected));

    // переопределяем текст
    option->text = getDisplayText(source_index, option);

    if (!option->icon.isNull() && option->text.isEmpty()) {
        option->displayAlignment = Qt::AlignCenter;
    }
}

} // namespace zf
