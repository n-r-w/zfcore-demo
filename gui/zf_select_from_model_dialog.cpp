#include "zf_select_from_model_dialog.h"
#include "ui_zf_select_from_model_dialog.h"

#include "zf_core.h"
#include "zf_table_view.h"
#include "zf_tree_view.h"
#include "zf_proxy_item_model.h"
#include "zf_framework.h"
#include "zf_callback.h"
#include "zf_translation.h"
#include "zf_i_cell_column_check.h"

#include <QClipboard>
#include <QMovie>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

namespace zf
{
//! Прокси модель, для организации головных и подчиненных наборов данных
class SelectionProxyItemModel : public LeafFilterProxyModel
{
public:
    SelectionProxyItemModel(
        //! Заголовок с отображаемой информацией
        QHeaderView* header_view,
        //! Представление (может быть nullptr)
        View* view,
        //! Положение колонок фильтрации
        const QList<int>& _filter_columns_pos,
        //! Где находятся данные (может быть nullptr)
        QAbstractItemModel* source_item_model,
        //! Где находятся данные (может быть nullptr)
        DataFilter* filter,
        //! Дополнительный фильтр (может быть nullptr)
        DataFilter* filter_external,
        //! Набор данных, на который назначен ProxyItemModel (может быть не задан)
        const DataProperty& dataset_property,
        //! Колонка, которая отвечает за отображение в наборе данных. Только для моделей без представления
        int display_column_pos,
        //! Интерфейс для унификации доступа к управлению чекбоксами ячеек TableView/TreeView (только для SelectionModel::Multi)
        I_CellColumnCheck* cell_column_check_interface,
        //! Родитель
        QObject* parent);

    bool isFiltering() const;
    void setFilterText(const QString& filter_text);
    QString filterText() const { return _filter_text; }
    //! Видимость строки бех учета дочерних
    bool filterAcceptsRowItself(int source_row, const QModelIndex& source_parent,
        //! Безусловно исключить строку из иерархии
        bool& exclude_hierarchy) const override;
    //! Уникальный ключ строки
    RowID getRowID(int source_row, const QModelIndex& source_parent) const override;

    //! Заголовок с отображаемой информацией
    QHeaderView* _header_view;
    //! Фильтр
    DataFilter* _filter;
    //! Представление
    View* _view = nullptr;
    //! Набор данных
    DataProperty _dataset_property;
    //! Положение колонок фильтрации
    QList<int> _filter_columns_pos;

    //! Текст для фильтрации
    QString _filter_text;
    //! В процессе фильтрации
    bool _is_filtering = false;
    //! Основной прокси
    ProxyItemModel* _proxy = nullptr;
    //! Контрол с отображаемой информацией
    QAbstractItemView* _item_view = nullptr;

    //! Колонка, которая отвечает за отображение в наборе данных. Только для моделей без представления
    int _display_column_pos = -1;
    //! Интерфейс для унификации доступа к управлению чекбоксами ячеек TableView/TreeView (только для SelectionModel::Multi)
    I_CellColumnCheck* _cell_column_check_interface = nullptr;

    //! Цепочка прокси для основного фильтра
    QList<QAbstractProxyModel*> _proxy_chain;
    //! Цепочка прокси для дополнительного фильтра
    QList<QAbstractProxyModel*> _external_proxy_chain;

