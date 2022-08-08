#include "zf_property_restriction.h"
#include "zf_core.h"

namespace zf
{
int PropertyRestriction::_meta_type_number = 0;

struct PropertyRestrictionInfo
{
    //! Значения
    QVariantList values;
    //! Условие
    ConditionType type = ConditionType::Undefined;
};
typedef std::shared_ptr<PropertyRestrictionInfo> PropertyRestrictionInfoPtr;

//! Разделяемые данные для PropertyRestriction
class PropertyRestriction_Data : public QSharedData
{
public:
    PropertyRestriction_Data();
    PropertyRestriction_Data(const DataProperty& entity);
    PropertyRestriction_Data(const PropertyRestriction_Data& d);
    ~PropertyRestriction_Data();

    void copyFrom(const PropertyRestriction_Data* d);

    static PropertyRestriction_Data* shared_null();

    void clear();

    //! Свойство - сущность
    DataProperty entity;

    //! Информация о свойстве
    mutable QMap<DataProperty, PropertyRestrictionInfoPtr> info;
    //! Ключи
    QSet<int> keys;
};

Q_GLOBAL_STATIC(PropertyRestriction_Data, nullResult)
PropertyRestriction_Data* PropertyRestriction_Data::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0)
        res->ref.ref(); // чтобы не было удаления самого nullResult
    return res;
}

PropertyRestriction_Data::PropertyRestriction_Data()
{
}

PropertyRestriction_Data::PropertyRestriction_Data(const DataProperty& _entity)
    : entity(_entity)
{
}

PropertyRestriction_Data::PropertyRestriction_Data(const PropertyRestriction_Data& d)
    : QSharedData(d)
{
    copyFrom(&d);
}

PropertyRestriction_Data::~PropertyRestriction_Data()
{
}

void PropertyRestriction_Data::copyFrom(const PropertyRestriction_Data* d)
{
    entity = d->entity;
    info.clear();
    for (auto i = d->info.constBegin(); i != d->info.constEnd(); ++i) {
        auto val = Z_MAKE_SHARED(PropertyRestrictionInfo);
        val->type = i.value()->type;
        val->values = i.value()->values;
        info[i.key()] = val;
    }
    keys = d->keys;
}

void PropertyRestriction_Data::clear()
{
    entity.clear();
    info.clear();
    keys.clear();
}

PropertyRestriction::PropertyRestriction()
    : _d(PropertyRestriction_Data::shared_null())
{
}

PropertyRestriction::PropertyRestriction(const DataProperty& entity)
    : _d(new PropertyRestriction_Data)

{
    Z_CHECK(entity.isValid());
    _d->entity = entity;
}

PropertyRestriction::PropertyRestriction(const EntityCode& entity_code)
    : _d(new PropertyRestriction_Data)
{
    Z_CHECK(entity_code.isValid());
    auto props = DataProperty::propertiesByType(entity_code, PropertyType::Entity);
    Z_CHECK(props.count() <= 1);
    if (props.isEmpty())
        Z_HALT(QString("Entity %1 not found").arg(entity_code.value()));
    Z_CHECK(props.constFirst().propertyType() == PropertyType::Entity);
    _d->entity = props.constFirst();
}

PropertyRestriction::PropertyRestriction(const PropertyRestriction& r)
    : _d(r._d)
{
}

PropertyRestriction::~PropertyRestriction()
{
}

PropertyRestriction& PropertyRestriction::operator=(const PropertyRestriction& r)
{
    if (this != &r)
        _d = r._d;
    return *this;
}

bool PropertyRestriction::isValid() const
{
    return _d->entity.isValid();
}

void PropertyRestriction::add(const DataProperty& property, const QVariantList& values, ConditionType type)
{
    Z_CHECK(isValid());
    Z_CHECK(property.isValid());
    Z_CHECK(type == ConditionType::And || type == ConditionType::Or);
    Z_CHECK(!values.isEmpty());

    auto info = this->info(property, false);
    if (info->type != ConditionType::Undefined)
        Z_CHECK(info->type == type);
    else
        info->type = type;

    info->values << values;
}

void PropertyRestriction::add(const PropertyID& property_id, const QVariantList& values, ConditionType type)
{
    add(DataStructure::structure(_d->entity)->property(property_id), values, type);
}

void PropertyRestriction::addKeys(const QList<int>& keys)
{
    _d->keys.unite(keys.toSet());
}

bool PropertyRestriction::contains(const DataProperty& property) const
{
    Z_CHECK(isValid());
    Z_CHECK(property.isValid());
    return _d->info.contains(property);
}

bool PropertyRestriction::contains(const PropertyID& property_id) const
{
    Z_CHECK(isValid());
    Z_CHECK(property_id.isValid());
    auto p = DataStructure::structure(_d->entity)->property(property_id, false);
    if (!p.isValid())
        return false;

    return contains(p);
}

QVariantList PropertyRestriction::values(const DataProperty& property) const
{
    Z_CHECK(isValid());
    Z_CHECK(property.isValid());
    return info(property, true)->values;
}

QVariantList PropertyRestriction::values(const PropertyID& property_id) const
{
    Z_CHECK(isValid());
    return values(DataStructure::structure(_d->entity)->property(property_id));
}

QSet<int> PropertyRestriction::keys() const
{
    return _d->keys;
}

ConditionType PropertyRestriction::type(const DataProperty& property) const
{
    Z_CHECK(isValid());
    Z_CHECK(property.isValid());
    return info(property, true)->type;
}

ConditionType PropertyRestriction::type(const PropertyID& property_id) const
{
    Z_CHECK(isValid());
    return type(DataStructure::structure(_d->entity)->property(property_id));
}

DataPropertyList PropertyRestriction::properties() const
{
    return _d->info.keys();
}

QVariant PropertyRestriction::variant() const
{
    return QVariant::fromValue(*this);
}

PropertyRestriction PropertyRestriction::fromVariant(const QVariant& v)
{
    return v.value<PropertyRestriction>();
}

bool PropertyRestriction::isVariant(const QVariant& v)
{
    return v.isValid() && (v.userType() == _meta_type_number);
}

PropertyRestrictionInfo* PropertyRestriction::info(const DataProperty& property, bool halt_if_not_found) const
{
    auto res = _d->info.find(property);
    PropertyRestrictionInfoPtr info;

    if (res == _d->info.end()) {
        if (halt_if_not_found)
            Z_HALT(QString("property not found %1:%2").arg(_d->entity.id().value()).arg(property.id().value()));

        Z_CHECK(DataStructure::structure(_d->entity)->contains(property));
        info = Z_MAKE_SHARED(PropertyRestrictionInfo);
        // это должно вызвать клонирование данных
        const_cast<PropertyRestriction*>(this)->_d->info[property] = info;

    } else {
        info = res.value();
    }

    return info.get();
}

} // namespace zf
