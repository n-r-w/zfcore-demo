#pragma once

#include "zf_dialog.h"
#include "zf_i_highlight_view.h"
#include "zf_i_hightlight_view_controller.h"
#include "zf_data_structure.h"
#include "zf_data_widgets.h"
#include "zf_object_extension.h"
#include "zf_highlight_processor.h"

namespace Ui
{
class HighlightDialog;
}

namespace zf
{
class HighlightView;
class HightlightViewController;
class HighlightProcessor;
class WidgetHighlighter;
class WidgetHighligterController;
class HighlightMapping;

/*! Базовый диалог с панелью ошибок. Интерфейс устанавливать в виджет body()
 * ВАЖНО: не создавать UI в конструкторе! Только в методе createUI
 * Заполнять начальные данные тоже в createUI */
class ZCORESHARED_EXPORT HighlightDialog : public Dialog,
                                           public I_HighlightView,
                                           public I_HightlightViewController,
                                           public I_HighlightProcessor,
                                           public I_HashedDatasetCutomize,
                                           public I_HighlightProcessorKeyValues,
                                           public I_DataChangeProcessor
{
    Q_OBJECT
public:
    //! Данные и виджеты создаются на основании переданного контейнера
    HighlightDialog(
        //! Контейнер
        const DataContainerPtr& container,
        //! Вид диалога (создает кнопки на основе типа)
        Type type,
        //! Параметры
        const HighlightDialogOptions& options = HighlightDialogOption::SimpleHighlight);
    //! Данные и виджеты создаются на основании переданного контейнера
    HighlightDialog(
        //! Контейнер
        const DataContainerPtr& container,
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list,
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = QStringList(),
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = QList<int>(),
        //! Параметры
        const HighlightDialogOptions& options = HighlightDialogOption::SimpleHighlight);

    //! Данные и виджеты создаются на основании переданной структуры
    HighlightDialog(
        //! Структура данных
        const DataStructurePtr& structure,
        //! Вид диалога (создает кнопки на основе типа)
        Type type,
        //! Параметры
        const HighlightDialogOptions& options = HighlightDialogOption::SimpleHighlight);
    //! Данные и виджеты создаются на основании переданной структуры
    HighlightDialog(
        //! Структура данных
        const DataStructurePtr& structure,
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list,
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = QStringList(),
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = QList<int>(),
        //! Параметры
        const HighlightDialogOptions& options = HighlightDialogOption::SimpleHighlight);

    //! Структура, данные и виджеты берутся из переданного интерфейса I_HightlightController
    HighlightDialog(
        //! Параметры
        const HighlightDialogOptions& options,
        //! Интерфейс сортировки ошибок
        I_HighlightView* highlight_view_interface,
        //! Интерфейс I_HightlightController
        I_HightlightViewController* highlight_controller_interface,
        //! Вид диалога (создает кнопки на основе типа)
        Type type);
    //! Структура, данные и виджеты берутся из переданного интерфейса I_HightlightController
    HighlightDialog(
        //! Параметры
        const HighlightDialogOptions& options,
        //! Интерфейс сортировки ошибок
        I_HighlightView* highlight_view_interface,
        //! Интерфейс I_HightlightController
        I_HightlightViewController* highlight_controller_interface,
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list,
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = QStringList(),
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = QList<int>());

    ~HighlightDialog();

    //! Открытие в не модальном режиме
    void open() override;
    //! Открите в модальном режиме
    int exec() override;

    //! Реализация обработки реакции на изменение данных и заполнения модели ошибок
    HighlightProcessor* highlightProcessor() const;
    //! Состояние ошибок
    const HighlightModel* highlight(bool execute_check = true) const;

    //! Виджеты
    DataWidgets* widgets() const;
    //! Данные
    DataContainer* data() const;
    //! Структура
    const DataStructure* structure() const;

    //! Место для размещения UI
    QWidget* body() const;

    //! Загрузить форму (ui файл) из ресурсов и установить ее в workspace.
    //! При необходимости загрузки ui по частям, надо использовать Utils::openUI
    Error setUI(const QString& resource_name);

    /*! Свойство по его идентификатору.
     * Доступны только поля (Field) и наборы данных (Dataset). Колонки, строки и ячейки не могут быть
     * получены через данный метод */
    DataProperty property(const PropertyID& property_id) const;

    //! Порядковый номер колонки для набора данных
    int column(const DataProperty& column_property) const;
    //! Порядковый номер колонки для набора данных по id свойств
    int column(const PropertyID& column_property_id) const;

    //! Задать порядок виджетов на форме
    void setTabOrder(const QList<QWidget*>& tab_order) final;

    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const DataProperty& property) const;
    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const PropertyID& property_id) const;
    //! Запросить проверку на ошибки для данной ячейки набора данных
    void registerHighlightCheck(int row, const DataProperty& column, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для данной ячейки набора данных
    void registerHighlightCheck(int row, const PropertyID& column, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для строки набора данных
    void registerHighlightCheck(const DataProperty& dataset, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для строки набора данных
    void registerHighlightCheck(const PropertyID& dataset, int row, const QModelIndex& parent = QModelIndex()) const;
    //! Запросить проверку на ошибки для всех свойств
    void registerHighlightCheck() const;

    //! Принудительно запустить зарегистрирированные проверки на ошибки
    void executeHighlightCheckRequests() const;
    //! Очистить зарегистрирированные проверки на ошибки
    void clearHighlightCheckRequests() const;

    //! Внешняя рамка панели ошибок
    QFrame* highlightPanelFrame() const;

    // *** реализация I_HighlightProcessor
protected:
    //! Проверить поле на ошибки
    void getFieldHighlight(const DataProperty& field, HighlightInfo& info) const override;
    //! Проверить набор данных на ошибки
    void getDatasetHighlight(const DataProperty& dataset, HighlightInfo& info) const override;
    //! Проверить ячейку набора данных на ошибки
    void getCellHighlight(const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent,
                          HighlightInfo& info) const override;
    //! Проверить свойство на ошибки и занести результаты проверки в HighlightInfo
    void getHighlight(const DataProperty& p, HighlightInfo& info) const override;

protected:
    //! Проверить на доступность кнопки диалога
    void checkButtonsEnabled() override;

    //! Установка фокуса на свойство
    void setFocus(const zf::DataProperty& property);

    //! Перейти на виджет с первой ошибкой
    void focusError();
    //! Перейти на следующий виджет с ошибкой
    void focusNextError(InformationTypes info_type);

    //! Задать нестандартную связь между виджетами и свойствами для обработки ошибок
    //! Нужно в случае когда ошибке соответствует произвольный виджет на форме, а не тот, которы был сгенерирован по свойству
    void setHighlightWidgetMapping(const DataProperty& property, QWidget* w);
    void setHighlightWidgetMapping(const PropertyID& property_id, QWidget* w);
    //! Задать соответствие между свойством в котором находится ошибка и свойством на котором ее нужно показать
    void setHighlightPropertyMapping(const DataProperty& source_property, const DataProperty& dest_property);
    void setHighlightPropertyMapping(const PropertyID& source_property_id, const PropertyID& dest_property_id);

    //! Виджет, который соответствует свойству
    QWidget* getHighlightWidget(const DataProperty& property) const;
    //! Свойство, которое соответсвует виджету
    DataProperty getHighlightProperty(QWidget* w) const;

    // *** реализация I_DataChangeProcessor
protected:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void onDataInvalidate(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;
    //! Изменилось состояние валидности данных
    void onDataInvalidateChanged(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;

    //! Сменился текущий язык данных контейнера
    void onLanguageChanged(QLocale::Language language) override;

    /*! Вызывается при любых изменениях свойства, которые могут потребовать обновления связанных данных:
     * Когда свойство инициализируется, изменяется, разблокируется
     * Этот метод нужен, если мы хотим отслеживать изменения какого-то свойства и реагировать на это в тот момент, когда
     * в нем есть данные и их можно считывать */
    void onPropertyUpdated(
        /*! Может быть:
         * PropertyType::Field - при инициализации, изменении или разблокировке поля
         * PropertyType::Dataset - при инициализации, разблокировке набора данных, 
         *                         перемещении строк, колонок, ячеек
         * PropertyType::RowFull - перед удалением или после добавлении строки
         * PropertyType::ColumnFull - перед удалении или после добавлении колонок
         * PropertyType::Cell - при изменении данных в ячейке */
        const DataProperty& p,
        //! Вид действия. Для полей только изменения, для таблиц изменения, удаление и добавление строк
        ObjectActionType action) override;

    //! Все свойства были заблокированы
    void onAllPropertiesBlocked() override;
    //! Все свойства были разблокированы
    void onAllPropertiesUnBlocked() override;

    //! Свойство были заблокировано
    void onPropertyBlocked(const DataProperty& p) override;
    //! Свойство были разаблокировано
    void onPropertyUnBlocked(const DataProperty& p) override;

    //! Изменилось значение свойства. Не вызывается если свойство заблокировано
    void onPropertyChanged(const DataProperty& p,
                           //! Старые значение (работает только для полей)
                           const LanguageMap& old_values) override;

    //! Изменились ячейки в наборе данных. Вызывается только если изменения относятся к колонкамЮ входящим в структуру данных
    void onDatasetCellChanged(
        //! Начальная колонка
        const DataProperty left_column,
        //! Конечная колонка
        const DataProperty& right_column,
        //! Начальная строка
        int top_row,
        //! Конечная строка
        int bottom_row,
        //! Родительский индекс
        const QModelIndex& parent) override;
    //! Изменились ячейки в наборе данных
    void onDatasetDataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                              const QVector<int>& roles) override;
    //! Изменились заголовки в наборе данных (лучше не использовать, т.к. представление не использует данные табличных заголовков Qt)
    void onDatasetHeaderDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last) override;
    //! Перед добавлением строк в набор данных
    void onDatasetRowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После добавления строк в набор данных
    void onDatasetRowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед удалением строк из набора данных
    void onDatasetRowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После удаления строк из набора данных
    void onDatasetRowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед добавлением колонок в набор данных
    void onDatasetColumnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После добавления колонок в набор данных
    void onDatasetColumnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Перед удалением колонок в набор данных
    void onDatasetColumnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! После удаления колонок из набора данных
    void onDatasetColumnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last) override;
    //! Модель данных будет сброшена (специфический случай - не использовать)
    void onDatasetModelAboutToBeReset(const DataProperty& p) override;
    //! Модель данных была сброшена (специфический случай - не использовать)
    void onDatasetModelReset(const DataProperty& p) override;
    //! Перед перемещением строк в наборе данных
    void onDatasetRowsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                     const QModelIndex& destinationParent, int destinationRow) override;
    //! После перемещения строк в наборе данных
    void onDatasetRowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                            int row) override;
    //! Перед перемещением колонок в наборе данных
    void onDatasetColumnsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                        const QModelIndex& destinationParent, int destinationColumn) override;
    //! После перемещения колонок в наборе данных
    void onDatasetColumnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                               int column) override;

    //! Вызывается перед фокусировкой свойства с ошибкой
    virtual void beforeFocusHighlight(const DataProperty& property);
    //! Вызывается после фокусировки свойства с ошибкой
    virtual void afterFocusHighlight(const DataProperty& property);

    //! Сменилась видимость нижней линии
    void onChangeBottomLineVisible() override;

