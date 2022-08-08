#include "zf_flat_item_model_p.h"
#include "zf_flat_item_model.h"
#include "zf_core.h"

namespace zf
{
int _FlatItemModel_data::_meta_type_number = -1;

_FlatRowData::_FlatRowData(int column_count, _FlatRows* owner)
    : _column_count(column_count)
    , _owner(owner)
{
}

_FlatRowData::~_FlatRowData()
{
    for (auto it = _data.cbegin(); it != _data.cend(); ++it) {
        if ((*it) != nullptr)
            Z_DELETE(*it);
    }

    if (_children)
        Z_DELETE(_children);
}

_FlatRows* _FlatRowData::children() const
{
    if (!_children)
        _children = Z_NEW(_FlatRows, owner()->model(), const_cast<_FlatRowData*>(this));

    return _children;
}

int _FlatRowData::childCount() const
{
    if (!_children)
        return 0;
    return _children->rowCount();
}

_FlatIndexData* _FlatRowData::getData(int column) const
{
    Z_CHECK(column < _column_count);

    _FlatIndexData* d;
    if (_data.size() <= column) {
        // превышает выделенное количество колонок. необходимо выделить память
        _data.resize(column + 1);
        _data[_data.size() - 1] = nullptr;
        d = Z_NEW(_FlatIndexData, const_cast<_FlatRowData*>(this));
        _data[column] = d;

    } else {
        d = _data.at(column);
        if (!d) {
            d = Z_NEW(_FlatIndexData, const_cast<_FlatRowData*>(this));
            _data[column] = d;
        }
    }

    return d;
}

void _FlatRowData::setColumnCount(int n)
{
    _column_count = n;

    if (_children)
        _children->setColumnCount(n);

    if (n < _data.size()) {
        // количество колонок стало меньше чем было выделено - очищаем память
        for (int i = n; i < _data.size(); i++) {
            Z_DELETE(_data.at(i));
        }

        _data.resize(n);
    }
}

bool _FlatRowData::insertColumns(int column, int count, bool no_childred)
{
    if (column >= 0 && count >= 0 && column <= _column_count) {
        if (count == 0)
            return true;

        if (_children && !no_childred)
            Z_CHECK(_children->insertColumns(column, count));

        if (_data.size() > column) {
            _data.insert(column, count, nullptr);
        }

        _column_count += count;
        return true;

    } else
        return false;
}

bool _FlatRowData::removeColumns(int column, int count, bool no_childred)
{
    if (column >= 0 && count >= 0 && column + count <= _column_count) {
        if (count == 0)
            return true;

        if (_children && !no_childred)
            Z_CHECK(_children->removeColumns(column, count));

        if (_data.size() > column)
            Utils::zDeleteAllCustom(Utils::takeVector<_FlatIndexData*>(_data, column, qMin(count, _data.size() - column)));
        _column_count -= count;

        return true;

    } else
        return false;
}

bool _FlatRowData::moveColumns(int source, int count, int destination)
{
    if (count < 0 || source < 0 || source + count > _column_count || destination < 0 || destination + count > _column_count)
        return false;

    if (source == destination)
        return true;

    if (destination >= source && destination <= source + count)
        return false;

    if (count == 0)
        return true;

    if (_children)
        Z_CHECK(_children->moveColumns(source, count, destination));

    Utils::moveVector<_FlatIndexData*>(_data, source, count, destination);

    return true;
}

int _FlatRowData::findChildRow(_FlatRowData* row_data) const
{
    return _children == nullptr ? -1 : _children->findRow(row_data);
}

_FlatRowData* _FlatRowData::clone(_FlatRows* owner, bool clone_children) const
{
    _FlatRowData* obj = Z_NEW(_FlatRowData, _column_count, owner);

    obj->_data.resize(_data.size());
    for (int i = 0; i < _data.size(); ++i) {
        if (_data.at(i) == nullptr)
            obj->_data[i] = nullptr;
        else
            obj->_data[i] = _data.at(i)->clone(obj);
    }

    if (_children && clone_children)
        obj->_children = _children->clone(const_cast<_FlatRowData*>(this), clone_children);

    return obj;
}

void _FlatRowData::alloc()
{
    if (_children)
        _children->alloc();
}

void _FlatRowData::resetChanged()
{
    for (auto& d : qAsConst(_data)) {
        if (d == nullptr)
            continue;
        d->resetChanged();
    }

    _children->resetChanged();
}

_FlatIndexData::_FlatIndexData(_FlatRowData* row)
    : _row(row)
{
}

_FlatIndexData::~_FlatIndexData()
{
    Utils::zDeleteAllCustom(_data);
    if (_flags != nullptr)
        Z_DELETE(_flags);
}

_FlatRowData* _FlatIndexData::row() const
{
    return _row;
}

Qt::ItemFlags _FlatIndexData::flags() const
{
    return hasFlags() ? *_flags : Qt::ItemFlags();
}

bool _FlatIndexData::hasFlags() const
{
    return _flags != nullptr;
}

QList<int> _FlatIndexData::roles() const
{
    QList<int> res;
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        res << (*i)->role;
    }
    return res;
}

