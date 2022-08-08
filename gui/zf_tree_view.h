#pragma once

#include "zf_global.h"
#include "zf_itemview_header_item.h"
#include "zf_item_delegate.h"
#include "zf_i_cell_column_check.h"

#include <QTreeView>

class QTreeViewPrivate;

namespace zf
{
class HeaderView;
class View;
class DataProperty;
class TreeCheckBoxPanel;

//! Древовидная таблица с иерархическим заголовком
class ZCORESHARED_EXPORT TreeView : public QTreeView, public I_ItemDelegateCheckInfo, public I_CellColumnCheck
{
    Q_OBJECT
public:
    explicit TreeView(QWidget* parent = nullptr);
    TreeView(
        //! Структура данных
        const DataStructurePtr& structure,
        //! Набор данных в структуре
        const DataProperty& dataset_property, QWidget* parent = nullptr);
    TreeView(
        //! Представление
        const View* view,
        //! Набор данных View
        const DataProperty& dataset_property, QWidget* parent = nullptr);
    TreeView(
        //! Виджеты
        const DataWidgets* widgets,
        //! Набор данных в DataWidgets
        const DataProperty& dataset_property,
        //! Информация об ошибках
        const HighlightProcessor* highlight, QWidget* parent = nullptr);

    //! Корневой узел иерархического заголовка
    HeaderItem* rootHeaderItem() const;

    //! Узел иерархического заголовка по его идентификатору
    HeaderItem* headerItem(int id) const;

    //! Иерархический заголовок
    HeaderView* horizontalHeader() const;
    HeaderView* header() const;

    //! Разрешать сортировку
    void setSortingEnabled(bool enable);

    //! Параметры меню конфигурации
    DatasetConfigOptions configMenuOptions() const;
    void setConfigMenuOptions(const DatasetConfigOptions& o);

    void setModel(QAbstractItemModel* model) override;

    //! Находится в процессе перезагрузки данных из rootItem
    bool isReloading() const;

    //! Представление
    View* view() const;
    //! Набор данных View
    DataProperty datasetProperty() const;

    //! Виджеты
    DataWidgets* widgets() const;
    //! Информация об ошибках
    HighlightProcessor* highlight() const;

    //! Указатель на структуру данных
    DataStructurePtr structure() const;

    //! Использовать html форматирование
    void setUseHtml(bool b);
    bool isUseHtml() const;

    //! Выделить узлы нижнего уровня
    void setFormatBottomItems(bool b);
    bool isFormatBottomItems() const;

    //! Сохранить состояние заголовков
    Error serialize(QIODevice* device) const;
    Error serialize(QByteArray& ba) const;
    //! Восстановить состояние заголовков
    Error deserialize(QIODevice* device);
    Error deserialize(const QByteArray& ba);

    //! Отметить указанную колонку как boolean. При этом двойной клик или надатие на пробел будет переключать состояние true/false
    void setBooleanColumn(int logical_index, bool is_boolean);
    //! Является ли колонка логической
    bool isBooleanColumn(int logical_index) const;

    //! Задать колонку, в которой заблокировано редактирование в ячейке
    void setNoEitTriggersColumn(int logical_index, bool no_edit_triggers);
    //! Заблокировано ли редактирование в ячейке для колонки
    bool isNoEditTriggersColumn(int logical_index) const;

    //! Показывать чекборкс как круглую иконку
    void setModernCheckBox(bool b);
    //! Показывать чекборкс как круглую иконку
    bool isModernCheckBox() const;

    void setCurrentIndex(const QModelIndex& index);

public: // выделение строк (панель слева от таблицы)
    //! Показать панель с чекбоксами
    void showCheckPanel(bool show);
    //! Отображается ли панель с чекбоксами
    bool isShowCheckPanel() const;
    //! Есть ли строки, выделенные чекбоксами
    bool hasCheckedRows() const;
    //! Проверка строки на выделение чекбоксом.
    bool isRowChecked(
        //! Если TreeView подключена к наследнику QAbstractProxyModel, то это индекс source
        const QModelIndex& index) const;
    //! Задать выделение строки чекбоксом
    void checkRow(
        //! Если TreeView подключена к наследнику QAbstractProxyModel, то это индекс source
        const QModelIndex& index, bool checked);
    //! Выделенные индексы. Если TreeView подключена к наследнику QAbstractProxyModel, то это индекс source
    QSet<QModelIndex> checkedRows() const;
    //! Все строки выделены чекбоксами
    bool isAllRowsChecked() const;
    //! Выделить все строки чекбоксами
    void checkAllRows(bool checked);

public: // выделение ячеек (чекбоксы внутри ячеек)
    //! Колонки, в ячейках которых, находятся чекбоксы (ключ - логические индексы колонок, значение - можно менять)
    QMap<int, bool> cellCheckColumns(
        //! Уровень вложенности
        int level) const override;
    //! Колонки, в ячейках которых, находятся чекбоксы
    void setCellCheckColumn(int logical_index,
        //! Колонка видима
        bool visible,
        //! Пользователь может менять состояние чекбоксов
        bool enabled,
        //! Ячейки выделяются только для данного уровня вложенности. Если -1, то для всех
        int level = -1) override;
    //! Состояние чекбокса ячейки
    bool isCellChecked(const QModelIndex& index) const override;
    //! Задать состояние чекбокса ячейки. Если колонка не находится в cellCheckColumns, то она туда добавляется
    void setCellChecked(const QModelIndex& index, bool checked) override;
    //! Все выделенные чеками ячейки
    QModelIndexList checkedCells() const override;
    //! Очистить все выделение чекбоксами
    void clearCheckedCells() override;

public:
    void updateGeometries() override;

