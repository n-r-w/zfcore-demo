#include "zf_uniview_widget.h"
#include "zf_webview_widget.h"
#include "zf_webengineview.h"
#include <QFileInfo>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QResource>
#include "zf_core.h"
#include "zf_picture_view.h"

#include "zf_pdf_view.h"
#include <QPdfPageNavigation>
#include <QPdfDocument>

#ifdef Q_OS_WINDOWS
#include "mso/zf_mswidget.h"
#endif

namespace zf
{
UniViewWidget::UniViewWidget(WebEngineViewInjection* web_injection, QWidget* parent)
    : QWidget(parent)
    , _web_injection(web_injection)
{
    _layout = new QVBoxLayout;
    _layout->setObjectName(QStringLiteral("zfla"));
    _layout->setMargin(0);
    setLayout(_layout);
}

UniViewWidget::~UniViewWidget()
{
    _destructing = true;
    clear();
}

bool UniViewWidget::hasData() const
{
#ifdef Q_OS_WINDOWS
    if (_ms_widget != nullptr)
        return true;
#endif
    if (_web_widget != nullptr || _text_widget != nullptr || _picture_widget != nullptr || _pdf_view != nullptr)
        return true;

    return false;
}

void UniViewWidget::clear()
{
    prepare(nullptr);
    clearHelper();    
    if (!_destructing)
        setParameters({}, {});
}

bool UniViewWidget::isValid() const
{
    return _web_widget || _text_widget || _picture_widget || _pdf_view
#ifdef Q_OS_WINDOWS
           || _ms_widget
#endif
        ;
}

bool UniViewWidget::isLoading() const
{
    return _is_loading;
}

bool UniViewWidget::isWebView() const
{
    return view() != nullptr;
}

WebEngineView* UniViewWidget::view() const
{
    return _web_widget ? _web_widget->view() : nullptr;
}

bool UniViewWidget::isChromiumPdfView() const
{
    return _web_widget == nullptr ? false : _web_widget->injection()->isPdfView();
}

bool UniViewWidget::isChromiumHidePdfToolbar() const
{
    return _web_widget == nullptr ? false : _web_widget->injection()->isHidePdfToolbar();
}

bool UniViewWidget::isChromiumHidePdfZoom() const
{
    return _web_widget == nullptr ? false : _web_widget->injection()->isHidePdfZoom();
}

Error UniViewWidget::setChromiumHidePdfToolbar(bool b)
{
    if (_web_widget == nullptr || !_web_widget->injection()->isPdfView())
        return Error::unsupportedError();

    _web_widget->injection()->setHidePdfToolbar(b);
    return {};
}

Error UniViewWidget::setChromiumHidePdfZoom(bool b)
{
    if (_web_widget == nullptr || !_web_widget->injection()->isPdfView())
        return Error::unsupportedError();

    _web_widget->injection()->setHidePdfZoom(b);
    return {};
}

void UniViewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateInternalGeometry();
}

bool UniViewWidget::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateInternalGeometry();
            break;
        default:
            break;
    }

    return QWidget::event(event);
}

void UniViewWidget::sl_web_load_finished(const Error& error)
{
    finishLoad(error);
}

void UniViewWidget::sl_pdf_statusChanged(QPdfDocument::Status status)
{
    Q_UNUSED(status)
}

void UniViewWidget::sl_pdf_zoomModeChanged(QPdfView::ZoomMode zoomMode)
{
    Q_UNUSED(zoomMode)
    emit sg_zoomScaleChanged(_pdf_view->zoomFactor());
}

Error UniViewWidget::loadHtmlUrl_helper(const QUrl& url)
{
    if (!_is_loading)
        startLoad();

    createWebViewWidget();
    _web_widget->loadHtmlUrl(url);

    UniviewParameters params;
    if (comp(url.fileName().right(4), ".pdf"))
        params = pdfChromiumParameters();

    setParameters(params);

    return Error();
}

Error UniViewWidget::loadFile_helper(const QString& file_name, FileType type, UniViewWidgetOptions options)
{
    if (type == FileType::Undefined)
        type = Utils::fileTypeFromExtension(file_name);

    if (type == FileType::Undefined)
        return Error::unsupportedError();

    if (Utils::isImage(type))
        return loadIconFile_helper(file_name, type);

    if (type == FileType::Html || type == FileType::Plain || type == FileType::Json || type == FileType::Xml)
        return loadTextFile_helper(file_name, type, options.testFlag(UniViewWidgetOption::Editable));

    if (type == FileType::Pdf)
        return loadPdfFile_helper(file_name);

    if (type == FileType::Rtf || type == FileType::Doc || type == FileType::Docx || type == FileType::Xls || type == FileType::Xlsx)
        return loadMsOfficeFile_helper(file_name, type, options);

    return Error::unsupportedError();
}

