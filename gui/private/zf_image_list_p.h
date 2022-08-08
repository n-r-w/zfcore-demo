#pragma once

#include "zf.h"
#include "zf_image_list_storage.h"
#include <QFrame>
#include <QTimer>

class QToolButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;

namespace zf
{
class Label;
class ImageList;
class ImageListImageFrame;

//! Отображение рамки категории для ImageList
class ImageListCategoryFrame : public QFrame
{
    Q_OBJECT
public:
    ImageListCategoryFrame(ImageList* image_list, const ImageCateroryID& category_id);

    ImageCateroryID id() const;

    //! Картинка по id
    ImageListImageFrame* image(const ImageID& image_id) const;
    //! Поиск id картинки по ее фрейму
    ImageID findImage(ImageListImageFrame* frame) const;

    //! Выделить картинку
    void setCurrentImage(const ImageID& image_id);
    //! Текущее выделение
    ImageID currentImage() const;

    //! Добавить картинку
    ImageListImageFrame* addImage(const ImageID& image_id);
    //! Добавить картинку
    ImageListImageFrame* insertImage(const ImageID& image_id, int pos);
    //! Удалить картинку
    void removeImage(const ImageID& image_id);
    //! Удалить картинку из фрейма, но не удалять сам виджет
    ImageListImageFrame* takeImage(const ImageID& image_id);
    //! Переместить картинку. Истина, если перемещение потребовалось
    bool moveImage(const ImageID& image_id, int pos);

    //! Задать минимальную высоту заголовка
    void setMinimumCaptionHeight(int h);
    int captionHeight();

    //! Можно ли нажимать на маркер
    bool isMarkerClickable() const;
    void setMarkerClickable(bool b);

    //! Разрешить/запретить перетаскивание в категории
    void setCategoryMovable(bool enabled);
    //! Разрешено ли перетаскивание в категории
    bool isCategoryMovable() const;

    //! Выравнивание текста в заголовке категории
    Qt::Alignment captionAlignment() const;
    void setCaptionAlignment(Qt::Alignment alignment);

    //! Шрифт заголовка категории
    QFont captionFont() const;
    void setCaptionFont(const QFont& font);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    //! Общий размер отступов вокруг картинок
    static QMargins imagesMargins();

    //! Обновить доступность контролов и т.п.
    void updateControlsStatus();

    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    //! Изображение было удалено
    void on_imageRemoved(const zf::ImageID& image_id);
    //! Изображение было добавлено
    void on_imageAdded(const zf::ImageID& image_id);
    //! Изображение было перемещено
    void on_imageMoved(const zf::ImageID& image_id, const zf::ImageCateroryID& previous_category_id, int previous_position,
                       const zf::ImageCateroryID& category_id, int position);
    //! Изменилось имя для категории
    void on_categoryNameChanged(const QString& name);
    //! Изменился маркер для категории
    void on_categoryMarkerChanged(const QIcon& marker);

signals:
    //! Выделена картинка
    void sg_imageSelected(const zf::ImageID& image_id);
    //! Нажатие на маркер
    void sg_markerClicked(const zf::ImageCateroryID& category_id);

private slots:    
    //! Изменился размер маркера
    void sl_markerSizeChanged(int size);    
    //! Нажатие на маркер
    void sl_markerClicked();

private:
    void updateInternalGeometry();
    void updateCaption();
    void updateMarker();
    int categoryMinSize() const;

    //! Позиция в которую принимает drop. -1 если не принимает
    int getDropPosition(const QPoint& pos, const ImageID& image_id) const;

    inline static QMargins FRAME_MARGINS = {0, 3, 0, 0};
    inline static QMargins FRAME_MARGINS_NO_CAPTION = {0, 0, 0, 0};
    inline static QMargins CAPTION_MARGINS = {6, 0, 3, 3};
    inline static QMargins IMAGES_MARGINS = {0, 0, 0, 0};

    ImageList* _image_list;
    ImageCateroryID _category_id;
    Label* _caption;
    int _minimum_caption_height = 0;
    QToolButton* _marker;
    bool _marker_clickable = true;
    bool _movable = true;

    QVBoxLayout* _main_la;
    QFrame* _main_widget;
    QVBoxLayout* _main_widget_la;
    QFrame* _caption_widget;
    QHBoxLayout* _images_la;
    QFrame* _rows_frame;

    QList<ImageID> _image_ids;
    QList<ImageListImageFrame*> _image_frames;

    ImageID _current_image;
    QTimer _update_caption_timer;    
};

class ImageListImageFrame : public QFrame
{
    Q_OBJECT
public:
    inline static QMargins MARGINS = {8, 8, 8, 8};

    ImageListImageFrame(ImageList* image_list, const ImageID& image_id);

    ImageID id() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    //! Фактический размер картинки
    QSize imageSize() const;

    //! Обновить доступность контролов и т.п.
    void updateControlsStatus();

    //! Можно ли нажимать на маркер
    bool isMarkerClickable() const;
    void setMarkerClickable(bool b);

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

    QMimeData* encodeMimeData() const;
    static bool decodeMimeData(const QMimeData* data, ImageID& image_id);

    inline static const QString MimeType = "application/x-ImageListImageFrame";

    //! Изменилось изображение
    void on_imageChanged(const QIcon& icon);
    //! Изменился маркер для изображения
    void on_imageMarkerChanged(const QIcon& marker);
    //! Изменение в свойствах картинки
    void on_optionsChanged(const zf::ImageListOptions& options);
    //! Изменилось имя для изображения
    void on_imageNameChanged(const QString& name);

private slots:    
    //! Изменился размер маркера
    void sl_markerSizeChanged(int size);
    //! Картинка выбрана
    void sl_currentChanged(const zf::ImageID& previous_image_id, const zf::ImageID& image_id);

    //! Нажатие на маркер
    void sl_markerClicked();

signals:
    //! Нажатие на маркер
    void sg_markerClicked(const zf::ImageID& image_id);

private:
    void updateInternalGeometry();
    void updateImage();
    void updateMarker();
    void updateCurrent();
    void updateStyle();

    ImageList* _image_list;
    ImageID _image_id;
    QLabel* _image_label;
    Label* _caption;
    QToolButton* _marker;
    bool _marker_clickable = true;
    QPoint _drag_start_pos;
    bool _is_current = false;    
};

} // namespace zf
