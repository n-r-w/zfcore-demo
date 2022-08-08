#pragma once

#include "zf_global.h"
#include "zf_error.h"
#include "zf_message.h"
#include "zf_defs.h"
#include "zf_external_request.h"
#include "zf_object_extension.h"

#include <QStyle>

class QLineEdit;
class QStyleOptionComboBox;
class QLabel;
class QListView;

namespace zf
{
class RequestSelectorItemModel;
class RequestSelectorContainer;
class RequestSelectorEditor;

/*! Выбор из списка, который получается в результате запроса к внешнему сервису */
class ZCORESHARED_EXPORT RequestSelector : public QWidget, public I_ObjectExtension
{
    Q_OBJECT

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)
    Q_PROPERTY(QString requestType READ requestType WRITE setRequestType DESIGNABLE true STORED true)
    Q_PROPERTY(RequestSelector::TextMode textMode READ textMode WRITE setTextMode DESIGNABLE true STORED true)
    Q_PROPERTY(QString placeholder READ placeholderText WRITE setPlaceholderText DESIGNABLE true STORED true)

public:
    //! Текущее состояние
    enum Status
    {
        Undefined,
        //! Не задано текущее значение или не задан делегат
        Invalid,
        //! Значение найдено
        Found,
        //! Значение не найдено
        NotFound,
        //! В процессе запроса к сервису на основании ключевого значения
        RequestingKey,
        //! В процессе запроса к сервису на основании текстового значения
        RequestingValue,
    };

    //! Режим доступности ввода текста
    enum TextMode
    {
        //! Требует чтобы I_ExternalRequest::canEdit было всегда истинно.
        //! Например для сервиса ФИАС это означает, что улицу без указания города вводить нельзя
        Strict,
        //! Можно вводить как текст даже если I_ExternalRequest::canEdit вернул ложь
        Combined,
        //! Можно вводить свободный текст, запросы к I_ExternalRequest не отправляются
        Free,
    };

    explicit RequestSelector(QWidget* parent = nullptr);
    RequestSelector(
        //! Интерфейс для установки делегата со спецификой внешнего сервиса
        I_ExternalRequest* delegate, QWidget* parent = nullptr);
    RequestSelector(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const Uid& request_service_uid, QWidget* parent = nullptr);
    RequestSelector(
        //! Идентификатор сущности, реализующей интерфейс для запросов к внешнему сервису
        const EntityCode& request_service_entity_code, QWidget* parent = nullptr);
    ~RequestSelector();

    //! Текущее состояние
    Status status() const;

    //! Задать интерфейс для установки делегата со спецификой внешнего сервиса
    void setDelegate(I_ExternalRequest* delegate);

    //! Размер popup: множитель к высоте RequestSelector
    int popupHeightCount() const;
    void setPopupHeightCount(int count);

    //! В состоянии только для чтения
    bool isReadOnly() const;
    void setReadOnly(bool b);

    //! Текст в редакторе при отсутствии значения
    QString placeholderText() const;
    //! Задать текст в редакторе при отсутствии значения
    void setPlaceholderText(const QString& s);

    //! Задать текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    void setOverrideText(const QString& text);

    //! Проверка одновременно на readOnly все прочее что может повлиять на
    //! возможность редактирования
    bool isEditable() const;

    //! Активен выпадающий список
    bool isPopup() const;

    /*! Задать текущий ключ
     * Если такой ключ существует в ранее найденных, то он выбирается текущим. Если нет - запускается поиск */
    void setCurrentID(const Uid& key);
    void setCurrentID(const QString& key);
    void setCurrentID(int key);
    //! Текущий ключ
    Uid currentID() const;
    QString currentID_string() const;
    int currentID_int() const;

    //! Набор аттрибутов для текущего выбора. Зависит от специфики внешнего сервиса
    const QMap<QString, QVariant>& attributes() const;

    //! Текущий текст в редакторе
    QString currentText() const;

    /*! Поиск по строке.Устанавливает новый текущий текст в поле редактора
     * Возвращает false если текст не подходит для поиска и поиск не был запущен
     * Метод асинхронный */
    bool search(const QString& text);

