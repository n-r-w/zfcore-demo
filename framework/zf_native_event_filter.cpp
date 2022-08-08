#include "zf_native_event_filter.h"

#include <QByteArray>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <xcb/xcb.h>
#endif

namespace zf
{
bool NativeEventFilter::nativeEventFilter(const QByteArray& eventType, void* message, long*)
{
    Q_UNUSED(eventType)
    Q_UNUSED(message)

    return false;
}

} // namespace zf
