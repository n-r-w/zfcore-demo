#pragma once

#include "zf_global.h"

#include <QDateEdit>
#include <QFrame>
#include <QValidator>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPointer>
#include <QDate>
#include <QStyleOption>

class QToolButton;

namespace zf
{
class FormattedEdit;

class FormattedEditPopupContainer : public QFrame
{
    Q_OBJECT
public:
    FormattedEditPopupContainer(FormattedEdit* editor, QWidget* popup_widget);
    void hideEvent(QHideEvent* event) override;

    FormattedEdit* _editor;
};

class ZCORESHARED_EXPORT FormattedEdit : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QString value READ value WRITE setValue DESIGNABLE true STORED true USER true)
    Q_PROPERTY(QString text READ text WRITE setText DESIGNABLE true STORED true)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText DESIGNABLE true STORED true)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition DESIGNABLE false STORED false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)
    Q_PROPERTY(bool popupOnEnter READ popupOnEnter WRITE setPopupOnEnter DESIGNABLE true STORED true)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment DESIGNABLE true STORED true)

public:
    FormattedEdit(QValidator* editor_validator, QWidget* parent);

    //! Текущее значение. Это корректное значение, которое прошло валидацию. Может отличаться от текущего текста
    QString value() const;
    //! Возвращает истину если значение прошло валидацию и было установлено
    Q_SLOT bool setValue(const QString& s);

    //! Текущий текст в редакторе ввода
    QString text() const;
    //! Если текст подходит, то устанавливает его как value. Иначе как текст
    void setText(const QString& s);

    //! Очистить содержимое
    Q_SLOT void clear();

    QString placeholderText() const;
    void setPlaceholderText(const QString& s);

    int cursorPosition() const;
    void setCursorPosition(int pos);

    void showPopup();
    //! Скрыть popup. Popup виджет должен отслеживать выбор пользователя и вызывать этот метод при необходимости (например по mousePressEvent)
    void hidePopup(
        //! Применить или отменить изменения
        bool apply,
        //! Действие пользователя или программно
        bool by_user = true);
    bool popupVisible() const;

    //! Только для чтения
    bool isReadOnly() const;
    void setReadOnly(bool b);

    //! Задать текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    void setOverrideText(const QString& text);

    //! Выбран NULL
    bool isNull() const;
    //! Текст задан, но он невалидный
    bool isInvalid() const;

    void setAlignment(Qt::Alignment flag);
    Qt::Alignment alignment() const;

    //! Открывать popup при нажатии клавиши Enter
    bool popupOnEnter() const;
    void setPopupOnEnter(bool b);

    //! Встроенный редактор
    QLineEdit* editor() const;
    //! Popup виджет
    QWidget* popupWidget() const;

    //! Кнопка для вызова popup
    QToolButton* popupButton() const;
    //! Кнопка очистки
    QToolButton* clearButton() const;

    //! Максимальный размер кнопок
    int maximumButtonsSize() const;
    void setMaximumButtonsSize(int n);

protected:
    //! Проверка текста на корректность и исправление при необходимости
    virtual QValidator::State validate(
        //! Введенный пользователем текст
        QString& input,
        //! Положение курсора
        int& pos) const = 0;
    //! Создать виджет для отображения в popup
    virtual QWidget* createPopup(
        //! Список внутренних виджетов, которые могут иметь фокус ввода (например viewport для ItemView)
        QWidgetList& internal_widgets) const = 0;
    //! Задать текущее значение для popup
    virtual void setPopupValue(QWidget* popup_widget, const QString& value) = 0;
    //! Получить текущее значение из popup
    virtual QString getPopupValue(QWidget* popup_widget) const = 0;
    //! Минимальный размер текста в символах
    virtual int minimumTextLength() const = 0;
    //! Вызывается когда надо обновить размеры и состояние кнопок
    virtual void onUpdateControls();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void paintEvent(QPaintEvent* event) override;

