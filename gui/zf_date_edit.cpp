#include "zf_date_edit.h"
#include "zf_core.h"
#include "zf_utils.h"
#include "zf_translation.h"
#include "zf_colors_def.h"
#include "zf_ui_size.h"

#include <QKeyEvent>
#include <QCalendar>
#include <QLocale>
#include <QCalendarWidget>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QToolButton>
#include <QTimer>
#include <QDesktopWidget>
#include <QtMath>
#include <QStyleOption>
#include <QStylePainter>

namespace zf
{
FormattedEdit::FormattedEdit(QValidator* editor_validator, QWidget* parent)
    : QFrame(parent)
{
    setAutoFillBackground(true);
    setFrameShape(QFrame::StyledPanel);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Colors::background());
    setPalette(pal);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QHBoxLayout* la = new QHBoxLayout;
    la->setMargin(0);
    la->setSpacing(0);
    setLayout(la);

    _internal_editor = new QLineEdit(this);
    _internal_editor->setFrame(false);
    _internal_editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _internal_editor->installEventFilter(this);
    setFocusProxy(_internal_editor);

    la->addWidget(_internal_editor);

    if (editor_validator != nullptr) {
        _internal_editor->setValidator(editor_validator);
        editor_validator->setParent(_internal_editor);
    }

    QHBoxLayout* la_buttons = new QHBoxLayout;
    la_buttons->setSpacing(0);
    la_buttons->setContentsMargins(0, 0, 2, 0);
    la->addLayout(la_buttons);

    _clear_button = new QToolButton;
    _clear_button->setAutoRaise(true);
    _clear_button->setFocusPolicy(Qt::NoFocus);
    _clear_button->setToolTip(ZF_TR(ZFT_CLEAR));
    la_buttons->addWidget(_clear_button);
    _clear_button->setIcon(QIcon(":/share_icons/cancel-bw.svg"));
    connect(_clear_button, &QToolButton::clicked, this, &FormattedEdit::clear);

    _popup_button = new QToolButton;
    _popup_button->setAutoRaise(true);
    _popup_button->setFocusPolicy(Qt::NoFocus);    
    la_buttons->addWidget(_popup_button);    
    connect(_popup_button, &QToolButton::clicked, this, [&]() { showPopup(); });

    connect(_internal_editor, &QLineEdit::cursorPositionChanged, this, &FormattedEdit::sg_cursorPositionChanged);
    connect(_internal_editor, &QLineEdit::textEdited, this, &FormattedEdit::sl_textEdited);
    connect(_internal_editor, &QLineEdit::textChanged, this, [&]() { _update_controls_timer->start(); });

    _update_controls_timer = new QTimer(this);
    _update_controls_timer->setSingleShot(true);
    _update_controls_timer->setInterval(0);
    connect(_update_controls_timer, &QTimer::timeout, this, [&]() { updateControlsHelper(); });
    _update_controls_timer->start();
}

QString FormattedEdit::value() const
{
    return _valid_text;
}

bool FormattedEdit::setValue(const QString& s)
{
    int pos = cursorPosition();
    QString s_prep = s.trimmed();
    if (!s_prep.isEmpty()) {
        QValidator::State state = validate(s_prep, pos);
        if (state != QValidator::Acceptable)
            return false;
    }

    setInternalText(s_prep, s_prep, false, pos);
    return true;
}

QString FormattedEdit::text() const
{
    return _editor_text;
}

void FormattedEdit::setText(const QString& s)
{
    if (!setValue(s))
        setInternalText(value(), s.trimmed(), false, cursorPosition());
}

void FormattedEdit::clear()
{
    setInternalText(QString(), QString(), true, 0);
}

QString FormattedEdit::placeholderText() const
{
    return _internal_editor->placeholderText();
}

void FormattedEdit::setPlaceholderText(const QString& s)
{
    _internal_editor->setPlaceholderText(s);
}

int FormattedEdit::cursorPosition() const
{
    return _internal_editor->cursorPosition();
}

void FormattedEdit::setCursorPosition(int pos)
{
    if (pos < 0)
        pos = 0;
    if (pos > _internal_editor->text().length())
        pos = _internal_editor->text().length();

    _internal_editor->setCursorPosition(pos);
}

