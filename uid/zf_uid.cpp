#include "zf_uid.h"
#include "zf_core.h"
#include "zf_logging.h"

#include <QApplication>
#include <QDataStream>
#include <QDebug>

namespace zf
{
//! Структура данных при сериализации
static const int _Uid_structure_version = 2;
//! Номер типа данных при регистрации через qRegisterMetaType
int Uid::_metaTypeNumber_uid = 0;
int Uid::_metaTypeNumber_uidList = 0;

// структура для формирования составного идентификатора сущности
union UniqueKeyUnion
{
    quint64 key_64 = 0;
    struct UniqueKeyStruct
    {
        int database : 16;
        int code : 16;
        int id : 32;
    };
    UniqueKeyStruct key_split;
};
static_assert(sizeof(UniqueKeyUnion::key_64) == sizeof(UniqueKeyUnion::UniqueKeyStruct));

//! Данные для Uid
class Uid_SharedData : public QSharedData
{
public:
    Uid_SharedData();
    Uid_SharedData(const Uid_SharedData& d);
    ~Uid_SharedData();

    void clear();

    static Uid_SharedData* shared_null();

    //! Тип
    UidType type = UidType::Invalid;
    //! Внутренний тип
    Uid::InternalType internal_type = Uid::InternalType::General;

    //! Код (тип сущности)
    int code = Consts::INVALID_ENTITY_CODE;

    //! Данные храняться в виде QVariantList или UniqueKeyUnion
    bool is_variant_list = false;
    //! Данные для сущностей
    UniqueKeyUnion int_key = {};
    //! Ключи для универсальных идентификаторов
    QVariantList* keys = nullptr;
    //! Произвольные данные
    QVariantList* data = nullptr;
    //! Временный ключ (для InternalType::TemporaryEntity)
    QString* temp_id = nullptr;

    uint hash_value = 0;
    //! Сериализованное значение
    QByteArray serialized;
    QString serialized_string;

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    //! Отладочная строка
    QString debug_string;
#endif
};

Q_GLOBAL_STATIC(Uid_SharedData, nullResult)
Uid_SharedData* Uid_SharedData::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0)
        res->ref.ref(); // чтобы не было удаления самого nullResult
    return res;
}

Uid_SharedData::Uid_SharedData()
{
    Z_DEBUG_NEW("Uid_SharedData");
}

Uid_SharedData::Uid_SharedData(const Uid_SharedData& d)
    : QSharedData(d)
{
    clear();

    if (d.keys != nullptr)
        keys = new QVariantList(*(d.keys));
    if (d.data != nullptr)
        data = new QVariantList(*(d.data));
    if (d.temp_id != nullptr)
        temp_id = new QString(*(d.temp_id));
    type = d.type;
    internal_type = d.internal_type;
    code = d.code;
    is_variant_list = d.is_variant_list;
    int_key = d.int_key;
    hash_value = d.hash_value;
    serialized = d.serialized;
    serialized_string = d.serialized_string;

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    debug_string = d.debug_string;
#endif

    Z_DEBUG_NEW("Uid_SharedData");
}

Uid_SharedData::~Uid_SharedData()
{
    clear();
    Z_DEBUG_DELETE("Uid_SharedData");
}

void Uid_SharedData::clear()
{
    if (keys != nullptr) {
        delete keys;
        keys = nullptr;
    }
    if (data != nullptr) {
        delete data;
        data = nullptr;
    }
    if (temp_id != nullptr) {
        delete temp_id;
        temp_id = nullptr;
    }

    type = UidType::Invalid;
    internal_type = Uid::InternalType::General;
    code = Consts::INVALID_ENTITY_CODE;
    is_variant_list = false;
    int_key.key_64 = 0;
    serialized.clear();
    serialized_string.clear();
    hash_value = ::qHash(0);
}

Uid::Uid()
    : _d(Uid_SharedData::shared_null())
{
}

