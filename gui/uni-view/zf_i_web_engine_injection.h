#pragma once

#include <QtCore>

namespace zf
{
//! Интерфейс для перехвата событий на веб странице
class I_WebEngineViewInjection
{
public:
    virtual ~I_WebEngineViewInjection() { }

    //! Нажата кнопка
    virtual void onWebKeyPressed(const QString& target_id, const QString& key) = 0;
    //! Нажата кнопка
    virtual void onWebMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button) = 0;
    //! Изменилась текущая станица (только для pdf)
    virtual void onCurrentPageChanged(int current, int maximum) = 0;

    //! Находимся ли в режиме просмотра pdf
    virtual bool isPdfView() const = 0;
    //! Скрывать ли верхний тулбар при просмотре pdf
    virtual bool isHidePdfToolbar() const = 0;
    //! Скрывать ли панель масштаба при просмотре pdf
    virtual bool isHidePdfZoom() const = 0;
};

} // namespace zf
