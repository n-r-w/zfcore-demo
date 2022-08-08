#pragma once

#include <QObject>
#include "zf_global.h"

class QTimer;

namespace zf
{
//! Служебный класс для управления освобождением ресурсов
class ZCORESHARED_EXPORT ResourceManager : public QObject
{
    Q_OBJECT
public:
    ResourceManager(
        //! период освобождения ресурсов в секундах
        int free_resource_period = 60);

    //! Были использованы временные ресурсы, которые имеет смысл освободить через определенный период
    //! Сбрасывает таймер особождения ресурсов в 0 (если он запущен)
    void resourceUsed() const;
    //! Были добавлены временные ресурсы, которые имеет смысл освободить через определенный период
    //! Запускает таймер освобождения ресурсов
    void resourceAdded() const;

    //! Задать период освобождения ресурсов в секундах. Если n <= 0 - ресурсы освобождаться не будут
    void setFreeResourcePeriod(int n);
    int freeResourcePeriod() const;

signals:
    //! Вызывается по таймеру особождения ресурсов
    void sg_freeResources();

private slots:
    //! Сработал таймер освобождения ресурсов
    void sl_freeResourceTimer();

private:
    //! Тамер для управления освобождением ресурсов
    mutable QTimer* _free_resources_timer = nullptr;
    //! период освобождения ресурсов в секундах
    int _free_resource_period;
};

} // namespace zf