Uid::Uid(int code, const DatabaseID& database, const QVariantList* keys, const QVariantList* data, const EntityID& id, UidType type,
         Uid::InternalType internal_type)
    : _d(new Uid_SharedData())
{
    _d->code = code;
    _d->type = type;
    _d->internal_type = internal_type;

    _d->is_variant_list = (internal_type == Uid::InternalType::General);
    if (_d->is_variant_list) {
        _d->keys = keys != nullptr ? new QVariantList(*keys) : new QVariantList();
        Z_CHECK(!database.isValid() && !id.isValid());

        if (internal_type == InternalType::TemporaryEntity)
            _d->temp_id = new QString(Utils::generateUniqueString());

    } else {
        Z_CHECK(keys == nullptr);
        _d->int_key.key_split.code = code;
        _d->int_key.key_split.database = database.value();
        _d->int_key.key_split.id = id.value();

        if (internal_type == InternalType::TemporaryEntity)
            _d->temp_id = new QString(Utils::generateUniqueString());
    }

    if (data != nullptr)
        _d->data = new QVariantList(*data);

    _d->serialized = serializeHelper();
    _d->serialized_string = _d->serialized.toBase64();
    _d->hash_value = ::qHash(_d->serialized);

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    _d->debug_string = toPrintableHelper();
#endif

    check();
}

void Uid::check() const
{
    Z_CHECK(_d->type != UidType::Invalid);
    Z_CHECK(_d->code > Consts::INVALID_ENTITY_CODE);

    if (_d->type == UidType::UniqueEntity || _d->type == UidType::Entity) {
        Z_CHECK(_d->keys == nullptr);
        Z_CHECK(_d->code <= Consts::MAX_ENTITY_CODE);
    }

    if (_d->type == UidType::Entity && _d->internal_type != InternalType::TemporaryEntity) {
        Z_CHECK(_d->int_key.key_split.id > Consts::INVALID_ENTITY_ID);
    }

    if (CoreUids::isCoreUid(*this))
        Z_CHECK(_d->code < Consts::MIN_ENTITY_CODE);
    else if (_d->type == UidType::Entity || _d->type == UidType::UniqueEntity)
        Z_CHECK(_d->code >= Consts::MIN_ENTITY_CODE);
}

Uid Uid::general(int code, const QVariantList& keys, bool is_temporary, const QVariantList& data)
{
    return Uid(code, DatabaseID(), &keys, data.isEmpty() ? nullptr : &data, EntityID(), UidType::General,
               is_temporary ? InternalType::TemporaryEntity : InternalType::General);
}

Uid Uid::general(const QVariantList& keys, const QVariantList& data)
{
    return general(1, keys, false, data);
}

Uid Uid::general(const QStringList& keys, const QVariantList& data)
{
    return general(Utils::toVariantList(keys), data);
}

Uid Uid::general(const QList<int>& keys, const QVariantList& data)
{
    return general(Utils::toVariantList(keys), data);
}

Uid Uid::general(const UidList& keys, const QVariantList& data)
{
    return general(Utils::toVariantList(keys), data);
}

Uid Uid::general(const QVariant& key, const QVariantList& data)
{
    return general(QVariantList {key}, data);
}

Uid Uid::general(const QString& key, const QVariantList& data)
{
    return general(QVariantList {key}, data);
}

Uid Uid::general(const char* key, const QVariantList& data)
{
    return general(QString::fromUtf8(key), data);
}

Uid Uid::general(qlonglong key, const QVariantList& data)
{
    return general(QVariantList {key}, data);
}

Uid::Uid(const Uid& uid)
    : _d(uid._d)
{
}

Uid::~Uid()
{
}

Uid& Uid::operator=(const Uid& uid)
{
    if (this != &uid) {
        _d = uid._d;
    }

    return *this;
}

