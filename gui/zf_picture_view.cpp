#include "zf_picture_view.h"
#include <QScrollArea>
#include <QLabel>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QMouseEvent>
#include "zf_utils.h"
#include "zf_core.h"

namespace zf
{
PictureViewer::PictureViewer(QWidget* parent)
    : QWidget(parent)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    _scroll_area = new QScrollArea;
    _scroll_area->setAlignment(Qt::AlignCenter);
    _scroll_area->setFrameShape(QFrame::NoFrame);
    _scroll_area->setStyleSheet(
        QStringLiteral("QScrollArea {background-color: %1}").arg(qApp->palette().brush(QPalette::Dark).color().name()));

    _scroll_area->viewport()->installEventFilter(this);
    layout()->addWidget(_scroll_area);

    _image_label = new QLabel;
    _image_label->setScaledContents(true);
    _scroll_area->setWidget(_image_label);

    _fit_timer.setInterval(0);
    _fit_timer.setSingleShot(true);
    connect(&_fit_timer, &QTimer::timeout, this, [&]() {
        if (_requested_fitting == FittingType::Undefined)
            return;

        if (_requested_fitting == FittingType::Width)
            setScale(fitToWidthScale());
        else if (_requested_fitting == FittingType::Height)
            setScale(fitToHeightScale());
        else if (_requested_fitting == FittingType::Page)
            setScale(qMin(fitToWidthScale(), fitToHeightScale()));

        _requested_fitting = FittingType::Undefined;
    });

    _scale_timer.setInterval(0);
    _scale_timer.setSingleShot(true);
    connect(&_scale_timer, &QTimer::timeout, this, [&]() {
        scaleImage();
        //        _scroll_area->setUpdatesEnabled(true);
        //        _image_label->setUpdatesEnabled(true);
    });

    fitToPage();
}

void PictureViewer::setIcon(const QIcon& icon)
{
    _icon = icon;

    // берем пиксмап максимального размера
    QSize size = Utils::maxPixmapSize(icon);
    if (!size.isValid())
        size = this->size();

    _pixmap = icon.pixmap(size);

    _image_label->setPixmap(_pixmap);

    _degrees = 0;
    _scale_factor = 1;
    _requested_fitting = FittingType::Undefined;

    scaleImage();

    _file_data.clear();
    _file_type.clear();

    emit sg_imageLoaded();
}

Error PictureViewer::loadFile(const QString& file_name)
{
    QFile file(file_name);
    if (!file.open(QFile::ReadOnly)) {
        clear();
        return Error::fileIOError(file_name);
    }

    QByteArray file_data = file.readAll();
    QString file_type = QFileInfo(file_name).suffix().toUpper();
    file.close();
    Error error = setFileData(file_data, file_type);

    if (error.isError()) {
        clear();
        return error;
    }

    return Error();
}

Error PictureViewer::setFileData(const QByteArray& file_data, const QString& file_type)
{        
    QPixmap image;
    if (!image.loadFromData(file_data, file_type.isEmpty() ? nullptr : file_type.toUpper().toLatin1().constData())) {
        clear();
        return Error::corruptedDataError();
    }

    setIcon(QIcon(image));

    _file_data = file_data;
    _file_type = file_type;    

    return {};
}

Error PictureViewer::setFileData(const QByteArray& file_data, FileType file_type)
{
    Z_CHECK(file_type != FileType::Undefined);
    return setFileData(file_data, Utils::fileExtensions(file_type).constFirst());
}

QIcon PictureViewer::icon() const
{
    return _icon;
}

QByteArray PictureViewer::fileData() const
{
    return _file_data;
}

QString PictureViewer::fileType() const
{
    return _file_type;
}

void PictureViewer::clear()
{
    _icon = QIcon();
    _scale_factor = 1;
    _requested_fitting = FittingType::Undefined;
    _degrees = 0;
    _image_label->setPixmap(QPixmap());
    _file_data.clear();
    _file_type.clear();

    emit sg_imageLoaded();
}

QSize PictureViewer::sizeForScale(double value) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QSize size = _image_label->pixmap(Qt::ReturnByValue).size() * value;
#else
    QSize size = _image_label->pixmap()->size() * value;
#endif
    size.setWidth(size.width() - _fit_borders * 2);
    size.setHeight(size.height() - _fit_borders * 2);
    return size;
}

void PictureViewer::scaleImage(double factor)
{
    setScale(_scale_factor * factor);
}

void PictureViewer::setScale(double value)
{
    bool changed;

    if (value < _minimum_scale) {
        value = _minimum_scale;
        changed = true;

    } else if (value > _maximum_scale) {
        value = _maximum_scale;
        changed = true;

    } else {
        changed = !fuzzyCompare(_scale_factor, value);
    }

    if (changed)
        _scale_factor = value;

    _scale_timer.start();

    //    _scroll_area->setUpdatesEnabled(false);
    //    _image_label->setUpdatesEnabled(false);

    if (changed) {
        _fit_timer.stop();
        _requested_fitting = FittingType::Undefined;

        emit sg_scaleChanged(_scale_factor);
    }
}

double PictureViewer::scaleFactor() const
{
    return _scale_factor;
}

void PictureViewer::setMinimumScale(double scale)
{
    Z_CHECK(scale > 0);
    _minimum_scale = scale;
    if (_minimum_scale > _scale_factor)
        setScale(_minimum_scale);
}

