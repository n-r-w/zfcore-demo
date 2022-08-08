#pragma once

#include "zf.h"
#include "zf_i_web_engine_injection.h"

namespace zf
{
//! Базовый класс для перехвата событий на веб странице
class ZCORESHARED_EXPORT WebEngineViewInjection : public QObject, public I_WebEngineViewInjection
{
    Q_OBJECT
public:
    WebEngineViewInjection();

    //! Нажата кнопка
    void onWebKeyPressed(const QString& target_id, const QString& key) override;
    //! Нажата кнопка
    void onWebMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button) override;
    //! Изменилась текущая станица (только для pdf)
    void onCurrentPageChanged(int current, int maximum) override;

    //! Находимся ли в режиме просмотра pdf
    bool isPdfView() const final;
    //! Скрывать ли верхний тулбар при просмотре pdf
    bool isHidePdfToolbar() const final;
    //! Скрывать ли панель масштаба при просмотре pdf
    bool isHidePdfZoom() const final;

    //! Находимся ли в режиме просмотра pdf
    void setPdfView(bool b);
    //! Скрывать ли верхний тулбар при просмотре pdf
    void setHidePdfToolbar(bool b);
    //! Скрывать ли панель масштаба при просмотре pdf
    void setHidePdfZoom(bool b);

signals:
    //! Изменились параметры
    void sg_optionsChanged();
    //! Нажата кнопка
    void sg_webKeyPressed(const QString& target_id, const QString& key);
    //! Нажата кнопка
    void sg_webMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button);
    //! Изменилась текущая станица (только для pdf)
    void sg_currentPageChanged(int current, int maximum);

private:
    //! Нажата кнопка
    void onWebKeyPressed_helper(const QString& target_id, const QString& key);
    //! Нажата кнопка
    void onWebMouseClicked_helper(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button);
    //! Изменилась текущая станица (только для pdf)
    void onCurrentPageChanged_helper(int current, int maximum);

    bool _is_pdf_view = false;
    bool _is_hide_pdf_toolbar = false;
    bool _is_hide_pdf_zoom = false;

    friend class WebEngineViewInjectionObject;
};

} // namespace zf
