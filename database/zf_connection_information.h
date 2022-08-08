#pragma once

#include "zf_uid.h"
#include "zf_error.h"
#include "zf_access_rights.h"
#include <QLocale>
#include <QHostInfo>

namespace zf
{
class ProgramFeature;
class ProgramFeatures;
class Permissions;
class UserInformation;
class ConnectionInformation;
class DatabaseInformation;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::ProgramFeature& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::ProgramFeature& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::ProgramFeatures& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::ProgramFeatures& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::Permissions& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::Permissions& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::UserInformation& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::UserInformation& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::ConnectionInformation& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::ConnectionInformation& c);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::DatabaseInformation& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::DatabaseInformation& c);

namespace zf
{
//! Возможность (глобальный параметр) программы
class ZCORESHARED_EXPORT ProgramFeature
{
public:
    ProgramFeature();
    ProgramFeature(int code, const QString& translation_id);
    ProgramFeature(const ProgramFeature& f);

    bool isValid() const;

    //! Уникальный код возможности
    int code() const;
    //! Наименование
    QString name() const;

    ProgramFeature& operator=(const ProgramFeature& f);
    bool operator==(const ProgramFeature& f) const;
    bool operator!=(const ProgramFeature& f) const;
    bool operator<(const ProgramFeature& f) const;

private:
    int _code = -1;
    QString _translation_id;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::ProgramFeature& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::ProgramFeature& c);
};

/*! Возможности (глобальные параметры) программы
 * ВАЖНО! Возможности для модулей и не зависящие от модулей никак не связаны друг с другом */
class ZCORESHARED_EXPORT ProgramFeatures
{
public:
    ProgramFeatures();
    ProgramFeatures(const ProgramFeatures& f);
    ProgramFeatures(
        //! Общие настройки
        const QList<ProgramFeature>& features,
        //! Настройки по модулям
        const QHash<EntityCode, ProgramFeatures>& module_features);

    bool operator==(const ProgramFeatures& f) const;

    bool isEmpty() const;

    //! Содержит ли возможность
    bool contains(int code) const;
    //! Все возможности, отсортированные по коду
    QList<ProgramFeature> features() const;
    //! Конкретная возможность по ее коду
    ProgramFeature feature(int code) const;

    //! Содержит ли возможность для указанного модуля.
    bool contains(const EntityCode& entity_code, int code) const;
    //! Все возможности, отсортированные по коду для указанного модуля
    QList<ProgramFeature> features(const EntityCode& entity_code) const;
    //! Конкретная возможность по ее коду для указанного модуля
    ProgramFeature feature(const EntityCode& entity_code, int code) const;

    ProgramFeatures& operator=(const ProgramFeatures& f);

private:
    //! Добавить возможность
    void addFeature(const ProgramFeature& f);
    void addFeature(const QList<ProgramFeature>& features);
    //! Удалить возможность
    bool removeFeature(int code);

    QHash<int, ProgramFeature> _features;
    QHash<EntityCode, ProgramFeatures> _module_features;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::ProgramFeatures& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::ProgramFeatures& c);
};

/*! Права доступа, разбитые на  три уровня:
 * - Глобальные права доступа (действуют на все, если не указана конкретика по модулям или сущностям)
 * - Права по модулям (действуют на модуль вцелом, если не указана конкретика по сущностям)
 * - Права по конкретным сущностям */
class ZCORESHARED_EXPORT Permissions
{
public:
    Permissions();
    Permissions(const Permissions& r) = default;
    Permissions(
        //! Глобальные права доступа
        const AccessRights& global_rights,
        //! Права по модулям
        const QMap<EntityCode, AccessRights>& module_rights,
        //! Права по конкретным сущностям
        const QMap<Uid, AccessRights>& entity_rights);

    bool operator==(const Permissions& r) const;
    Permissions& operator=(const Permissions& r) = default;

