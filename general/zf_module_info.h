#ifndef ZF_MODULE_INFO_H
#define ZF_MODULE_INFO_H

#include "zf.h"
#include "zf_data_structure.h"
#include "zf_operation.h"
#include "zf_version.h"

namespace zf
{
class DataStructure;
typedef std::shared_ptr<DataStructure> DataStructurePtr;

//! Общая информация по модулю
class ZCORESHARED_EXPORT ModuleInfo
{
    Q_GADGET
public:
    ModuleInfo();
    ModuleInfo(
        //! Уникальный код модуля
        const EntityCode& entity_code,
        //! Вид модуля
        ModuleType type,
        //! Идентификатор перевода для названия
        const QString& translation_id,
        //! Версия
        const Version& version,
        //! Структура данных. Если не задана, то определяется динамически при создании сущности с конкретным
        //! идентификатором
        const DataStructurePtr& data_structure = nullptr,
        //! Иконка
        const QIcon& icon = QIcon(),
        //! Список поддерживаемых операций
        const OperationList& operations = OperationList());
    ModuleInfo(const ModuleInfo& mi);
    ModuleInfo& operator=(const ModuleInfo& mi) = default;

    //! Уникальный код модуля
    EntityCode code() const { return _code; }
    //! Уникальный признак модуля в виде DataProperty
    DataProperty property() const;
    //! Вид модуля
    ModuleType type() const { return _type; }
    //! Название
    QString name() const;
    //! ID перевода
    QString translationId() const { return _translation_id; }
    //! Версия
    Version version() const { return _version; }
    //! Иконка
    QIcon icon() const { return _icon; }
    //! Структура данных
    DataStructurePtr dataStructure() const;
    //! Инициализирована ли структура данных
    bool isDataStructureInitialized() const;
    //! Список поддерживаемых операций
    OperationList operations() const { return _operations; }

    //! Уникальный идентификатор модуля (плагина)
    Uid uid() const;

    bool isValid() const;

    //! Создать копию объекта с установленной в него структурой данных (только для объектов, которые были изначально не
    //! инициализирвованы конкретным DataStructure)
    ModuleInfo createWithDataStructure(const DataStructurePtr& data_structure) const;

private:
    EntityCode _code;
    ModuleType _type = ModuleType::Invalid;
    QString _translation_id;
    Version _version;
    QIcon _icon;
    DataStructurePtr _data_structure;
    //! Список поддерживаемых операций
    OperationList _operations;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::ModuleInfo);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::ModuleInfo& c);

#endif // ZF_MODULE_INFO_H
