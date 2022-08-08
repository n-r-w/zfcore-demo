#pragma once

#include "zf_dialog.h"
#include "zf_view.h"

namespace Ui {
class ZFSelectFromModelDialog;
}

namespace zf
{
class SelectionProxyItemModel;
class I_CellColumnCheck;

/*! Диалог выбора элемента из модели
 * Если в плагине определено представление, то оно будет использовано для отображения.
 * В противном случае будет создано временное QTableView или QTreeView */
class ZCORESHARED_EXPORT SelectFromModelDialog : public Dialog
{
    Q_OBJECT
public:
    //! Отображение zf::Model
    explicit SelectFromModelDialog(
        //! Режим диалога
        SelectionMode::Enum selection_mode,
        //! Модель
        const ModelPtr& model,
        //! Свойство - набор данных
        const DataProperty& dataset = {},
        //! Колонка, которая отвечает за позиционирование в наборе данных
        const DataProperty& value_column = {},
        //! Роль, которая отвечает за позиционирование в наборе данных
        int value_role = Qt::DisplayRole,
        //! Колонка с основными названиями строк
        const DataProperty& display_column = {},
        //! Уровни на которых можно выделять элементы (только для SelectionMode::Multi)
        //! Имеет смысл для выборки из иерархических наборов данных
        const QList<int>& multi_selection_levels = {},
        //! Фильтр
        const DataFilterPtr& data_filter = nullptr,
        //! Фильтрация по указанным колонкам. Если не задано, то по первому свойству типа ColumnFull без параметра Hidden
        const DataPropertyList filter_by_columns = {},
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list = {},
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = {},
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = {});

    //! Отображение QAbstractItemModel
    explicit SelectFromModelDialog(
        //! Режим диалога
        SelectionMode::Enum selection_mode,
        //! Уникальный код диалога. Используется для сохранения его размера между сессиями
        const QString& dialog_code,
        //! Заголовок
        const QString& caption,
        //! Иконка
        const QIcon& icon,
        //! Модель
        QAbstractItemModel* model,
        //! Колонка, которая отвечает за позиционирование в наборе данных
        int value_column = 0,
        //! Роль, которая отвечает за позиционирование в наборе данных
        int value_role = Qt::DisplayRole,
        //! Колонка, которая отвечает за отображение в наборе данных
        int display_column = 0,
        //! Роль, которая отвечает за отображение в наборе данных
        int display_role = Qt::DisplayRole,
        //! Уровни на которых можно выделять элементы (только для SelectionMode::Multi)
        //! Имеет смысл для выборки из иерархических наборов данных
        const QList<int>& multi_selection_levels = {},
        //! Фильтрация по указанным колонкам
        const QList<int>& filter_by_columns = {},
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list = {},
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = {},
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = {});

    ~SelectFromModelDialog();

    //! Режим диалога
    SelectionMode::Enum selectionMode() const;
    //! Для деревьев - выбор только узла нижнего уровня. Работает только для PopupMode::PopupDialog
    bool onlyBottomItem() const;
    //! Для деревьев - выбор только узла нижнего уровня. Работает только для PopupMode::PopupDialog
    void setOnlyBottomItem(bool b);

    //! Запустить диалог выбора
    virtual bool select(
        //! Текущий выбор (значения в колонке, предназначенной для позиционирования)
        const QVariantList& values = {});

    //! Выбранные элементы модели из колонки, предназначенной для позиционирования
    QVariantList selectedValues() const;
    //! Установить выбранные элементы по значению в колонке, предназначенной для позиционирования
    bool setSelected(const QVariantList& values);

    //! Выбранные элементы модели. Если диалог основан на выборке из zf::Model,
    //! то индекс соответствует свойству dataset для model (не прокси!)
    QModelIndexList selectedIndexes() const;
    //! Установить выбранные элементы по их индексу. Если диалог основан на выборке из zf::Model,
    //! то индекс соответствует свойству dataset для model (не прокси!)
    bool setSelected(const QModelIndexList& indexes);

    //! Выбранные идентификаты строк в модели (только для диалога, основанного на выборке из zf::Model)
    QList<RowID> selectedIds() const;
    //! Установить выбранные идентификаторы строк в модели (только для диалога, основанного на выборке из zf::Model)
    bool setSelected(const QList<RowID>& row_ids);

    //! Убрать выбранные элементы модели
    void clearSelected();

    //! Отображать фильтр
    bool isFilterVisible() const;
    void setFilterVisible(bool b);
    //! Установить фильтр
    void setFilterText(const QString& s);

    //! Разрешать ли выбор пустого значения
    bool isAllowEmptySelection() const;
    void setAllowEmptySelection(bool b);

    //! Виджет для заголовка (по умолчанию невидим)
    QWidget* headerWorkspace() const;

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

