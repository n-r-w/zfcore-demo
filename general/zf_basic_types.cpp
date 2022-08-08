#include "zf_basic_types.h"
#include "zf_core.h"
#include <limits>

namespace zf
{
int InvalidValue::_user_type = -1;

ID_Wrapper::ID_Wrapper()
    : _id(0)
{
}

ID_Wrapper::ID_Wrapper(int id)
    : _id(id)
{    
}

ID_Wrapper::ID_Wrapper(const ID_Wrapper& d)
    : _id(d._id)
{
}

ID_Wrapper::~ID_Wrapper()
{
}

ID_Wrapper& ID_Wrapper::operator=(const ID_Wrapper& d)
{
    _id = d._id;
    return *this;
}

bool ID_Wrapper::operator==(const ID_Wrapper& d) const
{
    return _id == d._id;
}

bool ID_Wrapper::operator!=(const ID_Wrapper& d) const
{
    return _id != d._id;
}

bool ID_Wrapper::operator<(const ID_Wrapper& d) const
{
    return _id < d._id;
}

bool ID_Wrapper::operator<=(const ID_Wrapper& d) const
{
    return _id <= d._id;
}

bool ID_Wrapper::operator>(const ID_Wrapper& d) const
{
    return _id > d._id;
}

bool ID_Wrapper::operator>=(const ID_Wrapper& d) const
{
    return _id >= d._id;
}

ID_Wrapper::operator QVariant() const
{
    return _id;
}

bool ID_Wrapper::isValid() const
{
    return value() != 0;
}

int ID_Wrapper::value() const
{
    return _id;
}

QString ID_Wrapper::string() const
{
    return QString::number(value());
}

void ID_Wrapper::clear()
{
    _id = 0;
}

DatabaseID::DatabaseID()
{
}

DatabaseID::DatabaseID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > Consts::INVALID_DATABASE_ID && id <= Consts::MAX_DATABASE_ID);
}

QVariant DatabaseID::variant() const
{
    return QVariant::fromValue(*this);
}

DatabaseID DatabaseID::fromVariant(const QVariant& v)
{
    return v.value<DatabaseID>();
}

EntityCode::EntityCode()
{
}

EntityCode::EntityCode(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > Consts::INVALID_ENTITY_CODE && id < Consts::MAX_ENTITY_CODE);
}

QVariant EntityCode::variant() const
{
    return QVariant::fromValue(*this);
}

EntityCode EntityCode::fromVariant(const QVariant& v)
{
    return v.value<EntityCode>();
}

EntityID::EntityID()
{
}

EntityID::EntityID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > Consts::INVALID_ENTITY_ID);
}

QVariant EntityID::variant() const
{
    return QVariant::fromValue(*this);
}

EntityID EntityID::fromVariant(const QVariant& v)
{
    return v.value<EntityID>();
}

PropertyID::PropertyID()
{
}

PropertyID::PropertyID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id >= Consts::MINUMUM_PROPERTY_ID);
}

PropertyID PropertyID::def()
{
    return PropertyID(Consts::MINUMUM_PROPERTY_ID);
}

QVariant PropertyID::variant() const
{
    return QVariant::fromValue(*this);
}

PropertyID PropertyID::fromVariant(const QVariant& v)
{
    return v.value<PropertyID>();
}

OperationID::OperationID()
{
}

OperationID::OperationID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > 0);
}

QVariant OperationID::variant() const
{
    return QVariant::fromValue(*this);
}

OperationID OperationID::fromVariant(const QVariant& v)
{
    return v.value<OperationID>();
}

bool operator==(const ID_Wrapper& w1, int w2)
{
    return w1.value() == w2;
}

bool operator!=(const ID_Wrapper& w1, int w2)
{
    return w1.value() != w2;
}

bool operator==(int w1, const ID_Wrapper& w2)
{
    return w2.value() == w1;
}

bool operator!=(int w1, const ID_Wrapper& w2)
{
    return w2.value() != w1;
}

Command::Command()
{
}

Command::Command(int id)
    : ID_Wrapper(id)
{
}

QVariant Command::variant() const
{
    return QVariant::fromValue(*this);
}

Command Command::fromVariant(const QVariant& v)
{
    return v.value<Command>();
}

InvalidValue::InvalidValue()
{
}

InvalidValue::InvalidValue(const InvalidValue& v)
{
    if (v.isValid())
        _value = std::make_unique<QVariant>(v.value());
}

InvalidValue::InvalidValue(const QVariant& value)
    : _value(std::make_unique<QVariant>(value))
{
}

InvalidValue::~InvalidValue()
{
}

bool InvalidValue::isValid() const
{
    return _value != nullptr;
}

void InvalidValue::clear()
{
    _value.reset();
}

QVariant InvalidValue::value() const
{
    return _value ? *_value : QVariant();
}

QVariant InvalidValue::variant() const
{
    return QVariant::fromValue(*this);
}

QString InvalidValue::string() const
{
    return isValid() ? Utils::variantToString(*_value) : QString();
}

InvalidValue InvalidValue::fromVariant(const QVariant& v)
{
    if (isInvalidValueVariant(v))
        return v.value<InvalidValue>();
    else
        return InvalidValue();
}

bool InvalidValue::isInvalidValueVariant(const QVariant& v)
{
    return v.isValid() && v.userType() == _user_type;
}

bool InvalidValue::operator==(const InvalidValue& v) const
{
    if (!isValid() && !v.isValid())
        return true;

    if (isValid() != v.isValid())
        return false;

    return Utils::compareVariant(value(), v.value());
}

bool InvalidValue::operator!=(const InvalidValue& v) const
{
    return !operator==(v);
}

InvalidValue& InvalidValue::operator=(const InvalidValue& v)
{
    if (!v.isValid())
        clear();
    else
        _value = std::make_unique<QVariant>(v.value());

    return *this;
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::ID_Wrapper& c)
{
    return s << c.value();
}

QDataStream& operator>>(QDataStream& s, zf::ID_Wrapper& c)
{
    int id;
    s >> id;
    if (s.status() == QDataStream::Ok)
        c._id = id;
    else
        c._id = 0;
    return s;
}

QDebug operator<<(QDebug debug, const zf::ID_Wrapper& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << c.value();

    return debug;
}

QDebug operator<<(QDebug debug, const zf::InvalidValue& c)
{
    Q_UNUSED(c);

    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    if (c.isValid())
        debug << "invalid: " << c.value();
    else
        debug << "invalid";

    return debug;
}

QDataStream& operator<<(QDataStream& s, const zf::InvalidValue& v)
{
    s << QStringLiteral("_zf_inv_") << v.isValid();
    if (v.isValid())
        s << v.value();
    return s;
}

QDataStream& operator>>(QDataStream& s, zf::InvalidValue& v)
{
    QString type;
    s >> type;
    if (type != QStringLiteral("_zf_inv_")) {
        v.clear();
        return s;
    }

    bool is_valid;
    s >> is_valid;
    if (!is_valid) {
        v.clear();
        return s;
    }

    QVariant value;
    s >> value;

    v = zf::InvalidValue(value);
    return s;
}
