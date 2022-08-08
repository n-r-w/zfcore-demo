#include "zf_row_id.h"
#include "zf_core.h"

namespace zf
{
//! Разделяемые данные для RowID
class RowID_data : public QSharedData
{
public:
    RowID_data();
    RowID_data(const QString& key, const QString& unique_id, const QModelIndex& index);
    RowID_data(const RowID_data& d);
    ~RowID_data();

    void copyFrom(const RowID_data* d);
    void setPositionInfo(const QModelIndex& index);

    static RowID_data* shared_null();

    void clear();

    QString* _key = nullptr;
    QString* _unique_id = nullptr;
    QString* _row_based_id = nullptr;
    QList<int>* _rows = nullptr;
    uint _hash_value = 0;
};

Q_GLOBAL_STATIC(RowID_data, nullResult)
RowID_data* RowID_data::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0)
        res->ref.ref(); // чтобы не было удаления самого nullResult
    return res;
}

RowID_data::RowID_data()
{
    Z_DEBUG_NEW("RowID_data");
}

RowID_data::RowID_data(const QString& key, const QString& unique_id, const QModelIndex& index)
{
    if (!key.isEmpty())
        _key = Z_NEW(QString, key);

    if (!unique_id.isEmpty())
        _unique_id = Z_NEW(QString, unique_id);

    setPositionInfo(index);

    if (_key != nullptr)
        _hash_value = ::qHash(*_key + Consts::KEY_SEPARATOR + QChar('K'));
    else if (_unique_id != nullptr)
        _hash_value = ::qHash(*_unique_id + Consts::KEY_SEPARATOR + QChar('U'));
    else
        _hash_value = 0;
}

RowID_data::RowID_data(const RowID_data& d)
    : QSharedData(d)
{
    copyFrom(&d);
    Z_DEBUG_NEW("RowID_data");
}

RowID_data::~RowID_data()
{
    Z_DEBUG_DELETE("RowID_data");
    if (_key != nullptr)
        Z_DELETE(_key);
    if (_unique_id != nullptr)
        Z_DELETE(_unique_id);
    if (_row_based_id != nullptr)
        Z_DELETE(_row_based_id);
    if (_rows != nullptr)
        Z_DELETE(_rows);
}

void RowID_data::copyFrom(const RowID_data* d)
{
    _key = d->_key == nullptr ? nullptr : Z_NEW(QString, *d->_key);
    _unique_id = d->_unique_id == nullptr ? nullptr : Z_NEW(QString, *d->_unique_id);
    _row_based_id = d->_row_based_id == nullptr ? nullptr : Z_NEW(QString, *d->_row_based_id);
    _rows = d->_rows == nullptr ? nullptr : Z_NEW(QList<int>, *d->_rows);
    _hash_value = d->_hash_value;
}

void RowID_data::setPositionInfo(const QModelIndex& index)
{
    if (_row_based_id != nullptr) {
        Z_DELETE(_row_based_id);
        _row_based_id = nullptr;
        Z_DELETE(_rows);
        _rows = nullptr;
    }

    if (!index.isValid())
        return;

    _row_based_id = Z_NEW(QString);
    _rows = Z_NEW(QList<int>);

    QModelIndex idx = index.model()->index(index.row(), 0, index.parent());
    while (idx.isValid()) {
        _row_based_id->prepend(QString::number(idx.row()) + Consts::KEY_SEPARATOR);
        _rows->prepend(idx.row());
        idx = idx.parent();
    }
}

void RowID_data::clear()
{
    if (_key != nullptr) {
        Z_DELETE(_key);
        _key = nullptr;
    }
    if (_unique_id != nullptr) {
        Z_DELETE(_unique_id);
        _unique_id = nullptr;
    }
    if (_row_based_id != nullptr) {
        Z_DELETE(_row_based_id);
        _row_based_id = nullptr;
    }
    if (_rows != nullptr) {
        Z_DELETE(_rows);
        _rows = nullptr;
    }
    _hash_value = 0;
}

RowID::RowID()
    : _d(RowID_data::shared_null())
{
}

RowID::RowID(const RowID& u)
    : _d(u._d)
{
}

RowID::~RowID()
{
}

RowID RowID::createRealKey(int id, const QModelIndex& index)
{
    return createRealKey(QString::number(id), index);
}

RowID RowID::createRealKey(const QString& key, const QModelIndex& index)
{
    Z_CHECK(!key.isEmpty());
    return RowID(key, QString(), index);
}

RowID RowID::createRealKey(const EntityID& id, const QModelIndex& index)
{
    Z_CHECK(id.isValid());
    return createRealKey(id.value(), index);
}

RowID RowID::createRealKey(const QModelIndex& index)
{
    Z_CHECK(index.isValid());
    QModelIndex idx = index.model()->index(index.row(), 0, index.parent());
    QString key;
    while (idx.isValid()) {
        key.prepend(QString::number(idx.row()) + Consts::KEY_SEPARATOR);
        idx = idx.parent();
    }
    return createRealKey(key, index);
}

RowID RowID::createGenerated(const QModelIndex& index)
{
    return RowID(QString(), Utils::generateUniqueString(), index);
}

RowID::RowID(const QString& key, const QString& unique_id, const QModelIndex& index)
    : _d(new RowID_data(key, unique_id, index))
{    
}

bool RowID::operator==(const RowID& u) const
{
    if (_d == u._d)
        return true;

    if (_d->_hash_value == 0 && u._d->_hash_value == 0)
        return true; // оба пустые

    if (_d->_key != nullptr && u._d->_key != nullptr)
        return *(_d->_key) == *(u._d->_key);

    if (_d->_unique_id != nullptr && u._d->_unique_id != nullptr)
        return *(_d->_unique_id) == *(u._d->_unique_id);

    return false;
}