Error UniViewWidget::loadTextFile_helper(const QString& file_name, FileType type, bool editable)
{
    if (type == FileType::Undefined)
        type = Utils::fileTypeFromExtension(QFileInfo(file_name).suffix());
    if (type == FileType::Undefined)
        return Error::unsupportedError();

    QFile file(file_name);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return Error::fileIOError(file_name);

    QTextStream st(&file);

    return loadTextData_helper(st.readAll(), type, editable);
}

Error UniViewWidget::loadMsOfficeFile_helper(const QString& file_name, FileType type, UniViewWidgetOptions options)
{
#ifdef Q_OS_LINUX
    Q_UNUSED(options);
    options.setFlag(UniViewWidgetOption::MsOfficeClone, false);
    options.setFlag(UniViewWidgetOption::Editable, false);
#endif

    Error error;
    if (type == FileType::Undefined)
        type = Utils::fileTypeFromExtension(file_name);

    if (!(type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf || type == FileType::Xlsx || type == FileType::Xls))
        return Error::unsupportedError();

    if (QResource(file_name).isValid()) {
        // это ресурс Qt. Надо предварительно скопировать во временный файл
        QByteArray data;
        error = Utils::loadByteArray(file_name, data);
        if (error.isError())
            return error;

        return loadMsOfficeData_helper(data, type);
    }

    const bool to_pdf =
#ifdef Q_OS_LINUX
        true;

#else
        (options.testFlag(UniViewWidgetOption::ExcelToPdf) && (type == FileType::Xlsx || type == FileType::Xls))
        || (options.testFlag(UniViewWidgetOption::WordToPdf) && (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf));
#endif

    if (to_pdf) {
        QString file_name_pdf_converted = Utils::generateTempFileName("pdf");
        if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf)
            error = Utils::wordToPdf(file_name, file_name_pdf_converted);
        else if (type == FileType::Xlsx || type == FileType::Xls) //-V560
            error = Utils::excelToPdf(file_name, file_name_pdf_converted);
        else
            Z_HALT_INT;

        if (error.isError())
            return error;

        error = loadPdfFile_helper(file_name_pdf_converted);
        Utils::removeFile(file_name_pdf_converted);

        return error;
    }

    if (options.testFlag(UniViewWidgetOption::MsOfficeClone)) {
        // Если пытаться открыть один и тот же файл два раза, то MSOControl сходит с ума
        QString temp_dir = Utils::generateTempDirName();
        QString temp_file = temp_dir + "/" + QFileInfo(file_name).fileName();
        error = Utils::copyFile(file_name, temp_file);
        if (error.isError()) {
            Utils::removeFile(temp_file);
            return error;
        }
        _to_remove_dirs << temp_dir;
        _to_remove_files << temp_file;

        options.setFlag(UniViewWidgetOption::MsOfficeClone, false);
        return loadMsOfficeFile_helper(temp_file, type, options);
    }

#if defined(Q_OS_WINDOWS)   
    startLoad();
    createMSWidget();

    if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf)
        error = _ms_widget->loadWord(file_name, options.testFlag(UniViewWidgetOption::Editable));
    else
        error = _ms_widget->loadExcel(file_name, options.testFlag(UniViewWidgetOption::Editable));

    if (error.isError()) {
        prepare(nullptr);
        finishLoad(error);
        return error;
    }

    finishLoad({});

    setParameters({}, {});
#endif

    return {};
}

Error UniViewWidget::loadPdfFile_helper(const QString& file_name)
{
    QByteArray data;
    auto error = Utils::loadByteArray(file_name, data);
    if (error.isError())
        return error;

    return loadPdfData_helper(data);
}

Error UniViewWidget::loadIconFile_helper(const QString& file_name, FileType type)
{
    if (type == FileType::Undefined)
        type = Utils::fileTypeFromExtension(QFileInfo(file_name).suffix());
    if (type == FileType::Undefined)
        return Error::unsupportedError();

    QByteArray data;
    auto error = Utils::loadByteArray(file_name, data);
    if (error.isError())
        return error;

    return loadIconData_helper(data, type);
}

Error UniViewWidget::loadData_helper(const QByteArray& data, FileType type, UniViewWidgetOptions options)
{
    if (type == FileType::Undefined)
        return Error::unsupportedError();

    if (Utils::isImage(type))
        return loadIconData_helper(data, type);

    if (type == FileType::Html || type == FileType::Plain || type == FileType::Json || type == FileType::Xml)
        return loadTextData_helper(data, type, options.testFlag(UniViewWidgetOption::Editable));

    if (type == FileType::Pdf)
        return loadPdfData_helper(data);

    if (type == FileType::Rtf || type == FileType::Doc || type == FileType::Docx || type == FileType::Xls || type == FileType::Xlsx)
        return loadMsOfficeData_helper(data, type
#ifdef Q_OS_WINDOWS
                                       ,
                                       options.testFlag(UniViewWidgetOption::WordToPdf) || options.testFlag(UniViewWidgetOption::ExcelToPdf)
#endif
        );

    return Error::unsupportedError();
}