signals:
    void sg_valueChanged(const QString& old_value, const QString& new_value);
    void sg_valueEdited(const QString& old_value, const QString& new_value);

    void sg_textChanged(const QString& old_text, const QString& new_text);
    void sg_textEdited(const QString& old_text, const QString& new_text);

    void sg_cursorPositionChanged(int old_pos, int new_pos);

    void sg_popupClosed();

private slots:
    void sl_textEdited(const QString& s);

private:
    void initStyleOption(QStyleOptionFrame* option) const;
    //! Изменение содержимого
    void setInternalText(const QString& valid_text, const QString& current_text, bool by_user, int pos);
    //! Контейнер для размещения popup виджета
    FormattedEditPopupContainer* container() const;
    //! Обновление состояния
    void updateControlsHelper();
    //! Проверка одновременно на isEnabled, isReadOnly все прочее что может повлиять на
    //! возможность редактирования
    bool isFullEnabled() const;

    //! Редактор
    QLineEdit* _internal_editor;
    //! Кнопка для вызова popup
    QToolButton* _popup_button;
    //! Кнопка очистки
    QToolButton* _clear_button;
    //! Контейнер для размещения popup виджета
    mutable QPointer<FormattedEditPopupContainer> _popup_container;
    //! popup виджет
    mutable QPointer<QWidget> _popup_widget;
    //! Список внутренних виджетов, которые могут иметь фокус ввода (например viewport для ItemView)
    mutable QWidgetList _internal_widgets;

    //! Текущее состояние popup
    bool _popup_visible = false;
    //! Корректный текст
    QString _valid_text;
    //! Содержимое редактора
    QString _editor_text;
    //! Состояние только для чтения
    bool _read_only = false;
    //! Текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    QString _override_text;
    //! Открывать popup при нажатии клавиши Enter
    bool _popup_on_enter = true;
    //! Максимальный размер кнопок
    int _maximum_buttons_size = -1;

    //! Таймер для обновления доступности контролов
    QTimer* _update_controls_timer = nullptr;

    friend class FormattedEditPopupContainer;
};

class ZCORESHARED_EXPORT DateEdit : public FormattedEdit
{
    Q_OBJECT
    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY sg_dateChanged DESIGNABLE true STORED true)

public:
    explicit DateEdit(QWidget* parent = nullptr);
    explicit DateEdit(const QDate& d, QWidget* parent = nullptr);

    QDate date() const;
    Q_SLOT void setDate(const QDate& date);

protected:
    //! Проверка текста на корректность и исправление при необходимости
    QValidator::State validate(
        //! Введенный пользователем текст
        QString& input,
        //! Положение курсора
        int& pos) const override;
    //! Создать виджет для отображения в popup
    QWidget* createPopup(
        //! Список внутренних виджетов, которые могут иметь фокус ввода (например viewport для ItemView)
        QWidgetList& internal_widgets) const override;
    //! Задать текущее значение для popup
    void setPopupValue(QWidget* popup_widget, const QString& value) override;
    //! Получить текущее значение из popup
    QString getPopupValue(QWidget* popup_widget) const override;
    //! Минимальный размер текста в символах
    int minimumTextLength() const override;
    //! Вызывается когда надо обновить размеры и состояние кнопок
    void onUpdateControls() override;

protected:
    bool event(QEvent* e) override;

signals:
    void sg_dateChanged(const QDate& old_date, const QDate& new_date);
    void sg_dateEdited(const QDate& old_date, const QDate& new_date);

private slots:
    void sl_valueChanged(const QString& old_value, const QString& new_value);
    void sl_valueEdited(const QString& old_value, const QString& new_value);
    void sl_calendarDateSelected(const QDate& date);

private:
    void init();
    void resetCache();
    QDate fromString(const QString& s) const;
    QString toString(const QDate& date) const;
    static QValidator* createValidator();
    QList<int> getSeparatorsPos(const QString& input) const;

    //! Нормальный формат даты
    QString fullShortFormat() const;

    mutable QString _full_short_format;
    mutable QChar _date_separator;
    mutable QList<int> _format_sep_positions;
};

} // namespace zf
