#include "zf_image_list_p.h"
#include "zf_image_list.h"
#include "zf_utils.h"
#include "zf_label.h"
#include "zf_core.h"
#include "zf_colors_def.h"

#include <QDrag>
#include <QMimeData>
#include <QGraphicsDropShadowEffect>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QStylePainter>

static const QString MARKER_STYLESHEET = R"(
QToolButton { 
  border-width: 1px;
    border-color: rgba(255, 255, 255, 0);
    border-style: solid;
    background-color: rgba(255, 255, 255, 0);
}

QToolButton:hover {
    border-width: 1px;
    border-color: %1;
    border-style: solid;
    background-color: rgba(255, 255, 255, 0);
}

QToolButton:pressed {
    border-width: 1px;
    border-color: %1;
    border-style: solid;
    background-color: rgba(255, 255, 255, 0);
})";

namespace zf
{
ImageListCategoryFrame::ImageListCategoryFrame(ImageList* image_list, const ImageCateroryID& category_id)
    : _image_list(image_list)
    , _category_id(category_id)
    , _main_la(new QVBoxLayout)
    , _main_widget(new QFrame)
    , _caption_widget(new QFrame)
    , _images_la(new QHBoxLayout)
{
    setAcceptDrops(true);

    _update_caption_timer.setInterval(0);
    _update_caption_timer.setSingleShot(true);
    connect(&_update_caption_timer, &QTimer::timeout, this, [&]() { updateCaption(); });
    connect(_image_list, &ImageList::sg_categoryMarkerSizeChanged, this, &ImageListCategoryFrame::sl_markerSizeChanged);

    _main_la->setMargin(0);
    _main_la->setSpacing(0);

    _main_widget_la = new QVBoxLayout;
    _main_widget_la->setContentsMargins(FRAME_MARGINS);
    _main_widget_la->setSpacing(0);
    _main_widget->setLayout(_main_widget_la);
    _main_widget->setFrameShape(QFrame::NoFrame);
    _main_widget->setObjectName("ImageListCategoryFrame_main_widget");
    _main_widget->setStyleSheet(QStringLiteral("QFrame#ImageListCategoryFrame_main_widget {"
                                               "border: 1px %1;"
                                               "border-top-style: none; "
                                               "border-right-style: solid; "
                                               "border-bottom-style: none; "
                                               "border-left-style: none; "
                                               "background-color: %2;}")
                                    .arg(Colors::uiLineColor(true).name())
                                    .arg(Colors::uiButtonColor().name()));

    _main_la->addWidget(_main_widget);

    _caption_widget->setFrameShape(QFrame::NoFrame);
    _caption_widget->setObjectName("ImageListCategoryFrame_caption_widget");
    _caption_widget->setLayout(new QVBoxLayout);
    _caption_widget->layout()->setContentsMargins(CAPTION_MARGINS);
    _caption_widget->layout()->setSpacing(0);
    _caption_widget->setStyleSheet(QStringLiteral("QFrame#ImageListCategoryFrame_caption_widget {"
                                                  "border: 1px %1;"
                                                  "border-top-style: none; "
                                                  "border-right-style: none; "
                                                  "border-bottom-style: solid; "
                                                  "border-left-style: none; "
                                                  "background-color: %2;}")
                                       .arg(Colors::uiLineColor(true).name())
                                       .arg(Colors::uiButtonColor().name()));

    _rows_frame = new QFrame;
    _rows_frame->setStyleSheet(QStringLiteral("background-color: white"));
    QVBoxLayout* _rows_layout = new QVBoxLayout;
    _rows_layout->setMargin(0);
    _rows_layout->setSpacing(0);
    _rows_frame->setLayout(_rows_layout);

    _rows_layout->addLayout(_images_la);

    _images_la->setContentsMargins(IMAGES_MARGINS);
    _images_la->setSpacing(0);
    _images_la->addSpacerItem(new QSpacerItem(0, 0,
                                              image_list->orientation() == Qt::Horizontal ? QSizePolicy::Expanding : QSizePolicy::Minimum,
                                              image_list->orientation() == Qt::Vertical ? QSizePolicy::Expanding : QSizePolicy::Minimum));
    _images_la->addSpacerItem(new QSpacerItem(0, 0,
                                              image_list->orientation() == Qt::Horizontal ? QSizePolicy::Expanding : QSizePolicy::Minimum,
                                              image_list->orientation() == Qt::Vertical ? QSizePolicy::Expanding : QSizePolicy::Minimum));

    _caption = new Label();
    _caption->setOrientation(image_list->orientation());
    _caption->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _caption_widget->layout()->addWidget(_caption);

    _marker = new QToolButton(this);
    _marker->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    connect(_marker, &QToolButton::clicked, this, &ImageListCategoryFrame::sl_markerClicked);
    updateMarker();

    _main_widget->layout()->addWidget(_caption_widget);
    _main_widget->layout()->addWidget(_rows_frame);
    _main_widget_la->setStretch(1, 1);

    setLayout(_main_la);

    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    _update_caption_timer.start();

    updateInternalGeometry();
}

ImageCateroryID ImageListCategoryFrame::id() const
{
    return _category_id;
}

ImageListImageFrame* ImageListCategoryFrame::image(const ImageID& image_id) const
{
    int pos = _image_ids.indexOf(image_id);
    return pos >= 0 ? _image_frames.at(pos) : nullptr;
}

ImageID ImageListCategoryFrame::findImage(ImageListImageFrame* frame) const
{
    int pos = _image_frames.indexOf(frame);
    return pos >= 0 ? _image_ids.at(pos) : ImageID();
}

void ImageListCategoryFrame::setCurrentImage(const ImageID& image_id)
{
    if (_current_image == image_id)
        return;

    Z_CHECK(!image_id.isValid() || _image_ids.contains(image_id));

    auto p = _current_image;
    _current_image = image_id;

    emit sg_imageSelected(image_id);
}

ImageID ImageListCategoryFrame::currentImage() const
{
    return _current_image;
}

ImageListImageFrame* ImageListCategoryFrame::addImage(const ImageID& image_id)
{
    return insertImage(image_id, _image_ids.count());
}

ImageListImageFrame* ImageListCategoryFrame::insertImage(const ImageID& image_id, int pos)
{
    Z_CHECK(pos <= _image_ids.count());
    //    auto icon = _image_list->storage()->icon(image_id);

    auto image_frame = new ImageListImageFrame(_image_list, image_id);

    _images_la->insertWidget(pos + 1, image_frame);
    _image_ids.insert(pos, image_id);
    _image_frames.insert(pos, image_frame);

    image_frame->installEventFilter(this);

    _update_caption_timer.start();

    return image_frame;
}

void ImageListCategoryFrame::removeImage(const ImageID& image_id)
{
    auto image_frame = takeImage(image_id);
    image_frame->hide();
    image_frame->deleteLater();
}

ImageListImageFrame* ImageListCategoryFrame::takeImage(const ImageID& image_id)
{
    int pos = _image_ids.indexOf(image_id);
    Z_CHECK(pos >= 0);

    if (image_id == _current_image)
        setCurrentImage({});

    auto image = _image_frames.at(pos);
    auto item = _images_la->takeAt(pos + 1);
    Z_CHECK(item->widget() == image);
    delete item;
    _image_ids.removeAt(pos);
    _image_frames.removeAt(pos);

    _update_caption_timer.start();
    return image;
}

bool ImageListCategoryFrame::moveImage(const ImageID& image_id, int pos)
{
    int c_pos = _image_ids.indexOf(image_id);
    Z_CHECK(c_pos >= 0);
    if (c_pos == pos)
        return false;

    _image_ids.move(c_pos, pos);
    _image_frames.move(c_pos, pos);

    auto item = _images_la->takeAt(c_pos + 1);
    _images_la->insertItem(pos + 1, item);

    _update_caption_timer.start();
    return true;
}

void ImageListCategoryFrame::setMinimumCaptionHeight(int h)
{
    if (_minimum_caption_height == h)
        return;
    _minimum_caption_height = h;
    updateInternalGeometry();
}

int ImageListCategoryFrame::captionHeight()
{
    return Utils::stringSize(_image_list->storage()->name(_category_id), false, -1, _caption->font()).height();
}

bool ImageListCategoryFrame::isMarkerClickable() const
{
    return _marker_clickable;
}

void ImageListCategoryFrame::setMarkerClickable(bool b)
{
    if (_marker_clickable == b)
        return;

    _marker_clickable = b;
    updateMarker();
}

void ImageListCategoryFrame::setCategoryMovable(bool enabled)
{
    _movable = enabled;
}

bool ImageListCategoryFrame::isCategoryMovable() const
{
    return _movable;
}

Qt::Alignment ImageListCategoryFrame::captionAlignment() const
{
    return _caption->alignment();
}

void ImageListCategoryFrame::setCaptionAlignment(Qt::Alignment alignment)
{
    if (_caption->alignment() == alignment)
        return;

    _caption->setAlignment(alignment);
}

QFont ImageListCategoryFrame::captionFont() const
{
    return _caption->font();
}

void ImageListCategoryFrame::setCaptionFont(const QFont& font)
{
    _caption->setFont(font);
}

QSize ImageListCategoryFrame::sizeHint() const
{
    if (_image_list->storage() == nullptr || !_image_list->storage()->contains(_category_id))
        return QSize();

    int line_size = 0;
    for (const auto& i : _image_frames) {
        if (i->isHidden())
            continue;

        line_size += _images_la->spacing();
        line_size += _image_list->orientation() == Qt::Horizontal ? i->sizeHint().width() : i->sizeHint().height();
    }

    int line_height = _image_list->realSize();

    int caption_size = 0;
    if (!_caption->text().isEmpty()) {
        auto size = Utils::stringSize(_caption->text(), false, -1, _caption->font());
        size.setWidth(size.width() + _caption_widget->contentsMargins().left() + +_caption_widget->contentsMargins().right()
                      + Utils::stringWidth("OO"));
        size.setHeight(size.height() + _caption_widget->contentsMargins().top() + +_caption_widget->contentsMargins().bottom());

        if (_image_list->orientation() == Qt::Horizontal) {
            caption_size = size.width();
        } else {
            caption_size = size.height();
        }
    }

    if (_marker->isVisible() && !_marker->icon().isNull()) {
        caption_size += _marker->width();
    }

    line_size = qMax(caption_size, line_size);

    if (_image_list->orientation() == Qt::Horizontal) {
        line_size += _main_widget_la->contentsMargins().left() + _main_widget_la->contentsMargins().right()
                     + _images_la->contentsMargins().left() + _images_la->contentsMargins().right();

        return {qMax(categoryMinSize(), line_size), line_height};
    } else {
        line_size += _main_widget_la->contentsMargins().top() + _main_widget_la->contentsMargins().bottom()
                     + _images_la->contentsMargins().top() + _images_la->contentsMargins().bottom();
        return {line_height, qMax(categoryMinSize(), line_size)};
    }
}

QSize ImageListCategoryFrame::minimumSizeHint() const
{
    return sizeHint();
}

QMargins ImageListCategoryFrame::imagesMargins()
{
    return IMAGES_MARGINS + FRAME_MARGINS + CAPTION_MARGINS;
}

void ImageListCategoryFrame::resizeEvent(QResizeEvent* event)
{
    _update_caption_timer.start();
    updateInternalGeometry();
    QFrame::resizeEvent(event);
}

bool ImageListCategoryFrame::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateInternalGeometry();
            break;
        default:
            break;
    }

    return QFrame::event(event);
}

