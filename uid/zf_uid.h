#pragma once

#include <QVariant>
#include <QSharedDataPointer>
#include <QCache>
#include "zf_global.h"
#include "zf_basic_types.h"

namespace zf
{
class Uid_SharedData;
class Uid;
typedef QList<Uid> UidList;
typedef QSet<Uid> UidSet;

//! Универсальный идентификатор
class ZCORESHARED_EXPORT Uid
{    
public:
    //! Неинициализированный идентификатор
    Uid();    
    Uid(const Uid& uid);    
    ~Uid();

    Uid& operator=(const Uid& uid);
    bool operator==(const Uid& uid) const;
    bool operator!=(const Uid& uid) const { return !(operator==(uid)); }
    bool operator<(const Uid& uid) const;

    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Набор ключей, который идентифицирует конкретный экземпляр
        const QVariantList& keys,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Набор ключей, который идентифицирует конкретный экземпляр
        const QStringList& keys,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Набор ключей, который идентифицирует конкретный экземпляр
        const QList<int>& keys,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Набор ключей, который идентифицирует конкретный экземпляр
        const UidList& keys,
        //! Произвольные данные
        const QVariantList& data = {});

    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Ключ, который идентифицирует конкретный экземпляр
        const QVariant& key,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Ключ, который идентифицирует конкретный экземпляр
        const QString& key,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Ключ, который идентифицирует конкретный экземпляр
        const char* key,
        //! Произвольные данные
        const QVariantList& data = {});
    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Ключ, который идентифицирует конкретный экземпляр
        qlonglong key,
        //! Произвольные данные
        const QVariantList& data = {});

    /*! Универсальный конструктор. Не использовать для сущностей */
    static Uid general(
        //! Уникальный код идентификаторов данного вида
        int code,
        //! Набор ключей, который идентифицирует конкретный экземпляр
        const QVariantList& keys,
        //! Является ли временным (например новый объект, не существующий в БД)
        bool is_temporary = false,
        //! Произвольные данные
        const QVariantList& data = {});

    //! Создать новый идентификатор уникальной сущности
    static Uid uniqueEntity(
        //! Код сущности
        const EntityCode& code,
        //! Идентификатор базы данных
        const DatabaseID& database);
    //! Создать новый идентификатор уникальной сущности. БД: Core::defaultDatabase()
    static Uid uniqueEntity(
        //! Код сущности
        const EntityCode& code);

    //! Создать новый идентификатор сущности
    static Uid entity(
        //! Код сущности
        const EntityCode& code,
        //! Ключевое значение
        const EntityID& id,
        //! Идентификатор базы данных
        const DatabaseID& database);
    //! Создать новый идентификатор сущности. БД: Core::defaultDatabase()
    static Uid entity(
        //! Код сущности
        const EntityCode& code,
        //! Ключевое значение
        const EntityID& id);

    //! Создать новый идентификатор временной сущности (не существует в БД)
    static Uid tempEntity(
        //! Код сущности
        const EntityCode& code,
        //! Идентификатор базы данных
        const DatabaseID& database);
    //! Создать новый идентификатор временной сущности (не существует в БД). БД: Core::defaultDatabase()
    static Uid tempEntity(
        //! Код сущности
        const EntityCode& code);

    //! Сформировать постоянный идентификатор из временного
    Uid createPersistent(const EntityID& id) const;

    bool isValid() const;
    //! Очистить
    void clear();

    //! Код (для UidType::General)
    int code() const;
    //! Тип
    UidType type() const;

    //! Является ли идентификатором сущности (обычной или уникальной)
    bool isEntity() const;
    //! Является ли идентификатором уникальной сущности
    bool isUniqueEntity() const;
    //! Является ли идентификатор временным. Для временных идентификаторов используется случайно сгенерированная строка
    //! в качестве ключа
    bool isTemporary() const;
    //! Является ли идентификатор постоянным
    bool isPersistent() const;
    //! Является ли зарезервированным идентификатор
    bool isCore() const;
    //! Идентификатор общего вида (не связанный с сущностями)
    bool isGeneral() const;

    //! Идентификатор базы данных (только для Entity)
    DatabaseID database() const;
    //! Код сущности
    EntityCode entityCode() const;
    //! Ключевое значение сущности (только для Entity и при условии что isTemporary() == false)
    EntityID id() const;
    //! Ключевое значение для временного идентификатора
    QString tempId() const;

    //! Наглядное представление для отладки
    QString toPrintable() const;
    //! Вывести содержимое для отладки
    void debPrint() const;

    //! Количество ключей
    int count() const;

    // Получить ключ с номером n
    QString asString(int n) const;
    int asInt(int n) const;
    uint asUInt(int n) const;
    qint64 asInt64(int n) const;
    quint64 asUInt64(int n) const;
    float asFloat(int n) const;
    double asDouble(int n) const;
    bool asBool(int n) const;
    QByteArray asByteArray(int n) const;
    QVariant asVariant(int n) const;

    //! Сериализация в QByteArray
    QByteArray serialize() const;
    //! Сериализация в QString
    QString serializeToString() const;

    //! Десериализация из QByteArray
    static Uid deserialize(const QByteArray& data);
    //! Десериализация из QString
    static Uid deserialize(const QString& data);
    //! Десериализация из QVariant
    static Uid deserialize(const QVariant& data);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static Uid fromVariant(const QVariant& v);
    //! Восстановить из QVariant
    static UidList fromVariantList(const QVariant& v);

    //! Проверка хранит ли данный QVariant Uid
    static bool isUidVariant(const QVariant& v);
    //! Проверка хранит ли данный QVariant UidList
    static bool isUidListVariant(const QVariant& v);

    //! Создать новый идентификатор уникальной сущности для зарезервированных объектов. Не вызывать
    static Uid coreEntity(
        //! Код сущности
        const EntityCode& code);

    //! Набор ключевых значений для идентификаторов виде General
    QVariantList keys() const;
    //! Набор произвольных данных значений для идентификаторов виде General
    QVariantList data() const;

    //! Значение для qHash
    uint hashValue() const;