void FormattedEdit::showPopup()
{
    if (_popup_visible)
        return;

    FormattedEditPopupContainer* c = container();
    setPopupValue(_popup_widget, value());

    _popup_visible = true;

    QRect place = QRect(QPoint(0, 0), c->minimumSizeHint());

    QPoint pos;
    QPoint pos2;
    QPoint cursor = mapFromGlobal(QCursor::pos());
    if (cursor.x() > width() / 2) {
        pos = rect().bottomRight();
        pos.setX(pos.x() - place.width());

        pos2 = rect().topRight();
        pos2.setX(pos2.x() - place.width());

    } else {
        pos = rect().bottomLeft();
        pos2 = rect().topLeft();
    }

    // частично выдрано из QDateTimeEditPrivate::positionCalendarPopup()
    pos = mapToGlobal(pos);
    pos2 = mapToGlobal(pos2);
    QSize size = c->sizeHint();

    // экран определяем на основании границ
    QRect screen = Utils::screenRect(this, Qt::AlignRight | Qt::AlignBottom);

    if (pos.x() + size.width() > screen.right())
        pos.setX(screen.right() - size.width());
    pos.setX(qMax(pos.x(), screen.left()));

    if (pos.y() + size.height() > screen.bottom())
        pos.setY(pos2.y() - size.height());
    else if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y() < screen.top())
        pos.setY(screen.top());
    if (pos.y() + size.height() > screen.bottom())
        pos.setY(screen.bottom() - size.height());
    place.moveTo(pos);

    c->setGeometry(place);
    c->show();
    _popup_widget->setFocus();
}

void FormattedEdit::hidePopup(bool apply, bool by_user)
{
    if (!_popup_visible)
        return;

    _popup_visible = false;
    container()->hide();

    if (apply) {
        QString new_text = getPopupValue(_popup_widget);
        int pos = qMax(0, new_text.length() >= cursorPosition() ? cursorPosition() : new_text.length());

        QValidator::State state = validate(new_text, pos);

        if (state == QValidator::Intermediate)
            setInternalText(QString(), new_text, by_user, pos);
        else if (state == QValidator::Acceptable)
            setInternalText(new_text, new_text, by_user, pos);
    }

    emit sg_popupClosed();
}

bool FormattedEdit::popupVisible() const
{
    return _popup_visible;
}

bool FormattedEdit::isReadOnly() const
{
    return _read_only;
}

void FormattedEdit::setReadOnly(bool b)
{
    if (b == _read_only)
        return;

    _read_only = b;
    _internal_editor->setReadOnly(b);
    _update_controls_timer->start();
    updateGeometry();
}

void FormattedEdit::setOverrideText(const QString& text)
{
    if (_override_text == text)
        return;

    _override_text = text;
    update();
    updateControlsHelper();
}

bool FormattedEdit::isNull() const
{
    return _valid_text.trimmed().isEmpty();
}

bool FormattedEdit::isInvalid() const
{
    return _valid_text.trimmed().isEmpty() && !_editor_text.trimmed().isEmpty();
}

void FormattedEdit::setAlignment(Qt::Alignment flag)
{
    _internal_editor->setAlignment(flag);
}

Qt::Alignment FormattedEdit::alignment() const
{
    return _internal_editor->alignment();
}

bool FormattedEdit::popupOnEnter() const
{
    return _popup_on_enter;
}

void FormattedEdit::setPopupOnEnter(bool b)
{
    _popup_on_enter = b;
}

QLineEdit* FormattedEdit::editor() const
{
    return _internal_editor;
}

QWidget* FormattedEdit::popupWidget() const
{
    container(); // чтобы _popup_widget был инициализирован
    return _popup_widget;
}

QToolButton* FormattedEdit::popupButton() const
{
    return _popup_button;
}

QToolButton* FormattedEdit::clearButton() const
{
    return _clear_button;
}

int FormattedEdit::maximumButtonsSize() const
{
    return _maximum_buttons_size;
}

void FormattedEdit::setMaximumButtonsSize(int n)
{
    if (_maximum_buttons_size == n)
        return;

    _maximum_buttons_size = n;
    _update_controls_timer->start();
}

void FormattedEdit::onUpdateControls()
{
}