Error UniViewWidget::loadTextData_helper(const QString& string, FileType type, bool editable)
{
    if (!(type == FileType::Html || type == FileType::Plain || type == FileType::Json || type == FileType::Xml))
        return Error::unsupportedError();

    if (type == FileType::Plain || type == FileType::Json || type == FileType::Xml) {
        startLoad();
        createTextView();

        _text_widget->setText(string);
        _text_widget->setReadOnly(!editable);

        finishLoad({});
        setParameters({}, {});

    } else if (type == FileType::Html) {
        startLoad();
        createWebViewWidget();

        _web_widget->loadHtmlString(string);
        setParameters({}, {});
    }

    return Error();
}

Error UniViewWidget::loadMsOfficeData_helper(const QByteArray& data, FileType type
#ifdef Q_OS_WINDOWS
                                             ,
                                             bool to_pdf
#endif
)
{
    if (!(type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf || type == FileType::Xlsx || type == FileType::Xls))
        return Error::unsupportedError();

        // офисные файлы можно показывать только через сохранение в файл
#ifdef Q_OS_LINUX
    const bool to_pdf = true;
#endif

    QString file_name_origin = Utils::generateTempFileName(Utils::fileExtensions(type).constFirst());
    auto error = Utils::saveByteArray(file_name_origin, data);
    if (error.isError())
        return error;

    if (to_pdf) {
        QString file_name_pdf_converted = Utils::generateTempFileName("pdf");
        if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf)
            error = Utils::wordToPdf(file_name_origin, file_name_pdf_converted);
        else if (type == FileType::Xlsx || type == FileType::Xls) //-V560
            error = Utils::excelToPdf(file_name_origin, file_name_pdf_converted);
        else
            Z_HALT_INT;

        if (error.isError())
            return error;

        error = loadPdfFile_helper(file_name_pdf_converted);

        Utils::removeFile(file_name_pdf_converted);
        Utils::removeFile(file_name_origin);

        return error;
    }

    _to_remove_files << file_name_origin;
    return loadMsOfficeFile_helper(file_name_origin, type, UniViewWidgetOptions());
}

Error UniViewWidget::loadPdfData_helper(const QByteArray& data)
{
    startLoad();
    createPdfView();

    if (_pdf_data_read_bufer == nullptr)
        _pdf_data_read_bufer = new QBuffer;
    else
        _pdf_data_read_bufer->close();

    if (_pdf_data == nullptr)
        _pdf_data = new QByteArray(data);
    else
        *_pdf_data = data;

    _pdf_data_read_bufer->setData(*_pdf_data);
    _pdf_data_read_bufer->open(QBuffer::ReadOnly);

    _pdf_doc->load(_pdf_data_read_bufer);

    if (_pdf_doc->error() != QPdfDocument::NoError) {
        auto error = Error::badFileError(_pdf_data_read_bufer);
        _pdf_data_read_bufer->close();
        finishLoad(error);
        return error;
    }

    setParameters(UniviewParameter::ZoomStep | UniviewParameter::AutoZoom | UniviewParameter::ReadPageNo | UniviewParameter::SetPageNo);
    finishLoad({});

    // даем расчитать размеры
    QTimer::singleShot(0, this, [this]() {
        emit sg_zoomScaleChanged(_pdf_view->zoomFactor());
        emit sg_currentPageChanged(1, _pdf_doc->pageCount());
    });

    return {};
}

Error UniViewWidget::loadIconData_helper(const QIcon& icon)
{
    if (icon.isNull()) {
        startLoad();
        prepare(nullptr);
        finishLoad({});
        return {};
    }

    startLoad();
    createPictureViewer();
    _picture_widget->setIcon(icon);

    setParameters(UniviewParameter::ZoomPercent | UniviewParameter::RotateStep | UniviewParameter::ZoomStep | UniviewParameter::AutoZoom);
    finishLoad({});
    emit sg_zoomScaleChanged(_picture_widget->scaleFactor());

    return Error();
}

