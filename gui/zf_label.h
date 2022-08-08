#pragma once

#include <QLabel>
#include "zf.h"

namespace zf
{
//! QLabel в возможности вертикальной отрисовки
class ZCORESHARED_EXPORT Label : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation DESIGNABLE true STORED true)

public:
    explicit Label(QWidget* parent = nullptr);
    explicit Label(const QString& text, QWidget* parent = nullptr);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation o);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    Qt::Orientation _orientation = Qt::Horizontal;
};

} // namespace zf
