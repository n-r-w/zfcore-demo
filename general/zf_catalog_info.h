#pragma once

#include "zf_uid.h"
#include <QDataStream>

namespace zf
{
class CatalogInfo;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::CatalogInfo& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::CatalogInfo& obj);

namespace zf
{
class DataStructure;
typedef std::shared_ptr<DataStructure> DataStructurePtr;

//! Информация о каталоге
class ZCORESHARED_EXPORT CatalogInfo
{
public:
    CatalogInfo();
    CatalogInfo(const CatalogInfo& i);
    CatalogInfo(const Uid& uid, DataStructurePtr structure, const CatalogOptions& options);

    CatalogInfo& operator=(const CatalogInfo& i);

    bool isValid() const;

    //! Идентификатор каталога
    Uid uid() const;
    //! Структура каталога
    DataStructurePtr structure() const;
    //! Параметры
    CatalogOptions options() const;

private:
    //! Идентификатор каталога
    Uid _uid;
    //! Структура каталога
    DataStructurePtr _structure;
    //! Параметры
    CatalogOptions _options = {};

    friend QDataStream& ::operator<<(QDataStream& out, const zf::CatalogInfo& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::CatalogInfo& obj);
};
typedef std::shared_ptr<CatalogInfo> CatalogInfoPtr;

} // namespace zf

Q_DECLARE_METATYPE(zf::CatalogInfo)
