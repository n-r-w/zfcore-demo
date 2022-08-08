#include "zf_condition.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_translation.h"
#include "zf_module_data_object.h"
#include "zf_view.h"

namespace zf
{
Condition::~Condition()
{
}

bool Condition::isEmpty() const
{
    return (_type == ConditionType::Or || _type == ConditionType::And) ? _children.isEmpty() : false;
}

QString Condition::id() const
{
    return _id;
}

ConditionType Condition::type() const
{
    return _type;
}

QString Condition::typeToText() const
{
    return _type == ConditionType::Undefined ? QString() : Utils::conditionTypeToText(_type);
}

bool Condition::isRoot() const
{
    return _parent.lock() == nullptr;
}

bool Condition::isLogical() const
{
    return _type == ConditionType::Or || _type == ConditionType::And;
}

bool Condition::isPropertyCompare() const
{
    return _is_property_compare;
}

bool Condition::isRawCompare() const
{
    if (_type != ConditionType::Compare || !_source.isValid())
        return false;

    if (_source.lookup() == nullptr)
        return true;

    return _compare_operator != CompareOperator::Undefined && _compare_operator != CompareOperator::Equal
        && _compare_operator != CompareOperator::NotEqual;
}

DataProperty Condition::source() const
{
    return _source;
}

ConversionType Condition::sourceConversion() const
{
    return _source_conversion;
}

CompareOperator Condition::compareOperator() const
{
    return _compare_operator;
}

QString Condition::compareOperatorText(bool for_java_script, bool full_name) const
{
    return _compare_operator == CompareOperator::Undefined ? QString()
                                                           : Utils::compareOperatorToText(_compare_operator, for_java_script, full_name);
}

QVariant Condition::targetValue() const
{
    return _target_value;
}

DataProperty Condition::targetProperty() const
{
    return _target_property;
}

ConversionType Condition::targetConversion() const
{
    return _target_conversion;
}

std::shared_ptr<Condition> Condition::parent() const
{
    return _parent.lock();
}

const QList<std::shared_ptr<Condition>>& Condition::children() const
{
    return _children;
}

std::shared_ptr<Condition> Condition::condition(const QString& id, bool recursive) const
{
    for (auto& c : _children) {
        if (c->id() == id)
            return c;
        if (recursive) {
            auto res = c->condition(id);
            if (res != nullptr)
                return res;
        }
    }
    return nullptr;
}

bool Condition::calculateOnData(const ModuleDataObject* data, QList<ModelPtr>& data_not_ready, Error& error,
                                const DataPropertyList& rows) const
{
    return calculate(data, nullptr, rows, data_not_ready, error);
}

bool Condition::calculateOnView(const View* view, QList<ModelPtr>& data_not_ready, Error& error,
                                const DataPropertyList& rows) const
{
    return calculate(nullptr, view, rows, data_not_ready, error);
}

bool Condition::isValid() const
{
    bool res = isValidSelf();

    if (res) {
        for (auto& c : _children) {
            if (!c->isValid())
                return false;
        }
    }

    return res;
}

Condition::Condition()
{
    _id = Utils::generateUniqueStringDefault();
}

bool Condition::calculate(const ModuleDataObject* data, const View* view, const DataPropertyList& rows,
                          QList<ModelPtr>& data_not_ready, Error& error) const
{
    Z_CHECK(data != nullptr || view != nullptr);

    if (_type == ConditionType::Or || _type == ConditionType::And) {
        if (_children.isEmpty())
            return true;

        for (auto& c : _children) {
            bool res = c->calculate(data, view, rows, data_not_ready, error);
            if (!data_not_ready.isEmpty() || error.isError())
                return false;

            if (res) {
                if (_type == ConditionType::Or)
                    return true;

            } else {
                if (_type == ConditionType::And)
                    return false;
                else
                    continue;
            }
        }
        if (_type == ConditionType::Or)
            return false;
        else
            return true;

    } else {
        return calculateSelf(data, view, rows, data_not_ready, error);
    }
}

Condition::Condition(const std::shared_ptr<Condition>& parent, ConditionType type, ConversionType source_conversion,
    const DataProperty& source, CompareOperator compare_operator, ConversionType target_conversion, const DataProperty& target_property,
    const QVariant& target_value, bool is_property_compare, bool validation)
{
    _id = Utils::generateUniqueStringDefault();
    _type = type;

    _source_conversion = source_conversion;
    _source = source;
    _compare_operator = compare_operator;
    _target_conversion = target_conversion;
    _target_property = target_property;
    _target_value = target_value;
    _parent = parent;
    _is_property_compare = is_property_compare;

    if (parent != nullptr)
        Z_CHECK(parent->isLogical());

    if (validation)
        Z_CHECK(isValid());
}

void Condition::clear()
{
    _children.clear();
    _type = ConditionType::Undefined;
    resetToDefault();
}

bool Condition::calculateSelf(const ModuleDataObject* data, const View* view, const DataPropertyList& rows, QList<ModelPtr>& data_not_ready,
                              Error& error) const
{
    QVariant source_value = calculateValue(data, view, rows, _source, _source_conversion, data_not_ready, error);
    if (error.isError())
        return true;

    if (!data_not_ready.isEmpty())
        return false;

    if (_type == ConditionType::Required) {
        return !source_value.toString().trimmed().isEmpty();

    } else if (_type == ConditionType::Compare) {
        QVariant value;

        if (_is_property_compare) {
            value = calculateValue(data, view, rows, _target_property, _target_conversion, data_not_ready, error);
            if (error.isError())
                return true;

            if (!data_not_ready.isEmpty())
                return false;

        } else {
            if (isRawCompare()) {
                value = _target_value;
            } else {
                // расшифровка lookup
                Z_CHECK_NULL(_source.lookup());

                if (_source.lookup()->type() == LookupType::List) {
                    value = _source.lookup()->listName(_target_value);

                } else {
                    ModelPtr source_model;
                    if (!Core::getEntityValue(_source.lookup(), _target_value, value, source_model, error)) {
                        if (error.isError()) {
                            return true;

                        } else {
                            data_not_ready << source_model;
                            return false;
                        }
                    }
                }
            }
        }

        // для сравнения Bool считаем invalid как false
        if (source_value.type() == QVariant::Bool && (!value.isValid() || value.isNull()))
            value = false;
        else if (value.type() == QVariant::Bool && (!source_value.isValid() || source_value.isNull()))
            source_value = false;

        return Utils::compareVariant(source_value, value, _compare_operator, Core::locale(LocaleType::UserInterface),
                                     CompareOption::CaseInsensitive);

    } else
        Z_HALT_INT;

    return false;
}

QList<Condition::ErrorType> Condition::errors() const
{
    QList<Condition::ErrorType> res;
    if (_type == ConditionType::Undefined)
        res << ErrorType::Type;

    if (_type == ConditionType::Or || _type == ConditionType::And) {
        if (_children.isEmpty())
            res << ErrorType::Type;
        if (_source_conversion != ConversionType::Undefined)
            res << ErrorType::SourceConversion;
        if (_source.isValid())
            res << ErrorType::Source;
        if (_compare_operator != CompareOperator::Undefined)
            res << ErrorType::Operator;
        if (_target_conversion != ConversionType::Undefined)
            res << ErrorType::TargetConversion;
        if (_target_property.isValid() || _target_value.isValid() || _is_property_compare)
            res << ErrorType::Target;

    } else if (_type == ConditionType::Required) {
        if (_source_conversion == ConversionType::Undefined)
            res << ErrorType::SourceConversion;
        if (!_source.isValid())
            res << ErrorType::Source;
        if (_compare_operator != CompareOperator::Undefined)
            res << ErrorType::Operator;
        if (_target_conversion != ConversionType::Undefined)
            res << ErrorType::TargetConversion;
        if (_target_property.isValid() || _target_value.isValid() || _is_property_compare)
            res << ErrorType::Target;

    } else if (_type == ConditionType::Compare) {
        if (_source_conversion == ConversionType::Undefined)
            res << ErrorType::SourceConversion;
        if (!_source.isValid())
            res << ErrorType::Source;
        if (_compare_operator == CompareOperator::Undefined)
            res << ErrorType::Operator;

        if (_is_property_compare) {
            if (_target_conversion == ConversionType::Undefined)
                res << ErrorType::TargetConversion;
            if (!_target_property.isValid() || _target_value.isValid())
                res << ErrorType::Target;
        } else {
            if (_target_conversion != ConversionType::Undefined)
                res << ErrorType::TargetConversion;
            if (_target_property.isValid())
                res << ErrorType::Target;
        }
    }

    return res;
}

bool Condition::isValidSelf() const
{
    return errors().isEmpty();
}

QVariant Condition::calculateValue(const ModuleDataObject* data, const View* view, const DataPropertyList& rows,
                                   const DataProperty& property, ConversionType conversion, QList<ModelPtr>& data_not_ready, Error& error)
{
    error.clear();

    if (view != nullptr) {
        Z_CHECK(data == nullptr);
        data = view;
    }
    Z_CHECK_NULL(data);
    if (property.propertyType() == PropertyType::Field)
        return convertValue(data, view, property, conversion, data->data()->value(property));

    if (property.propertyType() == PropertyType::ColumnFull) {
        // ищем строку, относящуюся к этому набору данных
        DataProperty row;
        for (auto& r : rows) {
            Z_CHECK(r.isValid() && r.propertyType() == PropertyType::RowFull);
            if (r.dataset() == property.dataset()) {
                row = r;
                break;
            }
        }
        Z_CHECK_X(row.isValid(), utf("Не задана строка с идентификатором для набора данных %1:%2")
                                     .arg(data->structure()->id().value())
                                     .arg(property.dataset().id().value()));
        Z_CHECK(row.rowId().isValid());

        QModelIndex row_index = data->findDatasetRowID(property.dataset(), row.rowId());
        Z_CHECK_X(row_index.isValid(), utf("Не найдена задана строка с идентификатором для набора данных %1:%2")
                                           .arg(data->structure()->id().value())
                                           .arg(property.dataset().id().value()));

        if (view == nullptr) {
            return convertValue(data, view, property, conversion,
                                data->data()->cell(row_index.row(), property, Qt::DisplayRole, row_index.parent()));

        } else {
            QModelIndex idx = view->data()->cellIndex(row_index.row(), property, row_index.parent());
            QVariant value = idx.data(Qt::DisplayRole);
            QString display_value = idx.data(Qt::DisplayRole).toString();
            QList<ModelPtr> model_data_not_ready;
            auto source_idx = view->sourceIndex(idx);
            error = view->getDatasetCellVisibleValue(source_idx.row(), property, source_idx.parent(), value,
                                                     VisibleValueOption::Application, display_value, model_data_not_ready);
            if (!display_value.isEmpty() || property.dataType() != DataType::Bool)
                value = display_value;

            for (auto& m : qAsConst(model_data_not_ready)) {
                Z_CHECK(m->isLoading());
            }

            data_not_ready << model_data_not_ready;

            return !model_data_not_ready.isEmpty() || error.isError() ? QVariant() : convertValue(data, view, property, conversion, value);
        }
    }

    Z_HALT_INT; // тут быть не должны
    return QVariant();
}

QVariant Condition::convertValue(
    const ModuleDataObject* data, const View* view, const DataProperty& property, ConversionType conversion, const QVariant& value)
{
    if (view != nullptr)
        data = view;

    if (conversion == ConversionType::Direct)
        return value;

    if (conversion == ConversionType::UpperCase)
        return value.toString().trimmed().toUpper();

    if (conversion == ConversionType::LowerCase)
        return value.toString().trimmed().toLower();

    if (conversion == ConversionType::Length)
        return value.toString().trimmed().length();

    if (conversion == ConversionType::Name) {
        Z_CHECK_X(property.lookup() != nullptr,
                  utf("Свойство %1:%2 не имеет lookup").arg(data->structure()->id().value()).arg(property.id().value()));
        return data->getLookupValue(property, value);
    }

    Z_HALT_INT; // тут быть не должны
    return QVariant();
}

void Condition::clearChildren(QStringList& ids)
{
    for (auto& c : _children) {
        ids.prepend(c->id());
        c->clearChildren(ids);
    }
    _children.clear();
}

void Condition::resetToDefault()
{
    if (_type == ConditionType::Undefined) {
        _is_property_compare = false;
        _source.clear();
        _source_conversion = ConversionType::Undefined;
        _compare_operator = CompareOperator::Undefined;
        _target_conversion = ConversionType::Undefined;
        _target_property.clear();
        _target_value.clear();

    } else if (isLogical()) {
        Z_CHECK(_children.isEmpty());
        _is_property_compare = false;
        _source.clear();
        _source_conversion = ConversionType::Undefined;
        _compare_operator = CompareOperator::Undefined;
        _target_conversion = ConversionType::Undefined;
        _target_property.clear();
        _target_value.clear();

    } else if (_type == ConditionType::Required) {
        _is_property_compare = false;
        _source_conversion = ConversionType::Direct;
        _compare_operator = CompareOperator::Undefined;
        _target_conversion = ConversionType::Undefined;
        _target_property.clear();
        _target_value.clear();

    } else if (_type == ConditionType::Compare) {
        _source_conversion = ConversionType::Direct;
        _compare_operator = CompareOperator::Equal;
        _target_conversion = ConversionType::Undefined;
        _target_property.clear();
        _target_value.clear();
        _is_property_compare = false;
    }
}

ComplexCondition::ComplexCondition()
{
    _root = addHelper(new Condition(nullptr, ConditionType::And, ConversionType::Undefined, DataProperty {}, CompareOperator::Undefined,
        ConversionType::Undefined, DataProperty {}, QVariant {}, false, false));
}

ConditionPtr ComplexCondition::root() const
{
    return _root;
}

bool ComplexCondition::isEmpty() const
{
    return _root->children().isEmpty();
}

ConditionPtr ComplexCondition::addLogical(ConditionType type, const ConditionPtr& parent)
{
    return addHelper(new Condition(parent == nullptr ? _root : parent, type, ConversionType::Undefined, DataProperty {},
        CompareOperator::Undefined, ConversionType::Undefined, DataProperty {}, QVariant {}, false, _validation));
}

ConditionPtr ComplexCondition::addRequired(const DataProperty& property, ConversionType conversion, const ConditionPtr& parent)
{
    return addHelper(new Condition(parent == nullptr ? _root : parent, ConditionType::Required, conversion, property,
        CompareOperator::Undefined, ConversionType::Undefined, DataProperty {}, QVariant {}, false, _validation));
}

ConditionPtr ComplexCondition::addCompare(
    const DataProperty& property, CompareOperator op, const QVariant& value, ConversionType conversion, const ConditionPtr& parent)
{
    return addHelper(new Condition(parent == nullptr ? _root : parent, ConditionType::Compare, conversion, property, op,
        ConversionType::Undefined, DataProperty {}, value, false, _validation));
}

ConditionPtr ComplexCondition::addCompare(const DataProperty& source_property, CompareOperator op, const DataProperty& target_property,
    ConversionType source_conversion, ConversionType target_conversion, const ConditionPtr& parent)
{
    return addHelper(new Condition(parent == nullptr ? _root : parent, ConditionType::Compare, source_conversion, source_property, op,
        target_conversion, target_property, QVariant {}, true, _validation));
}

ConditionPtr ComplexCondition::updateLogical(const ConditionPtr& condition, ConditionType type)
{
    Z_CHECK_NULL(condition);
    Z_CHECK(type == ConditionType::Or || type == ConditionType::And);

    if (condition->type() != type) {
        bool was_logical = condition->isLogical();

        condition->_type = type;

        if (condition == _root)
            Z_CHECK(condition->isLogical());

        if (was_logical != condition->isLogical())
            condition->resetToDefault();

        emit sg_changed(condition->id());
    }

    return condition;
}

ConditionPtr ComplexCondition::updateRequired(const ConditionPtr& condition, const DataProperty& property, ConversionType conversion)
{
    Z_CHECK_NULL(condition);
    Z_CHECK(condition != _root);

    if (condition->_type != ConditionType::Required || condition->_source != property || condition->_source_conversion != conversion) {
        removeChildrenHelper(condition.get());
        condition->_type = ConditionType::Required;
        condition->resetToDefault();

        condition->_source = property;
        condition->_source_conversion = conversion;

        emit sg_changed(condition->id());
    }

    return condition;
}

ConditionPtr ComplexCondition::updateCompare(
    const ConditionPtr& condition, const DataProperty& property, CompareOperator op, const QVariant& value, ConversionType conversion)
{
    Z_CHECK_NULL(condition);
    Z_CHECK(condition != _root);

    if (condition->_type != ConditionType::Compare || condition->_source != property || condition->_compare_operator != op
        || condition->_source_conversion != conversion || condition->_is_property_compare
        || Utils::compareVariant(condition->_target_value, value, CompareOperator::NotEqual, Core::locale(LocaleType::UserInterface),
                                 CompareOption::CaseInsensitive)) {
        removeChildrenHelper(condition.get());
        condition->_type = ConditionType::Compare;
        condition->resetToDefault();

        condition->_source = property;
        condition->_compare_operator = op;
        condition->_source_conversion = conversion;
        condition->_target_value = value;
        condition->_is_property_compare = false;

        emit sg_changed(condition->id());
    }

    return condition;
}

ConditionPtr ComplexCondition::updateCompare(const ConditionPtr& condition, const DataProperty& source_property, CompareOperator op,
    const DataProperty& target_property, ConversionType source_conversion, ConversionType target_conversion)
{
    Z_CHECK_NULL(condition);
    Z_CHECK(condition != _root);

    if (condition->_type != ConditionType::Compare || condition->_source != source_property || condition->_compare_operator != op
        || condition->_source_conversion != source_conversion || condition->_target_property != target_property
        || condition->_target_conversion != target_conversion || !condition->_is_property_compare) {
        removeChildrenHelper(condition.get());
        condition->_type = ConditionType::Compare;
        condition->resetToDefault();

        condition->_source = source_property;
        condition->_compare_operator = op;
        condition->_source_conversion = source_conversion;
        condition->_target_property = target_property;
        condition->_target_conversion = target_conversion;
        condition->_is_property_compare = true;

        emit sg_changed(condition->id());
    }

    return condition;
}

void ComplexCondition::remove(const ConditionPtr& c)
{
    Z_CHECK_NULL(c);
    Z_CHECK(c->parent() != nullptr);

    removeChildren(c);
    QString id = c->id();
    c->parent()->_children.removeOne(c);

    emit sg_removed(id);
}

void ComplexCondition::removeChildren(const ConditionPtr& c)
{
    Z_CHECK_NULL(c);
    QStringList ids;
    c->clearChildren(ids);
    for (auto& id : ids) {
        emit sg_removed(id);
    }
}

ConditionPtr ComplexCondition::addEmpty(const ConditionPtr& parent)
{
    Z_CHECK(!validation());
    return addCompare(DataProperty(), CompareOperator::Equal, QVariant(), ConversionType::Direct, parent);
}

ConditionPtr ComplexCondition::condition(const QString& id) const
{
    if (_root->id() == id)
        return _root;

    return _root->condition(id, true);
}

bool ComplexCondition::calculateOnData(const ModuleDataObject* data, QList<ModelPtr>& data_not_ready, Error& error,
                                       const DataPropertyList& rows) const
{
    return _root->calculateOnData(data, data_not_ready, error, rows);
}

bool ComplexCondition::calculateOnView(const View* view, QList<ModelPtr>& data_not_ready, Error& error,
                                       const DataPropertyList& rows) const
{
    return _root->calculateOnView(view, data_not_ready, error, rows);
}

void ComplexCondition::clear()
{
    removeChildren(_root);
    updateLogical(_root, ConditionType::And);
}

void ComplexCondition::copyFrom(const ComplexCondition* c, bool interactive)
{
    Z_CHECK_NULL(c);

    if (!interactive)
        blockSignals(true);

    removeChildren(_root);
    updateLogical(_root, c->root()->type());

    bool val = validation();
    setValidation(false);
    copyConditionHelper(nullptr, c->root());
    if (isValid())
        setValidation(val);

    if (!interactive) {
        blockSignals(false);
        emit sg_changed(_root->id());
    }
}

std::shared_ptr<ComplexCondition> ComplexCondition::createForProperty(const DataProperty& property)
{
    Z_CHECK(property.isValid());
    std::shared_ptr<ComplexCondition> res = Z_MAKE_SHARED(ComplexCondition);

    for (auto& c : property.constraints()) {
        if (c->type() == ConditionType::Required) {
            res->addRequired(property);
        } else {
            Core::logError("ComplexCondition::createForProperty - тип огранчиения не поддерживается");
        }
    }

    return res;
}

bool ComplexCondition::validation() const
{
    return _validation;
}

void ComplexCondition::setValidation(bool b)
{
    if (b == _validation)
        return;

    if (b)
        Z_CHECK(isValid());

    _validation = b;
}

bool ComplexCondition::isValid() const
{
    return isEmpty() || _root->isValid();
}

QList<ConditionPtr> ComplexCondition::findInvalidConditions() const
{
    QList<std::shared_ptr<Condition>> errors;
    findInvalidConditionsHelper(errors, _root);
    return errors;
}

void ComplexCondition::findInvalidConditionsHelper(QList<ConditionPtr>& errors, const ConditionPtr& parent)
{
    if (!parent->isValidSelf())
        errors << parent;

    for (auto& c : parent->children())
        findInvalidConditionsHelper(errors, c);
}

ConditionPtr ComplexCondition::addHelper(Condition* p)
{
    ConditionPtr ptr = std::shared_ptr<Condition>(p);
    if (p->parent() != nullptr)
        p->parent()->_children << ptr;

    emit sg_inserted(p->id());
    return ptr;
}

void ComplexCondition::copyConditionHelper(ConditionPtr parent, ConditionPtr source)
{
    ConditionPtr target;

    if (source->isRoot()) {
        target = _root;

    } else {
        if (source->isLogical()) {
            target = addLogical(source->type(), parent);

        } else if (source->type() == ConditionType::Required) {
            target = addRequired(source->source(), source->sourceConversion(), parent);

        } else if (source->type() == ConditionType::Compare) {
            if (source->isPropertyCompare())
                target = addCompare(source->source(), source->compareOperator(), source->targetProperty(), source->sourceConversion(),
                    source->targetConversion(), parent);
            else
                target = addCompare(source->source(), source->compareOperator(), source->targetValue(), source->sourceConversion(), parent);
        } else
            Z_HALT_INT;
    }

    for (auto& child : source->children()) {
        copyConditionHelper(target, child);
    }
}

void ComplexCondition::removeChildrenHelper(Condition* condition)
{
    while (!condition->children().isEmpty()) {
        remove(condition->children().last());
    }
}

//! Версия данных стрима Condition
static int _ConditionStreamVersion = 1;
QDataStream& operator<<(QDataStream& out, const Condition& obj)
{
    out << _ConditionStreamVersion;
    out << obj._id;
    toStreamInt(out, obj._type);
    out << obj._is_property_compare;
    toStreamInt(out, obj._source_conversion);
    out << obj._source;
    toStreamInt(out, obj._compare_operator);
    toStreamInt(out, obj._target_conversion);
    out << obj._target_property;
    out << obj._target_value;

    out << obj._children.count();
    for (auto& c : obj._children) {
        out << *c.get();
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, const ConditionPtr& obj)
{
    Z_CHECK_NULL(obj);

    int version;
    in >> version;
    if (in.status() != QDataStream::Ok || version != _ConditionStreamVersion) {
        obj->clear();
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    in >> obj->_id;
    fromStreamInt(in, obj->_type);
    in >> obj->_is_property_compare;
    fromStreamInt(in, obj->_source_conversion);
    in >> obj->_source;
    fromStreamInt(in, obj->_compare_operator);
    fromStreamInt(in, obj->_target_conversion);

    in >> obj->_target_property;
    in >> obj->_target_value;

    int c_count;
    in >> c_count;

    if (in.status() != QDataStream::Ok) {
        obj->clear();
        return in;
    }

    for (int i = 0; i < c_count; i++) {
        ConditionPtr c_child = ConditionPtr(new Condition());
        in >> c_child;

        if (in.status() != QDataStream::Ok) {
            obj->clear();
            return in;
        }

        c_child->_parent = obj;
        obj->_children << c_child;
    }

    return in;
}

//! Версия данных стрима ComplexCondition
static int _ComplexConditionStreamVersion = 1;
QDataStream& operator<<(QDataStream& out, const ComplexCondition& obj)
{
    out << _ComplexConditionStreamVersion;
    out << *obj._root;
    return out;
}

QDataStream& operator>>(QDataStream& in, ComplexCondition& obj)
{
    int version;
    in >> version;
    if (in.status() != QDataStream::Ok || version != _ComplexConditionStreamVersion) {
        obj.clear();
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    in >> obj._root;
    if (in.status() != QDataStream::Ok)
        obj.clear();

    return in;
}

} // namespace zf
