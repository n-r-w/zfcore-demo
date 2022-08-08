#include "zf_double_spin_box.h"
#include "zf_utils.h"

#include <QPaintEvent>

namespace zf
{
DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
}

void DoubleSpinBox::changeEvent(QEvent* event)
{
    QDoubleSpinBox::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange)
        Utils::updatePalette(!isEnabled() || isReadOnly(), false, this);
}

void DoubleSpinBox::paintEvent(QPaintEvent* event)
{
    QDoubleSpinBox::paintEvent(event);
}

void DoubleSpinBox::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

} // namespace zf
