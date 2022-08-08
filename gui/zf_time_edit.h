#pragma once

#include "zf_global.h"
#include <QTimeEdit>

namespace zf
{
class ZCORESHARED_EXPORT TimeEdit : public QTimeEdit
{
    Q_OBJECT

public:
    explicit TimeEdit(QWidget* parent = nullptr);
    ~TimeEdit();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    int minimumWidth(int height) const;

    //! Режим редактирования
    bool isReadOnlyHelper() const;

    QLineEdit* _internal_editor = nullptr;
};

} // namespace zf