    //! Тип запроса (зависит от специфики внешнего сервиса:
    //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
    bool setRequestType(const QString& request_type);
    QString requestType() const;

    //! Задать параметры запроса (зависит от специфики внешнего сервиса)
    void setRequestOptions(const QVariant& o);
    //! Параметры запроса (зависит от специфики внешнего сервиса)
    QVariant requestOptions() const;

    //! Задать ключ родительского объекта. Например для поиска улицы в рамках города - это код города и т.п.)
    //! Возвращает истину если было изменение
    bool setParentKeys(
        //! Родительский ключ. Например для поиска улицы в рамках города - это код города и т.п.
        const UidList& parent_keys);
    //! Задать ключ родительского объекта. Например для поиска улицы в рамках города - это код города и т.п.)
    //! Возвращает истину если было изменение
    bool setParentKeys(
        //! Родительский ключ. Например для поиска улицы в рамках города - это код города и т.п.
        const QVariantList& parent_keys);
    //! Задать ключ родительского объекта. Например для поиска улицы в рамках города - это код города и т.п.)
    //! Возвращает истину если было изменение
    bool setParentKeys(
        //! Родительский ключ. Например для поиска улицы в рамках города - это код города и т.п.
        const QStringList& parent_keys);
    //! Задать ключ родительского объекта. Например для поиска улицы в рамках города - это код города и т.п.)
    //! Возвращает истину если было изменение
    bool setParentKeys(
        //! Родительский ключ. Например для поиска улицы в рамках города - это код города и т.п.
        const QList<int>& parent_keys);

    //! Родительские ключи. Например для поиска улицы в рамках города - это код города и т.п.
    UidList parentKeys() const;

    //! Установить дополнительный фильтр, который будет присоединен к строке поиска
    void setFilterText(const QString& s);
    //! Дополнительный фильтр, который будет присоединен к строке поиска
    QString filterText() const;

    //! Очистить
    void clear(
        //! Если задано, то кроме стирания ID, устанавливает текст в поле ввода. Поиск по тексту не прозводится
        const QString& text = {});

    //! Режим ввода текста
    TextMode textMode() const;
    void setTextMode(TextMode mode);

    //! Очищать содержимое при потере фокуса, если ID не выбран
    bool isClearOnExit() const;
    void setClearOnExit(bool b);

    //! Режим "комбобокса". Показывает кнопку комбобокса и по нажатию на нее производит поиск
    //! Если задан ID, то выводит все допустимые значения
    void setComboboxMode(bool b);
    bool isComboboxMode() const;

    //! Редактор
    QLineEdit* editor() const;

    //! Максимальный размер встроенных элементов (крутилка и т.п.)
    int maximumControlsSize() const;
    void setMaximumControlsSize(int n);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    //! Обновить доступность и видимость контролов
    //! Вызов может потребоваться, если меняется поведение сервиса в методе I_ExternalRequest::canEdit, но данный виджет об этом не знает
    Q_SLOT void updateEnabled();

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

protected:
    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

signals:
    //! Изменилось текущее значение (программно или действия пользователя)
    void sg_changed(const zf::Uid& key);
    //! Изменилось текущее значение (действия пользователя)
    void sg_edited(const zf::Uid& key);

    //! Поменялся введенный текст (программно или действия пользователя)
    void sg_textChaged(const QString& text);
    //! Поменялся введенный текст (действия пользователя)
    void sg_textEdited(const QString& text);

    //! Получены и обработаны данные от внешнего сервиса
    void sg_dataReceived(bool started_by_user);

    //! Изменился набор атрибутов. old_attributes и new_attributes могут совпадать, т.к. их может быть много и нет смысла делать полное сравнение
    void sg_attributesChanged(bool started_by_user, const QMap<QString, QVariant>& old_attributes,
                              const QMap<QString, QVariant>& new_attributes);

    //! Поиск начат
    void sg_searchStarted(bool started_by_user);
    //! Поиск закончен
    void sg_searchFinished(bool started_by_user, bool cancelled);

