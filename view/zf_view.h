#pragma once

#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSplitter>
#include <QToolBar>

#include "zf_collapsible_group_box.h"
#include "zf_combo_box.h"
#include "zf_condition.h"
#include "zf_data_filter.h"
#include "zf_data_widgets.h"
#include "zf_date_edit.h"
#include "zf_date_time_edit.h"
#include "zf_double_spin_box.h"
#include "zf_entity_object.h"
#include "zf_header_view.h"
#include "zf_i_hightlight_view_controller.h"
#include "zf_i_modal_window.h"
#include "zf_i_module_window.h"
#include "zf_item_selector.h"
#include "zf_line_edit.h"
#include "zf_model.h"
#include "zf_object_extension.h"
#include "zf_operation.h"
#include "zf_operation_menu_manager.h"
#include "zf_plain_text_edit.h"
#include "zf_property_table.h"
#include "zf_request_selector.h"
#include "zf_spin_box.h"
#include "zf_table_view.h"
#include "zf_text_edit.h"
#include "zf_tree_view.h"
#include "zf_widget_scheme.h"

namespace zf {
class Dialog;
class View;
class DataWidgets;
class WidgetReplacer;
class WidgetReplacerDataStructure;
class WidgetHighlighter;
class ObjectHierarchy;
class ObjectsFinder;
class ChildObjectsContainer;
class WidgetHighligterController;

//! Интерфейс для перехвата выполнения операций View
class I_ViewOperationHandler {
public:
    virtual ~I_ViewOperationHandler() { }
    //! Если возвращает истину, то операция перехвачена и выполнятся в View не будет
    virtual bool executeOperationHandler(View* view, const Operation& op, Error& error) = 0;
};

//! Базовый класс для создания визуальной части модуля
class ZCORESHARED_EXPORT View : public EntityObject,
                                public I_ConditionFilter,
                                public I_DatasetVisibleInfo,
                                public I_DataWidgetsLookupModels,
                                public I_HightlightViewController {
    Q_OBJECT
    Q_PROPERTY(zf::ObjectHierarchy uiHierarchy READ uiHierarchy)
public:
    //! Имя свойства для mainWidget, указывающего на его View
    static const char* MAIN_WIDGET_VIEW_PRT_NAME;
    //! Имя mainWidget
    static const char* MAIN_WIDGET_OBJECT_NAME;

    //! Структура данных и сами данные наследуются из модели
    View(
        //! Модель
        const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Механизм замены виджетов. Если не задано, то используется стандартный WidgetReplacerDataStructure
        const std::shared_ptr<WidgetReplacer>& widget_replacer = nullptr,
        //! Фильтрация и сортировка (если не задано, то внутри представления создается экземпляр по умолчанию)
        const DataFilterPtr& filter = nullptr,
        //! Параметры ModuleDataOption
        const ModuleDataOptions& data_options = ModuleDataOption::HighlightEnabled | ModuleDataOption::SimpleHighlight,
        //! Если истина, то не использовать HightlightModel из модели, а сделать свою независимую от модели
        bool detached_highlight = false,
        //! Параметры представления
        const ViewOptions& view_options = ViewOptions());
    //! Собственная структура данных view
    View(
        //! Модель
        const ModelPtr& model,
        //! Состояние представления
        const ViewStates& states,
        //! Структура данных view
        const DataStructurePtr& data_structure,
        /*! Список свойств, которые будут перенаправлены в source.
         * Ключ - свойство этого объекта, Значение - свойство из source.
         * Если не задано, то данные view будут полностью независимы от модели */
        const QMap<DataProperty, DataProperty>& property_proxy_mapping = QMap<DataProperty, DataProperty>(),
        //! Механизм замены виджетов. Если не задано, то используется стандартный WidgetReplacerDataStructure
        const std::shared_ptr<WidgetReplacer>& widget_replacer = nullptr,
        //! Фильтрация и сортировка (если не задано, то внутри представления создается экземпляр по умолчанию)
        const DataFilterPtr& filter = DataFilterPtr(),
        //! Параметры ModuleDataOption
        const ModuleDataOptions& data_options = ModuleDataOption::HighlightEnabled | ModuleDataOption::SimpleHighlight,
        //! Параметры представления
        const ViewOptions& view_options = ViewOptions());
    ~View() override;

    //! Установить новую модель. Модель должна иметь такую же структуру данных
    void setModel(const ModelPtr& model,
        //! Механизм замены виджетов. Если не задано, то используется стандартный WidgetReplacerDataStructure
        const std::shared_ptr<WidgetReplacer>& widget_replacer = nullptr,
        //! Фильтрация и сортировка (если не задано, то внутри представления создается экземпляр по умолчанию)
        const DataFilterPtr& filter = nullptr);

    //! Модель
    ModelPtr model() const;

    //! Постоянный идентификатор сущности (если был присвоен)
    Uid persistentUid() const;
    //! Целочисленный идентификатор в рамках типа сущности. Можно вызывать только если имеется persistentUid и сущность не является уникальной
    int id() const;
    //! Является ли модель отсоединенной. Такие объекты предназначены для редактирования и не доступны для получения
    //! через менеджер моделей
    bool isDetached() const;

    //! Параметры представления
    ViewOptions options() const;

    //! Параметры окна, в которое встроено представление
    ModuleWindowOptions moduleWindowOptions() const;

    //! Виджет, который встраивается в окно (использовать для композиции нескольких модулей в один UI)
    QWidget* mainWidget() const;

    //! Виджет, на котором должен размещаться пользовательский интерфейс модуля
    QWidget* moduleUI() const;
    //! Иерархия виджетов для вывода в QDebug
    ObjectHierarchy uiHierarchy() const;

    //! Последний виджет, который имел фокус
    QWidget* lastFocused() const;

    //! Имя сущности
    QString entityName() const final;

    /*! Главный тулбар модуля (формируется набором операций данного модуля). Метод сделать виртуальным для особых случаев,
     * когда нам необходимо сделать данный модуль как "прокси" к другим модулям, из которых будут браться тулбары */
    virtual QToolBar* mainToolbar() const;
    /*! Дополнительные тулбары представления (добавляются отдельно, операции автоматически не обрабатываются)
     * Метод сделать виртуальным для особых случаев, когда нам необходимо сделать данный модуль как "прокси" к другим модулям,
     * из которых будут браться тулбары */
    virtual QList<QToolBar*> additionalToolbars() const;

    //! Список тулбаров на которых есть экшены
    QList<QToolBar*> toolbarsWithActions(bool visible_only) const;
    //! Добавить видимые тулбары в layout. Возвращает истину, если что-то было добавлено
    bool configureToolbarsLayout(QBoxLayout* layout);
    //! Есть ли экшены хотя бы в одном из тулбаров
    bool hasToolbars(bool visible_only) const;

    //! Скрыты ли все тулбары
    bool isToolbarsHidden() const;
    //! Скрыть все тулбары. Действует только в конструкторе или createUI
    void setToolbarsHidden(bool b);

    /*! Найти на форме по id свойства */
    QWidget* widget(const PropertyID& property_id) const;
    /*! Найти на форме по его свойству */
    QWidget* widget(const DataProperty& property_id) const;
    /*! Найти на форме виджет по имени */
    QWidget* widget(const QString& name) const;

    //! Свойство для виджета (если такого нет - DataProperty::isValid() == false)
    DataProperty widgetProperty(const QWidget* widget) const;

    //! Родительские представления. Порядок не определен
    QList<View*> parentViews() const;
    //! Головное родительское представление
    View* topParentView() const;

    //! Родительский диалог
    Dialog* parentDialog() const;

    //! Запросить обработку смены видимости виджетов. Например когда представление встроено в табвиджет и не может отслеживать
    //! фактическое изменение видимости свойств и поэтому не знает что надо загрузить их их БД
    void requestProcessVisibleChange() const;

    /*! Найти на форме объект (виджет) класса T по id свойства
     * Метод для унификации поиска объектов через object */
    template <class T>
    T* object(
        //! ID свойства
        const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        return widgets()->field<T>(property_id, halt_if_not_found, halt_if_not_cast);
    }
    /*! Найти на форме объект (виджет) класса T по id свойства
     * Метод для унификации поиска объектов через object */
    template <class T>
    T* object(
        //! Свойство
        const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        return widgets()->field<T>(property, halt_if_not_found, halt_if_not_cast);
    }
    /*! Найти на форме объект класса T по имени */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QString& name,
        //! Запустить автозамену виджетов
        bool execute_widgets_replacement = true) const
    {
        return object<T>(QStringList(name), execute_widgets_replacement);
    }
    /*! Найти на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QStringList& path,
        //! Запустить автозамену виджетов
        bool execute_widgets_replacement = true) const
    {
        if (execute_widgets_replacement)
            const_cast<View*>(this)->replaceWidgets();

        QList<QObject*> objects = findElementHelper(path, T::staticMetaObject.className());
        Z_CHECK_X(!objects.isEmpty(), QString("Object %1:%2:%3 not found").arg(entityCode().value()).arg(T::staticMetaObject.className()).arg(path.join("\\")));
        Z_CHECK_X(objects.count() == 1, QString("Object %1:%2:%3 found more then once (%4)").arg(entityCode().value()).arg(T::staticMetaObject.className()).arg(path.join("\\")).arg(objects.count()));
        T* res = qobject_cast<T*>(objects.first());
        Z_CHECK_X(res != nullptr, QString("Casting error %1:%2:%3").arg(entityCode().value()).arg(T::staticMetaObject.className()).arg(path.join("\\")));

        return res;
    }
    /*! Существует ли на форме объект (виджет) класса T по id свойства */
    template <class T>
    bool objectExists(
        //! ID свойства
        const PropertyID& property_id) const
    {
        return objectExists<T>(property(property_id));
    }
    /*! Существует ли на форме объект (виджет) класса T по id свойства */
    template <class T>
    bool objectExists(
        //! Свойство
        const DataProperty& property) const
    {
        QWidget* w = object<QWidget>(property, false);
        return w == nullptr ? false : qobject_cast<T*>(w) != nullptr;
    }
    /*! Существует ли на форме объект класса T по имени */
    template <class T>
    bool objectExists(const QString& name,
        //! Запустить автозамену виджетов
        bool execute_widgets_replacement = true) const
    {
        return objectExists<T>(QStringList(name), execute_widgets_replacement);
    }
    /*! Существует ли на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T>
    bool objectExists(const QStringList& path,
        //! Запустить автозамену виджетов
        bool execute_widgets_replacement = true) const
    {
        if (execute_widgets_replacement)
            const_cast<View*>(this)->replaceWidgets();

        return !findElementHelper(path, reinterpret_cast<T>(0)->staticMetaObject.className()).isEmpty();
    }

    //! Задать свойства, которые поддерживают частичную загрузку
    void setPartialLoadProperties(const PropertyIDList& properties);
    //! Задать свойства, которые поддерживают частичную загрузку
    void setPartialLoadProperties(const DataPropertyList& properties);
    //! Свойства, которые поддерживают частичную загрузку
    DataPropertySet partialLoadProperties() const;

    //! Поддерживают ли свойство частичную загрузку
    bool isPartialLoad(const DataProperty& p) const;
    //! Поддерживают ли свойство частичную загрузку
    bool isPartialLoad(const PropertyID& p) const;

    //! Находится в режиме "только для чтения"
    virtual bool isReadOnly() const;
    //! Установить режим "только для чтения"
    virtual void setReadOnly(bool b);

    //! Заблокировать поля ввода (не влияет на права доступа)
    void setBlockControls(bool b);
    //! Заблокированы ли поля ввода
    bool isBlockControls() const;

    //! Прямые права доступа к данной сущности
    AccessRights directAccessRights() const override;
    //! Косвенные права доступа к данному типу сущности
    AccessRights relationAccessRights() const override;

    //! Находится в MDI окне
    bool isMdiWindow() const;
    //! Находится в диалоговом окне редактирования
    bool isEditWindow() const;
    //! Находится в диалоговом окне просмотра
    bool isViewWindow() const;
    //! Встроено в другое представление
    bool isEmbeddedWindow() const;
    //! Находится в обычном диалоговом окне
    bool isDialogWindow() const;
    //! Истина, если представление вообще никуда не встроено (в окно просмотра, редактирования, в диалог и т.п.)
    bool isNotInWindow() const;
    //! Состояния представления
    ViewStates states() const;

    //! Какой набор данных находится в режиме редактирования ячейки
    DataProperty editingDataset() const;

    //! Полностью заблокировать интерфейс (допустимы вложенные вызовы)
    //! Поверх окна накладывается невидимый виджет, который блокирует любые нажатия на элементы интерфейса
    void blockUi(
        //! Показать блокировку сразу (иначе она будет показана при следующей обработке событий)
        bool force = false);
    //! Разаблокировать полностью заблокированный интерфейс
    void unBlockUi();
    //! Заблокирован ли интерфейс полностью
    bool isBlockedUi() const;

    //! Метод для корректного встраивания подчиненного представления. Автоматически создает модель и представление. Обновляет модель при необходимости
    //! Логичнее всего вызвать этот метод первый раз в createUI, а затем в onPropertyUpdated при изменении key_property
    zf::Error updateEmbeddedView(
        //! Код сущности встраиваемого объекта
        const zf::EntityCode& entity_code,
        //! Какое свойство этой модели хранит id встраиваемого объекта
        const zf::DataProperty& key_property,
        //! Произвольный параметр, который передается в Plugin::createView
        const QVariant& view_param,
        //! Имя объекта (виджета, лайаута), который должен быть извлечен из встраевоемого представления и помещен в target_layout_name
        //! Если не задан, то используется главный виджет представления moduleUI
        const QString& source_object_name,
        //! Имя лайаута в этом представлении, куда будет встроен source_object_name
        const QString& target_layout_name,
        //! Указатель на встроенную модель. Надо хранить эту переменную в этом представлении и передавать сюда
        zf::ModelPtr& model,
        //! Указатель на встроенное представление. Надо хранить эту переменную в этом представлении и передавать сюда
        zf::ViewPtr& view,
        //! Параметры
        const UpdateEmbeddedViewOptions& options = UpdateEmbeddedViewOptions());

    //! Экшн для операции
    QAction* operationAction(const OperationID& operation_id) const;
    QAction* operationAction(const Operation& operation) const;
    //! Есть ли экшн для данной операции (была добавлена в методе createOperationMenu)
    bool isOperationActionExists(const OperationID& operation_id) const;
    bool isOperationActionExists(const Operation& operation) const;

    //! Установить для операции setText, setIconText и т.д.
    void setOperationActionText(const OperationID& operation_id, const QString& text);
    void setOperationActionText(const Operation& operation, const QString& text);

    //! Экшн для головного меню операций
    QAction* nodeAction(const QString& accesible_id) const;
    //! Экшн для головного меню операций существует
    bool isNodeExists(const QString& accesible_id) const;

    //! Меню операций
    const OperationMenu* operationMenu() const;

    //! Операции режима редактирования (setModalWindowOperations). Будут добавлены на панель кнопок модального диалога
    OperationList modalWindowOperations() const;
    //! Кнопки, созданные на основе setModalWindowOperations
    QPushButton* modalWindowOperationButton(const OperationID& operation_id) const;

    //! Выполнить операцию
    Error executeOperation(const Operation& op, bool emit_signal = true);
    Error executeOperation(const OperationID& operation_id);
    //! Установить интерфейс перехвата выполнения операций. Используется при встраивании одного представления в другое
    void setOperationHandler(I_ViewOperationHandler* handler);

    //! Копировать свойства QAction из source в dest
    static void cloneAction(const QAction* source, QAction* dest);
    //! Копировать свойтсва  операции в DataWidgets::datasetAction
    void cloneAction(const OperationID& operation_id, const PropertyID& dataset_property_id, ObjectActionType type);

    //! Экшн для набора данных
    QAction* datasetAction(const DataProperty& property, ObjectActionType type) const;
    QAction* datasetAction(const PropertyID& property_id, ObjectActionType type) const;

    //! Кнопка для экшена набора данных
    QToolButton* datasetActionButton(const DataProperty& property, ObjectActionType type) const;
    QToolButton* datasetActionButton(const PropertyID& property_id, ObjectActionType type) const;

    //! Создать popup меню для экшена набора данных. После вызова этого метода при нажатии на кнопку экшена ее экшен активировать не будет,
    //! вместо этого появится меню с экшенами, переданными в этот метод
    void createDatasetActionMenu(const DataProperty& property, ObjectActionType type, const QList<QAction*>& actions);
    void createDatasetActionMenu(const PropertyID& property_id, ObjectActionType type, const QList<QAction*>& actions);

    //! Для lookup полей очищает содержимое, если такого значения нет в выпадающем списке
    bool sanitizeLookup(const DataProperty& property);
    bool sanitizeLookup(const PropertyID& property_id);

    //! Фильтрация
    DataFilter* filter() const;

    //! Фильтрация на основании условий для указанного набора данных
    ComplexCondition* conditionFilter(const DataProperty& dataset) const;
    /*! Имеется ли фильтрация на основании условий для указанного набора данных
     * По умолчанию имеется для всех наборов данных, для которых установлено свойство PropertyOption::Filtered */
    virtual bool hasConditionFilter(const DataProperty& dataset) const;
    //! Ключ для хранения ComplexCondition. Зависит от сущности и вида окна (MDI, Edit и т.д.)
    QString conditionFilterUniqueKey() const;

