#pragma once

#include "zf_global.h"
#include <QSpinBox>

namespace zf
{
class ZCORESHARED_EXPORT SpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit SpinBox(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
};

} // namespace zf