bool ImageListCategoryFrame::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {        
        auto frame = qobject_cast<ImageListImageFrame*>(watched);
        if (frame != nullptr) {
            int pos = _image_frames.indexOf(frame);
            Z_CHECK(pos >= 0);
            setCurrentImage(_image_ids.at(pos));
        }
    }

    return QFrame::eventFilter(watched, event);
}

void ImageListCategoryFrame::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(ImageListImageFrame::MimeType)) {
        if (!_movable) {
            event->ignore();
            return;
        }

        ImageID image_id;
        ImageListImageFrame::decodeMimeData(event->mimeData(), image_id);
        if (!image_id.isValid()) {
            event->ignore();
            return;
        }

        if (getDropPosition(event->pos(), image_id) < 0) {
            event->ignore();
            return;
        }

        event->acceptProposedAction();

    } else {
        QFrame::dragEnterEvent(event);
    }
}

void ImageListCategoryFrame::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(ImageListImageFrame::MimeType)) {
        if (!_movable) {
            event->ignore();
            return;
        }

        ImageID image_id;
        ImageListImageFrame::decodeMimeData(event->mimeData(), image_id);
        if (!image_id.isValid()) {
            event->ignore();
            return;
        }

        auto image_category = _image_list->storage()->category(image_id);
        if (image_category.isValid() && !_image_list->categoryFrame(image_category)->_movable) {
            event->ignore();
            return;
        }

        int drop_pos = getDropPosition(event->pos(), image_id);
        if (drop_pos < 0 || !_image_list->storage()->canMove(image_id, _category_id, drop_pos)) {
            event->ignore();
            return;
        }

        event->acceptProposedAction();

    } else {
        QFrame::dragMoveEvent(event);
    }
}

