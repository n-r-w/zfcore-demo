#include "zf_spin_box.h"
#include "zf_utils.h"

#include <QPaintEvent>

namespace zf
{
SpinBox::SpinBox(QWidget* parent)
    : QSpinBox(parent)
{
}

void SpinBox::changeEvent(QEvent* event)
{
    QSpinBox::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange)
        Utils::updatePalette(!isEnabled() || isReadOnly(), false, this);
}

void SpinBox::paintEvent(QPaintEvent* event)
{
    QSpinBox::paintEvent(event);
}

void SpinBox::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

} // namespace zf
