#pragma once

#include "zf_data_container.h"
#include "zf_utils.h"

namespace zf
{
template <typename Entity, typename Property> class ObjectTable;
}

template <typename T1, typename T2> QDataStream& operator<<(QDataStream& out, const zf::ObjectTable<T1, T2>& obj);
template <typename T1, typename T2> QDataStream& operator>>(QDataStream& in, zf::ObjectTable<T1, T2>& obj);

namespace zf
{
inline QString zfUniqueKey(int value)
{
    return QString::number(value);
}

inline QString zfUniqueKey(const EntityID& value)
{
    return QString::number(value.value());
}

inline QString zfUniqueKey(const DataProperty& value)
{
    return value.uniqueKey();
}

/*! Шаблонный класс для таблиц свойств
 * Для классов, участвующих в шаблоне должна быть определена функция
 * QString zfUniqueKey(const T& value);
 * которая должна вернуть уникальный ключ для значения
 */
template <typename Entity, typename Property> class ObjectTable
{
public:
    ObjectTable(const ObjectTable& p)
        : _properties(p._properties)
        , _entities(p._entities)
        , _data(p._data)
        , _ignore_set_error(p._ignore_set_error)
        , _auto_add_properties(p._auto_add_properties)
    {
    }

    ~ObjectTable() { clearCache(); }

    ObjectTable& operator=(const ObjectTable& t)
    {
        _properties = t._properties;
        _entities = t._entities;
        _data = t._data;
        _ignore_set_error = t._ignore_set_error;
        _auto_add_properties = t._auto_add_properties;
        return *this;
    }

    //! Нет данных
    bool isEmpty() const { return _entities.isEmpty(); }

    //! Количество объектов (строки)
    int entityCount() const { return _entities.count(); }
    //! Количество свойств (колонки)
    int propertyCount() const { return _properties.count(); }

    //! Свойства
    QList<Property> properties() const { return _properties; }
    //! Объекты. Значение - порядок сортировки
    const QHash<Entity, int>& entities() const { return _entities; }
    //! Не сортированные объекты
    QList<Entity> entitiesUnsorted() const { return _entities.keys(); }

    //! Отсортированные объекты (если сортировка была при добавлении)
    QList<Entity> entitiesSorted(Qt::SortOrder order = Qt::AscendingOrder) const
    {
        QList<Entity> res(_entities.keys());
        std::sort(res.begin(), res.end(), [&](const Entity& e1, const Entity& e2) -> bool {
            int order1 = _entities.value(e1);
            int order2 = _entities.value(e2);
            if (order1 >= 0 && order2 >= 0)
                return (order == Qt::AscendingOrder) ? order1 < order2 : order1 > order2;

            if (order1 < 0 && order2 < 0)
                return (order == Qt::AscendingOrder) ? e1 < e2 : e1 > e2;

            if (order1 >= 0)
                return (order == Qt::AscendingOrder) ? true : false;

            return (order == Qt::AscendingOrder) ? false : true; // order2 >= 0
        });
        return res;
    }

    //! Содержит ли такие свойства
    bool containsProperty(const Property& property) const { return _properties.contains(property); }
    //! Содержит ли такие свойства
    bool containsEntity(const Entity& entity) const { return _entities.contains(entity); }

    //! Получить значение свойства
    QVariant value(const Entity& id, const Property& property, QLocale::Language language = QLocale::AnyLanguage) const
    {
        return Utils::valueToLanguage(valueLanguages(id, property), language);
    }
    //! Получить первое попавшееся свойство. Полезно если известно, что имеется всего одна запись.
    QVariant firstValue(const Property& property, QLocale::Language language = QLocale::AnyLanguage) const
    {
        Q_ASSERT(!_entities.isEmpty());
        return value(_entities.constBegin().key(), property, language);
    }

    //! Получить значение свойства на всех языках
    LanguageMap valueLanguages(const Entity& id, const Property& property) const { return getDataHelper(id, property); }

    //! Получить все значения свойств сущности
    QMap<Property, QVariant> values(const Entity& id, QLocale::Language language = QLocale::AnyLanguage) const
    {
        QMap<Property, QVariant> res;
        for (auto& p : _properties) {
            res[p] = value(id, p, language);
        }
        return res;
    }

    //! Получить все свойства сущности на всех языках
    QMap<Property, LanguageMap> valuesLanguages(const Entity& id) const
    {
        QMap<Property, LanguageMap> res;
        for (auto& p : _properties) {
            res[p] = valueLanguages(id, p);
        }
        return res;
    }

    //! Найти объекты по комбинации значений свойств
    QList<Entity> findEntity(
        //! Комбинация значений свойств
        const QMap<Property, QVariant>& values,
        //! Сортировка результата по ID
        bool sort = false) const
    {
        createCache();

        QSet<Entity> res;
        bool first_init = true;
        for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
            QList<Entity> objects = _find_hash.values(hashKey(it.key(), it.value()));

            if (first_init) {
                res = Utils::toSet(objects);
                first_init = false;

            } else {
                res.intersect(Utils::toSet(objects));
            }
        }

        QList<Entity> list = res.values();
        if (sort)
            std::sort(list.begin(), list.end());
        return list;
    }

