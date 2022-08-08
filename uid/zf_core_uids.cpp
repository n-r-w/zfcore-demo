#include "zf_core_uids.h"
#include "zf_core.h"

#include <climits>

namespace zf
{
//! Код идентификатора ядра
const EntityCode CoreUids::UID_CODE_CORE = EntityCode(1);
//! Идентификатор ядра для отправки синхронных команд
const Uid CoreUids::CORE = Uid::coreEntity(UID_CODE_CORE);

//! Код идентификатора менеджера БД
const EntityCode CoreUids::UID_CODE_DATABASE_MANAGER = EntityCode(2);
//! Идентификатор менеджера БД для отправки ему команд через MessageDispatcher
const Uid CoreUids::DATABASE_MANAGER = Uid::coreEntity(UID_CODE_DATABASE_MANAGER);

//! Код идентификатора менеджера моделей
const EntityCode CoreUids::UID_CODE_MODEL_MANAGER = EntityCode(3);
//! Идентификатор менеджера моделей для отправки ему команд через MessageDispatcher
const Uid CoreUids::MODEL_MANAGER = Uid::coreEntity(UID_CODE_MODEL_MANAGER);

//! Код идентификатора диспетчера сообщений
const EntityCode CoreUids::UID_CODE_MESSAGE_DISPATCHER = EntityCode(4);
//! Идентификатор диспетчера сообщений для отправки ему команд через MessageDispatcher
const Uid CoreUids::MESSAGE_DISPATCHER = Uid::coreEntity(UID_CODE_MESSAGE_DISPATCHER);

//! Код идентификатора ModelKeeper
const EntityCode CoreUids::UID_CODE_MODEL_KEEPER = EntityCode(5);
//! Идентификатор ModelKeeper для отправки ему команд через MessageDispatcher
const Uid CoreUids::MODEL_KEEPER = Uid::coreEntity(UID_CODE_MODEL_KEEPER);

//! Код идентификатора оболочки
const EntityCode CoreUids::UID_CODE_SHELL = EntityCode(6);
//! Идентификатор оболочки для отправки ему команд через MessageDispatcher
const Uid CoreUids::SHELL = Uid::coreEntity(UID_CODE_SHELL);

//! Код идентификатора отладочного окна оболочки
const EntityCode CoreUids::UID_CODE_SHELL_DEBUG = EntityCode(7);
//! Идентификатор отладочного окна для отправки ему команд через MessageDispatcher
const Uid CoreUids::SHELL_DEBUG = Uid::coreEntity(UID_CODE_SHELL_DEBUG);

//! Код ItemSelector
const EntityCode CoreUids::UID_CODE_ITEM_SELECTOR = EntityCode(8);
//! Идентификатор ItemSelector для отправки ему команд через MessageDispatcher
const Uid CoreUids::ITEM_SELECTOR = Uid::coreEntity(UID_CODE_ITEM_SELECTOR);

//! Код ReportTemplateStorage
const EntityCode CoreUids::UID_CODE_REPORT_TEMPLATE_STORAGE = EntityCode(9);
//! Идентификатор ReportTemplateStorage для отправки ему команд через MessageDispatcher
const Uid CoreUids::REPORT_TEMPLATE_STORAGE = Uid::coreEntity(UID_CODE_REPORT_TEMPLATE_STORAGE);

//! Код RequestSelector
const EntityCode CoreUids::UID_CODE_REQUEST_SELECTOR = EntityCode(10);
//! Идентификатор RequestSelector для отправки ему команд через MessageDispatcher
const Uid CoreUids::REQUEST_SELECTOR = Uid::coreEntity(UID_CODE_REQUEST_SELECTOR);

//! Код ExternalRequester
const EntityCode CoreUids::UID_CODE_EXTERNAL_REQUESTER = EntityCode(11);
//! Идентификатор ExternalRequester для отправки ему команд через MessageDispatcher
const Uid CoreUids::EXTERNAL_REQUESTER = Uid::coreEntity(UID_CODE_EXTERNAL_REQUESTER);

//! Код EntityNameWidget
const EntityCode CoreUids::UID_CODE_ENTITY_NAME_WIDGET = EntityCode(12);
//! Идентификатор EntityNameWidget для отправки ему команд через MessageDispatcher
const Uid CoreUids::ENTITY_NAME_WIDGET = Uid::coreEntity(UID_CODE_ENTITY_NAME_WIDGET);

//! Код драйвера БД
const EntityCode CoreUids::UID_CODE_DRIVER = EntityCode(13);
//! Идентификатор драйвера БД для отправки ему команд через MessageDispatcher
const Uid CoreUids::DRIVER = Uid::coreEntity(UID_CODE_DRIVER);

//! Код воркера драйвера БД
const EntityCode CoreUids::UID_CODE_DRIVER_WORKER = EntityCode(14);
//! Идентификатор воркера драйвера БД для отправки ему команд через MessageDispatcher
const Uid CoreUids::DRIVER_WORKER = Uid::coreEntity(UID_CODE_DRIVER_WORKER);

//! Код сервиса моделей
const EntityCode CoreUids::UID_CODE_MODEL_SERVICE = EntityCode(15);
//! Идентификатор сервиса моделей для отправки ему команд через MessageDispatcher
const Uid CoreUids::MODEL_SERVICE = Uid::coreEntity(UID_CODE_MODEL_SERVICE);

bool CoreUids::isCoreUid(const Uid& uid)
{
    Z_CHECK(uid.isValid());
    return uid.isCore();
}

bool CoreUids::isCoreUid(const EntityCode& uid_code)
{
    Z_CHECK(uid_code.isValid());
    return (uid_code == UID_CODE_CORE || uid_code == UID_CODE_DATABASE_MANAGER || uid_code == UID_CODE_MODEL_MANAGER
            || uid_code == UID_CODE_MESSAGE_DISPATCHER || uid_code == UID_CODE_MODEL_KEEPER || uid_code == UID_CODE_SHELL
            || uid_code == UID_CODE_SHELL_DEBUG || uid_code == UID_CODE_ITEM_SELECTOR || uid_code == UID_CODE_REPORT_TEMPLATE_STORAGE
            || uid_code == UID_CODE_REQUEST_SELECTOR || uid_code == UID_CODE_EXTERNAL_REQUESTER || uid_code == UID_CODE_ENTITY_NAME_WIDGET
            || uid_code == UID_CODE_DRIVER || uid_code == UID_CODE_DRIVER_WORKER || uid_code == UID_CODE_MODEL_SERVICE);
}

QString CoreUids::coreUidName(const Uid& uid)
{
    Z_CHECK(uid.isValid());
    if (uid == CORE)
        return "CORE";
    if (uid == DATABASE_MANAGER)
        return "DATABASE_MANAGER";
    if (uid == MODEL_MANAGER)
        return "MODEL_MANAGER";
    if (uid == MESSAGE_DISPATCHER)
        return "MESSAGE_DISPATCHER";
    if (uid == MODEL_KEEPER)
        return "MODEL_KEEPER";
    if (uid == SHELL)
        return "SHELL";
    if (uid == SHELL_DEBUG)
        return "SHELL_DEBUG";
    if (uid == ITEM_SELECTOR)
        return "ITEM_SELECTOR";
    if (uid == REPORT_TEMPLATE_STORAGE)
        return "REPORT_TEMPLATE_STORAGE";
    if (uid == REQUEST_SELECTOR)
        return "REQUEST_SELECTOR";
    if (uid == EXTERNAL_REQUESTER)
        return "EXTERNAL_REQUESTER";
    if (uid == ENTITY_NAME_WIDGET)
        return "ENTITY_NAME_WIDGET";
    if (uid == DRIVER)
        return "DRIVER";
    if (uid == DRIVER_WORKER)
        return "DRIVER_WORKER";
    if (uid == MODEL_SERVICE)
        return "MODEL_SERVICE";

    Z_HALT_INT;
    return QString();
}

} // namespace zf
