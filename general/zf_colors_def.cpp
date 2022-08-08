#include "zf_colors_def.h"
#include <QApplication>

namespace zf
{
void Colors::bootstrap()
{
}

QColor Colors::readOnlyBackground()
{
    qreal h, s, l, a;
    qApp->palette().color(QPalette::Window).getHslF(&h, &s, &l, &a);
    l = (1.0 - l) * 0.3 + l;
    QColor res;
    res.setHslF(h, s, l, a);
    return res;
}

QColor Colors::background()
{
    return qApp->palette().color(QPalette::Base);
}

QColor Colors::uiLineColor(bool bold)
{
    return qApp->palette().color(bold ? QPalette::Mid : QPalette::Midlight);
}

QColor Colors::uiWindowColor()
{
    return qApp->palette().color(QPalette::Normal, QPalette::Window);
}

QColor Colors::uiButtonColor()
{
    return qApp->palette().color(QPalette::Normal, QPalette::Button);
}

QColor Colors::uiDarkColor()
{
    return qApp->palette().color(QPalette::Normal, QPalette::Dark);
}

QColor Colors::uiAlternateTableBackgroundColor()
{
    return QColor(252, 248, 227);
}

QColor Colors::uiInfoTableBackgroundColor()
{
    return QColor(255, 214, 0);
}

QColor Colors::uiInfoTableTextColor()
{
    return QColor(162, 0, 37);
}

} // namespace zf