    //! Выполнить поиск
    virtual void searchText(const DataProperty& dataset, const QString& text);
    //! Поиск следующего
    virtual void searchTextNext(const DataProperty& dataset, const QString& text);
    //! Поиск предыдущего
    virtual void searchTextPrevious(const DataProperty& dataset, const QString& text);

    //! Находится в процессе автопоиска
    bool isAutoSearchActive() const;
    //! Текущий текст автопоиска
    QString autoSearchText() const;

    //! Разрешить сортировку набора данных пользователем. Не использовать совместно с DataFilter::setEasySort
    //! За исключением тех, у кого установлен PropertyOption::SortDisabled
    void setDatasetSortEnabled(const DataProperty& dataset_property, bool enabled = true);
    void setDatasetSortEnabled(const PropertyID& dataset_property_id, bool enabled = true);
    //! Разрешить сортировку всех наборов данных пользователем
    //! За исключением тех, у кого установлен DataFilter::setEasySort и PropertyOption::SortDisabled
    void setDatasetSortEnabledAll(bool enabled = true);

    //! Отсортировать набор данных. Не использовать совместно с DataFilter::setEasySort
    //! Можно не разрешать сортировку пользователю (setSortDatasetEnabled) но при этом сортировать программно
    void sortDataset(const DataProperty& column_property, Qt::SortOrder order);
    void sortDataset(const PropertyID& column_property_id, Qt::SortOrder order);

    //! Требуется ли проверка на ошибки (требуется только для моделей, которые не сущестувуют в БД(новые) и для
    //! отсоедененных (detached))
    bool isHighlightViewIsCheckRequired() const override;

    //! Ожидает загрузки. Отслеживается не только сам факт начала загрузки моделью, но и запрос на такую загрузку
    bool isLoading() const;
    //! В отличие от isLoading проверяет не только сам факт нахождения в процессе загрузки, но и завершение всех операций после загрузки
    //! Таких как вызов sg_finishLoad и т.п. По хорошему так себя должна вести isLoading, но для совместимости со старым кодом мы ее не трогаем
    bool isLoadingComplete() const;

    //! Последнее основание перезагрузки модели
    ChangeInfo reloadReason() const;
    //! Последнее основание перезагрузки модели на основании идентификатора
    UidList reloadReason(ObjectActionType type) const;

    //! Метод выгрузки в excel. Запускается в отдельном потоке. Показывает диалог ожидания
    //! Если была ошибка, то показывает ее
    bool exportToExcel(const DataProperty& dataset_property,
        //! Имя файла (без указания пути, просто имя)
        //! Если не задано, берет имя набора данных из структуры
        //! Если оно и там не задано, то просто "таблица"
        const QString& file_name = {}) const;
    //! Метод выгрузки в excel. Запускается в отдельном потоке. Показывает диалог ожидания
    //! Если была ошибка, то показывает ее
    bool exportToExcel(const PropertyID& dataset_property_id,
        //! Имя файла (без указания пути, просто имя)
        //! Если не задано, берет имя набора данных из структуры
        //! Если оно и там не задано, то просто "таблица"
        const QString& file_name = {}) const;

public:
    /*! Набор данных ProxyItemModel, созданный на базе оригинального ItemModel. Набор данных должен быть инициализирован
     * Используется для сортировки и фильтрации (см. filterAcceptsRow) */
    ProxyItemModel* proxyDataset(const DataProperty& dataset_property) const;
    ProxyItemModel* proxyDataset(const PropertyID& property_id) const;
    //! Индекс, приведенный из прокси набора данных к базовому
    QModelIndex sourceIndex(const QModelIndex& index) const;
    //! Индекс, приведенный из базовому набора данных к прокси
    QModelIndex proxyIndex(const QModelIndex& index) const;