Error UniViewWidget::loadIconData_helper(const QByteArray& data, const QString& file_type)
{
    if (data.isEmpty()) {
        startLoad();
        prepare(nullptr);
        finishLoad({});
        return {};
    }

    startLoad();
    createPictureViewer();
    Error error = _picture_widget->setFileData(data, file_type);
    if (error.isError()) {
        delete _picture_widget;
        _picture_widget = nullptr;
        finishLoad(error);
        setParameters({}, {});
        return error;
    }

    setParameters(UniviewParameter::ZoomPercent | UniviewParameter::RotateStep | UniviewParameter::ZoomStep | UniviewParameter::AutoZoom);
    finishLoad({});
    emit sg_zoomScaleChanged(_picture_widget->scaleFactor());

    return {};
}

Error UniViewWidget::loadIconData_helper(const QByteArray& data, FileType file_type)
{
    Z_CHECK(file_type != FileType::Undefined);
    return loadIconData_helper(data, Utils::fileExtensions(file_type).constFirst());
}

void UniViewWidget::startLoad()
{
    _is_loading = true;
    emit sg_loadStarted();
}

void UniViewWidget::finishLoad(const Error& error)
{
    _is_loading = false;

    if (_fitting_type != FittingType::Undefined)
        setFittingTypeHelper(_fitting_type);

    emit sg_loadFinished(error);
}

void UniViewWidget::updateInternalGeometry()
{
    if (_fitting_type == FittingType::Undefined)
        return;

    setFittingTypeHelper(_fitting_type);
}

void UniViewWidget::clearHelper()
{
    if (!_to_remove_files.isEmpty()) {
        for (auto& f : qAsConst(_to_remove_files)) {
            Utils::removeFile(f);
        }
    }
    if (!_to_remove_dirs.isEmpty()) {
        for (auto& f : qAsConst(_to_remove_dirs)) {
            Utils::removeDir(f);
        }
    }

    _to_remove_files.clear();
    _to_remove_dirs.clear();

    _options = {};
    _is_loading = false;
}

void UniViewWidget::setParameters(UniviewParameters supported_params, UniviewParameters available_params)
{
    if (_supported_params == supported_params && _available_params == available_params)
        return;
    _supported_params = supported_params;
    _available_params = available_params;
    emit sg_parametersChanged();
}

void UniViewWidget::setParameters(UniviewParameters params)
{
    setParameters(params, params);
}

void UniViewWidget::prepare(QWidget* keep)
{
#ifdef Q_OS_WINDOWS
    if (keep != _ms_widget) {
        if (_ms_widget != nullptr)
            delete _ms_widget;
        _ms_widget = nullptr;
    }
#endif
    if (keep != _web_widget) {
        if (_web_widget != nullptr)
            delete _web_widget;
        _web_widget = nullptr;
    }
    if (keep != _text_widget) {
        if (_text_widget != nullptr)
            delete _text_widget;
        _text_widget = nullptr;
    }
    if (keep != _picture_widget) {
        if (_picture_widget != nullptr)
            delete _picture_widget;
        _picture_widget = nullptr;
    }

    if (keep != _pdf_view) {
        if (_pdf_view != nullptr)
            delete _pdf_view;
        _pdf_view = nullptr;

        if (_pdf_doc != nullptr)
            delete _pdf_doc;
        _pdf_doc = nullptr;

        if (_pdf_data_read_bufer != nullptr)
            delete _pdf_data_read_bufer;
        _pdf_data_read_bufer = nullptr;

        if (_pdf_data != nullptr)
            delete _pdf_data;
        _pdf_data = nullptr;
    }
}

void UniViewWidget::createPictureViewer()
{
    prepare(_picture_widget);
    if (_picture_widget == nullptr) {
        _picture_widget = new PictureViewer;
        _picture_widget->setObjectName("zf_picture_view");
        _layout->addWidget(_picture_widget);
        connect(_picture_widget, &PictureViewer::sg_scaleChanged, this, &UniViewWidget::sg_zoomScaleChanged);
        connect(_picture_widget, &PictureViewer::sg_roteated, this, &UniViewWidget::sg_roteated);
    }
}

void UniViewWidget::createWebViewWidget()
{
    prepare(_web_widget);
    if (_web_widget == nullptr) {
        _web_widget = new WebViewWidget(_web_injection);
        _web_widget->setObjectName(QStringLiteral("zf_web_widget"));
        _layout->addWidget(_web_widget);

        connect(_web_widget, &WebViewWidget::sg_load_finished, this, &UniViewWidget::sl_web_load_finished);
        connect(_web_widget, &WebViewWidget::sg_webKeyPressed, this, &UniViewWidget::sg_webKeyPressed);
        connect(_web_widget, &WebViewWidget::sg_webMouseClicked, this, &UniViewWidget::sg_webMouseClicked);
        connect(_web_widget, &WebViewWidget::sg_currentPageChanged, this, &UniViewWidget::sg_currentPageChanged);        
    }
}