void ImageListCategoryFrame::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat(ImageListImageFrame::MimeType)) {
        if (!_movable) {
            event->ignore();
            return;
        }

        ImageID image_id;
        ImageListImageFrame::decodeMimeData(event->mimeData(), image_id);
        if (!image_id.isValid()) {
            event->ignore();
            return;            
        }

        auto image_category = _image_list->storage()->category(image_id);
        if (image_category.isValid() && !_image_list->categoryFrame(image_category)->_movable) {
            event->ignore();
            return;
        }

        int drop_pos = getDropPosition(event->pos(), image_id);
        if (drop_pos < 0 || !_image_list->storage()->canMove(image_id, _category_id, drop_pos)) {
            event->ignore();
            return;
        }

        event->acceptProposedAction();
        _image_list->dragMoveRequest(image_id, _category_id, drop_pos);

    } else {
        QFrame::dropEvent(event);
    }
}

void ImageListCategoryFrame::mousePressEvent(QMouseEvent* event)
{
    // выделение при не точном попадании мыши в картинки
    if (!_image_ids.isEmpty() && _rows_frame->geometry().contains(event->pos())) {
        bool found = false;
        for (auto f : qAsConst(_image_frames)) {
            if (f->underMouse()) {
                found = true;
                break;
            }
        }

        if (!found) {
            if (event->pos().x() < width() / 2)
                setCurrentImage(_image_ids.constFirst());
            else
                setCurrentImage(_image_ids.constLast());
            event->ignore();
        }
    }

    QFrame::mousePressEvent(event);
}

