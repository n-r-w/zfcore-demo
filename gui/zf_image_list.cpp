#include "zf_image_list.h"
#include "zf_core.h"
#include "private/zf_image_list_p.h"
#include "zf_line_scrollarea.h"
#include "zf_graphics_effects.h"
#include <QScrollBar>

namespace zf
{
ImageList::ImageList(ImageListStorage* storage, const QSize& image_size, int text_rows, Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent)
    , _orientation(orientation)
    , _text_rows(text_rows)
{
    _category_marker_size = Utils::scaleUI(24);
    _image_marker_size = Utils::scaleUI(16);

    _selection_timer.setInterval(0);
    _selection_timer.setSingleShot(true);
    connect(&_selection_timer, &QTimer::timeout, this, &ImageList::sl_selectionChangeTimeout);

    _align_category_captions_timer.setInterval(0);
    _align_category_captions_timer.setSingleShot(true);
    connect(&_align_category_captions_timer, &QTimer::timeout, this, [this]() { alignCategoryCaptionsHelper(); });

    _drag_move_timer.setInterval(0);
    _drag_move_timer.setSingleShot(true);
    connect(&_drag_move_timer, &QTimer::timeout, this,
            [&]() { _storage->move(_drag_move_image_id, _drag_move_target_category_id, _drag_move_position); });

    if (orientation == Qt::Vertical)
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    else
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setLayout(new QVBoxLayout);
    layout()->setMargin(0);

    _scroll_area = new LineScrollArea(nullptr, orientation);
    _scroll_area->setFrameShape(QFrame::StyledPanel);
    setImageSize(image_size);

    layout()->addWidget(_scroll_area);

    setStorage(storage);
}

ImageList::ImageList(const QSize& image_size, int text_rows, Qt::Orientation orientation, QWidget* parent)
    : ImageList(nullptr, image_size, text_rows, orientation, parent)
{
}

ImageList::ImageList(QWidget* parent)
    : ImageList(nullptr, {Utils::scaleUI(100), Utils::scaleUI(100)}, 1, Qt::Horizontal, parent)
{
}

Qt::Orientation ImageList::orientation() const
{
    return _orientation;
}

bool ImageList::isReadOnly() const
{
    return _read_only;
}

void ImageList::setReadOnly(bool b)
{
    if (_read_only == b)
        return;

    _read_only = b;
    updateControlsStatus();
}

bool ImageList::isDragEnabled() const
{
    return _drag_enabled && !_read_only;
}

void ImageList::setDragEnabled(bool b)
{
    if (_drag_enabled == b)
        return;

    _drag_enabled = b;
    updateControlsStatus();
}

int ImageList::textRows() const
{
    return _text_rows;
}

void ImageList::setTextRows(int n)
{
    if (_text_rows == n)
        return;

    _text_rows = n;
    updateGeometry();
}

ImageListStorage* ImageList::storage() const
{
    return _storage;
}

void ImageList::setStorage(ImageListStorage* storage)
{
    if (storage == _storage)
        return;

    _drag_move_timer.stop();

    if (_storage != nullptr) {
        disconnect(_storage, &ImageListStorage::sg_categoryRemoved, this, &ImageList::sl_categoryRemoved);
        disconnect(_storage, &ImageListStorage::sg_categoryAdded, this, &ImageList::sl_categoryAdded);
        disconnect(_storage, &ImageListStorage::sg_categoryMoved, this, &ImageList::sl_categoryMoved);
        disconnect(_storage, &ImageListStorage::sg_imageMoved, this, &ImageList::sl_imageMoved);
        disconnect(_storage, &ImageListStorage::sg_categoryNameChanged, this, &ImageList::sl_categoryNameChanged);
        disconnect(_storage, &ImageListStorage::sg_categoryMarkerChanged, this, &ImageList::sl_categoryMarkerChanged);
        disconnect(_storage, &ImageListStorage::sg_imageAdded, this, &ImageList::sl_imageAdded);
        disconnect(_storage, &ImageListStorage::sg_imageRemoved, this, &ImageList::sl_imageRemoved);
        disconnect(_storage, &ImageListStorage::sg_imageChanged, this, &ImageList::sl_imageChanged);
        disconnect(_storage, &ImageListStorage::sg_imageMarkerChanged, this, &ImageList::sl_imageMarkerChanged);
        disconnect(_storage, &ImageListStorage::sg_optionsChanged, this, &ImageList::sl_optionsChanged);
        disconnect(_storage, &ImageListStorage::sg_imageNameChanged, this, &ImageList::sl_imageNameChanged);
    }

    if (storage != nullptr) {
        connect(storage, &ImageListStorage::sg_categoryRemoved, this, &ImageList::sl_categoryRemoved);
        connect(storage, &ImageListStorage::sg_categoryAdded, this, &ImageList::sl_categoryAdded);
        connect(storage, &ImageListStorage::sg_categoryMoved, this, &ImageList::sl_categoryMoved);
        connect(storage, &ImageListStorage::sg_imageMoved, this, &ImageList::sl_imageMoved);
        connect(storage, &ImageListStorage::sg_categoryNameChanged, this, &ImageList::sl_categoryNameChanged);
        connect(storage, &ImageListStorage::sg_categoryMarkerChanged, this, &ImageList::sl_categoryMarkerChanged);
        connect(storage, &ImageListStorage::sg_imageAdded, this, &ImageList::sl_imageAdded);
        connect(storage, &ImageListStorage::sg_imageRemoved, this, &ImageList::sl_imageRemoved);
        connect(storage, &ImageListStorage::sg_imageChanged, this, &ImageList::sl_imageChanged);
        connect(storage, &ImageListStorage::sg_imageMarkerChanged, this, &ImageList::sl_imageMarkerChanged);
        connect(storage, &ImageListStorage::sg_optionsChanged, this, &ImageList::sl_optionsChanged);
        connect(storage, &ImageListStorage::sg_imageNameChanged, this, &ImageList::sl_imageNameChanged);
    }

    _storage = storage;
    reload();
    alignCategoryCaptions();
}

QSize ImageList::imageSize() const
{
    return _image_size;
}

void ImageList::setImageSize(const QSize& size)
{
    Z_CHECK(size.isValid());
    if (_image_size == size)
        return;

    _image_size = size;

    updateLineSize();
}

int ImageList::categoryMarkerSize() const
{
    return _category_marker_size;
}

void ImageList::setCategoryMarkerSize(int size)
{
    if (_category_marker_size == size)
        return;

    _category_marker_size = size;
    updateGeometry();
    emit sg_categoryMarkerSizeChanged(size);
}

Qt::Alignment ImageList::categoryCaptionAlignment(const ImageCateroryID& category_id) const
{
    return categoryFrame(category_id)->captionAlignment();
}

void ImageList::setCategoryCaptionAlignment(const ImageCateroryID& category_id, Qt::Alignment alignment)
{
    categoryFrame(category_id)->setCaptionAlignment(alignment);
}

QFont ImageList::categoryСaptionFont(const ImageCateroryID& category_id) const
{
    return categoryFrame(category_id)->captionFont();
}

void ImageList::setСategoryCaptionFont(const ImageCateroryID& category_id, const QFont& font)
{
    categoryFrame(category_id)->setCaptionFont(font);
    alignCategoryCaptions();
}

int ImageList::imageMarkerSize() const
{
    return _image_marker_size;
}

void ImageList::setImageMarkerSize(int size)
{
    if (_image_marker_size == size)
        return;

    _image_marker_size = size;
    updateGeometry();
    emit sg_imageMarkerSizeChanged(size);
}

bool ImageList::isVisible(const ImageCateroryID &category_id) const
{
    return !categoryFrame(category_id)->isHidden();
}

bool ImageList::isVisible(const ImageID &image_id) const
{
    return !imageFrame(image_id).second->isHidden();
}

void ImageList::setVisible(const ImageCateroryID &category_id, bool b)
{
    auto cf = categoryFrame(category_id);
    cf->setHidden(!b);

    if (!b) {
        auto ci = currentImage();
        if (ci.isValid()) {
            auto i_f = imageFrame(ci);
            if (i_f.first->id() == category_id)
                setCurrentImage(ImageID());
        }
    }
    alignCategoryCaptions();
}

void ImageList::setVisible(const ImageID &image_id, bool b)
{
    auto i_f = imageFrame(image_id);
    i_f.second->setHidden(!b);

    if (!b && currentImage() == image_id)
        setCurrentImage(ImageID());

    alignCategoryCaptions();
}

bool ImageList::isMarkerClickable(const ImageCateroryID& category_id) const
{
    return categoryFrame(category_id)->isMarkerClickable();
}

void ImageList::setMarkerClickable(const ImageCateroryID& category_id, bool b)
{
    categoryFrame(category_id)->setMarkerClickable(b);
}

bool ImageList::isMarkerClickable(const ImageID& image_id) const
{
    return imageFrame(image_id).second->isMarkerClickable();
}

void ImageList::setMarkerClickable(const ImageID& image_id, bool b)
{
    imageFrame(image_id).second->setMarkerClickable(b);
}

int ImageList::realSize() const
{
    int text_rows;
    if (storage() == nullptr || storage()->categoryCount() == 0
        || (storage()->categoryCount() == 1 && storage()->name(storage()->categories().at(0)).isEmpty()))
        text_rows = 0;
    else
        text_rows = _text_rows;

    if (_orientation == Qt::Horizontal)
        return _image_size.height() + ImageListCategoryFrame::imagesMargins().top() + ImageListCategoryFrame::imagesMargins().bottom()
               + qApp->fontMetrics().lineSpacing() * text_rows;
    else
        return _image_size.width() + ImageListCategoryFrame::imagesMargins().left() + ImageListCategoryFrame::imagesMargins().right()
               + qApp->fontMetrics().lineSpacing() * text_rows;
}

ImageID ImageList::currentImage() const
{
    return _current_image;
}

void ImageList::setCurrentImage(const ImageID& image)
{
    if (_current_image == image)
        return;

    _current_changed_block = true;

    if (_current_image.isValid() && _storage->contains(_current_image))
        imageFrame(_current_image).first->setCurrentImage({});

    if (image.isValid())
        imageFrame(image).first->setCurrentImage(image);

    auto p = _current_image;
    _current_image = image;

    emit sg_currentChanged(p, _current_image);
    _current_changed_block = false;
}

void ImageList::setCategoryMovable(const ImageCateroryID& category_id, bool enabled)
{
    categoryFrame(category_id)->setCategoryMovable(enabled);
}

bool ImageList::isCategoryMovable(const ImageCateroryID& category_id) const
{
    return categoryFrame(category_id)->isCategoryMovable();
}

ImageCateroryID ImageList::currentCategory() const
{
    return _current_image.isValid() ? _storage->category(_current_image) : ImageCateroryID();
}

QSize ImageList::sizeHint() const
{
    QSize size = QWidget::sizeHint();
    size.setHeight(qMax(_image_size.height(), size.height()));
    size.setWidth(qMax(_image_size.width(), size.width()));
    return size;
}

QSize ImageList::minimumSizeHint() const
{
    return sizeHint();
}

void ImageList::sl_categoryRemoved(const ImageCateroryID& category_id)
{
    auto frame = categoryFrame(category_id);
    _categories.removeOne(frame);
    _categories_map.remove(category_id);

    if (currentCategory() == category_id)
        setCurrentImage(ImageID());

    frame->hide();
    frame->deleteLater();

    _drag_move_timer.stop();

    updateLineSize();
    alignCategoryCaptions();
}

void ImageList::sl_categoryAdded(const ImageCateroryID& category_id)
{
    auto frame = new ImageListCategoryFrame(this, category_id);
    _categories << frame;
    _categories_map[category_id] = frame;
    _scroll_area->widgetLayout()->insertWidget(_scroll_area->widgetLayout()->count() - 1, frame);
    connect(frame, &ImageListCategoryFrame::sg_imageSelected, this, &ImageList::sl_frameImageSelected);
    connect(frame, &ImageListCategoryFrame::sg_markerClicked, this, &ImageList::sg_categoryMarkerClicked);

    updateLineSize();
    alignCategoryCaptions();
}

void ImageList::sl_categoryMoved(const ImageCateroryID& category_id, int previous_position, int position)
{
    auto frame = _categories_map.value(category_id);
    Z_CHECK_NULL(frame);

    _categories.move(previous_position, position);
    auto la = qobject_cast<QHBoxLayout*>(_scroll_area->widgetLayout());
    auto item = la->takeAt(previous_position);
    if (position > previous_position)
        position--;
    la->insertItem(position, item);

    updateLineSize();    
}

void ImageList::sl_categoryNameChanged(const ImageCateroryID& category_id, const QString& name)
{
    categoryFrame(category_id)->on_categoryNameChanged(name);
    alignCategoryCaptions();
}

void ImageList::sl_categoryMarkerChanged(const ImageCateroryID& category_id, const QIcon& marker)
{
    categoryFrame(category_id)->on_categoryMarkerChanged(marker);
}

void ImageList::sl_imageMoved(const ImageID& image_id, const ImageCateroryID& previous_category_id, int previous_position,
                              const ImageCateroryID& category_id, int position)
{
    categoryFrame(previous_category_id)->on_imageMoved(image_id, previous_category_id, previous_position, category_id, position);
    categoryFrame(category_id)->on_imageMoved(image_id, previous_category_id, previous_position, category_id, position);

    if (image_id == _current_image) {
        _selection_timer.start();
        _request_current_changed = image_id;
    }

    updateLineSize();
}

void ImageList::sl_imageRemoved(const ImageCateroryID& category_id, const ImageID& image_id)
{
    Q_UNUSED(category_id)

    categoryFrame(category_id)->on_imageRemoved(image_id);

    if (_current_image == image_id)
        setCurrentImage(ImageID());

    alignCategoryCaptions();
}

void ImageList::sl_imageAdded(const ImageID& image_id)
{
    categoryFrame(_storage->category(image_id))->on_imageAdded(image_id);
    connect(imageFrame(image_id).second, &ImageListImageFrame::sg_markerClicked, this, &ImageList::sl_imageMarkerClicked);

    alignCategoryCaptions();
}

void ImageList::sl_imageMarkerClicked(const ImageID& image_id)
{
    emit sg_imageMarkerClicked(storage()->category(image_id), image_id);
}

void ImageList::sl_imageChanged(const ImageID& image_id, const QIcon& icon)
{
    imageFrame(image_id).second->on_imageChanged(icon);
}

void ImageList::sl_imageMarkerChanged(const ImageID& image_id, const QIcon& marker)
{
    imageFrame(image_id).second->on_imageMarkerChanged(marker);
}

void ImageList::sl_optionsChanged(const ImageID& image_id, const ImageListOptions& options)
{
    imageFrame(image_id).second->on_optionsChanged(options);
}

void ImageList::sl_imageNameChanged(const ImageID& image_id, const QString& name)
{
    imageFrame(image_id).second->on_imageNameChanged(name);
}

void ImageList::sl_frameImageSelected(const ImageID& image_id)
{
    if (_current_changed_block)
        return;

    if (_selection_timer.isActive()) {
        if (image_id.isValid())
            _request_current_changed = image_id;
    } else {
        _selection_timer.start();
        _request_current_changed = image_id;
    }
}

void ImageList::sl_selectionChangeTimeout()
{
    _current_changed_block = true;
    auto p = _current_image;

    if (_current_image.isValid()) {
        auto current_category = currentCategory();
        auto category = _request_current_changed.isValid() ? _storage->category(_request_current_changed) : ImageCateroryID();
        if (current_category != category)
            categoryFrame(current_category)->setCurrentImage({});
    }

    bool need_emit = (_request_current_changed != _current_image);

    _current_image = _request_current_changed;
    _current_changed_block = false;
    _request_current_changed.clear();

    if (need_emit)
        emit sg_currentChanged(p, _current_image);
}

void ImageList::reload()
{
    for (auto& c : qAsConst(_categories)) {
        c->hide();
        c->deleteLater();
    }
    _categories.clear();
    _categories_map.clear();
    if (_storage == nullptr)
        return;

    _block_align_captions++;

    auto categories = _storage->categories();
    for (auto& c : qAsConst(categories)) {
        sl_categoryAdded(c);

        auto category_frame = categoryFrame(c);

        auto images = _storage->images(c);
        for (auto& i : qAsConst(images)) {
            category_frame->addImage(i);
            connect(imageFrame(i).second, &ImageListImageFrame::sg_markerClicked, this, &ImageList::sl_imageMarkerClicked);
        }
    }

    _block_align_captions--;

    alignCategoryCaptions();
    updateGeometry();
}

void ImageList::updateLineSize()
{
    _scroll_area->setLineSize(realSize());
    updateGeometry();
}

void ImageList::updateControlsStatus()
{
    for (auto& c : _storage->categories()) {
        categoryFrame(c)->updateControlsStatus();
    }
}

void ImageList::alignCategoryCaptions()
{
    if (_block_align_captions > 0)
        return;

    if (_orientation == Qt::Vertical)
        return;

    _align_category_captions_timer.start();
}

void ImageList::alignCategoryCaptionsHelper()
{
    if (_block_align_captions > 0)
        return;

    if (_orientation == Qt::Vertical)
        return;

    int max_height = 0;

    for (auto& c : _storage->categories()) {
        if (!isVisible(c))
            continue;
        max_height = qMax(max_height, categoryFrame(c)->captionHeight());
    }

    for (auto& c : _storage->categories()) {
        if (!isVisible(c))
            continue;
        categoryFrame(c)->setMinimumCaptionHeight(max_height);
    }
}

void ImageList::dragMoveRequest(const ImageID& image_id, const ImageCateroryID& target_category_id, int position)
{
    _drag_move_timer.start();
    _drag_move_image_id = image_id;
    _drag_move_target_category_id = target_category_id;
    _drag_move_position = position;
}

ImageListCategoryFrame* ImageList::categoryFrame(const ImageCateroryID& category_id) const
{
    Z_CHECK_NULL(_storage);
    auto res = _categories_map.value(category_id);
    Z_CHECK_NULL(res);
    return res;
}

QPair<ImageListCategoryFrame*, ImageListImageFrame*> ImageList::imageFrame(const ImageID& image_id) const
{
    Z_CHECK_NULL(_storage);
    auto frame = categoryFrame(_storage->category(image_id));
    auto image = frame->image(image_id);
    Z_CHECK_NULL(image);
    return {frame, image};
}

ImageID ImageList::findImageID(ImageListImageFrame* image_frame) const
{
    for (auto c : qAsConst(_categories)) {
        auto id = c->findImage(image_frame);
        if (id.isValid())
            return id;
    }
    return ImageID();
}

ImageCateroryID::ImageCateroryID()
{
}

ImageCateroryID::ImageCateroryID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > 0);
}

QVariant ImageCateroryID::variant() const
{
    return QVariant::fromValue(*this);
}

ImageCateroryID ImageCateroryID::fromVariant(const QVariant& v)
{
    return v.value<ImageCateroryID>();
}

ImageID::ImageID()
{
}

ImageID::ImageID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > 0);
}

QVariant ImageID::variant() const
{
    return QVariant::fromValue(*this);
}

ImageID ImageID::fromVariant(const QVariant& v)
{
    return v.value<ImageID>();
}

} // namespace zf
