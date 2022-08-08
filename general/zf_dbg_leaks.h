#pragma once

#include "zf_global.h"
#include <QMutex>

namespace zf
{
//! Класс для поиска утечки памяти
class ZCORESHARED_EXPORT DebugMemoryLeak
{
public:
    //! Создан новый объект
    static void new_object(const QString& key, bool print = false);
    //! Удален объект
    static void delete_object(const QString& key, bool print = false);
    //! Все утечки
    static QMap<QString, qint64> leaks();

private:
    static QHash<QString, qint64>* counter();
    static QMutex _mutex;
    static std::unique_ptr<QHash<QString, qint64>> _counter;
};

} // namespace zf

#ifdef Z_DEBUG_MEMORY_LEAK
#define Z_DEBUG_NEW(_key_) zf::DebugMemoryLeak::new_object(_key_)
#define Z_DEBUG_DELETE(_key_) zf::DebugMemoryLeak::delete_object(_key_)
#else
#define Z_DEBUG_NEW(_key_) while (false)
#define Z_DEBUG_DELETE(_key_) while (false)
#endif