LanguageMap _FlatIndexData::values(int role) const
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        if ((*i)->role != role)
            continue;
        return ((*i)->toLanguageMap());
    }

    return {};
}

void _FlatIndexData::remove(int role)
{
    for (int i = 0; i < _data.size(); ++i) {
        if (_data.at(i)->role != role)
            continue;

        Z_DELETE(_data.at(i));
        _data.remove(i);
        return;
    }
}

QVector<int>* _FlatIndexData::clear(bool take_data)
{
    QVector<int>* res = nullptr;

    if (take_data)
        res = Z_NEW(QVector<int>, _data.size());

    for (int i = 0; i < _data.size(); ++i) {
        if (take_data)
            (*res)[i] = _data.at(i)->role;
        Z_DELETE(_data.at(i));
    }
    _data.clear();

    return res;
}

void _FlatIndexData::set(const QVariant& v, int role, QLocale::Language language)
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        if ((*i)->role != role)
            continue;
        (*i)->setValue(language, v);
        (*i)->changed = true;
        return;
    }
    _data.resize(_data.size() + 1);
    _data[_data.size() - 1] = Z_NEW(_FlatIndexDataValues, v, role, language);
}

void _FlatIndexData::set(const LanguageMap& v, int role)
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        if ((*i)->role != role)
            continue;
        (*i)->setValues(v);
        (*i)->changed = true;
        return;
    }
    _data.resize(_data.size() + 1);
    _data[_data.size() - 1] = Z_NEW(_FlatIndexDataValues, v, role);
}

void _FlatIndexData::resetChanged(int role)
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        if ((*i)->role != role)
            continue;
        (*i)->changed = false;
        return;
    }
}

void _FlatIndexData::resetChanged()
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        (*i)->changed = false;
    }
}

bool _FlatIndexData::isChanged(int role) const
{
    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        if ((*i)->role != role)
            continue;
        return (*i)->changed;
    }
    return false;
}

void _FlatIndexData::setFlags(const Qt::ItemFlags& f)
{
    if (!_flags)
        _flags = Z_NEW(Qt::ItemFlags, f);
    else
        *_flags = f;
}

QMap<int, QVariant> _FlatIndexData::toMap(QLocale::Language language) const
{
    QMap<int, QVariant> res;

    for (auto i = _data.cbegin(); i != _data.cend(); ++i) {
        res[(*i)->role] = Utils::valueToLanguage((*i)->toLanguageMap(), language);
    }

    return res;
}

QMap<int, LanguageMap> _FlatIndexData::toMap() const
{
    QMap<int, LanguageMap> res;
    for (auto& i : _data) {
        res[i->role] = i->toLanguageMap();
    }
    return res;
}

void _FlatIndexData::fromMap(const QMap<int, QVariant>& v, QLocale::Language language)
{
    if (v.isEmpty()) {
        clear(false); //-V773
        return;
    }

    int old_size = _data.size();

    if (_data.size() != v.size()) {
        if (_data.size() > v.size()) {
            for (int i = _data.size() - 1; i >= v.size(); --i) {
                Z_DELETE(_data.at(i));
            }
        }
        _data.resize(v.size());
    }

    int n = 0;
    for (auto i = v.constBegin(); i != v.constEnd(); ++i) {
        _FlatIndexDataValues* d = (n >= old_size ? nullptr : _data.at(n));
        if (!d) {
            d = Z_NEW(_FlatIndexDataValues, i.value(), i.key(), language);
            _data[n] = d;
        } else {
            d->role = i.key();
            d->setValue(language, i.value());
        }
        n++;
    }
}