protected:
    //! Запрос на отображение панели ошибок
    Q_SLOT void requestHighlightShow();

    //! После отображения диалога
    void afterPopup() override;
    //! Служебный обработчик менеджера обратных вызовов (не использовать)
    void onCallbackManagerInternal(int key, const QVariant& data) override;

    //! Вернуть текущую конфигурацию
    QMap<QString, QVariant> getConfiguration() const override;
    //! Применить конфигурацию
    Error applyConfiguration(const QMap<QString, QVariant>& config) override;
    //! Надо ли сохранять конфигурацию для сплиттера. По умолчанию все сплитеры сохраняют свое положение
    bool isSaveSplitterConfiguration(const QSplitter* splitter) const override;

public: // *** реализация I_HighlightView по умолчанию
    //! Определение порядка сортировки свойств при отображении HighlightView
    //! Вернуть истину если property1 < property2
    bool highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const final;
    //! Инициализирован ли порядок отображения ошибок
    bool isHighlightViewIsSortOrderInitialized() const final;
    //! А надо ли вообще проверять на ошибки
    bool isHighlightViewIsCheckRequired() const final;

    //! Сигнал об изменении порядка отображения
    Q_SIGNAL void sg_highlightViewSortOrderChanged() final;

public: // *** реализация I_HightlightController по умолчанию
        //! Виджеты
    DataWidgets* hightlightControllerGetWidgets() const final;
    //! Реализация обработки реакции на изменение данных и заполнения модели ошибок
    HighlightProcessor* hightlightControllerGetProcessor() const final;

    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* hightlightControllerGetMapping() const final;

    //! Текущий фокус
    DataProperty hightlightControllerGetCurrentFocus() const final;
    //! Установить фокус ввода. Возвращает false, если фокус не был установлен
    bool hightlightControllerSetFocus(const DataProperty& p) final;

    //! Вызывается перед фокусировкой свойства с ошибкой
    void hightlightControllerBeforeFocusHighlight(const DataProperty& property) final;
    //! Вызывается после фокусировки свойства с ошибкой
    void hightlightControllerAfterFocusHighlight(const DataProperty& property) final;

    // *** реализация I_HighlightProcessorKeyValues
