#pragma once

#include "zf_basic_types.h"
#include "zf_defs.h"
#include <QSharedDataPointer>

namespace zf
{
class DataProperty;
class PropertyRestriction_Data;

struct PropertyRestrictionInfo;

//! Ограничения на выборку по полям структуры данных
class ZCORESHARED_EXPORT PropertyRestriction
{
public:
    PropertyRestriction();
    PropertyRestriction(
        //! Свойство - сущность
        const DataProperty& entity);
    PropertyRestriction(
        //! Код сущности сущность
        const EntityCode& entity_code);
    PropertyRestriction(const PropertyRestriction& r);
    ~PropertyRestriction();

    PropertyRestriction& operator=(const PropertyRestriction& r);

    bool isValid() const;

    //! Добавить ограничение на допустимые значения для свойства
    //! Нельзя смешивать ConditionType::And и ConditionType::Or в рамках одного PropertyRestriction
    void add(const DataProperty& property,
             //! Список допустимых значений
             const QVariantList& values,
             //! Условие. Допустимо только ConditionType::And, ConditionType::Or
             ConditionType type = ConditionType::And);
    //! Добавить ограничение на допустимые значения для свойства
    //! Нельзя смешивать ConditionType::And и ConditionType::Or в рамках одного PropertyRestriction
    void add(const PropertyID& property_id,
             //! Список допустимых значений
             const QVariantList& values,
             //! Условие. Допустимо только ConditionType::And, ConditionType::Or
             ConditionType type = ConditionType::And);
    //! Добавить ограничение по ключевому полю. Можно использовать совместно с обычным add
    void addKeys(const QList<int>& keys);

    //! Содержит свойство
    bool contains(const DataProperty& property) const;
    //! Содержит свойство
    bool contains(const PropertyID& property_id) const;

    //! Допустимые значения для свойства
    QVariantList values(const DataProperty& property) const;
    //! Допустимые значения для свойства
    QVariantList values(const PropertyID& property_id) const;

    //! Допустимые ключевые значения
    QSet<int> keys() const;

    //! Условие выборки
    ConditionType type(const DataProperty& property) const;
    //! Условие выборки
    ConditionType type(const PropertyID& property_id) const;

    //! Список всех свойств
    DataPropertyList properties() const;

    //! Преобразование в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static PropertyRestriction fromVariant(const QVariant& v);
    //! Проверка хранит ли данный QVariant PropertyRestriction
    static bool isVariant(const QVariant& v);

private:
    //! Данные
    QSharedDataPointer<PropertyRestriction_Data> _d;
    //! Номер типа данных при регистрации через qRegisterMetaType
    static int _meta_type_number;

    PropertyRestrictionInfo* info(const DataProperty& property, bool halt_if_not_found) const;

    friend class Framework;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::PropertyRestriction)
