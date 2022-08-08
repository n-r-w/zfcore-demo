#include "zf_web_engine_injection.h"
#include "zf_core.h"

namespace zf
{
WebEngineViewInjection::WebEngineViewInjection()
{
}

void WebEngineViewInjection::onWebKeyPressed(const QString& target_id, const QString& key)
{
    Q_UNUSED(target_id)
    Q_UNUSED(key)
}

void WebEngineViewInjection::onWebMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button)
{
    Q_UNUSED(target_id)
    Q_UNUSED(modifiers)
    Q_UNUSED(button)
}

void WebEngineViewInjection::onCurrentPageChanged(int current, int maximum)
{
    Q_UNUSED(current)
    Q_UNUSED(maximum)
}

bool WebEngineViewInjection::isPdfView() const
{
    return _is_pdf_view;
}

bool WebEngineViewInjection::isHidePdfToolbar() const
{
    return isPdfView() && _is_hide_pdf_toolbar;
}

bool WebEngineViewInjection::isHidePdfZoom() const
{
    return isPdfView() && _is_hide_pdf_zoom;
}

void WebEngineViewInjection::setPdfView(bool b)
{
    if (_is_pdf_view == b)
        return;

    _is_pdf_view = b;
    emit sg_optionsChanged();
}

void WebEngineViewInjection::setHidePdfToolbar(bool b)
{
    if (_is_hide_pdf_toolbar == b)
        return;

    _is_hide_pdf_toolbar = b;
    emit sg_optionsChanged();
}

void WebEngineViewInjection::setHidePdfZoom(bool b)
{
    if (_is_hide_pdf_zoom == b)
        return;

    _is_hide_pdf_zoom = b;
    emit sg_optionsChanged();
}

void WebEngineViewInjection::onWebKeyPressed_helper(const QString& target_id, const QString& key)
{
    onWebKeyPressed(target_id, key);
    emit sg_webKeyPressed(target_id, key);
}

void WebEngineViewInjection::onWebMouseClicked_helper(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button)
{
    onWebMouseClicked(target_id, modifiers, button);
    emit sg_webMouseClicked(target_id, modifiers, button);
}

void WebEngineViewInjection::onCurrentPageChanged_helper(int current, int maximum)
{
    onCurrentPageChanged(current, maximum);
    emit sg_currentPageChanged(current, maximum);
}

} // namespace zf
