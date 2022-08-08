#include "zf_image_list_storage.h"
#include "zf_core.h"

namespace zf
{

ImageListStorage::ImageListStorage(QObject* parent)
    : QObject(parent)
{
}

int ImageListStorage::categoryCount() const
{
    return _categories.count();
}

int ImageListStorage::imageCount(const ImageCateroryID& category_id) const
{
    return info(category_id)->images.count();
}

int ImageListStorage::imageCount() const
{
    return _image_map.count();
}

QList<ImageCateroryID> ImageListStorage::categories() const
{
    QList<ImageCateroryID> res;
    for (const auto& c : qAsConst(_categories)) {
        res << c->id;
    }
    return res;
}

QList<ImageID> ImageListStorage::images(const ImageCateroryID& category_id) const
{
    auto info = _categories_map.value(category_id);
    QList<ImageID> res;
    if (info == nullptr)
        return res;

    for (const auto& c : qAsConst(info->images)) {
        res << c->id;
    }
    return res;
}

QMultiMap<ImageCateroryID, ImageID> ImageListStorage::images() const
{
    QMultiMap<ImageCateroryID, ImageID> res;
    for (const auto& c : qAsConst(_categories)) {
        for (const auto& i : qAsConst(c->images)) {
            res.insert(c->id, i->id);
        }
    }
    return res;
}

ImageCateroryID ImageListStorage::category(const ImageID& image_id) const
{
    return info(image_id)->category->id;
}

bool ImageListStorage::contains(const ImageCateroryID& category_id) const
{
    return _categories_map.contains(category_id);
}

bool ImageListStorage::contains(const ImageID& image_id) const
{
    return _image_map.contains(image_id);
}

QString ImageListStorage::name(const ImageCateroryID& category_id) const
{
    return info(category_id)->name;
}

void ImageListStorage::setName(const ImageCateroryID& category_id, const QString& name)
{
    if (this->name(category_id) == name)
        return;

    info(category_id)->name = name;
    emit sg_categoryNameChanged(category_id, name);
}

QString ImageListStorage::name(const ImageID& image_id) const
{
    return info(image_id)->name;
}

void ImageListStorage::setName(const ImageID& image_id, const QString& name)
{
    if (this->name(image_id) == name)
        return;

    info(image_id)->name = name;
    emit sg_imageNameChanged(image_id, name);
}

int ImageListStorage::minimumSize(const ImageCateroryID& category_id) const
{
    return info(category_id)->minimum_size;
}

QIcon ImageListStorage::icon(const ImageID& image_id) const
{
    return info(image_id)->icon;
}

void ImageListStorage::setIcon(const ImageID& image_id, const QIcon& icon)
{
    info(image_id)->icon = icon;
    emit sg_imageChanged(image_id, icon);
}

QIcon ImageListStorage::marker(const ImageID& image_id) const
{
    return info(image_id)->marker;
}

QIcon ImageListStorage::marker(const ImageCateroryID& category_id) const
{
    return info(category_id)->marker;
}

void ImageListStorage::setMarker(const ImageCateroryID& category_id, const QIcon& marker)
{
    info(category_id)->marker = marker;
    emit sg_categoryMarkerChanged(category_id, marker);
}

void ImageListStorage::setMarker(const ImageID& image_id, const QIcon& marker)
{
    info(image_id)->marker = marker;
    emit sg_imageMarkerChanged(image_id, marker);
}

QVariant ImageListStorage::data(const ImageID& image_id) const
{
    return info(image_id)->data;
}

QVariant ImageListStorage::data(const ImageCateroryID& category_id) const
{
    return info(category_id)->data;
}

void ImageListStorage::setData(const ImageCateroryID& category_id, const QVariant& data)
{
    info(category_id)->data = data;
}

void ImageListStorage::setData(const ImageID& image_id, const QVariant& data)
{
    info(image_id)->data = data;
}

int ImageListStorage::position(const ImageCateroryID& category_id) const
{
    return _categories.indexOf(info(category_id));
}

int ImageListStorage::position(const ImageID& image_id) const
{
    return info(category(image_id))->images.indexOf(info(image_id));
}

void ImageListStorage::addCategory(const ImageCateroryID& category_id, const QString& name, int minimum_size)
{
    Z_CHECK(category_id.isValid());
    Z_CHECK(!contains(category_id));    
    Z_CHECK(minimum_size > 0);

    if (categoryCount() > 0)
        Z_CHECK(!name.isEmpty());

    auto info = Z_MAKE_SHARED(CategoryInfo);
    info->id = category_id;
    info->name = name;
    info->minimum_size = minimum_size;

    _categories << info;
    _categories_map[category_id] = info;

    emit sg_categoryAdded(category_id);
}

void ImageListStorage::remove(const ImageCateroryID& category_id)
{
    auto i_info = info(category_id);
    for (int i = i_info->images.count() - 1; i >= 0; i--) {
        remove(i_info->images.at(i)->id);
    }
    _categories.removeOne(i_info);
    _categories_map.remove(category_id);

    emit sg_categoryRemoved(category_id);
}

void ImageListStorage::addImage(const ImageID& image_id, const ImageCateroryID& category_id, const QIcon& icon, const QString& name)
{
    Z_CHECK(image_id.isValid());
    Z_CHECK(category_id.isValid());
    Z_CHECK(contains(category_id));
    Z_CHECK(!contains(image_id));

    auto info = Z_MAKE_SHARED(ImageInfo);
    info->id = image_id;
    info->name = name;
    info->icon = icon.isNull() ? Utils::resizeSvg(":/share_icons/document.svg", 512) : icon;
    info->options = ImageListOption::CanMove | ImageListOption::CanShift;

    auto c_info = this->info(category_id);
    c_info->images << info;
    info->category = c_info.get();

    _image_map[image_id] = info;

    emit sg_imageAdded(image_id);
}

Error ImageListStorage::addImage(const ImageID& image_id, const ImageCateroryID& category_id, const QByteArray& data, FileType file_type,
                                 const QString& name)
{
    QPixmap image;
    if (!image.loadFromData(data, file_type != FileType::Undefined ? Utils::fileExtensions(file_type).constFirst().toLocal8Bit().constData()
                                                                   : nullptr))
        return Error::corruptedDataError();

    addImage(image_id, category_id, QIcon(image), name);
    return {};
}

Error ImageListStorage::addSvg(const ImageID& image_id, const ImageCateroryID& category_id, const QString& svg_file_name, const QString& name)
{
    addImage(image_id, category_id, QIcon(svg_file_name), name);
    return {};
}

void ImageListStorage::remove(const ImageID& image_id)
{
    auto i_info = info(image_id);
    i_info->category->images.removeOne(i_info);
    _image_map.remove(image_id);

    emit sg_imageRemoved(i_info->category->id, image_id);
}

void ImageListStorage::clear()
{
    for (int i = categoryCount() - 1; i >= 0; i--) {
        clear(categories().at(i));
    }
}

void ImageListStorage::clear(const ImageCateroryID& category_id)
{
    for (int i = imageCount(category_id) - 1; i >= 0; i--) {
        remove(images(category_id).at(i));
    }
}

bool ImageListStorage::isCategoryReadOnly(const ImageCateroryID& category_id) const
{
    return info(category_id)->read_only;
}

void ImageListStorage::setCategoryReadOnly(const ImageCateroryID& category_id, bool b)
{
    info(category_id)->read_only = b;
}

Error ImageListStorage::move(const ImageCateroryID& category_id, int position)
{
    int cur_pos = this->position(category_id);
    if (cur_pos == position)
        return {};

    _categories.move(cur_pos, position);

    emit sg_categoryMoved(category_id, cur_pos, position);
    return {};
}

Error ImageListStorage::move(const ImageID& image_id, const ImageCateroryID& target_category_id, int position)
{
    if (!canMove(image_id, target_category_id, position))
        return Error::forbiddenError();

    auto c_info = info(category(image_id));
    auto target_c_info = info(target_category_id);
    int cur_pos = this->position(image_id);
    Z_CHECK(cur_pos >= 0);

    if (c_info->id == target_category_id) {
        if (cur_pos == position)
            return {};

        Z_CHECK(position < c_info->images.count());
        c_info->images.move(cur_pos, position);

    } else {
        auto i_info = info(image_id);
        i_info->category = target_c_info.get();

        c_info->images.removeAt(cur_pos);
        target_c_info->images.insert(position, i_info);
    }

    emit sg_imageMoved(image_id, c_info->id, cur_pos, target_category_id, position);
    return {};
}

bool ImageListStorage::canMove(const ImageID& image_id, const ImageCateroryID& target_category_id, int position) const
{
    Z_CHECK(position >= 0);

    auto opt = options(image_id);

    auto c_info = info(category(image_id));
    auto target_c_info = info(target_category_id);

    if (c_info->read_only || target_c_info->read_only)
        return false;

    int cur_pos = this->position(image_id);
    Z_CHECK(cur_pos >= 0);

    if (c_info->id == target_category_id) {
        if (cur_pos == position)
            return true;

        Z_CHECK(position < c_info->images.count());

        if (!opt.testFlag(ImageListOption::CanMove) || !opt.testFlag(ImageListOption::CanShift))
            return false;

        if (cur_pos < position) {
            for (int i = cur_pos + 1; i <= position; i++) {
                if (!c_info->images.at(i)->options.testFlag(ImageListOption::CanShift))
                    return false;
            }
        } else {
            for (int i = position; i < cur_pos; i++) {
                if (!c_info->images.at(i)->options.testFlag(ImageListOption::CanShift))
                    return false;
            }
        }

    } else {
        if (!opt.testFlag(ImageListOption::CanMove) || !opt.testFlag(ImageListOption::CanShift))
            return false;

        for (int i = cur_pos + 1; i < c_info->images.count(); i++) {
            if (!c_info->images.at(i)->options.testFlag(ImageListOption::CanShift))
                return false;
        }

        auto i_info = info(image_id);
        for (int i = position; i < target_c_info->images.count(); i++) {
            if (!target_c_info->images.at(i)->options.testFlag(ImageListOption::CanShift))
                return false;
        }
    }

    return true;
}

ImageListOptions ImageListStorage::options(const ImageID& image_id) const
{
    return info(image_id)->options;
}

void ImageListStorage::setOptions(const ImageID& image_id, const ImageListOptions& options)
{
    auto i = info(image_id);

    if (i->options == options)
        return;

    i->options = options;
    emit sg_optionsChanged(image_id, options);
}

ImageListStorage::ImageInfoPtr ImageListStorage::info(ImageID id) const
{
    auto info = _image_map.value(id);
    Z_CHECK(info != nullptr);
    return info;
}

ImageListStorage::CategoryInfoPtr ImageListStorage::info(ImageCateroryID id) const
{
    auto info = _categories_map.value(id);
    Z_CHECK(info != nullptr);
    return info;
}
} // namespace zf
