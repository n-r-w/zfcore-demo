#include "zf_webview_widget.h"
#include "zf_webengineview.h"
#include "zf_utils.h"

#include "zf_translation.h"
#include "zf_translator.h"

#include <QWebEngineSettings>
#include <QVBoxLayout>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QContextMenuEvent>

namespace zf
{
WebViewWidget::WebViewWidget(WebEngineViewInjection* injection, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* la = new QVBoxLayout;
    la->setObjectName(QStringLiteral("zfla"));
    la->setMargin(0);
    setLayout(la);

    _view = new WebEngineView(injection);
    _view->setObjectName(QStringLiteral("zf_view"));
    _view->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    _view->settings()->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
#endif
    la->addWidget(_view);

    connect(_view, &WebEngineView::sg_load_finished, this, &WebViewWidget::sl_load_finished);
    connect(_view, &WebEngineView::sg_webKeyPressed, this, &WebViewWidget::sg_webKeyPressed);
    connect(_view, &WebEngineView::sg_webMouseClicked, this, &WebViewWidget::sg_webMouseClicked);
    connect(_view, &WebEngineView::sg_currentPageChanged, this, &WebViewWidget::sg_currentPageChanged);
}

zf::Error WebViewWidget::loadFile(const QString& file_name)
{
    if (!QFile::exists(file_name))
        return zf::Error::fileNotFoundError(file_name);

    _view->setUrl(QUrl::fromLocalFile(file_name));

    if (injection() != nullptr)
        injection()->setPdfView(comp(QFileInfo(file_name).suffix(), "pdf"));

    return zf::Error();
}

void WebViewWidget::loadHtmlString(const QString& content)
{
    if (injection() != nullptr)
        injection()->setPdfView(false);

    _view->setHtml(content);
}

void WebViewWidget::loadHtmlUrl(const QUrl& url)
{
    if (injection() != nullptr)
        injection()->setPdfView(comp(url.fileName().right(4), ".pdf"));

    _view->setUrl(url);
}

Error WebViewWidget::setFittingType(FittingType type)
{
    return _view->setFittingType(type);
}

Error WebViewWidget::zoomIn()
{
    return _view->zoomIn();
}

Error WebViewWidget::zoomOut()
{
    return _view->zoomOut();
}

Error WebViewWidget::rotateRight()
{
    return _view->rotateRight();
}

Error WebViewWidget::rotateLeft()
{
    return _view->rotateLeft();
}

Error WebViewWidget::setCurrentPage(int page)
{
    return _view->setCurrentPage(page);
}

WebEngineView* WebViewWidget::view() const
{
    return _view;
}

WebEngineViewInjection* WebViewWidget::injection() const
{
    return _view != nullptr ? _view->injection() : nullptr;
}

void WebViewWidget::sl_load_finished(bool ok)
{
    emit sg_load_finished(ok ? Error() : Error(translate(TR::ZFT_ERROR)));
}
} // namespace zf
