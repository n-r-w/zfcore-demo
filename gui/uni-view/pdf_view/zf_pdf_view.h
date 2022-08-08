#pragma once

#include "zf_defs.h"
#include <QPdfView>

namespace zf
{
class PdfView : public QPdfView
{
    Q_OBJECT
public:
    explicit PdfView(QWidget* parent = nullptr);
    ~PdfView();

    //! Увеличить/уменьшить пропорционально (1.1 = +10%)
    void scale(qreal factor);
    //! Задать режим масштаба
    void setZoomMode(ZoomMode mode);
    //! Текущий масштаб
    qreal zoomFactor() const;
    void setZoomFactor(qreal factor);
};

} // namespace zf
