#pragma once

#include "zf.h"
#include <QScrollArea>
#include <QBoxLayout>
#include <QSpacerItem>

namespace zf
{
/*! Скрол с прокруткой элементов по одному направлению
 * Центральный виджет инициализируется в конструкторе и ему задается layout в зависимости от ориентации
 * Создается QSpacerItem, который всегда автоматически помещается в конец
 * Обычный QScrollArea не подходит, т.к. он дает слишком большой минимальный размер */
class ZCORESHARED_EXPORT LineScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit LineScrollArea(QWidget* parent = nullptr, Qt::Orientation orientation = Qt::Horizontal, int line_size = 0);

    //! Ориентация
    Qt::Orientation orientation() const;
    //! Лайаут центрального виджета
    QBoxLayout* widgetLayout() const;

    //! Размер
    int lineSize() const;
    void setLineSize(int size);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    //! Минимальный размер по противополжному направлению
    const int MINIMUM_OPPOZITE = 10;
    //! Минимальный размер
    int _line_size = 0;
    //! Ориентация
    Qt::Orientation _orientation;
    //! Центральный виджет
    QWidget* _body;
    //! Разделитель в конце
    QSpacerItem* _spacer;
    //! Таймер для перемещения разделителя
    QTimer* _spacer_timer;
};

} // namespace zf