void _FlatIndexData::fromMap(const QMap<int, LanguageMap>& v)
{
    clear(false); //-V773
    for (auto i = v.constBegin(); i != v.constEnd(); ++i) {
        set(i.value(), i.key());
    }
}

_FlatIndexData* _FlatIndexData::clone(_FlatRowData* row) const
{
    _FlatIndexData* obj = Z_NEW(_FlatIndexData, row);

    if (_flags)
        obj->_flags = Z_NEW(Qt::ItemFlags, *_flags);

    obj->_data.resize(_data.size());
    for (int i = 0; i < _data.size(); ++i) {
        if (_data.at(i)->multi_values != nullptr)
            obj->_data[i] = Z_NEW(_FlatIndexDataValues, *_data.at(i)->multi_values, _data.at(i)->role);
        else
            obj->_data[i] = Z_NEW(_FlatIndexDataValues, _data.at(i)->single_value == nullptr ? QVariant() : *_data.at(i)->single_value,
                                  _data.at(i)->role, QLocale::Language::AnyLanguage);
    }

    return obj;
}

_FlatRows::_FlatRows(FlatItemModel* model, _FlatRowData* parent)
    : _model(model)
    , _parent(parent)
{
    _column_count = (_parent == nullptr) ? 0 : _parent->columnCount();
}

_FlatRows::~_FlatRows()
{
    for (auto it = _rows.cbegin(); it != _rows.cend(); ++it) {
        if (*it != nullptr)
            Z_DELETE(*it);
    }
}

QModelIndex _FlatRows::parentIndex() const
{
    if (!_model || !_parent)
        return QModelIndex();

    int row = _parent->owner()->findRow(_parent);
    Z_CHECK(row >= 0);

    _FlatIndexData* id = _parent->getData(0);
    return _model->indexFromData(row, 0, id);
}

bool _FlatRows::hasRowData(int row) const
{
    if (row < 0 || row >= _row_count)
        return false;

    if (_rows.size() <= row)
        return false;

    return _rows.at(row) != nullptr;
}

_FlatRowData* _FlatRows::getRow(int row) const
{
    Z_CHECK(row < _row_count);

    _FlatRowData* d;
    if (_rows.size() <= row) {
        // не хватает выделенной памяти - добавляем
        _rows.resize(row + 1);
        d = Z_NEW(_FlatRowData, _column_count, const_cast<_FlatRows*>(this));
        _rows[row] = d;

    } else {
        d = _rows.at(row);
        if (!d) {
            d = Z_NEW(_FlatRowData, _column_count, const_cast<_FlatRows*>(this));
            _rows[row] = d;
        }
    }

    return d;
}

void _FlatRows::setRowCount(int n)
{
    if (_row_count == n)
        return;

    if (_row_count < n)
        insertRows(_row_count, n - _row_count);
    else
        removeRows(n, _row_count - n, false);
}

void _FlatRows::setColumnCount(int n)
{
    if (_column_count == n)
        return;

    if (_column_count < n)
        insertColumns(_column_count, n - _column_count);
    else
        removeColumns(n, _column_count - n);
}

int _FlatRows::findRow(_FlatRowData* row_data) const
{
    if (_last_found_row >= 0 && _last_found_data == row_data)
        return _last_found_row; // есть в кэше

    int position = _rows.indexOf(row_data);
    if (position >= 0) {
        _last_found_data = row_data;
        _last_found_row = position;
        return _last_found_row;
    } else
        return -1;
}

bool _FlatRows::insertRows(int row, int count)
{
    if (row < 0 || count < 0 || row > rowCount())
        return false;

    if (count == 0)
        return true;

    clearCache();

    if (_model)
        _model->beginInsertRows(parentIndex(), row, row + count - 1);

    _row_count += count;

    if (row <= _rows.size()) {
        _rows.insert(row, count, nullptr);
    }

    if (_model) {
        _model->clearMatchCache();
        _model->endInsertRows();
    }

    return true;
}

bool _FlatRows::removeRows(int row, int count, bool keep_objects)
{
    if (row < 0 || count < 0 || row + count > rowCount())
        return false;

    if (count == 0)
        return true;

    clearCache();

    if (_model)
        _model->beginRemoveRows(parentIndex(), row, row + count - 1);

    _row_count -= count;
    if (row < _rows.size()) {
        if (!keep_objects) {
            Utils::zDeleteAllCustom(Utils::takeVector<_FlatRowData*>(_rows, row, count));
        } else
            Utils::takeVector<_FlatRowData*>(_rows, row, count);
    }

    if (_model) {
        _model->clearMatchCache();
        _model->endRemoveRows();
    }

    return true;
}

