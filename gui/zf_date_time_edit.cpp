#include "zf_date_time_edit.h"
#include "zf_core.h"
#include "zf_utils.h"
#include "zf_translation.h"
#include "zf_colors_def.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QToolButton>
#include <QStylePainter>

namespace zf
{
DateTimeEdit::DateTimeEdit(QWidget* parent)
    : QDateTimeEdit(parent)
{
    setCalendarPopup(true);

    setFocusPolicy(Qt::StrongFocus);
    _internal_editor = findChild<QLineEdit*>("qt_spinbox_lineedit");
    Z_CHECK(_internal_editor);
    _internal_editor->setFocusPolicy(Qt::StrongFocus);
    _internal_editor->installEventFilter(this);
    connect(_internal_editor, &QLineEdit::textEdited, this, &DateTimeEdit::sl_textEdited);

    Utils::updatePalette(isReadOnlyHelper(), false, this);
}

DateTimeEdit::~DateTimeEdit()
{
    if (_clear_button != nullptr)
        delete _clear_button;
}

QDateTime DateTimeEdit::dateTime() const
{
    if (_is_nullable && _is_null) {
        return QDateTime();
    } else {
        return QDateTimeEdit::dateTime();
    }
}

QDate DateTimeEdit::date() const
{
    if (_is_nullable && _is_null) {
        return QDate();
    } else {
        return QDateTimeEdit::date();
    }
}

QTime DateTimeEdit::time() const
{
    if (_is_nullable && _is_null) {
        return QTime();
    } else {
        return QDateTimeEdit::time();
    }
}

bool DateTimeEdit::isNull() const
{
    return _is_null;
}

void DateTimeEdit::setDateTime(const QDateTime& dateTime)
{
    if (_is_nullable && !dateTime.isValid()) {
        setNull(true);
    } else {
        setNull(false);
        QDateTimeEdit::setDateTime(dateTime);
    }
}

void DateTimeEdit::setDate(const QDate& date)
{
    if (_is_nullable && !date.isValid()) {
        setNull(true);
    } else {
        setNull(false);
        QDateTimeEdit::setDate(date);
    }
}

void DateTimeEdit::setDate(const QVariant& date)
{
    if (date.isNull() || !date.isValid())
        setDate(QDate());
    else
        setDate(date.toDate());
}

void DateTimeEdit::setTime(const QTime& time)
{
    if (_is_nullable && !time.isValid()) {
        setNull(true);
    } else {
        setNull(false);
        QDateTimeEdit::setTime(time);
    }
}

bool DateTimeEdit::isNullable() const
{
    return _is_nullable;
}

void DateTimeEdit::setNullable(bool enable)
{
    if (_is_nullable == enable)
        return;

    _is_nullable = enable;

    if (enable && !_clear_button) {
        _clear_button = new QToolButton(this);
        _clear_button->setToolTip(ZF_TR(ZFT_CLEAR));
        _clear_button->setObjectName(QStringLiteral("zf_clear_button"));
        _clear_button->setAutoRaise(true);

        _clear_button->setIcon(QIcon(":/share_icons/cancel-bw.svg"));
        _clear_button->setFocusPolicy(Qt::NoFocus);
        connect(_clear_button, &QToolButton::clicked, this, &DateTimeEdit::sl_clearButtonClicked);
        _clear_button->installEventFilter(this);
        updateControlsGeometry();
        updateClearButton();

    } else if (_clear_button) {
        disconnect(_clear_button, &QToolButton::clicked, this, &DateTimeEdit::sl_clearButtonClicked);
        delete _clear_button;
        _clear_button = nullptr;
    }

    update();
}

QSize DateTimeEdit::sizeHint() const
{
    QSize size = QDateTimeEdit::sizeHint();
    size.setWidth(minimumSizeHint().width());

    return size;
}

QSize DateTimeEdit::minimumSizeHint() const
{
    QSize size = QDateTimeEdit::minimumSizeHint();
    size.setWidth(minimumWidth());

    return size;
}

void DateTimeEdit::clear()
{
    if (isNullable())
        setDate(QDate());
    else
        QDateTimeEdit::clear();
}

void DateTimeEdit::setOverrideText(const QString& text)
{
    if (_override_text == text)
        return;

    _override_text = text;
    update();
}

bool DateTimeEdit::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == _internal_editor && _override_text.isEmpty()) {
        if (event->type() == QEvent::MouseButtonDblClick && _is_nullable && _is_null) {
            if (showPopup()) {
                setDateTime(QDateTime::currentDateTime());
                event->accept();
                return true;
            }
        }
    }

    return QDateTimeEdit::eventFilter(obj, event);
}

