#pragma once

#include "zf_error.h"
#include "zf_basic_types.h"

#include <QIcon>

namespace zf
{
//! Идентификатор категории изображения
class ZCORESHARED_EXPORT ImageCateroryID : public ID_Wrapper
{
public:
    ImageCateroryID();
    explicit ImageCateroryID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static ImageCateroryID fromVariant(const QVariant& v);
};

//! Идентификатор изображения
class ZCORESHARED_EXPORT ImageID : public ID_Wrapper
{
public:
    ImageID();
    explicit ImageID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static ImageID fromVariant(const QVariant& v);
};

inline uint qHash(const zf::ImageCateroryID& d)
{
    return ::qHash(d.value());
}
inline uint qHash(const zf::ImageID& d)
{
    return ::qHash(d.value());
}

//! Хранилище изображений, разбитых по категориям
class ZCORESHARED_EXPORT ImageListStorage : public QObject
{
    Q_OBJECT
public:
    explicit ImageListStorage(QObject* parent = nullptr);

    //! Количество категорий
    int categoryCount() const;
    //! Количество картинок в категории
    int imageCount(const ImageCateroryID& category_id) const;
    //! Общее количество картинок
    int imageCount() const;

    //! Все категории
    QList<ImageCateroryID> categories() const;

    //! Список картинок для категории
    QList<ImageID> images(const ImageCateroryID& category_id) const;
    //! Все картинки
    QMultiMap<ImageCateroryID, ImageID> images() const;

    //! Категория для картинки
    ImageCateroryID category(const ImageID& image_id) const;

    //! Есть ли такая категория
    bool contains(const ImageCateroryID& category_id) const;
    //! Есть ли такая картинка
    bool contains(const ImageID& image_id) const;

    //! Имя категории
    QString name(const ImageCateroryID& category_id) const;
    void setName(const ImageCateroryID& category_id, const QString& name);

    //! Имя катринки
    QString name(const ImageID& image_id) const;
    void setName(const ImageID& image_id, const QString& name);

    //! Минимальная ширина категории (в символах)
    int minimumSize(const ImageCateroryID& category_id) const;

    //! Изображение
    QIcon icon(const ImageID& image_id) const;
    //! Задать изображение
    void setIcon(const ImageID& image_id, const QIcon& icon);

    //! Маркер для категории
    QIcon marker(const ImageID& image_id) const;
    //! Маркер для изображения
    QIcon marker(const ImageCateroryID& category_id) const;

    //! Задать маркер для категории
    void setMarker(const ImageCateroryID& category_id, const QIcon& marker);
    //! Задать маркер для изображения
    void setMarker(const ImageID& image_id, const QIcon& marker);

    //! Произвольные данные для категории
    QVariant data(const ImageID& image_id) const;
    //! Произвольные данные для изображения
    QVariant data(const ImageCateroryID& category_id) const;

    //! Задать произвольные данные для категории
    void setData(const ImageCateroryID& category_id, const QVariant& data);
    //! Задать произвольные данные для изображения
    void setData(const ImageID& image_id, const QVariant& data);

    //! Положение категории в списке других категория
    int position(const ImageCateroryID& category_id) const;
    //! Положение изображения в рамках категории
    int position(const ImageID& image_id) const;

    //! Добавить категорию
    void addCategory(const ImageCateroryID& category_id,
                     //! Имя категории. Должно быть задано если имеется больше одной категории
                     const QString& name = QString(),
                     //! Минимальная ширина категории (в символах)
                     int minimum_size = 10);
    //! Удалить категорию (удаляет все картинки в ней)
    void remove(const ImageCateroryID& category_id);

    //! Добавить изображение в категорию
    void addImage(const ImageID& image_id, const ImageCateroryID& category_id, const QIcon& icon, const QString& name = QString());
    //! Добавить изображение в категорию. SVG добавлять методом addSvg!
    Error addImage(const ImageID& image_id, const ImageCateroryID& category_id, const QByteArray& data,
        FileType file_type = FileType::Undefined, const QString& name = QString());
    //! Добавить SVG изображение в категорию
    Error addSvg(const ImageID& image_id, const ImageCateroryID& category_id, const QString& svg_file_name, const QString& name = QString());
    //! Удалить изображение
    void remove(const ImageID& image_id);

