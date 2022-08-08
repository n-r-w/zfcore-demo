#ifndef ZF_DATA_WIDGETS_H
#define ZF_DATA_WIDGETS_H

#include "zf_core.h"
#include "zf_data_container.h"
#include "zf_request_selector.h"
#include <QPointer>

namespace zf
{
class View;
class DataFilter;
class HighlightProcessor;
class ItemSelector;

//! Интерфейс для получения информации о списке значений PropertyLookup в случае, если не задана сущность выборки
class I_DataWidgetsLookupModels
{
public:
    virtual ~I_DataWidgetsLookupModels() {}

    //! Получить lookup модель для указанного свойства
    virtual void getPropertyLookupModel(
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
        int& lookup_role_id) const = 0;
};

//! Виджеты, автоматически сгенерированные по DataStructure, и связанные с DataContainer
class ZCORESHARED_EXPORT DataWidgets : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    //! Причина обновления данных. Важен порядок: от наименее важного к наиболее важному
    enum class UpdateReason
    {
        Undefined,
        //! Просто потому что захотелось
        NoReason,
        //! Удаление
        Removing,
        //! Сохранение
        Saving,
        //! Загрузка
        Loading,
    };
    //! Преобразовать состояние модели в DataWidgets::UpdateReason
    static UpdateReason modelStatusToUpdateReason(const ModelPtr& model);
    //! Вернуть наиболее "важную" из двух причин обновления с точки зрения отображения на экране
    static UpdateReason getImportantUpdateReason(UpdateReason r1, UpdateReason r2);

    //! На основе контейнера
    DataWidgets(
        //! База данных
        const DatabaseID& database_id,
        //! Данные
        const DataContainerPtr& data,
        //! Интерфейс для получения информации о списке значений PropertyLookup в случае, если не задана сущность
        //! выборки
        I_DataWidgetsLookupModels* dynamic_lookup = nullptr,
        //! Менеджер обратных вызовов
        CallbackManager* callback_manager = nullptr,
        //! Информация об ошибках
        HighlightProcessor* highlight = nullptr);
    //! На основе контейнера
    DataWidgets(
        //! Данные
        const DataContainerPtr& data,
        //! Менеджер обратных вызовов
        CallbackManager* callback_manager = nullptr,
        //! Информация об ошибках
        HighlightProcessor* highlight = nullptr);
    //! На основе view и ее контейнера
    DataWidgets(
        //! Представление
        View* view,
        //! Фильтрация (если не задано, то данные берутся из data напрямую
        DataFilter* filter,
        //! Интерфейс для получения информации о списке значений PropertyLookup в случае, если не задана сущность
        //! выборки
        I_DataWidgetsLookupModels* dynamic_lookup = nullptr,
        //! Менеджер обратных вызовов
        CallbackManager* callback_manager = nullptr);
    ~DataWidgets() override;

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;    
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Заменить источник данных (На основе контейнера)
    void setSource(
        //! База данных
        const DatabaseID& database_id,
        //! Данные
        const DataContainerPtr& data,
        //! Информация об ошибках
        HighlightProcessor* highlight = nullptr);
    //! Заменить источник данных (На основе view и ее контейнера)
    void setSource(
        //! Представление
        View* view,
        //! Фильтрация (если не задано, то данные берутся из data напрямую
        DataFilter* filter);

    //! Данные. В зависимости от конструктора - из DataContainer или контейнера View
    DataContainerPtr data() const;
    //! Структура данных
    DataStructurePtr structure() const;

    //! Фильтрация
    DataFilter* filter() const;

    //! Фильтр для полей с lookup
    DataFilter* lookupFilter(const PropertyID& property_id) const;
    //! Фильтр для полей с lookup
    DataFilter* lookupFilter(const DataProperty& property) const;

    //! Создать виджеты на основе данных
    void generateWidgets();
    //! Удалить виджеты
    void clearWidgets();

    //! Перезагрузить данные всех виджетов (обновление будет проведено сразу)
    void updateWidgets(UpdateReason reason);

    //! Запросить обновление всех виджетов (обновление будет проведено при следующей обработке событий)
    //! Возвращает истину если запрос был выполнен
    bool requestUpdateWidgets(UpdateReason reason, bool force_reason = false);
    //! Запросить отложенное обновление виджета. Возвращает истину если запрос был выполнен
    bool requestUpdateWidget(const DataProperty& property, UpdateReason reason,
                             //! запрашивать не смотря на то, что свойство заблокировано от изменений
                             bool ignore_block = false);