bool FormattedEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _internal_editor) {
        if (!isReadOnly() && event->type() == QEvent::KeyPress && _override_text.isEmpty()) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->modifiers() == Qt::KeyboardModifier::ControlModifier && ke->key() == Qt::Key_Backspace) {
                clear();
                event->ignore();
                return true;
            }

            if (ke->modifiers() == Qt::KeyboardModifier::NoModifier) {
                switch (ke->key()) {
                    case Qt::Key_Enter:
                    case Qt::Key_Return: {
                        if (_popup_on_enter) {
                            showPopup();
                            event->ignore();
                            return true;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        } else if (event->type() == QEvent::EnabledChange) {
            // надо игнорировать установку состояния disabled для эдитора через систему свойств
            if (_internal_editor->testAttribute(Qt::WA_Disabled))
                _internal_editor->setAttribute(Qt::WA_Disabled, false);

            _update_controls_timer->start();
            return true;

        } else if (event->type() == QEvent::MouseButtonDblClick) {
            if (text().isEmpty() && !isReadOnly()) {
                showPopup();
                event->ignore();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

QSize FormattedEdit::sizeHint() const
{
    return minimumSizeHint();
}

QSize FormattedEdit::minimumSizeHint() const
{
    QSize size = UiSizes::defaultLineEditSize(font());
    const QFontMetrics& fm = fontMetrics();
    size.setWidth(fm.boundingRect("N").width() * minimumTextLength() + fm.averageCharWidth());
    size.setWidth(size.width() + (isFullEnabled() ? (double)_popup_button->minimumSizeHint().width() * 2 : 0));

    return size;
    /*
    ensurePolished();
    const QFontMetrics& fm = fontMetrics();

    QStyleOptionFrame opt;
    initStyleOption(&opt);

    QSize size;
    // magic numbers взяты из QComboBoxPrivate::recomputeSizeHint, просто верим им
    size.setHeight(qMax(qCeil(fm.height()), 14) + 2);
    size.setWidth(fm.boundingRect("N").width() * minimumTextLength() + fm.averageCharWidth());
    QSize n_size = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, size, this);
    size.setHeight(n_size.height() + 2);

    size.setWidth(size.width() + (isFullEnabled() ? (double)_popup_button->minimumSizeHint().width() * 2 : 0));
    return size;*/
}

void FormattedEdit::paintEvent(QPaintEvent* event)
{
    if (!_override_text.isEmpty()) {
        QColor background = isFullEnabled() ? Colors::background() : Colors::readOnlyBackground();

        QStylePainter painter(this);
        painter.fillRect(rect(), background);

        QStyleOptionFrame option;
        option.initFrom(this);
        option.rect = contentsRect();
        option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
        option.midLineWidth = 0;
        option.state |= QStyle::State_Sunken;
        option.features = QStyleOptionFrame::None;

        QRect text_rect = style()->subElementRect(QStyle::SE_LineEditContents, &option, this);
        text_rect.setY(text_rect.y() + 1);
        text_rect.adjust(2, 1, -2, -1);

        painter.drawItemText(text_rect, alignment(), palette(), true, _override_text.isEmpty() ? text() : _override_text);

        if (frameShape() != QFrame::NoFrame)
            Utils::paintBorder(this);

    } else {
        QFrame::paintEvent(event);
    }
}

void FormattedEdit::sl_textEdited(const QString& s)
{
    QString new_text = s;
    int pos = cursorPosition();

    QValidator::State state = validate(new_text, pos);

    if (state == QValidator::Acceptable)
        setInternalText(new_text, new_text, true, pos);
    else
        setInternalText(QString(), new_text, true, pos);
}

void FormattedEdit::initStyleOption(QStyleOptionFrame* option) const
{
    if (!option)
        return;

    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = frameShape() != QFrame::NoFrame ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this) : 0;
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (isReadOnly())
        option->state |= QStyle::State_ReadOnly;

    option->features = QStyleOptionFrame::None;
}

void FormattedEdit::setInternalText(const QString& valid_text, const QString& current_text, bool by_user, int pos)
{
    if (value() != valid_text) {
        QString old = value();
        _valid_text = valid_text;
        emit sg_valueChanged(old, valid_text);
        if (by_user)
            emit sg_valueEdited(old, valid_text);
    }

    QString old = text();

    if (pos < 0)
        pos = _internal_editor->cursorPosition();
    if (_internal_editor->text() != current_text) {
        _internal_editor->blockSignals(true);
        _internal_editor->setText(current_text);
        _internal_editor->blockSignals(false);        
    }

    setCursorPosition(pos);

    if (old != current_text) {
        _editor_text = current_text;
        emit sg_textChanged(old, current_text);
        if (by_user)
            emit sg_textEdited(old, current_text);
    }

    _update_controls_timer->start();
}

FormattedEditPopupContainer* FormattedEdit::container() const
{
    if (_popup_container == nullptr) {
        _internal_widgets.clear();
        _popup_widget = createPopup(_internal_widgets);
        Q_ASSERT(_popup_widget != nullptr);
        _popup_widget->setLocale(locale());
        connect(_popup_widget, &QWidget::destroyed, this, [&]() { _internal_widgets.clear(); });

        _popup_widget->installEventFilter(const_cast<FormattedEdit*>(this));

        _popup_container = new FormattedEditPopupContainer(const_cast<FormattedEdit*>(this), _popup_widget);
        _popup_container->setObjectName(QStringLiteral("zffec"));
        _popup_container->hide();
        _popup_container->installEventFilter(const_cast<FormattedEdit*>(this));
    }

    return _popup_container;
}

void FormattedEdit::updateControlsHelper()
{
    QColor background = isFullEnabled() ? Colors::background() : Colors::readOnlyBackground();

    setStyleSheet(QStringLiteral("QFrame {background-color: %1}").arg(background.name()));
    _internal_editor->setStyleSheet(QStringLiteral("QLineEdit {border: none; background-color: %1}").arg(background.name()));
    _internal_editor->setVisible(_override_text.isEmpty());
    _internal_editor->setReadOnly(!isFullEnabled());

    _clear_button->setVisible(_override_text.isEmpty() && isFullEnabled());
    _clear_button->setEnabled(isFullEnabled() && !_internal_editor->text().isEmpty());
    _popup_button->setVisible(_override_text.isEmpty() && isFullEnabled());

    UiSizes::prepareEditorToolButton(font(), _clear_button, true);
    UiSizes::prepareEditorToolButton(font(), _popup_button, true);

    onUpdateControls();
}

bool FormattedEdit::isFullEnabled() const
{
    return !isReadOnly() && isEnabled();
}

FormattedEditPopupContainer::FormattedEditPopupContainer(FormattedEdit* editor, QWidget* popup_widget)
    : QFrame(editor, Qt::Popup)
    , _editor(editor)
{
    setFrameShape(StyledPanel);
    setAttribute(Qt::WA_WindowPropagation);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setObjectName("zffepcla");
    layout->setSpacing(0);
    layout->setMargin(0);

    layout->addWidget(popup_widget);
    popup_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void FormattedEditPopupContainer::hideEvent(QHideEvent* event)
{
    QFrame::hideEvent(event);
    _editor->hidePopup(false, true);
}

DateEdit::DateEdit(QWidget* parent)
    : FormattedEdit(createValidator(), parent)
{
    init();
}

DateEdit::DateEdit(const QDate& d, QWidget* parent)
    : FormattedEdit(createValidator(), parent)
{
    init();
    setDate(d);
}

QDate DateEdit::date() const
{
    return fromString(value());
}

void DateEdit::setDate(const QDate& date)
{
    setValue(toString(date));
}

QValidator::State DateEdit::validate(QString& input, int& pos) const
{
    Q_UNUSED(pos);

    QString format = fullShortFormat();

    int first_symbol = -1;
    for (int i = 0; i < input.length(); i++) {
        if (input.at(i).isSpace())
            continue;
        first_symbol = i;
        break;
    }

    input = input.trimmed();

    if (pos > input.length())
        pos = input.length();
    else if (pos < first_symbol)
        pos = 0;

    if (input.isEmpty())
        return QValidator::Acceptable;

    input.replace('.', _date_separator);
    input.replace(';', _date_separator);
    input.replace(',', _date_separator);
    input.replace('/', _date_separator);
    input.replace('\\', _date_separator);
    input.replace('-', _date_separator);

    QList<int> input_separators = getSeparatorsPos(input);

    // TODO если курсор перед сепаратором и введен не сепаратор - перескакивать на следующий и вносить туда

    if (input_separators.count() < _format_sep_positions.count()) {
        if (input_separators.count() == 0) {
            if (pos == input.length() && input.length() == _format_sep_positions.at(0) + 1) {
                input.insert(_format_sep_positions.at(0), _date_separator);
                pos = input.length();
            }
        } else {
            Q_ASSERT(input_separators.count() == 1);
            if (pos == input.length() && input.length() == _format_sep_positions.at(1) + 1) {
                input.insert(_format_sep_positions.at(1), _date_separator);
                pos = input.length();
            }
        }
    }

    if (input.length() != format.length()) {
        if (pos == input.length() && input.length() == format.length() + 1) {
            input.remove(format.length(), input.length() - format.length());
            pos = input.length();
        }        
    }

    QDate date = fromString(input);
    if (date.isNull())
        return QValidator::Invalid;
    if (!date.isValid())
        return QValidator::Intermediate;
    if (!Utils::checkVariantSrv(date))
        return QValidator::Intermediate;

    return QValidator::Acceptable;
}

QWidget* DateEdit::createPopup(QWidgetList& internal_widgets) const
{
    Q_UNUSED(internal_widgets);

    auto w = new QCalendarWidget;
    connect(w, &QCalendarWidget::activated, this, &DateEdit::sl_calendarDateSelected);
    connect(w, &QCalendarWidget::clicked, this, &DateEdit::sl_calendarDateSelected);
    connect(w, &QCalendarWidget::currentPageChanged, this, [w](int year, int month) {
        QDate date = QDate(year, month,
                           qMin(w->calendar().daysInMonth(month, year),
                                w->selectedDate().isNull() ? QDate::currentDate().day() : w->selectedDate().day()));
        w->setSelectedDate(date);
    });

    return w;
}

void DateEdit::setPopupValue(QWidget* popup_widget, const QString& value)
{
    QCalendarWidget* w = qobject_cast<QCalendarWidget*>(popup_widget);
    Q_ASSERT(w != nullptr);

    w->setSelectedDate(fromString(value));
}

QString DateEdit::getPopupValue(QWidget* popup_widget) const
{
    QCalendarWidget* w = qobject_cast<QCalendarWidget*>(popup_widget);
    Q_ASSERT(w != nullptr);

    return toString(w->selectedDate());
}

int DateEdit::minimumTextLength() const
{
    return fullShortFormat().length();
}

void DateEdit::onUpdateControls()
{
    FormattedEdit::onUpdateControls();
    UiSizes::prepareEditorToolButton(font(), popupButton(), true, -3);
}

bool DateEdit::event(QEvent* e)
{
    if (e->type() == QEvent::LocaleChange) {
        resetCache();
    }

    return FormattedEdit::event(e);
}

void DateEdit::sl_valueChanged(const QString& old_value, const QString& new_value)
{
    QDate old_date = fromString(old_value);
    QDate new_date = fromString(new_value);
    if (old_date != new_date)
        emit sg_dateChanged(old_date, new_date);
}

void DateEdit::sl_valueEdited(const QString& old_value, const QString& new_value)
{
    QDate old_date = fromString(old_value);
    QDate new_date = fromString(new_value);
    if (old_date != new_date)
        emit sg_dateEdited(old_date, new_date);
}

void DateEdit::sl_calendarDateSelected(const QDate& date)
{
    Q_UNUSED(date);
    hidePopup(true, true);
}

void DateEdit::init()
{
    popupButton()->setToolTip(ZF_TR(ZFT_SELECT_CALENDAR));
    popupButton()->setIcon(QIcon(":/share_icons/blue/calendar-small.svg"));

    connect(this, &FormattedEdit::sg_valueChanged, this, &DateEdit::sl_valueChanged);
    connect(this, &FormattedEdit::sg_valueEdited, this, &DateEdit::sl_valueEdited);
}

void DateEdit::resetCache()
{
    _full_short_format.clear();
    _date_separator = QChar();
    _format_sep_positions.clear();
}

QDate DateEdit::fromString(const QString& s) const
{
    QString s_prep = s.trimmed();
    if (s_prep.isEmpty())
        return QDate();

    QString format = fullShortFormat();

    QDate res = locale().toDate(s_prep, format);
    if (res.isNull())
        res = locale().toDate(s_prep, locale().dateFormat(QLocale::FormatType::ShortFormat));

    if (res.isNull()) {
        // надо обработать дату с неполным заполнением вида 1.1.2000 -> 01.01.2000
        QList<int> input_separators = getSeparatorsPos(s_prep);
        if (input_separators.count() == 2 && _format_sep_positions.count() == 2) {
            if (format.at(0) == 'y') {
                // перевернутая американская локаль 2000/1/1 -> 2000/01/01
                if (s_prep.length() - input_separators.at(1) == 1)
                    s_prep.insert(input_separators.at(1) + 1, '0');
                if (input_separators.at(1) - input_separators.at(0) == 2)
                    s_prep.insert(input_separators.at(0) + 1, '0');

            } else {
                if (input_separators.at(1) - input_separators.at(0) == 2)
                    s_prep.insert(input_separators.at(0) + 1, '0');
                if (input_separators.at(0) == 1)
                    s_prep.insert(0, '0');
            }

            res = locale().toDate(s_prep, format);
        }
    }

    return res;
}

QString DateEdit::toString(const QDate& date) const
{
    return date.isNull() ? QString() : locale().toString(date, fullShortFormat());
}

QValidator* DateEdit::createValidator()
{
    return new QRegularExpressionValidator(QRegularExpression(R"([\d.,;\-\\\/]*)"));
}

QList<int> DateEdit::getSeparatorsPos(const QString& input) const
{
    QList<int> res;
    int separator_pos = -1;
    while (true) {
        int pos = input.indexOf(_date_separator, separator_pos + 1);
        if (pos < 0)
            break;

        separator_pos = pos;
        res << pos;
    }

    return res;
}

QString DateEdit::fullShortFormat() const
{
    if (_full_short_format.isEmpty()) {
        QString format = locale().dateFormat(QLocale::FormatType::ShortFormat);
        // для  некоторых локалей выдает идиотский сокращенный формат вида M/d/yy. Такое надо разворачивать в MM/dd/yyyy
        int day_pos = -1;
        int month_pos = -1;
        int year_pos = -1;

        for (int i = 0; i < format.length(); i++) {
            const QChar& c = format.at(i);

            if (c == 'd') {
                if (day_pos < 0)
                    day_pos = i;

            } else if (day_pos >= 0 && i - day_pos < 2) {
                _full_short_format.append(QString(2 - (i - day_pos), 'd'));
                day_pos = -1;
            }

            if (c == 'M') {
                if (month_pos < 0)
                    month_pos = i;

            } else if (month_pos >= 0 && i - month_pos < 2) {
                _full_short_format.append(QString(2 - (i - month_pos), 'M'));
                month_pos = -1;
            }

            if (c == 'y') {
                if (year_pos < 0)
                    year_pos = i;

            } else if (year_pos >= 0 && i - year_pos < 4) {
                _full_short_format.append(QString(4 - (i - year_pos), 'y'));
                year_pos = -1;
            }

            _full_short_format.append(c);
        }

        if (day_pos >= 0 && format.length() - day_pos < 2)
            _full_short_format.append(QString(2 - (format.length() - day_pos), 'd'));
        if (month_pos >= 0 && format.length() - month_pos < 2)
            _full_short_format.append(QString(2 - (format.length() - month_pos), 'M'));
        if (year_pos >= 0 && format.length() - year_pos < 4)
            _full_short_format.append(QString(4 - (format.length() - year_pos), 'y'));

        for (int i = 0; i < _full_short_format.length(); i++) {
            const QChar& c = _full_short_format.at(i);

            if (c != 'd' && c != 'y' && c != "M") {
                if (_date_separator.isNull())
                    _date_separator = c;
                else
                    Q_ASSERT(_date_separator == c);

                _format_sep_positions << i;
            }
        }
        Q_ASSERT(_format_sep_positions.count() == 2);
        Q_ASSERT(!_date_separator.isNull());
    }

    return _full_short_format;
}

} // namespace zf