void UniViewWidget::createPdfView()
{
    prepare(_pdf_view);
    if (_pdf_view == nullptr) {
        _pdf_doc = new QPdfDocument(this);

        _pdf_view = new PdfView;
        _pdf_view->setDocument(_pdf_doc);
        _pdf_view->setObjectName(QStringLiteral("zf_pdf_widget"));
        _pdf_view->setZoomMode(PdfView::FitInView);
        _pdf_view->setZoomFactor(1);

        _layout->addWidget(_pdf_view);

        connect(_pdf_doc, &QPdfDocument::statusChanged, this, &UniViewWidget::sl_pdf_statusChanged);
        connect(_pdf_view, &PdfView::zoomFactorChanged, this, &UniViewWidget::sg_zoomScaleChanged);
        connect(_pdf_view, &PdfView::zoomModeChanged, this, &UniViewWidget::sl_pdf_zoomModeChanged);
        connect(_pdf_view->pageNavigation(), &QPdfPageNavigation::currentPageChanged, this,
                [this](int currentPage) { emit sg_currentPageChanged(currentPage + 1, _pdf_doc->pageCount()); });
    }
}

void UniViewWidget::createTextView()
{
    prepare(_text_widget);
    if (_text_widget == nullptr) {
        _text_widget = new QTextEdit;
        _text_widget->setObjectName(QStringLiteral("zf_text_widget"));
        _text_widget->setFrameShape(QFrame::NoFrame);
        _layout->addWidget(_text_widget);
    }
}

#ifdef Q_OS_WINDOWS
void UniViewWidget::createMSWidget()
{
    // MSWidget приходится удалять полностью, т.к. иначе имеются загадочные глюки с захватом вордом фокуса ввода
    prepare(nullptr);
    _ms_widget = new MSWidget;
    _ms_widget->setObjectName(QStringLiteral("zf_ms_widget"));
    _layout->addWidget(_ms_widget);
}
#endif

Error UniViewWidget::setFittingTypeHelper(FittingType type)
{
    Error error;

    if (!_supported_params.testFlag(UniviewParameter::AutoZoom)) {
        error = Error::unsupportedError();

    } else if (_web_widget != nullptr) {
        error = _web_widget->setFittingType(type);

    } else if (_pdf_view != nullptr) {
        if (type == FittingType::Height)
            _pdf_view->setZoomMode(QPdfView::FitInView);
        else if (type == FittingType::Page)
            _pdf_view->setZoomMode(QPdfView::FitInView);
        else if (type == FittingType::Width)
            _pdf_view->setZoomMode(QPdfView::FitToWidth);
        else
            Z_HALT_INT;

    } else if (_picture_widget != nullptr) {
        if (type == FittingType::Height)
            _picture_widget->fitToHeight();
        else if (type == FittingType::Page)
            _picture_widget->fitToPage();
        else if (type == FittingType::Width)
            _picture_widget->fitToWidth();
        else
            Z_HALT_INT;

    } else {
        error = Error::unsupportedError();
    }

    if (error.isOk())
        emit sg_fittingTypeChaged(type);

    return error;
}

UniviewParameters UniViewWidget::pdfChromiumParameters()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 2))
    // https://bugreports.qt.io/browse/QTBUG-90712
    // UniviewParameter::ZoomStep не работает, вроде исправлено в 5.15.4
    return UniviewParameter::RotateStep | UniviewParameter::AutoZoom | UniviewParameter::ReadPageNo;
#else
    return UniviewParameter::RotateStep | UniviewParameter::ZoomStep | UniviewParameter::AutoZoom | UniviewParameter::ReadPageNo;
#endif
}

Error UniViewWidget::loadPdfData(const QByteArray& data)
{
    clearHelper();
    return loadPdfData_helper(data);
}