    //! I_ItemDelegateCheckInfo
    void delegateGetCheckInfo(QAbstractItemView* item_view, const QModelIndex& index, bool& show, bool& checked) const override;

    //! Выводим в паблик
    QStyleOptionViewItem viewOptions() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    bool viewportEvent(QEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

signals:
    //! Изменилось выделение чекбоксами
    void sg_checkedRowsChanged();
    //! Изменилась видимость панели выделения чекбоксами
    void sg_checkPanelVisibleChanged();
    //! Изменилось выделение ячеек чекбоксами
    void sg_checkedCellChanged(
        //! Если TableView подключена к наследнику QAbstractProxyModel, то это индекс source
        const QModelIndex& index, bool checked) override;

private slots:
    //! Выделить указанную колонку
    void selectColumn(int column);
    //! Запрошено меню конфигурации заголовка
    void sl_configMenuRequested(const QPoint& pos, const zf::DatasetConfigOptions& options);
    //! Вызывается перед началом перезагрузки данных из rootItem
    void sl_beforeLoadDataFromRootHeader();
    //! Вызывается после окончания перезагрузки данных из rootItem
    void sl_afterLoadDataFromRootHeader();
    //! Колонки горизонтального заголовка перемещаются через Drag&Drop
    void sl_columnsDragging(
        //! Визуальный индекс колонки начала перемещаемой группы
        int from_begin,
        //! Визуальный индекс колонки окончания перемещаемой группы
        int from_end,
        //! Визуальный индекс колонки куда произошло перемещение
        int to,
        //! Визуальный индекс колонки куда произошло перемещение с учетом скрытых
        int to_hidden,
        //! Если истина, то вставка слева от to, иначе справа
        bool left,
        //! Перемещение колонки разрешено
        bool allow);

    //! Перемещение колонок завершено
    void sl_columnsDragFinished();
    //! Смена ширины колонки
    void sl_columnResized(int column, int oldWidth, int newWidth);

    void sl_expanded(const QModelIndex& index);
    void sl_collapsed(const QModelIndex& index);

    void sl_layoutChanged();
    void sl_rowsRemoved(const QModelIndex& parent, int first, int last);
    void sl_rowsInserted(const QModelIndex& parent, int first, int last);
    void sl_rowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
    void sl_modelReset();

private:
    void init();
    void selectColumnHelper(QItemSelection& selection, int column, const QModelIndex& parent);
    //! Глубина вложенности индекса
    static int indexLevel(const QModelIndex& index);
    std::shared_ptr<QMap<int, bool>> cellCheckColumnsHelper(int level) const;
    QModelIndex sourceIndex(const QModelIndex& index) const;

    QPersistentModelIndex _saved_index;
    int _reloading = 0;

    //! Представление
    const View* _view = nullptr;
    //! Виджеты
    const DataWidgets* _widgets = nullptr;
    //! Информация об ошибках
    const HighlightProcessor* _highlight = nullptr;
    //! Набор данных View
    DataProperty _dataset_property;
    //! Указатель на структуру данных
    DataStructurePtr _structure;

    //! Таймер изменения ширины колонки
    QTimer* _column_resize_timer;

    //! Панель с чекбоксами
    TreeCheckBoxPanel* _check_panel;
    //! Выделенные строки. Если TreeView подключена к наследнику QAbstractProxyModel, то это индекс source
    QSet<QPersistentModelIndex> _checked;
    //! Все строки выделены
    int _all_checked = false;

    bool _geometry_recursion_block = false;
    // Отслеживание обновления ячеек при переходе с одной на другую
    QPersistentModelIndex _hover_index;

    //! Колонки, в ячейках которых, находятся чекбоксы (логические индексы колонок)
    //! Ключ - уровень вложенности (-1 для всех уровней)
    //! Значение мап: ключ - логические индексы колонок, значение - можно менять
    QMap<int, std::shared_ptr<QMap<int, bool>>> _cell_сheck_сolumns;
    //! Состояние чекбокса ячейки. Если TableView подключена к наследнику QAbstractProxyModel, то это номер индекс source
    mutable QList<QPersistentModelIndex> _cell_checked;

    //! Колонки, в которых заблокировано редактирование в ячейке
    QList<int> _no_edit_triggers_columns;
    //! Колонки типа boolean
    QList<int> _boolean_columns;

    //! Показывать чекборкс как круглую иконку
    bool _modern_check_box = true;

public:
    //! Указатель на приватные данные Qt для особых извращений
    QTreeViewPrivate* privatePtr() const;
};

} // namespace zf
