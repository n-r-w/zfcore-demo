#pragma once

#include "zf_global.h"
#include <QBoxLayout>
#include <QWidget>
#include <QIcon>

namespace zf
{
class CollapsibleGroupBoxHeader;

//! Групбокс с возможностью схлопывания содержимого
class ZCORESHARED_EXPORT CollapsibleGroupBox : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY sg_titleChanged DESIGNABLE true STORED true)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY sg_expandedChanged DESIGNABLE true STORED true)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY sg_iconChanged DESIGNABLE true STORED true)
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize NOTIFY sg_iconChanged DESIGNABLE true STORED true)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor DESIGNABLE true STORED true)
    Q_PROPERTY(QColor titleBackgroundColor READ titleBackgroundColor WRITE setTitleBackgroundColor NOTIFY sg_iconChanged
                   DESIGNABLE true STORED true)
    Q_PROPERTY(bool isBoldTitle READ isBoldTitle WRITE setBoldTitle DESIGNABLE true STORED true)

public:
    explicit CollapsibleGroupBox(QWidget* parent = nullptr);
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);

    QLayout* layout() const;
    void setLayout(QLayout* layout);

    //! Создать layout (если не создан) и установить в него виджет
    void appendWidget(QWidget* widget, Qt::Orientation orientation = Qt::Vertical);
    //! Создать layout (если не создан) и добавить в него виджет
    void insertWidget(int pos, QWidget* widget, Qt::Orientation orientation = Qt::Vertical);

    void setTitle(const QString& title);
    QString title() const;

    void setExpanded(bool expanded);
    bool isExpanded() const;

    //! Рисунок в заголовке
    QIcon icon() const;
    void setIcon(const QIcon& icon);

    //! Размер рисунка в заголовке
    QSize iconSize() const;
    void setIconSize(const QSize& size);

    //! Фоновый цвет
    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& c);

    //! Цвет фона заголовка
    QColor titleBackgroundColor() const;
    void setTitleBackgroundColor(const QColor& c);

    //! Заголовок жирным текстом
    bool isBoldTitle() const;
    void setBoldTitle(bool b);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    //! Внутренний виджет. Метод нужен для работы в Qt Designer - не вызывать
    QWidget* body() const;

public slots:
    void toggle();
    void expand();
    void collapse();

    //! Замена контейнера, метод нужен для работы в Qt Designer - не вызывать
    void replaceBody(QWidget* w);

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void sg_titleChanged();
    void sg_expandedChanged();
    void sg_iconChanged();

private:
    using QWidget::layout;
    using QWidget::setLayout;

    QSize widgetSizeHint() const;
    QSize bodySizeHint() const;

    //! Создать layout если он не создан
    QBoxLayout* createLayout(Qt::Orientation orientation);

    QVBoxLayout* _layout = nullptr;
    QWidget* _body = nullptr;
    CollapsibleGroupBoxHeader* _header = nullptr;

    QString _title;
    bool _expanded = true;
    QIcon _icon;
    QSize _icon_size;
    QColor _background_color;
    QColor _title_background_color;
    bool _title_bold = true;
};
} // namespace zf