bool Uid::operator==(const Uid& uid) const
{
    if (_d == uid._d)
        return true;

    if (_d->type != uid._d->type || _d->internal_type != uid._d->internal_type || _d->code != uid._d->code)
        return false;

    if (_d->internal_type == Uid::InternalType::TemporaryEntity)
        return *(_d->temp_id) == *(uid._d->temp_id);

    return _d->is_variant_list ? _d->serialized == uid._d->serialized : _d->int_key.key_64 == uid._d->int_key.key_64;
}

bool Uid::operator<(const Uid& uid) const
{
    if (_d == uid._d)
        return false;

    if (_d->internal_type == Uid::InternalType::TemporaryEntity && uid._d->internal_type == Uid::InternalType::TemporaryEntity)
        return *(_d->temp_id) < *(uid._d->temp_id);
    else if (_d->internal_type == Uid::InternalType::TemporaryEntity && uid._d->internal_type != Uid::InternalType::TemporaryEntity)
        return true;
    else if (_d->internal_type != Uid::InternalType::TemporaryEntity && uid._d->internal_type == Uid::InternalType::TemporaryEntity)
        return false;

    return _d->is_variant_list ? _d->serialized < uid._d->serialized : _d->int_key.key_64 < uid._d->int_key.key_64;
}

bool Uid::isValid() const
{
    return _d->type != UidType::Invalid;
}

void Uid::clear()
{
    if (!isValid())
        return;

    _d->clear();
}

int Uid::code() const
{
    Z_CHECK(isValid());
    return _d->code;
}

UidType Uid::type() const
{
    return _d->type;
}

bool Uid::isEntity() const
{
    return _d->type == UidType::Entity || _d->type == UidType::UniqueEntity;
}

bool Uid::isUniqueEntity() const
{
    return _d->type == UidType::UniqueEntity;
}

bool Uid::isTemporary() const
{
    return _d->internal_type == InternalType::TemporaryEntity;
}

bool Uid::isPersistent() const
{
    return isEntity() && !isTemporary();
}

bool Uid::isCore() const
{
    return _d->internal_type == InternalType::CoreEntity;
}

bool Uid::isGeneral() const
{
    return _d->type == UidType::General;
}

DatabaseID Uid::database() const
{
    Z_CHECK(isEntity());
    return _d->int_key.key_split.database == Consts::INVALID_DATABASE_ID ? DatabaseID() : DatabaseID(_d->int_key.key_split.database);
}

EntityCode Uid::entityCode() const
{
    Z_CHECK(isValid());
    return EntityCode(_d->code);
}

EntityID Uid::id() const
{
    Z_CHECK(isEntity() && !isUniqueEntity() && !isTemporary());
    return EntityID(_d->int_key.key_split.id);
}

QString Uid::tempId() const
{
    Z_CHECK(isEntity() && !isUniqueEntity() && isTemporary());
    return *_d->temp_id;
}

QString Uid::toPrintable() const
{
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    return _d->debug_string;
#else
    return toPrintableHelper();
#endif
}

void Uid::debPrint() const
{
    Core::debPrint(variant());
}

int Uid::count() const
{
    Z_CHECK(isValid());
    if (_d->is_variant_list)
        return _d->keys != nullptr ? _d->keys->count() : 0;

    return 1;
}

QString Uid::asString(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n).toString() : QString::number(_d->int_key.key_64);
}

int Uid::asInt(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count() && _d->is_variant_list);
    return _d->keys->at(n).toInt();
}

uint Uid::asUInt(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count() && _d->is_variant_list);
    return _d->keys->at(n).toUInt();
}

qint64 Uid::asInt64(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count() && _d->is_variant_list);
    return _d->keys->at(n).toLongLong();
}

quint64 Uid::asUInt64(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n).toULongLong() : _d->int_key.key_64;
}

float Uid::asFloat(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n).toFloat() : _d->int_key.key_64;
}

double Uid::asDouble(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n).toDouble() : _d->int_key.key_64;
}

bool Uid::asBool(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count() && _d->is_variant_list);
    return _d->keys->at(n).toBool();
}

QByteArray Uid::asByteArray(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n).toByteArray() : QString::number(_d->int_key.key_64).toLatin1();
}