void ImageListCategoryFrame::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
}

void ImageListCategoryFrame::on_imageRemoved(const ImageID& image_id)
{    
    removeImage(image_id);
}

void ImageListCategoryFrame::on_imageAdded(const ImageID& image_id)
{
    if (_image_list->storage()->category(image_id) != _category_id)
        return;

    addImage(image_id);
}

void ImageListCategoryFrame::on_imageMoved(const ImageID& image_id, const ImageCateroryID& previous_category_id, int previous_position,
                                           const ImageCateroryID& category_id, int position)
{
    Q_UNUSED(previous_position);

    bool current = (_image_list->currentImage() == image_id);

    if (previous_category_id == _category_id) {
        if (_category_id != category_id) {
            removeImage(image_id);

        } else {
            moveImage(image_id, position);
            return;
        }
    }
    if (category_id == _category_id) {
        insertImage(image_id, position);
        if (current)
            setCurrentImage(image_id);
    }
}

void ImageListCategoryFrame::updateInternalGeometry()
{
    if (_minimum_caption_height > 0)
        _caption->setMinimumHeight(qMax(_minimum_caption_height, _image_list->categoryMarkerSize()));

    if (_marker->icon().isNull() || (_image_list->isReadOnly() && _marker_clickable)) {
        _marker->setHidden(true);
        _caption->setContentsMargins(0, 0, 0, 0);

    } else {
        _marker->setVisible(true);

        int marker_size;
        if (_minimum_caption_height <= 0)
            marker_size = _image_list->categoryMarkerSize();
        else
            marker_size = qMin(_caption->minimumHeight(), _image_list->categoryMarkerSize());

        _marker->setGeometry(3, 3, marker_size, marker_size);
        _marker->raise();

        _caption->setContentsMargins(_marker->geometry().right(), 0, 0, 0);
    }
}

void ImageListCategoryFrame::on_categoryMarkerChanged(const QIcon& marker)
{
    Q_UNUSED(marker)    
    updateMarker();
}

void ImageListCategoryFrame::sl_markerSizeChanged(int size)
{
    Q_UNUSED(size)
    updateMarker();
    updateCaption();
}

void ImageListCategoryFrame::on_categoryNameChanged(const QString& name)
{
    Q_UNUSED(name)
    updateCaption();
}

void ImageListCategoryFrame::sl_markerClicked()
{
    if (_marker_clickable && !_image_list->isReadOnly())
        emit sg_markerClicked(_category_id);
}

