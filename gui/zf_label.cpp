#include "zf_label.h"
#include <QPainter>

namespace zf
{
Label::Label(QWidget* parent)
    : QLabel("", parent)
{    
}

Label::Label(const QString& text, QWidget* parent)
    : QLabel(text, parent)
{
}

Qt::Orientation Label::orientation() const
{
    return _orientation;
}

void Label::setOrientation(Qt::Orientation o)
{
    if (o == _orientation)
        return;

    _orientation = o;
    updateGeometry();
    repaint();
}

void Label::paintEvent(QPaintEvent* e)
{
    if (_orientation == Qt::Horizontal) {
        QLabel::paintEvent(e);
        return;
    }

    QPainter painter(this);
    int width, height;

    painter.translate(0, sizeHint().height());
    painter.rotate(270);

    width = sizeHint().height();
    height = sizeHint().width();

    painter.drawText(QRect(QPoint(-(this->height() / 2 - width / 2), this->width() / 2 - height / 2), QLabel::sizeHint()), alignment(),
                     text());
}

QSize Label::minimumSizeHint() const
{
    if (_orientation == Qt::Horizontal)
        return QLabel::minimumSizeHint();

    QSize s = QLabel::minimumSizeHint();
    return QSize(s.height(), s.width());
}

QSize Label::sizeHint() const
{
    if (_orientation == Qt::Horizontal)
        return QLabel::sizeHint();

    QSize s = QLabel::sizeHint();
    return QSize(s.height(), s.width());
}

} // namespace zf