    //! Получить индекс ячейки (source)
    QModelIndex cellIndex(const DataProperty& dataset_property, int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex cellIndex(const PropertyID& dataset_property_id, int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex cellIndex(int row, const DataProperty& column_property, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex cellIndex(int row, const PropertyID& column_property_id, const QModelIndex& parent = QModelIndex()) const;
    //! Получить индекс ячейки по ее свойству
    QModelIndex cellIndex(const DataProperty& cell_property) const;

    //! Колонка для индекса
    DataProperty indexColumn(const QModelIndex& index) const;

    //! Установить фокус ввода. Возвращает false, если фокус не был установлен
    virtual bool setFocus(const DataProperty& p);
    bool setFocus(const PropertyID& property_id);
    //! Текущий фокус
    DataProperty currentFocus() const;

    //! Текущая активная ячейка для активного набора данных
    QModelIndex currentActiveIndex() const;
    //! Текущая активная колонка для активного набора данных
    DataProperty currentActiveColumn() const;

    //! Текущий активная ячейка (source индекс)
    QModelIndex currentIndex(const DataProperty& dataset_property) const;
    //! Текущий активная ячейка (source индекс)
    QModelIndex currentIndex(const PropertyID& dataset_property_id) const;
    //! Текущий активная ячейка (source индекс). Для сущностей с одним набором данных
    QModelIndex currentIndex() const;

    //! Текущий активная строка (source индекс). Только для плоских таблиц
    int currentRow(const DataProperty& dataset_property) const;
    //! Текущий активная строка (source индекс). Только для плоских таблиц
    int currentRow(const PropertyID& dataset_property_id) const;
    //! Текущий активная строка (source индекс). Для сущностей с одним набором данных. Только для плоских таблиц
    int currentRow() const;

    //! Текущие выделенные строки набора данных  (source индекс)
    QModelIndexList selectedIndexes(const DataProperty& dataset_property) const;
    //! Текущие выделенные строки набора данных  (source индекс)
    QModelIndexList selectedIndexes(const PropertyID& dataset_property_id) const;
    //! Текущие выделенные строки набора данных  (source индекс). Для сущностей с одним набором данных.
    QModelIndexList selectedIndexes() const;

    //! Текущая активная колонка
    DataProperty currentColumn(const DataProperty& dataset_property) const;
    //! Текущая активная колонка
    DataProperty currentColumn(const PropertyID& dataset_property_id) const;
    //! Текущая активная колонка. Для сущностей с одним набором данных
    DataProperty currentColumn() const;

    //! Задать текущий активную ячейку (source или proxy индекс)
    bool setCurrentIndex(const DataProperty& dataset_property, const QModelIndex& index, bool current_column = false) const;
    //! Задать текущий активную ячейку (source или proxy индекс)
    bool setCurrentIndex(const PropertyID& dataset_property_id, const QModelIndex& index, bool current_column = false) const;
    //! Задать текущий активную ячейку (source или proxy индекс). Для сущностей с одним набором данных
    bool setCurrentIndex(const QModelIndex& index, bool current_column = false) const;

    //! Задать текущий активную ячейку
    bool setCurrentIndex(const RowID& row, const DataProperty& column_property);
    //! Задать текущий активную ячейку
    bool setCurrentIndex(const RowID& row, const PropertyID& column_property_id);

    //! Текущие выделенные строки (индексы с колонкой 0). Индексы приведены к source и отсортированы
    Rows selectedRows(const DataProperty& dataset_property) const;
    //! QItemSelectionModel для набора данных. Все индексы в QItemSelectionModel относятся к proxy
    QItemSelectionModel* selectionModel(const DataProperty& dataset_property) const;

    //! Первая видимая колонка набора данных
    int firstVisibleColumn(const DataProperty& dataset_property) const;
    //! Последняя видимая колонка набора данных
    int lastVisibleColumn(const DataProperty& dataset_property) const;

    //! Последний набор данных, который имел фокус ввода (или просто считался активным)
    DataProperty lastActiveDataset() const;

    //! Заблокировать реакцию на ошибки (показ ошибок) при действиях с моделью и т.п.
    void blockShowErrors();
    //! Разблокировать реакцию на ошибки (показ ошибок) при действиях с моделью и т.п.
    void unBlockShowErrors();

    //! Конфигурация окна для сохранения/восстановления
    struct WindowConfiguration {
        WindowConfiguration() { }

        //! Размер окна
        QSize size;
        //! Состояние окна
        Qt::WindowState state = Qt::WindowNoState;

        bool isValid() const;
    };

    //! Сохранить конфигурацию представления
    Error saveConfiguration(const WindowConfiguration& window_config) const;
    Error saveConfiguration() const;
    //! Восстановить конфигурацию представления
    Error loadConfiguration(
        //! Размер MDI окна. Если загружать конфигурацию не надо, то возвращается isValid() == false
        WindowConfiguration& window_config) const;
    Error loadConfiguration() const;

    //! Закодировать свойство в ключ, сохраняемый в концигурации
    QString configPropertyKeyEncode(const DataProperty& prop) const;
    //! Раскодировать свойство в ключ, сохраняемый в концигурации
    DataProperty configPropertyKeyDecode(const QString& key) const;

    //! Код, под которым сохраняется конфигурация представления
    static const QString VIEW_CONFIG_KEY;

    //! Виджеты, сгенерированные по данным
    DataWidgets* widgets() const;

    //! Встроенные представления
    const QList<View*>& embedViews() const;

    //! Режим "только для чтения" для свойства
    bool isPropertyReadOnly(const DataProperty& p) const;
    bool isPropertyReadOnly(const PropertyID& p) const;
    //! Задать режим "только для чтения" для свойства
    void setPropertyReadOnly(const DataProperty& p, bool read_only = true);
    void setPropertyReadOnly(const PropertyID& p, bool read_only = true);

    //! Игнорировать установку readOnly для виджета
    void setWidgetIgnoreReadOnly(QWidget* w);

    //! Вызывать если изменилась видимость представления в целом
    //! Если false, то представление перестанет реагировать на изменение данных в модели, до тех пор пока не будет выставлено true
    virtual void setVisibleStatus(bool b);

    /*! Свойства, которые пользователь видит на экране (не путать с QWidget::isVisible)
     *  Влияет на то, какие свойства будут запрашиваться для загрузки из БД */
    virtual DataPropertySet visibleProperties() const;

    //! Видим ли виджет
    bool isWidgetVisible(
        //! Имя виджета
        const QString& name,
        //! Учитывает нахождение в невидимых закладках QTabWidget и т.п.
        bool real_visible = false) const;
    //! Задать видимость для виджета
    void setWidgetVisible(
        //! Имя виджета
        const QString& name, bool visible = true);
    void setWidgetHidden(
        //! Имя виджета
        const QString& name, bool hidden = true);

    //! Видимость для свойства
    bool isPropertyVisible(const DataProperty& p,
        //! Учитывает нахождение в невидимых закладках QTabWidget и т.п.
        bool real_visible = false) const;
    bool isPropertyVisible(const PropertyID& p,
        //! Учитывает нахождение в невидимых закладках QTabWidget и т.п.
        bool real_visible = false) const;
    bool isPropertyHidden(const DataProperty& p,
        //! Учитывает нахождение в невидимых закладках QTabWidget и т.п.
        bool real_hidden = false) const;
    bool isPropertyHidden(const PropertyID& p,
        //! Учитывает нахождение в невидимых закладках QTabWidget и т.п.
        bool real_hidden = false) const;

    //! Задать видимость для свойства
    void setPropertyVisible(const DataProperty& p, bool visible = true);
    void setPropertyVisible(const PropertyID& p, bool visible = true);
    void setPropertyHidden(const DataProperty& p, bool hidden = true);
    void setPropertyHidden(const PropertyID& p, bool hidden = true);

    //! Установить свойство readOnly для всех виджетов в лайауте
    void setLayoutWidgetsReadOnly(QLayout* layout, bool read_only,
        //! Список исключений виджетов
        const QSet<QWidget*>& excluded_widgets = QSet<QWidget*>(),
        //! Список исключений вложенных лайаутов
        const QSet<QLayout*>& excluded_layouts = QSet<QLayout*>());

    //! Создать виджет поиска. При выборе нового элемента в этом виджете, модель перезагружается
    //! Если виджет поиска надо автоматически размещать на форме через префикс _SW_, то этот метод должен быть вызван в методе createUI
    ItemSelector* createSearchWidget(
        //! Из какой сущности делать список выбора
        const Uid& entity,
        //! Колонка в которой хранятся коды
        const PropertyID& id_column_property_id,
        //! Колонка для отображения
        const PropertyID& display_column_property_id,
        //! Текст при отсутствии значения (константа перевода)
        const QString& placeholder_translation_id = {},
        //! Роль для колонки кодов
        int id_colum_role = Qt::DisplayRole,
        //! Роль для отображения
        int display_colum_role = Qt::DisplayRole);
    //! Создать виджет поиска. При выборе нового элемента в этом виджете, модель перезагружается
    //! Если виджет поиска надо автоматически размещать на форме через префикс _SW_, то этот метод должен быть вызван в методе createUI
    ItemSelector* createSearchWidget(
        //! Из какой сущности делать список выбора
        const ModelPtr& model,
        //! Колонка в которой хранятся коды
        const PropertyID& id_column_property_id,
        //! Колонка для отображения
        const PropertyID& display_column_property_id,
        //! Текст при отсутствии значения (константа перевода)
        const QString& placeholder_translation_id = {},
        //! Роль для колонки кодов
        int id_colum_role = Qt::DisplayRole,
        //! Роль для отображения
        int display_colum_role = Qt::DisplayRole,
        //! Фильтр
        const DataFilterPtr& data_filter = nullptr);
    //! Виджет поиска. Если не был вызван метод createSearchWidget, то nullptr
    ItemSelector* searchWidget() const;

    //! Корневой узел иерархического заголовка
    HeaderItem* rootHeaderItem(const DataProperty& dataset_property, Qt::Orientation orientation) const;
    HeaderItem* rootHeaderItem(const PropertyID& dataset_property_id, Qt::Orientation orientation) const;

    //! Корневой узел горизонтального иерархического заголовка по его идентификатору
    HeaderItem* horizontalRootHeaderItem(const DataProperty& dataset_property) const;
    HeaderItem* horizontalRootHeaderItem(const PropertyID& dataset_property_id) const;
    //! Корневой узел вертикального иерархического заголовка по его идентификатору
    HeaderItem* verticalRootHeaderItem(const DataProperty& dataset_property) const;
    HeaderItem* verticalRootHeaderItem(const PropertyID& dataset_property_id) const;

    //! Узел горизонтального иерархического заголовка по его идентификатору
    HeaderItem* horizontalHeaderItem(const DataProperty& dataset_property,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id) const;
    HeaderItem* horizontalHeaderItem(const PropertyID& dataset_property_id,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id) const;
    //! Узел горизонтального иерархического заголовка по его идентификатору
    HeaderItem* horizontalHeaderItem(const PropertyID& section_id) const;
    HeaderItem* horizontalHeaderItem(const DataProperty& section_id) const;

    //! Узел вертикального иерархического заголовка по его идентификатору
    HeaderItem* verticalHeaderItem(const DataProperty& dataset_property, int section_id) const;
    HeaderItem* verticalHeaderItem(const PropertyID& dataset_property_id, int section_id) const;

    //! Заголовок для набора данных
    HeaderView* horizontalHeaderView(const DataProperty& dataset_property) const;
    HeaderView* horizontalHeaderView(const PropertyID& dataset_property_id) const;

    //! Задать размер секции в пикселях горизонтального заголовка
    void setSectionSizePx(const DataProperty& dataset_property,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id,
        //! Размер секции
        int size);
    //! Задать размер секции в пикселях горизонтального заголовка
    void setSectionSizePx(const PropertyID& dataset_property_id,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id,
        //! Размер секции
        int size);
    //! Задать размер секции в пикселях горизонтального заголовка
    void setSectionSizePx(
        //! Свойство - колонка
        const DataProperty& column_property,
        //! Размер секции
        int size);

    //! Задать размер секции в символах горизонтального заголовка
    void setSectionSizeChar(const DataProperty& dataset_property,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id,
        //! Размер секции
        int size);
    //! Задать размер секции в символах горизонтального заголовка
    void setSectionSizeChar(const PropertyID& dataset_property_id,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id,
        //! Размер секции
        int size);
    //! Задать размер секции в символах горизонтального заголовка
    void setSectionSizeChar(
        //! Свойство - колонка
        const DataProperty& column_property,
        //! Размер секции
        int size);

    //! Размер секции в пикселях горизонтального заголовка
    int sectionSizePx(const DataProperty& dataset_property,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id) const;
    //! Размер секции в пикселях горизонтального заголовка
    int sectionSizePx(const PropertyID& dataset_property_id,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id) const;
    //! Размер секции в пикселях горизонтального заголовка
    int sectionSizePx(
        //! Свойство - колонка
        const DataProperty& column_property) const;

    //! Инициализация горизонтального заголовка набора данных значениями по умолчанию на основании структуры данных
    void initDatasetHorizontalHeader(const DataProperty& dataset_property);
    //! Инициализация горизонтального заголовка всех наборов данных значениями по умолчанию на основании структуры данных
    void initDatasetHorizontalHeaders();

    //! Задать размер секции в значение по умолчанию
    void resetSectionSize(const PropertyID& dataset_property_id,
        //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
        //! совпадать с кодом колонки
        int section_id);
    //! Задать размер секции по умолчанию
    void resetSectionSize(const DataProperty& column_property);

    /*! Текущий активный набор даных. Определяется по текущему видимому набору данных, который последним имел фокус ввода.
     * Если набор данных один, то всегда он. Переопределять при необходимости нестандартной логики */
    virtual DataProperty currentDataset() const;

    //! Параметры по умолчанию для делегатов набора данных
    QStyleOptionViewItem getDatasetItemViewOptions(const DataProperty& dataset) const;

    //! Перевести набор данных в режим правки в ячейке для текущего индекса. Если текущий не выбран - false
    bool editCell(const DataProperty& dataset);
    bool editCell(const PropertyID& dataset_property_id);

    //! Показать панель с чекбоксами для набора данных
    void showDatasetCheckPanel(const DataProperty& dataset_property, bool show);
    void showDatasetCheckPanel(const PropertyID& dataset_property_id, bool show);
    //! Отоббражается ли панель с чекбоксами для набора данных
    bool isShowDatasetCheckPanel(const DataProperty& dataset_property) const;
    bool isShowDatasetCheckPanel(const PropertyID& dataset_property_id) const;
    //! Есть ли строки, выделенные чекбоксами для набора данных
    bool hasCheckedDatasetRows(const DataProperty& dataset_property) const;
    bool hasCheckedDatasetRows(const PropertyID& dataset_property_id) const;
    //! Проверка строки на выделение чекбоксом для набора данных
    bool isDatasetRowChecked(const DataProperty& dataset_property, int row) const;
    bool isDatasetRowChecked(const PropertyID& dataset_property_id, int row) const;
    //! Задать выделение строки чекбоксом для набора данных
    void checkDatasetRow(const DataProperty& dataset_property, int row, bool checked);
    void checkDatasetRow(const PropertyID& dataset_property_id, int row, bool checked);
    //! Выделенные строки для набора данных
    QSet<int> checkedDatasetRows(const DataProperty& dataset_property) const;
    QSet<int> checkedDatasetRows(const PropertyID& dataset_property_id) const;
    //! Количество выделенных строк
    int checkedDatasetRowsCount(const DataProperty& dataset_property) const;
    int checkedDatasetRowsCount(const PropertyID& dataset_property_id) const;
    //! Очистить все выделения чекбоксами
    void clearAllDatasetCheckRows(const DataProperty& dataset_property);
    void clearAllDatasetCheckRows(const PropertyID& dataset_property_id);
    //! Все строки выделены чекбоксами для набора данных
    bool isAllDatasetRowsChecked(const DataProperty& dataset_property) const;
    bool isAllDatasetRowsChecked(const PropertyID& dataset_property_id) const;
    //! Выделить все строки чекбоксами для набора данных
    void checkAllDatasetRows(const DataProperty& dataset_property, bool checked);
    void checkAllDatasetRows(const PropertyID& dataset_property_id, bool checked);

    //! Количество зафиксированных групп (узлов верхнего уровня)
    int datasetFrozenGroupCount(const DataProperty& dataset_property) const;
    int datasetFrozenGroupCount(const PropertyID& dataset_property_id) const;
    //! Установить количество зафиксированных групп (узлов верхнего уровня)
    void setDatasetFrozenGroupCount(const DataProperty& dataset_property, int count);
    void setDatasetFrozenGroupCount(const PropertyID& dataset_property_id, int count);

    //! Делает невалидными все используемые модели (лукапы и встроенные представления для view)
    void invalidateUsingModels() const override;

    using EntityObject::getColumnValues;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<RowID>& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QModelIndexList& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const DataProperty& column,
        //! Только указанные строки
        const QList<int>& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<RowID>& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QModelIndexList& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const QList<int>& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;
    //! Получить список значений из указанной колонки набора данных
    QVariantList getColumnValues(const PropertyID& column,
        //! Только указанные строки
        const Rows& rows,
        //! Роль
        int role,
        //! Использовать наложенный фильтр
        bool use_filter,
        //! Только содержащие значения
        bool valid_only) const;

protected: // виртуальные методы, которые может потребоваться переопределять
    /*! Создать меню операций представления. Меню должно содержать операции вида OperationScope::Entity или текстовые
       узлы. Операции должны входить в список Plugin::operations */
    virtual OperationMenu createOperationMenu() const;

    //! Изменилось состояние isReadOnly
    virtual void onReadOnlyChanged();

    //! Сменилось родительское представление
    void onParentViewChanged(zf::View* view);
    //! Сменился родительский диалог
    void onParentDialogChanged(zf::Dialog* dialog);

    //! Создано ли UI
    bool isUiCreated() const;
    /*! Создать пользовательский интерфейс путем формирования главного виджета moduleUI()
     * Если он не сформирован, то вызовется configureWidgetScheme */
    virtual void createUI();
    //! Вызывается при необходимости конфигурации схемы расположения виджетов (автоматическое формирование UI)
    //! Если moduleUI()->layout() был задан в конструкторе, то метод не вызывается и ядро считает, что UI будет сформирован вручную
    virtual void configureWidgetScheme(WidgetScheme* scheme) const;
    /*! Вызывается после того, как виджет был создан. Здесь можно настроить его параметры программно.
     * В принципе все виджеты создаются либо в createUI, либо в configureWidgetScheme. Но после этого активируется механизм замены виджетов
     * на форме, по совпадению имени с id свойства (имена вида _p_33) в котором параметры виджетов копируются из шаблона в целевой виджет.
     * Т.е. может быть ситуация, когда программист поменял параметры виджета в createUI, но эти параметры потом перезапишутся при
     * копировании из шаблона. */
    virtual void onWidgetCreated(QWidget* w,
        //! Свойство данных (может быть не задано)
        const DataProperty& p);

    //! Обработка нажатия клавиши в наборе данных. Если обработано, то true. По умолчанию ничего не делает
    virtual bool processDatasetKeyPress(const DataProperty& p, QAbstractItemView* view, QKeyEvent* e);

    //! Получить lookup модель для указанного свойства. Вызывается для свойств, у которых установлено lookup с помощью
    //! DataProperty::setLookupDynamic
    virtual void getDynamicLookupModel(
        //! Свойство данных, для которого необходимо определить lookup
        const DataProperty& property,
        //! Модель, откуда брать данные для расшифровки lookup
        ModelPtr& model,
        //! Фильтр
        DataFilterPtr& filter,
        //! Колонка, откуда брать данные для расшифровки lookup
        PropertyID& lookup_column_display,
        //! Колонка, откуда брать данные для подстановки ID в исходный набор данных
        PropertyID& lookup_column_id,
        //! Роль для получения расшифровки из lookup_column_display
        int& lookup_role_display,
        //! Роль для получения id из lookup_column_id
        int& lookup_role_id) const;

    //! Выполнение проверки всех экшенов и виджетов на доступность. Вызывается автоматически или после
    //! ручного вызова requestUpdateView
    virtual void onUpdateAccessRights();
    //! Выполнение неких действий после создания объекта. Вызывается автоматически после отработки конструктора
    virtual void afterCreated();
    //! Вызывается один раз после открытия представления и его инициализации (createUI) после того как будут загружены данные
    //! Использовать для начальной инициализации
    virtual void onDataReady(const QMap<QString, QVariant>& custom_data);
    //! Были ли загружены данные после открытия представления
    bool isDataReady() const;
    //! Вызывается один раз после открытия представления и его инициализации (createUI), после того как будут загружены данные и
    //! инициализированы все виджеты
    //! Использовать для начально инициализации
    virtual void onWidgetsReady();
    //! Запрос на подтверждение операции. Вызывается автоматически перед processOperation для
    //! OperationOption::Confirmation
    virtual bool confirmOperation(const Operation& op);
    //! Обработчик операции OperationScope::Entity. Если возвращается false и нет ошибки, то обработка передается в
    //! модель. Вызывается автоматически при активации экшена операции
    virtual bool processOperation(const Operation& op, Error& error);
    //! Инициализация стилей виджетов. Вызывается автоматически для всех виджетов
    virtual void initWidgetStyle(QWidget* w,
        //! Свойство данных (может быть не задано)
        const DataProperty& p);

    //! Обновить состояние виджета (по умолчанию проверяется состояние readOnly представления)
    //! Вызывается автоматически или после ручного вызова requestUpdateView
    //! По умолчанию меняет состояние виджета isWidgetReadOnly на основании метода isNeedWidgetReadonly
    virtual void updateWidgetStatus(
        //! Виджет
        QWidget* w,
        //! Свойство данных (может быть не задано)
        const DataProperty& p);
    //! Надо ли менять состояние readOnly для виджета
    //! Answer::Undefined - не менять
    //! Answer::Yes - установить readOnly в true
    //! Answer::No - установить readOnly в false
    virtual Answer isNeedWidgetReadonly(
        //! Виджет
        QWidget* w,
        //! Свойство данных (может быть не задано)
        const DataProperty& p) const;

    //! Находится ли виджет в состоянии "только для чтения". Метод обобщает работу со свойствами readOnly и enabled
    static bool isWidgetReadOnly(const QWidget* w);
    //! Задать для виджета состояние "только для чтения". Метод обобщает работу со свойствами readOnly и enabled
    static bool setWidgetReadOnly(QWidget* w, bool read_only);
    bool setWidgetReadOnly(const QString& widget_name, bool read_only);

    //! Изменилось значение свойства. Вызывается автоматически после изменения данных
    void onPropertyChanged(const DataProperty& p,
        //! Старые значение (работает только для полей)
        const LanguageMap& old_values) override;

    //! Все свойства были разблокированы
    void onAllPropertiesUnBlocked() override;
    //! Свойство были разаблокировано
    void onPropertyUnBlocked(const DataProperty& p) override;

    //! Сменилась информация о подключении к БД
    void onConnectionChanged(const ConnectionInformation& info) override;

    /*! Преобразовать отображаемое в ячейке значение. По умолчанию конвертирует коды в lookup расшифровку. Переопределять при необходимости
     * нестандартной обработки. Для колонок, которые имеют lookup, переопределять стандартное поведение надо осторожно, принимая во внимание
     * работу системы универсальной фильтрации (ComplexCondition и т.п.) */
    virtual Error getDatasetCellVisibleValue(
        //! Строка
        int row,
        //! Колонка
        const DataProperty& column,
        //! Индекс родителя, приведенный к source
        const QModelIndex& parent,
        //! Значение до изменения
        const QVariant& original_value,
        //! Параметры
        const VisibleValueOptions& options,
        //! Значение после изменения
        QString& value,
        //! Если инициализировано, то требуется дождаться загрузки указанной модели
        QList<ModelPtr>& data_not_ready) const;
    //! Преобразовать отображаемое в ячейке значение. Базовый метод, который преобразует lookup коды в расшифровку
    static Error getDatasetCellVisibleValue(QAbstractItemView* item_view, const DataProperty& column,
        //! Индекс, приведенный к source
        const QModelIndex& source_index,
        //! Значение до изменения
        const QVariant& original_value,
        //! Параметры
        const VisibleValueOptions& options,
        //! Значение после изменения
        QString& value,
        //! Если инициализировано, то требуется дождаться загрузки указанной модели
        QList<ModelPtr>& data_not_ready);

    //! Переопределение цвета ячейки таблицы и т.п. свойств
    virtual void getDatasetCellProperties(int row, const DataProperty& column, const QModelIndex& parent, QStyle::State state, QFont& font, QBrush& background,
        QPalette& palette, QIcon& icon, Qt::Alignment& alignment) const;
    //! Можно ли редактировать ячейку таблицы
    virtual bool isDatasetCellReadOnly(int row, const DataProperty& column, const QModelIndex& parent) const;

    /*! Нажата кнопка бокового тулбара набора данных или активирован соответсвующий ей action
     * По умолчанию добавляет/удаляет строки или активирует правку в ячейке
     * Возвращать true если вызов был обработан */
    virtual bool onDatasetActionActivated(const DataProperty& dataset_property, ObjectActionType action_type,
        //! Текущая ячейка набора данных (source)
        const QModelIndex& index,
        //! Колонка, соответствующая индексу. Может быть не задана (если колонки добавлены динамически)
        const DataProperty& column_property);
    //! Запрос на удаление строки. По умолчанию ничего не спрашивает
    virtual bool doAskRemoveDatasetRows(const DataProperty& dataset_property, const Rows& rows);
    //! Проверка на видимость и доступность кнопки бокового тулбара набора данных
    virtual void checkDatasetActionProperties(const DataProperty& dataset_property, ObjectActionType action_type, bool& enabled, bool& visible) const;

    //! Поведение по умолчанию для экшена, связанного с набором данных - видимость
    bool isStandardActionVisibleDefault(const DataProperty& dataset_property, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false) const;
    bool isStandardActionVisibleDefault(const PropertyID& dataset_property_id, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false) const;
    //! Поведение по умолчанию для экшена, связанного с набором данных - доступность
    bool isStandardActionEnabledDefault(const DataProperty& dataset_property, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false) const;
    bool isStandardActionEnabledDefault(const PropertyID& dataset_property_id, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false) const;

    //! Выставляет доступность и видимость операции, связанной с набором данных на основании стандартного экшена
    void updateAction(const Operation& operation, const DataProperty& dataset_property, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false);
    void updateAction(const Operation& operation, const PropertyID& dataset_property_id, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false);
    void updateAction(const OperationID& operation_id, const DataProperty& dataset_property, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false);
    void updateAction(const OperationID& operation_id, const PropertyID& dataset_property_id, ObjectActionType action_type,
        //! Игнорировать состояние readOnly представления
        bool ignore_read_only = false);

    /*! Выставляет доступность и видимость операции, не связанной с набором данных на основании стандартного экшена
     * В отличие от аналогичных методов, связанных с наборами данных, проверяет права доступа к модулю на уровне AccessRight::Create,
     * AccessRight::Remove и т.п. Если прав нет, то экшн скрывается */
    void updateAction(const Operation& operation, ObjectActionType action_type);
    void updateAction(const OperationID& operation_id, ObjectActionType action_type);
    /*! Выставляет доступность и видимость операции, не связанной с набором данных на основании стандартного экшена
     * В отличие от аналогичных методов, связанных с наборами данных, проверяет права доступа к модулю на уровне AccessRight::Create,
     * AccessRight::Remove и т.п. Если прав нет, то экшн скрывается. Метод может только усилить ограничения уже примененные к QAction, т.е.
     * сделать видимый невидимым, но не наоборот. Использовать, если перед окончательной установкой прав доступа к операциям надо сначала
     * сделать для них собственные проверки */
    void updateAction(QAction* operation_action, ObjectActionType action_type);

    //! Скрыть/показать тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    void setDatasetToolbarHidden(const DataProperty& property, bool hidden = true);
    //! Скрыть/показать тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    void setDatasetToolbarHidden(const PropertyID& property_id, bool hidden = true);

    //! Одиночное нажатие на набор данных
    virtual void onDatasetClicked(const DataProperty& dataset_property,
        //! Индекс - source
        const QModelIndex& index,
        //! Колонка, соответствующая индексу. Может быть не задана (если колонки добавлены динамически)
        const DataProperty& column_property);
    /*! Двойное нажатие на набор данных. Если возвращает false, то ядро активирует правку в ячейке (если она доступна). Если возвращает
     * true, то дальнейшая обработка прекращается */
    virtual bool onDatasetDoubleClicked(const DataProperty& dataset_property,
        //! Индекс - source
        const QModelIndex& index,
        //! Колонка, соответствующая индексу. Может быть не задана (если колонки добавлены динамически)
        const DataProperty& column_property);
    //! itemView набора данных активирован
    virtual void onDatasetActivated(const DataProperty& p, const QModelIndex& index);

    //! Изменилось выделение набора данных
    virtual void onDatasetSelectionChanged(const DataProperty& p, const QItemSelection& selected, const QItemSelection& deselected);
    //! Сменилась текущая ячейка набора данных
    virtual void onDatasetCurrentCellChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous);
    //! Сменилась текущая строка набора данных
    virtual void onDatasetCurrentRowChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous);
    //! Сменилась текущая колонка набора данных
    virtual void onDatasetCurrentColumnChanged(const DataProperty& p, const QModelIndex& current, const QModelIndex& previous);

    //! Сменилась текущая ячейка с задержкой
    virtual void onDatasetCurrentCellChangedTimeout(const DataProperty& p, const QModelIndex& current);
    //! Сменилась текущая строка набора данных
    virtual void onDatasetCurrentRowChangedTimeout(const DataProperty& p, const QModelIndex& current);
    //! Сменилась текущая колонка набора данных
    virtual void onDatasetCurrentColumnChangedTimeout(const DataProperty& p, const QModelIndex& current);

    //! Первая попытка загрузки модели
    virtual void onModelFirstLoadRequest();

    /*! Активировано редактирование ячейки (действие пользователя)
     * QKeyEvent не NULL если действие вызвано нажатием кнопки в таблицк. Текст нажатой кнопки - в text */
    virtual bool onEditCell(const DataProperty& dataset_property, const QModelIndex& cell_index, QKeyEvent* e, const QString& text);

    //! Сформировать контекстное меню для набора данных
    virtual void getDatasetContextMenu(const DataProperty& p, QMenu& m);

    //! Изменилось выделение чекбоксами для набора данных
    virtual void onCheckedRowsChanged(const DataProperty& p);
    //! Изменилась видимость панели выделения чекбоксами для набора данных
    virtual void onCheckPanelVisibleChanged(const DataProperty& p);

    //! Вызывается при необходимости загрузить конфигурацию (пользовательские настройки окна)
    virtual Error onLoadConfiguration();
    //! Вызывается после загрузки конфигурацию (пользовательские настройки окна).
    //! Когда вызван этот метод, окно уже восстановило положения сплиттеров, размер и т.п.
    virtual void afterLoadConfiguration();

    //! Вызывается при необходимости сохранить конфигурацию (пользовательские настройки окна). Для сохранения
    //! использовать метод classConfiguration
    virtual Error onSaveConfiguration() const;
    //! Надо ли сохранять конфигурацию для сплиттера. По умолчанию все сплитеры сохраняют свое положение
    virtual bool isSaveSplitterConfiguration(const QSplitter* splitter) const;

    //! Перед началом загрузки модели. ВАЖНО: это только запрос на загрузку, она может и не начаться если все данные уже есть или загрузка их уже в очереди
    virtual void onModelBeforeLoad(
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const zf::DataPropertySet& properties);
    //! Начата загрузка
    virtual void onModelStartLoad();
    //! Начато сохранение
    virtual void onModelStartSave();
    //! Начато удаление
    virtual void onModelStartRemove();

    //! Завершена загрузка
    virtual void onModelFinishLoad(const Error& error,
        //! Параметры загрузки
        const LoadOptions& load_options,
        //! Какие свойства обновлялись
        const DataPropertySet& properties);
    //! Завершено сохранение
    virtual void onModelFinishSave(const Error& error,
        //! Какие свойства сохранялись
        const zf::DataPropertySet& requested_properties,
        //! Какие свойства сохранялись фактически. Перечень не может быть шире чем requested_properties, т.к. сервер может сохранять свойства группами
        const zf::DataPropertySet& saved_properties,
        /*! Постоянный идентификатор, полученный после сохранения модели с isTemporary() == true. Если модель уже
         * имеет постоянный идентификатор, то возвращается пустой */
        const Uid& persistent_uid);
    //! Завершено удаление
    virtual void onModelFinishRemove(const Error& error);

    //! Данные были отфильтрованы
    virtual void onRefiltered(
        //! Набор данных
        const zf::DataProperty& dataset_property);

    //! Начато выполнение задачи. Вызывается только при первом входе в состояние прогресса
    void onProgressBegin(const QString& text, int start_persent) override;
    //! Окончено выполнение задачи. Вызывается только при полном выходе из состояния прогресса
    void onProgressEnd() override;

    //! Перед установкой новой модели (вызывается только при установке модели вне конструктора)
    virtual void beforeModelChange(const ModelPtr& old_model, const ModelPtr& new_model);
    //! После установки новой модели (вызывается только при установке модели вне конструктора)
    virtual void afterModelChange(const ModelPtr& model);

    //! Сохранить состояние виджетов на время перезагрузки модели
    virtual void saveReloadState();
    //! Восстановить состояние виджетов после перезагрузки модели
    virtual void restoreReloadState();

    //! Сменились состояния представления
    virtual void onStatesChanged(const ViewStates& new_states, const ViewStates& old_states);

    //! Создать свой виджет для редактирования в ячейке
    virtual QWidget* createDatasetEditor(
        //! Виджет, который надо установить в качестве родителя для создаваемого
        QWidget* parent_widget,
        //! Строка набора данных
        int row,
        //! Колонка (свойство) набора данных. Может быть не инициализировано, если колонка бьыла добавлена динамически
        const DataProperty& column_property,
        //! Колонка (номер) набора данных
        int column_pos,
        //! Родительский индекс (для иерархических наборов данных)
        const QModelIndex& parent_index);
    //! Подготовить виджет для редактирования в ячейке
    //! Если возвращает истину, то ядро не будет делать это самостоятельно
    virtual bool prepareDatasetEditor(
        //! Виджет для обработки
        QWidget* editor,
        //! Строка набора данных
        int row,
        //! Колонка (свойство) набора данных. Может быть не инициализировано, если колонка бьыла добавлена динамически
        const DataProperty& column_property,
        //! Колонка (номер) набора данных
        int column_pos,
        //! Родительский индекс (для иерархических наборов данных)
        const QModelIndex& parent_index);
    //! Установить данные в виджет для редактирования в ячейке
    //! Если возвращает истину, то ядро не будет делать это самостоятельно
    virtual bool setDatasetEditorData(
        //! Виджет для обработки
        QWidget* editor,
        //! Строка набора данных
        int row,
        //! Колонка (свойство) набора данных. Может быть не инициализировано, если колонка бьыла добавлена динамически
        const DataProperty& column_property,
        //! Колонка (номер) набора данных
        int column_pos,
        //! Родительский индекс (для иерархических наборов данных)
        const QModelIndex& parent_index);
    //! Извлечь данные из виджета редактирования в ячейке и записать их в данные
    //! Если возвращает истину, то ядро не будет делать это самостоятельно
    virtual bool setDatasetEditorModelData(
        //! Виджет для обработки
        QWidget* editor,
        //! Строка набора данных
        int row,
        //! Колонка (свойство) набора данных. Может быть не инициализировано, если колонка бьыла добавлена динамически
        const DataProperty& column_property,
        //! Колонка (номер) набора данных
        int column_pos,
        //! Родительский индекс (для иерархических наборов данных)
        const QModelIndex& parent_index);

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

    //! Вызывается перед фокусировкой свойства с ошибкой
    virtual void beforeFocusHighlight(const DataProperty& property);
    //! Вызывается после фокусировки свойства с ошибкой
    virtual void afterFocusHighlight(const DataProperty& property);

    /* Вызывается после любых изменений в highlight и при создании представления
     * В методе можно обратится к HighlightModel, проверить состояние ошибок и реализовать свою логику
     * Для обращения к модели ошибок лучше использовать переданный указатель, т.к. метод ModuleDataObject::highlight
     * по умолчанию запускает проверку на наличие новых ошибок, что тут не нужно */
    virtual void onHighlightChanged(const HighlightModel* hm);

    //! Установить у виджета рамку
    static void setHighlightWidgetFrame(QWidget* w, InformationType type = InformationType::Error);
    //! Убрать у виджета рамку
    static void removeHighlightWidgetFrame(QWidget* w);

    //! Подключено встроенное представление
    virtual void embedViewAttached(View* view);
    //! Отключено встроенное представление
    virtual void embedViewDetached(View* view);
    //! Вызывается когда произошла ошибка во встроенном представлении при загрузке, сохранении или удалении объекта
    virtual void onEmbedViewError(View* view, ObjectActionType type, const Error& error);

    //! Проверить свойство на ошибки и занести результаты проверки в HighlightInfo
    //! Имеет смысл переопределять только если объект отвечает за обработку ошибок (к примеру представление по умолчанию
    //! их не проверяет (если у него структура данных совпадает с моделью), а делегирует это модели)
    void getHighlight(const DataProperty& p, HighlightInfo& info) const override;

protected: // поддержка Drag&Drop. Работает как для MDI, так и для окон редактирования
    //! Начинать ли перемещение
    virtual bool isDragBegin(
        //! Виджет над которым начинается перетаскивание
        QWidget* drag_widget,
        //! Свойство, которое соответствует виджету, над которым начинается перетаскивание. Может быть не валидным
        const zf::DataProperty& property,
        //! Положение мыши на экране в глобальных координатах
        const QPoint& pos,
        //! Передаваемые данные. Необходимо инициализировать
        QMimeData* drag_data,
        //! Рисунок для отрисовки в процессе перемещения. Если не задан, то используется рисунок  "Файл"
        QPixmap& drag_pixmap);
    //! Разрешить ли обработку drag move events при заходе курсора с перетаскиваемым объектом на target
    virtual bool onDragEnter(
        //! Представление - источник. Может быть nullptr, если источником является другая программа
        View* source_view,
        //! Виджет - источник. Может быть nullptr, если источником является другая программа
        QWidget* source_widget,
        //! Свойство, которое соответствует виджету источнику. Может быть не валидным
        const zf::DataProperty& source_property,
        //! Текущая цель. Может быть как окном в целом, так и виджетом на форме (в зависимости от положения мыши)
        QWidget* target,
        //! Свойство, которое соответствует виджету цели. Может быть не валидным
        const zf::DataProperty& target_property,
        //! Положение мыши на экране в глобальных координатах
        const QPoint& pos,
        //! Передаваемые данные
        const QMimeData* drag_data);
    //! Разрешить ли выполнить drop для target
    virtual bool onDragMove(
        //! Представление - источник. Может быть nullptr, если источником является другая программа
        View* source_view,
        //! Виджет - источник. Может быть nullptr, если источником является другая программа
        QWidget* source_widget,
        //! Свойство, которое соответствует виджету источнику. Может быть не валидным
        const zf::DataProperty& source_property,
        //! Ткущая цель. Может быть как окном в целом, так и виджетом на форме (в зависимости от положения мыши)
        QWidget* target,
        //! Свойство, которое соответствует виджету цели. Может быть не валидным
        const zf::DataProperty& target_property,
        //! Положение мыши на экране в глобальных координатах
        const QPoint& pos,
        //! Передаваемые данные
        const QMimeData* drag_data);
    //! Выполнить drop
    virtual bool onDrop(
        //! Представление - источник. Может быть nullptr, если источником является другая программа
        View* source_view,
        //! Виджет - источник. Может быть nullptr, если источником является другая программа
        QWidget* source_widget,
        //! Свойство, которое соответствует виджету источнику. Может быть не валидным
        const zf::DataProperty& source_property,
        //! Ткущая цель. Может быть как окном в целом, так и виджетом на форме (в зависимости от положения мыши)
        QWidget* target,
        //! Свойство, которое соответствует виджету цели. Может быть не валидным
        const zf::DataProperty& target_property,
        //! Положение мыши на экране в глобальных координатах
        const QPoint& pos,
        //! Передаваемые данные
        const QMimeData* drag_data);
    //! Событие QDragLeaveEvent для виджета
    virtual void onDragLeave(QWidget* drag_widget,
        //! Свойство, которое соответствует виджету. Может быть не валидным
        const zf::DataProperty& property);

protected: // взаимодействие с модальным окном редактирования
    /*
     * Для полного переопределения стандартной логики работы окна редактировния в части реакции на нажатия кнопок и ошибки, надо:
     * 1) Реализовать onModalWindowButtonClick (ВАЖНО: ModalWindowButtonAction::Continue возвращать только для обработки операций)
     * 2) Реализовать getModalWindowShowHightlightFrame
     */

    //! Действия после вызова onModalWindowButtonClick
    enum class ModalWindowButtonAction {
        //! Отменить все дальнейшие стандартные действия после нажатия на кнопку
        Cancel,
        //! Закрыть окно без всяких проверок и действий
        Close,
        //! Закрыть окно с проверкой на изменение данных (стандартное поведение при нажатии на "крестик")
        CloseCheck,
        //! Закрыть окно с сохранением данных (стандартное поведение при нажатии на "сохранить")
        CloseSave,
        //! Продолжить дальнейшие стандартные действия после нажатия на кнопку
        Continue
    };
    //! Метод, для полного перехвата всех действий при нажатии на кнопки диалога
    //! Вызывается до всех остальных методов
    virtual ModalWindowButtonAction onModalWindowButtonClick(
        //! Нажатая кнопка
        //! Может быть QDialogButtonBox::StandardButton::NoButton. В этом случае это означает нажатие кнопки, заданной через setModalWindowOperations
        QDialogButtonBox::StandardButton button,
        //! Нажата кнопка операции, заданной через setModalWindowOperations
        const Operation& op,
        //! Нажато закрытие окна через "крестик"
        bool cross_button);

    //! Вызывается перед установкой/снятием "красной" рамки вокруг кнопки "сохранить"
    //! Cтандартное поведение - проверка h->hasErrorsWithoutOptions(HighlightOption::NotBlocked) и установка рамки вокруг кнопки "Сохранить"
    //! Если вернуть false, то стандартное поведение отменяется и используются данные в buttons/operations
    virtual bool getModalWindowShowHightlightFrame(
        //! Ошибки
        const zf::HighlightModel* h,
        //! Состояние кнопок
        QMap<QDialogButtonBox::StandardButton, InformationType>& buttons,
        //! Состояние операций
        QMap<OperationID, InformationType>& operations);

    //! Вызывается когда представление встроено в модальное окно редактирования или просмотра при нажатии на "OK/Сохранить"
    //! При возврате false действие отменяется. При возврате true выполняется стандартное сохранение
    virtual bool onModalWindowOkSave();
    //! Нестандартная обработка сохранения модели в БД. Метод синхронный
    //! Вызывается когда представление встроено в модальное окно редактирования
    //! Если программист выполнил сохранение модели, то вернуть true, иначе false (в том числе и при ошибке)
    //! При возврате true стандартое сохранение выполняться не будет
    virtual bool onModalWindowCustomSave(Error& error);
    //! Вызывается когда представление встроено в модальное окно редактирования или просмотра при нажатии на "Отмена"
    //! При возврате false действие отменяется
    virtual bool onModalWindowCancel();
    //! Вызывается после успешного сохранения в модальном окне
    virtual void afterModalWindowSave();

    //! Вызывается когда представление встроено в модальное окно редактирования, при проверке на доступность кнопки "OK/Сохранить"
    //! ВАЖНО: это условие в дополнение к доступности, которой управляет модальное редактирования
    bool isModalWindowOkSaveEnabled() const;
    //! Задать доступность кнопки "OK/Сохранить". Используется когда представление встроено в модальное окно редактирования
    //! ВАЖНО: это условие в дополнение к доступности, которой управляет модальное редактирования
    void setModalWindowOkSaveEnabled(bool b);

    //! В режиме модального редактирования/просмотра: вызывается когда окно встраивается в модальный диалог редактирования
    //! После этого становится доступен интерфейс I_ModalWindow через modalWindowInterface
    virtual void onModalWindowActivated();

    //! В режиме MDI просмотра: вызывается когда окно встраивается в MDI окно
    //! После этого становится доступен интерфейс I_ModuleWindow через moduleWindowInterface
    virtual void onModuleWindowActivated();

    //! Задать операции режима редактирования. Будут добавлены на панель кнопок модального диалога при его вызове
    void setModalWindowOperations(const OperationIDList& operations);
    //! В режиме модального редактирования/просмотра: возвращает интерфейс взаимодействия с окном диалога
    I_ModalWindow* modalWindowInterface() const;
    //! Был ли уже установлен интерфейс I_ModalWindow (вызывается когда окно встраивается в модальный диалог редактирования)
    bool isModalWindowInterfaceInstalled() const;

    //! В режиме MDI окна: возвращает интерфейс взаимодействия с MDI окном
    I_ModuleWindow* moduleWindowInterface() const;
    //! Был ли уже установлен интерфейс I_ModuleWindow (вызывается когда окно встраивается в MDI окно)
    bool isModuleWindowInterfaceInstalled() const;

    //! Есть ли несохраненные данные. По умолчанию проверяет различие данных до сохранения с текущими
    virtual bool hasUnsavedData(
        //! Ошибка
        Error& error) const;
    //! Есть ли несохраненные данные
    bool hasUnsavedData() const;

protected:
    //! Загрузить форму (ui файл) из ресурсов и установить ее в moduleUI.
    //! При необходимости загрузки ui по частям, надо использовать Utils::openUI
    Error setUI(const QString& resource_name);

    //! Добавить дополнительный тулбар
    void addToolbar(QToolBar* toolbar);
    //! Удалить дополнительный тулбар (можно просто удалить объект, не вызывая этот метод)
    void removeToolbar(QToolBar* toolbar);

    //! Провести автоматическую "подмену" виджетов. Происходит автоматически
    //! Вызывать только в случае добавления виджетов, требующих подмены, вне конструктора расширения
    void replaceWidgets();
    //! Запросить проверку экшенов и виджетов на доступность. Несколько вызовов подряд приведут к одному вызову
    //! onUpdateAccessRights и updateWidgetStatus(для каждого виджета)
    Q_SLOT void requestUpdateView() const;

    //! Запросить обновление состояния виджета. Запрос будет поставлен в очередь, по окончании вызовется метод updateWidgetStatus
    void requestUpdatePropertyStatus(const DataProperty& p) const;
    void requestUpdatePropertyStatus(const PropertyID& p) const;
    //! Запросить обновление состояния всех виджетов. Запрос будет поставлен в очередь, по окончании вызовется метод updateWidgetStatus
    Q_SLOT void requestUpdateAllPropertiesStatus() const;

    //! Запросить обновление конкретного виджета
    void requestUpdateWidgetStatus(QWidget* w) const;
    void requestUpdateWidgetStatus(const QString& widget_name) const;
    //! Запросить обновление виджетов
    void requestUpdateWidgetStatuses() const;

    //! Запросить проверку прав доступа
    Q_SLOT void requestUpdateAccessRights() const;

    //! Фильтр событий
    bool eventFilter(QObject* obj, QEvent* event) override;

    //! Обработчик входящих сообщений от диспетчера сообщений
    zf::Error processInboundMessage(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle) override;

    //! Обработчик менеджера обратных вызовов
    void processCallbackInternal(int key, const QVariant& data) final;

    //! Нестандартное имя конфигурации (служебный)
    QString customConfigurationCodeInternal() const final;

    //! Вызывается при необходимости сгенерировать сигнал об ошибке
    void registerError(const Error& error, ObjectActionType type = ObjectActionType::Undefined) final;

    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void onDataInvalidate(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;
    //! Изменилось состояние валидности данных. Вызывается при каждом изменении состояния для каждого свойства
    void onDataInvalidateChanged(const DataProperty& p, bool invalidate, const zf::ChangeInfo& info) override;
    //! Вызывается один раз после изменения состояния invalidate в true для одного или нескольких свойств
    virtual void onInvalidated(const zf::ChangeInfo& info);

protected: // I_HighlightView
    //! Определение порядка сортировки свойств при отображении HighlightView
    //! Вернуть истину если property1 < property2
    bool highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const override;
    //! Инициализирован ли порядок отображения ошибок
    bool isHighlightViewIsSortOrderInitialized() const override;

private: // реализация интерфейса I_ConditionFilter
    //! Фильтрация на основании условий для указанного набора данных
    ComplexCondition* getConditionFilter(const DataProperty& dataset) const final;
    //! Вычислить условие
    bool calculateConditionFilter(const DataProperty& dataset, int row, const QModelIndex& parent) const final;
    //! Сигнал о том, что поменялся фильтр для набора данных
    Q_SIGNAL void sg_conditionFilterChanged(const zf::DataProperty& dataset) final;

private: // реализация интерфейса I_DatasetVisibleInfo
    //! Преобразовать отображаемое в ячейке значение
    Error convertDatasetItemValue(const QModelIndex& index,
        //! Оригинальное значение
        const QVariant& original_value,
        //! Параметры
        const VisibleValueOptions& options,
        //! Значение после преобразования
        QString& value,
        //! Получение данных не возможно - требуется дождаться загрузки моделей
        QList<ModelPtr>& data_not_ready) const final;
    //! Информация о видимости строк набора данных
    void getDatasetRowVisibleInfo(
        //! Оригинальный индекс
        const QModelIndex& index,
        //! Индекс прокси (тот что виден)
        QModelIndex& visible_index) const final;
    //! Информация о видимости колонок набора данных
    void getDatasetColumnVisibleInfo(
        //! Источник данных
        const QAbstractItemModel* model,
        //! Колонка
        int column,
        //! Колонка сейчас видима
        bool& visible_now,
        //! Колонка может быть видима, даже если сейчас скрыта
        bool& can_be_visible,
        //! Порядковый номер с точки зрения отображения
        //! Если колонка не видна, то -1
        int& visible_pos) const final;
    //! Имя колонки набора данных
    QString getDatasetColumnName(
        //! Источник данных
        const QAbstractItemModel* model,
        //! Колонка
        int column) const final;

private: // реализация интерфейса I_DataWidgetsLookupModels
    //! Получить lookup модель для указанного свойства
    void getPropertyLookupModel(
        //! Свойство данных, для которого необходимо определить lookup
        const DataProperty& property,
        //! Модель, откуда брать данные для расшифровки lookup
        ModelPtr& model,
        //! Фильтр
        DataFilterPtr& filter,
        //! Колонка, откуда брать данные для расшифровки lookup
        DataProperty& lookup_column_display,
        //! Колонка, откуда брать данные для подстановки ID в исходный набор данных
        DataProperty& lookup_column_id,
        //! Роль для получения расшифровки из lookup_column_display
        int& lookup_role_display,
        //! Роль для получения id из lookup_column_id
        int& lookup_role_id) const final;

    /*! Метод вызывается ядром или лаунчером в момент, когда модель или представление активируется
     * (открывается диалог, MDI окно получает фокус, активируется рабочая зона лаунчера и т.п.) или наоброт становится неактивным
     * Не вызывать */
    void onActivatedInternal(bool active) override final;

public: // *** реализация I_HightlightController
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

signals:
    //! Завершилась обработка операции
    void sg_operationProcessed(const zf::Operation& op, const zf::Error& error);
    //! Интерфейс был заблокирован
    void sg_uiBlocked();
    //! Интерфейс был разблокирован
    void sg_uiUnBlocked();

    //! Перед установкой новой модели (вызывается только при установке модели вне конструктора)
    void sg_beforeModelChange(const zf::ModelPtr& old_model, const zf::ModelPtr& new_model);
    //! После установки новой модели (вызывается только при установке модели вне конструктора)
    void sg_afterModelChange(const zf::ModelPtr& old_model, const zf::ModelPtr& new_model);

    //! Сменились состояния представления
    void sg_statesChanged(const zf::ViewStates& new_states, const zf::ViewStates& old_states);

    //! Добавлен тулбар
    void sg_toolbarAdded(QToolBar* toolbar);
    //! Удален один из тулбаров
    void sg_toolbarRemoved();

    //! Завершена загрузка модели
    void sg_model_finishLoad(const zf::Error& error,
        //! ПаdatasetIndexраметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Изменилось состояние "только для чтения"
    void sg_readOnlyChanged(bool read_only);

    //! Сменилось состояние автопоиска
    void sg_autoSearchStatusChanged(bool is_active);
    //! Сменился текст автопоиска
    void sg_autoSearchTextChanged(const QString& text);

    //! Смена фокуса, закладки тулбара и т.п.
    void sg_activeControlChanged();

    //! Изменился текущий выбор виджета поиска searchWidget (действие пользователя)
    void sg_searchWidgetEdited(
        //! Какой идентификатор был выбран
        const zf::Uid& entity);

    //! Изменилась видимость свойства
    void sg_fieldVisibleChanged(const zf::DataProperty& property, QWidget* widget);

    //! Сменилось родительское представление
    void sg_parentViewChanged(const QList<zf::View*>& views);
    //! Сменился родительский диалог
    void sg_parentDialogChanged(zf::Dialog* dialog);

    //!  Используется когда представление встроено в модальное окно редактирования.
    //! Срабатывает при изменении доступностт кнопки "OK/Сохранить" через метод setEditWindowOkSaveEnabled
    void sg_editWindowOkSaveEnabledChanged();

    //! Начата загрузка данных. Отслеживается не только сам факт начала загрузки моделью, но и запрос на такую загрузку
    void sg_startLoad();
    //! Завершена зугрузка данных. Отслеживается не только сам факт начала загрузки моделью, но и запрос на такую загрузку
    void sg_finishLoad();

    //! Сигнал генерируется если при операциях с моделью произошла ошибка
    void sg_error(zf::ObjectActionType type, const zf::Error& error);

private slots:
    //! Изменился признак существования объекта в БД
    void sl_existsChanged();
    //! Изменилось имя сущности в модели
    void sl_modelEntityNameChanged();

    //! Удален дополнительный тулбар
    void sl_additionalToolbarDestroyed(QObject* obj);

    //! Сработал экшен операции
    void sl_operationActionActivated(const zf::Operation& op);

    //! Раскрылся или закрылся CollapsibleGroupBox
    void sl_expandedChanged();
    //! У одного из табов поменялась текущая закладка. Представление подписывается на сигналы всех табов
    void sl_currentTabChanged(int index);
    //! Сменилось положение сплиттера. Представление подписывается на сигналы всех сплиттеров
    void sl_splitterMoved(int pos, int index);

    //! Перед началом загрузки модели. ВАЖНО: это только запрос на загрузку, она может и не начаться если все данные уже есть или загрузка их уже в очереди
    void sl_model_beforeLoad(
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Атрибуты, которые надо загрузить. Если не задано - все
        const zf::DataPropertySet& properties);
    //! Начата загрузка
    void sl_model_startLoad();
    //! Начато сохранение
    void sl_model_startSave();
    //! Начато удаление
    void sl_model_startRemove();

    //! Тамер блокировки UI
    void sl_blockUiTimeout();
    //! Таймер слепого поиска
    void sl_searchTimeout();

    //! Таймер _dataset_current_cell_changed_timer
    void sl_datasetCurrentCellChangedTimeout();
    //! Таймер _dataset_row_changed_timer
    void sl_datasetCurrentRowChangedTimeout();
    //! Таймер _dataset_column_changed_timer
    void sl_datasetCurrentColumnChangedTimeout();

    //! Изменился текущий выбор виджета поиска searchWidget (действие пользователя)
    void sl_searchWidgetEdited(const QModelIndex& index, bool by_keys);

    //! Завершена загрузка
    void sl_model_finishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);
    //! Завершено сохранение
    void sl_model_finishSave(const zf::Error& error,
        //! Какие свойства сохранялись
        const zf::DataPropertySet& requested_properties,
        //! Какие свойства сохранялись фактически. Перечень не может быть шире чем requested_properties, т.к. сервер может сохранять свойства группами
        const zf::DataPropertySet& saved_properties,
        /*! Постоянный идентификатор, полученный после сохранения модели с isTemporary() == true.
         * Если модель уже имеет постоянный идентификатор, то возвращается пустой */
        const zf::Uid& persistent_uid);
    //! Завершено удаление
    void sl_model_finishRemove(const zf::Error& error);

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

    //! Сработал action для тулбара набора данных
    void sl_datasetActionActivated(const zf::DataProperty& property, zf::ObjectActionType type);

    //! Набор данных активирован
    void sl_datasetActivated(const QModelIndex& index);
    //! Одиночное нажатие на набор данных
    void sl_datasetClicked(const QModelIndex& index);
    //! Двойное нажатие на набор данных
    void sl_datasetDoubleClicked(const QModelIndex& index);

    //! Изменилось выделение набор данных
    void sl_datasetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    //! Сменилась текущая ячейка набор данных
    void sl_datasetCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Сменилась текущая строка набор данных
    void sl_datasetCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Сменилась текущая колонка набор данных
    void sl_datasetCurrentColumnChanged(const QModelIndex& current, const QModelIndex& previous);

    //! Активировано контекстное меню для набор данных
    void sl_datasetContextMenuRequested(const QPoint& point);

    //! Завершена загрузка лукап модели, используемой в фильтрации и сортировке
    void sl_conditionLookupModelLoaded(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Завершена загрузка лукап модели
    void sl_lookupModelLoaded(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Изменились данные в lookup модели
    void sl_lookupModelChanged();

    //! Изменилось выделение чекбоксами
    void sl_checkedRowsChanged();
    //! Изменилась видимость панели выделения чекбоксами
    void sl_checkPanelVisibleChanged();

    //! Вызывается когда происходит ошибка внутри всяких обработчиков сообщений и т.п.
    //! Для самого представления и его модели
    void sl_error(zf::ObjectActionType type, const zf::Error& error);

    //! Отображена блокировка родительского представления
    void sl_parentViewUiBlocked();

    //! Добавлены строки в прокси набор данных
    void sl_proxy_rowsInserted(const QModelIndex& parent, int first, int last);
    //! Прокси набор данных сброшен
    void sl_proxy_modelReset();

    //! Вызывается когда добавлен новый объект
    void sl_containerObjectAdded(QObject* obj);
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    void sl_containerObjectRemoved(QObject* obj);

    //! Все виджеты были обновлены
    void sl_widgetsUpdated();

    //! Был удален модальный виджет верхнего уровня
    void sl_topWidgetDestroyed(QObject* obj);

    //! Данные были отфильтрованы
    void sl_refiltered(
        //! Набор данных
        const zf::DataProperty& dataset_property);

    //! Первая попытка загрузки
    void sl_onModelFirstLoadRequest();

private:
    using EntityObject::setEntity;

    //! Начальная инициализация
    void bootstrap();
    //! Изменилось имя сущности
    void onEntityNameChangedHelper() final;
    //! Закончить создание
    void finishConstruction();
    //! Отключиться от виджетов свойств
    void disconnectFromProperties();
    //! Подключиться к виджетам свойств
    void connectToProperties();
    //! Инициализировать виджеты свойств
    void initializeProperties();
    //! Вызывается автоматически после создания view
    void afterCreatedHelper();
    //! Вызывается один раз после открытия представления и его инициализации (createUI) после того как будут загружены данные
    void onDataReadyHelper();
    //! Создать UI на основании схемы
    void createUiByScheme();
    //! Подготовить тулбар для отображения
    void prepareToolbar(QToolBar* toolbar);

    //! Задать порядок виджетов на форме
    void setTabOrder(const QList<QWidget*>& tab_order);

    //! Поиск элемента по имени (служебная функция)
    QList<QObject*> findElementHelper(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        const QStringList& path, const char* class_name) const;

    //! Обновить отрисовку ошибки на виджете
    void updateWidgetHighlight(const HighlightItem& item);

    //! Изменились условия фильтрации
    void conditionFilterChanged(const DataProperty& dataset);

    //! Удалить лишние сепараторы
    void compressActionsHelper();

    //! Доступ к OperationMenuManager с "ленивой" инициализацией
    OperationMenuManager* operationMenuManager() const;

    void connectToWidget(QWidget* w, const DataProperty& property);
    void disconnectFromWidget(QWidget* w);

    //! Обновить отображение блокировки интерфейса
    void updateBlockUiGeometryHelper();
    void updateBlockUiGeometry(bool force);
    //! Отобразить блокировку интерфейса
    void showBlockUi(bool force);
    void hideBlockUi();
    void showBlockUiHelper(bool force);
    void hideBlockUiHelper();

    //! Поиск родительского представления
    void requestUpdateParentView();
    void updateParentView();
    //! Заблокировано ли UI родительского представления
    bool isParentViewUiBlocked() const;

    //! Событие QDragEnterEvent для виджета. Если результат true, то дальнейшая обработка не производится
    bool onDragEnterHelper(QWidget* widget, QDragEnterEvent* event);
    //! Событие QDragMoveEvent для виджета. Если результат true, то дальнейшая обработка не производится
    bool onDragMoveHelper(QWidget* widget, QDragMoveEvent* event);
    //! Событие QDropEvent для виджета. Если результат true, то дальнейшая обработка не производится
    bool onDropHelper(QWidget* widget, QDropEvent* event);
    //! Событие QDragLeaveEvent для виджета. Если результат true, то дальнейшая обработка не производится
    bool onDragLeaveHelper(QWidget* widget, QDragLeaveEvent* event);
    //! Перехватывать ли события Drag&Drop для указанного виджета
    bool isCatchDragDropEvents(QWidget* w) const;

    //! Установить состояние представления
    void setStates(const ViewStates& s);

    //! Автоматическая расстановка табуляции
    void requestInitTabOrder();
    void initTabOrder();

    //! Загрузить всю информацию об ошибках из модели
    void loadAllHighlights();

    //! Обновить состояние статус бара с ошибками
    void updateStatusBarErrorInfo();

    //! Определяет надо ли реагировать на событие в QItemSelectionModel
    bool getDatasetSelectionProperty(QObject* sender, DataProperty& p) const;

    //! Переопределение цвета ячейки таблицы и т.п. свойств
    void getDatasetCellPropertiesHelper(const DataProperty& column, const QModelIndex& index, QStyle::State state, QFont& font, QBrush& background,
        QPalette& palette, QIcon& icon, Qt::Alignment& alignment) const;

    //! Обработка нажатия клавиши в наборе данных. Если обработано, то true
    bool processDatasetKeyPressHelper(const DataProperty& p, QAbstractItemView* view, QKeyEvent* e);

    //! Подготовить виджет к работе в представлении
    void prepareWidget(QWidget* w,
        //! Свойство данных. Может быть не задано
        const DataProperty& p);
    //! Обновить состояние всех виджетов
    void updateAllWidgetsStatusHelper();
    //! Выполнение проверки всех экшенов и виджетов на доступность
    void updateAccessRightsHelper() const;

    //! Активировано редактирование ячейки
    bool editCellInternal(const DataProperty& dataset_property, QModelIndex current_index, QKeyEvent* e, const QString& text);

    //! Обновить состояние виджета поиска
    void updateSearchWidget();

    //! Вызывается после загрузки конфигурацию (пользовательские настройки окна).
    //! Когда вызван этот метод, окно уже восстановило положения сплиттеров, размер и т.п.
    void afterLoadConfigurationHelper();

    //! Установить интерефейс взаимодействия с модальным окном
    void setModalWindowInterface(I_ModalWindow* i_modal_window);

    //! Установить интерефейс взаимодействия с MDI окном
    void setModuleWindowInterface(I_ModuleWindow* i_module_window);

    //! Исправить текущий фокус ввода
    void fixFocusedWidget();

    //! Автоматически выделить первую строку набора данных, если она не выбрана
    void autoSelectFirstRow(const DataProperty& dataset_property);
    //! Обработать автоматическое выделение первых строку набора данных, если они не выбраны
    void processAutoSelectFirstRow();

    //! Фильтр событий для модального виджета верхнего уровня
    bool processTopWidgetFilter(QObject* obj, QEvent* event);
    //! Убрать контроль скрытия/удаления для модального виджета верхнего уровня
    void stopProcessTopWidgetHide(QObject* obj);

    //! Перезагрузить данные из модели. Возвращает истину если попытка загрузки была выполнена
    bool loadModel();
    //! Обработка смены видимости виджетов
    void processVisibleChange();

    //! Изменилось состояние isReadOnly
    Q_SLOT void onReadOnlyChangedHelper();

    //! Подключено встроенное представление
    void embedViewAttachedHelper(View* view);
    //! Отключено встроенное представление
    void embedViewDetachedHelper(View* view);

    //! Обработать ошибку
    void processErrorHelper(zf::ObjectActionType type, const Error& error);

    //! Начало загрузки данных. Отслеживается не только сам факт начала загрузки моделью, но и запрос на такую загрузку
    void onStartLoading();
    //! Окончание загрузки данных. Отслеживается не только сам факт начала загрузки моделью, но и запрос на такую загрузку
    void onFinishLoading();

    // заполнение lookup полей, значениями по умолчанию если в lookup списке всего один элемент
    void fillLookupValues(
        //! Если не задано, то для всех полей
        const DataProperty& property = {});

    //! Задать произвольные данные, передаваемые при создании окна в WindowManager
    void setCustomData(const QMap<QString, QVariant>& custom_data);

    //! Реация на изменение в списке ошибок и передача их в WidgetHighlighter
    WidgetHighligterController* widgetHighligterController() const;
    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* highligtMapping() const;

    //! Разная служебная информация о свойстве
    struct PropertyInfo //-V730
    {
        DataProperty property;
        //! Текущая позиция в наборе данных до перезагрузки набора данных
        RowID reload_id;
        //! Текущая колонка в наборе данных до перезагрузки набора данных
        int reload_column = -1;

        //! Состояние "только для чтения"
        Answer read_only = Answer::Undefined;
        //! Принудительно скрыть тулбар для набора данных
        Answer toolbar_hidded  = Answer::Undefined;
        //! В тулбаре нет видимых экшенов
        Answer toolbar_has_visible_actions  = Answer::Undefined;

        struct ConditionLookupLoadWaitingInfo {
            // модель lookup
            QPointer<Model> lookup_model;
            //!  Подписка на сигнал об окончании перезагрузки lookup
            QMetaObject::Connection connection;
        };
        //! Список lookup моделей, которые должны загрузиться прежде чем с ними можно будет работать в фильтрации или сортировке
        QList<ConditionLookupLoadWaitingInfo> condition_lookup_load_waiting_info;

        //!  Подписка на сигнал об окончании перезагрузки lookup
        QMetaObject::Connection connection_lookup;
    };
    //! Разная служебная информация о свойстве
    mutable QHash<DataProperty, std::shared_ptr<PropertyInfo>> _property_info;
    PropertyInfo* propertyInfo(const DataProperty& p) const;

    //! Соответствие между лукап моделями и свойствами
    QMap<Model*, DataProperty> _lookup_model_map;

    //! Свойства, которые надо обновить по Framework::VIEW_UPDATE_PROPERTY_STATUS
    mutable DataPropertySet _properties_to_update;
    //! Виджеты, состояние которых надо обновить по Framework::VIEW_REQUEST_UPDATE_WIDGET_STATUS
    mutable QList<QPointer<QWidget>> _widgets_to_update;

    //! Модель
    ModelPtr _model;
    //! Отображение свойств прокси
    QMap<DataProperty, DataProperty> _property_proxy_mapping;
    //! Режим работы
    bool _model_proxy_mode;
    //! Главный виджет, который встраивается в окно
    QPointer<QWidget> _main_widget;
    //! Виджет, на котором должен размещаться пользовательский интерфейс модуля
    QPointer<QWidget> _body;
    //! Менеджер меню
    mutable ObjectExtensionPtr<OperationMenuManager> _operation_menu_manager;

    //! Интерфейс для перехвата выполнения операций View
    I_ViewOperationHandler* _execute_operation_handler = nullptr;

    //! Интерфейс был создан автоматически
    bool _ui_auto_created = false;
    //! Имеет ли созданный автоматически интерфейс видимые рамки
    bool _ui_auto_created_has_borders = false;

    //! Дополнительные тулбары представления (добавляются отдельно, операции автоматически не обрабатываются)
    mutable QList<QPointer<QToolBar>> _additional_toolbars;

    //! Механизм замены виджетов. Если не задано, то используется стандартный WidgetReplacerDataStructure
    mutable std::shared_ptr<WidgetReplacer> _widget_replacer;
    WidgetReplacerDataStructure* _widget_replacer_ds = nullptr;
    //! Виджеты, сгенерированные по данным
    mutable ObjectExtensionPtr<DataWidgets> _widgets;

    //! Виджеты, для которых надо игнорировать установку readonly
    QSet<QPointer<QWidget>> _ignore_read_only_widgets;

    //! Фильтрация
    DataFilterPtr _filter;

    //! В процессе отработки конструктора
    bool _is_constructing = true;
    //! В процессе отработки деструктора
    bool _is_destructing = false;
    //! Создание формы закончено полностью
    bool _is_full_created = false;
    //! Был ли вызов onDataReady
    bool _is_data_ready_called = false;
    //! Был ли вызов onWidgetsReady
    bool _is_widgets_ready_called = false;
    //! Для исключения рекурсии в processDatasetViewKeyPressHelper
    bool _process_dataset_press_helper = false;

    //! Находится в режиме "только для чтения"
    bool _is_read_only = false;
    //! Заблокированы ли поля ввода
    bool _is_block_controls = false;

    //! Состояние представления
    ViewStates _states;
    //! Находится в диалоговом окне
    bool _is_dialog = false;

    //! Счетчик блокировки интерфейса
    int _block_ui_counter = 0;
    //! Счетчик блокировки интерфейса до окончания создания представления
    int _block_ui_counter_before_create = 0;

    //! Виджет поиска
    QPointer<ItemSelector> _search_widget;

    //! Анимация ожидания (виджет)
    QLabel* _wait_movie_label = nullptr;

    //! Виджет для блокировки нажатий на UI в режиме ожидания
    QWidget* _block_ui_top_widget = nullptr;
    //! Таймер для задержки отображения ожидания
    QTimer* _show_block_ui_timer = nullptr;

    //! Задержка последнего выделения ячейки набора данных
    QTimer* _dataset_current_cell_changed_timer = nullptr;
    DataProperty _dataset_current_cell_changed;

    //! Задержка последнего выделения колонки набора данных
    QTimer* _dataset_current_row_changed_timer = nullptr;
    DataPropertySet _dataset_current_row_changed;

    //! Задержка последнего выделения строки набора данных
    QTimer* _dataset_current_column_changed_timer = nullptr;
    DataProperty _dataset_current_column_changed;

    //! Последний набор данных, который имел фокус ввода (или просто считался активным)
    DataProperty _last_active_dataset;

    //! Индикация ошибок
    WidgetHighlighter* _widget_highlighter = nullptr;
    //! Реация на изменение в списке ошибок и передача их в WidgetHighlighter
    mutable ObjectExtensionPtr<WidgetHighligterController> _widget_highligter_controller;
    //! Связь между виджетами и свойствами для обработки ошибок
    mutable HighlightMapping* _highligt_mapping = nullptr;

    //! Фильтрация на основании условий
    mutable QMap<DataProperty, std::shared_ptr<ComplexCondition>> _condition_filters;

    //! Последний текст поиска при слепом поиске
    QString _last_search;
    //! Таймер поиска, сбрасывающий поисковую строку через интервал
    QTimer* _search_timer = nullptr;

    //! Точка начала перетаскивания
    QPoint _drag_start_pos;

    //! Поиск объектов по имени
    ObjectsFinder* _objects_finder;

    //! Порядок виджетов на форме
    QList<QWidget*> _tab_order;

    //! Интерефейс взаимодействия с модальным окном
    I_ModalWindow* _i_modal_window = nullptr;
    //! Интерефейс взаимодействия с MDI окном
    I_ModuleWindow* _i_module_window = nullptr;

    //! Операции режима редактирования. Будут добавлены на панель кнопок модального диалога
    OperationList _modal_window_operations;
    //! Кнопки, созданные по _edit_window_operations
    mutable QMap<OperationID, QPushButton*> _modal_window_buttons;

    struct ParentViewInfo {
        //! Родительское представление
        QPointer<View> view;
        //! Коннект к родительскому представлению к сигналу отображения блокировки UI
        QMetaObject::Connection parent_view_block_ui_connection;
        //! Коннект к родительскому представлению к сигналу смены состояния readOnly
        QMetaObject::Connection parent_view_readonly_changed_connection;
    };
    //! Список родительских представлений от нижнего к верхнему
    QList<std::shared_ptr<ParentViewInfo>> _parent_views;

    //! Указатель на родительский диалог
    QPointer<Dialog> _parent_dialog;

    //! Время последнего изменения конфигурации для данного экземпляра представления
    mutable QDateTime _config_time;
    //! Имя конфигурации во время последней загрузки
    mutable QString _config_code;

    //! Время последнего изменения конфигурации представления для модуля в целом
    static QHash<EntityCode, QDateTime>* globalConfigTime();
    static std::unique_ptr<QHash<EntityCode, QDateTime>> _global_config_time;

    //! Последний виджет, который имел фокус
    QPointer<QWidget> _last_focused;

    //! Доступность кнопки "OK/Сохранить". Используется когда представление встроено в модальное окно редактирования
    bool _eit_window_ok_save_enabled = true;

    //! Запросы на проверку необходимости выделения первой строки в наборах данных
    DataPropertySet _auto_select_first_row_requests;

    //! Свойства, которые поддерживают частичную загрузку
    DataPropertySet _partial_load_properties;

    //! Встроенные представления
    QList<View*> _embed_views;

    //! Признак загрузки данных
    bool _loading = false;
    //! Если истина, то не использовать HightlightModel из модели, а сделать свою независимую от модели
    bool _is_detached_highlight = false;

    //! Подписки на изменения в лукапах
    QSet<SubscribeHandle> _lookup_subscribe;

    //! Произвольные данные, передаваемые при создании окна в WindowManager
    QMap<QString, QVariant> _custom_data;

    //! Все ItemSelector
    mutable std::unique_ptr<QHash<DataProperty, ItemSelector*>> _item_selectors;

    //! Состояние видимости представления в целом
    bool _visible_status = true;
    //! Есть ли необходимость в обновлении данных при переходе в _visible_status true
    bool _data_update_required = false;

    //! Скрыты ли все тулбары
    bool _is_toolbars_hidden = false;

    //! Причина перезагрузки данных модели
    ChangeInfo _reload_reason;

    //! Параметры представления
    ViewOptions _view_options;

    //! Список модальных виджетов, открытых поверх представления в момент начала блокировки UI
    QSet<QObject*> _top_widget_filtered;
    //! Ключ - модальных виджет, открытый поверх представления. Значение - коннект к его сигналу destroyed
    QMap<QObject*, QMetaObject::Connection> _top_widget_destroyed_connection;

    //! В процессе вызова invalidateUsingModels. На всякий случай, для исключения бесконечного обновления
    mutable bool _invalidating_using_models = false;

    //! Запрос на загрузку модели после изменения видимости
    bool _request_load_after_loading = false;

    //! Заблокировать реакцию на ошибки (показ ошибок) при действиях с моделью и т.п.
    int _block_show_errors = 0;

    friend class Framework;
    friend class ItemDelegate;
    friend class WindowManager;
    friend class Condition;
    friend class ModuleWindow;
    friend class ModalWindow;
    friend class SelectionProxyItemModel;
    friend class HightlightController;
};

typedef std::shared_ptr<View> ViewPtr;

} // namespace zf

Q_DECLARE_METATYPE(zf::ViewPtr)