private:
    //! Внутренняя классификация идентификаторов
    enum class InternalType
    {
        General,
        //! Сущность существует в БД
        PersistentEntity,
        //! Сущность отсутствует в БД
        TemporaryEntity,
        //! Зарезервированно под нужны ядра
        CoreEntity
    };

    // Универсальный конструктор
    explicit Uid(int code, const DatabaseID& database, const QVariantList* keys, const QVariantList* data, const EntityID& id, UidType type,
                 Uid::InternalType internal_type);

    //! Внутренняя классификация идентификаторов
    InternalType internalType() const;

    //! Сериализация в QByteArray
    QByteArray serializeHelper() const;
    //! Приведение к наглядному виду
    QString toPrintableHelper() const;

    //! Создание составного ключа сущности
    static quint64 createUniqueKey(InternalType i_type, UidType type, int code, const DatabaseID& database, const EntityID& id);
    //! Разбор составного ключа для сущности на части
    static void parseUniqueKey(quint64 key, int& code, DatabaseID& database, EntityID& id);

    //! Кэш идентификаторов сущностей
    static QCache<quint64, Uid>* entityCache();

    //! Проверка на корректность
    void check() const;

    //! Разделяемые данные
    QSharedDataPointer<Uid_SharedData> _d;

    //! Номер типа данных при регистрации через qRegisterMetaType
    static int _metaTypeNumber_uid;
    static int _metaTypeNumber_uidList;

    friend class Uid_SharedData;
    friend class Framework;
};

inline uint qHash(const Uid& uid)
{
    return uid.hashValue();
}

} // namespace zf

Q_DECLARE_METATYPE(zf::Uid)

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::Uid& uid);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::Uid& uid);

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Uid& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::UidList& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::UidSet& c);