    //! Глобальные права доступа
    AccessRights rights() const;
    //! Права доступа для модуля (вида сущности)
    AccessRights rights(const EntityCode& entity_code) const;
    //! Права доступа для конкретной сущности
    AccessRights rights(const Uid& entity_uid) const;

    //! Проверка наличия глобального права
    bool hasRight(const AccessRights& right) const;
    //! Проверка наличия глобального права по виду действия
    bool hasRight(ObjectActionType action_type) const;

    //! Проверка наличия права для вида сущности
    bool hasRight(const EntityCode& entity_code, const AccessRights& right) const;
    //! Проверка наличия права для модуля по виду действия
    bool hasRight(const EntityCode& entity_code, ObjectActionType action_type) const;

    //! Проверка наличия права для конкретной сущности
    bool hasRight(const Uid& entity_uid, const AccessRights& right) const;
    //! Проверка наличия права для конкретной сущности по виду действия
    bool hasRight(const Uid& entity_uid, ObjectActionType action_type) const;

    //! Права по модулям
    const QHash<EntityCode, AccessRights>& moduleRights() const;
    //! Права по сущностям
    const QHash<Uid, AccessRights>& entityRights() const;

protected:
    AccessRights _global_rights;
    QHash<EntityCode, AccessRights> _module_rights;
    QHash<Uid, AccessRights> _entity_rights;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::Permissions& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::Permissions& c);
};

//! Информация о пользователе
class ZCORESHARED_EXPORT UserInformation
{
public:
    UserInformation();
    UserInformation(const UserInformation& u) = default;
    UserInformation(const QString& login, const QString& last_name, const QString& first_name, const QString& middle_name, int gender_id);

    bool operator==(const UserInformation& u) const;
    UserInformation& operator=(const UserInformation& u) = default;

    QString login() const;

    QString firstname() const;
    QString lastName() const;
    QString middleName() const;

    QString fio() const;

    //! Пол (код из Catalog::GENDER)
    int genderId() const;

    bool isValid() const;

private:
    QString _login;
    QString _first_name;
    QString _last_name;
    QString _middle_name;
    //! Пол (код из Catalog::GENDER)
    int _gender_id = -1;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::UserInformation& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::UserInformation& c);
};

/*! Информация о подключении к БД. Запрашиваются у драйвера.
 * Результаты зависят от проверки прав доступа пользователя с указанными Credentials */
class ZCORESHARED_EXPORT ConnectionInformation
{
public:
    ConnectionInformation();
    ConnectionInformation(const ConnectionInformation& o) = default;
    ConnectionInformation(
        //! Адрес сервера
        const QString& host,
        //! Информация о пользователе
        const UserInformation& user_info,
        //! Права доступа для приложения в целом, типа сущностей или конкретной сущности
        const Permissions& direct_permissions,
        /*! Косвенные права доступа. Нужны для объектов(документов), связанных с какой либо сущностью.
         * Например: если даны права на просмотр для проекта, то это значит права доступа на просмотр для документов по данному проекту, но
         * не дает прав на просмотр карточки самого проекта. Для просмотра карточки проекта надо задать direct_permissions */
        const Permissions& relation_permissions,
        //! Возможности программы
        const ProgramFeatures& features,
        //! Доступные языки для хранения данных (это не языки пользовательского интерфейса)
        const QList<QLocale::Language>& languages,
        //! Предупреждение
        const QString& warning,
        //! Свойства соединения
        const ConnectionProperties& properties);

    bool operator==(const ConnectionInformation& o) const;
    bool operator!=(const ConnectionInformation& o) const;
    ConnectionInformation& operator=(const ConnectionInformation& o) = default;

    void clear();

    //! Адрес сервера
    const QString host() const;
    //! Информация о пользователе
    const UserInformation& userInformation() const;