QVariant Uid::asVariant(int n) const
{
    Z_CHECK(isValid() && n >= 0 && n < count());
    return _d->is_variant_list ? _d->keys->at(n) : QVariant(_d->int_key.key_64);
}

QByteArray Uid::serialize() const
{
    return _d->serialized;
}

QString Uid::serializeToString() const
{
    return _d->serialized_string;
}

QByteArray Uid::serializeHelper() const
{
    if (!isValid())
        return QByteArray();

    QByteArray ba;
    QDataStream st(&ba, QIODevice::WriteOnly);
    st.setVersion(Consts::DATASTREAM_VERSION);
    st << _Uid_structure_version;
    st << static_cast<int>(_d->type);
    st << static_cast<int>(_d->internal_type);
    st << static_cast<int>(_d->code);
    st << _d->int_key.key_64;    
    st << _d->is_variant_list;
    if (_d->is_variant_list) {
        st << *_d->keys;
    }

    int data_count = (_d->data == nullptr ? 0 : _d->data->count());
    st << data_count;
    if (data_count > 0)
        st << *_d->data;

    if (_d->internal_type == InternalType::TemporaryEntity)
        st << *_d->temp_id;

    Z_CHECK_X(st.status() == QDataStream::Ok, QString::number(static_cast<int>(st.status())));
    return ba;
}

QString Uid::toPrintableHelper() const
{
    switch (_d->type) {
        case UidType::Invalid:
            return QStringLiteral("invalid");

        case UidType::UniqueEntity:
            return QStringLiteral("Unique(%1)").arg(_d->code);

        case UidType::Entity:
            if (_d->internal_type == InternalType::TemporaryEntity)
                return QStringLiteral("Entity(code:%1-db:%2-key:%3)")
                    .arg(_d->code)
                    .arg(_d->int_key.key_split.database)
                    .arg(*_d->temp_id);
            else
                return QStringLiteral("Entity(code:%1-db:%2-key:%3)")
                    .arg(_d->code)
                    .arg(_d->int_key.key_split.database)
                    .arg(_d->int_key.key_split.id);

        case UidType::General: {
            QString res = QStringLiteral("General(%2").arg(_d->code);
            if (_d->keys != nullptr) {
                for (int i = 0; i < _d->keys->count(); i++) {
                    if (i == 0)
                        res += ':';
                    else
                        res += ',';
                    res += _d->keys->at(i).toString();
                }
            }
            res += ')';
            return res;
        }
    }
    Z_HALT_INT;
    return QString();
}

Uid Uid::deserialize(const QByteArray& data)
{
    Uid uid;
    bool error = false;

    if (!data.isEmpty()) {
        QDataStream st(data);
        st.setVersion(Consts::DATASTREAM_VERSION);
        int int_data;
        st >> int_data;
        if (st.status() != QDataStream::Ok)
            error = true;
        if (!error && int_data != _Uid_structure_version)
            error = true;

        if (!error) {
            st >> int_data;
            if (st.status() == QDataStream::Ok)
                uid._d->type = static_cast<UidType>(int_data);
            else
                error = true;
        }
        if (!error) {
            st >> int_data;
            if (st.status() == QDataStream::Ok)
                uid._d->internal_type = static_cast<Uid::InternalType>(int_data);
            else
                error = true;
        }
        if (!error) {
            st >> int_data;
            if (st.status() == QDataStream::Ok)
                uid._d->code = int_data;
            else
                error = true;
        }
        if (!error) {
            st >> uid._d->int_key.key_64;
            if (st.status() != QDataStream::Ok)
                error = true;
        }        
        if (!error) {
            st >> uid._d->is_variant_list;
            if (st.status() != QDataStream::Ok)
                error = true;
        }
        if (!error && uid._d->is_variant_list) {
            uid._d->keys = new QVariantList;
            st >> *uid._d->keys;
            if (st.status() != QDataStream::Ok)
                error = true;
        }

        int data_count;
        st >> data_count;
        if (data_count > 0 && st.status() == QDataStream::Ok) {
            // защита от тупорылого Qt, который не может проверить наличие массива в стриме и пытаетсмя его сразу создать
            auto st_status = st.status();
            quint32 n;
            st >> n;
            if (st.status() == QDataStream::Ok && n < 5000) // идентификатор с таким количеством - это бред
            {
                st.resetStatus();
                st.setStatus(st_status);
                uid._d->data = new QVariantList;
                st >> *(uid._d->data);
            } else {
                error = true;
            }
        }
        if (st.status() != QDataStream::Ok)
            error = true;

        if (!error && uid._d->internal_type == InternalType::TemporaryEntity) {
            uid._d->temp_id = new QString;
            st >> *uid._d->temp_id;
            if (st.status() != QDataStream::Ok)
                error = true;
        }
    }

    if (error) {
        uid.clear();

    } else {
        uid._d->serialized = data;
        uid._d->serialized_string = data.toBase64();
        uid._d->hash_value = ::qHash(data);
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
        uid._d->debug_string = uid.toPrintableHelper();
#endif
    }

    return uid;
}

