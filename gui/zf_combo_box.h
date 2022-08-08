#pragma once

#include "zf_global.h"
#include <QComboBox>

namespace zf
{
class ZCORESHARED_EXPORT ComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true STORED true)

public:
    explicit ComboBox(QWidget* parent = nullptr);

    bool isReadOnly() const;
    void setReadOnly(bool b);

    void showPopup() override;
    void hidePopup() override;

    //! Задать текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    void setOverrideText(const QString& text);

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

signals:
    void sg_popupOpened();
    void sg_popupClosed();

private:
    //! Текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    QString _override_text;
};

} // namespace zf