    QAbstractItemModel* _source_item_model = nullptr;
};

SelectionProxyItemModel::SelectionProxyItemModel(QHeaderView* header_view, View* view, const QList<int>& filter_columns_pos,
    QAbstractItemModel* source_item_model, DataFilter* filter, DataFilter* filter_external, const DataProperty& dataset_property, int display_column_pos,
    I_CellColumnCheck* cell_column_check_interface, QObject* parent)
    : LeafFilterProxyModel(LeafFilterProxyModel::SearchMode, parent)
    , _header_view(header_view)
    , _filter(filter)
    , _view(view)
    , _dataset_property(dataset_property)
    , _filter_columns_pos(filter_columns_pos)
    , _display_column_pos(display_column_pos)
    , _cell_column_check_interface(cell_column_check_interface)
    , _source_item_model(source_item_model)

{
    Z_CHECK(!_filter_columns_pos.isEmpty());
    Z_CHECK_NULL(_header_view);
    Z_CHECK(!_dataset_property.options().testFlag(PropertyOption::SimpleDataset));

    if (source_item_model != nullptr) {
        Z_CHECK(_view == nullptr);
        Z_CHECK(_filter == nullptr);
        Z_CHECK(filter_external == nullptr);
        Z_CHECK(!_dataset_property.isValid());
    }

    if (filter != nullptr) {
        Z_CHECK(_view == nullptr || _view->filter() == filter);
        Z_CHECK(_source_item_model == nullptr || (_view != nullptr && _view->dataset(_dataset_property) == _source_item_model));

        _proxy = filter->proxyDataset(_dataset_property);
        _source_item_model = _proxy;

        // цепочка прокси для преобразования индексов
        QAbstractItemModel* filter_base = nullptr;
        QAbstractProxyModel* p = _proxy;
        while (true) {
            _proxy_chain.prepend(p);
            p = qobject_cast<QAbstractProxyModel*>(p->sourceModel());
            if (p == nullptr) {
                filter_base = _proxy_chain.at(0)->sourceModel();
                Z_CHECK(filter_base == filter->source()->data()->dataset(_dataset_property));
                break;
            }
        }

        if (filter_external != nullptr && filter_external != filter) {
            // надо найти общего родителя у обеих фильтров, чтобы можно было делать преобразование индексов
            p = filter_external->proxyDataset(_dataset_property);
            while (true) {
                _external_proxy_chain.prepend(p);

                int pos = _proxy_chain.indexOf(p);
                if (pos >= 0) {
                    while (pos > 0) {
                        _proxy_chain.removeFirst();
                        pos--;
                    }
                    break;
                }

                p = qobject_cast<QAbstractProxyModel*>(p->sourceModel());
                if (p == nullptr) {
                    Z_CHECK(_external_proxy_chain.at(0)->sourceModel() == filter_base);
                    break;
                }
            }
        }
    }

    if (_view != nullptr)
        _item_view = _view->object<QAbstractItemView>(_dataset_property);

    setUseCache(true);
    setSourceModel(_source_item_model);
}

bool SelectionProxyItemModel::isFiltering() const
{
    return _is_filtering;
}

void SelectionProxyItemModel::setFilterText(const QString& filter_text)
{
    QString f = filter_text.trimmed().simplified();

    if (_filter_text == f)
        return;

    _is_filtering = true;
    _filter_text = f;
    beginResetModel();
    resetCache();
    endResetModel();

    //    invalidateFilter();

    _is_filtering = false;
}

bool SelectionProxyItemModel::filterAcceptsRowItself(int source_row, const QModelIndex& source_parent, bool& exclude_hierarchy) const
{
    if (!_external_proxy_chain.isEmpty()) {
        Z_CHECK_NULL(_proxy);

        if (_cell_column_check_interface != nullptr) {
            // выделенные элементы при SelectionMode::Multi показываем всегда
            Z_CHECK(_display_column_pos >= 0);
            if (_cell_column_check_interface->isCellChecked(_proxy->index(source_row, _display_column_pos, source_parent)))
                return true;
        }

        // сначала откатываем индекс до базового
        QModelIndex index = _proxy->index(source_row, 0, source_parent);
        for (int i = _proxy_chain.count() - 1; i >= 0; i--) {
            index = _proxy_chain.at(i)->mapToSource(index);
            Z_CHECK(index.isValid());
        }
        // потом поднимаемся вверх по _external_proxy_chain
        for (int i = 0; i < _external_proxy_chain.count(); i++) {
            index = _external_proxy_chain.at(i)->mapFromSource(index);
            if (!index.isValid())
                return false; // строка отфильтрована через filter_external
        }

    } else {
        if (_cell_column_check_interface != nullptr) {
            // выделенные элементы при SelectionMode::Multi показываем всегда
            Z_CHECK(_display_column_pos >= 0);
            if (_cell_column_check_interface->isCellChecked(_source_item_model->index(source_row, _display_column_pos, source_parent)))
                return true;
        }
    }

    if (_filter_text.isEmpty())
        return true;

    bool all_false = true;
    QVariant value;
    QString converted;
    for (int i = 0; i < _filter_columns_pos.count(); i++) {
        if (_header_view->isSectionHidden(_filter_columns_pos.at(i)))
            continue;

        QModelIndex index = _source_item_model->index(source_row, _filter_columns_pos.at(i), source_parent);
        value = index.data(Qt::DisplayRole);
        if (_view != nullptr) {
            QList<ModelPtr> data_not_ready;
            _view->getDatasetCellVisibleValue(index.row(), _dataset_property.columns().at(_filter_columns_pos.at(i)), index.parent(), value,
                VisibleValueOption::Application, converted, data_not_ready);
            for (auto& m : qAsConst(data_not_ready)) {
                Z_CHECK(m->isLoading());
            }

            if (!data_not_ready.isEmpty())
                return false;

        } else {
            converted = value.toString().trimmed();
        }

        if (converted.contains(_filter_text, Qt::CaseInsensitive)) {
            all_false = false;
            break;
        }
    }

    if (all_false)
        return false;

    return LeafFilterProxyModel::filterAcceptsRowItself(source_row, source_parent, exclude_hierarchy);
}

RowID SelectionProxyItemModel::getRowID(int source_row, const QModelIndex& source_parent) const
{
    QModelIndex index = _source_item_model->index(source_row, 0, source_parent);
    for (int i = _proxy_chain.count() - 1; i >= 0; i--) {
        index = _proxy_chain.at(i)->mapToSource(index);
        Z_CHECK(index.isValid());
    }

    if (_filter != nullptr)
        return _filter->source()->datasetRowID(_dataset_property, index.row(), index.parent());

    return RowID::createRealKey(index);
}

SelectFromModelDialog::SelectFromModelDialog(SelectionMode::Enum selection_mode, const ModelPtr& model, const DataProperty& dataset,
    const DataProperty& value_column, int value_role, const DataProperty& display_column, const QList<int>& multi_selection_levels,
    const DataFilterPtr& data_filter, const DataPropertyList filter_by_columns, const QList<int>& buttons_list, const QStringList& buttons_text,
    const QList<int>& buttons_role)
    : Dialog(buttons_list.isEmpty() ? buttonsList(selection_mode) : buttons_list, buttons_text.isEmpty() ? buttonsText(selection_mode) : buttons_text,
        buttons_role.isEmpty() ? buttonsRole(selection_mode) : buttons_role)
    , _data_mode(DataMode::Structure)
    , _selection_mode(selection_mode)
    , _model(model)
    , _data_filter_external(data_filter)
    , _value_role(value_role)
    , _filter_by_columns(filter_by_columns)
    , _multi_selection_levels(multi_selection_levels)
{
    Z_CHECK_NULL(_model);

    setDialogCode("SelectFromModelDialog_" + QString::number(_model->entityCode().value()));

    _icon = _model->moduleInfo().icon();
    _name = _model->entityName();

    if (dataset.isValid()) {
        _dataset = dataset;
        Z_CHECK(dataset.propertyType() == PropertyType::Dataset);
        Z_CHECK(_model->structure()->contains(dataset));
    } else {
        Z_CHECK(_model->structure()->isSingleDataset());
        _dataset = _model->structure()->singleDataset();
    }

    // определяем колонку для позиционирования
    if (!value_column.isValid()) {
        auto id_column = _model->structure()->datasetColumnsByOptions(_dataset, PropertyOption::Id);
        if (!id_column.isEmpty()) {
            _value_column = id_column.first();
        } else {
            for (auto& p : _dataset.columns()) {
                if (!p.options().testFlag(PropertyOption::Hidden)) {
                    _value_column = p;
                    break;
                }
            }
        }
    } else {
        _value_column = value_column;
    }
    Z_CHECK(_model->structure()->contains(_value_column));
    _value_column_pos = _value_column.pos();

    if (_selection_mode == SelectionMode::Multi) {
        // определяем колонку которая отвечает за отображение чекбоксов множественного выбора
        if (!display_column.isValid() || display_column.options().testFlag(PropertyOption::Hidden)) {
            auto id_column = _model->structure()->datasetColumnsByOptions(_dataset, PropertyOption::FullName);
            if (id_column.isEmpty())
                id_column = _model->structure()->datasetColumnsByOptions(_dataset, PropertyOption::Name);

            for (auto& p : id_column.isEmpty() ? _dataset.columns() : id_column) {
                if (!p.options().testFlag(PropertyOption::Hidden)) {
                    _display_column = p;
                    break;
                }
            }

        } else {
            _display_column = display_column;
        }

    } else {
        _display_column = display_column;
    }

    if (!_display_column.isValid()) {
        for (auto& p : _dataset.columns()) {
            if (!p.options().testFlag(PropertyOption::Hidden)) {
                _display_column = p;
                break;
            }
        }
    }

    Z_CHECK(_model->structure()->contains(_display_column));
    _display_column_pos = _display_column.pos();

    // по каким колонкам фильтровать
    if (_filter_by_columns.isEmpty()) {
        for (auto& p : _dataset.columns()) {
            if ((p.options() & PropertyOption::Hidden) == 0) {
                _filter_by_columns << p;
            }
        }
    }
    Z_CHECK(!_filter_by_columns.isEmpty());

    init({});
}

SelectFromModelDialog::SelectFromModelDialog(SelectionMode::Enum selection_mode, const QString& dialog_code, const QString& caption, const QIcon& icon,
    QAbstractItemModel* model, int value_column, int value_role, int display_column, int display_role, const QList<int>& multi_selection_levels,
    const QList<int>& filter_by_columns, const QList<int>& buttons_list, const QStringList& buttons_text, const QList<int>& buttons_role)
    : Dialog(buttons_list.isEmpty() ? buttonsList(selection_mode) : buttons_list, buttons_text.isEmpty() ? buttonsText(selection_mode) : buttons_text,
        buttons_role.isEmpty() ? buttonsRole(selection_mode) : buttons_role)
    , _data_mode(DataMode::ItemModel)
    , _selection_mode(selection_mode)
    , _icon(icon)
    , _name(caption)
    , _source_model(model)
    , _value_column_pos(value_column)
    , _value_role(value_role)
    , _display_column_pos(display_column)
    , _display_role(display_role)
    , _multi_selection_levels(multi_selection_levels)
{
    Z_CHECK_NULL(model);
    Z_CHECK(!dialog_code.isEmpty());
    Z_CHECK(value_column >= 0 && value_column < model->columnCount());
    Z_CHECK(display_column >= 0 && display_column < model->columnCount());

    setDialogCode("SelectFromModelDialog_" + dialog_code);

    init(!filter_by_columns.isEmpty() ? filter_by_columns : QList<int> {value_column});
}

SelectFromModelDialog::~SelectFromModelDialog()
{
    if (_view != nullptr)
        _view->saveConfiguration();
    delete _ui;
}

SelectionMode::Enum SelectFromModelDialog::selectionMode() const
{
    return _selection_mode;
}

bool SelectFromModelDialog::onlyBottomItem() const
{
    return _only_bottom_item;
}

void SelectFromModelDialog::setOnlyBottomItem(bool b)
{
    if (_only_bottom_item == b)
        return;

    _only_bottom_item = b;

    if (_tree_view)
        _tree_view->setFormatBottomItems(_only_bottom_item);
}

bool SelectFromModelDialog::select(const QVariantList& values)
{
    setSelected(values);

    Utils::setFocus(view(), false);

    if (static_cast<QDialogButtonBox::StandardButton>(exec()) == QDialogButtonBox::Ok)
        return true;
    else
        return false;
}

QVariantList SelectFromModelDialog::selectedValues() const
{
    QVariantList res;

    auto indexes = selectedIndexes();
    for (auto& i : qAsConst(indexes)) {
        res << i.model()->index(i.row(), _value_column_pos, i.parent()).data(_value_role);
    }

    return res;
}

bool SelectFromModelDialog::setSelected(const QVariantList& values)
{
    QModelIndexList indexes;

    for (auto& value : values) {
        QModelIndex index;
        if (_data_mode == DataMode::Structure) {
            Z_CHECK_NULL(_model);
            Rows rows = _model->hash()->findRows(_value_column, value, false, Qt::DisplayRole);
            if (!rows.isEmpty()) {
                index = _data_filter->proxyIndex(rows.at(0));
                if (index.isValid())
                    index = _proxy->mapFromSource(index);
            }
        } else {
            if (_proxy->rowCount() > 0)
                index = Utils::searchItemModel(_proxy->index(0, _value_column_pos), value, true);
        }

        if (!index.isValid())
            continue;

        indexes << index;
    }

    Z_CHECK(setSelected(indexes));
    return values.count() == indexes.count();
}

QModelIndexList SelectFromModelDialog::selectedIndexes() const
{
    if (_selection_mode == SelectionMode::Single) {
        auto index = view()->currentIndex();

        if (!index.isValid())
            return {};

        index = _proxy->mapToSource(index);
        Z_CHECK(index.isValid());
        if (_data_filter != nullptr)
            index = _data_filter->sourceIndex(index);

        Z_CHECK(index.isValid());
        return {index.model()->index(index.row(), 0, index.parent())};

    } else {
        // для множественного выбора используем чекбоксы в TableView/TreeView
        auto indexes = _cell_column_check_interface->checkedCells();
        std::sort(indexes.begin(), indexes.end());
        for (int i = 0; i < indexes.count(); i++) {
            QModelIndex index = indexes.at(i);
            Z_CHECK(index.isValid());
            indexes[i] = index.model()->index(index.row(), 0, index.parent());
        }
        return indexes;
    }
}

bool SelectFromModelDialog::setSelected(const QModelIndexList& indexes)
{
    if (indexes.isEmpty()) {
        clearSelected();
        return true;
    }

    if (_selection_mode == SelectionMode::Single) {
        Z_CHECK(indexes.count() == 1);
        QModelIndex index = indexes.constFirst();
        QModelIndex new_index;
        if (index.model() == _source_model)
            new_index = _proxy->mapFromSource(index);
        else {
            Z_CHECK(index.model() == _proxy);
            new_index = index;
        }

        int current_col = -1;
        if (view()->currentIndex().isValid())
            current_col = view()->currentIndex().column();
        new_index = _proxy->index(new_index.row(), current_col >= 0 ? current_col : firstVisibleColumn(), new_index.parent());

        if (_tree_view)
            _tree_view->expandAll();

        view()->setCurrentIndex(new_index);
        view()->scrollTo(new_index, QAbstractItemView::PositionAtCenter);

        testEnabled();
        return new_index.isValid();

    } else {
        for (auto& i : indexes) {
            Z_CHECK(i.isValid());
            _block_check_reaction++;
            _cell_column_check_interface->setCellChecked(i.model()->index(i.row(), _display_column_pos, i.parent()), true);
            _block_check_reaction--;
        }
        return true;
    }
}

QList<RowID> SelectFromModelDialog::selectedIds() const
{
    Z_CHECK(_data_mode == DataMode::Structure);

    QList<RowID> res;
    auto indexes = selectedIndexes();
    for (auto& index : qAsConst(indexes)) {
        if (!index.isValid())
            continue;

        auto id = _model->datasetRowID(_dataset, index.row(), index.parent());
        Z_CHECK(id.isValid());
        res << id;
    }

    return res;
}

bool SelectFromModelDialog::setSelected(const QList<RowID>& row_ids)
{
    Z_CHECK(_data_mode == DataMode::Structure);

    QModelIndexList indexes;
    for (auto& r : row_ids) {
        Z_CHECK(r.isValid());
        auto index = _model->findDatasetRowID(_dataset, r);
        if (index.isValid())
            indexes << index;
    }

    Z_CHECK(setSelected(indexes));
    return indexes.count() == row_ids.count();
}

void SelectFromModelDialog::clearSelected()
{
    if (_selection_mode == SelectionMode::Single)
        view()->setCurrentIndex(QModelIndex());
    else
        _cell_column_check_interface->clearCheckedCells();

    testEnabled();
}

bool SelectFromModelDialog::isFilterVisible() const
{
    return !_ui->filter_container->isHidden();
}

void SelectFromModelDialog::setFilterVisible(bool b)
{
    _ui->filter_container->setHidden(!b);
}

void SelectFromModelDialog::setFilterText(const QString& s)
{
    if (objectExtensionDestroyed())
        return;

    if (!isFilterVisible())
        return;

    QString sTmp = s.trimmed();
    if (_proxy->filterText() == sTmp)
        return;

    if (_proxy->isFiltering()) {
        Framework::internalCallbackManager()->addRequest(this, Framework::SELECT_FROM_MODEL_DIALOG_SET_FILTER_KEY, true);
        _need_refilter_text = s;
        return;
    }

    Utils::setWaitCursor();

    RowID row_id;
    if (_model != nullptr && view()->currentIndex().isValid()) {
        auto index = Utils::alignIndexToModel(view()->currentIndex(), _model->dataset(_dataset));
        Z_CHECK(index.isValid());
        row_id = _model->datasetRowID(_dataset, index.row(), index.parent());
    }

    _proxy->setFilterText(sTmp);

    if (_tree_view)
        _tree_view->expandAll();

    QModelIndex index;

    if (row_id.isValid()) {
        auto old_index = _model->findDatasetRowID(_dataset, row_id);
        Z_CHECK(old_index.isValid());
        index = Utils::alignIndexToModel(old_index, _proxy);
    }

    if (!index.isValid())
        index = view()->currentIndex();

    if (_proxy->rowCount() > 0) {
        if (_tree_view && _only_bottom_item) {
            auto top_index = index.isValid() ? _proxy->index(index.row(), 0, index.parent()) : _proxy->index(0, 0);
            while (top_index.isValid() && _proxy->rowCount(top_index) > 0) {
                top_index = _proxy->index(0, 0, top_index);
            }
            index = _proxy->index(top_index.row(), firstVisibleColumn(), top_index.parent());

        } else {
            if (!index.isValid())
                index = _proxy->index(0, firstVisibleColumn());
        }
    }

    if (index.isValid()) {
        view()->setCurrentIndex(index);
        view()->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }

    Utils::restoreCursor();

    if (!Framework::internalCallbackManager()->isRequested(this, Framework::SELECT_FROM_MODEL_DIALOG_SET_FILTER_KEY) && _ui->search_item->text() != s)
        _ui->search_item->setText(s);

    testEnabled();
}

bool SelectFromModelDialog::isAllowEmptySelection() const
{
    return _allow_empty_selection;
}

void SelectFromModelDialog::setAllowEmptySelection(bool b)
{
    if (_allow_empty_selection == b)
        return;

    _allow_empty_selection = b;
    testEnabled();
}

QWidget* SelectFromModelDialog::headerWorkspace() const
{
    return _ui->header_container;
}

bool SelectFromModelDialog::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* event = static_cast<QKeyEvent*>(e);