/*
Error UniViewWidget::loadFile(const QString& file_name, FileType type, UniViewWidgetOptions options)
{     
    if (!QFileInfo::exists(file_name))
        return Error::fileNotFoundError(file_name);

    clearHelper();

    if (type == FileType::Undefined) {
        QString ext = QFileInfo(file_name).suffix().toLower();
        if (ext.isEmpty())
            type = FileType::Plain;
        else
            type = Utils::fileTypeFromExtension(ext);

        if (type == FileType::Html || type == FileType::Pdf || Utils::isImage(type))
            options = options.setFlag(UniViewWidgetOption::Editable, false);
    }

#ifdef Q_OS_LINUX
    // под линукс преобразуем в PDF всегда
    if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf || type == FileType::Xlsx || type == FileType::Xls) {
        options.setFlag(UniViewWidgetOption::WordToPdf);
        options.setFlag(UniViewWidgetOption::ExcelToPdf);
    }
#endif

    QString file_name_origin = file_name;
    QString file_source = file_name;

    if (QResource(file_name_origin).isValid()) {
        // это ресурс Qt. Надо предварительно скопировать во временный файл
        QString temp_dir = Utils::generateTempDirName();
        QString temp_file = temp_dir + "/" + QFileInfo(file_name_origin).fileName();
        Error error = Utils::copyFile(file_name_origin, temp_file);
        if (error.isError())
            return error;

        _to_remove_dirs << temp_dir;
        _to_remove_files << file_name;
        file_name_origin = temp_file;
        file_source = temp_file;
        options = options & (~static_cast<int>(UniViewWidgetOption::MsOfficeClone));
        options = options & (~static_cast<int>(UniViewWidgetOption::Editable));
    }

    bool editable = (options & UniViewWidgetOption::Editable) > 0;
#ifdef Q_OS_WIN
    bool ms_office_clone
        = (options & UniViewWidgetOption::MsOfficeClone) > 0
          && (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf || type == FileType::Xlsx || type == FileType::Xls);
#endif
    bool word_pdf = (options & UniViewWidgetOption::WordToPdf) > 0;
    bool excel_pdf = (options & UniViewWidgetOption::ExcelToPdf) > 0;

    if (((type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf) && word_pdf)
        || ((type == FileType::Xlsx || type == FileType::Xls) && excel_pdf)) {
        Error error;
        QString temp_dir = Utils::generateTempDirName();
        QString temp_file = temp_dir + "/" + QFileInfo(file_name_origin).completeBaseName() + ".pdf";
        if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf)
            error = Utils::wordToPdf(file_name_origin, temp_file);
        else if (type == FileType::Xlsx || type == FileType::Xls)
            error = Utils::excelToPdf(file_name_origin, temp_file);
        else
            Z_HALT_INT;
        if (error.isError()) {
            startLoad();
            finishLoad(error);
            Utils::removeFile(temp_file);
            return error;
        }

        options.setFlag(UniViewWidgetOption::Editable, false);
        editable = false;
        type = FileType::Pdf;
        file_source = temp_file;
        _to_remove_dirs << temp_dir;
        _to_remove_files << file_name;

    }
#ifdef Q_OS_WIN
    else if (!editable && ms_office_clone) {
        // Если пытаться открыть один и тот же файл два раза, то MSOControl сходит с ума
        Error error;
        QString temp_dir = Utils::generateTempDirName();
        QString temp_file = temp_dir + "/" + QFileInfo(file_name_origin).fileName();
        error = Utils::copyFile(file_source, temp_file);
        if (error.isError()) {
            startLoad();
            finishLoad(error);
            Utils::removeFile(temp_file);
            return error;
        }
        file_source = temp_file;
        _to_remove_dirs << temp_dir;
        _to_remover_file << temp_file;
    }
#endif

    if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf || type == FileType::Xlsx
        || type == FileType::Xls) {
#if defined(Q_OS_WINDOWS)
        Error error;
        startLoad();

        // MSWidget приходится удалять полностью, т.к. иначе имеются загадочные глюки с захватом вордом фокуса ввода
        prepare(nullptr);
        _ms_widget = new MSWidget;
        _ms_widget->setObjectName(QStringLiteral("zf_ms_widget"));
        _layout->addWidget(_ms_widget);

        if (type == FileType::Docx || type == FileType::Doc || type == FileType::Rtf)
            error = _ms_widget->loadWord(file_source, editable);
        else
            error = _ms_widget->loadExcel(file_source, editable);

        if (error.isError()) {
            delete _ms_widget;
            _ms_widget = nullptr;
            finishLoad(error);
            return error;
        }

        finishLoad({});

        setParameters({}, {});
#else
        setParameters({}, {});
        return Error::unsupportedError();
#endif
    } else if (Utils::isImage(type)) {
        createPictureViewer();
        Error error = _picture_widget->loadFile(file_source);
        if (error.isError()) {
            delete _picture_widget;
            _picture_widget = nullptr;
            setParameters({}, {});
            finishLoad(error);
            return error;
        }

        setParameters(UniviewParameter::ZoomPercent | UniviewParameter::RotateStep | UniviewParameter::ZoomStep
                      | UniviewParameter::AutoZoom);
        finishLoad({});
        emit sg_zoomScaleChanged(_picture_widget->scaleFactor());

    } else if (type == FileType::Html) {
        startLoad();

        createWebViewWidget();

        Error error = _web_widget->loadFile(file_source);
        if (error.isError()) {
            delete _web_widget;
            _web_widget = nullptr;
            finishLoad(error);
            setParameters({}, {});
            return error;
        }

        UniviewParameters params;
        if (comp(QFileInfo(file_source).suffix(), "pdf"))
            params = pdfChromiumParameters();

        setParameters(params);

    } else if (type == FileType::Plain || type == FileType::Json || type == FileType::Xml) {
        startLoad();

        QFile file(file_source);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            _is_loading = false;
            auto error = Error::fileIOError(file_source);
            finishLoad(error);
            return error;
        }
        QTextStream st(&file);

        Error error = loadTextData(st.readAll(), type, editable);
        file.close();

        if (error.isError())
            return error;

    } else if (type == FileType::Pdf) {
        startLoad();
        createPdfView();
        QPdfDocument::DocumentError error = _pdf_doc->load(file_source);
        if (error != QPdfDocument::NoError) {
            auto error = Error::fileIOError(file_source);
            finishLoad(error);
            return error;
        }

        setParameters(UniviewParameter::ZoomStep | UniviewParameter::AutoZoom | UniviewParameter::ReadPageNo | UniviewParameter::SetPageNo);
        finishLoad({});

        // даем расчитать размеры
        QTimer::singleShot(0, this, [this]() {
            emit sg_zoomScaleChanged(_pdf_view->zoomFactor());
            emit sg_currentPageChanged(1, _pdf_doc->pageCount());
        });
    } else
        return Error::unsupportedError();

    _type = type;
    _options = options;
    
    return Error();
}
*/