    QList<Entity> findEntity(
        //! Свойство
        const Property& property,
        //! Значение
        const QVariant& value,
        //! Сортировка результата по ID
        bool sort = false) const
    {
        return findEntity(QMap<Property, QVariant> {{property, value}}, sort);
    }

    //! Установить значение свойства
    void setValue(const Entity& id, const Property& property, const QVariant& value, int order = -1,
                  QLocale::Language language = QLocale::AnyLanguage)
    {
        QString key = dataKey(id, property, _ignore_set_error);
        if (key.isEmpty()) {
            if (!_auto_add_properties && _ignore_set_error)
                return;
            if (_auto_add_properties) {
                addProperties({property});
                key = dataKey(id, property, false);
            }
        }

        auto it = _data.find(key);
        if (it == _data.end()) {
            _data[key] = LanguageMap({{language, value.isNull() ? QVariant() : value}});
        } else {
            it.value().insert(language, value.isNull() ? QVariant() : value);
        }

        _entities.insert(id, order);
        clearCache();
    }

    //! Установить значение свойства на всех языках
    void setValue(const Entity& id, const Property& property, const LanguageMap& value, int order = -1)
    {
        QString key = dataKey(id, property, _ignore_set_error);
        if (key.isEmpty()) {
            if (_ignore_set_error)
                return;
            if (_auto_add_properties) {
                addProperties({property});
                key = dataKey(id, property, false);
            }
        }

        auto it = _data.find(key);
        if (it == _data.end()) {
            _data[key] = value;
        } else {
            it.value() = value;
        }

        _entities.insert(id, order);
        clearCache();
    }

protected:
    ObjectTable(
        //! Игнорировать попытку записи в несуществующие свойства
        bool ignore_set_error,
        //! Автоматически добавлять несуществующие свойства
        bool auto_add_properties)
        : _ignore_set_error(ignore_set_error)
        , _auto_add_properties(auto_add_properties)
    {
    }

    void addProperties(const QList<Property>& p) { _properties << p; }

private:
    QString dataKey(const Entity& id, const Property& property, bool ignore_error) const
    {        
        if (ignore_error) {
            if (!_properties.contains(property))
                return QString();
        } else {
            Z_CHECK(_properties.contains(property));
        }
        return zfUniqueKey(id) + Consts::KEY_SEPARATOR + zfUniqueKey(property);
    }

    QString hashKey(const Property& property, const QVariant& value) const
    {
        return zfUniqueKey(property) + Consts::KEY_SEPARATOR + value.toString();
    }

    void clearCache() { _find_hash.clear(); }

    void createCache() const
    {
        if (_data.isEmpty() || !_find_hash.isEmpty())
            return;

        for (auto ent = _entities.constBegin(); ent != _entities.constEnd(); ++ent) {
            QMap<Property, QVariant> values_by_id = this->values(ent.key());
            for (auto it = values_by_id.constBegin(); it != values_by_id.constEnd(); ++it) {
                _find_hash.insert(hashKey(it.key(), it.value()), ent.key());
            }
        }
    }