void ImageListCategoryFrame::updateCaption()
{
    if (!_image_list->storage()->contains(_category_id))
        return;

    _caption->setText(_image_list->storage()->name(_category_id));

    if (_caption->text().isEmpty()) {
        _main_widget_la->setContentsMargins(FRAME_MARGINS_NO_CAPTION);
        _caption_widget->setHidden(true);
    } else {
        _main_widget_la->setContentsMargins(FRAME_MARGINS);
        _caption_widget->setHidden(false);
    }
}

void ImageListCategoryFrame::updateMarker()
{
    int target_size = _image_list->categoryMarkerSize();
    QIcon icon = _image_list->storage()->marker(_category_id);

    if (icon.isNull()) {
        _marker->setIcon({});

    } else {
        QSize size = Utils::bestPixmapSize(icon, {target_size, target_size});
        QPixmap pixmap = Utils::scalePicture(icon.pixmap(size), target_size, target_size, true);
        _marker->setIcon(pixmap);
        _marker->setIconSize(pixmap.size());
    }

    if (_marker_clickable && !_image_list->isReadOnly())
        _marker->setStyleSheet(MARKER_STYLESHEET.arg(Colors::uiLineColor(false).name()));
    else
        _marker->setStyleSheet("border:none; background-color: rgba(255, 255, 255, 0)");

    updateInternalGeometry();
}

void ImageListCategoryFrame::updateControlsStatus()
{
    updateMarker();

    for (auto i : qAsConst(_image_frames)) {
        i->updateControlsStatus();
    }
}

int ImageListCategoryFrame::categoryMinSize() const
{
    return _image_list->storage()->minimumSize(_category_id) * qApp->fontMetrics().horizontalAdvance(QChar('O'));
}

int ImageListCategoryFrame::getDropPosition(const QPoint& pos, const ImageID& image_id) const
{
    if (_image_ids.isEmpty())
        return 0;

    int drop_index = -1;
    bool left = false;
    bool replace = false;

    for (int i = 0; i < _image_frames.count(); i++) {
        auto geo = _image_frames.at(i)->geometry();
        auto image_size = _image_frames.at(i)->imageSize();

        if (_image_list->orientation() == Qt::Horizontal) {
            if (geo.left() <= pos.x() && geo.right() >= pos.x()) {
                if (pos.x() < geo.center().x())
                    left = true;

                replace = (pos.x() >= geo.center().x() - image_size.width() / 2 && pos.x() <= geo.center().x() + image_size.width() / 2);
                drop_index = i;
                break;
            }
        } else {
            if (geo.top() <= pos.y() && geo.bottom() >= pos.y()) {
                if (pos.x() < geo.center().x())
                    left = true;

                replace = (pos.y() >= geo.center().y() - image_size.height() / 2 && pos.y() <= geo.center().y() + image_size.height() / 2);
                drop_index = i;
                break;
            }
        }
    }

    int drag_index = _image_ids.indexOf(image_id);
    if (drop_index >= 0) { //-V1051
        if (replace)
            return drop_index;

        if (drag_index < 0) {
            if (left)
                return drop_index;
            else
                return drop_index + 1;

        } else {
            if (drag_index > drop_index) {
                if (left)
                    return drop_index;
                else
                    return drop_index + 1;
            } else {
                if (left)
                    return drop_index - 1;
                else
                    return drop_index;
            }
        }
    }

    if (_image_list->orientation() == Qt::Horizontal) {
        if (pos.x() < width() / 2)
            return 0;
        else
            return drag_index < 0 ? _image_frames.count() : _image_frames.count() - 1;

    } else {
        if (pos.y() < height() / 2)
            return 0;
        else
            return drag_index < 0 ? _image_frames.count() : _image_frames.count() - 1;
    }
}

ImageListImageFrame::ImageListImageFrame(ImageList* image_list, const ImageID& image_id)
    : _image_list(image_list)
    , _image_id(image_id)
    , _image_label(new QLabel)
    , _caption(new Label(this))
    , _marker(new QToolButton(this))
{
    _image_label->setAlignment(Qt::AlignCenter);
    _image_label->setMargin(0);
    _image_label->setAutoFillBackground(true);

    _caption->setAlignment(Qt::AlignCenter);
    _caption->setMargin(0);
    _caption->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _caption->setStyleSheet("background-color: white");

    _marker->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    connect(_marker, &QToolButton::clicked, this, &ImageListImageFrame::sl_markerClicked);

    updateMarker();

    updateImage();

    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(MARGINS);

    layout()->addWidget(_image_label);

    connect(_image_list, &ImageList::sg_imageMarkerSizeChanged, this, &ImageListImageFrame::sl_markerSizeChanged);
    connect(_image_list, &ImageList::sg_currentChanged, this, &ImageListImageFrame::sl_currentChanged);

    updateCurrent();
}

