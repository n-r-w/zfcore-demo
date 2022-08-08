#pragma once

#include "zf_data_structure.h"
#include "zf_global.h"
#include "zf_model.h"
#include "zf_line_edit.h"
#include "zf_plain_text_edit.h"
#include "zf_squeezed_label.h"

#include <QFrame>
#include <QListView>
#include <QModelIndex>
#include <QStyle>
#include <QWidget>
#include <QCompleter>

class QAbstractItemModel;
class QAbstractItemView;
class QStyleOptionComboBox;
class QToolButton;
class QAccessibleInterface;

namespace zf
{
class ItemSelectorContainer;
class ItemSelectorEditor;
class ItemSelectorMemo;

/*! Виджет выбора из списка (справочника)
 * Может работать в нескольких режимах:
 * 1. На базе QAbstractItemModel - вообще не связан с моделью данных на базе zf::Model. Отображает только одну колонку в выпадающем списке
 * 2. На базе zf::Model - связан с определенной моделью данных и отслеживает ее перезагрузку для обновления отображаемого значения. 
 *      Отображает только одну колонку в выпадающем списке
 * 3. На базе zf::View - связан с представлением. Работает в режиме модального диалога */
class ZCORESHARED_EXPORT ItemSelector : public QWidget, public I_ObjectExtension
{
    Q_OBJECT

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)
    Q_PROPERTY(int displayColumn READ displayColumn WRITE setDisplayColumn DESIGNABLE true STORED true)
    Q_PROPERTY(int displayRole READ displayRole WRITE setDisplayRole DESIGNABLE true STORED true)
    Q_PROPERTY(int idColumn READ idColumn WRITE setIdColumn DESIGNABLE true STORED true)
    Q_PROPERTY(int idRole READ idRole WRITE setIdRole DESIGNABLE true STORED true)
    Q_PROPERTY(QList<int> multiSelectionLevels READ multiSelectionLevels WRITE setMultiSelectionLevels DESIGNABLE true STORED true)
    Q_PROPERTY(bool matchContains READ isMatchContains WRITE setMatchContains DESIGNABLE true STORED true)
    Q_PROPERTY(bool selectAllOnFocus READ isSelectAllOnFocus WRITE setSelectAllOnFocus DESIGNABLE true STORED true)
    Q_PROPERTY(QString placeholder READ placeholderText WRITE setPlaceholderText DESIGNABLE true STORED true)
    Q_PROPERTY(zf::SelectionMode::Enum selectionMode READ selectionMode WRITE setSelectionMode DESIGNABLE true STORED true)
    Q_PROPERTY(PopupMode popupMode READ popupMode WRITE setPopupMode DESIGNABLE true STORED true)
    Q_PROPERTY(int popupHeightCount READ popupHeightCount WRITE setPopupHeightCount DESIGNABLE true STORED true)
    Q_PROPERTY(bool eraseEnabled READ isEraseEnabled WRITE setEraseEnabled DESIGNABLE true STORED true)
    Q_PROPERTY(bool popupEnter READ isPopupEnter WRITE setPopupEnter DESIGNABLE true STORED true)
    Q_PROPERTY(bool hasFrame READ hasFrame WRITE setFrame DESIGNABLE true STORED true)
    Q_PROPERTY(bool onlyBottomItem READ onlyBottomItem WRITE setOnlyBottomItem DESIGNABLE true STORED true)

public:
    //! Режим работы popup
    enum PopupMode
    {
        //! Выпадающий список
        PopupComboBox,
        //! Модальный диалог
        PopupDialog,
    };
    Q_ENUM(PopupMode)

    explicit ItemSelector(QWidget* parent = nullptr);
    //! Инициализация данными из QAbstractItemModel (режим ItemModel)
    explicit ItemSelector(
        //! Данные
        QAbstractItemModel* itemModel, QWidget* parent = nullptr);
    //! Инициализация данными из Model (режим DataModel)
    explicit ItemSelector(
        //! Модель
        const ModelPtr& model,
        //! Свойство - набор данных
        const DataProperty& dataset,
        //! Фильтр
        const DataFilterPtr& data_filter = nullptr, QWidget* parent = nullptr);
    //! Асинхронная загрузка в режиме DataModel
    explicit ItemSelector(
        //! Идентификатор модели
        const Uid& entity_uid,
        //! Идентификатор свойства - набора данных
        const PropertyID& dataset_id, QWidget* parent = nullptr);