public:
    //! Преобразовать набор значений ключевых полей в уникальную строку.
    //! Если возвращена пустая строка, то проверка ключевых полей для данной записи набора данных игнорируется
    QString keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent,
                                    const QVariantList& key_values) const override;
    //! Проверка ключевых значений на уникальность. Использовать для реализации нестандартных алгоритмов проверки
    //! ключевых полей По умолчанию используется алгоритм работы по ключевым полям, заданным через свойство
    //! PropertyOption::Key для колонок наборов данных
    void checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent,
                        //! Если ошибка есть, то текст не пустой
                        QString& error_text,
                        //! Ячейка где должна быть ошибка если она есть
                        DataProperty& error_property) const override;

    // *** реализация I_HashedDatasetCutomize
public:
    //! Преобразовать набор значений ключевых полей в уникальную строку
    QString hashedDatasetkeyValuesToUniqueString(
        //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
        const QString& key,
        //! Строка набора данных
        int row,
        //! Ролитель строки
        const QModelIndex& parent,
        //! Ключевые значения
        const QVariantList& keyValues) const override;

protected:
    //! Создание ui при необходимости. Не переопределять и не вызывыать
    void createUI_helper() const final;
    //! Вызывается когда добавлен новый объект
    void onObjectAdded(QObject* obj) override;
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    void onObjectRemoved(QObject* obj) override;