        QString clipboard_text;
        if (event->matches(QKeySequence::Paste)) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            clipboard_text = clipboard->text();
        }

        if ((event->key() == Qt::Key_Escape && (event->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier)) {
            if (!_proxy->isFiltering())
                reject();
            return true;
        }

        if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && ((event->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier)
            && isOkButtonEnabled()) {
            if (!_proxy->isFiltering())
                acceptSelection();

            return true;
        }

        // переходы только по узлам нижнего уровня
        if (((event->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier) && (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) && _tree_view
            && _only_bottom_item && _tree_view->currentIndex().isValid()) {
            auto next_index = Utils::getNextItemModelIndex(_tree_view->currentIndex(), event->key() == Qt::Key_Down);
            while (next_index.isValid() && _proxy->rowCount(next_index) > 0) {
                next_index = Utils::getNextItemModelIndex(next_index, event->key() == Qt::Key_Down);
            }
            if (next_index.isValid() && _proxy->rowCount(next_index) == 0) {
                next_index = _proxy->index(next_index.row(), firstVisibleColumn(), next_index.parent());
                _tree_view->setCurrentIndex(next_index);
                scrollTo(next_index);

                return true;
            }
        }

        if (obj != view()->viewport()) {
            // Реакция на стрелки когда фокус не на таблице
            bool send_to_view = false;
            if (((event->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier)
                && (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown)) {
                send_to_view = true;

            } else if ((event->modifiers() == Qt::ControlModifier && (event->key() == Qt::Key_Home || event->key() == Qt::Key_End))
                       || (((event->modifiers() | Qt::KeypadModifier) == Qt::KeypadModifier) && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
                           && _ui->search_item->text().isEmpty())) {
                send_to_view = true;
            }

            if (send_to_view) {
                // Для QAbstractItemView зачем то спрятали event в protected. Поэтому приводим к QObject
                static_cast<QObject*>(view())->event(event);
                Utils::setFocus(view());
                return true;
            }
        }

        if (obj != _ui->search_item) {
            // переключение множественного выбора по пробелу
            if (_selection_mode == SelectionMode::Multi && event->key() == Qt::Key_Space && view()->currentIndex().isValid()) {
                Z_CHECK_NULL(_cell_column_check_interface);
                _block_check_reaction++;
                _cell_column_check_interface->setCellChecked(view()->currentIndex(), !_cell_column_check_interface->isCellChecked(view()->currentIndex()));
                _block_check_reaction--;
                return true;
            }

            // Нужно ли передать символ в строку отбора, если курсор не на ней
            if (isFilterVisible()) {
                if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Left || event->key() == Qt::Key_Right
                    || (!event->text().isEmpty() && event->text().at(0).isPrint()) || !clipboard_text.isEmpty()) {
                    bool need_focus = false;
                    if (clipboard_text.isEmpty()) {
                        if ((!event->text().isEmpty() && event->text().at(0).isPrint()) || !_ui->search_item->text().isEmpty()) {
                            _ui->search_item->event(e);
                            need_focus = true;
                        }
                    } else {
                        setFilterText(clipboard_text);
                        need_focus = true;
                    }

                    if (need_focus && !_ui->search_item->hasFocus()) {
                        Utils::setFocus(_ui->search_item, false);
                        return true;
                    }
                }
            }
        }
    }

    return Dialog::eventFilter(obj, e);
}

