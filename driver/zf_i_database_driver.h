#ifndef ZF_I_DRIVER_H
#define ZF_I_DRIVER_H

#include "zf_data_structure.h"

namespace zf
{
class DatabaseDriverWorker;
class Error;
class Uid;
class DatabaseDriverConfig;

//! Интерфейс драйвера БД
class I_DatabaseDriver
{
public:
    virtual ~I_DatabaseDriver() {}

    //! Начальная инициализация. Вызывается в методе Core::registerDatabaseDriver
    virtual Error bootstrap(const DatabaseDriverConfig& config) = 0;
    //! Завершение работы
    virtual void shutdown() = 0;
    //! Остановлен ли драйвер
    virtual bool isShutdowned() const = 0;

    //! Параметры драйвера
    virtual const DatabaseDriverConfig* config() const = 0;

    //! Создать обработчик. За удаление отвечает ядро
    virtual DatabaseDriverWorker* createWorker(Error& error) = 0;

    //! Получить БД, в которой находятся объекты указанного типа
    virtual DatabaseID entityCodeDatabase(
        //! Владелец объекта (идентификатор клиента и т.п.)
        const Uid& owner_uid,
        //! Код сущности
        const EntityCode& entity_code)
        = 0;
};

} // namespace zf

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(zf::I_DatabaseDriver, "ru.ancor.metastaff.I_DatabaseDriver/1.0")
QT_END_NAMESPACE

#endif // ZF_I_DRIVER_H