bool _FlatRows::insertColumns(int column, int count)
{
    if (column < 0 || count < 0 || column > _column_count)
        return false;

    if (count == 0)
        return true;

    clearCache();

    if (_model)
        _model->beginInsertColumns(parentIndex(), column, column + count - 1);

    _column_count += count;
    for (auto i = _rows.cbegin(); i != _rows.cend(); ++i) {
        if ((*i) != nullptr)
            (*i)->insertColumns(column, count, false);
    }

    if (_model) {
        _model->clearMatchCache();
        _model->endInsertColumns();
    }

    return true;
}

bool _FlatRows::removeColumns(int column, int count)
{
    if (column < 0 || count < 0 || column + count > _column_count)
        return false;

    if (count == 0)
        return true;

    clearCache();

    if (_model)
        _model->beginRemoveColumns(parentIndex(), column, column + count - 1);

    _column_count -= count;
    for (auto i = _rows.cbegin(); i != _rows.cend(); ++i) {
        if ((*i) != nullptr)
            (*i)->removeColumns(column, count, false);
    }

    if (_model) {
        _model->clearMatchCache();
        _model->endRemoveColumns();
    }

    return true;
}

bool _FlatRows::moveRows(int source, int count, int destination)
{
    if (source < 0 || destination < 0 || count < 0 || source + count > rowCount() || destination + count > rowCount())
        return false;

    if (source == destination || count == 0)
        return true;

    Utils::moveVector<_FlatRowData*>(_rows, source, count, destination);

    return true;
}

bool _FlatRows::moveColumns(int source, int count, int destination)
{
    if (source < 0 || destination < 0 || count < 0 || source + count > columnCount() || destination + count > columnCount())
        return false;

    if (source == destination || count == 0)
        return true;

    if (_row_count > 0) {
        for (auto i = _rows.cbegin(); i != _rows.cend(); ++i) {
            if ((*i) != nullptr)
                Z_CHECK((*i)->moveColumns(source, count, destination));
        }
    }

    return true;
}

QList<_FlatRowData*> _FlatRows::takeRows(int row, int count)
{
    Z_CHECK(count > 0);
    if (row + count > _row_count)
        return QList<_FlatRowData*>();

    QList<_FlatRowData*> rows;
    for (int i = row; i < row + count; i++) {
        _FlatRowData* r_data = getRow(i);
        rows << r_data;
        r_data->setOwner(nullptr);
    }

    removeRows(row, count, true);
    return rows;
}

bool _FlatRows::putRows(const QList<_FlatRowData*>& rows, int row)
{
    if (row < 0 || row > _row_count)
        return false;

    if (rows.isEmpty())
        return true;

    Z_CHECK(insertRows(row, rows.count()));

    for (int i = row; i < row + rows.count(); i++) {
        Z_CHECK(replaceRow(i, rows.at(i - row)));
    }

    return true;
}

bool _FlatRows::replaceRow(int row, _FlatRowData* row_data)
{
    Z_CHECK(row_data);

    if (row < 0 || row >= _row_count)
        return false;

    Z_DELETE(getRow(row));

    row_data->setOwner(this);
    _rows[row] = row_data;

    return true;
}

_FlatRows* _FlatRows::clone(_FlatRowData* parent, bool clone_children) const
{
    _FlatRows* obj = Z_NEW(_FlatRows, nullptr, parent);

    obj->_row_count = _row_count;
    obj->_column_count = _column_count;
    obj->_rows.resize(_rows.size());

    for (int i = 0; i < _rows.size(); ++i) {
        if (_rows.at(i) == nullptr)
            obj->_rows[i] = nullptr;
        else
            obj->_rows[i] = _rows.at(i)->clone(const_cast<_FlatRows*>(this), clone_children);
    }

    return obj;
}

void _FlatRows::alloc()
{
    if (_row_count == 0)
        return;

    for (int row = _row_count - 1;; --row) {
        getRow(row)->alloc();

        if (row == 0)
            break;
    }
}

void _FlatRows::resetChanged()
{
    for (auto& r : qAsConst(_rows)) {
        if (r == nullptr)
            continue;

        r->resetChanged();
    }
}