void SelectFromModelDialog::onCallbackManagerInternal(int key, const QVariant& data)
{
    Dialog::onCallbackManagerInternal(key, data);

    if (key == Framework::SELECT_FROM_MODEL_DIALOG_SET_FILTER_KEY) {
        setFilterText(_need_refilter_text);

    } else if (key == Framework::SELECT_FROM_MODEL_DIALOG_ENABLED_KEY) {
        testEnabled_helper();
    }
}

bool SelectFromModelDialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (_proxy->isFiltering())
        return true;

    if (button == QDialogButtonBox::Reset) {
        clearSelected();
        return false;

    } else if (button == QDialogButtonBox::Help) {
        Z_CHECK(_selection_mode == SelectionMode::Multi);
        for (int row = 0; row < view()->model()->rowCount(); row++) {
            _cell_column_check_interface->setCellChecked(view()->model()->index(row, _display_column_pos), true);
        }
        return false;
    }

    return Dialog::onButtonClick(button);
}

void SelectFromModelDialog::beforePopup()
{
    Dialog::beforePopup();
}

void SelectFromModelDialog::afterPopup()
{
    Dialog::afterPopup();

    if (_view != nullptr)
        _view->loadConfiguration();

    Utils::processEvents();
    Core::messageDispatcher()->start();
    delete _wait_movie_label;
    _wait_movie_label = nullptr;
    _ui->view_container->addWidget(view());

    if (_tree_view)
        _tree_view->expandAll();

    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(0);
    connect(timer, &QTimer::timeout, this, [&, timer]() {
        // без этого в центре не позиционируется
        if (view()->currentIndex().isValid())
            view()->scrollTo(view()->currentIndex(), QAbstractItemView::PositionAtCenter);

        Utils::setFocus(view());

        timer->deleteLater();
    });
}

