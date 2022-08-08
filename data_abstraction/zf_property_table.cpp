#include "zf_property_table.h"
#include "zf_core.h"

namespace zf
{
PropertyTable::PropertyTable()
    : ObjectTable(false, false)
{
}

PropertyTable::PropertyTable(const PropertyTable& p)
    : ObjectTable(p)
{
}

PropertyTable::PropertyTable(const DataPropertyList& properties, bool ignore_set_error)
    : ObjectTable(ignore_set_error, false)
{
    Z_CHECK(!properties.isEmpty());
    addProperties(properties);

    EntityCode code;
    for (auto& p : properties) {
        if (!code.isValid())
            code = p.entityCode();
        else
            Z_CHECK(code == p.entityCode());
    }
}

PropertyTable::PropertyTable(const EntityCode& entity_code, const PropertyIDList& property_codes, bool ignore_set_error)
    : ObjectTable(ignore_set_error, false)
{
    Z_CHECK(!property_codes.isEmpty());
    for (auto& id : property_codes) {
        addProperties({DataProperty::property(entity_code, id)});
    }
}

bool PropertyTable::isValid() const
{
    return !properties().isEmpty();
}

EntityCode PropertyTable::entityCode() const
{
    Z_CHECK(isValid());
    return properties().at(0).entityCode();
}

bool PropertyTable::containsProperty(const PropertyID& property_id) const
{
    return containsProperty(DataProperty::property(entityCode(), property_id, false));
}

QVariant PropertyTable::value(const EntityID& id, const PropertyID& property_id, QLocale::Language language) const
{
    return value(id, DataProperty::property(entityCode(), property_id), language);
}

LanguageMap PropertyTable::valueLanguages(const EntityID& id, const PropertyID& property_id) const
{
    return valueLanguages(id, DataProperty::property(entityCode(), property_id));
}

void PropertyTable::setValue(const EntityID& id, const PropertyID& property_id, const QVariant& value, int order,
                             QLocale::Language language)
{
    setValue(id, DataProperty::property(entityCode(), property_id), value, order, language);
}

void PropertyTable::setValue(const EntityID& id, const PropertyID& property_id, const LanguageMap& value, int order)
{
    setValue(id, DataProperty::property(entityCode(), property_id), value, order);
}

QList<EntityID> PropertyTable::findEntity(const PropertyID& property_id, const QVariant& value, bool sort) const
{
    return findEntity(DataProperty::property(entityCode(), property_id), value, sort);
}

void PropertyTable::debug() const
{
    EntityIDList objects = entitiesSorted();
    for (auto& o : qAsConst(objects)) {
        QStringList values;
        for (auto& p : properties()) {
            values << QString("%1(%2)").arg(p.id().value()).arg(Utils::variantToString(value(o, p)));
        }
        qDebug() << "=>>> id:" << o.value();
        qDebug() << values.join(" | ");
    }
}

IntTable::IntTable()
    : ObjectTable<int, int>(true, true)
{
}

IntTable::IntTable(const IntTable& p)
    : ObjectTable<int, int>(p)
{
}

} // namespace zf