double PictureViewer::minimumScale() const
{
    return _minimum_scale;
}

void PictureViewer::setMaximumScale(double scale)
{
    Z_CHECK(scale > 0);
    _maximum_scale = scale;
    if (_maximum_scale < _scale_factor)
        setScale(_maximum_scale);
}

double PictureViewer::maximumScale() const
{
    return _maximum_scale;
}

void PictureViewer::fitToWidth()
{
    _requested_fitting = FittingType::Width;
    _fit_timer.start();
}

void PictureViewer::fitToHeight()
{
    _requested_fitting = FittingType::Height;
    _fit_timer.start();
}

void PictureViewer::fitToPage()
{
    _requested_fitting = FittingType::Page;
    _fit_timer.start();
}

int PictureViewer::rotated() const
{
    return _degrees;
}

void PictureViewer::rotate(int degrees)
{
    Z_CHECK(qAbs(degrees) <= 360);
    if (degrees == 360)
        degrees = 0;

    if (_degrees == degrees)
        return;

    int old = _degrees;
    _degrees = degrees;
    QPixmap pm = _pixmap;
    QTransform trans;
    trans.rotate(_degrees);
    _image_label->setPixmap(pm.transformed(trans));
    setScale(_scale_factor);

    emit sg_roteated(old, _degrees);
}

void PictureViewer::rotateRight(int degrees)
{
    Z_CHECK(degrees >= 0 && degrees <= 360);
    if (degrees == 0)
        return;

    int d = _degrees + degrees;
    if (d > 360)
        d = degrees - 360;

    rotate(d);
}

void PictureViewer::rotateLeft(int degrees)
{
    Z_CHECK(degrees >= 0 && degrees <= 360);
    if (degrees == 0)
        return;

    int d = _degrees - degrees;
    if (d < 0)
        d = 360 + d;

    rotate(d);
}

void PictureViewer::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    if (event->button() == Qt::LeftButton && QApplication::keyboardModifiers() == Qt::NoModifier) {
        _drag_pos = event->pos();
        setCursor(Qt::PointingHandCursor);
    }
}

void PictureViewer::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);

    if ((event->buttons() & Qt::LeftButton) && !_drag_pos.isNull()
        && (event->pos() - _drag_pos).manhattanLength() >= QApplication::startDragDistance()) {
        int x_move = event->pos().x() - _drag_pos.x();
        int y_move = event->pos().y() - _drag_pos.y();
        _scroll_area->horizontalScrollBar()->setValue(_scroll_area->horizontalScrollBar()->value() - x_move);
        _scroll_area->verticalScrollBar()->setValue(_scroll_area->verticalScrollBar()->value() - y_move);
        _drag_pos = event->pos();
    }
}

void PictureViewer::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if (!_drag_pos.isNull()) {
        _drag_pos = QPoint();
        unsetCursor();
    }
}

void PictureViewer::wheelEvent(QWheelEvent* event)
{
    setScale(_scale_factor + (double)event->angleDelta().y() / 3000.0);
    QWidget::wheelEvent(event);
}

bool PictureViewer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _scroll_area->viewport() && event->type() == QEvent::Wheel) {
        event->ignore();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

double PictureViewer::fitToWidthScale()
{
    QRect content_rect = _scroll_area->contentsRect();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    int image_width = _image_label->pixmap(Qt::ReturnByValue).width();
#else
    int image_width = _image_label->pixmap()->width();
#endif

    double full_scale = (double)content_rect.width() / (double)image_width;
    QSize full_size = sizeForScale(full_scale);

    double scroll_scale = (double)(content_rect.width() - _scroll_area->verticalScrollBar()->width()) / (double)image_width;
    bool full_fit = (full_size.height() <= content_rect.height());

    if (full_fit)
        return full_scale;
    else
        return scroll_scale;
}

double PictureViewer::fitToHeightScale()
{
    QRect content_rect = _scroll_area->geometry();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    int image_height = _image_label->pixmap(Qt::ReturnByValue).height();
#else
    int image_height = _image_label->pixmap()->height();
#endif

    double full_scale = (double)content_rect.height() / (double)image_height;
    QSize full_size = sizeForScale(full_scale);

    double scroll_scale = (double)(content_rect.height() - _scroll_area->horizontalScrollBar()->height()) / (double)image_height;
    bool full_fit = (full_size.width() <= content_rect.width());

    if (full_fit)
        return full_scale;
    else
        return scroll_scale;
}

void PictureViewer::scaleImage()
{
    QSize new_size = sizeForScale(_scale_factor);

    if (_image_label->size().width() > 0 && new_size.width() > 0) {
        double zoom = (double)new_size.width() / (double)_image_label->size().width();

        auto center_before = geometry().center();
        center_before.setX(center_before.x() - _image_label->geometry().x());
        center_before.setY(center_before.y() - _image_label->geometry().y());

        QPointF center_after = {(double)center_before.x() * zoom, (double)center_before.y() * zoom};

        _image_label->resize(sizeForScale(_scale_factor));

        _scroll_area->horizontalScrollBar()->setValue(_scroll_area->horizontalScrollBar()->value()
                                                      + (center_after.x() - center_before.x()));
        _scroll_area->verticalScrollBar()->setValue(_scroll_area->verticalScrollBar()->value() + (center_after.y() - center_before.y()));

    } else {
        _image_label->resize(sizeForScale(_scale_factor));
    }
}

} // namespace zf