    //! Права доступа для приложения в целом, типа сущностей или конкретной сущности
    const Permissions& directPermissions() const;
    /*! Косвенные права доступа. Нужны для объектов(документов), связанных с какой либо сущностью.
     * Например: если даны права на просмотр для проекта, то это значит права доступа на просмотр для документов по данному проекту, но
     * не дает прав на просмотр карточки самого проекта. Для просмотра карточки проекта надо задать direct_permissions */
    const Permissions& relationPermissions() const;

    //! Возможности программы
    const ProgramFeatures& features() const;
    //! Доступные языки для хранения данных и их названия (это не языки пользовательского интерфейса)
    const QMap<QLocale::Language, QString>& languages() const;

    //! Предупреждение
    QString warning() const;
    //! Свойства соединения
    ConnectionProperties properties() const;

    bool isValid() const;

    QByteArray toByteArray() const;
    static ConnectionInformation fromByteArray(const QByteArray& ba, Error& error);

private:
    bool _is_init = false;
    //! Адрес сервера
    QString _host;
    //! Информация о пользователе
    UserInformation _user_info;
    //! Права доступа прямые
    Permissions _direct_permissions;
    //! Права доступа для зависимостей
    Permissions _relation_permissions;
    //! Возможности программы
    ProgramFeatures _features;
    //! Доступные языки (это не языки пользовательского интерфейса)
    QMap<QLocale::Language, QString> _languages;
    //! Произвольная информация
    QString _warning;
    //! Свойства соединения
    ConnectionProperties _properties;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::ConnectionInformation& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::ConnectionInformation& c);
};

//! Информация о базе данных
class ZCORESHARED_EXPORT DatabaseInformation
{
public:
    DatabaseInformation();
    DatabaseInformation(
        //! Идентификатор БД
        const DatabaseID& id,
        //! Имя хоста до его распознавания
        const QString& host_original_name,
        //! Порт
        quint16 port,
        //! Подключение по SSL
        bool ssl = false,
        //! Имя базы
        const QString& database_name = {});
    DatabaseInformation(const DatabaseInformation& di);

    DatabaseInformation& operator=(const DatabaseInformation& di);

    bool isValid() const;

    //! Имя базы
    QString databaseName() const;

    //! Идентификатор БД
    DatabaseID id() const;
    //! Порт
    quint16 port() const;
    //! Подключение по SSL
    bool ssl() const;

    //! Имя хоста до его распознавания
    QString hostOriginalName() const;
    //! Составное имя из URL и IP (действительно после lookup)
    QString hostName() const;
    //! Распознать hostOriginalName. Запускает локальный event loop
    Error lookup(int timeout_ms);
    //! Было ли запущено lookup
    bool isLookup() const;
    //! Информация о хосте (действительно после lookup)
    QHostInfo hostInfo() const;

    bool operator<(const DatabaseInformation& di) const;
    bool operator==(const DatabaseInformation& di) const;

private:
    //! Идентификатор БД
    DatabaseID _id;
    //! Имя хоста до его распознавания
    QString _host_original_name;
    //! Порт
    quint16 _port = 0;
    //! Подключение по SSL
    bool _ssl = false;
    //! Имя базы
    mutable QString _database_name;
    //! Результат запуска lookup
    QHostInfo _lookup_result;
    //! Составное имя из URL и IP (действительно после lookup)
    QString _host_name;

    mutable QMutex _database_name_mutex;
    mutable QMutex _host_name_mutex;
    mutable QMutex _lookup_result_mutex;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::DatabaseInformation& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::DatabaseInformation& c);
};

inline uint qHash(const DatabaseInformation& di)
{
    return ::qHash(di.id().value());
}

} // namespace zf

typedef QMap<zf::Uid, zf::AccessRights> UidAccessRights;

Q_DECLARE_METATYPE(zf::ProgramFeature)
Q_DECLARE_METATYPE(zf::ProgramFeatures)
Q_DECLARE_METATYPE(zf::ConnectionInformation)
Q_DECLARE_METATYPE(zf::UserInformation)
Q_DECLARE_METATYPE(zf::DatabaseInformation)