Uid Uid::deserialize(const QString& data)
{
    return deserialize(QByteArray::fromBase64(data.toLatin1()));
}

Uid Uid::deserialize(const QVariant& data)
{
    if (!data.isValid())
        return Uid();

    if (data.type() == QVariant::String)
        return deserialize(QByteArray::fromBase64(data.toString().toLatin1()));
    if (data.type() == QVariant::ByteArray)
        return deserialize(data.toByteArray());
    return Uid();
}

QVariant Uid::variant() const
{
    return QVariant::fromValue(*this);
}

Uid Uid::fromVariant(const QVariant& v)
{
    return v.value<Uid>();
}

UidList Uid::fromVariantList(const QVariant& v)
{
    return v.value<UidList>();
}

bool Uid::isUidVariant(const QVariant& v)
{
    return v.isValid() && (v.userType() == _metaTypeNumber_uid);
}

bool Uid::isUidListVariant(const QVariant& v)
{
    return v.isValid() && (v.userType() == _metaTypeNumber_uidList);
}

Uid Uid::coreEntity(const EntityCode& code)
{
    return Uid(code.value(), DatabaseID(), nullptr, nullptr, EntityID(), UidType::UniqueEntity, InternalType::CoreEntity);
}

QVariantList Uid::keys() const
{
    Z_CHECK(type() == UidType::General);
    return _d->keys == nullptr ? QVariantList() : *_d->keys;
}

QVariantList Uid::data() const
{
    Z_CHECK(type() == UidType::General);
    return _d->data == nullptr ? QVariantList() : *_d->data;
}

Uid::InternalType Uid::internalType() const
{
    return _d->internal_type;
}

Uid Uid::entity(const EntityCode& code, const EntityID& id, const DatabaseID& database)
{
    if (!Utils::isMainThread())
        return Uid(code.value(), database, nullptr, nullptr, id, UidType::Entity, InternalType::PersistentEntity);

    quint64 cache_key = createUniqueKey(InternalType::PersistentEntity, UidType::Entity, code.value(), database, id);
    Uid* res = entityCache()->object(cache_key);
    if (res == nullptr) {
        res = new Uid(code.value(), database, nullptr, nullptr, id, UidType::Entity, InternalType::PersistentEntity);
        if (!Core::isBootstraped()) {
            Uid r = *res;
            delete res;
            return r;
        }

        entityCache()->insert(cache_key, res);
    }
    return *res;
}

Uid Uid::entity(const EntityCode& code, const EntityID& id)
{
    return entity(code, id, Core::defaultDatabase());
}