void SelectFromModelDialog::beforeHide()
{
    if (!objectExtensionDestroyed()) {
        Framework::internalCallbackManager()->removeRequest(this, Framework::SELECT_FROM_MODEL_DIALOG_SET_FILTER_KEY);
        Framework::internalCallbackManager()->removeRequest(this, Framework::SELECT_FROM_MODEL_DIALOG_ENABLED_KEY);
    }

    Dialog::beforeHide();
}

void SelectFromModelDialog::afterHide()
{
    _search_delay_timer->stop();
    Dialog::afterHide();
}

void SelectFromModelDialog::adjustDialogSize()
{
    adjustSize();
    QSize maxSize = Utils::screenSize();

    int w = qMax(rootHeader()->bottomCount(true) * Utils::scaleUI(200), maxSize.width() / 3);
    Utils::resizeWindowToScreen(this, QSize(qMax(w, geometry().width()), qMax(maxSize.height() / 2, geometry().height())), true, 10);
}

void SelectFromModelDialog::sl_doubleClicked(const QModelIndex& index)
{
    if (_proxy->isFiltering())
        return;

    if (_selection_mode == SelectionMode::Multi) {
        if (index.column() == _display_column_pos)
            _cell_column_check_interface->setCellChecked(index, !_cell_column_check_interface->isCellChecked(index));

    } else {
        if (!index.isValid() || !isOkButtonEnabled())
            return;

        acceptSelection();
    }
}

