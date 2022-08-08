#include "zf_mime.h"
#include <QMimeData>

namespace zf
{
const QString MimeEntityList::MIME_TYPE = "zf_mimeentitylist";
const QString MimeDataContainer::MIME_TYPE = "zf_mimedatacontainer";

MimeEntityList::MimeEntityList()
{
}

MimeEntityList::MimeEntityList(const MimeEntityList& m)
{
    copyFrom(m);
}

MimeEntityList::MimeEntityList(const EntityCode& entity_code, const EntityIDList& ids, const DatabaseID& database_id)
    : _is_valid(true)
    , _single_checked(true)
    , _entity_code(entity_code)
    , _database_id(database_id)
    , _ids(std::make_unique<EntityIDList>(ids))
{
    Z_CHECK(entity_code.isValid());
}

MimeEntityList::MimeEntityList(const UidList& entities)
    : _is_valid(true)
    , _entities(std::make_unique<UidList>(entities))
{
}

MimeEntityList& MimeEntityList::operator=(const MimeEntityList& m)
{
    copyFrom(m);
    return *this;
}

bool MimeEntityList::isValid() const
{
    return _is_valid;
}

bool MimeEntityList::isEmpty() const
{
    if (!_is_valid)
        return true;

    if (_entities != nullptr)
        return _entities->isEmpty();

    if (_ids != nullptr)
        return _ids->isEmpty();

    return true;
}

bool MimeEntityList::isSingleEntityCode() const
{
    if (!_is_valid || _entity_code.isValid() || isEmpty())
        return true;

    if (_ids != nullptr)
        return true;

    Z_CHECK(_entities != nullptr);
    fill_ids();

    return _entity_code.isValid();
}

UidList MimeEntityList::entities() const
{
    if (!_is_valid)
        return {};

    fill_entities();
    return *_entities;
}

EntityCode MimeEntityList::entityCode() const
{
    if (!_is_valid)
        return {};

    fill_ids();
    Z_CHECK(isSingleEntityCode());
    return _entity_code;
}

EntityIDList MimeEntityList::ids() const
{
    if (!_is_valid)
        return {};

    fill_ids();
    Z_CHECK(isSingleEntityCode());
    return _ids == nullptr ? EntityIDList() : *_ids;
}

DatabaseID MimeEntityList::databaseId() const
{
    if (!_is_valid)
        return DatabaseID();

    fill_ids();
    Z_CHECK(isSingleEntityCode());
    return _database_id;
}

void MimeEntityList::applyData(QMimeData* data) const
{
    Z_CHECK_NULL(data);
    data->setData(MIME_TYPE, toByteArray());    
}

MimeEntityList MimeEntityList::fromData(const QMimeData* data)
{
    Z_CHECK_NULL(data);
    MimeEntityList res;
    if (data->hasFormat(MIME_TYPE))
        res = fromByteArray(data->data(MIME_TYPE));

    return res;
}

static int _MimeEntityList_stream_version = 1;
QByteArray MimeEntityList::toByteArray() const
{
    QByteArray b;
    QDataStream st(&b, QIODevice::WriteOnly);
    st.setVersion(Consts::DATASTREAM_VERSION);

    st << _MimeEntityList_stream_version;

    st << _is_valid;
    if (_is_valid) {
        st << (_entities != nullptr);
        if (_entities != nullptr)
            st << *_entities;

        st << _single_checked;
        st << _entity_code;
        st << _database_id;

        st << (_ids != nullptr);
        if (_ids != nullptr)
            st << *_ids;
    }

    return b;
}

MimeEntityList MimeEntityList::fromByteArray(const QByteArray& b)
{
    QDataStream ds(b);
    ds.setVersion(Consts::DATASTREAM_VERSION);

    MimeEntityList res;

    int version;
    ds >> version;
    if (version != _MimeEntityList_stream_version)
        return res;

    ds >> res._is_valid;
    if (res._is_valid) {
        bool has_value;
        ds >> has_value;
        if (has_value) {
            UidList entities;
            ds >> entities;
            res._entities = std::make_unique<UidList>(entities);
        }

        ds >> res._single_checked;
        ds >> res._entity_code;
        ds >> res._database_id;

        ds >> has_value;
        if (has_value) {
            EntityIDList ids;
            ds >> ids;
            res._ids = std::make_unique<EntityIDList>(ids);
        }
    }

    return ds.status() == QDataStream::Ok ? res : MimeEntityList();
}

void MimeEntityList::copyFrom(const MimeEntityList& m)
{
    _is_valid = m._is_valid;
    _entities = (m._entities != nullptr ? std::make_unique<UidList>(*m._entities.get()) : nullptr);
    _single_checked = m._single_checked;
    _entity_code = m._entity_code;
    _database_id = m._database_id;
    _ids = (m._ids != nullptr ? std::make_unique<EntityIDList>(*m._ids.get()) : nullptr);
}

void MimeEntityList::fill_entities() const
{
    if (_entities != nullptr || isEmpty())
        return;

    if (_ids == nullptr)
        return;

    Z_CHECK(_entity_code.isValid());

    _entities = std::make_unique<UidList>();
    for (auto& i : qAsConst(*_ids)) {
        _entities->append(Uid::entity(_entity_code, i, _database_id));
    }
}

void MimeEntityList::fill_ids() const
{
    if (_single_checked)
        return;

    Z_CHECK(_entity_code.isValid());
    Z_CHECK(_ids == nullptr);

    _single_checked = true;

    if (_entities == nullptr || _entities->isEmpty())
        return;

    _ids = std::make_unique<EntityIDList>();
    EntityCode code;
    DatabaseID db;

    for (auto& i : qAsConst(*_entities)) {
        if (!code.isValid()) {
            code = i.entityCode();
            db = i.database();
        }

        if (code != i.entityCode() || db != i.database()) {
            _ids.reset();
            return;
        }

        _ids->append(i.id());
    }

    _entity_code = code;
    _database_id = db;
}

MimeDataContainer::MimeDataContainer()
{
}

MimeDataContainer::MimeDataContainer(const MimeDataContainer& m)

{
    copyFrom(m);
}

MimeDataContainer::MimeDataContainer(const DataContainer& c)
    : _data(new DataContainer(c))
{
}

MimeDataContainer::~MimeDataContainer()
{
    if (_data != nullptr)
        delete _data;
}

MimeDataContainer& MimeDataContainer::operator=(const MimeDataContainer& m)
{
    copyFrom(m);
    return *this;
}

bool MimeDataContainer::isValid() const
{
    return _data != nullptr;
}

DataContainer MimeDataContainer::data() const
{
    return _data == nullptr ? DataContainer() : *_data;
}

void MimeDataContainer::applyData(QMimeData* data) const
{
    Z_CHECK_NULL(data);
    data->setData(MIME_TYPE, toByteArray());
}

MimeDataContainer MimeDataContainer::fromData(const QMimeData* data)
{
    Z_CHECK_NULL(data);
    MimeDataContainer res;
    if (data->hasFormat(MIME_TYPE))
        res = fromByteArray(data->data(MIME_TYPE));

    return res;
}

static int _MimeDataContainer_stream_version = 1;
QByteArray MimeDataContainer::toByteArray() const
{
    QByteArray b;
    QDataStream st(&b, QIODevice::WriteOnly);
    st.setVersion(Consts::DATASTREAM_VERSION);

    st << _MimeDataContainer_stream_version;
    st << isValid();
    if (isValid()) {
        st << _data->isValid();
        if (_data->isValid())
            st << *_data;
    }

    return b;
}

MimeDataContainer MimeDataContainer::fromByteArray(const QByteArray& b)
{
    QDataStream ds(b);
    ds.setVersion(Consts::DATASTREAM_VERSION);

    MimeDataContainer res;

    int version;
    ds >> version;
    if (version != _MimeDataContainer_stream_version)
        return res;

    bool valid;
    ds >> valid;
    if (valid) {
        ds >> valid;
        if (valid) {
            DataContainer c;
            ds >> c;
            res = MimeDataContainer(c);

        } else {
            res = MimeDataContainer(DataContainer());
        }
    }

    return ds.status() == QDataStream::Ok ? res : MimeDataContainer();
}

void MimeDataContainer::copyFrom(const MimeDataContainer& m)
{
    if (&m == this)
        return;

    if (_data != nullptr)
        delete _data;

    _data = m._data != nullptr ? new DataContainer(*m._data) : nullptr;
}

} // namespace zf

QDataStream& operator<<(QDataStream& s, const zf::MimeEntityList& c)
{
    return s << c.toByteArray();
}

QDataStream& operator>>(QDataStream& s, zf::MimeEntityList& c)
{
    QByteArray ba;
    s >> ba;
    c.fromByteArray(ba);
    return s;
}

QDataStream& operator<<(QDataStream& s, const zf::MimeDataContainer& c)
{
    return s << c.toByteArray();
}

QDataStream& operator>>(QDataStream& s, zf::MimeDataContainer& c)
{
    QByteArray ba;
    s >> ba;
    c.fromByteArray(ba);
    return s;
}
