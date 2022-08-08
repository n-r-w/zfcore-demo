#pragma once

#include "zf.h"

namespace zf {

//! Интерфейс управления MDI окном. Встраивается в представление
class I_ModuleWindow {
public:
    //! Параметры
    virtual ModuleWindowOptions options() const = 0;
};

} // namespace zf