    //! Служебный обработчик менеджера обратных вызовов (не использовать)
    void onCallbackManagerInternal(int key, const QVariant& data) override;
    //! Была нажата кнопка
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;
    void beforePopup() override;
    //! После отображения диалога
    void afterPopup() override;
    //! Перед скрытием диалога
    void beforeHide() override;
    //! После скрытия диалога
    void afterHide() override;
    //! Изменение размера при запуске
    void adjustDialogSize() override;

private slots:
    //! Двойное нажатие на таблицу
    void sl_doubleClicked(const QModelIndex& index);
    //! Изменение текста поиска
    void sl_textEdited();
    //! Изменилась текущая строка в списке
    void sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Таймер для задержки реакции на набор текста в фильтре
    void sl_searchDelayTimer();
    //! Изменилось выделение ячеек при множественном выборе
    void sl_cellCheckedChanged(const QModelIndex& index, bool checked);

private:
    //! Режим диалога
    enum class DataMode
    {
        //! Отображение произвольной QAbstractItemModel
        ItemModel,
        //! Отображение zf::Model
        Structure
    };

    void init(
        //! Положение колонок фильтрации
        const QList<int>& filter_columns_pos);

    //! Отображаемая таблица/дерево
    QAbstractItemView* view() const;
    //! Заголовок отображаемой таблицы
    HeaderItem* rootHeader() const;

    //! Текущая первая видимая колонка
    int firstVisibleColumn() const;
    //! Проверка на доступность кнопок
    void testEnabled();
    void testEnabled_helper();
    //! Применить выбор
    void acceptSelection();

    //! Корректный скрол до индекса с учетом родителей
    void scrollTo(const QModelIndex& index);

    //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
    static QList<int> buttonsList(
        //! Режим диалога
        SelectionMode::Enum selection_mode);
    //! Список текстовых названий кнопок
    static QStringList buttonsText(
        //! Режим диалога
        SelectionMode::Enum selection_mode);
    //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
    static QList<int> buttonsRole(
        //! Режим диалога
        SelectionMode::Enum selection_mode);    

    Ui::ZFSelectFromModelDialog* _ui;

    //! Режим данных
    DataMode _data_mode;
    //! Режим диалога
    SelectionMode::Enum _selection_mode;
    //! Для деревьев - выбор только узла нижнего уровня
    bool _only_bottom_item = false;

    QIcon _icon;
    QString _name;

    ModelPtr _model;
    DataProperty _dataset;
    DataFilterPtr _data_filter_external;
    DataFilterPtr _data_filter_ptr;
    DataFilter* _data_filter = nullptr;

    //! Представление (если есть)
    ViewPtr _view = nullptr;
    //! TableView для отображения на экране
    TableView* _table_view = nullptr;
    //! TreeView для отображения на экране
    TreeView* _tree_view = nullptr;

    //! Прокси для фильтрации
    SelectionProxyItemModel* _proxy = nullptr;
    //! исходая модель для Mode::ItemModel
    QAbstractItemModel* _source_model = nullptr;

    //! Колонка, которая отвечает за позиционирование в наборе данных
    DataProperty _value_column;
    //! Колонка, которая отвечает за позиционирование в наборе данных
    int _value_column_pos = -1;
    //! Роль, которая отвечает за позиционирование в наборе данных
    int _value_role = Qt::DisplayRole;

    //! Колонка, которая отвечает за отображение в наборе данных
    DataProperty _display_column;
    //! Колонка, которая отвечает за отображение в наборе данных
    int _display_column_pos = 0;
    //! Роль, которая отвечает за отображение в наборе данных. Только для моделей без представления
    int _display_role = Qt::DisplayRole;

    //! Фильтрация по указанным колонкам. Если не задано, то по первому свойству типа ColumnFull без параметра Hidden
    DataPropertyList _filter_by_columns;

    //! Интерфейс для унификации доступа к управлению чекбоксами ячеек TableView/TreeView
    I_CellColumnCheck* _cell_column_check_interface = nullptr;
    //! Уровни на которых можно выделять элементы (только для SelectionMode::Multi)
    //! Имеет смысл для выборки из иерархических наборов данных
    QList<int> _multi_selection_levels;

    //! Разрешать ли выбор пустого значения
    bool _allow_empty_selection = true;

    //! Строка отложенного перефильтрования
    QString _need_refilter_text;
    //! Таймер для задержки реакции на набор текста в фильтре
    QTimer* _search_delay_timer = nullptr;

    //! Блокировать реакцию на изменение состояния чекбоксов
    int _block_check_reaction = 0;

    //! Анимация ожидания (виджет)
    QLabel* _wait_movie_label = nullptr;
    //! Анимация ожидания
    QMovie* _wait_movie = nullptr;
};

} // namespace zf