Error UniViewWidget::loadTextFile(const QString& file_name, FileType type, bool editable)
{
    clearHelper();
    return loadTextFile_helper(file_name, type, editable);
}

Error UniViewWidget::loadMsOfficeFile(const QString& file_name, FileType type, UniViewWidgetOptions options)
{
    clearHelper();
    return loadMsOfficeFile_helper(file_name, type, options);
}

Error UniViewWidget::loadMsOfficeData(const QByteArray& data, FileType type
#ifdef Q_OS_WINDOWS
                                      ,
                                      bool to_pdf
#endif
)
{
    clearHelper();
    return loadMsOfficeData_helper(data, type
#ifdef Q_OS_WINDOWS
                                   ,
                                   to_pdf
#endif
    );
}

Error UniViewWidget::loadPdfFile(const QString &file_name)
{
    clearHelper();
    return loadPdfFile_helper(file_name);
}

Error UniViewWidget::loadIconFile(const QString& file_name, FileType type)
{
    clearHelper();
    return loadIconFile_helper(file_name, type);
}

Error UniViewWidget::loadData(const QByteArray& data, FileType type, UniViewWidgetOptions options)
{
    clearHelper();
    return loadData_helper(data, type, options);
}

Error UniViewWidget::loadTextData(const QString& string, FileType type, bool editable)
{
    clearHelper();
    return loadTextData_helper(string, type, editable);
}

Error UniViewWidget::loadHtmlUrl(const QUrl& url)
{
    if (_web_widget == nullptr)
        clearHelper();

    return loadHtmlUrl_helper(url);
}

Error UniViewWidget::loadFile(const QString& file_name, FileType type, UniViewWidgetOptions options)
{
    clearHelper();
    return loadFile_helper(file_name, type, options);
}

Error UniViewWidget::loadIconData(const QIcon& icon)
{
    clearHelper();
    return loadIconData_helper(icon);
}

Error UniViewWidget::loadIconData(const QByteArray& data, const QString& file_type)
{
    clearHelper();
    return loadIconData_helper(data, file_type);
}

Error UniViewWidget::loadIconData(const QByteArray& data, FileType file_type)
{
    clearHelper();
    return loadIconData_helper(data, file_type);
}

Error UniViewWidget::saveAs(const QString& file_name, bool override) const
{
    Z_CHECK(isValid());

    if (!override && QFileInfo::exists(file_name))
        return Error::fileIOError(file_name);

#ifdef Q_OS_WINDOWS
    if (_ms_widget != nullptr) {
        Utils::removeFile(file_name);
        return _ms_widget->saveAs(file_name);
    }
#endif

    if (_text_widget != nullptr) {
        Utils::removeFile(file_name);

        QFile file(file_name);
        if (!file.open(QFile::WriteOnly | QFile::Truncate))
            return Error::fileIOError(file_name);
        QTextStream st(&file);

        st << _text_widget->toPlainText();
        Error error;
        if (st.status() != QTextStream::Ok)
            error = Error::fileIOError(file_name);
        file.close();

        return error;
    }

    return Error::unsupportedError();
}

UniviewParameters UniViewWidget::supportedParameters() const
{
    return _supported_params;
}

UniviewParameters UniViewWidget::availableParameters() const
{
    return _available_params;
}

Error UniViewWidget::setFittingType(FittingType type)
{
    if (_fitting_type == type)
        return {};

    _fitting_type = type;
    return setFittingTypeHelper(type);
}

