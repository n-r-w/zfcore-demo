#pragma once

#include "zf.h"

class QToolButton;

namespace zf
{
// Настройка размеров виджетов
class ZCORESHARED_EXPORT UiSizes
{
public:
    static void bootstrap();

    //! Значение по умолчанию для высоты строк таблиц
    static int defaultTableRowHeight();
    //! Размер по умолчанию для однострочного редактора типа QLineEdit
    static QSize defaultLineEditSize(const QFont f);

    //! Размер кнопки QToolButton для внутренних экшенов виджетов (очистка и т.п.)
    static int defaultEditorToolButtonSize(const QFont f);
    //! Размер иконки для внутренних экшенов виджетов (очистка и т.п.)
    static int defaultEditorIconSize(const QFont f);
    //! Размер иконки для тулбаров
    static int defaultToolbarIconSize(ToolbarType type);
    //! Задать размеры тулбатона для внутренних экшенов виджетов (очистка и т.п.)
    static void prepareEditorToolButton(const QFont f, QToolButton* b,
                                        //! Задать ограничения по размеру. Иначе задать геометрию
                                        bool set_maximum_size,
                                        //! На сколько увеличить или уменьшить иконку от стандарта
                                        int delta = 0);
};

} // namespace zf
