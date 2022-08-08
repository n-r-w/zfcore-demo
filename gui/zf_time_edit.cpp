#include "zf_time_edit.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_colors_def.h"
#include "zf_ui_size.h"

#include <QPaintEvent>
#include <QStylePainter>
#include <QLineEdit>

namespace zf
{
TimeEdit::TimeEdit(QWidget* parent)
    : QTimeEdit(parent)
{
    _internal_editor = findChild<QLineEdit*>("qt_spinbox_lineedit");
    Z_CHECK_NULL(_internal_editor);
}

TimeEdit::~TimeEdit()
{
}

QSize TimeEdit::sizeHint() const
{
    return minimumSizeHint();
}

QSize TimeEdit::minimumSizeHint() const
{
    QSize size = zf::UiSizes::defaultLineEditSize(font());
    size.setWidth(qMax(minimumWidth(size.height()), QTimeEdit::minimumSizeHint().width()));

    return size;
}

void TimeEdit::changeEvent(QEvent* event)
{
    QTimeEdit::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange)
        _internal_editor->setHidden(isReadOnlyHelper());

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange)
        Utils::updatePalette(isReadOnlyHelper(), false, this);
}

void TimeEdit::paintEvent(QPaintEvent* event)
{
    if (isReadOnlyHelper()) {
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

        painter.drawItemText(text_rect, alignment(), palette(), true, text());

        if (hasFrame())
            Utils::paintBorder(this);

    } else {
        QTimeEdit::paintEvent(event);
    }
}

void TimeEdit::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

int TimeEdit::minimumWidth(int height) const
{
    int w = fontMetrics().horizontalAdvance(QTime::currentTime().toString(displayFormat()));

    if (calendarPopup())
        w += height;

    return w;
}

bool TimeEdit::isReadOnlyHelper() const
{
    return isReadOnly() || !isEnabled();
}
} // namespace zf
