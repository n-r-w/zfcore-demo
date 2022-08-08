#include "zf_graphics_effects.h"
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QtMath>
#include "zf_utils.h"
#include "zf_plain_text_edit.h"

namespace zf
{
HighlightBorderEffect::HighlightBorderEffect(QWidget* widget, InformationType type)
    : QGraphicsEffect(widget)
    , _widget(widget)
    , _type(type)
{
    Z_CHECK_NULL(widget);

    connect(qApp, &QApplication::focusChanged, this, [&]() { update(); });
}

bool HighlightBorderEffect::eventFilter(QObject* watched, QEvent* event)
{
    return QGraphicsEffect::eventFilter(watched, event);
}

void HighlightBorderEffect::draw(QPainter* painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);

    if (pixmap.isNull())
        return;

    bool focused = isFocused();
    bool thick_line = focused;

    QPainter p(&pixmap);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Utils::informationTypeColor(_type), thick_line ? 2 : 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QRect rect;
    if (qobject_cast<PlainTextEdit*>(_widget->parentWidget()) != nullptr) {
        /* Очень странная ситуация
         * В PlainTextEdit сдвинут viewport. Если просто отрисовывать рамку только на viport или на самом виджете, то она не отрисовывается полностью.
         * Это понятно, т.к. сдвинутый viewport смещает его рамку и еще перекрывает рамку на самом виджете. 
         * Но если добавить HighlightBorderEffect на оба, при этом не отрисовывая тут рамку на viewport, то все работает как надо.
         * В общем работает и ладно.
         */
    } else {
        if (thick_line)
            rect = QRect(1, 1, pixmap.width() - 2, pixmap.height() - 2);
        else
            rect = QRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    }

    p.drawRect(rect);

    p.drawPixmap(0, 0, pixmap);
    painter->drawPixmap(offset, pixmap);
}

bool HighlightBorderEffect::isFocused() const
{
    if (_widget->hasFocus())
        return true;

    QWidget* child = qApp->focusWidget();
    while (child != nullptr) {
        if (child == _widget)
            return true;
        child = child->parentWidget();
    }

    return false;
}

GlowEffect::GlowEffect(QObject* parent)
    : QGraphicsEffect(parent)
{
}

void GlowEffect::setColor(QColor value)
{
    _color = value;
}

void GlowEffect::setStrength(int value)
{
    _strength = value;
}

void GlowEffect::setBlurRadius(qreal value)
{
    _blur_radius = value;
    _extent = qCeil(value);
    updateBoundingRect();
}

QColor GlowEffect::color() const
{
    return _color;
}

int GlowEffect::strength() const
{
    return _strength;
}

qreal GlowEffect::blurRadius() const
{
    return _blur_radius;
}

QRectF GlowEffect::boundingRectFor(const QRectF& rect) const
{
    return QRect(rect.left() - _extent, rect.top() - _extent, rect.width() + 2 * _extent, rect.height() + 2 * _extent);
}

void GlowEffect::draw(QPainter* painter)
{
    QPoint offset;
    QPixmap source = sourcePixmap(Qt::LogicalCoordinates, &offset);
    QPixmap glow;

    QGraphicsColorizeEffect* colorize = new QGraphicsColorizeEffect;
    colorize->setColor(_color);
    colorize->setStrength(1);
    glow = applyEffectToPixmap(source, colorize, 0);

    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(_blur_radius);
    glow = applyEffectToPixmap(glow, blur, _extent);

    for (int i = 0; i < _strength; i++)
        painter->drawPixmap(offset - QPoint(_extent, _extent), glow);
    drawSource(painter);
}

QPixmap GlowEffect::applyEffectToPixmap(QPixmap src, QGraphicsEffect* effect, int extent)
{
    if (src.isNull())
        return QPixmap();
    if (!effect)
        return src;
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(src);
    item.setGraphicsEffect(effect);
    scene.addItem(&item);
    QSize size = src.size() + QSize(extent * 2, extent * 2);
    QPixmap res(size.width(), size.height());
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(&ptr, QRectF(), QRectF(-extent, -extent, size.width(), size.height()));
    return res;
}

SelectBorderEffect::SelectBorderEffect(QWidget* widget, QColor color, int size)
    : QGraphicsEffect(widget)
    , _color(color)
    , _size(size)
{
}

void SelectBorderEffect::draw(QPainter* painter)
{
    QPoint offset;
    QPixmap pixmap = sourcePixmap(Qt::LogicalCoordinates, &offset, QGraphicsEffect::NoPad);

    if (pixmap.isNull())
        return;

    QPainter p(&pixmap);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(_color, _size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    QRect rect = QRect(_size / 2, _size / 2, pixmap.width() - _size, pixmap.height() - _size);

    p.drawRect(rect);

    p.drawPixmap(0, 0, pixmap);
    painter->drawPixmap(offset, pixmap);
}

} // namespace zf
