#pragma once

#include <QCheckBox>
#include "zf_global.h"

namespace zf
{
class ZCORESHARED_EXPORT CheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate DESIGNABLE true STORED true)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState DESIGNABLE true STORED true)

public:
    explicit CheckBox(QWidget* parent = nullptr);
    explicit CheckBox(const QString& text, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* e) override;
};
} // namespace zf