bool RowID::operator!=(const RowID& u) const
{
    return !operator==(u);
}

bool RowID::operator<(const RowID& u) const
{
    if (_d == u._d
        || (
            // оба пустые
            _d->_hash_value == 0 && u._d->_hash_value == 0))
        return false;

    if (_d->_key != nullptr && u._d->_key != nullptr)
        return *(_d->_key) < *(u._d->_key);

    if (_d->_row_based_id != nullptr && u._d->_row_based_id != nullptr)
        return *(_d->_row_based_id) < *(u._d->_row_based_id);

    if (_d->_unique_id != nullptr && u._d->_unique_id != nullptr)
        return *(_d->_unique_id) < *(u._d->_unique_id);

    if (u._d->_unique_id != nullptr)
        return true; // сгенерированные всегда "больше"

    return false;
}

RowID& RowID::operator=(const RowID& u)
{
    if (this != &u)
        _d = u._d;
    return *this;
}

bool RowID::isValid() const
{
    return _d->_key != nullptr || _d->_unique_id != nullptr;
}

void RowID::clear()
{
    _d->clear();
}

bool RowID::isRealKey() const
{
    return _d->_key != nullptr;
}

bool RowID::isGenerated() const
{
    return _d->_unique_id != nullptr;
}

bool RowID::hasPositionInfo() const
{
    return _d->_row_based_id != nullptr;
}

void RowID::setPositionInfo(const QModelIndex& index)
{
    _d->setPositionInfo(index);
}

QString RowID::key() const
{
    Z_CHECK(isRealKey());
    return *_d->_key;
}

const QList<int>& RowID::positionInfo() const
{
    Z_CHECK(hasPositionInfo());
    return *_d->_rows;
}

QModelIndex RowID::restoreModelIndex(const QAbstractItemModel* model, int column) const
{
    Z_CHECK(hasPositionInfo());
    Z_CHECK_NULL(model);
    Z_CHECK(column >= 0);

    QModelIndex index;
    for (int row : qAsConst(*_d->_rows)) {
        QModelIndex p_index = model->index(row, 0, index);
        if (!p_index.isValid()) {
            // если такой строки нет, то берем максимальную
            p_index = model->index(model->rowCount(index) - 1, 0, index);
            if (p_index.isValid())
                index = p_index;
            break;
        }
        index = p_index;
    }

    return index.isValid() ? model->index(index.row(), column, index.parent()) : index;
}

QVariant RowID::variant() const
{
    return QVariant::fromValue(*this);
}

RowID RowID::fromVariant(const QVariant& v)
{
    return v.value<RowID>();
}

uint RowID::hashValue() const
{
    return _d->_hash_value;
}

QString RowID::uniqueKey() const
{
    if (isRealKey())
        return *_d->_key;
    if (isGenerated())
        return *_d->_unique_id;
    return QStringLiteral("E");
}

QString RowID::toPrintable() const
{
    if (!isValid())
        return QStringLiteral("Invalid");

    return  (isGenerated()? QStringLiteral("G:") : QStringLiteral("R:")) + uniqueKey();
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::RowID& c)
{
    if (c._d->_key != nullptr)
        s << *c._d->_key;
    else
        s << QString();

    if (c._d->_unique_id != nullptr)
        s << *c._d->_unique_id;
    else
        s << QString();

    if (c._d->_row_based_id != nullptr)
        s << *c._d->_row_based_id;
    else
        s << QString();

    if (c._d->_rows != nullptr)
        s << *c._d->_rows;
    else
        s << QList<int>();

    return s << c._d->_hash_value;
}

QDataStream& operator>>(QDataStream& s, zf::RowID& c)
{
    QString str;

    s >> str;
    if (!str.isEmpty()) {
        c._d->_key = Z_NEW(QString, str);
    } else {
        if (c._d->_key != nullptr) {
            Z_DELETE(c._d->_key);
            c._d->_key = nullptr;
        }
    }

    s >> str;
    if (!str.isEmpty()) {
        c._d->_unique_id = Z_NEW(QString, str);
    } else {
        if (c._d->_unique_id != nullptr) {
            Z_DELETE(c._d->_unique_id);
            c._d->_unique_id = nullptr;
        }
    }

    s >> str;
    if (!str.isEmpty()) {
        c._d->_row_based_id = Z_NEW(QString, str);
    } else {
        if (c._d->_row_based_id != nullptr) {
            Z_DELETE(c._d->_row_based_id);
            c._d->_row_based_id = nullptr;
        }
    }

    QList<int> rows;
    s >> rows;
    if (!str.isEmpty()) {
        c._d->_rows = Z_NEW(QList<int>, rows);
    } else {
        if (c._d->_rows != nullptr) {
            Z_DELETE(c._d->_rows);
            c._d->_rows = nullptr;
        }
    }

    return s >> c._d->_hash_value;
}

QDebug operator<<(QDebug debug, const zf::RowID& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    if (!c.isValid()) {
        debug << "invalid";

    } else if (c.isRealKey()) {
        if (c.hasPositionInfo())
            debug << "position:" << c.positionInfo() << ", key:" << c.key();
        else
            debug << "key:" << c.key();

    } else {
        Z_CHECK(c.isGenerated());
        if (c.hasPositionInfo())
            debug << "position:" << c.positionInfo() << ", key:" << c.uniqueKey();
        else
            debug << "key:" << c.uniqueKey();
    }

    return debug;
}
