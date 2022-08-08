#pragma once

#include "zf.h"
#include <QMetaEnum>

namespace zf
{
//! Размерность массива count_of(массив)
#if QT_VERSION >= 0x040800
#if defined(Q_OS_WIN)
#define count_of(x) _countof(x)
#else // !Q_OS_WIN
template <typename T, size_t N> inline size_t count_of(T (&)[N])
{
    return N;
}
#endif // !Q_OS_WIN
#else
#define count_of(x) (sizeof(x) / sizeof(x[0]))
#endif

/*! Экстренная остановка приложения. На вход можно подавать строки и Error */
#ifdef PVS_STUDIO
#define Z_HALT(_text_) abort()
#else
#define Z_HALT(_text_) ::zf::Utils::fatalErrorHelper(_text_, __FILE__, __FUNCTION__, __LINE__)
#endif

#define Z_HALT_INT Z_HALT("Internal error")

//! В режиме отладки - завершение приложения, иначе запись в журнал
#if defined(Z_DEBUG_MODE)
#define Z_HALT_DEBUG(_text_) Z_HALT(_text_)
#else
#ifdef PVS_STUDIO
#define Z_HALT_DEBUG(_text_) abort()
#else
#define Z_HALT_DEBUG(_text_) ::zf::Core::logError(_text_)
#endif
#endif

//! Проверка значения на NULL
#define Z_CHECK_NULL(_value_) ((_value_ != nullptr) ? static_cast<void>(0) : Z_HALT("ASSERT: Invalid NULL value"))
//! Проверка произвольного условия
#define Z_CHECK_X(_condition_, _error_text_) ((_condition_) ? static_cast<void>(0) : Z_HALT(_error_text_))
#define Z_CHECK(_condition_) Z_CHECK_X(_condition_, ::zf::utf("ASSERT: %1").arg(#_condition_))
//! Проверка результата Error
#define Z_CHECK_ERROR(_error_) ((_error_.isOk()) ? static_cast<void>(0) : Z_HALT(_error_))

// Имеется загадочная несовместимость Visual Studio и QStringLiteral для utf8
// поэтому надо использовать Z_STRING_LITERAL вместо QStringLiteral для русских строк
#ifdef Q_CC_MSVC
#define Z_STRING_LITERAL(str) QString::fromUtf8(str)
#else
#define Z_STRING_LITERAL(str) QStringLiteral(str)
#endif

//! Макро для простого создания идентификатора справочника. Пример использования: CATALOG_UID(LANGUAGE)
#define CATALOG_UID(_id_) zf::Core::catalogUid(EntityCodes::CATALOG_##_id_)

//! Идентификатор подписки на канал
Z_DECLARE_HANDLE(SubscribeHandle);

/* Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded). */
#define Z_PPCAT_NX(A, B) A##B

// Concatenate preprocessor tokens A and B after macro-expanding them.
#define Z_PPCAT(A, B) Z_PPCAT_NX(A, B)

/* Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded) */
#define Z_STRINGIZE_NX(A) #A

// Turn A into a string literal after macro-expanding it.
#define Z_STRINGIZE(A) Z_STRINGIZE_NX(A)

class View;
class Model;
class ModuleDataObject;
class DataProperty;
class DataProperty_SharedData;
class PropertyConstraint;
class PropertyLookup;
class DataStructure;
class PropertyLink;
class DataWidgets;
class DataContainer;
class PropertyID;
class EntityID;
class EntityCode;
class Uid;
class Message;
class Error;

typedef QSet<DataProperty> DataPropertySet;
typedef QList<DataProperty> DataPropertyList;
typedef QList<PropertyID> PropertyIDList;
typedef QMap<QString, PropertyID> PropertyIDMap;
typedef QMultiMap<QString, PropertyID> PropertyIDMultiMap;

typedef std::shared_ptr<DataProperty> DataPropertyPtr;
typedef std::shared_ptr<PropertyConstraint> PropertyConstraintPtr;
typedef std::shared_ptr<PropertyLink> PropertyLinkPtr;
typedef std::shared_ptr<PropertyLookup> PropertyLookupPtr;
typedef std::shared_ptr<DataStructure> DataStructurePtr;
typedef std::shared_ptr<DataWidgets> DataWidgetsPtr;
typedef std::shared_ptr<Model> ModelPtr;
typedef std::shared_ptr<View> ViewPtr;
typedef std::shared_ptr<DataContainer> DataContainerPtr;
typedef std::shared_ptr<Message> MessagePtr;

inline bool operator!=(const QMetaObject::Connection& c1, bool b)
{
    return (bool)c1 != b;
}
inline bool operator!=(bool b, const QMetaObject::Connection& c1)
{
    return (bool)c1 != b;
}

//! Функция для выполнения в рамках SyncAskDialog
typedef std::function<zf::Error()> SyncAskDialogFunction;

} // namespace zf

Q_DECLARE_METATYPE(zf::SubscribeHandle)