QString DateTimeEdit::textFromDateTime(const QDateTime& dt) const
{
    QString res;
    if (!dt.isValid() || dt.isNull() || (_is_nullable && _is_null))
        res = QString();
    else
        res = QDateTimeEdit::textFromDateTime(dt);

    return res;
}

bool DateTimeEdit::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateControlsGeometry();
            break;

        case QEvent::Wheel:
            // Отключение прокрутки даты при прокрутке scroll area
            event->setAccepted(false);
            return true;

        default:
            break;
    }

    return QDateTimeEdit::event(event);
}

void DateTimeEdit::changeEvent(QEvent* event)
{
    QDateTimeEdit::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange)
        _internal_editor->setHidden(isReadOnlyHelper());

    switch (event->type()) {
        case QEvent::StyleChange:
        case QEvent::FontChange:
            Utils::updatePalette(isReadOnlyHelper(), false, this);
            updateControlsGeometry();
            break;

        case QEvent::PaletteChange:
        case QEvent::ReadOnlyChange:
        case QEvent::EnabledChange: {
            if (isReadOnlyHelper())
                _internal_editor->setCursor(Qt::IBeamCursor);

            updateClearButton();
            Utils::updatePalette(isReadOnlyHelper(), false, this);
            break;
        }

        default:
            break;
    }
}

void DateTimeEdit::showEvent(QShowEvent* event)
{
    QDateTimeEdit::showEvent(event);
    setNull(_is_null); // force empty string back in
}

void DateTimeEdit::resizeEvent(QResizeEvent* event)
{
    QDateTimeEdit::resizeEvent(event);
    updateControlsGeometry();
}

void DateTimeEdit::paintEvent(QPaintEvent* event)
{
    if (isReadOnlyHelper() || !_override_text.isEmpty()) {
        QStylePainter painter(this);
        painter.fillRect(rect(), Colors::readOnlyBackground());

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

        if (hasFrame())
            Utils::paintBorder(this);

    } else {
        QDateTimeEdit::paintEvent(event);
    }
}

void DateTimeEdit::keyPressEvent(QKeyEvent* event)
{
    if (!_override_text.isEmpty()) {
        event->ignore();
        return;
    }

    if (!isReadOnly() && isEnabled()) {
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            showPopup();
            event->accept();
            return;
        }

        if (_is_nullable && !event->text().isEmpty() && event->text().at(0).isLetter()) {
            event->ignore();
            return;
        }

        if (_is_nullable && (event->key() >= Qt::Key_0) && (event->key() <= Qt::Key_9) && _is_null) {
            if (event->key() == Qt::Key_0) {
                setDate(QDate::currentDate());
                _internal_editor->setCursorPosition(0);
            } else {
                QDate curDate = QDate::currentDate();
                setDate(QDate(curDate.year(), curDate.month(), event->text().toInt()));
                _internal_editor->setCursorPosition(1);
            }
        }
        if (event->key() == Qt::Key_Tab && _is_nullable && _is_null) {
            QAbstractSpinBox::keyPressEvent(event); // игнорируем базовый класс
            return;
        }
        if ((event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) && _is_nullable) {
            if (_is_null)
                return;

            if (_internal_editor->selectedText() == _internal_editor->text()) {
                setDateTime(QDateTime());
                event->accept();
                return;
            }
        }
    }

    QDateTimeEdit::keyPressEvent(event);
}

void DateTimeEdit::mousePressEvent(QMouseEvent* event)
{
    if (!_override_text.isEmpty()) {
        event->ignore();
        return;
    }

    bool saveNull = _is_null;
    QDateTimeEdit::mousePressEvent(event);
    if (_is_nullable && saveNull && calendarWidget() != nullptr && calendarWidget()->isVisible()) {
        setDateTime(QDateTime::currentDateTime());
    }
}

void DateTimeEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!_override_text.isEmpty()) {
        event->ignore();
        return;
    }

    QDateTimeEdit::mouseDoubleClickEvent(event);
}

bool DateTimeEdit::focusNextPrevChild(bool next)
{
    if (_is_nullable && _is_null) {
        return QAbstractSpinBox::focusNextPrevChild(next); // игнорируем базовый класс
    } else {
        return QDateTimeEdit::focusNextPrevChild(next);
    }
}

