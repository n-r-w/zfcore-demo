#include "zf_utils.h"
#include <QAction>
#include <private/qaction_p.h>

namespace zf
{
//! Взлом защищенной переменной QAction
class HackedAction : public QAction
{
public:
    using QAction::d_ptr;

    //! Приватный указатель
    static QActionPrivate* privatePtr(QAction* a)
    {
        Z_CHECK_NULL(a);
        auto action = reinterpret_cast<HackedAction*>(a);
        return reinterpret_cast<QActionPrivate*>(action->d_ptr.data());
    }
    //! Проверка на доступность для любой видимости экшена
    static bool isEnabled(QAction* a)
    {
        Z_CHECK_NULL(a);
        return a->isEnabled() || !privatePtr(a)->forceDisabled;
    }
};

bool Utils::isActionEnabled(QAction* a)
{
    return HackedAction::isEnabled(a);
}

} // namespace zf