    ~ItemSelector() override;

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
    //! Режим работы popup
    ItemSelector::PopupMode popupMode() const;
    //! Задать режим работы popup
    void setPopupMode(ItemSelector::PopupMode m);

    //! Режим выбора
    SelectionMode::Enum selectionMode() const;
    //! Задать режим выбора
    void setSelectionMode(SelectionMode::Enum m);

    //! Для деревьев - выбор только узла нижнего уровня. Работает только для PopupMode::PopupDialog
    bool onlyBottomItem() const;
    //! Для деревьев - выбор только узла нижнего уровня. Работает только для PopupMode::PopupDialog
    void setOnlyBottomItem(bool b);

    //! Текущее состояние
    enum Status
    {
        //! Обычное состояние
        Ready,
        //! Поиск в данных выпадающего списка
        Searching,
    };
    //! Текущее состояние
    Status status() const;

    //! Данные
    QAbstractItemModel* itemModel() const;
    //! Инициализация данными из QAbstractItemModel
    void setItemModel(QAbstractItemModel* model);

    //! Модель
    ModelPtr model() const;
    //! Фильтр
    DataFilterPtr filter() const;
    //! Свойство - набор данных
    DataProperty datasetProperty() const;

    //! Имеется ли такое ключевое значение в фильтре
    bool hasFilteredValue(const QVariant& v) const;

    //! Установить источник lookup
    void setModel(
        //! Модель
        const ModelPtr& model,
        //! Свойство - набор данных. Если не задано, то предполагается что
        //! либо model == nullptr, либо модель имеет только один набора данных
        const DataProperty& dataset = {},
        //! Данные
        const DataFilterPtr& data_filter = nullptr);
    //! Установить источник lookup. Асинхронная загрузка
    void setModel(
        //! Идентификатор модели
        const Uid& entity_uid,
        //! Идентификатор свойства - набора данных. Если не задан, то предполагается что
        //! либо entity_uid.isValid() == false, либо модель имеет только один набора данных
        const PropertyID& dataset_id = {});

    //! Задать колонку для отображения (по умолчанию - 0)
    void setDisplayColumn(int c);
    //! Задать колонку для отображения (только при работе через Model)
    void setDisplayColumn(const DataProperty& column);
    //! Колонка для отображения (по умолчанию - 0)
    int displayColumn() const;

    //! Роль для отображения данных (по умолчанию Qt::DisplayRole)
    int displayRole() const;
    void setDisplayRole(int role);

    //! Отображаемый текст (то что видит пользователь)
    QString displayText() const;

    //! Задать колонку в которой хранятся коды (по умолчанию совпадает с displayColumn)
    void setIdColumn(int c);
    //! Задать колонку в которой хранятся коды. По умолчанию совпадает с displayColumn (только при работе через Model)
    void setIdColumn(const DataProperty& column);
    //! Колонка в которой хранятся коды (по умолчанию совпадает с displayColumn)
    int idColumn() const;

    //! Роль для колонки кодов (по умолчанию совпадает с displayRole)
    int idRole() const;
    void setIdRole(int role);

    //! Режим, в котором можно вводить произвольный текст, не совпадающий с выпадающим списком
    bool isEditable() const;
    void setEditable(bool b);

    //! Задать текущий текст (только для isEditable() == true)
    void setCurrentText(const QString& s);
    //! Текущий текст
    QString currentText() const;

    //! Задать уровни на которых можно выделять элементы (только для SelectionMode::Multi)
    //! Имеет смысл для выборки из иерархических наборов данных
    void setMultiSelectionLevels(const QList<int>& levels);
    //! Уровни на которых можно выделять элементы (только для SelectionMode::Multi)
    //! Имеет смысл для выборки из иерархических наборов данных
    QList<int> multiSelectionLevels() const;

    //! Текущий выбор (для режима SelectionMode::Single)
    QModelIndex currentIndex() const;
    //! Текущая строка (для режима SelectionMode::Single)
    int currentRow() const;
    //! Текущий выборанное значение в колонке id (для режима SelectionMode::Single)
    QVariant currentValue() const;

    //! Позиционировать по значению (для режима SelectionMode::Single)
    void setCurrentValue(const QVariant& value,
        //! По какой колонке данных (-1 : по колонке ID)
        int column = -1,
        //! По какой роли (-1 : по роли ID)
        int role = -1);
    //! Позиционировать по значению (для режима SelectionMode::Single)
    void setCurrentValue(const QVariant& value,
        //! По какой колонке данных (только если источник данных задан через ModelPtr)
        const DataProperty& column,
        //! По какой роли (-1 : по роли ID)
        int role = -1);