    //! Содержит ли виджет с таким свойством
    bool contains(const DataProperty& property) const;
    //! Содержит ли виджет с таким свойством
    bool contains(const PropertyID& property_id) const;

    //! Свойство по его идентификатору
    DataProperty property(const PropertyID& property_id) const;

    //! Редактор для свойства
    QWidget* field(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    //! Редактор для свойства
    QWidget* field(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;

    //! Редактор для свойства
    template <class T>
    inline T* field(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        QWidget* w = field(property, halt_if_not_found);
        if (!halt_if_not_found && w == nullptr)
            return nullptr;

#ifdef Q_OS_LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

        T* res = dynamic_cast<T*>(w);
        if (halt_if_not_cast && res == nullptr) {
            if (property.isEntityIndependent())
                Z_HALT(QString("Type conversion error %1,%2: %3 -> %4")
                           .arg(data()->id())
                           .arg(property.id().value())
                           .arg(w->metaObject()->className())
                           .arg(T::staticMetaObject.className()));
            else
                Z_HALT(QString("Type conversion error %1,%2: %3 -> %4") //-V523
                           .arg(property.entityCode().value())
                           .arg(property.id().value())
                           .arg(w->metaObject()->className())
                           .arg(T::staticMetaObject.className()));
        }

#ifdef Q_OS_LINUX
#pragma GCC diagnostic pop
#endif

        return res;
    }
    //! Редактор для свойства
    template <class T>
    inline T* field(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        return field<T>(property(property_id), halt_if_not_found, halt_if_not_cast);
    }

    //! Для совместимости названия с zf::View
    //! Редактор для свойства
    template <class T>
    inline T* object(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        return field<T>(property_id, halt_if_not_found, halt_if_not_cast);
    }
    //! Для совместимости названия с zf::View
    //! Редактор для свойства
    template <class T>
    inline T* object(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true,
        //! если невозможно преобразовать к типу - остановка
        bool halt_if_not_cast = true) const
    {
        return field<T>(property, halt_if_not_found, halt_if_not_cast);
    }

    //! Редактор для свойства. В отличие от field может включать составные виджеты (например набор данных с тулбаром)
    QWidget* complexField(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    //! Редактор для свойства. В отличие от field может включать составные виджеты (например набор данных с тулбаром)
    QWidget* complexField(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;

    //! Свойство для виджета (если такого нет - DataProperty::isValid() == false)
    DataProperty widgetProperty(const QWidget* widget) const;

    //! Виджет, совмещающий набор данных и его тулбар
    QWidget* datasetGroup(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    QWidget* datasetGroup(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;

    //! Тулбар для набора данных
    QToolBar* datasetToolbar(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    QToolBar* datasetToolbar(const PropertyID& property_id,
                             //! если не найдено - остановка
                             bool halt_if_not_found = true) const;

    //! Виджет, в котором находится тулбар для набора данных
    QFrame* datasetToolbarWidget(const DataProperty& property,
                                 //! если не найдено - остановка
                                 bool halt_if_not_found = true) const;
    QFrame* datasetToolbarWidget(const PropertyID& property_id,
                                 //! если не найдено - остановка
                                 bool halt_if_not_found = true) const;
    //! Установить стиль виджета, в котором находится тулбар для набора данных в соответствии с его параметрами
    static void setDatasetToolbarWidgetStyle(bool has_left_border, bool has_right_border, bool has_top_border, bool has_bottom_border,
                                             QFrame* toolbar_widget);

    //! Показывается ли тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    bool isDatasetToolbarHidden(const DataProperty& property,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    //! Показывается ли тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    bool isDatasetToolbarHidden(const PropertyID& property_id,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;

    //! Скрыть/показать тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    void setDatasetToolbarHidden(const DataProperty& property, bool hidden,
        //! если не найдено - остановка
        bool halt_if_not_found = true);
    //! Скрыть/показать тулбар набора данных. Скрывается не сам тулбар, а его контейнер
    void setDatasetToolbarHidden(const PropertyID& property_id, bool hidden,
        //! если не найдено - остановка
        bool halt_if_not_found = true);

    //! Экшн для набора данных
    QAction* datasetAction(const DataProperty& property, ObjectActionType type,
        //! если не найдено - остановка
        bool halt_if_not_found = true) const;
    QAction* datasetAction(const PropertyID& property_id, ObjectActionType type,
                           //! если не найдено - остановка
                           bool halt_if_not_found = true) const;

    //! Активация экшна набора данных
    bool activateDatasetAction(const DataProperty& property, ObjectActionType type,
                               //! если не найдено - остановка
                               bool halt_if_not_found = true);
    bool activateDatasetAction(const PropertyID& property_id, ObjectActionType type,
                               //! если не найдено - остановка
                               bool halt_if_not_found = true);

    //! Кнопка для экшена набора данных
    QToolButton* datasetActionButton(const DataProperty& property, ObjectActionType type) const;
    QToolButton* datasetActionButton(const PropertyID& property_id, ObjectActionType type) const;

    //! Создать popup меню для экшена набора данных. После вызова этого метода при нажатии на кнопку экшена ее экшен активировать не будет,
    //! вместо этого появится меню с экшенами, переданными в этот метод
    void createDatasetActionMenu(const DataProperty& property, ObjectActionType type, const QList<QAction*>& actions);
    void createDatasetActionMenu(const PropertyID& property_id, ObjectActionType type, const QList<QAction*>& actions);

    //! Сформировать все поля в единый виджет
    QWidget* content() const;

    //! Идет загрузка данных лукап моделей
    bool isLookupLoading() const;

    //! Колонка для индекса
    DataProperty indexColumn(const QModelIndex& index) const;

    //! Текущий активная ячейка (source индекс)
    QModelIndex currentIndex(const DataProperty& dataset_property) const;
    //! Текущий активная ячейка (source индекс)
    QModelIndex currentIndex(const PropertyID& dataset_property_id) const;
    //! Текущий активная ячейка (source индекс). Для сущностей с одним набором данных
    QModelIndex currentIndex() const;

    //! Текущий активная строка (source индекс). Только для плоских таблиц
    //! Если не задана: -1
    int currentRow(const DataProperty& dataset_property) const;
    //! Текущий активная строка (source индекс). Только для плоских таблиц
    //! Если не задана: -1
    int currentRow(const PropertyID& dataset_property_id) const;
    //! Текущий активная строка (source индекс). Для сущностей с одним набором данных. Только для плоских таблиц
    //! Если не задана: -1
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
    bool setCurrentIndex(const DataProperty& dataset_property, const QModelIndex& index,
                         //! Сохранить текущую колонку
                         bool current_column = false) const;
    //! Задать текущий активную ячейку (source или proxy индекс)
    bool setCurrentIndex(const PropertyID& dataset_property_id, const QModelIndex& index,
                         //! Сохранить текущую колонку
                         bool current_column = false) const;
    //! Задать текущий активную ячейку (source или proxy индекс). Для сущностей с одним набором данных
    bool setCurrentIndex(const QModelIndex& index,
                         //! Сохранить текущую колонку
                         bool current_column = false) const;

    //! Интерфейс для получения информации о списке значений PropertyLookup в случае, если не задана сущность выборки
    I_DataWidgetsLookupModels* dynamicLookup() const;

    //! Показать все колонки для набора данных (для отладки)
    void showAllDatasetHeaders(const DataProperty& dataset_property);
    void showAllDatasetHeaders(const PropertyID& dataset_property_id);

    //! Информация о виджете
    struct WidgetInfo
    {
        DataProperty property;
        //! Виджет, созданный на основании свойства
        QPointer<QWidget> field;
        //! был принудительно создан read-only виджет
        bool force_read_only = false;

        //! Виджет, содержащий тулбар набора данных
        QPointer<QFrame> dataset_toolbar_widget;
        //! Тулбар набора данных
        QPointer<QToolBar> dataset_toolbar;
        //! Виджет, совмещающий тулбар и набор данных
        QPointer<QWidget> dataset_group;
        //! Вертикальная линия - разделитель между набором данных и его тулбаром
        QPointer<QFrame> dataset_v_line;
        //! Экшены набора данных
        QMap<ObjectActionType, QAction*> dataset_actions;

        //! Lookup: Модель, откуда брать расшифровку
        ModelPtr lookup_model;
        //! Lookup: Фильтрация при расшифровке
        DataFilterPtr lookup_filter;
        //! Lookup: Какая колонка набора данных dataset отвечает за отображение информации
        DataProperty lookup_column_display;
        //! Lookup: Какая колонка набора данных dataset отвечает за ID
        DataProperty lookup_column_id;
        //! Lookup: Роль для получения расшифровки из lookup_column_display
        int lookup_role_display = -1;
        //! Lookup: Роль для получения id из lookup_column_id
        int lookup_role_id = -1;

        //! Положение курсора
        int cursor_pos = 0;
    };
    //! Создать виджет для свойства. Виджет просто создается и его сигналы не обрабатываются. За удаление и
    //! использование отвечает вызывающий
    std::shared_ptr<WidgetInfo> generateWidget(
        //! Свойство данных
        const DataProperty& property,
        //! Отображать ли рамку
        bool show_frame,
        //! По возможности создать read-only виджет (например вместо QLineEdit - QLabel)
        bool force_read_only,
        //! По возможности создать редактируемый виджет
        bool force_editable) const;
    //! Создать виджет для свойства. Виджет просто создается и его сигналы не обрабатываются. За удаление и
    //! использование отвечает вызывающий
    static std::shared_ptr<WidgetInfo> generateWidget(
        //! Структура данных
        const DataStructure* data_structure,
        //! Свойство данных
        const DataProperty& property,
        //! Отображать ли рамку
        bool show_frame,
        //! По возможности создать read-only виджет (например вместо QLineEdit - QLabel)
        bool force_read_only,
        //! По возможности создать редактируемый виджет
        bool force_editable,
        //! Структура данных
        const DataStructurePtr& structure = nullptr,
        //! Представление (нужно для генерации табличных виджетов)
        const View* view = nullptr,
        //! Интерфейс для получения информации о списке значений PropertyLookup в случае, если не задана сущность выборки
        const I_DataWidgetsLookupModels* dynamic_lookup = nullptr,
        //! Виджеты. Используется если не задано view
        const DataWidgets* widgets = nullptr,
        //! Информация об ошибках. Используется если не задано view
        const HighlightProcessor* highlight = nullptr);
    //! Обновить содержимое виджета в соответствие со значением поля. Используется для виджетов, созданных через
    //! generateWidget
    static void updateWidget(QWidget* widget, const DataProperty& main_property, const QMap<PropertyID, QVariant>& values,
                             UpdateReason reason, int cursor_pos = 0);
    //! Обновить содержимое виджета в соответствие с набором значений. Используется для виджетов, созданных через
    //! generateWidget
    static void updateWidget(QWidget* widget, const DataProperty& property, const QVariantList& values, UpdateReason reason);
    //! Извлечь значение из виджета. Результат может содержать несколько значений, если виджет поддерживает это
    static QMap<PropertyID, QVariant> extractWidgetValues(const DataProperty& p, QWidget* widget,
                                                          //! Интерфейс для получения информации о списке значений PropertyLookup в случае,
                                                          //! если не задана сущность выборки
                                                          const I_DataWidgetsLookupModels* dynamic_lookup = nullptr);
    //! Преобразование значение lookup для записи из данных в ItemSelector
    static QVariant convertToLookupValue(const DataProperty& property, const QVariant& value);
    //! Преобразование значение lookup для записи в данные
    static QVariant convertFromLookupValue(const DataProperty& property, const QVariant& value);

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

    //! Количество зафиксированных групп (узлов верхнего уровня)
    int datasetFrozenGroupCount(const DataProperty& dataset_property) const;
    int datasetFrozenGroupCount(const PropertyID& dataset_property_id) const;
    //! Установить количество зафиксированных групп (узлов верхнего уровня)
    void setDatasetFrozenGroupCount(const DataProperty& dataset_property, int count);
    void setDatasetFrozenGroupCount(const PropertyID& dataset_property_id, int count);

    //! Начать обновление данных
    void beginUpdate(UpdateReason reason);
    //! Закончить обновление данных
    void endUpdate(UpdateReason reason);
    //! В процессе обновления данных
    bool isUpdating() const;

protected:
    //! Фильтр событий
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    //! Начата загрузка lookup моделей
    void sg_beginLookupLoad();
    //! Завершена загрузка lookup моделей
    void sg_endLookupLoad();

    //! Сработал action для тулбара набора данных
    void sg_datasetActionTriggered(const zf::DataProperty& property, zf::ObjectActionType type);

    //! Изменился фокус
    void sg_focusChanged(QWidget* previous_widget, const zf::DataProperty& previous_property, QWidget* current_widget,
                         const zf::DataProperty& current_property);

    //! Изменилась видимость свойства
    void sg_fieldVisibleChanged(const zf::DataProperty& property, QWidget* widget);

    //! Все виджеты были обновлены
    void sg_widgetsUpdated();

private slots:
    //! После установки новой модели (вызывается только при установке модели вне конструктора)
    void sl_afterModelChange(const zf::ModelPtr& old_model, const zf::ModelPtr& new_model);

    //! Обратный вызов
    void sl_callback(int key, const QVariant& data);

    //! Сменился текущий язык данных контейнера
    void sl_languageChanged(QLocale::Language language);

    //! Изменилось значение свойства
    void sl_propertyChanged(const zf::DataProperty& p,
                            //! Старые значение (работает только для полей)
                            const zf::LanguageMap& old_values);

    //! Изменилось значение набора данных
    void sl_dataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                const QVector<int>& roles);
    //! Добавлена строка в набор данных
    void sl_dataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! Удалена строка из набора данных
    void sl_dataset_rowsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! Набор данных сброшен
    void sl_dataset_modelReset(const zf::DataProperty& p);

    //! Сработал action для тулбара набора данных
    void sl_datasetActionTriggered();

    //! Изменилось состояние валидности данных
    void sl_invalidateChanged(const zf::DataProperty& p, bool invalidate);
    //! Свойство было инициализировано
    void sl_propertyInitialized(const zf::DataProperty& p);
    //! Свойство стало неинициализировано
    void sl_propertyUnInitialized(const zf::DataProperty& p);
    //! Все свойства были заблокированы
    void sl_allPropertiesBlocked();
    //! Все свойства были разблокированы
    void sl_allPropertiesUnBlocked();
    //! Свойство было заблокировано
    void sl_propertyBlocked(const zf::DataProperty& p);
    //! Свойство было разаблокировано
    void sl_propertyUnBlocked(const zf::DataProperty& p);

    //! Изменилось значение в ItemSelector
    void sl_ItemSelector_edited(const QModelIndex& index, bool by_keys);
    //! Изменилось значение в ItemSelector в режиме edited
    void sl_ItemSelector_edited(const QString& text);
    //! Изменился текущий выбор для режима SelectionMode::Single
    void sl_ItemSelector_editedMulti(const QVariantList& values);

    //! Изменилось значение в ItemSelector для lookup в режиме List
    void sl_ItemSelectorList_edited(const QModelIndex& index, bool by_keys);

    //! Изменилось значение в RequestSelector
    void sl_RequestSelector_edited(const zf::Uid& key);
    //! Изменилось значение текст в RequestSelector
    void sl_RequestSelector_textEdited(const QString text);    
    //! Изменился набор атрибутов RequestSelector
    void sl_RequestSelector_attributesChanged(bool started_by_user, const QMap<QString, QVariant>& old_attributes,
                                              const QMap<QString, QVariant>& new_attributes);
    //! RequestSelector - поиск начат
    void sl_RequestSelector_searchStarted(bool started_by_user);
    //! RequestSelector - поиск закончен
    void sl_RequestSelector_searchFinished(bool started_by_user, bool cancelled);

    //! Изменилось значение в QLineEdit
    void sl_LineEdit_textEdited(const QString& text);
    //! Изменилось значение в DateEdit
    void sl_DateEdit_dateChanged(const QDate& old_date, const QDate& new_date);
    void sl_DateEdit_textChanged(const QString& old_text, const QString& new_text);
    //! Изменилось значение в QDateTimeEdit
    void sl_DateTimeEdit_dateTimeChanged(const QDateTime& datetime);
    //! Изменилось значение в QTimeEdit
    void sl_TimeEdit_dateTimeChanged(const QTime& time);
    //! Изменилось значение в QCheckBox
    void sl_CheckBox_stateChanged(int state);
    //! Изменилось значение в QPlainTextEdit
    void sl_PlainTextEdit_textChanged();
    //! Изменилось значение в QTextEdit
    void sl_RichTextEdit_textChanged();
    //! Изменена картинка
    void sl_PictureSelectorEdited();

    //! Завершена загрузка модели
    void sl_model_finishLoad(const zf::Error& error,
                             //! Параметры загрузки
                             const zf::LoadOptions& load_options,
                             //! Какие свойства обновлялись
                             const zf::DataPropertySet& properties);

    //! Смена текущей ячейки набора данных
    void sl_currentCellChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Смена фокуса
    void sl_focusChanged(QWidget* prev, QWidget* current);

    //! Начало загрузки lookup модели
    void sl_lookupModelStartLoad();
    //! Завершена загрузка lookup модели
    void sl_lookupModelFinishLoad(const zf::Error& error,
                                  //! Параметры загрузки
                                  const zf::LoadOptions& load_options,
                                  //! Какие свойства обновлялись
                                  const zf::DataPropertySet& properties);

    //! Вызывается до инициации подгонки высоты строк у таблицы
    void sl_beforeResizeRowsToContent();
    //! Вызывается после инициации подгонки высоты строк у таблицы
    void sl_afterResizeRowsToContent();

    //! Смена положения курсора для LineEdit
    void sl_lineEditCursorPositionChanged(int oldPos, int newPos);
    //! Смена положения курсора для DateEdit
    void sl_dateEditCursorPositionChanged(int oldPos, int newPos);

private:
    //! Начальная инициализация
    void bootstrap();
    //! Установить источник данных
    void setDataSource(const DataContainerPtr& data);
    //! Создать единый виджет
    QWidget* createContentWidget() const;

    //! Создать виджеты на основе данных (отложенное создание)
    void generateWidgetsLazy() const;

    //! Создать виджет для свойства
    QWidget* generateWidgetHelper(const DataProperty& property);

    //! Получить информацию о lookup поле/колонке для динамического lookup
    static void getLookupInfo(const I_DataWidgetsLookupModels* dynamic_lookup,
                              //! Поле/колонка, которое имеет lookup
                              const DataProperty& property,
                              //! Модель, откуда брать расшифровку
                              ModelPtr& lookup_model,
                              //! Фильтрация при расшифровке
                              DataFilterPtr& lookup_filter,
                              //! Какая колонка набора данных dataset отвечает за отображение информации
                              DataProperty& lookup_column_display,
                              //! Какая колонка набора данных dataset отвечает за ID
                              DataProperty& lookup_column_id,
                              //! Роль для получения расшифровки из lookup_column_display
                              int& lookup_role_display,
                              //! Роль для получения id из lookup_column_id
                              int& lookup_role_id);
    //! Получить информацию о lookup колонке для статического lookup
    static void getLookupInfo(
        //! Структура данных
        const DataStructure* lookup_data_structure,
        //! Поле/колонка, которая имеет lookup
        const DataProperty& property,
        //! Из какого набора данных берутся значения
        DataProperty& lookup_dataset,
        //! Какая колонка набора данных dataset отвечает за отображение информации
        DataProperty& lookup_column_display,
        //! Какая колонка набора данных dataset отвечает за ID
        DataProperty& lookup_column_id,
        //! Роль для получения расшифровки из lookup_column_display
        int& lookup_role_display,
        //! Роль для получения id из lookup_column_id
        int& lookup_role_id);

    //! Размер секции в пикселах по умолчанию
    int defaultSectionSizePx(const PropertyID& dataset_property_id,
                             //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
                             //! совпадать с кодом колонки
                             int section_id) const;
    //! Размер секции в пикселах по умолчанию
    int defaultSectionSizePx(const DataProperty& dataset_property,
                             //! Идентификатор секции. Если заголовок инициализировался на основании структуры данных, то будет
                             //! совпадать с кодом колонки
                             int section_id) const;
    //! Размер секции в пикселах по умолчанию
    int defaultSectionSizePx(const DataProperty& column_property) const;
    //! Размер секции в символах по умолчанию для указанного типа данных. -1 если не определено
    static int defaultSectionSizeChar(DataType data_type);

    //! Обновить содержимое виджета в соответствие со значением поля
    void updateWidgetHelper(const DataProperty& property, UpdateReason reason, bool ignore_block = false, int cursor_pos = 0);

    //! Обновить состояние RequestSelector
    void updateRequestSelectorWidget(const DataProperty& property, UpdateReason reason);
    //! Обновлить поле свободного ввода, связанное с RequestSelector
    void updateRequestSelectorFreeTextProperty(const DataProperty& property);

    //! Обработка изменения значения поля с датой
    void onDateChanged(QWidget* w);

    //! Установить новое значение свойства после изменения виджета
    void setNewValue(const DataProperty& main_property, const QMap<PropertyID, QVariant>& values,
                     //! Какие свойства надо заблокировать для изменений
                     const PropertyIDList& block_properties);
    //! Установить новые значения свойства после изменения виджета. Для наборов данных PropertyOption::SimpleDataset
    void setNewValueSimpleDataset(const DataProperty& property, const QVariantList& values);

    //! Положение курсора
    int getCursorPos(const DataProperty& property) const;

    //! Убрать у всех наследников QAbstractItemView назначенную на них model
    void disconnectFromItemModels();
    //! Задать для всех наследников QAbstractItemView назначенную на них model (если не задано)
    void connectToItemModels();

    void checkProperty(const DataProperty& p, PropertyType type) const;
    void checkProperty(const DataProperty& p, const QList<PropertyType>& type) const;

    //! Добавить информацию о лукапе
    void addLookupInfo(const DataProperty& property, const ModelPtr& model);
    //! Удалить информацию о лукапе
    void removeLookupInfo(const ModelPtr& model);

    DatabaseID _database_id;
    DataContainerPtr _data;
    View* _view = nullptr;
    HighlightProcessor* _highlight = nullptr;
    I_DataWidgetsLookupModels* _dynamic_lookup = nullptr;
    bool _connected = false;

    //! Фильтр
    DataFilter* _filter = nullptr;

    //! Виджеты, созданные на основании свойств
    QHash<DataProperty, std::shared_ptr<WidgetInfo>> _properties_info;
    WidgetInfo* propertyInfo(const DataProperty& property, bool halt_if_not_found) const;

    bool isBlockUpdateProperty(const DataProperty& property) const;
    bool isBlockUpdateProperty(const PropertyID& property_id) const;
    void blockUpdateProperty(const DataProperty& property) const;
    void blockUpdateProperty(const PropertyID& property_id) const;
    bool unBlockUpdateProperty(const DataProperty& property) const;
    bool unBlockUpdateProperty(const PropertyID& property_id) const;

    //! Поиск свойства по его виджету
    QHash<QWidget*, DataProperty> _properties;

    //! Информация о lookup моделях
    struct LookupInfo
    {
        //! Модель lookup
        ModelPtr lookup;
        //! Счетчик загрузки lookup
        int loading_counter = 0;
        //! Какие свойства относятся к lookup для этой модели
        DataPropertySet properties;
    };
    QMap<Model*, std::shared_ptr<LookupInfo>> _lookup_info_by_model;
    //! Глобальный счетчик загрузки lookup
    int _lookup_loading_counter = 0;

    //! Блокировка обновления поля
    mutable QHash<DataProperty, int> _block_update;
    //! Все поля в одном виджете
    mutable QPointer<QWidget> _content;

    //! Отложенные запросы на обновление виджетов
    QHash<DataProperty, UpdateReason> _update_requests;

    //! Информация о фокусе ввода виджетов
    DataProperty _focus_previous;
    DataProperty _focus_current;
    //! Информация о фокусе ввода ячеек
    DataProperty _focus_cell_previous;
    DataProperty _focus_cell_current;

    //! В процессе обновления данных
    int _updating_counter = 0;
    //! В процессе обновления виджетов
    bool _update_widgets_in_process = false;

    //! Ключ - свойство отвечающее за родителя RequestSelector, значение - свойство RequestSelector
    QMultiMap<DataProperty, DataProperty> _request_selector_parent;

    //! В процессе отложенной генерации виджетов
    mutable bool _lazy_generation = false;

    //! Надо вызывать scrollTo для таблицы после подгонки высоты
    bool _need_resize_scroll_to = false;

    //! Менеджер обратных вызовов
    CallbackManager* _callback_manager = nullptr;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf

#endif // ZF_DATA_WIDGETS_H
