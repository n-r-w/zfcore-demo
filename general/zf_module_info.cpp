#include "zf_module_info.h"
#include "zf_core.h"
#include "zf_data_structure.h"

namespace zf
{
ModuleInfo::ModuleInfo()   
{
}

ModuleInfo::ModuleInfo(const zf::EntityCode& code, zf::ModuleType type, const QString& translation_id, const zf::Version& version,
                       const DataStructurePtr& data_structure, const QIcon& icon, const OperationList& operations)
    : _code(code)
    , _type(type)
    , _translation_id(translation_id)
    , _version(version)
    , _icon(icon)
    , _data_structure(data_structure)
    , _operations(operations)
{
    Z_CHECK(_code.isValid());
    Z_CHECK(_type != ModuleType::Invalid);
    Z_CHECK(!_translation_id.isEmpty());

    if (_data_structure != nullptr) {
        Z_CHECK(!_data_structure->isEntityIndependent());
        Z_CHECK(_data_structure->entity().isValid() && _data_structure->entity().entityCode() == _code);
    }

    QSet<OperationID> ids_unique;
    for (auto& o : _operations) {
        Z_CHECK_X(o.entityCode() == _code, QString("Использование операции из другого модуля %1:%2 в модуле %3")
                                               .arg(o.entityCode().string(), o.id().string(), _code.string()));
        Z_CHECK_X(!ids_unique.contains(o.id()), QString("Дублирование операции %1:%2").arg(_code.string(), o.id().string()));
        ids_unique << o.id();
    }
}

ModuleInfo::ModuleInfo(const ModuleInfo& mi)
    : _code(mi._code)    
    , _type(mi._type)
    , _translation_id(mi._translation_id)
    , _version(mi._version)
    , _icon(mi._icon)
    , _data_structure(mi._data_structure)
    , _operations(mi._operations)
{
}

ModuleInfo ModuleInfo::createWithDataStructure(const DataStructurePtr& data_structure) const
{
    Z_CHECK_NULL(data_structure);
    Z_CHECK(_data_structure == nullptr);

    ModuleInfo info = *this;
    info._data_structure = data_structure;
    return info;
}

DataProperty ModuleInfo::property() const
{
    Z_CHECK(isValid());
    Z_CHECK_NULL(_data_structure);
    return _data_structure->entity();
}

QString ModuleInfo::name() const
{
    return Translator::translate(_translation_id);
}

DataStructurePtr ModuleInfo::dataStructure() const
{
    Z_CHECK(isValid());
    Z_CHECK_NULL(_data_structure);
    return _data_structure;
}

bool ModuleInfo::isDataStructureInitialized() const
{
    return _data_structure != nullptr;
}

Uid ModuleInfo::uid() const
{
    return isValid() ? Core::pluginUid(_code) : Uid();
}

bool ModuleInfo::isValid() const
{
    return _code.isValid();
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::ModuleInfo& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    using namespace zf;

    if (!c.isValid()) {
        debug << "invalid";

    } else {
        debug << '\n';

        Core::beginDebugOutput();
        debug << Core::debugIndent() << "code: " << c.code() << '\n';
        debug << Core::debugIndent() << "property:" << c.property() << '\n';
        debug << Core::debugIndent() << "type: " << c.type() << '\n';
        debug << Core::debugIndent() << "name: " << c.name() << '\n';
        debug << Core::debugIndent() << "translationId: " << c.translationId() << '\n';
        debug << Core::debugIndent() << "version: " << c.version() << '\n';
        debug << Core::debugIndent() << "icon: " << !c.icon().isNull() << '\n';

        debug << Core::debugIndent() << "operations: ";
        if (!c.operations().isEmpty()) {
            debug << '\n';
            Core::beginDebugOutput();
            for (int i = 0; i < c.operations().count(); i++) {
                debug << Core::debugIndent() << c.operations().at(i);
                if (i < c.operations().count() - 1)
                    debug << "\n";
            }
            zf::Core::endDebugOutput();
        }

        zf::Core::endDebugOutput();
    }

    return debug;
}
