#pragma once

#include "zf_image_list_storage.h"
#include <QTimer>

class QScrollArea;

namespace zf
{
class ImageListCategoryFrame;
class ImageListImageFrame;
class LineScrollArea;

//! Список в превью изображений, разделенных на категории
class ZCORESHARED_EXPORT ImageList : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QSize imageSize READ imageSize WRITE setImageSize DESIGNABLE true STORED true)
    Q_PROPERTY(int categoryMarkerSize READ categoryMarkerSize WRITE setCategoryMarkerSize DESIGNABLE true STORED true)
    Q_PROPERTY(int imageMarkerSize READ imageMarkerSize WRITE setImageMarkerSize DESIGNABLE true STORED true)
    Q_PROPERTY(int textRows READ textRows WRITE setTextRows DESIGNABLE true STORED true)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)
    Q_PROPERTY(bool dragImageEnabled READ isDragEnabled WRITE setDragEnabled DESIGNABLE true STORED true)

public:
    explicit ImageList(ImageListStorage* storage, const QSize& image_size,
                       //! Количество строк текста в заголовках групп
                       int text_rows = 1, Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);
    explicit ImageList(const QSize& image_size,
                       //! Количество строк текста в заголовках групп
                       int text_rows = 1, Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);
    explicit ImageList(QWidget* parent = nullptr);

    //! Ориентация. Пока не поддерживается смена ориентации
    Qt::Orientation orientation() const;

    //! Режим только для чтения
    bool isReadOnly() const;
    void setReadOnly(bool b);

    //! Разрешено перетаскивание
    bool isDragEnabled() const;
    void setDragEnabled(bool b);

    //! Количество строк текста в заголовках групп
    int textRows() const;
    void setTextRows(int n);

    //! Хранилище картинок
    ImageListStorage* storage() const;
    //! Указать картинок
    void setStorage(ImageListStorage* storage);

    //! Размер картинки по умолчанию
    QSize imageSize() const;
    //! Задать размер картинки по умолчанию
    void setImageSize(const QSize& size);

    //! Размер маркера категории
    int categoryMarkerSize() const;
    //! Задать размер маркера категории
    void setCategoryMarkerSize(int size);

    //! Выравнивание текста в заголовке категории
    Qt::Alignment categoryCaptionAlignment(const ImageCateroryID& category_id) const;
    void setCategoryCaptionAlignment(const ImageCateroryID& category_id, Qt::Alignment alignment);

    //! Шрифт заголовка категории
    QFont categoryСaptionFont(const ImageCateroryID& category_id) const;
    void setСategoryCaptionFont(const ImageCateroryID& category_id, const QFont& font);

    //! Размер маркера изображения
    int imageMarkerSize() const;
    //! Задать размер маркера изображения
    void setImageMarkerSize(int size);

    //! Видимость категории
    bool isVisible(const ImageCateroryID& category_id) const;
    //! Видимость картинки
    bool isVisible(const ImageID& image_id) const;
    //! Задать видимость категории
    void setVisible(const ImageCateroryID& category_id, bool b);
    //! Задать видимость картинки
    void setVisible(const ImageID& image_id, bool b);

    //! Можно ли нажимать на маркер категории
    bool isMarkerClickable(const ImageCateroryID& category_id) const;
    void setMarkerClickable(const ImageCateroryID& category_id, bool b);
    //! Можно ли нажимать на маркер картинки
    bool isMarkerClickable(const ImageID& image_id) const;
    void setMarkerClickable(const ImageID& image_id, bool b);

    //! Фактическая высота(Qt::Horizontal)/ширина(Qt::Vertical) с учетом отступов
    int realSize() const;

    //! Текущая картинка
    ImageID currentImage() const;
    //! Задать текущую картинку
    void setCurrentImage(const ImageID& image);

    //! Разрешить/запретить перетаскивание в категории
    void setCategoryMovable(const ImageCateroryID& category_id, bool enabled);
    //! Разрешено ли перетаскивание в категории
    bool isCategoryMovable(const ImageCateroryID& category_id) const;

    //! Текущая категория
    ImageCateroryID currentCategory() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    using QWidget::isVisible;
    using QWidget::setVisible;

signals:
    //! Картинка выбрана
    void sg_currentChanged(const zf::ImageID& previous_image_id, const zf::ImageID& image_id);

    //! Изменился размер маркера категории
    void sg_categoryMarkerSizeChanged(int size);
    //! Изменился размер маркера изображения
    void sg_imageMarkerSizeChanged(int size);

    //! Нажатие на маркер категории
    void sg_categoryMarkerClicked(const zf::ImageCateroryID& category_id);
    //! Нажатие на маркер изображения
    void sg_imageMarkerClicked(const zf::ImageCateroryID& category_id, const zf::ImageID& image_id);

