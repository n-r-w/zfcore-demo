#pragma once

#include "zf_error.h"
#include <QIcon>
#include <QTimer>

class QScrollArea;
class QLabel;

namespace zf
{
//! Просмотр, масштабирование и поворот картинок с прокруткой
class ZCORESHARED_EXPORT PictureViewer : public QWidget
{
    Q_OBJECT
public:
    PictureViewer(QWidget* parent = nullptr);

    void clear();

    //! Установить картинку
    void setIcon(const QIcon& icon);

    //! Установить картинку из файла
    Error loadFile(const QString& file_name);
    //! Загрузить из bytearray (файл)
    Error setFileData(
        //! Данные файла
        const QByteArray& file_data,
        //! Тип файла (расширение)
        const QString& file_type = QString());
    //! Загрузить из bytearray (файл)
    Error setFileData(
        //! Данные файла
        const QByteArray& file_data,
        //! Расширение файлов, относящихся к этим данным
        FileType file_type);

    //! Картинка (оригинальная)
    QIcon icon() const;
    //! Оригинальный файл (при загрузке через loadFile или setFileData)
    QByteArray fileData() const;
    //! Тип файла (расширение в верхнем регистре) (при загрузке через loadFile или setFileData)
    QString fileType() const;

    //! Увеличить/уменьшить пропорционально (1.1 = +10%)
    void scaleImage(double factor);
    //! Задать масштаб (1 = 100%)
    void setScale(double value);
    //! Текущий масштаб
    double scaleFactor() const;

    //! Задать минимальный масштаб
    void setMinimumScale(double scale);
    //! Минимальный масштаб
    double minimumScale() const;

    //! Задать максимальный масштаб
    void setMaximumScale(double scale);
    //! Максимальный масштаб
    double maximumScale() const;

    //! Подогнать по ширине
    void fitToWidth();
    //! Подогнать по высоте
    void fitToHeight();
    //! Подогнать по ширине и высоте
    void fitToPage();

    //! На какой угол повернута картинка
    int rotated() const;
    //! Установить угол поворота в градусах (положительное значение - по часовой стрелке)
    void rotate(int degrees);
    //! Повернуть по часовой стрелке
    void rotateRight(int degrees);
    //! Повернуть против часовой стрелки
    void rotateLeft(int degrees);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    //! Была установлена новая картинка
    void sg_imageLoaded();
    //! Картинка была повернута
    void sg_roteated(int old_degrees, int new_degrees);
    //! Изменился масштаб
    void sg_scaleChanged(double scale);

private:
    //! Размер для данного масштаба
    QSize sizeForScale(double value) const;
    //! Масштаб для подгонки по ширине
    double fitToWidthScale();
    //! Масштаб для подгонки по высоте
    double fitToHeightScale();

    void scaleImage();

    QScrollArea* _scroll_area;
    QLabel* _image_label;

    //! Текущий масштаб
    double _scale_factor = 1.0;
    //! Минимальный масштаб
    double _minimum_scale = 0.05; // 5%
    //! Максимальный масштаб
    double _maximum_scale = 2.0; // 200%

    //! Рамки при подгоне размера
    const int _fit_borders = 5;

    //! Текущий угол поворота
    int _degrees = 0;

    QIcon _icon;
    QPixmap _pixmap;
    //! Оригинальный файл
    QByteArray _file_data;
    //! Тип файла (расширение в верхнем регистре)
    QString _file_type;

    QPoint _drag_pos;

    FittingType _requested_fitting = FittingType::Undefined;
    QTimer _fit_timer;
    QTimer _scale_timer;
};

} // namespace zf
