#ifndef ZF_CORE_UIDS_H
#define ZF_CORE_UIDS_H

#include "zf_uid.h"

namespace zf
{
//! Зарезервированные идентификаторы
class ZCORESHARED_EXPORT CoreUids
{
public:
    //! Является ли идентификатор зарезервированным
    static bool isCoreUid(const Uid& uid);
    static bool isCoreUid(const EntityCode& uid_code);
    //! Название зарезервированного идентификатора
    static QString coreUidName(const Uid& uid);

    //! Код идентификатора ядра
    static const EntityCode UID_CODE_CORE;
    //! Идентификатор ядра для отправки синхронных команд
    static const Uid CORE;

    //! Код идентификатора менеджера БД
    static const EntityCode UID_CODE_DATABASE_MANAGER;
    //! Идентификатор менеджера БД для отправки ему команд через MessageDispatcher
    static const Uid DATABASE_MANAGER;

    //! Код идентификатора менеджера моделей
    static const EntityCode UID_CODE_MODEL_MANAGER;
    //! Идентификатор менеджера моделей для отправки ему команд через MessageDispatcher
    static const Uid MODEL_MANAGER;

    //! Код идентификатора диспетчера сообщений
    static const EntityCode UID_CODE_MESSAGE_DISPATCHER;
    //! Идентификатор диспетчера сообщений для отправки ему команд через MessageDispatcher
    static const Uid MESSAGE_DISPATCHER;

    //! Код идентификатора ModelKeeper
    static const EntityCode UID_CODE_MODEL_KEEPER;
    //! Идентификатор ModelKeeper для отправки ему команд через MessageDispatcher
    static const Uid MODEL_KEEPER;

    //! Код идентификатора оболочки
    static const EntityCode UID_CODE_SHELL;
    //! Идентификатор оболочки для отправки ему команд через MessageDispatcher
    static const Uid SHELL;

    //! Код идентификатора отладочного окна оболочки
    static const EntityCode UID_CODE_SHELL_DEBUG;
    //! Идентификатор отладочного окна для отправки ему команд через MessageDispatcher
    static const Uid SHELL_DEBUG;

    //! Код ItemSelector
    static const EntityCode UID_CODE_ITEM_SELECTOR;
    //! Идентификатор ItemSelector для отправки ему команд через MessageDispatcher
    static const Uid ITEM_SELECTOR;

    //! Код ReportTemplateStorage
    static const EntityCode UID_CODE_REPORT_TEMPLATE_STORAGE;
    //! Идентификатор ReportTemplateStorage для отправки ему команд через MessageDispatcher
    static const Uid REPORT_TEMPLATE_STORAGE;

    //! Код RequestSelector
    static const EntityCode UID_CODE_REQUEST_SELECTOR;
    //! Идентификатор RequestSelector для отправки ему команд через MessageDispatcher
    static const Uid REQUEST_SELECTOR;

    //! Код ExternalRequester
    static const EntityCode UID_CODE_EXTERNAL_REQUESTER;
    //! Идентификатор ExternalRequester для отправки ему команд через MessageDispatcher
    static const Uid EXTERNAL_REQUESTER;

    //! Код EntityNameWidget
    static const EntityCode UID_CODE_ENTITY_NAME_WIDGET;
    //! Идентификатор EntityNameWidget для отправки ему команд через MessageDispatcher
    static const Uid ENTITY_NAME_WIDGET;

    //! Код драйвера БД
    static const EntityCode UID_CODE_DRIVER;
    //! Идентификатор драйвера БД для отправки ему команд через MessageDispatcher
    static const Uid DRIVER;

    //! Код воркера драйвера БД
    static const EntityCode UID_CODE_DRIVER_WORKER;
    //! Идентификатор воркера драйвера БД для отправки ему команд через MessageDispatcher
    static const Uid DRIVER_WORKER;

    //! Код сервиса моделей
    static const EntityCode UID_CODE_MODEL_SERVICE;
    //! Идентификатор сервиса моделей для отправки ему команд через MessageDispatcher
    static const Uid MODEL_SERVICE;
};

} // namespace zf

#endif // ZF_CORE_UIDS_H