Uid Uid::uniqueEntity(const EntityCode& code, const DatabaseID& database)
{
    if (CoreUids::isCoreUid(code))
        return coreEntity(code);

    if (!Utils::isMainThread())
        return Uid(code.value(), database, nullptr, nullptr, EntityID(), UidType::UniqueEntity, InternalType::PersistentEntity);

    quint64 cache_key = createUniqueKey(InternalType::PersistentEntity, UidType::UniqueEntity, code.value(), database, EntityID());
    Uid* res = entityCache()->object(cache_key);
    if (!res) {
        res = new Uid(code.value(), database, nullptr, nullptr, EntityID(), UidType::UniqueEntity, InternalType::PersistentEntity);
        if (!Core::isBootstraped()) {
            Uid r = *res;
            delete res;
            return r;
        }

        entityCache()->insert(cache_key, res);
    }
    return *res;
}

Uid Uid::uniqueEntity(const EntityCode& code)
{
    if (CoreUids::isCoreUid(code))
        return coreEntity(code);

    return uniqueEntity(code, Core::defaultDatabase());
}

Uid Uid::tempEntity(const EntityCode& code, const DatabaseID& database)
{
    return Uid(code.value(), database, nullptr, nullptr, EntityID(), UidType::Entity, InternalType::TemporaryEntity);
}

Uid Uid::tempEntity(const EntityCode& code)
{
    return tempEntity(code, Core::defaultDatabase());
}

Uid Uid::createPersistent(const EntityID& id) const
{
    Z_CHECK(isTemporary());
    return entity(entityCode(), id, database());
}

uint Uid::hashValue() const
{
    return _d->hash_value;
}

quint64 Uid::createUniqueKey(InternalType i_type, UidType type, int code, const DatabaseID& database, const EntityID& id)
{
    UniqueKeyUnion ukey;
    switch (type) {
        case UidType::Invalid:
            break;
        case UidType::UniqueEntity: {
            if (i_type == InternalType::CoreEntity) {
                Z_CHECK(code > 0 && code <= Consts::MAX_ENTITY_CODE);
            } else {
                Z_CHECK(code > 0 && code <= Consts::MAX_ENTITY_CODE && database.isValid());
            }
            ukey.key_split.code = code;
            ukey.key_split.database = database.value();
            break;
        }

        case UidType::Entity: {
            Z_CHECK(code > 0 && code <= Consts::MAX_ENTITY_CODE && database.isValid() && id.isValid());
            ukey.key_split.code = code;
            ukey.key_split.database = database.value();
            ukey.key_split.id = id.value();
            break;
        }
        case UidType::General:
            Z_HALT_INT;
            break;
    }

    return ukey.key_64;
}

void Uid::parseUniqueKey(quint64 key, int& code, DatabaseID& database, EntityID& id)
{
    UniqueKeyUnion ukey;
    ukey.key_64 = key;
    code = ukey.key_split.code;
    database = DatabaseID(ukey.key_split.database);
    id = EntityID(ukey.key_split.id);
}

QCache<quint64, Uid>* Uid::entityCache()
{
    static std::unique_ptr<QCache<quint64, Uid>> _entity_cache;
    if (_entity_cache == nullptr)
        _entity_cache = std::make_unique<QCache<quint64, Uid>>(1000);
    return _entity_cache.get();
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::Uid& uid)
{
    return s << uid.serialize();
}

QDataStream& operator>>(QDataStream& s, zf::Uid& uid)
{
    QByteArray buffer;
    s >> buffer;
    uid = (s.status() == QDataStream::Ok ? zf::Uid::deserialize(buffer) : zf::Uid());
    return s;
}

QDebug operator<<(QDebug debug, const zf::Uid& c)
{
    QDebugStateSaver saver(debug);
    debug.noquote().nospace();
    debug << c.toPrintable();
    return debug;
}

QDebug operator<<(QDebug debug, const zf::UidList& c)
{
    QDebugStateSaver saver(debug);
    debug.noquote().nospace();

    QStringList names;
    for (auto& u : c) {
        names << u.toPrintable();
    }

    debug << names.join(" | ");
    return debug;
}

QDebug operator<<(QDebug debug, const zf::UidSet& c)
{
    return operator<<(debug, c.values());
}
