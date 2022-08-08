#include "zf_connection_information.h"
#include "zf_core.h"
#include "zf_translator.h"
#include <QDebug>

namespace zf
{
ProgramFeatures::ProgramFeatures()
{
}

ProgramFeatures::ProgramFeatures(const ProgramFeatures& f)
    : _features(f._features)
    , _module_features(f._module_features)
{
}

ProgramFeatures::ProgramFeatures(const QList<ProgramFeature>& features, const QHash<EntityCode, ProgramFeatures>& module_features)
{
    addFeature(features);
    _module_features = module_features;
}

bool ProgramFeatures::operator==(const ProgramFeatures& f) const
{
    return _features == f._features && _module_features == f._module_features;
}

bool ProgramFeatures::isEmpty() const
{
    return _features.isEmpty() && _module_features.isEmpty();
}

void ProgramFeatures::addFeature(const ProgramFeature& f)
{
    Z_CHECK(f.isValid());
    _features[f.code()] = f;
}

void ProgramFeatures::addFeature(const QList<ProgramFeature>& features)
{
    for (auto& f : features)
        addFeature(f);
}

bool ProgramFeatures::removeFeature(int code)
{
    return _features.remove(code);
}

bool ProgramFeatures::contains(int code) const
{
    return _features.contains(code);
}

QList<ProgramFeature> ProgramFeatures::features() const
{
    QList<ProgramFeature> res = _features.values();
    std::sort(res.begin(), res.end());
    return res;
}

ProgramFeature ProgramFeatures::feature(int code) const
{
    return _features.value(code);
}

bool ProgramFeatures::contains(const EntityCode& entity_code, int code) const
{
    return _module_features.value(entity_code).contains(code);
}

QList<ProgramFeature> ProgramFeatures::features(const EntityCode& entity_code) const
{
    return _module_features.value(entity_code).features();
}

ProgramFeature ProgramFeatures::feature(const EntityCode& entity_code, int code) const
{
    return _module_features.value(entity_code).feature(code);
}

ProgramFeatures& ProgramFeatures::operator=(const ProgramFeatures& f)
{
    _features = f._features;
    _module_features = f._module_features;
    return *this;
}

ProgramFeature::ProgramFeature()
{
}

ProgramFeature::ProgramFeature(int code, const QString& translation_id)
    : _code(code)
    , _translation_id(translation_id)
{
    Z_CHECK(code >= 0);
}

ProgramFeature::ProgramFeature(const ProgramFeature& f)
    : _code(f._code)
    , _translation_id(f._translation_id)
{
}

bool ProgramFeature::isValid() const
{
    return _code >= 0;
}

int ProgramFeature::code() const
{
    return _code;
}

QString ProgramFeature::name() const
{
    return _translation_id.isEmpty() ? QString::number(_code) : translate(_translation_id);
}

ProgramFeature& ProgramFeature::operator=(const ProgramFeature& f)
{
    _code = f._code;
    _translation_id = f._translation_id;
    return *this;
}

bool ProgramFeature::operator==(const ProgramFeature& f) const
{
    return _code == f._code;
}

bool ProgramFeature::operator!=(const ProgramFeature& f) const
{
    return !operator==(f);
}

bool ProgramFeature::operator<(const ProgramFeature& f) const
{
    return _code < f._code;
}

ConnectionInformation::ConnectionInformation()
{
}

ConnectionInformation::ConnectionInformation(const QString& host, const UserInformation& user_info, const Permissions& rights,
                                             const Permissions& relation_permissions, const ProgramFeatures& features,
                                             const QList<QLocale::Language>& languages, const QString& warning,
                                             const ConnectionProperties& properties)
    : _is_init(true)
    , _host(host.trimmed())
    , _user_info(user_info)
    , _direct_permissions(rights)
    , _relation_permissions(relation_permissions)
    , _features(features)
    , _warning(warning)
    , _properties(properties)
{
    Z_CHECK(!_host.isEmpty());
    Z_CHECK(_user_info.isValid());

    for (auto l : languages) {
        if (l == QLocale::AnyLanguage) {
            Core::logError("ConnectionInformation: wrong language");
            continue;
        }
        _languages[l] = QLocale::languageToString(l);
    }
}

bool ConnectionInformation::operator==(const ConnectionInformation& o) const
{
    if (!isValid() && !o.isValid())
        return true;

    if (isValid() != o.isValid())
        return false;

    return _host == o._host && _user_info == o._user_info && _direct_permissions == o._direct_permissions
           && _relation_permissions == o._relation_permissions && _features == o._features && _languages == o._languages
           && _properties == o._properties;
}

bool ConnectionInformation::operator!=(const ConnectionInformation& o) const
{
    return !operator==(o);
}

void ConnectionInformation::clear()
{
    *this = ConnectionInformation();
}

const QString ConnectionInformation::host() const
{
    return _host;
}

const UserInformation& ConnectionInformation::userInformation() const
{
    return _user_info;
}

const Permissions& ConnectionInformation::directPermissions() const
{
    return _direct_permissions;
}

const Permissions& ConnectionInformation::relationPermissions() const
{
    return _relation_permissions;
}

const ProgramFeatures& ConnectionInformation::features() const
{
    return _features;
}

const QMap<QLocale::Language, QString>& ConnectionInformation::languages() const
{
    return _languages;
}

QString ConnectionInformation::warning() const
{
    return _warning;
}

ConnectionProperties ConnectionInformation::properties() const
{
    return _properties;
}

bool ConnectionInformation::isValid() const
{
    return _is_init;
}

QByteArray ConnectionInformation::toByteArray() const
{
    QByteArray ba;
    QDataStream st(&ba, QIODevice::WriteOnly);
    st.setVersion(Consts::DATASTREAM_VERSION);
    st << *this;
    return ba;
}

ConnectionInformation ConnectionInformation::fromByteArray(const QByteArray& ba, Error& error)
{
    QDataStream st(ba);
    st.setVersion(Consts::DATASTREAM_VERSION);
    ConnectionInformation ci;
    st >> ci;

    if (st.status() != QDataStream::Ok) {
        error = Error::corruptedDataError();
        return {};
    } else
        error.clear();

    return ci;
}

UserInformation::UserInformation()
{
}

UserInformation::UserInformation(const QString& login, const QString& last_name, const QString& first_name, const QString& middle_name,
                                 int gender_id)
    : _login(login.trimmed())
    , _first_name(first_name.trimmed())
    , _last_name(last_name.trimmed())
    , _middle_name(middle_name.trimmed())
    , _gender_id(gender_id)
{
    Z_CHECK(!_login.isEmpty());
    Z_CHECK(_gender_id > 0);
    if (_first_name.isEmpty() && _last_name.isEmpty() && _middle_name.isEmpty())
        _last_name = _login;
}

bool UserInformation::operator==(const UserInformation& u) const
{
    return _login == u._login && _first_name == u._first_name && _last_name == u._last_name && _middle_name == u._middle_name
           && _gender_id == u._gender_id;
}

QString UserInformation::login() const
{
    return _login;
}

QString UserInformation::firstname() const
{
    return _first_name;
}

QString UserInformation::lastName() const
{
    return _last_name;
}

QString UserInformation::middleName() const
{
    return _middle_name;
}

QString UserInformation::fio() const
{
    return QString(_last_name + " " + _first_name + " " + _middle_name).simplified();
}

int UserInformation::genderId() const
{
    return _gender_id;
}

bool UserInformation::isValid() const
{
    return _gender_id > 0;
}

Permissions::Permissions()
{
}

Permissions::Permissions(const AccessRights& global_rights, const QMap<EntityCode, AccessRights>& module_rights,
                         const QMap<Uid, AccessRights>& entity_rights)
{
    _global_rights = global_rights;
    _module_rights = Utils::toHash<EntityCode, AccessRights>(module_rights);
    _entity_rights = Utils::toHash<Uid, AccessRights>(entity_rights);
}

bool Permissions::operator==(const Permissions& r) const
{
    return _global_rights == r._global_rights && _module_rights == r._module_rights && _entity_rights == r._entity_rights;
}

AccessRights Permissions::rights() const
{
    return _global_rights;
}

AccessRights Permissions::rights(const EntityCode& entity_code) const
{
    Z_CHECK(entity_code.isValid());

    auto it = _module_rights.find(entity_code);
    return (it == _module_rights.end()) ? rights() : it.value();
}

AccessRights Permissions::rights(const Uid& entity_uid) const
{
    Z_CHECK(entity_uid.isValid());

    auto it = _entity_rights.find(entity_uid);
    return (it == _entity_rights.end()) ? rights(entity_uid.entityCode()) : it.value();
}

const QHash<EntityCode, AccessRights>& Permissions::moduleRights() const
{
    return _module_rights;
}

const QHash<Uid, AccessRights>& Permissions::entityRights() const
{
    return _entity_rights;
}

bool Permissions::hasRight(const AccessRights& right) const
{
    return _global_rights.contains(right);
}

bool Permissions::hasRight(const EntityCode& entity_code, const AccessRights& right) const
{
    Z_CHECK(entity_code.isValid());

    if (_global_rights.contains(AccessFlag::Administrate) || _global_rights.contains(AccessFlag::Developer))
        return true;

    auto it = _module_rights.find(entity_code);
    if (it == _module_rights.end())
        return hasRight(right);

    return it.value().contains(right);
}

bool Permissions::hasRight(ObjectActionType action_type) const
{
    return hasRight(Utils::accessForAction(action_type));
}

bool Permissions::hasRight(const EntityCode& entity_code, ObjectActionType action_type) const
{
    return hasRight(entity_code, Utils::accessForAction(action_type));
}

bool Permissions::hasRight(const Uid& entity_uid, const AccessRights& right) const
{
    Z_CHECK(entity_uid.isValid());

    if (_global_rights.contains(AccessFlag::Administrate) || _global_rights.contains(AccessFlag::Developer))
        return true;

    auto it = _entity_rights.find(entity_uid);
    if (it == _entity_rights.end())
        return hasRight(entity_uid.entityCode(), right);

    return it.value().contains(right);
}

bool Permissions::hasRight(const Uid& entity_uid, ObjectActionType action_type) const
{
    return hasRight(entity_uid, Utils::accessForAction(action_type));
}

DatabaseInformation::DatabaseInformation()    
{
}

DatabaseInformation::DatabaseInformation(const DatabaseID& id, const QString& host_original_name, quint16 port, bool ssl,
                                         const QString& database_name)
    : _id(id)
    , _host_original_name(host_original_name)
    , _port(port)
    , _ssl(ssl)
    , _database_name(database_name)
{
    Z_CHECK(_id.isValid());
    Z_CHECK(!_host_original_name.isEmpty());
    Z_CHECK(_port > 0);
}

DatabaseInformation::DatabaseInformation(const DatabaseInformation& di)
    : _id(di._id)
    , _host_original_name(di._host_original_name)
    , _port(di._port)
    , _ssl(di._ssl)
    , _database_name(di._database_name)
{
}

DatabaseInformation& DatabaseInformation::operator=(const DatabaseInformation& di)
{
    _id = di._id;
    _host_original_name = di._host_original_name;
    _port = di._port;
    _ssl = di._ssl;
    _database_name = di._database_name;

    return *this;
}

bool DatabaseInformation::isValid() const
{
    return _id.isValid();
}

QString DatabaseInformation::databaseName() const
{
    QMutexLocker lock(&_database_name_mutex);

    if (_database_name.isEmpty())
        _database_name = hostName();

    return _database_name;
}

DatabaseID DatabaseInformation::id() const
{
    return _id;
}

quint16 DatabaseInformation::port() const
{
    return _port;
}

bool DatabaseInformation::ssl() const
{
    return _ssl;
}

QString DatabaseInformation::hostOriginalName() const
{
    return _host_original_name;
}

QString DatabaseInformation::hostName() const
{
    QMutexLocker lock(&_host_name_mutex);
    return _host_name.isEmpty() ? _host_original_name : _host_name;
}

Error DatabaseInformation::lookup(int timeout_ms)
{
    if (isLookup())
        return Error();

    QEventLoop loop;
    QHostInfo::lookupHost(_host_original_name, &loop, [&loop, this](const QHostInfo& h_info) {
        _lookup_result_mutex.lock();
        _lookup_result = h_info;
        _lookup_result_mutex.unlock();
        loop.exit();
    });

    QTimer timeout;
    bool timeout_error = false;
    timeout.setInterval(timeout_ms);
    timeout.setSingleShot(true);
    timeout.connect(&timeout, &QTimer::timeout, &loop, [&loop, &timeout_error]() {
        timeout_error = true;
        loop.exit();
    });
    timeout.start();

    loop.exec();

    if (timeout_error)
        return Error::timeoutError(_host_original_name);

    QMutexLocker lock(&_lookup_result_mutex);

    if (_lookup_result.error() != QHostInfo::NoError)
        return Error(QStringLiteral("%1: %2").arg(_host_original_name, _lookup_result.errorString()));

    Z_CHECK(!_lookup_result.addresses().isEmpty());

    QStringList h;
    if (_lookup_result.addresses().count() == 1
        && _lookup_result.hostName() == _lookup_result.addresses().constFirst().toString()) {
        h << _lookup_result.hostName();

    } else {
        for (auto& a : _lookup_result.addresses()) {
            h << a.toString();
        }
    }

    lock.unlock();

    _host_name_mutex.lock();
    _host_name = QStringLiteral("%1:%2").arg(h.join(",")).arg(_port);
    _host_name_mutex.unlock();

    return Error();
}

bool DatabaseInformation::isLookup() const
{
    QMutexLocker lock(&_lookup_result_mutex);
    return !_lookup_result.addresses().isEmpty();
}

QHostInfo DatabaseInformation::hostInfo() const
{
    QMutexLocker lock(&_lookup_result_mutex);
    return _lookup_result;
}

bool DatabaseInformation::operator<(const DatabaseInformation& di) const
{
    return _id < di._id;
}

bool DatabaseInformation::operator==(const DatabaseInformation& di) const
{
    return _id == di._id;
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::ProgramFeature& c)
{
    return s << c._code << c._translation_id;
}

QDataStream& operator>>(QDataStream& s, zf::ProgramFeature& c)
{
    return s >> c._code >> c._translation_id;
}

QDataStream& operator<<(QDataStream& s, const zf::ProgramFeatures& c)
{
    return s << c._features << c._module_features;
}

QDataStream& operator>>(QDataStream& s, zf::ProgramFeatures& c)
{
    return s >> c._features >> c._module_features;
}

QDataStream& operator<<(QDataStream& s, const zf::Permissions& c)
{
    return s << c._global_rights << c._module_rights << c._entity_rights;
}

QDataStream& operator>>(QDataStream& s, zf::Permissions& c)
{
    return s >> c._global_rights >> c._module_rights >> c._entity_rights;
}

QDataStream& operator<<(QDataStream& s, const zf::UserInformation& c)
{
    return s << c._login << c._first_name << c._last_name << c._middle_name << c._gender_id;
}

QDataStream& operator>>(QDataStream& s, zf::UserInformation& c)
{
    return s >> c._login >> c._first_name >> c._last_name >> c._middle_name >> c._gender_id;
}

QDataStream& operator<<(QDataStream& s, const zf::ConnectionInformation& c)
{
    return s << c._is_init << c._host << c._user_info << c._direct_permissions << c._relation_permissions << c._features << c._languages
             << c._warning << c._properties;
}

QDataStream& operator>>(QDataStream& s, zf::ConnectionInformation& c)
{
    return s >> c._is_init >> c._host >> c._user_info >> c._direct_permissions >> c._relation_permissions >> c._features >> c._languages
           >> c._warning >> c._properties;
}

QDataStream& operator<<(QDataStream& s, const zf::DatabaseInformation& c)
{
    return s << c._id << c._port << c._ssl << c._database_name << c._host_original_name << c._host_name
             << c._lookup_result.addresses() << c._lookup_result.hostName() << c._lookup_result.lookupId();
}

QDataStream& operator>>(QDataStream& s, zf::DatabaseInformation& c)
{
    s >> c._id >> c._port >> c._ssl >> c._database_name >> c._host_original_name >> c._host_name;

    QList<QHostAddress> addresses;
    s >> addresses;
    QString hostName;
    s >> hostName;
    int lookupId;
    s >> lookupId;

    if (s.status() != QDataStream::Ok) {
        c = zf::DatabaseInformation();
    } else {
        c._lookup_result = QHostInfo();
        c._lookup_result.setAddresses(addresses);
        c._lookup_result.setHostName(hostName);
        c._lookup_result.setLookupId(lookupId);
    }

    return s;
}
