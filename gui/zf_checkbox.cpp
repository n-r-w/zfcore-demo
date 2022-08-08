#include "zf_checkbox.h"
#include "zf_core.h"
#include "zf_utils.h"
#include <QStylePainter>
#include <QApplication>

namespace zf
{
CheckBox::CheckBox(QWidget* parent)
    : QCheckBox(parent)
{
}

CheckBox::CheckBox(const QString& text, QWidget* parent)
    : QCheckBox(text, parent)
{
}

void CheckBox::paintEvent(QPaintEvent* e)
{
    if (isEnabled()) {
        QCheckBox::paintEvent(e);
        return;
    }

    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);

    // делаем символ чека не затемненным
    opt.state = opt.state | QStyle::State_Enabled | QStyle::State_Active;
    opt.state = opt.state & ~(QStyle::State_ReadOnly);

    opt.palette.setColor(QPalette::Text, qApp->palette().color(QPalette::Text));
    opt.palette.setColor(QPalette::WindowText, qApp->palette().color(QPalette::Text));

    p.drawControl(QStyle::CE_CheckBox, opt);
}

} // namespace zf