    //! Изменился статус
    void sg_statusChanged(RequestSelector::Status status);

    //! Выпадающий список открылся
    void sg_popupOpened();
    //! Выпадающий список закрылся
    void sg_popupClosed(
        //! Закрытие с выбором значения
        bool applied);

private slots:
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Объект, реализующий I_ExternalRequest был удален
    void sl_delegateObjectDestroyed(QObject* obj);
    //! Изменился текст поиска (пользователь или программно)
    void sl_editTextChanged(const QString& s);
    //! Изменился текст поиска (пользователь)
    void sl_editTextEdited(const QString& s);    
    //! Отработал таймер задержки на изменение текста пользователем
    void sl_textEditTimeout();

    //! Начало обновления внутренних данных выпадающего списка
    void sl_onBeginInternalUpdateData();
    //! Окончание обновления внутренних данных выпадающего списка
    void sl_onEndUpdateInternalData(
        //! Текущая, выделенная строки
        int key_row);

private:
    //! Начальная инициализация
    void init();
    void initStyleOption(QStyleOptionComboBox* option) const;

    RequestSelectorContainer* container() const;
    QListView* view() const;

    //! Задать текущий ключ. Возвращает истину, если были изменения
    bool setCurrentID_helper(const Uid& key);
    //! Обновить текущие атрибуты
    void updateCurrentAttributes(bool by_user);

    //! Обновить доступность и видимость контролов
    void updateEnabledHelper();

    //! Позиционирование внутренних контролов
    void updateControlsGeometry();

    //! Обновить текст лукапа. Возвращает false, если данные не найдены и надо делать новый запрос
    //! Никаких запросов не выполняет, только обновление текста
    bool updateLookupText();

    //! Поиск по текущему ключу
    void keySearch();
    //! Поиск по тексту. Возвращает false если текст не подходит для поиска и поиск не был запущен
    bool textSearch(const QString& text, bool clear_id);
    //! Отмена поиска
    void cancelSearch();

    bool setEditorTextHelper(const QString& text);

    //! Поиск завершен и получен ответ
    void onSearchFeedback(const Message& message);

    //! Задать текущий статус
    void setStatus(Status status);

    //! Скрыть выпадающее список
    void popupHide(bool apply);
    //! Отобразить выпадающий список
    void popupShow();
    //! Обновить размер и положение выпадающего списка. false, если ошибка
    bool updatePopupGeometry();

    //! Перемещение курсора выпадающего списка
    int pageStep() const;
    void scrollPageUp();
    void scrollPageDown();
    void scrollUp();
    void scrollDown();
    void itemUp();
    void itemDown();

    //! Область редактора
    QRect editorRect() const;
    //! Область отрисовки индикации ожидания
    QRect waitMovieRect() const;
    //! Область кнопки очистки
    QRect clearButtonRect() const;
    //! Область кнопки "комбобокса"
    QRect arrowRect() const;

    //! Сдвиг контролов влево, в зависимости от видимости кнопки комбобокса
    int arrowButtonShift() const;
    //! Находится ли точка в области стрелки комбобокса
    bool isArrowPos(const QPoint& pos) const;
    //! Можно ли нажимать на кнопку комбобокса
    bool isAllowPressArrow() const;

    //! Отобразить состояние поиска
    void showSearchStatus();
    //! Скрыть состояние поиска
    void hideSearchStatus();

    //! Запуск таймера для начала запроса
    void startRequestTimer();

    void updateArrow(QStyle::StateFlag state);
    bool updateHoverControl(const QPoint& pos);
    QStyle::SubControl newHoverControl(const QPoint& pos);

    //! Обновить текущий выбор в выпадающем списке
    void updateViewCurrent();

    //! Очистить
    void clearHelper(
        //! Если задано, то кроме стирания ID, устанавливает текст в поле ввода. Поиск по тексту не прозводится
        const QString& text,
        //! Действие пользователя
        bool by_user);

    void updateInternalDataFinish();

    //! Текущее состояние
    Status _status = Invalid;

