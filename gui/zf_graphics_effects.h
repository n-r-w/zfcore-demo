#pragma once

#include "zf.h"
#include <QGraphicsEffect>

namespace zf
{
//! Отрисовка рамки с ошибкой вокруг виджета
class ZCORESHARED_EXPORT HighlightBorderEffect : public QGraphicsEffect
{
public:
    HighlightBorderEffect(QWidget* widget, InformationType type);

    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void draw(QPainter* painter) override;

private:
    bool isFocused() const;

    QWidget* _widget;
    InformationType _type;
};

//! Отрисовка рамки с выделением вокруг виджета
class ZCORESHARED_EXPORT SelectBorderEffect : public QGraphicsEffect
{
public:
    SelectBorderEffect(QWidget* widget, QColor color, int size);

protected:
    void draw(QPainter* painter) override;

private:
    QColor _color;
    int _size;
};

//! Эффект свечения
class ZCORESHARED_EXPORT GlowEffect : public QGraphicsEffect
{
public:
    explicit GlowEffect(QObject* parent = 0);

    QRectF boundingRectFor(const QRectF& rect) const;
    void setColor(QColor value);
    void setStrength(int value);
    void setBlurRadius(qreal value);
    QColor color() const;
    int strength() const;
    qreal blurRadius() const;

protected:
    void draw(QPainter* painter);

private:
    static QPixmap applyEffectToPixmap(QPixmap src, QGraphicsEffect* effect, int extent);
    int _extent = 5;
    QColor _color = QColor(255, 255, 255);
    int _strength = 3;
    qreal _blur_radius = 5.0;
};

} // namespace zf
