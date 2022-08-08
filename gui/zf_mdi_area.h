#pragma once

#include <QMdiArea>
#include <QPointer>
#include "zf_global.h"

namespace zf
{
//! MdiArea с возможностью устновки фонового виджета
class ZCORESHARED_EXPORT MdiArea : public QMdiArea
{
    Q_OBJECT
public:
    MdiArea(QWidget* parent = nullptr);
    ~MdiArea() override;

    //! Установить новый фоновый виджет. Возвращает старый если был. За удаление отвечает вызывающий
    QWidget* setBackgroundWidget(QWidget* w);
    //! Фоновый виджет
    QWidget* backgroundWidget() const;

protected:
    bool viewportEvent(QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void updateBackgroundPosition();

    QPointer<QWidget> _background_widget;
};
} // namespace zf