void _FlatRows::clearCache()
{
    _last_found_row = -1;
    _last_found_data = nullptr;
}

void _FlatRows::setItemModel(FlatItemModel* m)
{
    _model = m;
    for (auto& r : qAsConst(_rows)) {
        if (r == nullptr || r->_children == nullptr)
            continue;
        r->_children->setItemModel(m);
    }
}

_FlatIndexDataValues::_FlatIndexDataValues(const QVariant& value, int role, QLocale::Language language)
    : role(role)
{
    setValue(language, value);
}

_FlatIndexDataValues::_FlatIndexDataValues(const LanguageMap& values, int role)
    : role(role)
{
    setValues(values);
}

_FlatIndexDataValues::~_FlatIndexDataValues()
{
    if (multi_values != nullptr)
        Z_DELETE(multi_values);
    if (single_value != nullptr)
        Z_DELETE(single_value);
}

QVariant* _FlatIndexDataValues::getValue(QLocale::Language language) const
{
    if (single_value != nullptr) {
        if (language == QLocale::Language::AnyLanguage)
            return single_value;
        return nullptr;
    }
    if (multi_values != nullptr) {
        auto res = multi_values->find(language);
        return res == multi_values->end() ? nullptr : &res.value();
    }

    return nullptr;
}

void _FlatIndexDataValues::setValue(QLocale::Language lang, const QVariant& value)
{
    if (multi_values != nullptr) {
        multi_values->operator[](lang) = value;
        return;
    }

    if (lang != QLocale::Language::AnyLanguage) {
        multi_values = Z_NEW(LanguageMap);
        multi_values->operator[](lang) = value;
        if (single_value != nullptr) {
            Z_DELETE(single_value);
            single_value = nullptr;
        }

        return;
    }

    if (single_value == nullptr)
        single_value = Z_NEW(QVariant, value);
    else
        *single_value = value;
}

void _FlatIndexDataValues::setValues(const LanguageMap& v)
{
    if (multi_values == nullptr) {
        multi_values = Z_NEW(LanguageMap);
        if (single_value != nullptr) {
            Z_DELETE(single_value);
            single_value = nullptr;
        }
    }
    *multi_values = v;
}

LanguageMap _FlatIndexDataValues::toLanguageMap() const
{
    if (multi_values != nullptr)
        return *multi_values;
    if (single_value != nullptr)
        return LanguageMap {{QLocale::Language::AnyLanguage, *single_value}};
    return LanguageMap();
}

_FlatItemModel_data::_FlatItemModel_data()
    : _d(_FlatItemModel_SharedData::shared_null())
{
}

_FlatItemModel_data::_FlatItemModel_data(FlatItemModel* model)
    : _d(new _FlatItemModel_SharedData(model))
{    
}

_FlatItemModel_data::_FlatItemModel_data(const _FlatItemModel_data& d)
    : _d(d._d)
{    
}

_FlatItemModel_data::~_FlatItemModel_data()
{    
}

_FlatItemModel_data& _FlatItemModel_data::operator=(const _FlatItemModel_data& d)
{
    if (this != &d) {
        detachHelper();
        _d = d._d;
    }

    return *this;
}

void _FlatItemModel_data::detachHelper()
{
    if (_d == _FlatItemModel_SharedData::shared_null())
        _d.detach();
}

Q_GLOBAL_STATIC(_FlatItemModel_SharedData, nullResult)
_FlatItemModel_SharedData* _FlatItemModel_SharedData::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0) {
        res->ref.ref(); // чтобы не было удаления самого nullResult
    }
    return res;
}

_FlatItemModel_SharedData::_FlatItemModel_SharedData()
{
}

_FlatItemModel_SharedData::_FlatItemModel_SharedData(FlatItemModel* model)
{
    Z_CHECK_NULL(model);
    model->disconnect(model, nullptr, nullptr, nullptr);
    model->setParent(nullptr);
    _model = std::shared_ptr<FlatItemModel>(model);
}

_FlatItemModel_SharedData::_FlatItemModel_SharedData(const _FlatItemModel_SharedData& d)
    : QSharedData(d)
{
    copyFrom(&d);
}

_FlatItemModel_SharedData::~_FlatItemModel_SharedData()
{
}

void _FlatItemModel_SharedData::copyFrom(const _FlatItemModel_SharedData* d)
{
    _model = d->_model;
}

} // namespace zf
