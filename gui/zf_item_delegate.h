#pragma once

#include "zf.h"
#include "zf_feedback_timer.h"
#include "zf_data_structure.h"
#include "zf_highlight_processor.h"

#include <QPointer>
#include <QStyledItemDelegate>

class QTextOption;
class QTextLayout;
class QTextDocument;

namespace zf
{
class View;

/*! Умеет отображать всплывающую подсказку, если текст не помещается в ячейку */
class ZCORESHARED_EXPORT HintItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    HintItemDelegate(QObject* parent = nullptr);
    QString displayText(const QVariant& value, const QLocale& locale) const override;  
    bool helpEvent(QHelpEvent* event, QAbstractItemView* v, const QStyleOptionViewItem& option,
            const QModelIndex& index) override;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    //! Полный размер ячейки с учетом ее текста
    virtual QSize cellRealSize(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

//! Интерфейс для динамического определения типа данных ячейки при работе с делегатом
class I_ItemDelegateCellProperty
{
public:
    virtual ~I_ItemDelegateCellProperty() { }
    virtual DataProperty delegateGetCellDataProperty(QAbstractItemView* item_view, const QModelIndex& index) const = 0;
};

//! Интерфейс для отображения чекбокса в делегате
class I_ItemDelegateCheckInfo
{
public:
    virtual ~I_ItemDelegateCheckInfo() { }
    virtual void delegateGetCheckInfo(QAbstractItemView* item_view, const QModelIndex& index, bool& show, bool& checked) const = 0;
};

/*! Базовый делегат для ячеек таблиц и деревьев */
class ZCORESHARED_EXPORT ItemDelegate : public HintItemDelegate
{
    Q_OBJECT
public:
    //! Для использования отдельно от представления набора данных View
    ItemDelegate(
        //! QAbstractItemView, который обслуживает делегат
        QAbstractItemView* item_view,
        //!  QAbstractItemView от которого наследуется состояние фокуса и т.п.
        QAbstractItemView* main_item_view,
        //! Интерфейс для динамического определения типа данных ячейки
        I_ItemDelegateCellProperty* cell_property_interface,
        //! Интерфейс для отображения чекбокса в делегате
        I_ItemDelegateCheckInfo* check_info_interface,
        //! Структура данных
        const DataStructurePtr& structure = nullptr,
        //! Набор данных в структуре
        const DataProperty& dataset_property = {}, QObject* parent = nullptr);
    //! Для использования как представление набора данных View
    ItemDelegate(
        //! Представление
        const View* view,
        //! Набор данных View
        const DataProperty& dataset_property,
        //!  QAbstractItemView от которого наследуется состояние фокуса и т.п.
        QAbstractItemView* main_item_view,
        //! Интерфейс для отображения чекбокса в делегате
        I_ItemDelegateCheckInfo* check_info_interface, QObject* parent = nullptr);
    //! Для использования как представление набора данных DataWidgets
    ItemDelegate(
        //! Виджеты
        const DataWidgets* widgets,
        //! Набор данных в DataWidgets
        const DataProperty& dataset_property,
        //! Информация об ошибках
        const HighlightProcessor* highlight,
        //!  QAbstractItemView от которого наследуется состояние фокуса и т.п.
        QAbstractItemView* main_item_view,
        //! Интерфейс для отображения чекбокса в делегате
        I_ItemDelegateCheckInfo* check_info_interface, QObject* parent = nullptr);

    //! Использовать html форматирование
    void setUseHtml(bool b);
    bool isUseHtml() const;

    //! Для деревьев выделить узлы нижнего уровня
    void setFormatBottomItems(bool b);
    bool isFormatBottomItems() const;

    //! QAbstractItemView, который обслуживает делегат
    QAbstractItemView* itemView() const;
    //! QAbstractItemView от которого наследуется состояние фокуса и т.п.
    QAbstractItemView* mainItemView() const;

    //! Набор данных View
    DataProperty datasetProperty() const;
    //! Активный редактор
    QWidget* currentEditor() const;
    //! Представление
    View* view() const;
    //! Виджеты
    const DataWidgets* widgets() const;

    //! Место чекбокса для индекса
    QRect checkBoxRect(const QModelIndex& index,
                       //! Если истина, то возвращает всю область чекбокса включая до границ ячейки, а не только "квадратик"
                       bool expand) const;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void destroyEditor(QWidget* editor, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;    
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    bool helpEvent(QHelpEvent* event, QAbstractItemView* v, const QStyleOptionViewItem& option, const QModelIndex& index) override;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;    
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;    
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool eventFilter(QObject* object, QEvent* event) override;

    //! Только создание редактора. Переопределять этот метод, а не createEditor
    virtual QWidget* createEditorInternal(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index,
                                          DataProperty& property) const;
    virtual void setEditorDataInternal(QWidget* editor, const QModelIndex& index) const;
    virtual void setModelDataInternal(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
    void paintErrorBox(
        QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QColor& color, int line_width) const;
    //! Полный размер ячейки с учетом ее текстав
    QSize cellRealSize(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private slots:
    void sl_closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);
    void sl_currentChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Закрылся popup (для комбобокса или т.п.)
    void sl_popupClosed();
    //! Закрылся popup (для комбобокса или т.п.)
    void sl_popupClosed(bool applied);
    //! Изменилось состояние чекбокса
    void sl_checkboxChanged(int);

    //! Завершена загрузка лукап модели
    void sl_lookupModelLoaded(const zf::Error& error,
                              //! Параметры загрузки
                              const zf::LoadOptions& load_options,
                              //! Какие свойства обновлялись
                              const zf::DataPropertySet& properties);

private:
    //! Получить отображаемый на экране текст
    QString getDisplayText(const QModelIndex& index, QStyleOptionViewItem* option) const;
    //! Получить свойства ячейки из представления (обновление параметра option)
    void getCellProperties(const QModelIndex& index, QStyleOptionViewItem* option) const;
    //! Получение состояния ошибки для ячейки
    void getHighlightInfo(QStyleOptionViewItem* option, const QModelIndex& index, QRect& rect, QRect& opt_rect, QColor& color) const;

    //! Отложенная инициализация
    void lazyInit() const;
    //! Свойство ячейки по ее индексу
    DataProperty cellDataProperty(const QModelIndex& index) const;
    //! Размер иконки по умолчанию
    static QSize iconSize(const QIcon& icon);

    //! Закрылся popup (для комбобокса или т.п.)
    void popupClosedInternal(bool applied);

    //! Отрисовка содержимого ячейки (частично выдрано из QCommonStyle)
    void paintCellContent(QStyle* style, QPainter* painter, const QStyleOptionViewItem* option, const QModelIndex& index, const QWidget* widget) const;
    //! Отрисовка текста ячейки (частично выдрано из QCommonStyle)
    void viewItemDrawText(QStyle* style, QPainter* p, const QStyleOptionViewItem* option, const QRect& rect) const;

    //! Инициализация rich text
    void initTextDocument(const QStyleOptionViewItem* option, const QModelIndex& index, QTextDocument& doc) const;

    //! выдрано из QCommonStyle
    static QSizeF viewItemTextLayout(QTextLayout& textLayout, int lineWidth, int maxHeight = -1, int* lastVisibleLine = nullptr);

    //! Полный размер ячейки с учетом ее текстав
    QSize cellRealSizeHelper(const QStyleOptionViewItem& option, const QModelIndex& index, bool is_html) const;

    //! Индекс source
    QModelIndex sourceIndex(const QModelIndex& index) const;

    //! Представление
    View* _view = nullptr;
    //! Виджеты
    const DataWidgets* _widgets = nullptr;
    //! Информация об ошибках
    const HighlightProcessor* _highlight = nullptr;    
    //! QAbstractItemView, который обслуживает делегат
    QAbstractItemView* _item_view = nullptr;
    //!  QAbstractItemView от которого наследуется состояние фокуса и т.п.
    QAbstractItemView* _main_item_view = nullptr;
    //! Свойство набора данных
    DataProperty _dataset_property;
    //! Структура данных
    DataStructurePtr _structure;
    //! Интерфейс для динамического определения типа данных ячейки
    I_ItemDelegateCellProperty* _cell_property_interface = nullptr;
    //! Интерфейс для отображения чекбокса в делегате
    I_ItemDelegateCheckInfo* _check_info_interface = nullptr;

    FeedbackTimer* _close_editor_timer = nullptr;
    mutable QPointer<QWidget> _close_widget;
    mutable QPointer<QWidget> _current_editor;
    mutable bool _initialized = false;
    //! Борьба с багом, при котором Qt вызывает сигнал закрытия редактора при открытии его через enter
    bool _fix_enter_key = false;

    //! Использовать html форматирование
    bool _use_html = true;

    //! Для деревьев выделить узлы нижнего уровня
    bool _format_bottom_items = false;

    //! Информация о не загруженных lookup
    struct DataNotReadyInfo
    {
        ModelPtr model;
        //!  Подписка на сигнал об окончании перезагрузки lookup
        QMetaObject::Connection connection;
    };
    mutable QList<DataNotReadyInfo> _data_not_ready_info;
};

} // namespace zf

Q_DECLARE_METATYPE(QAbstractItemDelegate::EndEditHint)
