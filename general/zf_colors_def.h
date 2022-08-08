#pragma once

#include "zf.h"

namespace zf
{
//! Настройка цветов
class ZCORESHARED_EXPORT Colors
{
public:
    static void bootstrap();

    //! Цвет фона "только для чтения"
    static QColor readOnlyBackground();
    //! Цвет обычного фона виджетов
    static QColor background();

    //! Цвет линий
    static QColor uiLineColor(bool bold);
    //! Цвет фона окон
    static QColor uiWindowColor();
    //! Цвет фона кнопок
    static QColor uiButtonColor();
    //! Темный цвет
    static QColor uiDarkColor();
    //! Альтернативный фон ячеек в таблицах (например при чередовании строк)
    static QColor uiAlternateTableBackgroundColor();
    //! Цвет выделения фона в таблицах
    static QColor uiInfoTableBackgroundColor();
    //! Цвет выделения текста в таблицах
    static QColor uiInfoTableTextColor();
};

} // namespace zf