private slots:
    //! Информация об ошибках: Добавлен элемент
    void sl_highlight_itemInserted(const zf::HighlightItem& item);
    //! Информация об ошибках: Удален элемент
    void sl_highlight_itemRemoved(const zf::HighlightItem& item);
    //! Информация об ошибках: Изменен элемент
    void sl_highlight_itemChanged(const zf::HighlightItem& item);
    //! Информация об ошибках: Начато групповое изменение
    void sl_highlight_beginUpdate();
    //! Информация об ошибках: Закончено групповое изменение
    void sl_highlight_endUpdate();

private:
    void bootstrap();
    //! Создать область отображения ошибок
    void bootstrapHightlight();

    Ui::HighlightDialog* _ui;

    //! Структура данных
    DataStructurePtr _structure;
    //! Данные
    DataContainerPtr _data;
    //! Виджеты
    DataWidgetsPtr _widgets;
    //! Отслеживание измнений данных в DataContainer
    DataChangeProcessor* _data_processor = nullptr;
    //! Индикация ошибок
    WidgetHighlighter* _widget_highlighter = nullptr;
    //! Реация на изменение в списке ошибок и передача их в WidgetHighlighter
    ObjectExtensionPtr<WidgetHighligterController> _widget_highligter_controller;
    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* _highligt_mapping = nullptr;

    I_HighlightView* _highlight_view_interface = nullptr;
    HighlightModel* _highlight_model = nullptr;
    I_HightlightViewController* _highlight_controller_interface = nullptr;

    //! Панель ошибок
    HighlightView* _highlight_view = nullptr;
    //! Контроллер для связи между панелью ошибок и виджетами
    ObjectExtensionPtr<HightlightViewController> _highlight_controller;
    //! Панель ошибок
    QFrame* _highlight_panel = nullptr;

    //! Отслеживание ошибок
    ObjectExtensionPtr<HighlightProcessor> _highlight_processor;

    //! Параметры
    HighlightDialogOptions _options;
};

} // namespace zf