ImageID ImageListImageFrame::id() const
{
    return _image_id;
}

QSize ImageListImageFrame::sizeHint() const
{
    auto size = imageSize();
    size.setWidth(size.width() + MARGINS.left() + MARGINS.right());
    size.setHeight(size.height() + MARGINS.top() + MARGINS.bottom());
    return size;
}

QSize ImageListImageFrame::minimumSizeHint() const
{
    return sizeHint();
}

QSize ImageListImageFrame::imageSize() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return _image_label->pixmap(Qt::ReturnByValue).size();
#else
    return _image_label->pixmap() == nullptr ? QSize() : _image_label->pixmap()->size();
#endif
}

void ImageListImageFrame::mousePressEvent(QMouseEvent* event)
{
    QFrame::mousePressEvent(event);

    if (event->button() == Qt::LeftButton && QApplication::keyboardModifiers() == Qt::NoModifier && _image_list->isDragEnabled()
        && !_image_list->isReadOnly() && !_image_list->storage()->isCategoryReadOnly(_image_list->storage()->category(_image_id))) {
        _drag_start_pos = event->pos();
    }
}

void ImageListImageFrame::mouseMoveEvent(QMouseEvent* event)
{
    QFrame::mouseMoveEvent(event);

    if ((event->buttons() & Qt::LeftButton) && !_drag_start_pos.isNull()
        && (event->pos() - _drag_start_pos).manhattanLength() >= QApplication::startDragDistance()) {
        _drag_start_pos = QPoint();

        // формируем картинку для отображения при переносе
        QDrag* drag = new QDrag(this);
        drag->setMimeData(encodeMimeData());
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        drag->setPixmap(_image_label->pixmap(Qt::ReturnByValue));
#else
        drag->setPixmap(*_image_label->pixmap());
#endif
        drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));

        drag->exec(Qt::MoveAction);
    }
}

void ImageListImageFrame::mouseReleaseEvent(QMouseEvent* event)
{
    QFrame::mouseReleaseEvent(event);
}

void ImageListImageFrame::resizeEvent(QResizeEvent* event)
{
    updateInternalGeometry();    
    QFrame::resizeEvent(event);
}

bool ImageListImageFrame::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateInternalGeometry();
            break;
        default:
            break;
    }

    return QFrame::event(event);
}

QMimeData* ImageListImageFrame::encodeMimeData() const
{
    QMimeData* data = new QMimeData;
    QByteArray content;
    QDataStream dataStream(&content, QIODevice::WriteOnly);
    dataStream.setVersion(Consts::DATASTREAM_VERSION);
    dataStream << _image_id.value();
    data->setData(MimeType, content);

    return data;
}

bool ImageListImageFrame::decodeMimeData(const QMimeData* data, ImageID& image_id)
{
    image_id.clear();
    if (!data->hasFormat(MimeType))
        return false;

    QByteArray itemData = data->data(MimeType);
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);
    dataStream.setVersion(Consts::DATASTREAM_VERSION);
    int id;
    dataStream >> id;
    if (dataStream.status() != QDataStream::Ok)
        return false;

    if (id > 0)
        image_id = ImageID(id);

    return true;
}

void ImageListImageFrame::on_imageChanged(const QIcon& icon)
{
    Q_UNUSED(icon);    
    updateImage();
}

void ImageListImageFrame::on_imageMarkerChanged(const QIcon& marker)
{
    Q_UNUSED(marker);    
    updateMarker();
}

void ImageListImageFrame::sl_markerSizeChanged(int size)
{
    Q_UNUSED(size)
    updateMarker();
}

void ImageListImageFrame::sl_currentChanged(const ImageID& previous_image_id, const ImageID& image_id)
{
    Q_UNUSED(previous_image_id)
    Q_UNUSED(image_id)

    updateCurrent();
}

