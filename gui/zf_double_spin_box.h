#pragma once

#include "zf_global.h"
#include <QDoubleSpinBox>

namespace zf
{
class ZCORESHARED_EXPORT DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    explicit DoubleSpinBox(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};

} // namespace zf