    const LanguageMap& getDataHelper(const Entity& id, const Property& property) const
    {
        auto it = _data.find(dataKey(id, property, _ignore_set_error));
        return (it == _data.end()) ? _null_value : it.value();
    }

    //! Список свойств (колонки)
    QList<Property> _properties;
    //! Список идентификаторов объектов. Значение - порядок сортировки
    QHash<Entity, int> _entities;
    //! Данные
    QHash<QString, LanguageMap> _data;

    //! Результаты поиска объектов
    mutable QMultiHash<QString, Entity> _find_hash;
    //! Игнорировать попытку записи в несуществующие свойства
    bool _ignore_set_error = false;
    //! Автоматически добавлять несуществующие свойства
    bool _auto_add_properties = false;
    //! Пустое значение;
    LanguageMap _null_value;

    template <typename T1, typename T2> friend QDataStream& ::operator<<(QDataStream& out, const zf::ObjectTable<T1, T2>& obj);
    template <typename T1, typename T2> friend QDataStream& ::operator>>(QDataStream& in, zf::ObjectTable<T1, T2>& obj);
};

//! Таблица, где в качестве id объектов и их свойств, используются целые числа (сами данные QVariant)
class ZCORESHARED_EXPORT IntTable : public ObjectTable<int, int>
{
public:
    IntTable();
    IntTable(const IntTable& p);
    IntTable& operator=(const IntTable& p) = default;
};

//! Таблица свойств сущности. По вертикали - коды сущностей, по горизонтали - свойства
class ZCORESHARED_EXPORT PropertyTable : public ObjectTable<zf::EntityID, zf::DataProperty>
{
public:
    PropertyTable();
    PropertyTable(const PropertyTable& p);
    //! Все свойства должны относиться к одной сущности
    PropertyTable(const DataPropertyList& properties,
                  //! Игнорировать попытку записи в несуществующие свойства
                  bool ignore_set_error = false);
    PropertyTable(
        //! Вид сущности
        const EntityCode& entity_code,
        //! id свойств
        const PropertyIDList& property_codes,
        //! Игнорировать попытку записи в несуществующие свойства
        bool ignore_set_error = false);

    bool isValid() const;

    //! Код сущности
    EntityCode entityCode() const;

    //! Содержит ли такие свойства
    bool containsProperty(const PropertyID& property_id) const;
    using ObjectTable::containsProperty;

    //! Получить значение свойства
    QVariant value(const EntityID& id, const PropertyID& property_id, QLocale::Language language = QLocale::AnyLanguage) const;
    using ObjectTable::value;

    //! Получить значение свойства на всех языках
    LanguageMap valueLanguages(const EntityID& id, const PropertyID& property_id) const;
    using ObjectTable::valueLanguages;

    //! Установить значение свойства
    void setValue(const EntityID& id, const PropertyID& property_id, const QVariant& value, int order = -1,
                  QLocale::Language language = QLocale::AnyLanguage);
    //! Установить значение свойства на всех языках
    void setValue(const EntityID& id, const PropertyID& property_id, const LanguageMap& value, int order = -1);
    using ObjectTable::setValue;

    //! Поиск объекты по значению свойства
    QList<zf::EntityID> findEntity(
        //! Свойство
        const PropertyID& property_id,
        //! Значение
        const QVariant& value,
        //! Сортировка результата по ID
        bool sort = false) const;
    using ObjectTable::findEntity;

    //! Вывести содержимое в qDebug
    void debug() const;    
};
} // namespace zf

Q_DECLARE_METATYPE(zf::PropertyTable)
Q_DECLARE_METATYPE(zf::IntTable)

template <typename T1, typename T2> inline QDataStream& operator<<(QDataStream& out, const zf::ObjectTable<T1, T2>& obj)
{
    return out << obj._properties << obj._entities << obj._data;
}
template <typename T1, typename T2> inline QDataStream& operator>>(QDataStream& in, zf::ObjectTable<T1, T2>& obj)
{
    obj._find_hash.clear();
    return in >> obj._properties >> obj._entities >> obj._data;
}
