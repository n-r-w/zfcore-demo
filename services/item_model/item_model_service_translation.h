#pragma once

#include <QtCore>

namespace ModelService
{
//! Константы для перевода. Сканируются с помощью lupdate
struct TR
{
    //: MODEL_SERVICE_MODULE_NAME
    //% "Сервис запросов к моделям"
    inline static const char* MODULE_NAME = QT_TRID_NOOP("MODEL_SERVICE_MODULE_NAME");
};
} // namespace fias
