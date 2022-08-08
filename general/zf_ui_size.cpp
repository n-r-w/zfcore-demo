#include "zf_ui_size.h"
#include "zf_core.h"

namespace zf
{
void UiSizes::bootstrap()
{
}

int UiSizes::defaultTableRowHeight()
{
    return qApp->style()->pixelMetric(QStyle::PM_HeaderDefaultSectionSizeVertical);
}

QSize UiSizes::defaultLineEditSize(const QFont f)
{
    // выдрано из кода Qt
    QFontMetrics fm(f);
    QStyle* style = qApp->style();
    const int iconSize = style->pixelMetric(QStyle::PM_SmallIconSize);
    QMargins tm = {0, 0, 0, 0}; // ?

    int h = qMax(fm.height(), qMax(14, iconSize - 2)) + 2 * 1 + tm.top() + tm.bottom();
    int w = fm.horizontalAdvance(QLatin1Char('x')) * 17 + 2 * 2 + tm.left() + tm.right();

    return {w + 2, h + 6};
}

int UiSizes::defaultEditorToolButtonSize(const QFont f)
{
    return defaultLineEditSize(f).height() - 4;
}

int UiSizes::defaultEditorIconSize(const QFont f)
{
    return defaultEditorToolButtonSize(f) - 1;
}

int UiSizes::defaultToolbarIconSize(ToolbarType type)
{
    switch (type) {
        case ToolbarType::Main:
            return Z_PM(PM_ToolBarIconSize);
        case ToolbarType::Window:
            return Z_PM(PM_ToolBarIconSize);
        case ToolbarType::Internal:
            return Utils::scaleUI(24);
    }
    Z_HALT_INT;
    return 0;
}

void UiSizes::prepareEditorToolButton(const QFont f, QToolButton* b, bool set_maximum_size, int delta)
{
    Z_CHECK_NULL(b);
    int bs = defaultEditorToolButtonSize(f);
    if (set_maximum_size) {
        b->setMaximumWidth(bs);
        b->setMaximumHeight(bs);

    } else {
        b->setGeometry(b->geometry().left(), b->geometry().top(), bs, bs);
    }

    int is = defaultEditorIconSize(f);
    b->setIconSize(QSize(is + delta, is + delta));
}

} // namespace zf
