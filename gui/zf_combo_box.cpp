#include "zf_combo_box.h"

#include "zf_core.h"
#include "zf_utils.h"

#include <QPaintEvent>
#include <QStylePainter>
#include <QAbstractItemView>

namespace zf
{
ComboBox::ComboBox(QWidget* parent)
    : QComboBox(parent)
{
    Utils::updatePalette(isReadOnly(), false, this);
}

bool ComboBox::isReadOnly() const
{
    return !isEnabled();
}

void ComboBox::setReadOnly(bool b)
{
    setEnabled(!b);
}

void ComboBox::showPopup()
{
    QComboBox::showPopup();

    // дятлы из Qt постоянно забывают про accessible id
    QObjectList list;
    Utils::findObjectsByClass(list, this, QStringLiteral("QComboBoxPrivateContainer"), false, false);
    Z_CHECK(list.count() == 1);
    list.constFirst()->setObjectName("qtpc");

    if (view() != nullptr && view()->objectName().isEmpty())
        view()->setObjectName("qtcl");

    emit sg_popupOpened();
}

void ComboBox::hidePopup()
{
    QComboBox::hidePopup();
    emit sg_popupClosed();
}

void ComboBox::setOverrideText(const QString& text)
{
    if (_override_text == text)
        return;

    _override_text = text;
    update();
}

void ComboBox::changeEvent(QEvent* event)
{
    QComboBox::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::PaletteChange)
        Utils::updatePalette(isReadOnly(), false, this);
}

void ComboBox::paintEvent(QPaintEvent* event)
{
    if (isReadOnly() || !_override_text.isEmpty()) {
        QStylePainter painter(this);

        QStyleOptionFrame option;
        option.initFrom(this);
        option.rect = contentsRect();
        option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
        option.midLineWidth = 0;
        option.state |= QStyle::State_Sunken;
        option.features = QStyleOptionFrame::None;

        QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &option, this);
        painter.fillRect(rect, palette().color(QPalette::Base));

        rect.setY(rect.y() + 1);
        rect.adjust(2, 1, -2, -1);
        painter.setPen(palette().color(QPalette::Active, QPalette::WindowText));
        painter.drawItemText(rect, 0, palette(), true, _override_text.isEmpty() ? currentText() : _override_text);

        if (hasFrame())
            Utils::paintBorder(this);

    } else {
        QComboBox::paintEvent(event);
    }
}

void zf::ComboBox::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

} // namespace zf