    //! Интерфейс для установки делегата со спецификой внешнего сервиса
    I_ExternalRequest* _delegate = nullptr;

    //! Данные
    RequestSelectorItemModel* _data = nullptr;
    //! Атрибуты. Количество и порядок совпадает с _data. Содержит произвольный набор пар ключ/значение для каждой строки в result
    QList<QMap<QString, QVariant>> _attributes;

    //! Встроенный редактор
    RequestSelectorEditor* _editor = nullptr;
    //! Контейнер выпадающего списка
    mutable QPointer<RequestSelectorContainer> _container;

    //! Таймер задержки реакции на изменение текста пользователем
    QTimer* _text_edited_timer = nullptr;

    //! В процессе удаления
    bool _destroing = false;
    //! Режим "только для чтения"
    bool _read_only = false;
    //! Выпадающий список активен
    bool _popup = false;
    //! В процессе изменения видимости выпадающего списка
    bool _popup_visible_chinging = false;
    //! Признак поглощения фокуса. Извращения, скопированные из реализации стандартного комбобокса Qt
    bool _eat_focus_out = true;
    //! Таймер для игнорирования нажатия на кнопку выпадающего списка
    QTimer* _ignore_popup_click_timer = nullptr;
    //! Выдалять ли текст при фокусе
    bool _select_all_on_focus = false;
    //! Показывать ли рамку
    bool _has_frame = true;
    //! Режим ввода текста
    TextMode _text_mode = TextMode::Combined;
    //! Очищать содержимое при потере фокуса, если ID не выбран
    bool _clear_on_exit = false;    
    //! Режим "комбобокса". Показывает кнопку комбобокса и по нажатию на нее производит поиск
    //! Если задан ID, то выводит все допустимые значения
    bool _combobox_mode = false;

    //! Запоминание области hoover. Взято из реализации стандартного комбобокса Qt
    QRect _hover_rect;
    QStyle::SubControl _hover_control = QStyle::SC_None;
    QStyle::StateFlag _arrow_state = QStyle::State_None;

    //! В процессе поиска, инициированного пользователем
    bool _user_search = false;    
    //! Родительские ключи. Например для поиска улицы в рамках города - это код города и т.п.
    UidList _parent_keys;
    //! Тип запроса (зависит от специфики внешнего сервиса:
    //! например "town" для поиска населенных пунктов, "street" для поиска улиц, "house" для домов и т.п.)
    QString _request_type;
    //! Параметры запроса (зависит от специфики внешнего сервиса)
    QVariant _request_options;
    //! Дополнительный фильтр, который будет присоединен к строке поиска
    QString _filter_text;

    //! Таймер для обновления доступности контролов
    QTimer* _update_enabled_timer = nullptr;

    //! Сколько строк выпадающего списка отображать
    int _view_height_count = 10;
    //! Насколько менять стандартную ширину выпадающего списка. (1 - 100%)
    qreal _view_width_count = 1;
    //! Максимальный размер элементов
    int _maximum_controls_size = -1;

    //! Текущий выбор
    Uid _id;
    //! Текущее значение (соответствует текущему id)
    QVariant _value;
    //! Набор аттрибутов для текущего выбора
    QMap<QString, QVariant> _current_attributes;

    //! id ответного сообщения
    MessageID _feedback_message_id;
    std::unique_ptr<QString> _current_search_text;

    //! Анимация ожидания (виджет)
    QLabel* _wait_movie_label = nullptr;    
    //! Кнопка очистки содержимого
    QToolButton* _erase_button = nullptr;
    QTimer* _hide_wait_movie_timer = nullptr;

    //! Текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    QString _override_text;
    QLineEdit* _override_editor = nullptr;

    //! Обновления внутренних данных выпадающего списка
    int _internal_data_update_counter = 0;
    bool _user_search_internal_data_update = false;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    friend class RequestSelectorContainer;
    friend class RequestSelectorListView;
    friend class RequestSelectorEditor;
    friend class RequestSelectorItemModel;

    using QWidget::setEnabled;
    Q_DISABLE_COPY(RequestSelector)
};

} // namespace zf
