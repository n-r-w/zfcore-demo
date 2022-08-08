#pragma once

#include <QObject>
#include "zf_uid.h"

namespace zf
{
class Error;
class Model;
class DataFilter;
class Rows;
class DataStructure;
typedef std::shared_ptr<Model> ModelPtr;

//! Интерфейс для универального доступа к каталогам
class I_Catalogs
{
public:
    //! Идентификатор каталога по его коду сущности
    virtual Uid catalogUid(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const DatabaseID& database_id) const = 0;
    //! Модель каталога по его коду сущности
    virtual ModelPtr catalogModel(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const DatabaseID& database_id,
        //! Ошибка
        Error& error) const = 0;
    //! Инициализация фильтра для каталога
    virtual void initCatalogFilter(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const DatabaseID& database_id,
        //! Фильтр
        DataFilter* filter) const = 0;
    //! Проверка на идентификатор каталога
    virtual bool isCatalogUid(const Uid& entity_uid) const = 0;

    //! Поиск в каталоге по ID строки. Если каталог не загружен или в процессе загрузки, то пустое значение
    virtual zf::Rows findById(
        //! Код сущности каталога
        const zf::EntityCode& catalog_id,
        //! Код строки каталога
        const zf::EntityID& id,
        //! База данных
        const DatabaseID& database_id) const = 0;
    //! Идентификатор основной сущности каталога. Используется если каталог является упрощенным видом для полноценной списочной сущности
    virtual Uid mainEntityUid(
        //! Идентификатор каталога
        const zf::EntityCode& catalog_id,
        //! База данных
        const DatabaseID& database_id)
        = 0;

    //! Идентификатор каталога, который содержит список сущностей данного типа
    virtual Uid catalogEntityUid(
        //! Код сущности
        const EntityCode& entity_code,
        //! База данных
        const DatabaseID& database_id)
        = 0;

    //! Извлечь ID каталога из идентификатора
    virtual zf::EntityCode catalogId(const Uid& catalog_uid) = 0;
    //! Получить расшифровку кода элемента каталога. Если каталог не загружен или в процессе загрузки, то пустое значение
    virtual QString catalogItemName(const Uid& catalog_uid, const EntityID& item_id, QLocale::Language language) = 0;

    //! Свойство каталога, которое отвечает за имя
    virtual PropertyID namePropertyId(
        //! Идентификатор каталога
        const Uid& catalog_uid)
        = 0;

    //! Может ли справочник обновляться на стороне БД (меняются ли в нем данные)
    virtual bool isCatalogUpdatable(const zf::Uid& catalog_uid) = 0;
    //! Является ли справочник фейковым (не существует на сервере)
    virtual bool isCatalogFake(const zf::Uid& catalog_uid) = 0;
};

} // namespace zf

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(zf::I_Catalogs, "ru.ancor.metastaff.I_Catalogs/1.0")
QT_END_NAMESPACE
