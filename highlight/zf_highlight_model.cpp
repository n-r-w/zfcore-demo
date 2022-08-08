#include "zf_highlight_model.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_translation.h"

#include <QMetaEnum>

namespace zf
{
//! Разделяемые данные для HighlightItem
class HighlightItem_data : public QSharedData
{
public:
    HighlightItem_data();
    HighlightItem_data(const HighlightItem_data& d);
    ~HighlightItem_data();

    void copyFrom(const HighlightItem_data* d);

    //! Вид ошибки
    InformationType type = InformationType::Invalid;
    //! Свойство, к которому относится ошибка
    DataProperty property;
    //! Код ошибки (должен быть уникален в рамках property)
    int id = -1;
    //! Текст ошибки
    QString text;
    //! Код группы для логического объединения разных ошибок в группу
    int group_code = -1;
    //! Произвольные данные
    QVariant data;
    //! Уникальный ключ
    QString key;
    //! Ключ для qHash
    uint hash_key = 0;
    //! Параметры
    HighlightOptions options;
};

HighlightItem_data::HighlightItem_data()
{
}

HighlightItem_data::HighlightItem_data(const HighlightItem_data& d)
    : QSharedData(d)
{
    copyFrom(&d);
}

HighlightItem_data::~HighlightItem_data()
{
}

void HighlightItem_data::copyFrom(const HighlightItem_data* d)
{
    type = d->type;
    property = d->property;
    id = d->id;
    text = d->text;
    group_code = d->group_code;
    data = d->data;
    key = d->key;
    hash_key = d->hash_key;
    options = d->options;
}

HighlightModel::HighlightModel(QObject* parent)
    : QObject(parent)
{
}

void HighlightModel::clear()
{
    beginUpdate();
    _items.clear();
    _item_properties.clear();
    _item_groups.clear();
    _item_types.clear();
    _item_property_types.clear();
    _ordered.clear();
    _items_options_hash.clear();
    endUpdate();
}

int HighlightModel::count(const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return _items.count();
    return itemsByOptions(options).count();
}

int HighlightModel::count(InformationTypes types, const HighlightOptions& options) const
{
    return items(types, false, options).count();
}

int HighlightModel::groupCount(int groupCode, const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return _item_groups.count(groupCode);

    int count = 0;
    auto items = _item_groups.values(groupCode);
    for (auto& i : qAsConst(items)) {
        if ((i.options() & options) == options)
            count++;
    }
    return count;
}

int HighlightModel::countWithoutOptions(const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return count();

    return count() - count(options);
}

int HighlightModel::countWithoutOptions(InformationTypes types, const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return count(types);

    return count(types) - count(types, options);
}

int HighlightModel::groupCountWithoutOptions(int groupCode, const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return groupCount(groupCode);

    return groupCount(groupCode) - groupCount(groupCode, options);
}

bool HighlightModel::hasErrors(const HighlightOptions& options) const
{
    return count(InformationType::Error, options) > 0;
}

bool HighlightModel::hasWarnings(const HighlightOptions& options) const
{
    return count(InformationType::Error | InformationType::Warning, options) > 0;
}

bool HighlightModel::isEmpty() const
{
    return count() == 0;
}

bool HighlightModel::hasErrorsWithoutOptions(const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return hasErrors(options);

    return count(InformationType::Error) > count(InformationType::Error, options);
}

bool HighlightModel::hasWarningsWithoutOptions(const HighlightOptions& options) const
{
    if (options == HighlightOptions())
        return hasWarnings(options);

    return count(InformationType::Warning) > count(InformationType::Warning, options);
}

bool HighlightModel::contains(const HighlightItem& item) const
{
    if (!item.isValid())
        return false;

    return contains(item.property(), item.id());
}

bool HighlightModel::contains(const DataProperty& property) const
{
    return !items(property, false).isEmpty();
}

bool HighlightModel::contains(const DataProperty& property, int id) const
{
    return _items.contains(HighlightItem::createKey(property, id));
}

bool HighlightModel::contains(const DataProperty& property, InformationType type, const HighlightOptions& options) const
{
    auto found = items(property, false);
    for (auto& i : found) {
        if (i.type() == type && (options == HighlightOptions() || (i.options() & options) == options))
            return true;
    }
    return false;
}

HighlightItem HighlightModel::add(InformationType type, const DataProperty& property, int id, const QString& text, int group_code,
                                  const QVariant& data, const HighlightOptions& options)
{
    HighlightItem item = HighlightItem(type, property, id, text, group_code, data, options);
    add(item);
    return item;
}

void HighlightModel::add(const HighlightItem& item)
{
    if (contains(item)) {
        change(item);
        return;
    }

    addHelper(item, true);
}

void HighlightModel::add(const QList<HighlightItem>& items)
{
    for (auto& i : items) {
        add(i);
    }
}

void HighlightModel::add(const QSet<HighlightItem>& items)
{
    add(items.values());
}

void HighlightModel::remove(const HighlightItem& item)
{
    removeHelper(item, true);
}

void HighlightModel::remove(const DataProperty& property)
{
    remove(_item_properties.values(property));
    removeChildProperties(property, {});
}

void HighlightModel::remove(const DataProperty& property, int id, const DataPropertySet& ignored)
{
    remove(_items.value(HighlightItem::createKey(property, id)));

    if (!_item_properties.contains(property)) {
        // таких свойст уже не осталось, удаляем дочерние
        removeChildProperties(property, ignored);
    }
}

void HighlightModel::remove(const QList<HighlightItem>& items)
{
    for (auto& item : items) {
        remove(item);
    }
}

void HighlightModel::remove(const QSet<HighlightItem>& items)
{
    remove(items.values());
}

void HighlightModel::change(const HighlightItem& item)
{
    if (!item.isValid())
        return;

    if (!contains(item)) {
        add(item);
        return;
    }

    removeHelper(item, false);
    addHelper(item, false);

    if (_update_counter == 0)
        emit sg_itemChanged(item);
}

bool HighlightModel::hasGroup(int group_code, const HighlightOptions& options) const
{
    return !groupItems(group_code, false, options).isEmpty();
}

HighlightItem HighlightModel::item(int pos) const
{
    Z_CHECK(pos >= 0 && pos < count());

    if (_ordered.isEmpty()) {
        _ordered = _items.values();
        sortHelper(_ordered);
    }

    return _ordered.at(pos);
}

QList<HighlightItem> HighlightModel::items(const DataProperty& property, bool sort, const HighlightOptions& options) const
{
    QList<HighlightItem> res = _item_properties.values(property);

    if (options != HighlightOptions()) {
        for (int i = res.count() - 1; i >= 0; i--) {
            if ((res.at(i).options() & options) != options)
                res.removeAt(i);
        }
    }

    if (sort)
        sortHelper(res);
    return res;
}

QList<HighlightItem> HighlightModel::items(const InformationTypes& types, bool sort, const HighlightOptions& options) const
{
    QList<HighlightItem> res;

    if (types & InformationType::Success)
        res << _item_types.values(InformationType::Success);
    if (types & InformationType::Information)
        res << _item_types.values(InformationType::Information);
    if (types & InformationType::Warning)
        res << _item_types.values(InformationType::Warning);
    if (types & InformationType::Error)
        res << _item_types.values(InformationType::Error);
    if (types & InformationType::Critical)
        res << _item_types.values(InformationType::Critical);
    if (types & InformationType::Required)
        res << _item_types.values(InformationType::Required);
    if (types & InformationType::Fatal)
        res << _item_types.values(InformationType::Fatal);

    if (options != HighlightOptions()) {
        for (int i = res.count() - 1; i >= 0; i--) {
            if ((res.at(i).options() & options) != options)
                res.removeAt(i);
        }
    }

    if (sort)
        sortHelper(res);

    return res;
}

QList<HighlightItem> HighlightModel::items(InformationType type, bool sort, const HighlightOptions& options) const
{
    QList<HighlightItem> res = items(InformationTypes(type), sort, options);
    if (sort)
        sortHelper(res);
    return res;
}

QList<HighlightItem> HighlightModel::items(PropertyType type, bool sort, const HighlightOptions& options) const
{
    QList<HighlightItem> res = _item_property_types.values(type);

    if (options != HighlightOptions()) {
        for (int i = res.count() - 1; i >= 0; i--) {
            if ((res.at(i).options() & options) != options)
                res.removeAt(i);
        }
    }

    if (sort)
        sortHelper(res);
    return res;
}

QList<HighlightItem> HighlightModel::groupItems(int group_code, bool sort, const HighlightOptions& options) const
{
    QList<HighlightItem> res = _item_groups.values(group_code);

    if (options != HighlightOptions()) {
        for (int i = res.count() - 1; i >= 0; i--) {
            if ((res.at(i).options() & options) != options)
                res.removeAt(i);
        }
    }

    if (sort)
        sortHelper(res);
    return res;
}

InformationType HighlightModel::topInformationType(const QList<HighlightItem>& items)
{
    InformationType res = InformationType::Invalid;
    for (auto& i : items) {
        res = static_cast<InformationType>(qMax(static_cast<int>(res), static_cast<int>(i.type())));
    }
    return res;
}

DataPropertyList HighlightModel::properties(bool sort, const HighlightOptions& options) const
{
    return properties(InformationTypes(std::numeric_limits<int>::max()), sort, options);
}

DataPropertyList HighlightModel::properties(const InformationTypes& types, bool sort, const HighlightOptions& options) const
{
    DataPropertyList res;
    auto found = items(types, sort, options);
    for (auto i = found.constBegin(); i != found.constEnd(); ++i) {
        if (res.contains(i->property()))
            continue;
        res << i->property();
    }
    return res;
}

DataPropertyList HighlightModel::properties(InformationType type, bool sort, const HighlightOptions& options) const
{
    DataPropertyList res;
    auto found = items(type, sort, options);
    for (auto i = found.constBegin(); i != found.constEnd(); ++i) {
        if (res.contains(i->property()))
            continue;
        res << i->property();
    }
    return res;
}

DataPropertyList HighlightModel::properties(PropertyType type, bool sort, const HighlightOptions& options) const
{
    DataPropertyList res;
    auto found = items(type, sort, options);
    for (auto i = found.constBegin(); i != found.constEnd(); ++i) {
        if (res.contains(i->property()))
            continue;
        res << i->property();
    }
    return res;
}

QStringList HighlightModel::toStringList(InformationTypes types, bool plain, bool show_type) const
{
    QStringList res;

    InformationTypes error_group
        = (InformationType::Error | InformationType::Critical | InformationType::Fatal | InformationType::Required);
    InformationTypes warning_group = InformationType::Warning;
    InformationTypes hint_group = (InformationType::Information | InformationType::Success);

    bool isError = (types & error_group) > 0;
    bool isWarning = (types & warning_group) > 0;
    bool isHint = (types & hint_group) > 0;

    auto items = _items.values();
    sortHelper(items);

    for (auto& i : items) {
        QString newText = plain ? i.plainText() : i.text();

        if (types != 0) {
            bool need_combine = false;
            if (isError && (error_group & i.type()) > 0) {
                if (show_type && (isWarning || isHint))
                    need_combine = true;

            } else if (isWarning && (warning_group & i.type()) > 0) {
                if (show_type && (isError || isHint))
                    need_combine = true;

            } else if (isHint && (hint_group & i.type()) > 0) {
                if (show_type && (isWarning || isError))
                    need_combine = true;

            } else
                continue;

            if (need_combine)
                newText = QString("%1: %2").arg(Utils::informationTypeText(i.type()), newText);
        }

        if (!res.contains(newText))
            res << newText;
    }

    return res;
}

ErrorList HighlightModel::toErrorList(InformationTypes types, bool plain, bool show_type) const
{
    ErrorList res;
    QStringList sl = toStringList(types, plain, show_type);

    for (auto& s : sl) {
        res << Error(s);
    }

    return res;
}

Error HighlightModel::toError(InformationTypes types, bool plain, bool show_type) const
{
    return Error(toErrorList(types, plain, show_type));
}

QString HighlightModel::toHtmlTable(InformationTypes types, bool plain, bool show_type) const
{
    return HtmlTools::table(toError(types, plain, show_type));
}

void HighlightModel::beginUpdate()
{
    _update_counter++;
    if (_update_counter == 1)
        emit sg_beginUpdate();
}

void HighlightModel::endUpdate()
{
    Z_CHECK(_update_counter > 0);
    _update_counter--;
    if (_update_counter == 0)
        emit sg_endUpdate();
}

bool HighlightModel::isUpdating() const
{
    return _update_counter > 0;
}

void HighlightModel::sortHelper(QList<HighlightItem>& items)
{
    std::sort(items.begin(), items.end());
}

QMap<DataProperty, QList<HighlightItem>> HighlightModel::groupByProperty(const QList<HighlightItem>& items)
{
    QMap<DataProperty, QList<HighlightItem>> prepared;
    for (auto& i : items) {
        auto list = prepared.value(i.property());
        list.append(i);
        prepared[i.property()] = list;
    }
    return prepared;
}

void HighlightModel::expandPropertyChild(const DataProperty& p, QList<HighlightItem>& expanded)
{
    PropertyType type = p.propertyType();
    DataProperty dataset;

    if (type == PropertyType::Dataset) {
        dataset = p;
        expanded = items(PropertyType::ColumnFull);
        expanded << items(PropertyType::RowFull);
        expanded << items(PropertyType::Cell);

    } else if (type == PropertyType::RowFull) {
        dataset = p.dataset();
        expanded = items(PropertyType::Cell);

    } else if (type == PropertyType::ColumnFull) {
        dataset = p.dataset();
        expanded = items(PropertyType::Cell);
    }

    for (int i = expanded.count() - 1; i >= 0; i--) {
        if (expanded.at(i).property().dataset() != dataset) {
            expanded.removeAt(i);
            continue;
        }

        if (type == PropertyType::RowFull && expanded.at(i).property().row() != p) {
            expanded.removeAt(i);
            continue;
        }

        if (type == PropertyType::ColumnFull && expanded.at(i).property().column() != p) {
            expanded.removeAt(i);
            continue;
        }
    }
}

void HighlightModel::removeChildProperties(const DataProperty& property, const DataPropertySet& ignore_child)
{
    Z_CHECK(!_child_removing); // проверка на криворукость

    _child_removing = true;

    QList<HighlightItem> expanded;
    expandPropertyChild(property, expanded);

    for (auto& item : expanded) {
        if (ignore_child.contains(item.property()))
            continue;
        if (item.property().isDatasetPart() && ignore_child.contains(item.property().dataset()))
            continue;
        remove(item);
    }

    _child_removing = false;
}

void HighlightModel::removeHelper(const HighlightItem& item, bool emit_signals)
{
    if (!item.isValid())
        return;

    _items.remove(item.uniqueKey());
    _item_properties.remove(item.property(), item);
    _item_groups.remove(item.groupCode(), item);
    _item_types.remove(item.type(), item);
    _item_property_types.remove(item.property().propertyType(), item);
    _items_options_hash.clear();
    _ordered.clear();

    if (emit_signals && _update_counter == 0)
        emit sg_itemRemoved(item);

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
//    qDebug() << "Highlight removed:" << item.property().uniqueKey().simplified();
#endif
}

void HighlightModel::addHelper(const HighlightItem& item, bool emit_signals)
{
    Z_CHECK(item.property().propertyType() != PropertyType::ColumnPartial
        && item.property().propertyType() != PropertyType::RowPartial);

    if (!item.isValid())
        return;

    _items[item.uniqueKey()] = item;
    _item_properties.insert(item.property(), item);
    _item_groups.insert(item.groupCode(), item);
    _item_types.insert(item.type(), item);
    _items_options_hash.clear();
    _item_property_types.insert(item.property().propertyType(), item);
    _ordered.clear();

    if (emit_signals && _update_counter == 0)
        emit sg_itemInserted(item);

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
//    qDebug() << "Highlight added:" << item.property().uniqueKey().simplified();
#endif
}

const QList<HighlightItem>& HighlightModel::itemsByOptions(const HighlightOptions& options) const
{
    auto res = _items_options_hash.constFind(options);
    if (res == _items_options_hash.constEnd()) {
        QList<HighlightItem> items;
        for (auto i = _items.constBegin(); i != _items.constEnd(); ++i) {
            if ((i.value().options() & options) == options)
                items << i.value();
        }
        _items_options_hash[options] = items;
        res = _items_options_hash.constFind(options);
    }
    return *res;
}

HighlightItem::HighlightItem()
    : _d(new HighlightItem_data)
{
}

HighlightItem::HighlightItem(const HighlightItem& item)
    : _d(item._d)
{
}

HighlightItem::~HighlightItem()
{
}

HighlightItem& HighlightItem::operator=(const HighlightItem& item)
{
    if (this != &item)
        _d = item._d;
    return *this;
}

bool HighlightItem::operator==(const HighlightItem& item) const
{
    if (type() != item.type())
        return false;

    return uniqueKey() == item.uniqueKey();
}

bool HighlightItem::operator!=(const HighlightItem& item) const
{
    return !operator==(item);
}

bool HighlightItem::operator<(const HighlightItem& item) const
{
    if (type() != item.type())
        return type() < item.type();

    if (property().propertyType() == PropertyType::Cell && item.property().propertyType() == PropertyType::Cell
        && property().dataset() == item.property().dataset()) {
        // TODO надо сделать сортировку в соответствии с видимым отображением. Сейчас сортируется по ключам и т.п.
        if (property().rowId() != item.property().rowId())
            return property().rowId() < item.property().rowId();

        return property().column() < item.property().column();
    }

    return uniqueKey() < item.uniqueKey();
}

bool HighlightItem::isValid() const
{
    return type() != InformationType::Invalid;
}

HighlightItem::HighlightItem(InformationType type, const DataProperty& property, int id, const QString& text, int group_code,
                             const QVariant& data, const HighlightOptions& options)
    : _d(new HighlightItem_data)
{
    Z_CHECK(property.isValid());

    _d->type = type;
    _d->property = property;
    _d->id = id;
    _d->text = text;
    _d->group_code = group_code;
    _d->data = data;
    _d->key = createKey(property, id);
    _d->hash_key = qHash(_d->key);
    _d->options = options;
}

InformationType HighlightItem::type() const
{
    return _d->type;
}

DataProperty HighlightItem::property() const
{
    return _d->property;
}

int HighlightItem::id() const
{
    return _d->id;
}

QString HighlightItem::text() const
{
    return _d->text;
}

QString HighlightItem::plainText() const
{
    return HtmlTools::plain(text(), false);
}

int HighlightItem::groupCode() const
{
    return _d->group_code;
}

QVariant HighlightItem::data() const
{
    return _d->data;
}

HighlightOptions HighlightItem::options() const
{
    return _d->options;
}

QString HighlightItem::uniqueKey() const
{
    return _d->key;
}

uint HighlightItem::hashKey() const
{
    return _d->hash_key;
}

QVariant HighlightItem::variant() const
{
    return QVariant::fromValue<HighlightItem>(*this);
}

HighlightItem HighlightItem::fromVariant(const QVariant& v)
{
    return v.value<HighlightItem>();
}

QString HighlightItem::createKey(const DataProperty& p, int id)
{
    return p.uniqueKey() + Consts::KEY_SEPARATOR + QString::number(id);
}

HighlightInfo::HighlightInfo(const DataStructure* structure)
    : _structure(structure)
{
    Z_CHECK_NULL(structure);
}

HighlightInfo::~HighlightInfo()
{
}

bool HighlightInfo::contains(const DataProperty& property) const
{
    Z_CHECK(property.isValid());

    HighlightItemHashPtr info = _data.value(property);
    if (info == nullptr)
        return false;

    for (auto i = info->constBegin(); i != info->constEnd(); ++i) {
        if (i.value() == nullptr)
            continue;
        return true;
    }

    return false;
}

bool HighlightInfo::contains(const PropertyID& property_id) const
{
    return contains(_structure->property(property_id));
}

bool HighlightInfo::hasLevel(const DataProperty& property, InformationType type) const
{
    return static_cast<int>(getHighlight(property)) >= static_cast<int>(type);
}

bool HighlightInfo::hasLevel(const PropertyID& property_id, InformationType type) const
{
    return hasLevel(_structure->property(property_id), type);
}

InformationType HighlightInfo::getHighlight(const DataProperty& property) const
{
    Z_CHECK(property.isValid());

    HighlightItemHashPtr info = _data.value(property);
    if (info == nullptr)
        return InformationType::Invalid;

    InformationType type = InformationType::Invalid;
    for (auto i = info->constBegin(); i != info->constEnd(); ++i) {
        const HighlightItemPtr& h = i.value();
        if (h == nullptr)
            continue;
        type = static_cast<InformationType>(qMax(static_cast<int>(h->type()), static_cast<int>(type)));
    }

    return type;
}

InformationType HighlightInfo::getHighlight(const PropertyID& property_id) const
{
    return getHighlight(_structure->property(property_id));
}

void HighlightInfo::insert(const DataProperty& p, int id, const QString& text, InformationType type, int group_code, const QVariant& data,
                           bool is_translate, const HighlightOptions& options)
{
    insertHelper(p, id, text, type, group_code, data, is_translate, options);
}

void HighlightInfo::insert(const PropertyID& property_id, int id, const QString& text, InformationType type, int group_code,
                           const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    insert(_structure->property(property_id), id, text, type, group_code, data, is_translate, options);
}

void HighlightInfo::insert(const DataProperty& property, const PropertyID& column_property_id, int id, const QString& text,
                           InformationType type, int group_code, const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    Z_CHECK(property.propertyType() == PropertyType::RowFull || property.propertyType() == PropertyType::Cell);
    auto cell = DataProperty::cellProperty(property.isCell() ? property.row() : property, _structure->property(column_property_id));
    insert(cell, id, text, type, group_code, data, is_translate, options);
}

void HighlightInfo::empty(const DataProperty& p, int id)
{
    Z_CHECK(id >= 0);
    Z_CHECK(_block_check_id > 0 || id >= Consts::MINIMUM_HIGHLIGHT_ID);

    HighlightItemHashPtr d = dataHelper(p);
    if (d->contains(id))
        return;

    d->insert(id, nullptr);
}

void HighlightInfo::empty(const PropertyID& property_id, int id)
{
    empty(_structure->property(property_id), id);
}

void HighlightInfo::empty(const DataProperty& property, const PropertyID& column_property_id, int id)
{
    Z_CHECK(property.propertyType() == PropertyType::RowFull || property.propertyType() == PropertyType::Cell);
    auto cell = DataProperty::cellProperty(property.isCell() ? property.row() : property, _structure->property(column_property_id));
    empty(cell, id);
}

void HighlightInfo::set(const DataProperty& property, int id, bool is_insert, const QString& text, InformationType type, int group_code,
                        const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    if (is_insert)
        insert(property, id, text, type, group_code, data, is_translate, options);
    else
        empty(property, id);
}

void HighlightInfo::set(const PropertyID& property_id, int id, bool is_insert, const QString& text, InformationType type, int group_code,
                        const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    set(_structure->property(property_id), id, is_insert, text, type, group_code, data, is_translate, options);
}

void HighlightInfo::set(const DataProperty& property, const PropertyID& column_property_id, int id, bool is_insert, const QString& text,
                        InformationType type, int group_code, const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    Z_CHECK(property.propertyType() == PropertyType::RowFull || property.propertyType() == PropertyType::Cell);
    auto cell = DataProperty::cellProperty(property.isCell() ? property.row() : property, _structure->property(column_property_id));
    set(cell, id, is_insert, text, type, group_code, data, is_translate, options);
}

void HighlightInfo::checkProperty(const DataProperty& p)
{
    Z_CHECK_X(_block_check_property > 0 || !_current_property.isValid() || _current_property == p, QString("Wrong highlight. Requested: %1. Inserted: %2").arg(_current_property.toPrintable(), p.toPrintable()));
}

void HighlightInfo::insertHelper(const DataProperty& p, int id, const QString& text, InformationType type, int group_code,
                                 const QVariant& data, bool is_translate, const HighlightOptions& options)
{
    Z_CHECK(type != InformationType::Invalid);
    Z_CHECK(id >= 0);
    Z_CHECK(_block_check_id > 0 || id >= Consts::MINIMUM_HIGHLIGHT_ID);
    checkProperty(p);

    auto d = dataHelper(p);
    d->insert(id, std::make_shared<HighlightItem>(type, p, id, is_translate ? translate(text) : text, group_code, data, options));
}

void HighlightInfo::setCurrentProperty(const DataProperty& p)
{
    _current_property = p;
}

void HighlightInfo::blockCheckId()
{
    _block_check_id++;
}

void HighlightInfo::unBlockCheckId()
{
    Z_CHECK(_block_check_id > 0);
    _block_check_id--;
}

void HighlightInfo::blockCheckCurrent()
{
    _block_check_property++;
}

void HighlightInfo::unBlockCheckCurrent()
{
    Z_CHECK(_block_check_property > 0);
    _block_check_property--;
}

HighlightInfo::HighlightItemHashPtr HighlightInfo::dataHelper(const DataProperty& p) const
{
    auto prop_data = _data.value(p, nullptr);
    if (prop_data == nullptr) {
        prop_data = std::make_shared<QHash<int, HighlightItemPtr>>();
        _data[p] = prop_data;
    }
    return prop_data;
}

} // namespace zf