Error UniViewWidget::zoomIn()
{
    _fitting_type = FittingType::Undefined;

    if (!_supported_params.testFlag(UniviewParameter::ZoomStep))
        return Error::unsupportedError();

    if (scaleFactor() >= maximumScale())
        return {};

    if (_web_widget != nullptr)
        return _web_widget->zoomIn();

    if (_picture_widget != nullptr) {
        _picture_widget->scaleImage(1.0 + _zoom_step);
        return {};
    }

    if (_pdf_view != nullptr) {
        _pdf_view->scale(1.0 + _zoom_step);
        return {};
    }

    return Error::unsupportedError();
}

Error UniViewWidget::zoomOut()
{
    _fitting_type = FittingType::Undefined;

    if (!_supported_params.testFlag(UniviewParameter::ZoomStep))
        return Error::unsupportedError();

    if (scaleFactor() <= minimumScale())
        return {};

    if (_web_widget != nullptr)
        return _web_widget->zoomOut();

    if (_picture_widget != nullptr) {
        _picture_widget->scaleImage(1.0 - _zoom_step);
        return {};
    }

    if (_pdf_view != nullptr) {
        _pdf_view->scale(1.0 - _zoom_step);
        return {};
    }

    return Error::unsupportedError();
}

Error UniViewWidget::setScale(double scale)
{
    _fitting_type = FittingType::Undefined;

    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return Error::unsupportedError();

    if (scale <= minimumScale() || scale >= maximumScale())
        return {};

    if (_picture_widget != nullptr) {
        _picture_widget->setScale(scale);
        return {};
    }

    if (_pdf_view != nullptr) {        
        _pdf_view->setZoomFactor(scale);
        return {};
    }

    return Error::unsupportedError();
}

double UniViewWidget::scaleFactor() const
{
    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return -1;

    if (_picture_widget != nullptr)
        return _picture_widget->scaleFactor();

    if (_pdf_view != nullptr)
        return _pdf_view->zoomFactor();

    return -1;
}

Error UniViewWidget::setMinimumScale(double scale)
{
    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return Error::unsupportedError();

    if (_picture_widget != nullptr) {
        _picture_widget->setMinimumScale(scale);
        return {};
    }

    return Error::unsupportedError();
}

double UniViewWidget::minimumScale() const
{
    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return -1;

    if (_picture_widget != nullptr)
        return _picture_widget->minimumScale();

    if (_pdf_view != nullptr)
        return 0.2;

    return -1;
}

Error UniViewWidget::setMaximumScale(double scale)
{
    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return Error::unsupportedError();

    if (_picture_widget != nullptr) {
        _picture_widget->setMaximumScale(scale);
        return {};
    }

    return Error::unsupportedError();
}

double UniViewWidget::maximumScale() const
{
    if (!_supported_params.testFlag(UniviewParameter::ZoomPercent))
        return -1;

    if (_picture_widget != nullptr)
        return _picture_widget->maximumScale();

    if (_pdf_view != nullptr)
        return 2;

    return -1;
}

Error UniViewWidget::rotateRight()
{
    if (!_supported_params.testFlag(UniviewParameter::RotateStep))
        return Error::unsupportedError();

    if (_web_widget != nullptr)
        return _web_widget->rotateRight();

    if (_picture_widget != nullptr) {
        _picture_widget->rotateRight(90);
        return {};
    }

    return Error::unsupportedError();
}

Error UniViewWidget::rotateLeft()
{
    if (!_supported_params.testFlag(UniviewParameter::RotateStep))
        return Error::unsupportedError();

    if (_web_widget != nullptr)
        return _web_widget->rotateLeft();

    if (_picture_widget != nullptr) {
        _picture_widget->rotateLeft(90);
        return {};
    }

    return Error::unsupportedError();
}

Error UniViewWidget::setCurrentPage(int page)
{
    if (!_supported_params.testFlag(UniviewParameter::SetPageNo))
        return Error::unsupportedError();

    if (_web_widget != nullptr)
        return _web_widget->setCurrentPage(page);

    if (_pdf_view != nullptr) {
        _pdf_view->pageNavigation()->setCurrentPage(page - 1);
        return {};
    }

    return Error::unsupportedError();
}

int UniViewWidget::currentPage() const
{
    if (!_supported_params.testFlag(UniviewParameter::ReadPageNo))
        return -1;

    if (_pdf_view != nullptr) {
        return _pdf_view->pageNavigation()->currentPage() + 1;
        return {};
    }

    return -1;
}

int UniViewWidget::maximumPage() const
{
    if (!_supported_params.testFlag(UniviewParameter::ReadPageNo))
        return -1;

    if (_pdf_view != nullptr) {
        _pdf_view->pageNavigation()->pageCount();
        return {};
    }

    return -1;
}

bool UniViewWidget::isEditable() const
{
    return (_options & UniViewWidgetOption::Editable) > 0;
}

} // namespace zf