    //! Выбранные элементы в колонке id (для режима SelectionMode::Multi)
    QVariantList currentValues() const;
    //! Задать выбранные элементы по значению, поиск по колонке id (для режима SelectionMode::Multi)
    //! Возвращает истину если значения изменились
    bool setCurrentValues(const QVariantList& values);

    //! Поиск по подстроке в любом месте (по умолчанию ищет по первым символам)
    bool isMatchContains() const;
    void setMatchContains(bool b);

    //! Найдено ли текущее значение в lookup (для режима SelectionMode::Single)
    bool isCurrentValueFound() const;
    //! Задано ли текущее значение в lookup (для режима SelectionMode::Single)
    bool hasCurrentValue() const;

    //! Очистить текущий выбор
    void clear();

    //! Текст в редакторе при отсутствии значения
    QString placeholderText() const;
    //! Задать текст в редакторе при отсутствии значения
    void setPlaceholderText(const QString& s);

    //! Задать текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    void setOverrideText(const QString& text);

    //! Размер popup: множитель к высоте ItemSelector
    int popupHeightCount() const;
    void setPopupHeightCount(int count);

    //! Включить или отключить кнопку очистки (не имеет смысла в режиме readOnly)
    void setEraseEnabled(bool b);
    //! Включена ли кнопка очистки (не имеет смысла в режиме readOnly)
    bool isEraseEnabled() const;

    //! Разрешен ли переход по нажатию на кнопку GOTO и на какую сущность
    Uid gotoEntity() const;

    //! Заблокирован на изменения
    bool isReadOnly() const;
    void setReadOnly(bool b);

    //! Проверка одновременно на isEnabled, isReadOnly все прочее что может повлиять на
    //! возможность редактирования
    bool isFullEnabled() const;

    //! Выдалять ли текст при фокусе
    bool isSelectAllOnFocus() const;
    void setSelectAllOnFocus(bool b);

    //! Открывать ли popup при нажатии enter
    bool isPopupEnter() const;
    void setPopupEnter(bool b);

    //! Показать или скрыть рамку
    void setFrame(bool b);
    //! Имеет рамку
    bool hasFrame() const;

    //! Максимальный размер кнопок
    int maximumButtonsSize() const;
    void setMaximumButtonsSize(int n);

    //! Максимальное количество строк текста, после которых начинается скроллинг. Только для режима SelectionMode::Multi
    int maximumLines() const;
    //! Максимальное количество строк текста, после которых начинается скроллинг. Только для режима SelectionMode::Multi
    void setMaximumLines(int n);

    //! Разрешать ли выбор пустого значения. Только для режима SelectionMode::Multi
    bool allowEmptySelection() const;
    //! Разрешать ли выбор пустого значения. Только для режима SelectionMode::Multi
    void setAllowEmptySelection(bool b);

    //! Заголовок диалога (по умолчанию не задано)
    QString dialogTitle() const;
    //! Заголовок диалога
    void setDialogTitle(const QString& s);

    //! Открыто ли выпадающее окно
    bool isPopup() const;
    //! Открыть выпадающее окно
    Q_SLOT void popupShow();
    //! Скрыть выпадающее окно
    Q_SLOT void popupHide(bool apply);
    //! Отобразить диалог выбора
    Q_SLOT void dialogShow(const QString& filter_text);

    //! Представление выпадающего окна. Может быть наследником QListView или QTreeView в зависимости от типа данных (DataType::Tree/Table)
    QAbstractItemView* popupView() const;

    //! Идет асинхронная загрузка модели с данными
    bool isAsyncLoading() const;

    //! Кнопка очистки
    QToolButton* eraseButton() const;
    //! Кнопка перехода
    QToolButton* gotoButton() const;
    //! Активный редактор
    QWidget* editor() const;

protected:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void wheelEvent(QWheelEvent* e) override;

signals:
    //! Изменился текущий выбор (действие пользователя и программно) для режима SelectionMode::Single
    void sg_changed(const QModelIndex& index);
    //! Изменился текущий выбор (действие пользователя) для режима SelectionMode::Single
    void sg_edited(const QModelIndex& index,
        //! Изменения сделаны путем набора текста
        bool by_keys);

