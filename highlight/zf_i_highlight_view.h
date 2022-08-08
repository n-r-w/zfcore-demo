#pragma once

#include "zf_data_structure.h"

namespace zf
{
//! Интерфейс для HighlightView
class I_HighlightView
{
public:    
    //! Определение порядка сортировки свойств при отображении HighlightView
    //! Вернуть истину если property1 < property2
    virtual bool highlightViewGetSortOrder(const DataProperty& property1, const DataProperty& property2) const = 0;
    //! А надо ли вообще проверять на ошибки
    virtual bool isHighlightViewIsCheckRequired() const = 0;
    //! Инициализирован ли порядок отображения ошибок
    virtual bool isHighlightViewIsSortOrderInitialized() const = 0;

    //! Сигнал об изменении порядка отображения
    virtual void sg_highlightViewSortOrderChanged() = 0;
};

} // namespace zf
