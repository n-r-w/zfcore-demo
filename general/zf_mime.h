#pragma once

#include "zf_core.h"

namespace zf
{
class MimeEntityList;
class MimeDataContainer;
}

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::MimeEntityList& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::MimeEntityList& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::MimeDataContainer& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::MimeDataContainer& c);

namespace zf
{
//! Список ключевых значений сущностей для Drag&Drop
class ZCORESHARED_EXPORT MimeEntityList
{    
public:
    //! Тип MIME
    static const QString MIME_TYPE;

    MimeEntityList();
    MimeEntityList(const MimeEntityList& m);
    //! Список однотипных сущностей
    MimeEntityList(
        //! Код сущности
        const EntityCode& entity_code,
        //! Список ключевых значений
        const EntityIDList& ids,
        //! База данных
        const DatabaseID& database_id = Core::defaultDatabase());
    //! Список сущностей
    MimeEntityList(const UidList& entities);

    MimeEntityList& operator=(const MimeEntityList& m);

    //! Был ли инициализирован
    bool isValid() const;
    //! Не содержит данных
    bool isEmpty() const;
    //! содержит однотипные сущности
    bool isSingleEntityCode() const;

    //! Список сущностей
    UidList entities() const;

    //! Код сущности. Только если объект содержит однотипные сущности
    EntityCode entityCode() const;
    //! Список ID сущностей. Только если объект содержит однотипные сущности
    EntityIDList ids() const;
    //! Идентификатор БД. Только если объект содержит однотипные сущности
    DatabaseID databaseId() const;

    //! Инициализировать mime для Drag&Drop
    void applyData(QMimeData* data) const;
    //! Восстановить из Drag&Drop
    static MimeEntityList fromData(const QMimeData* data);

    //! Преобразовать в QByteArray
    QByteArray toByteArray() const;
    //! Восстановить из QByteArray
    static MimeEntityList fromByteArray(const QByteArray& b);

private:
    void copyFrom(const MimeEntityList& m);
    void fill_entities() const;
    void fill_ids() const;

    bool _is_valid = false;
    //! Сущности
    mutable std::unique_ptr<UidList> _entities;
    //! Была проверка на однотипные сущности
    mutable bool _single_checked = false;
    //! Код сущности
    mutable EntityCode _entity_code;
    //! БД
    mutable DatabaseID _database_id;
    //! Идентификаторы
    mutable std::unique_ptr<EntityIDList> _ids;

    friend QDataStream& operator<<(QDataStream& s, const zf::MimeEntityList& c);
    friend QDataStream& operator>>(QDataStream& s, zf::MimeEntityList& c);
};

//! Передача DataContainer для Drag&Drop
class ZCORESHARED_EXPORT MimeDataContainer
{
public:
    //! Тип MIME
    static const QString MIME_TYPE;

    MimeDataContainer();
    MimeDataContainer(const MimeDataContainer& m);
    MimeDataContainer(const DataContainer& c);
    ~MimeDataContainer();

    MimeDataContainer& operator=(const MimeDataContainer& m);

    //! Был ли инициализирован
    bool isValid() const;
    //! Данные
    DataContainer data() const;

    //! Инициализировать mime для Drag&Drop
    void applyData(QMimeData* data) const;
    //! Восстановить из Drag&Drop
    static MimeDataContainer fromData(const QMimeData* data);

    //! Преобразовать в QByteArray
    QByteArray toByteArray() const;
    //! Восстановить из QByteArray
    static MimeDataContainer fromByteArray(const QByteArray& b);

private:
    void copyFrom(const MimeDataContainer& m);

    DataContainer* _data = nullptr;
};

} // namespace zf