void ImageListImageFrame::on_optionsChanged(const ImageListOptions& options)
{
    Q_UNUSED(options)    
    updateStyle();
}

void ImageListImageFrame::on_imageNameChanged(const QString& name)
{
    Q_UNUSED(name)    
    updateInternalGeometry();
}

void ImageListImageFrame::sl_markerClicked()
{
    if (_marker_clickable && !_image_list->isReadOnly())
        emit sg_markerClicked(_image_id);
}

void ImageListImageFrame::updateInternalGeometry()
{
    int v_shift = qMax(0, _image_label->pixmap() == nullptr || _image_label->pixmap()->isNull()
                              ? 0
                              : (height() - _image_label->pixmap()->size().height()) / 2);
    int h_shift = qMax(0, _image_label->pixmap() == nullptr || _image_label->pixmap()->isNull()
                              ? 0
                              : (width() - _image_label->pixmap()->size().width()) / 2);

    if (_marker->icon().isNull() || (_image_list->isReadOnly() && _marker_clickable)) {
        _marker->setHidden(true);
    } else {
        _marker->setVisible(true);

        _marker->setGeometry(width() - _image_list->imageMarkerSize() - h_shift / 2, v_shift / 2, _image_list->imageMarkerSize(),
                             _image_list->imageMarkerSize());
        _marker->raise();
    }

    QString name = _image_list->storage()->name(_image_id);
    if (name.isEmpty()) {
        _caption->setHidden(true);

    } else {
        _caption->setVisible(true);

        _caption->setText(name.simplified());
        QSize text_size = qApp->fontMetrics().boundingRect(_caption->text()).size();
        text_size.setWidth(text_size.width() + qApp->fontMetrics().horizontalAdvance('X'));
        const int margin = 3;
        _caption->setGeometry((width() - text_size.width()) / 2, height() - text_size.height() - v_shift - margin,
            text_size.width(), text_size.height());

        _caption->raise();
    }
}

void ImageListImageFrame::updateImage()
{
    QSize target_size = _image_list->imageSize();
    target_size.setWidth(target_size.width() - MARGINS.left() - MARGINS.right());
    target_size.setHeight(target_size.height() - MARGINS.top() - MARGINS.bottom());

    QIcon icon = _image_list->storage()->icon(_image_id);
    if (!icon.isNull()) {
        int scroll_size = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

        if (_image_list->orientation() == Qt::Orientation::Horizontal)
            target_size.setHeight(target_size.height() - scroll_size);
        else
            target_size.setWidth(target_size.width() - scroll_size);

        QSize size = Utils::bestPixmapSize(icon, target_size);
        QPixmap pixmap = Utils::scalePicture(icon.pixmap(size), target_size, size.width() > size.height());

        _image_label->setPixmap(pixmap);

    } else {
        _image_label->setPixmap({});
    }

    updateInternalGeometry();
}

void ImageListImageFrame::updateMarker()
{
    int target_size = _image_list->imageMarkerSize();
    QIcon icon = _image_list->storage()->marker(_image_id);

    if (icon.isNull()) {
        _marker->setIcon({});

    } else {
        QSize size = Utils::bestPixmapSize(icon, {target_size, target_size});
        QPixmap pixmap = Utils::scalePicture(icon.pixmap(size), target_size, target_size, true);
        _marker->setIcon(pixmap);
    }

    if (_marker_clickable && !_image_list->isReadOnly())
        _marker->setStyleSheet(MARKER_STYLESHEET.arg(Colors::uiLineColor(false).name()));
    else
        _marker->setStyleSheet("border:none; background-color: rgba(255, 255, 255, 0)");

    updateInternalGeometry();
}

void ImageListImageFrame::updateCurrent()
{
    bool current = _image_list->currentImage() == _image_id;
    if (_is_current == current)
        return;

    _is_current = current;
    updateStyle();
}

void ImageListImageFrame::updateStyle()
{
    if (_is_current)
        setStyleSheet(QStringLiteral("background-color: LightBlue"));
    else
        setStyleSheet(QStringLiteral("background-color: white"));
}

void ImageListImageFrame::updateControlsStatus()
{
    updateMarker();
}

bool ImageListImageFrame::isMarkerClickable() const
{
    return _marker_clickable;
}

void ImageListImageFrame::setMarkerClickable(bool b)
{
    if (_marker_clickable == b)
        return;

    _marker_clickable = b;
    updateMarker();
}

} // namespace zf
