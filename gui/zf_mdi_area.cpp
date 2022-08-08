#include "zf_mdi_area.h"
#include <QScrollBar>
#include <QEvent>
#include <QStyle>

namespace zf
{
MdiArea::MdiArea(QWidget* parent)
    : QMdiArea(parent)
{
    QObject::connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [&]() { updateBackgroundPosition(); });
    QObject::connect(
        verticalScrollBar(), &QScrollBar::rangeChanged, this, [&]() { updateBackgroundPosition(); }, Qt::QueuedConnection);
    QObject::connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [&]() { updateBackgroundPosition(); });
    QObject::connect(
        horizontalScrollBar(), &QScrollBar::rangeChanged, this, [&]() { updateBackgroundPosition(); }, Qt::QueuedConnection);
}

MdiArea::~MdiArea()
{
    if (!_background_widget.isNull()) {
        _background_widget->setParent(nullptr);
        _background_widget.clear();
    }
}

QWidget* MdiArea::setBackgroundWidget(QWidget* w)
{
    QWidget* old = _background_widget;
    if (old != nullptr) {
        old->setHidden(true);
        old->setParent(nullptr);
    }

    _background_widget = w;
    if (!_background_widget.isNull()) {
        _background_widget->setParent(viewport());
        _background_widget->lower();
        _background_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _background_widget->show();
    }
    updateBackgroundPosition();

    return old;
}

QWidget* MdiArea::backgroundWidget() const
{
    return _background_widget;
}

bool MdiArea::viewportEvent(QEvent* e)
{
    bool res = QMdiArea::viewportEvent(e);

    if (e->type() == QEvent::Resize)
        updateBackgroundPosition();

    return res;
}

void MdiArea::resizeEvent(QResizeEvent* e)
{
    QMdiArea::resizeEvent(e);
    updateBackgroundPosition();
}

void MdiArea::updateBackgroundPosition()
{
    if (!_background_widget.isNull()) {
        QSize max_size = maximumViewportSize();
        QSize hbar_extent = horizontalScrollBar()->sizeHint();
        QSize vbar_extent = verticalScrollBar()->sizeHint();

        if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0)) {
            const int double_frame_width = frameWidth() * 2;
            if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
                max_size.rheight() -= double_frame_width;
            if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
                max_size.rwidth() -= double_frame_width;
            hbar_extent.rheight() += double_frame_width;
            vbar_extent.rwidth() += double_frame_width;
        }

        _background_widget->setGeometry(0, 0, width() - (verticalScrollBar()->isVisible() ? vbar_extent.width() : 0),
                                        height() - (horizontalScrollBar()->isVisible() ? hbar_extent.height() : 0));
    }
}

} // namespace zf