void DateTimeEdit::focusInEvent(QFocusEvent* e)
{
    // шаманские действия чтобы убрать warning внутри qt при попытке выделить несуществующий текст даты
    if (_is_nullable && _is_null)
        _internal_editor->setText("   ");

    QDateTimeEdit::focusInEvent(e);

    if (_is_nullable && _is_null) {
        _internal_editor->clear();
    }
}

QValidator::State DateTimeEdit::validate(QString& input, int& pos) const
{
    if (_is_nullable && _is_null) {
        return QValidator::Acceptable;
    }
    return QDateTimeEdit::validate(input, pos);
}

void DateTimeEdit::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

void DateTimeEdit::sl_clearButtonClicked()
{
    if (isReadOnlyHelper())
        return;

    setNull(true);
}

void DateTimeEdit::sl_textEdited(const QString& s)
{
    if (_is_nullable && _is_null) {
        QString v = s;
        int pos = 0;
        if (QDateTimeEdit::validate(v, pos) == QValidator::Acceptable) {
            setNull(false);
            setDateTime(QDateTime::fromString(v, displayFormat()));
        }
    }
}

void DateTimeEdit::setNull(bool b)
{
    if (b == _is_null)
        return;

    _is_null = b;
    if (_is_null) {
        setDateTime(QDateTime());
        QLineEdit* edit = findChild<QLineEdit*>("qt_spinbox_lineedit");
        Z_CHECK_NULL(edit);
        if (!edit->text().isEmpty()) {
            edit->setText(QString());
        }
    }

    updateClearButton();
}

int DateTimeEdit::minimumWidth() const
{
    int w = fontMetrics().horizontalAdvance(
        Core::locale(LocaleType::UserInterface)->toString(QDateTime(QDate(2000, 12, 12), QTime(23, 23)), displayFormat()));

    if (isNullable())
        w += clearButtonRect().width() + 8;

    w += buttonRect().width();

    return w;
}

void DateTimeEdit::updateControlsGeometry()
{
    if (_clear_button) {
        auto rect = clearButtonRect();
        _clear_button->setGeometry(rect);
        _clear_button->setIconSize(rect.adjusted(2, 2, -2, -2).size());
    }
}

QRect DateTimeEdit::clearButtonRect() const
{
    auto button_rect = buttonRect();

    QRect rect;
    if (calendarPopup()) {
        rect = button_rect.adjusted(0, 2, 0, -2);
        rect.setLeft(rect.left() - (rect.height() - rect.width()));
        rect.moveRight(button_rect.left() - 1);

    } else {
        rect = button_rect;
        rect.setLeft(rect.left() - (rect.height() - rect.width()));
        rect.moveRight(button_rect.left() - 1);
    }

    return rect;
}

QRect DateTimeEdit::buttonRect() const
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    QRect rect;
    if (calendarPopup()) {
        QStyleOptionComboBox optCombo;

        optCombo.init(this);
        optCombo.editable = true;
        optCombo.frame = opt.frame;
        optCombo.subControls = opt.subControls;
        optCombo.activeSubControls = opt.activeSubControls;
        optCombo.state = opt.state;
        if (isReadOnlyHelper()) {
            optCombo.state &= ~QStyle::State_Enabled;
        }

        opt.subControls = QStyle::SC_ComboBoxArrow;
        rect = style()->subControlRect(QStyle::CC_ComboBox, &optCombo, QStyle::SC_ComboBoxArrow);

    } else {
        opt.subControls = QStyle::SC_SpinBoxUp;
        auto rect_up = style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this);
        opt.subControls = QStyle::SC_SpinBoxDown;
        auto rect_down = rect.united(style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this));

        rect = rect_up.united(rect_down);
    }

    return rect;
}

bool DateTimeEdit::isReadOnlyHelper() const
{
    return isReadOnly() || !isEnabled();
}

void DateTimeEdit::updateClearButton()
{
    if (_clear_button && _is_nullable) {
        _clear_button->setHidden(isReadOnlyHelper());
        _clear_button->setEnabled(!_is_null);
    }
}

bool DateTimeEdit::showPopup()
{
    if (calendarPopup() && !isReadOnlyHelper() && (calendarWidget() == nullptr || !calendarWidget()->isVisible())) {
        // Извращенная генерация нажатия на кнопку для показа календаря
        QMouseEvent* me
            = new QMouseEvent(QEvent::MouseButtonPress, QPointF(static_cast<qreal>(width()) - 10, static_cast<qreal>(height()) - 10),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

        QDateTimeEdit::mousePressEvent(me);
        delete me;
        return true;
    }
    return false;
}
} // namespace zf