void SelectFromModelDialog::sl_textEdited()
{
    _search_delay_timer->start();
}

void SelectFromModelDialog::sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    testEnabled();
}

void SelectFromModelDialog::sl_searchDelayTimer()
{
    _search_delay_timer->stop();
    setFilterText(_ui->search_item->text());
}

void SelectFromModelDialog::sl_cellCheckedChanged(const QModelIndex& index, bool checked)
{
    Z_CHECK_NULL(_cell_column_check_interface);
    if (_block_check_reaction > 0)
        return;

    int level = 0;
    QModelIndex idx = index;
    while (idx.parent().isValid()) {
        level++;
        idx = idx.parent();
    }

    if (!_cell_column_check_interface->cellCheckColumns(level + 1).isEmpty()) {
        // выделение дочерних
        for (int row = 0; row < index.model()->rowCount(index); row++) {
            _cell_column_check_interface->setCellChecked(index.model()->index(row, index.column(), index), checked);
        }
    }

    if (level > 0 && !_cell_column_check_interface->cellCheckColumns(level - 1).isEmpty()) {
        // если на этом уровне высе выбраны, то надо отметить родителя
        bool all_checked = true;
        for (int row = 0; row < index.model()->rowCount(index.parent()); row++) {
            if (!_cell_column_check_interface->isCellChecked(index.model()->index(row, index.column(), index.parent()))) {
                all_checked = false;
                break;
            }
        }
        _block_check_reaction++;
        _cell_column_check_interface->setCellChecked(index.model()->index(index.parent().row(), index.column(), index.parent().parent()), all_checked);
        _block_check_reaction--;
    }

    testEnabled();
}