private slots:
    //! Категория была удалена
    void sl_categoryRemoved(const zf::ImageCateroryID& category_id);
    //! Категория была добавлена
    void sl_categoryAdded(const zf::ImageCateroryID& category_id);
    //! Изображение было перемещено
    void sl_categoryMoved(const zf::ImageCateroryID& category_id, int previous_position, int position);
    //! Изменилось имя для категории
    void sl_categoryNameChanged(const zf::ImageCateroryID& category_id, const QString& name);
    //! Изменился маркер для категории
    void sl_categoryMarkerChanged(const zf::ImageCateroryID& category_id, const QIcon& marker);
    //! Изображение было перемещено
    void sl_imageMoved(const zf::ImageID& image_id, const zf::ImageCateroryID& previous_category_id, int previous_position,
                       const zf::ImageCateroryID& category_id, int position);
    //! Изображение было удалено
    void sl_imageRemoved(const zf::ImageCateroryID& category_id, const zf::ImageID& image_id);
    //! Изображение было добавлено
    void sl_imageAdded(const zf::ImageID& image_id);
    //! Нажатие на маркер изображения
    void sl_imageMarkerClicked(const zf::ImageID& image_id);
    //! Изменилось изображение
    void sl_imageChanged(const zf::ImageID& image_id, const QIcon& icon);
    //! Изменился маркер для изображения
    void sl_imageMarkerChanged(const zf::ImageID& image_id, const QIcon& marker);
    //! Изменение в свойствах картинки
    void sl_optionsChanged(const zf::ImageID& image_id, const zf::ImageListOptions& options);
    //! Изменилось имя для изображения
    void sl_imageNameChanged(const zf::ImageID& image_id, const QString& name);

    //! Поменялось текущее выделение
    void sl_frameImageSelected(const zf::ImageID& image_id);
    //! Таймер смены текущего элемента
    void sl_selectionChangeTimeout();

private:
    void reload();
    //! Обновить высоту/ширину виджета
    void updateLineSize();
    //! Обновить доступность контролов и т.п.
    void updateControlsStatus();
    //! Выровнять высоту заголовков категорий
    void alignCategoryCaptions();
    //! Выровнять высоту заголовков категорий
    void alignCategoryCaptionsHelper();

    //! Переместить изображение через drag&drop (position - положение, в котором она должны быть)
    void dragMoveRequest(const ImageID& image_id, const ImageCateroryID& target_category_id, int position);

    ImageListCategoryFrame* categoryFrame(const ImageCateroryID& category_id) const;
    QPair<ImageListCategoryFrame*, ImageListImageFrame*> imageFrame(const ImageID& image_id) const;
    //! Если не найдено, то не валидно
    ImageID findImageID(ImageListImageFrame* image_frame) const;

    //! Ориентация
    Qt::Orientation _orientation;
    //! Количество строк текста в заголовках групп
    int _text_rows;
    //! Источник данных
    ImageListStorage* _storage = nullptr;
    //! Скролл
    LineScrollArea* _scroll_area;

    //! Фреймы категорий
    QList<ImageListCategoryFrame*> _categories;
    QMap<ImageCateroryID, ImageListCategoryFrame*> _categories_map;

    //! Целевой размер одной картинки
    QSize _image_size;
    //! Текущая картинка
    ImageID _current_image;

    //! Размер маркера категории
    int _category_marker_size;
    //! Размер маркера изображения
    int _image_marker_size;

    //! Отложенная смена текущего элемента
    QTimer _selection_timer;
    ImageID _request_current_changed;
    bool _current_changed_block = false;

    //! Реакция на перемещение по таймеру. Иначе будет креш при удалении фреймов
    QTimer _drag_move_timer;
    ImageID _drag_move_image_id;
    ImageCateroryID _drag_move_target_category_id;
    int _drag_move_position = -1;

    //! Режим только для чтения
    bool _read_only = false;
    //! Разрешено перетаскивание
    bool _drag_enabled = true;

    //! Блокировка выравнивания заголовков
    int _block_align_captions = 0;

    QTimer _align_category_captions_timer;

    friend class ImageListImageFrame;
    friend class ImageListCategoryFrame;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::ImageCateroryID)
Q_DECLARE_METATYPE(zf::ImageID)
