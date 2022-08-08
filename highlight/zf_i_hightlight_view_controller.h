#pragma once

#include "zf_data_structure.h"

namespace zf
{
class HighlightProcessor;
class HighlightMapping;

class I_HightlightViewController
{
public:
    //! Виджеты
    virtual DataWidgets* hightlightControllerGetWidgets() const = 0;
    //! Реализация обработки реакции на изменение данных и заполнения модели ошибок
    virtual HighlightProcessor* hightlightControllerGetProcessor() const = 0;

    //! Связь между виджетами и свойствами для обработки ошибок
    virtual HighlightMapping* hightlightControllerGetMapping() const = 0;

    //! Текущий фокус
    virtual DataProperty hightlightControllerGetCurrentFocus() const = 0;
    //! Установить фокус ввода. Возвращает false, если фокус не был установлен
    virtual bool hightlightControllerSetFocus(const DataProperty& p) = 0;

    //! Вызывается перед фокусировкой свойства с ошибкой
    virtual void hightlightControllerBeforeFocusHighlight(const DataProperty& property) = 0;
    //! Вызывается после фокусировки свойства с ошибкой
    virtual void hightlightControllerAfterFocusHighlight(const DataProperty& property) = 0;
};

} // namespace zf