    //! Изменился текущий выбор (действие пользователя и программно) для режима SelectionMode::Single
    //! Генерируется когда isEditable()==true и программно изменен текст в редакторе. При этом текст не совпал с lookup
    void sg_changed(const QString& text);
    //! Изменился текущий выбор (действие пользователя) для режима SelectionMode::Single
    //! Генерируется когда isEditable()==true и пользователь изменил текст в редакторе. При этом текст не совпал с lookup
    void sg_edited(const QString& index);

    //! Изменился текущий выбор (действие пользователя и программно) для режима SelectionMode::Multi
    void sg_changedMulti(const QVariantList& values);
    //! Изменился текущий выбор (действие пользователя) для режима SelectionMode::Multi
    void sg_editedMulti(const QVariantList& values);

    //! Выпадающий список открылся
    void sg_popupOpened();
    //! Выпадающий список закрылся
    void sg_popupClosed(
        //! Закрыто с подтверждением
        bool applied);

    //! Завершена асинхронная загрузка модели с данными
    void sg_lookupModelLoaded();
    //! Изменились данные в lookup модели
    void sg_lookupModelChanged();

    //! Изменился режим popup
    void sg_popupModeChanged();
    //! Изменился режим выбора
    void sg_selectionModeChanged();

    //! Изменилось текущее состояние
    void sg_statusChanged(ItemSelector::Status status);

    //! Изменился отображаемый текст. Вызывается при любых изменениях
    //! Тут речь не о том что видит пользователь (игнорируется отображение overrideText и ошибок), а о том, что имеет
    //! смысл програмно забирать из ItemSelector для переноса в поля расшифровки
    void sg_displayTextChanged(const QString& display_text);
    //! Изменился отображаемый текст. Вызывается при любых изменениях пользователем
    //! Тут речь не о том что видит пользователь (игнорируется отображение overrideText и ошибок), а о том, что имеет
    //! смысл програмно забирать из ItemSelector для переноса в поля расшифровки
    void sg_displayTextEdited(const QString& display_text);

    //! Изменилось состояние readOnly
    void sg_readOnlyChanged();

private slots:
    //! Нажата кнопка стирания
    void sl_erase();
    //! Нажата кнопка перехода
    void sl_goto();
    //! Изменился текст поиска (пользователь)
    void sl_editTextEdited(const QString& s);
    //! Модель удалена
    void sl_modelDestroyed();
    //! Изменились данные в модели
    void sl_modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());
    void sl_modelRowsRemoved(const QModelIndex& parent, int first, int last);
    void sl_modelColumnsRemoved(const QModelIndex& parent, int first, int last);
    void sl_modelReset();
    void sl_modelRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
    void sl_modelColumnsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int displayColumn);

    //! zf::Model - загрузился асинхронно
    void sl_model_finishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);
    //! zf::Model - Изменилось состояние валидности данных
    void sl_model_invalidateChanged(const zf::DataProperty& p, bool invalidate);

    //! Обновить доступность кнопок
    void sl_updateEnabled();

    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

    //! Сработал таймер накопления фильтрации в режиме View
    void sl_filter_timeout();

    //! Обратный вызов
    void sl_callback(int key, const QVariant& data);

    //! Рефильтрация
    void sl_refiltered(
        //! Набор данных
        const zf::DataProperty& dataset_property);

