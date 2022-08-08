#pragma once

#include <QWidget>
#include <QPicture>
#include <QIcon>
#include "zf_error.h"

class QLabel;
class QFrame;
class QToolButton;

namespace zf
{
//! Выбор и отображение картинки. Для управления размером на форме, выставить minimumSize
class ZCORESHARED_EXPORT PictureSelector : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)
    Q_PROPERTY(int margin READ margin WRITE setMargin DESIGNABLE true STORED true)
    Q_PROPERTY(QSize imageSize READ imageSize WRITE setImageSize DESIGNABLE true STORED true)

public:
    explicit PictureSelector(QWidget* parent = nullptr);
    ~PictureSelector() override;

    //! Загрузить из файла
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

    //! Задать QIcon fileData и fileType сбрасываются
    void setIcon(const QIcon& icon);
    //! Картинка
    QIcon icon() const;

    //! Оригинальный файл (при загрузке через loadFile или setFileData)
    QByteArray fileData() const;
    //! Тип файла (расширение в верхнем регистре) (при загрузке через loadFile или setFileData)
    QString fileType() const;

    //! Указать что отображать когда ничего не выбрано
    void setIconNoImage(const QIcon& icon);
    //! Что отображать когда ничего не выбрано
    QIcon iconNoImage() const;

    void setImageSize(const QSize& size);
    QSize imageSize() const;

    //! Очистить картинку
    void clear();

    //! Установлен ли рисунок
    bool isEmpty() const;
    //! Можно ли изменять
    bool isReadOnly() const;
    //! Можно ли изменять
    void setReadOnly(bool b);

    //! Отступы
    int margin() const;
    //! Задать отступы
    void setMargin(int m);

    //! Выбрать из файла
    Q_SLOT void selectPicture();

    //! Кнопка выбора
    QToolButton* openButton() const;
    //! Кнопка очистки
    QToolButton* clearButton() const;
    //! Кнопка вставки
    QToolButton* pasteButton() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

signals:
    //! Изменено пользователем
    void sg_edited();
    //! Изменено пользователем или программно
    void sg_changed();

private slots:
    //! Вставка картинки
    void sl_paste();

private:
    void setIconToPixmap(const QIcon& icon);
    void updateDrawIcon();
    void setIconHelper(const QIcon& icon, bool changed);
    Q_SLOT void updateInternalGeometry();
    Q_SLOT void updateEnabled();
    //! Принимать ли файл из mime. Возвращает поддерживаемый файл или пустую строку
    static QString allowMimeFile(const QMimeData* mime_data);

    QSize _size = {128, 128};

    //! Отступ
    int _margin = 2;
    //! Ширина рамки
    int _border_width = 1;

    //! Надо обновить _draw_pixmap
    bool _need_update_draw = false;
    //! QPixmap для отрисовки
    QPixmap _draw_pixmap;
    //! QIcon для отрисовки
    QIcon _draw_icon;
    //! Иконка
    QIcon _icon;
    //! Что отображать когда ничего не выбрано
    QIcon _icon_no_image;
    //! Внешняя рамка
    QFrame* _frame = nullptr;
    //! Виджет с кнопками управления
    QWidget* _manage = nullptr;
    //! Режим "только для чтения"
    bool _is_read_only = false;

    //! Оригинальный файл
    QByteArray _file_data;
    //! Тип файла (расширение в верхнем регистре)
    QString _file_type;
    //! Кнопка выбора
    QToolButton* _open_button = nullptr;
    //! Кнопка очистки
    QToolButton* _clear_button = nullptr;
    //! Кнопка вставки
    QToolButton* _paste_button = nullptr;

    friend class PictureSelectorAccessibilityInterface;
};
} // namespace zf
