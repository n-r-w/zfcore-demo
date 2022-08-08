#pragma once

#include "zf_global.h"
#include "zf_header_view.h"
#include "zf_data_structure.h"
#include "zf_item_delegate.h"
#include "zf_i_cell_column_check.h"

#include <QTableView>

namespace zf
{
class View;
class DataProperty;

//! Таблица с иерархическим заголовком, базовая для основной и фиксированной таблицы
class ZCORESHARED_EXPORT TableViewBase : public QTableView, public I_ItemDelegateCheckInfo, public I_CellColumnCheck
{
    Q_OBJECT
public:
    explicit TableViewBase(QWidget* parent = nullptr);
    TableViewBase(const DataStructurePtr& structure,
        //! Набор данных в структуре
        const DataProperty& dataset_property, QWidget* parent);
    TableViewBase(
        //! Представление
        const View* view,
        //! Набор данных View
        const DataProperty& dataset_property, QWidget* parent = nullptr);
    TableViewBase(
        //! Виджеты
        const DataWidgets* widgets,
        //! Набор данных в DataWidgets
        const DataProperty& dataset_property,
        //! Информация об ошибках
        const HighlightProcessor* highlight, QWidget* parent = nullptr);
    ~TableViewBase() override;

    void setModel(QAbstractItemModel* model) override;

    //! Корневой узел иерархического заголовка
    virtual HeaderItem* rootHeaderItem(Qt::Orientation orientation) const = 0;

    //! Горизонтальный иерархический заголовок
    HeaderView* horizontalHeader() const;
    //! Вертикальный иерархический заголовок
    HeaderView* verticalHeader() const;
    //!  Иерархический заголовок
    HeaderView* header(Qt::Orientation orientation) const;

    //! Количество зафиксированных групп (узлов верхнего уровня)
    virtual int frozenGroupCount() const = 0;
    //! Установить количество зафиксированных групп (узлов верхнего уровня)
    virtual void setFrozenGroupCount(int count) = 0;
    //! Количество зафиксированных секций
    int frozenSectionCount(bool visible_only) const;

    //! Разрешать сортировку
    void setSortingEnabled(bool enable);

    //! Разрешать диалог конфигурации
    DatasetConfigOptions configMenuOptions() const;
    void setConfigMenuOptions(const DatasetConfigOptions& o);

    //! Автоматически растягивать высоту под количество строк
    virtual bool isAutoShrink() const = 0;
    //! Минимальное количество строк при автоподгоне высоты
    virtual int shrinkMinimumRowCount() const = 0;
    //! Максимальное количество строк при автоподгоне высоты
    virtual int shrinkMaximumRowCount() const = 0;
    //! Автоматически подгонять высоту строк под содержимое
    virtual bool isAutoResizeRowsHeight() const = 0;
    //! Показывать чекборкс как круглую иконку
    virtual bool isModernCheckBox() const = 0;

    //! Использовать html форматирование
    virtual void setUseHtml(bool b);
    bool isUseHtml() const;

    void updateGeometries() override;

public:
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

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    //! Запросить подгонку строк по высоте
    virtual void requestResizeRowsToContents();
    //! Смена ширины колонки
    virtual void onColumnResized(int column, int oldWidth, int newWidth);
    //! I_ItemDelegateCheckInfo
    void delegateGetCheckInfo(QAbstractItemView* item_view, const QModelIndex& index, bool& show, bool& checked) const override;

    //! Является ли колонка логической
    virtual bool isBooleanColumn(int logical_index) const = 0;
    //! Заблокировано ли редактирование в ячейке для колонки
    virtual bool isNoEditTriggersColumn(int logical_index) const = 0;

    using QTableView::sizeHintForRow;

signals:
    //! Вызывается до инициации подгонки высоты строк
    void sg_beforeResizeRowsToContent();
    //! Вызывается после инициации подгонки высоты строк
    void sg_afterResizeRowsToContent();

protected:
    //! Колонки горизонтального заголовка перемещаются через Drag&Drop
    virtual void onColumnsDragging(
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
    virtual void onColumnsDragFinished();

    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
    bool event(QEvent* event) override;
    bool viewportEvent(QEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;

    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;
    void rowsInserted(const QModelIndex& parent, int first, int last) override;

    //! Ширина бокового сдвига
    virtual int leftPanelWidth() const;
    //! Высота горизонтального заголовка
    virtual int horizontalHeaderHeight() const;

private slots:

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

    //! Запрошено меню конфигурации заголовка
    void sl_configMenuRequested(const QPoint& pos, const zf::DatasetConfigOptions& options);

    //! Вызывается перед началом перезагрузки данных из rootItem
    void sl_beforeLoadDataFromRootHeader();
    //! Вызывается после окончания перезагрузки данных из rootItem
    void sl_afterLoadDataFromRootHeader();

    //! Смена ширины колонки
    void sl_columnResized(int column, int oldWidth, int newWidth);

    void sl_rowsRemoved(const QModelIndex& parent, int first, int last);
    void sl_modelReset();

    void sl_resizeToContents();

private:
    void init();

    //! Надо ли
    static bool isNeedRowsAutoHeight(int row_count, int column_count);

    QModelIndex _saved_index;
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
    //! Таймер изменения ширины колонки и автовысоты строк
    QTimer* _resize_timer;

    // Отслеживание обновления ячеек при переходе с одной на другую
    QPersistentModelIndex _hover_index;

    bool _geometry_recursion_block = false;

    //! Надо ли было подгонять высоту таблиц при последнем анализе
    bool _last_need_row_auto_height = false;

    friend class FrozenTableView;
};

} // namespace zf
