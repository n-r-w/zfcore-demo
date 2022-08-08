#include "zf_pdf_view.h"
#include "zf_core.h"
#include <QPdfDocument>
#include <QPdfPageNavigation>
#include <QScreen>

namespace zf
{
PdfView::PdfView(QWidget* parent)
    : QPdfView(parent)
{
    setPageMode(PageMode::MultiPage);
}

PdfView::~PdfView()
{
}

void PdfView::scale(qreal factor)
{
    setZoomFactor(zoomFactor() * factor);
}

void PdfView::setZoomMode(QPdfView::ZoomMode mode)
{
    if (mode == zoomMode())
        return;

    QPdfView::setZoomMode(mode);
    QPdfView::setZoomFactor(zoomFactor());
}

qreal PdfView::zoomFactor() const
{
    if (zoomMode() == CustomZoom)
        return QPdfView::zoomFactor();

    // криворукие дятлы из Qt не подумали что масштаб надо определять всегда
    const qreal screen_resolution = QGuiApplication::primaryScreen()->logicalDotsPerInch() / 72.0;

    QSizeF p_size = document()->pageSize(pageNavigation()->currentPage());
    auto vp_size = viewport()->size();
    if (p_size.isEmpty() || vp_size.width() == 0)
        return 1;

    if (zoomMode() == QPdfView::FitToWidth) {
        QSize page_size = QSizeF(p_size * screen_resolution).toSize();
        return (qreal(vp_size.width() - documentMargins().left() - documentMargins().right()) / qreal(page_size.width()));

    } else if (zoomMode() == QPdfView::FitInView) {
        const QSize viewport_size_x(vp_size + QSize(-documentMargins().left() - documentMargins().right(), -pageSpacing()));
        const QSize viewport_size_y(vp_size + QSize(-documentMargins().top() - documentMargins().bottom(), -pageSpacing()));

        QSize page_size = QSizeF(p_size * screen_resolution).toSize();
        QSize page_size_x = page_size.scaled(viewport_size_x, Qt::KeepAspectRatio);
        QSize page_size_y = page_size.scaled(viewport_size_y, Qt::KeepAspectRatio);

        qreal zoom_x = qreal(page_size_x.width()) / (qreal(vp_size.width() - documentMargins().left() - documentMargins().right()));
        qreal zoom_y = qreal(page_size_y.height()) / (qreal(vp_size.height() - documentMargins().top() - documentMargins().bottom()));
        return (zoom_x + zoom_y) / 2.0;

    } else {
        Z_HALT_INT;
        return 1;
    }
}

void PdfView::setZoomFactor(qreal factor)
{
    // травокуры из Qt внутри QPdfView сравнивают два qreal на равенство
    auto current_zoom = zoomFactor();
    if (qAbs(current_zoom - factor) <= 0.01)
        return;

    QPdfView::setZoomMode(PdfView::ZoomMode::CustomZoom);
    QPdfView::setZoomFactor(factor);
}
} // namespace zf