    //! Очистить все
    void clear();
    //! Очистить категорию
    void clear(const ImageCateroryID& category_id);

    //! Категория закрыта для изменений (нельзя вставлять или перемещать картинки)
    bool isCategoryReadOnly(const ImageCateroryID& category_id) const;
    //! Категория закрыта для изменений (нельзя вставлять или перемещать картинки)
    void setCategoryReadOnly(const ImageCateroryID& category_id, bool b);

    //! Переместить категорию (position - положение, в котором она должны быть)
    Error move(const ImageCateroryID& category_id, int position);
    //! Переместить изображение (position - положение, в котором она должны быть)
    Error move(const ImageID& image_id, const ImageCateroryID& target_category_id, int position);
    //! Можно ли переместить изображение на основании его параметров. Метод не проверяет выход за границы, только параметры изображения
    bool canMove(const ImageID& image_id, const ImageCateroryID& target_category_id, int position) const;

    //! Параметры для изображения
    ImageListOptions options(const ImageID& image_id) const;
    //! Задать параметры для изображения
    void setOptions(const ImageID& image_id, const ImageListOptions& options);

signals:
    //! Категория была удалена
    void sg_categoryRemoved(const zf::ImageCateroryID& category_id);
    //! Изображение было удалено
    void sg_imageRemoved(const zf::ImageCateroryID& category_id, const zf::ImageID& image_id);

    //! Категория была добавлена
    void sg_categoryAdded(const zf::ImageCateroryID& category_id);
    //! Изображение было добавлено
    void sg_imageAdded(const zf::ImageID& image_id);

    //! Изображение было перемещено
    void sg_categoryMoved(const zf::ImageCateroryID& category_id, int previous_position, int position);
    //! Изображение было перемещено
    void sg_imageMoved(const zf::ImageID& image_id, const zf::ImageCateroryID& previous_category_id, int previous_position,
                       const zf::ImageCateroryID& category_id, int position);

    //! Изменилось изображение
    void sg_imageChanged(const zf::ImageID& image_id, const QIcon& icon);

    //! Изменился маркер для категории
    void sg_categoryMarkerChanged(const zf::ImageCateroryID& category_id, const QIcon& marker);
    //! Изменился маркер для изображения
    void sg_imageMarkerChanged(const zf::ImageID& image_id, const QIcon& marker);

    //! Изменился маркер для категории
    void sg_categoryNameChanged(const zf::ImageCateroryID& category_id, const QString& name);
    //! Изменилось имя для изображения
    void sg_imageNameChanged(const zf::ImageID& image_id, const QString& name);

    //! Изменение в свойствах картинки
    void sg_optionsChanged(const zf::ImageID& image_id, const zf::ImageListOptions& options);

private:
    struct CategoryInfo;
    struct ImageInfo;
    typedef std::shared_ptr<CategoryInfo> CategoryInfoPtr;
    typedef std::shared_ptr<ImageInfo> ImageInfoPtr;

    struct CategoryInfo
    {
        ImageCateroryID id;
        QString name;
        QIcon marker;
        int minimum_size = 0;
        QVariant data;
        bool read_only = false;
        QList<ImageInfoPtr> images;
    };

    struct ImageInfo
    {
        ImageID id;
        QString name;
        QIcon icon;
        QIcon marker;
        QVariant data;
        ImageListOptions options;
        CategoryInfo* category = nullptr;
    };

    ImageInfoPtr info(ImageID id) const;
    CategoryInfoPtr info(ImageCateroryID id) const;

    QList<CategoryInfoPtr> _categories;
    QMap<ImageCateroryID, CategoryInfoPtr> _categories_map;
    QMap<ImageID, ImageInfoPtr> _image_map;
};

} // namespace zf
