#pragma once

#include <QApplication>
#include <QProxyStyle>

#include "zf_global.h"

namespace zf
{
class ZCORESHARED_EXPORT ProxyStyle : public QProxyStyle
{
public:
    ProxyStyle(QStyle *style = nullptr);
    ProxyStyle(const QString &key);

    QSize sizeFromContents(ContentsType ct, const QStyleOption* opt, const QSize& csz, const QWidget* widget = 0) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt, SubControl sc, const QWidget* widget) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override;
    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter,
                            const QWidget* widget = nullptr) const override;
    QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* opt = nullptr, const QWidget* widget = nullptr) const override;
    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption* opt = nullptr, const QWidget* widget = nullptr) const override;
    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const override;
};

} // namespace zf