void SelectFromModelDialog::init(const QList<int>& filter_columns_pos)
{
    Core::registerNonParentWidget(this);

    Core::messageDispatcher()->stop();

    if (_selection_mode == SelectionMode::Multi)
        button(QDialogButtonBox::Help)->setIcon(QIcon(":/share_icons/select_all.svg"));

    _ui = new Ui::ZFSelectFromModelDialog;
    _ui->setupUi(workspace());

    _wait_movie_label = new QLabel;
    _ui->view_container->addWidget(_wait_movie_label);
    _wait_movie_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _wait_movie_label->setAlignment(Qt::AlignCenter);
    _wait_movie_label->setObjectName(QStringLiteral("zfwm"));
    _wait_movie = new QMovie(":/share_icons/loader.gif");
    _wait_movie_label->setMovie(_wait_movie);
    _wait_movie_label->show();
    _wait_movie->start();

    setBottomLineVisible(false);
    _ui->select_label->setText(ZF_TR(ZFT_SELECTION));
    _ui->header_container->setHidden(true);

    _search_delay_timer = new QTimer(this);
    _search_delay_timer->setSingleShot(true);
    _search_delay_timer->setInterval(Consts::USER_INPUT_TIMEOUT_MS);
    connect(_search_delay_timer, &QTimer::timeout, this, &SelectFromModelDialog::sl_searchDelayTimer);

    if (_data_filter_external != nullptr) {
        Z_CHECK(_data_filter_external->source() == _model.get());
    }

    // есть ли представление
    if (_data_mode == DataMode::Structure) {
        _source_model = _model->data()->dataset(_dataset);

        Error error;
        _view = Core::createView<View>(_model, ViewState::SelectItemModelDialog, {}, error, false);
        if (error.isError())
            Z_HALT(error);
        if (_view == nullptr) {
            _view = Core::createView<View>(_model, ViewState::MdiWindow, {}, error, false);
            if (error.isError())
                Z_HALT(error);
        }
        if (_view != nullptr)
            _view->setCustomConfigurationCode(_view->customConfigurationCode() + "_sim");
    }

    bool show_header = false;
    if (_view == nullptr) {
        // представления нет, надо создать TableView или TreeView
        HeaderItem* root = nullptr;

        if (_data_mode == DataMode::ItemModel || _dataset.dataType() == DataType::Table) {
            _table_view = new TableView(this);
            root = _table_view->horizontalRootHeaderItem();

        } else if (_dataset.dataType() == DataType::Tree) {
            _tree_view = new TreeView(this);
            root = _tree_view->rootHeaderItem();
            _tree_view->setTreePosition(_display_column_pos);
        } else {
            Z_HALT_INT;
        }

        root->beginUpdate();
        for (int col = 0; col <= _display_column_pos; col++) {
            auto header = root->append();

            if (_dataset.isValid() && _dataset.columns().count() > col)
                header->setLabel(_dataset.columns().at(col).name());

            if (col < _display_column_pos) {
                if (_dataset.isValid() && _dataset.columns().count() > col
                    && _dataset.columns().at(col).options().testFlag(PropertyOption::VisibleItemModelDialog)) {
                    header->setVisible(true);
                    show_header = true;

                } else {
                    header->setPermanentHidden(true);
                }
            } else {
                header->setResizeMode(QHeaderView::Stretch);
            }
        }
        root->endUpdate();

        if (_data_mode == DataMode::Structure) {
            // представления нет, создаем фильтр сами
            _data_filter_ptr = Z_MAKE_SHARED(DataFilter, _model.get());
            _data_filter = _data_filter_ptr.get();
        }

    } else {
        Z_CHECK(_data_mode == DataMode::Structure);
        // берем готовую талицу из представления
        if (_dataset.dataType() == DataType::Table)
            _table_view = _view->object<TableView>(_dataset);
        else if (_dataset.dataType() == DataType::Tree)
            _tree_view = _view->object<TreeView>(_dataset);
        else
            Z_HALT_INT;

        // есть представление, берем его фильтр
        _data_filter = _view->filter();
        Z_CHECK(_view->filter() == _data_filter);

        auto root = _view->horizontalRootHeaderItem(_dataset);
        auto bottom_items = root->allBottom();

        root->beginUpdate();
        for (auto i : qAsConst(bottom_items)) {
            auto column = _dataset.columns().at(i->bottomPos());
            if (column.options().testFlag(PropertyOption::VisibleItemModelDialog)) {
                i->setPermanentHidden(false);
                i->setVisible(true);

            } else {
                if (!i->isPermanentHidded())
                    i->setVisible(true);
            }
        }
        root->endUpdate();

        show_header = _view->horizontalRootHeaderItem(_dataset)->bottomPermanentVisibleCount() > 1;
    }

    if (_data_mode == DataMode::ItemModel || _dataset.dataType() == DataType::Table)
        _table_view->horizontalHeader()->setHidden(!show_header);
    else if (_dataset.dataType() == DataType::Tree)
        _tree_view->header()->setHidden(!show_header);

    view()->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (_table_view != nullptr)
        _table_view->setAutoResizeRowsHeight(false);

    if (_selection_mode == SelectionMode::Multi) {
        // инициализация множественного выбора
        if (_table_view != nullptr)
            _cell_column_check_interface = _table_view;
        else if (_tree_view != nullptr)
            _cell_column_check_interface = _tree_view;
        else
            Z_HALT_INT;

        if (_multi_selection_levels.isEmpty()) {
            _cell_column_check_interface->setCellCheckColumn(_display_column_pos, true, true, -1);

        } else {
            for (int level : qAsConst(_multi_selection_levels)) {
                _cell_column_check_interface->setCellCheckColumn(_display_column_pos, true, true, level);
            }
        }
    }

    // создаем прокси модель для фильтрации

    QList<int> fp;
    if (!_filter_by_columns.isEmpty()) {
        Z_CHECK(filter_columns_pos.isEmpty());
        for (auto& p : qAsConst(_filter_by_columns)) {
            Z_CHECK(_data_filter->source()->structure()->contains(p));
            fp << p.pos();
            Z_CHECK(fp.last() >= 0);
            Z_CHECK(_data_filter->proxyDataset(_dataset)->columnCount() >= fp.last());
        }
    } else {
        Z_CHECK(!filter_columns_pos.isEmpty());
        fp = filter_columns_pos;
    }

    _proxy = new SelectionProxyItemModel((_table_view != nullptr ? _table_view->horizontalHeader() : _tree_view->horizontalHeader()), //-V595
        _view.get(), fp, _data_mode == DataMode::Structure ? nullptr : _source_model, _data_filter, _data_filter_external.get(), _dataset, _display_column_pos,
        _cell_column_check_interface, this);

    // инициализируем отображаемую таблицу данными для показа
    if (_table_view != nullptr) {
        _table_view->setModel(_proxy);

    } else if (_tree_view != nullptr) {
        _tree_view->setModel(_proxy);

        if (_data_mode == DataMode::Structure) {
            connect(_model.get(), &Model::sg_finishLoad, this,
                [&](const zf::Error&, const zf::LoadOptions&, const zf::DataPropertySet&) { _tree_view->expandAll(); });
        }
    } else {
        Z_HALT_INT;
    }

    view()->setFrameShape(QFrame::StyledPanel);

    connect(_ui->search_item, &QLineEdit::textEdited, this, &SelectFromModelDialog::sl_textEdited);
    connect(view()->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &SelectFromModelDialog::sl_currentRowChanged);
    connect(view(), &QAbstractItemView::doubleClicked, this, &SelectFromModelDialog::sl_doubleClicked);

    if (_selection_mode == SelectionMode::Multi) {
        // к интерфейсу через синтаксис Qt5 не подключиться
        connect(dynamic_cast<QObject*>(_cell_column_check_interface), SIGNAL(sg_checkedCellChanged(QModelIndex, bool)), this,
            SLOT(sl_cellCheckedChanged(QModelIndex, bool)));
    }

    // решено игнорировать иконки моделей - источников данных
    _icon = qApp->windowIcon();

    setWindowIcon(_icon);
    setWindowTitle(_name);
}

