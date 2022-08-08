#include "zf_catalog_info.h"
#include "zf_data_structure.h"
#include "zf_core.h"

namespace zf
{
CatalogInfo::CatalogInfo()
{
}

CatalogInfo::CatalogInfo(const CatalogInfo& i)
{
    operator=(i);
}

CatalogInfo::CatalogInfo(const Uid& uid, DataStructurePtr structure, const CatalogOptions& options)
    : _uid(uid)
    , _structure(structure)
    , _options(options)
{
    Z_CHECK(_uid.isValid());
    Z_CHECK_NULL(structure);
}

CatalogInfo& CatalogInfo::operator=(const CatalogInfo& i)
{
    _uid = i._uid;
    _structure = i._structure;
    _options = i._options;
    return *this;
}

bool CatalogInfo::isValid() const
{
    return _uid.isValid();
}

Uid CatalogInfo::uid() const
{
    return _uid;
}

DataStructurePtr CatalogInfo::structure() const
{
    return _structure;
}

CatalogOptions CatalogInfo::options() const
{
    return _options;
}

} // namespace zf

QDataStream& operator<<(QDataStream& out, const zf::CatalogInfo& obj)
{
    return out << obj._uid << obj._structure << obj._options;
}

QDataStream& operator>>(QDataStream& in, zf::CatalogInfo& obj)
{
    return in >> obj._uid >> obj._structure >> obj._options;
}
