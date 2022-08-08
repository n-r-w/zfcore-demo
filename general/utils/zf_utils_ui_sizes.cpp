#include "zf_utils.h"
#include "zf_core.h"

namespace zf
{
static void prepareWidgetScaleHelper(QWidget* w)
{
    if (w->minimumWidth() > 0)
        w->setMinimumWidth(Utils::scaleUI(w->minimumWidth()));
    if (w->minimumHeight() > 0)
        w->setMinimumHeight(Utils::scaleUI(w->minimumHeight()));

    if (w->maximumWidth() < QWIDGETSIZE_MAX)
        w->setMaximumWidth(Utils::scaleUI(w->maximumWidth()));
    if (w->maximumHeight() < QWIDGETSIZE_MAX)
        w->setMaximumHeight(Utils::scaleUI(w->maximumHeight()));

    if (!w->baseSize().isEmpty())
        w->setBaseSize(Utils::scaleUI(w->baseSize().width()), Utils::scaleUI(w->baseSize().height()));
}

//! Подгонка размеров содержимого виджета на основе текущего масштаба
void Utils::prepareWidgetScale(QWidget* w)
{
    Z_CHECK_NULL(w);

    if (w->objectName().isEmpty())
        return;

    static const QSet<const char*> TO_PREPARE_INHERIT = {
        "zf::RequestSelector", "zf::ItemSelector", "QLineEdit", "QAbstractSpinBox", "QComboBox", "QPlainTextEdit", "QTextEdit", "QDialog",
    };
    static const QSet<QString> TO_PREPARE_DIRECT = {
        "QWidget",
    };
    static const char* PREPARED_MARKER = "_zf_scale_prepared";

    if (w->property(PREPARED_MARKER).toBool())
        return;
    w->setProperty(PREPARED_MARKER, QVariant(true));

    for (auto& c : TO_PREPARE_INHERIT) {
        if (!w->inherits(c))
            continue;

        prepareWidgetScaleHelper(w);
        break;
    }

    for (auto& c : TO_PREPARE_DIRECT) {
        if (QString(w->metaObject()->className()) != c)
            continue;

        prepareWidgetScaleHelper(w);
        break;
    }
}

} // namespace zf