QAbstractItemView* SelectFromModelDialog::view() const
{
    if (_table_view != nullptr)
        return _table_view;
    else if (_tree_view != nullptr)
        return _tree_view;
    else
        Z_HALT_INT;

    return nullptr;
}

HeaderItem* SelectFromModelDialog::rootHeader() const
{
    if (_table_view != nullptr)
        return _table_view->horizontalRootHeaderItem();

    if (_tree_view != nullptr)
        return _tree_view->rootHeaderItem();

    Z_HALT_INT;
    return nullptr;
}

int SelectFromModelDialog::firstVisibleColumn() const
{
    return rootHeader()->firstVisibleSection();
}

void SelectFromModelDialog::testEnabled()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::SELECT_FROM_MODEL_DIALOG_ENABLED_KEY);
}

void SelectFromModelDialog::testEnabled_helper()
{
    if (_selection_mode == SelectionMode::Single) {
        auto index = view()->currentIndex();
        if (index.isValid()) {
            if (_only_bottom_item) {
                setOkButtonEnabled(index.model()->rowCount(index.model()->index(index.row(), 0, index.parent())) == 0);

            } else {
                setOkButtonEnabled(true);
            }

        } else {
            setOkButtonEnabled(_allow_empty_selection);
        }
        button(QDialogButtonBox::Reset)->setVisible(_allow_empty_selection);

    } else {
        if (!_allow_empty_selection)
            setOkButtonEnabled(!_cell_column_check_interface->checkedCells().isEmpty());

        button(QDialogButtonBox::Reset)->setVisible(true);
    }
}

void SelectFromModelDialog::acceptSelection()
{
    // если введен фильтр, но он еще не применен по таймауту, то применяем
    if (_search_delay_timer->isActive()) {
        sl_searchDelayTimer();
    }

    activateButton(QDialogButtonBox::Ok);
}

void SelectFromModelDialog::scrollTo(const QModelIndex& index)
{
    if (!_tree_view) {
        view()->scrollTo(index);
        return;
    }

    QModelIndex parent = index;
    while (parent.parent().isValid() && parent.row() == 0) {
        parent = parent.parent();
    }

    _tree_view->scrollTo(index);
    if (parent != index)
        _tree_view->scrollTo(parent);
}

QList<int> SelectFromModelDialog::buttonsList(SelectionMode::Enum selection_mode)
{
    auto res = QList<int> {
        QDialogButtonBox::Ok,
        QDialogButtonBox::Cancel,
        QDialogButtonBox::Reset,
    };

    if (selection_mode == SelectionMode::Multi) {
        res += QDialogButtonBox::Help;
    }

    return res;
}

QStringList SelectFromModelDialog::buttonsText(SelectionMode::Enum selection_mode)
{
    auto res = QStringList {
        ZF_TR(ZFT_SELECT),
        ZF_TR(ZFT_CANCEL),
        ZF_TR(ZFT_RESET),
    };

    if (selection_mode == SelectionMode::Multi) {
        res += ZF_TR(ZFT_SELECT_ALL);
    }

    return res;
}

QList<int> SelectFromModelDialog::buttonsRole(SelectionMode::Enum selection_mode)
{
    auto res = QList<int> {
        QDialogButtonBox::AcceptRole,
        QDialogButtonBox::RejectRole,
        QDialogButtonBox::ResetRole,
    };

    if (selection_mode == SelectionMode::Multi) {
        res += QDialogButtonBox::ResetRole;
    }

    return res;
}

} // namespace zf
