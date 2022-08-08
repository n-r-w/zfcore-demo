#include "zf_work_zones.h"
#include "zf_core.h"

namespace zf
{
int WorkZone::id() const
{
    return _id;
}

QString WorkZone::name() const
{
    return Translator::translate(_translation_id);
}

QWidget* WorkZone::widget() const
{
    return _widget;
}

QList<QToolBar*> WorkZone::toolbars() const
{
    QList<QToolBar*> res;
    for (auto& t : _toolbars) {
        if (!t.isNull())
            res << t;
    }

    return res;
}

QIcon WorkZone::icon() const
{
    return _icon;
}

WorkZone::WorkZone(int id, const QString& translation_id, QWidget* widget, const QList<QToolBar*>& toolbars, const QIcon& icon)
    : _id(id)
    , _translation_id(translation_id)
    , _widget(widget)
    , _icon(icon)
{
    Z_CHECK(id > 0);
    Z_CHECK(!translation_id.isEmpty());

    for (auto t : toolbars) {
        Z_CHECK_NULL(t);
        _toolbars << t;
    }
}

QWidget* WorkZone::replaceWidget(QWidget* widget)
{
    QWidget* old = _widget;
    _widget = widget;
    return old;
}

QList<QToolBar*> WorkZone::replaceToolbars(const QList<QToolBar*>& toolbars)
{
    auto old = _toolbars;

    _toolbars.clear();
    for (auto t : toolbars) {
        Z_CHECK_NULL(t);
        _toolbars << t;
    }

    QList<QToolBar*> res;
    for (auto t : qAsConst(old)) {
        if (!t.isNull())
            res << t;
    }

    return res;
}

} // namespace zf