private:
    void init();

    //! Выпадающий контрол
    QAbstractItemView* view() const;

    //! Редактор для режима SelectionMode::Single
    LineEdit* editorSingle() const;
    //! Редактор для режима SelectionMode::Multi
    PlainTextEdit* editorMulti() const;
    //! Редактор для режима SelectionMode::Multi, но при заданном ограничении в одну строку
    SqueezedTextLabel* editorMultiOneLine() const;

    //! Текст в текущем редакторе
    QString editorText() const;

    //! Задать текущий выбор. Возвращает истину если было изменение (для режима SelectionMode::Single)
    bool setCurrentIndex(const QModelIndex& index, bool by_user, bool by_keys, bool clear);

    //! Очистить текущий выбор
    void clearHelper(bool by_user);

    //! Позиционировать по значению (для режима SelectionMode::Single)
    void setCurrentValueHelper(const QVariant& value,
        //! По какой колонке данных (-1 : по колонке ID)
        int column,
        //! По какой роли (-1 : по роли ID)
        int role,
        //! Действия пользовател
        bool by_user);

    //! Инициализация данными из QAbstractItemModel
    void setItemModelHelper(QAbstractItemModel* model);

    void initStyleOption(QStyleOptionComboBox* option) const;
    //! Позиционирование внутренних контролов
    void updateControlsGeometry();

    void updateText(bool by_user);
    void updateTextHelper(bool by_user);

    //! какой текст должен быть виден с программной точки зрения
    QString calcDisplayText() const;

    //! Обновить доступность кнопок
    void updateEnabledHelper();

    void updateExactValue(bool by_user);
    void lookupToExactValue(bool by_user);

    //! Области кнопок и редактора
    QRect arrowRect() const;
    QRect editorRect() const;
    QRect eraseRect() const;
    QRect gotoRect() const;
    QRect waitMovieRect() const;

    //! Размеры кнопок и редактора
    QSize arrowSizeHint() const;
    //! Размер редактора. Это внутренний размер, без учета отступов
    QSize editorSizeHint() const;
    QSize eraseSizeHint() const;
    QSize gotoSizeHint() const;

    //! Ширина стрелки
    int arrowWidth() const;

    bool isArrowPos(const QPoint& pos) const;
    bool isErasePos(const QPoint& pos) const;
    bool isGotoPos(const QPoint& pos) const;

    void updateArrow(QStyle::StateFlag state);
    bool updateHoverControl(const QPoint& pos);
    QStyle::SubControl newHoverControl(const QPoint& pos);

    void popupClicked();

    int pageStep() const;
    void scrollPageUp();
    void scrollPageDown();
    void scrollUp();
    void scrollDown();
    void itemUp();
    void itemDown();

    // для дерева скрываем все колонки, кроме основной
    void updateTreeViewColumns();
    void setView(QAbstractItemModel* model);
    ItemSelectorContainer* container() const;

    //! Режим работы popup
    ItemSelector::PopupMode popupModeInternal() const;
    void updateEditorReadOnly();

    //! Поиск в lookup модели
    QModelIndex search(const QString& s, int displayColumn, int displayRole,
        //! Точное совпадение
        bool exact, QAbstractItemModel* model = nullptr) const;

    enum class EditorMode
    {
        LineEdit,
        Memo,
        Label
    };

    //! Режим встроенного редактора
    EditorMode editorMode() const;

    //! Подготовить редактор в соответствии с текущими параметрами
    void prepareEditor();
    //! Обновить видимость редакторов
    void updateEditorVisibility();

    //! Начало поиска
    void beginSearch();
    //! Отмена текущего поиска
    void cancelSearch();
    //! Конец поиска
    void endSearch();
    //! Отобразить состояние поиска
    void showSearchStatus();
    //! Скрыть состояние поиска
    void hideSearchStatus();

    //! Обновить размер и положение окна popup
    void updatePopupGeometry();

    //! В процессе удаления
    bool _destroing = false;
    //! Режим "только для чтения"
    bool _read_only = false;

    //! Режим работы popup
    PopupMode _popup_mode = PopupMode::PopupComboBox;
    //! Режим выбора
    SelectionMode::Enum _selection_mode = SelectionMode::Single;
    //! Для деревьев - выбор только узла нижнего уровня. Работает только для PopupMode::PopupDialog
    bool _only_bottom_item = false;

    //! QAbstractItemModel для отображения
    QAbstractItemModel* _model = nullptr;
    //! Модель для отображения
    ModelPtr _model_ptr;
    //! Представление для отображения
    ViewPtr _view_ptr;
    //! Фильтрация данных
    DataFilterPtr _data_filter;
    //! Набор данных для отображения из _model_ptr
    DataProperty _dataset;
    //! Идентификатор модели для отображения
    Uid _entity_uid;
    //! Идентификатор набора данных в lookup модели
    PropertyID _dataset_id;

    //! Ошибка загрузки набора данных для выборки
    Error _error_loading;

    //! Показывать ли рамку
    bool _has_frame = true;
    //! Включена ли кнопка очистки
    bool _is_erase_enabled = true;
    //! Выдалять ли текст при фокусе
    bool _select_all_on_focus = true;
    //! Открывать ли popup при нажатии enter
    bool _is_popup_enter = true;
    //! Режим ввода произвольного текста
    bool _editable = false;
    //! Максимальный размер кнопок
    int _maximum_buttons_size = -1;
    //! Разрешать ли выбор пустого значения
    bool _allow_empty_selection = true;
    //! Заголовок диалога
    QString _dialog_title;

    //! Контейнер выпадающего списка
    mutable QPointer<ItemSelectorContainer> _container;

    //! Встроенный редактор в режиме Single
    ItemSelectorEditor* _default_editor = nullptr;
    //! Встроенный контрол  в режиме Multi
    ItemSelectorMemo* _multiline_editor = nullptr;
    //! Встроенный контрол в режиме Multi, но при ограничении на отображение одной строки
    SqueezedTextLabel* _multiline_single_editor = nullptr;

    //! Кнопка стирания
    QToolButton* _erase_button = nullptr;
    //! Кнопка перехода
    QToolButton* _goto_button = nullptr;
    //! Таймер для обновления доступности контролов
    QTimer* _update_enabled_timer = nullptr;
    //! Для автозавершения ввода
    QCompleter* _completer = nullptr;

    //! Можно ли перейти по кнопке goto и на какую сущность
    mutable std::unique_ptr<Uid> _goto_entity;

    //! Колонка для отображения расшифровки значения
    int _display_column = 0;
    //! Роль для выбора расшифровки значения
    int _display_role = Qt::DisplayRole;

    //! Колонка для кодов
    int _id_column = -1;
    //! Роль для кодов
    int _id_role = -1;

    //! Уровни на которых можно выделять элементы (только для SelectionMode::Multi)
    //! Имеет смысл для выборки из иерархических наборов данных
    QList<int> _multi_selection_levels;

    //! Сколько строк выпадающего списка отображать
    int _view_height_count = 10;
    //! Насколько менять стандартную ширину выпадающего списка. (1 - 100%)
    qreal _view_width_count = 1;
    //! Правило поиска расшифровки
    Qt::MatchFlags _match = Qt::MatchStartsWith;

    //! Какое значение было выставлено через setCurrentValue
    QVariantPtr _exact_value;
    //! По какой колонке данных (-1 : по колонке отображения)
    int _exact_value_column = -1;
    //! По какой роли (-1 : по роли отображения)
    int _exact_value_role = -1;
    //! Действие пользователя
    bool _exact_value_by_user = false;

    //! Найдено ли _exact_value в lookup
    enum class ExactValueStatus
    {
        Undefined,
        Found,
        NotFound,
        Error,
    };
    ExactValueStatus _exact_value_found = ExactValueStatus::Undefined;

    //! Отложенное обновление текста в редакторе
    QTimer* _update_text_timer = nullptr;
    bool _update_text_by_user = false;

    //! Текст в режиме editable
    QString _editable_text;

    //! В процессе асинхронной загрузки lookup
    bool _async_loading = false;

    //! Последний выбранный индекс
    QModelIndex _last_selected;
    //! Признак поглощения фокуса. Извращения, скопированные из реализации стандартного комбобокса Qt
    bool _eat_focus_out = true;
    //! Таймер для игнорирования нажатия на кнопку выпадающего списка
    QTimer* _ignore_popup_click_timer = nullptr;

    //! Выпадающий список активен
    bool _popup = false;
    //! В процессе изменения видимости выпадающего списка
    bool _popup_visible_chinging = false;

    //! Счетчик блокировки обновления текста
    int _block_update_text = 0;
    bool _block_update_text_helper = false;

    //! Запоминание области hoover. Взято из реализации стандартного комбобокса Qt
    QRect _hover_rect;
    QStyle::SubControl _hover_control = QStyle::SC_None;
    QStyle::StateFlag _arrow_state = QStyle::State_None;

    //! Таймер для набора текста фильтра перед отображением модального диалога в режиме View
    QTimer* _filter_timer = nullptr;
    //! Накапливаемый текст фильтра перед отображением модального диалога в режиме View
    QString _filter_text;

    //! Какие значения были выставлены через setCurrentValues
    QVariantList _current_values;
    //! Какие из _current_values были найдены и какой у них текст
    QList<QStringPtr> _current_values_text;

    //! Текущее состояние
    Status _status = Status::Ready;

    //! Текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    QString _override_text;

    //! Видимый пользователю текст
    QString _display_text;

    //! Подключение к SelectionModel
    QMetaObject::Connection _selection_model_handle;

    //! Индекс до открытия popup
    std::shared_ptr<QModelIndex> _before_popup_index;

#if 0 // не используем, но пусть будет  
    //! Анимация ожидания (виджет)
    QLabel* _wait_movie_label = nullptr;
#endif

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    using QWidget::setEnabled;
    Q_DISABLE_COPY(ItemSelector)

    friend class Framework;
    friend class ItemSelectorContainer;
    friend class ItemSelectorListView;
    friend class ItemSelectorTreeView;
    friend class ItemSelectorEditor;
    friend class ItemSelectorAccessibilityInterface;
};

} // namespace zf
